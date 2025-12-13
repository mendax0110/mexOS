#include "ahci.h"
#include "../bus/pci.h"
#include "../../lib/log.h"
#include "../../include/string.h"
#include "../../mm/heap.h"
#include "../../arch/i686/arch.h"
#include "../include/cast.h"

static int ahci_identify_device(uint8_t port, const uint16_t* buffer);

static struct hba_mem* abar = NULL;
static bool ahci_available = false;
static uint8_t port_device_type[32];
static uint64_t port_size_sectors[32];


static int ahci_check_type(const struct hba_port* port)
{
    const uint32_t ssts = port->ssts;
    const uint8_t ipm = (ssts >> 8) & 0x0F;
    const uint8_t det = ssts & 0x0F;

    if (det != 3 || ipm != 1)
    {
        return AHCI_DEV_NULL;
    }

    const uint32_t sig = port->sig;
    switch (sig)
    {
        case AHCI_SIG_ATAPI: return AHCI_DEV_SATAPI;
        case AHCI_SIG_SEMB: return AHCI_DEV_SEMB;
        case AHCI_SIG_PM: return AHCI_DEV_PM;
        default: return AHCI_DEV_SATA;
    }
}

static void ahci_stop_cmd(struct hba_port* port)
{
    port->cmd &= ~AHCI_PORT_CMD_ST;
    port->cmd &= ~AHCI_PORT_CMD_FRE;

    while (1)
    {
        if (port->cmd & AHCI_PORT_CMD_FR)
            continue;
        if (port->cmd & AHCI_PORT_CMD_CR)
            continue;
        break;
    }
}

static void ahci_start_cmd(struct hba_port* port)
{
    while (port->cmd & AHCI_PORT_CMD_CR)
        ;

    port->cmd |= AHCI_PORT_CMD_FRE;
    port->cmd |= AHCI_PORT_CMD_ST;
}

static int ahci_find_cmdslot(const struct hba_port* port)
{
    uint32_t slots = (port->sact | port->ci);
    for (int i = 0; i < 32; i++)
    {
        if ((slots & 1) == 0)
            return i;
        slots >>= 1;
    }
    return -1;
}

static void ahci_port_rebase(struct hba_port* port)
{
    ahci_stop_cmd(port);

    uint32_t clb = PTR_TO_U32(kmalloc_aligned(1024, 1024));
    port->clb = clb;
    port->clbu = 0;
    memset(PTR_FROM_U32(clb), 0, 1024);

    uint32_t fb = PTR_TO_U32(kmalloc_aligned(256, 256));
    port->fb = fb;
    port->fbu = 0;
    memset(PTR_FROM_U32(fb), 0, 256);

    struct hba_cmd_header* cmdheader = PTR_FROM_U32_TYPED(struct hba_cmd_header, port->clb);
    for (int i = 0; i < 32; i++)
    {
        cmdheader[i].prdtl = 8;

        uint32_t ctba = PTR_TO_U32(kmalloc_aligned(256, 256));
        cmdheader[i].ctba = ctba;
        cmdheader[i].ctbau = 0;
        memset(PTR_FROM_U32(ctba), 0, 256);
    }

    ahci_start_cmd(port);
}

static void ahci_probe_ports(struct hba_mem* abar)
{
    uint32_t pi = abar->pi;

    for (int i = 0; i < 32; i++)
    {
        if (pi & 1)
        {
            const int dt = ahci_check_type(&abar->ports[i]);
            port_device_type[i] = dt;

            if (dt == AHCI_DEV_SATA)
            {
                log_info_fmt("SATA drive found at port %d", i);
                ahci_port_rebase(&abar->ports[i]);

                uint16_t identify_buf[256];
                if (ahci_identify_device(i, identify_buf) == 0)
                {
                    port_size_sectors[i] = ((uint64_t)identify_buf[103] << 48) |
                                          ((uint64_t)identify_buf[102] << 32) |
                                          ((uint64_t)identify_buf[101] << 16) |
                                          identify_buf[100];

                    if (port_size_sectors[i] == 0)
                    {
                        port_size_sectors[i] = ((uint32_t)identify_buf[61] << 16) | identify_buf[60];
                    }

                    const uint32_t size_mb = (uint32_t)((port_size_sectors[i] * 512) / (1024 * 1024));
                    log_info_fmt("  Size: %u MB (%llu sectors)", size_mb, port_size_sectors[i]);
                }
            }
            else if (dt == AHCI_DEV_SATAPI)
            {
                log_info_fmt("SATAPI drive found at port %d", i);
            }
            else if (dt == AHCI_DEV_SEMB)
            {
                log_info_fmt("SEMB device found at port %d", i);
            }
            else if (dt == AHCI_DEV_PM)
            {
                log_info_fmt("Port multiplier found at port %d", i);
            }
        }

        pi >>= 1;
    }
}

