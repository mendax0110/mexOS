#include "network.h"
#include "arch/i686/arch.h"
#include "arch/i686/idt.h"
#include "drivers/bus/pci.h"
#include "lib/log.h"
#include "lib/string.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

static e1000_device_t g_nic;

/**
 * @brief e1000 MMIO BAR size. Not reported by this kernel's pci_get_bar(), so we map
 * a fixed upper bound generously covering every register we touch (highest offset
 * used is the multicast table at 0x5200 + 512 bytes). Real 8254x BARs are typically
 * 128KB; 0x20000 covers that with margin.
 */
#define E1000_MMIO_MAP_SIZE 0x20000

/**
 * @brief Read one word from the NIC's MMIO space at the given register offset.
 * @param reg The register offset to read from (e.g., E1000_REG_CTRL)
 * @return The read 32-bit value
 */
static uint32_t mmio_read32(const uint16_t reg)
{
    return *(volatile uint32_t*)(g_nic.mmio_base + reg);
}

/**
 * @brief Write one word to the NIC's MMIO space at the given register offset.
 * @param reg The register offset to write to (e.g., E1000_REG_CTRL)
 * @param value The 32-bit value to write
 */
static void mmio_write32(const uint16_t reg, const uint32_t value)
{
    *(volatile uint32_t*)(g_nic.mmio_base + reg) = value;
}

/**
 * @brief Allocate one physically-contiguous 4KB page for DMA use.
 * @param out_phys Receives the physical address to program into hardware registers/descriptors
 * @return CPU-dereferenceable virtual pointer to the same page, or NULL on failure
 */
static void* dma_alloc_page(uint32_t* out_phys)
{
    void* phys = pmm_alloc_block();
    if (!phys)
    {
        return NULL;
    }

    *out_phys = (uint32_t)(uintptr_t)phys;
    return phys_to_virt(*out_phys);
}

/**
 * @brief Read one word from the EEPROM via the EERD register. Used as a MAC-address
 * @param addr The address of the word to read
 * @return The read word
 * fallback on hardware where RAL0/RAH0 aren't pre-populated by firmware.
 */
static uint16_t e1000_eeprom_read(const uint8_t addr)
{
    mmio_write32(E1000_REG_EERD, E1000_EERD_START | ((uint32_t)addr << E1000_EERD_ADDR_SHIFT));

    uint32_t value = 0;
    uint32_t timeout = 100000;
    while (--timeout)
    {
        value = mmio_read32(E1000_REG_EERD);
        if (value & E1000_EERD_DONE)
        {
            break;
        }
        io_wait();
    }

    return (uint16_t)(value >> E1000_EERD_DATA_SHIFT);
}

/**
 * @brief Read the MAC address from RAL0/RAH0 if present, otherwise fall back to EEPROM.
 * Populates g_nic.mac_address[].
 */
static void e1000_read_mac_address(void)
{
    const uint32_t ral = mmio_read32(E1000_REG_RAL0);
    const uint32_t rah = mmio_read32(E1000_REG_RAH0);

    if (ral != 0 || (rah & 0xFFFF) != 0)
    {
        g_nic.mac_address[0] = (uint8_t)(ral & 0xFF);
        g_nic.mac_address[1] = (uint8_t)((ral >> 8) & 0xFF);
        g_nic.mac_address[2] = (uint8_t)((ral >> 16) & 0xFF);
        g_nic.mac_address[3] = (uint8_t)((ral >> 24) & 0xFF);
        g_nic.mac_address[4] = (uint8_t)(rah & 0xFF);
        g_nic.mac_address[5] = (uint8_t)((rah >> 8) & 0xFF);
        return;
    }

    log_warn("e1000 RAL0/RAH0 empty, falling back to EEPROM for MAC address");

    for (int i = 0; i < 3; i++)
    {
        const uint16_t word = e1000_eeprom_read((uint8_t)i);
        g_nic.mac_address[i * 2] = (uint8_t)(word & 0xFF);
        g_nic.mac_address[i * 2 + 1] = (uint8_t)(word >> 8);
    }
}

/**
 * @brief Unmask a legacy PIC IRQ line (0-15) so its interrupts reach the CPU.
 * @param irq_line
 * Mirrors the remap performed by idt_init(): IRQ0-7 -> PIC1, IRQ8-15 -> PIC2.
 */
static void e1000_unmask_irq(const uint8_t irq_line)
{
    uint16_t port = PIC1_DATA;
    uint8_t irq = irq_line;

    if (irq >= 8)
    {
        port = PIC2_DATA;
        irq -= 8;
    }

    const uint8_t mask = inb(port) & ~((uint8_t)(1 << irq));
    outb(port, mask);
}

/**
 * @brief Drain every RX descriptor the hardware has marked done, handing each
 * frame to the net_stack.
 *
 * RDT semantics: the descriptor AT index RDT is owned by software (it's the
 * boundary hardware won't write past); everything from RDH up to RDT-1 is
 * hardware's to fill. So after processing and clearing descriptor rx_cur, we
 * write RDT = rx_cur (the index we just freed) to hand that slot back to the
 * NIC, then advance rx_cur to the next one to inspect. Advancing rx_cur first
 * and writing the new value would hand back the wrong (still-boundary) slot.
 */
static void e1000_drain_rx(void)
{
    while (g_nic.rx_ring[g_nic.rx_cur].status & E1000_RXD_STATUS_DD)
    {
        const uint32_t idx = g_nic.rx_cur;
        e1000_rx_desc_t* desc = &g_nic.rx_ring[idx];

        if ((desc->status & E1000_RXD_STATUS_EOP) && desc->length > 0 && desc->length <= E1000_BUFFER_SIZE)
        {
            net_stack_handle_frame(g_nic.rx_buffer_virt[idx], desc->length);
        }
        else
        {
            log_warn_fmt("e1000 dropping fragmented/invalid RX descriptor, len=%d", desc->length);
        }

        desc->status = 0;

        g_nic.rx_cur = (idx + 1) % E1000_RX_DESC_COUNT;
        mmio_write32(E1000_REG_RDT, idx);
    }
}

/**
 * @brief Interrupt handler for the e1000 NIC. Reads the ICR to determine the cause,
 * @param regs The register state at the time of the interrupt
 */
static void e1000_irq_handler(struct registers* regs)
{
    UNUSED(regs);

    // Reading ICR both tells us the cause and clears it.
    const uint32_t cause = mmio_read32(E1000_REG_ICR);

    if (cause == 0)
    {
        return;
    }

    if (cause & (E1000_INT_RXT0 | E1000_INT_RXO))
    {
        if (cause & E1000_INT_RXO)
        {
            log_warn("e1000 receiver overrun");
        }
        e1000_drain_rx();
    }

    if (cause & E1000_INT_LSC)
    {
        const uint32_t status = mmio_read32(E1000_REG_STATUS);
        log_info_fmt("e1000 link status changed: %s", (status & (1u << 1)) ? "up" : "down");
    }
}

bool network_is_present(void)
{
    return g_nic.present;
}

bool network_get_mac_address(uint8_t* out_mac)
{
    if (!g_nic.present || !out_mac)
    {
        return false;
    }

    memcpy(out_mac, g_nic.mac_address, 6);
    return true;
}

bool network_send(const uint8_t* data, const size_t length)
{
    if (!g_nic.present || !data || length == 0 || length > E1000_BUFFER_SIZE)
    {
        return false;
    }

    const uint32_t idx = g_nic.tx_tail;
    e1000_tx_desc_t* desc = &g_nic.tx_ring[idx];

    if (desc->cmd != 0 && !(desc->status & E1000_TXD_STATUS_DD))
    {
        log_warn("e1000 TX ring full, dropping frame");
        return false;
    }

    memcpy(g_nic.tx_buffer_virt[idx], data, length);

    desc->addr = g_nic.tx_buffer_phys[idx];
    desc->length = (uint16_t)length;
    desc->cso = 0;
    desc->cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
    desc->status = 0;
    desc->css = 0;
    desc->special = 0;

    g_nic.tx_tail = (idx + 1) % E1000_TX_DESC_COUNT;
    mmio_write32(E1000_REG_TDT, g_nic.tx_tail);

    return true;
}

static struct pci_device* e1000_find_device(void)
{
    static const uint16_t known_ids[] = { E1000_KNOWN_DEVICE_IDS };

    for (size_t i = 0; i < ARRAY_SIZE(known_ids); i++)
    {
        struct pci_device* dev = pci_find_device_by_id(E1000_VENDOR_INTEL, known_ids[i]);
        if (dev)
        {
            return dev;
        }
    }

    return NULL;
}