int ahci_init(void)
{
    log_info("Initializing AHCI driver");

    memset(port_device_type, 0, sizeof(port_device_type));
    memset(port_size_sectors, 0, sizeof(port_size_sectors));

    const struct pci_device* pci_dev = pci_find_device_by_class(1, 6);

    if (pci_dev == NULL)
    {
        log_warn("No AHCI controller found");
        return -1;
    }

    log_info_fmt("Found AHCI controller (vendor: 0x%x, device: 0x%x)",
                 pci_dev->vendor_id, pci_dev->device_id);

    const uint32_t bar5 = pci_dev->bar[5];

    if (bar5 == 0 || bar5 == 0xFFFFFFFF)
    {
        log_error("Invalid BAR5 address");
        return -1;
    }

    abar = PTR_CAST(struct hba_mem*, bar5 & 0xFFFFFFF0);
    log_info_fmt("AHCI ABAR at 0x%x", PTR_TO_U32(abar));

    uint16_t command = pci_config_read_word(pci_dev->bus, pci_dev->device, pci_dev->function, PCI_REG_COMMAND);
    command |= 0x04;
    pci_config_write_word(pci_dev->bus, pci_dev->device, pci_dev->function, PCI_REG_COMMAND, command);

    abar->ghc |= AHCI_GHC_AHCI_EN;

    ahci_probe_ports(abar);

    ahci_available = true;
    log_info("AHCI driver initialized successfully");

    return 0;
}

static int ahci_identify_device(const uint8_t port, const uint16_t* buffer)
{
    if (!ahci_available || port >= 32)
    {
        log_error_fmt("Invalid port number %d for IDENTIFY", port);
        return -1;
    }

    if (port_device_type[port] != AHCI_DEV_SATA)
    {
        log_error_fmt("No SATA device at port %d for IDENTIFY", port);
        return -1;
    }

    struct hba_port* hba_port = &abar->ports[port];

    hba_port->is = (uint32_t)-1;

    const int slot = ahci_find_cmdslot(hba_port);
    if (slot == -1)
    {
        log_error("Cannot find free command slot");
        return -1;
    }

    struct hba_cmd_header* cmdheader = PTR_FROM_U32_TYPED(struct hba_cmd_header, hba_port->clb);
    cmdheader += slot;
    cmdheader->cfl = sizeof(struct fis_reg_h2d) / sizeof(uint32_t);
    cmdheader->w = 0;
    cmdheader->prdtl = 1;

    struct hba_cmd_tbl* cmdtbl = PTR_FROM_U32_TYPED(struct hba_cmd_tbl, cmdheader->ctba);
    memset(cmdtbl, 0, sizeof(struct hba_cmd_tbl) + sizeof(struct hba_prdt_entry) * 8);

    cmdtbl->prdt_entry[0].dba = PTR_TO_U32(buffer);
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = 512 - 1;
    cmdtbl->prdt_entry[0].i = 1;

    struct fis_reg_h2d* cmdfis = (struct fis_reg_h2d*)(&cmdtbl->cfis);
    memset(cmdfis, 0, sizeof(struct fis_reg_h2d));

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = ATA_CMD_IDENTIFY;

    uint32_t spin = 0;
    while ((hba_port->tfd & (0x80 | 0x08)) && spin < 1000000)
    {
        spin++;
    }

    if (spin == 1000000)
    {
        log_error("Port is hung");
        return -1;
    }

    hba_port->ci = (1 << slot);

    while (1)
    {
        if ((hba_port->ci & (1 << slot)) == 0)
            break;
        if (hba_port->is & (1 << 30))
        {
            log_error("IDENTIFY command failed");
            return -1;
        }
    }

    return 0;
}

int ahci_read_sectors(const uint8_t port, const uint64_t lba, uint16_t count, void* buffer)
{
    if (!ahci_available || port >= 32)
        return -1;

    if (port_device_type[port] != AHCI_DEV_SATA)
        return -1;

    struct hba_port* hba_port = &abar->ports[port];

    hba_port->is = (uint32_t)-1;

    const int slot = ahci_find_cmdslot(hba_port);
    if (slot == -1)
    {
        return -1;
    }

    struct hba_cmd_header* cmdheader = PTR_FROM_U32_TYPED(struct hba_cmd_header, hba_port->clb);
    cmdheader += slot;
    cmdheader->cfl = sizeof(struct fis_reg_h2d) / sizeof(uint32_t);
    cmdheader->w = 0;
    cmdheader->prdtl = (uint16_t)((count - 1) / 16 + 1);

    struct hba_cmd_tbl* cmdtbl = PTR_FROM_U32_TYPED(struct hba_cmd_tbl, cmdheader->ctba);
    memset(cmdtbl, 0, sizeof(struct hba_cmd_tbl) + sizeof(struct hba_prdt_entry) * cmdheader->prdtl);

    int i;
    for (i = 0; i < cmdheader->prdtl - 1; i++)
    {
        cmdtbl->prdt_entry[i].dba = PTR_TO_U32(buffer);
        cmdtbl->prdt_entry[i].dbau = 0;
        cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1;
        cmdtbl->prdt_entry[i].i = 1;
        buffer = (uint8_t*)buffer + 8 * 1024;
        count -= 16;
    }

    cmdtbl->prdt_entry[i].dba = PTR_TO_U32(buffer);
    cmdtbl->prdt_entry[i].dbau = 0;
    cmdtbl->prdt_entry[i].dbc = (count * 512) - 1;
    cmdtbl->prdt_entry[i].i = 1;

    struct fis_reg_h2d* cmdfis = (struct fis_reg_h2d*)(&cmdtbl->cfis);
    memset(cmdfis, 0, sizeof(struct fis_reg_h2d));

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = ATA_CMD_READ_DMA_EX;

    cmdfis->lba0 = (uint8_t)lba;
    cmdfis->lba1 = (uint8_t)(lba >> 8);
    cmdfis->lba2 = (uint8_t)(lba >> 16);
    cmdfis->device = 1 << 6;

    cmdfis->lba3 = (uint8_t)(lba >> 24);
    cmdfis->lba4 = (uint8_t)(lba >> 32);
    cmdfis->lba5 = (uint8_t)(lba >> 40);

    cmdfis->count = count;

    uint32_t spin = 0;
    while ((hba_port->tfd & (0x80 | 0x08)) && spin < 1000000)
    {
        spin++;
    }

    if (spin == 1000000)
    {
        return -1;
    }

    hba_port->ci = (1 << slot);

    while (1)
    {
        if ((hba_port->ci & (1 << slot)) == 0)
            break;
        if (hba_port->is & (1 << 30))
        {
            return -1;
        }
    }

    return 0;
}

int ahci_write_sectors(const uint8_t port, const uint64_t lba, uint16_t count, const void* buffer)
{
    if (!ahci_available || port >= 32)
        return -1;

    if (port_device_type[port] != AHCI_DEV_SATA)
        return -1;

    struct hba_port* hba_port = &abar->ports[port];

    hba_port->is = (uint32_t)-1;

    const int slot = ahci_find_cmdslot(hba_port);
    if (slot == -1)
    {
        return -1;
    }

    struct hba_cmd_header* cmdheader = PTR_FROM_U32_TYPED(struct hba_cmd_header, hba_port->clb);
    cmdheader += slot;
    cmdheader->cfl = sizeof(struct fis_reg_h2d) / sizeof(uint32_t);
    cmdheader->w = 1;
    cmdheader->prdtl = (uint16_t)((count - 1) / 16 + 1);

    struct hba_cmd_tbl* cmdtbl = PTR_FROM_U32_TYPED(struct hba_cmd_tbl, cmdheader->ctba);
    memset(cmdtbl, 0, sizeof(struct hba_cmd_tbl) + sizeof(struct hba_prdt_entry) * cmdheader->prdtl);

    int i;
    for (i = 0; i < cmdheader->prdtl - 1; i++)
    {
        cmdtbl->prdt_entry[i].dba = PTR_TO_U32(buffer);
        cmdtbl->prdt_entry[i].dbau = 0;
        cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1;
        cmdtbl->prdt_entry[i].i = 1;
        buffer = (const uint8_t*)buffer + 8 * 1024;
        count -= 16;
    }

    cmdtbl->prdt_entry[i].dba = PTR_TO_U32(buffer);
    cmdtbl->prdt_entry[i].dbau = 0;
    cmdtbl->prdt_entry[i].dbc = (count * 512) - 1;
    cmdtbl->prdt_entry[i].i = 1;

    struct fis_reg_h2d* cmdfis = (struct fis_reg_h2d*)(&cmdtbl->cfis);
    memset(cmdfis, 0, sizeof(struct fis_reg_h2d));

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = ATA_CMD_WRITE_DMA_EX;

    cmdfis->lba0 = (uint8_t)lba;
    cmdfis->lba1 = (uint8_t)(lba >> 8);
    cmdfis->lba2 = (uint8_t)(lba >> 16);
    cmdfis->device = 1 << 6;

    cmdfis->lba3 = (uint8_t)(lba >> 24);
    cmdfis->lba4 = (uint8_t)(lba >> 32);
    cmdfis->lba5 = (uint8_t)(lba >> 40);

    cmdfis->count = count;

    uint32_t spin = 0;
    while ((hba_port->tfd & (0x80 | 0x08)) && spin < 1000000)
    {
        spin++;
    }

    if (spin == 1000000)
    {
        return -1;
    }

    hba_port->ci = (1 << slot);

    while (1)
    {
        if ((hba_port->ci & (1 << slot)) == 0)
            break;
        if (hba_port->is & (1 << 30))
        {
            return -1;
        }
    }

    return 0;
}

bool ahci_port_exists(const uint8_t port)
{
    if (!ahci_available || port >= 32)
    {
        log_warn_fmt("Invalid port number %d for existence check", port);
        return false;
    }

    return port_device_type[port] == AHCI_DEV_SATA;
}

uint64_t ahci_get_port_size(const uint8_t port)
{
    if (!ahci_available || port >= 32)
    {
        log_warn_fmt("Invalid port number %d for size query", port);
        return 0;
    }

    if (port_device_type[port] != AHCI_DEV_SATA)
    {
        log_warn_fmt("No SATA device at port %d for size query", port);
        return 0;
    }

    return port_size_sectors[port];
}