bool network_init(void)
{
    memset(&g_nic, 0, sizeof(g_nic));

    struct pci_device* dev = e1000_find_device();
    if (!dev)
    {
        log_info("No e1000-family NIC found");
        return false;
    }

    uint8_t is_io = 0;
    const uint32_t bar0 = pci_get_bar(dev, 0, &is_io);
    if (is_io || bar0 == 0)
    {
        log_error("e1000 BAR0 is not a memory BAR, cannot continue");
        return false;
    }

    g_nic.mmio_phys = bar0;
    g_nic.mmio_size = E1000_MMIO_MAP_SIZE;
    g_nic.irq_line = dev->interrupt_line;

    g_nic.mmio_base = (volatile uint8_t*)vmm_map_temp(g_nic.mmio_phys, g_nic.mmio_size);
    if (!g_nic.mmio_base)
    {
        log_error("e1000 failed to map MMIO region");
        return false;
    }

    pci_enable_bus_mastering(dev);

    // Disable interrupts while we bring the chip up.
    mmio_write32(E1000_REG_IMC, 0xFFFFFFFF);

    // Reset, then wait for the chip to clear the bit itself.
    mmio_write32(E1000_REG_CTRL, mmio_read32(E1000_REG_CTRL) | E1000_CTRL_RST);
    uint32_t timeout = 1000000;
    while ((mmio_read32(E1000_REG_CTRL) & E1000_CTRL_RST) && --timeout)
    {
        io_wait();
    }
    if (timeout == 0)
    {
        log_error("e1000 reset timed out");
        return false;
    }

    mmio_write32(E1000_REG_IMC, 0xFFFFFFFF);
    mmio_read32(E1000_REG_ICR);

    e1000_read_mac_address();

    for (int i = 0; i < 128; i++)
    {
        mmio_write32((uint16_t)(E1000_REG_MTA + i * 4), 0);
    }

    // RX ring setup...
    uint32_t rx_ring_phys = 0;
    g_nic.rx_ring = (e1000_rx_desc_t*)dma_alloc_page(&rx_ring_phys);
    if (!g_nic.rx_ring)
    {
        log_error("e1000 failed to allocate RX descriptor ring");
        return false;
    }
    memset(g_nic.rx_ring, 0, PMM_BLOCK_SIZE);
    g_nic.rx_ring_phys = rx_ring_phys;

    for (int i = 0; i < E1000_RX_DESC_COUNT; i++)
    {
        uint32_t buf_phys = 0;
        void* buf_virt = dma_alloc_page(&buf_phys);
        if (!buf_virt)
        {
            log_error_fmt("e1000 failed to allocate RX buffer %d", i);
            return false;
        }

        g_nic.rx_buffer_virt[i] = (uint8_t*)buf_virt;
        g_nic.rx_buffer_phys[i] = buf_phys;
        g_nic.rx_ring[i].addr = buf_phys;
        g_nic.rx_ring[i].status = 0;
    }

    mmio_write32(E1000_REG_RDBAL, g_nic.rx_ring_phys);
    mmio_write32(E1000_REG_RDBAH, 0);
    mmio_write32(E1000_REG_RDLEN, E1000_RX_DESC_COUNT * sizeof(e1000_rx_desc_t));
    mmio_write32(E1000_REG_RDH, 0);
    mmio_write32(E1000_REG_RDT, E1000_RX_DESC_COUNT - 1);
    g_nic.rx_cur = 0;

    //TX ring setup ....
    uint32_t tx_ring_phys = 0;
    g_nic.tx_ring = (e1000_tx_desc_t*)dma_alloc_page(&tx_ring_phys);
    if (!g_nic.tx_ring)
    {
        log_error("e1000 failed to allocate TX descriptor ring");
        return false;
    }
    memset(g_nic.tx_ring, 0, PMM_BLOCK_SIZE);
    g_nic.tx_ring_phys = tx_ring_phys;

    for (int i = 0; i < E1000_TX_DESC_COUNT; i++)
    {
        uint32_t buf_phys = 0;
        void* buf_virt = dma_alloc_page(&buf_phys);
        if (!buf_virt)
        {
            log_error_fmt("e1000 failed to allocate TX buffer %d", i);
            return false;
        }

        g_nic.tx_buffer_virt[i] = (uint8_t*)buf_virt;
        g_nic.tx_buffer_phys[i] = buf_phys;
        g_nic.tx_ring[i].status = E1000_TXD_STATUS_DD;
    }

    mmio_write32(E1000_REG_TDBAL, g_nic.tx_ring_phys);
    mmio_write32(E1000_REG_TDBAH, 0);
    mmio_write32(E1000_REG_TDLEN, E1000_TX_DESC_COUNT * sizeof(e1000_tx_desc_t));
    mmio_write32(E1000_REG_TDH, 0);
    mmio_write32(E1000_REG_TDT, 0);
    g_nic.tx_tail = 0;

    mmio_write32(E1000_REG_TIPG, E1000_TIPG_DEFAULT);
    mmio_write32(E1000_REG_TCTL, E1000_TCTL_DEFAULT);
    mmio_write32(E1000_REG_RCTL, E1000_RCTL_DEFAULT);

    mmio_write32(E1000_REG_CTRL, mmio_read32(E1000_REG_CTRL) | E1000_CTRL_SLU | E1000_CTRL_ASDE);

    if (g_nic.irq_line < 16)
    {
        register_interrupt_handler((uint8_t)(PIC1_VECTOR_OFFSET + g_nic.irq_line), e1000_irq_handler);
        e1000_unmask_irq(g_nic.irq_line);
    }
    else
    {
        log_warn_fmt("e1000 interrupt_line %d looks invalid, IRQ not hooked", g_nic.irq_line);
    }

    mmio_write32(E1000_REG_IMS, E1000_IMS_ENABLE);

    g_nic.present = true;

    log_info_fmt("e1000 initialized: mmio=0x%x irq=%d mac=%x:%x:%x:%x:%x:%x",
                 g_nic.mmio_phys, g_nic.irq_line,
                 g_nic.mac_address[0], g_nic.mac_address[1], g_nic.mac_address[2],
                 g_nic.mac_address[3], g_nic.mac_address[4], g_nic.mac_address[5]);

    net_stack_init(NULL);
    configure_hardware_address(g_nic.mac_address);

    return true;
}