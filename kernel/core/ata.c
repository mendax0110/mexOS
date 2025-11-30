#include "ata.h"
#include "../arch/i686/arch.h"
#include "log.h"
#include "../include/string.h"

/// @brief ATA I/O port bases \struct ata_drive
struct ata_drive
{
    bool exists;
    uint16_t base_io;
    uint16_t ctrl_io;
    uint8_t drive_select;  // 0 for master, 1 for slave
    uint32_t size;         // Size in sectors
};

static struct ata_drive drives[4];  // Primary master/slave, Secondary master/slave

/**
 * @brief Wait for the drive to be ready (not busy)
 * @param base_io Base I/O port
 * @return 0 on success, -1 on timeout
 */
static int ata_wait_bsy(const uint16_t base_io)
{
    uint32_t timeout = 100000;
    while (timeout--)
    {
        const uint8_t status = inb(base_io + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY))
        {
            return 0;
        }
    }
    return -1;  // Timeout
}

/**
 * @brief Wait for the drive to be ready for data transfer
 * @param base_io Base I/O port
 * @return 0 on success, -1 on error
 */
static int ata_wait_drq(const uint16_t base_io)
{
    uint32_t timeout = 100000;
    while (timeout--)
    {
        const uint8_t status = inb(base_io + ATA_REG_STATUS);
        if (status & ATA_SR_ERR)
        {
            return -1;  // Error
        }
        if (status & ATA_SR_DRQ)
        {
            return 0;  // Ready
        }
    }
    return -1;  // Timeout
}

/**
 * @brief Detect and identify an ATA drive
 * @param base_io Base I/O port
 * @param ctrl_io Control I/O port
 * @param drive_select 0 for master, 1 for slave
 * @return true if drive exists, false otherwise
 */
static bool ata_identify(const uint16_t base_io, const uint16_t ctrl_io, const uint8_t drive_select)
{
    (void)ctrl_io;

    //select drive
    outb(base_io + ATA_REG_DRIVE, 0xA0 | (drive_select << 4));
    io_wait();

    //identify send
    outb(base_io + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    io_wait();

    const uint8_t status = inb(base_io + ATA_REG_STATUS);
    if (status == 0)
    {
        return false;  // No drive
    }

    if (ata_wait_bsy(base_io) != 0)
    {
        return false;
    }

    if (inb(base_io + ATA_REG_LBA_MID) != 0 || inb(base_io + ATA_REG_LBA_HI) != 0)
    {
        return false;
    }

    if (ata_wait_drq(base_io) != 0)
    {
        return false;
    }

    // Read identification 512 bytes
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++)
    {
        identify_data[i] = inw(base_io + ATA_REG_DATA);
    }

    // Extract drive size (LBA28 sector count)
    const uint32_t size = ((uint32_t)identify_data[61] << 16) | identify_data[60];

    return size > 0;
}

int ata_init(void)
{
    log_info("Initializing ATA driver");

    memset(drives, 0, sizeof(drives));

    drives[0].base_io = ATA_PRIMARY_IO;
    drives[0].ctrl_io = ATA_PRIMARY_CTRL;
    drives[0].drive_select = ATA_MASTER;
    drives[0].exists = ata_identify(ATA_PRIMARY_IO, ATA_PRIMARY_CTRL, ATA_MASTER);
    if (drives[0].exists)
    {
        log_info("Primary master detected");
    }

    drives[1].base_io = ATA_PRIMARY_IO;
    drives[1].ctrl_io = ATA_PRIMARY_CTRL;
    drives[1].drive_select = ATA_SLAVE;
    drives[1].exists = ata_identify(ATA_PRIMARY_IO, ATA_PRIMARY_CTRL, ATA_SLAVE);
    if (drives[1].exists)
    {
        log_info("Primary slave detected");
    }

    drives[2].base_io = ATA_SECONDARY_IO;
    drives[2].ctrl_io = ATA_SECONDARY_CTRL;
    drives[2].drive_select = ATA_MASTER;
    drives[2].exists = ata_identify(ATA_SECONDARY_IO, ATA_SECONDARY_CTRL, ATA_MASTER);
    if (drives[2].exists)
    {
        log_info("Secondary master detected");
    }

    drives[3].base_io = ATA_SECONDARY_IO;
    drives[3].ctrl_io = ATA_SECONDARY_CTRL;
    drives[3].drive_select = ATA_SLAVE;
    drives[3].exists = ata_identify(ATA_SECONDARY_IO, ATA_SECONDARY_CTRL, ATA_SLAVE);
    if (drives[3].exists)
    {
        log_info("Secondary slave detected");
    }

    return 0;
}

int ata_read_sectors(const uint8_t drive, const uint32_t lba, const uint8_t sector_count, void* buffer)
{
    if (drive >= 4 || !drives[drive].exists)
    {
        return -1;
    }

    if (sector_count == 0)
    {
        return 0;
    }

    const struct ata_drive* d = &drives[drive];
    uint16_t* buf = (uint16_t*)buffer;

    // Wait for drive to be ready
    if (ata_wait_bsy(d->base_io) != 0)
    {
        log_info("Drive not ready for read");
        return -1;
    }

    outb(d->base_io + ATA_REG_DRIVE, 0xE0 | (d->drive_select << 4) | ((lba >> 24) & 0x0F));
    io_wait();

    outb(d->base_io + ATA_REG_SECCOUNT, sector_count);
    outb(d->base_io + ATA_REG_LBA_LO, (uint8_t)lba);
    outb(d->base_io + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(d->base_io + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));

    outb(d->base_io + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    for (int i = 0; i < sector_count; i++)
    {
        if (ata_wait_drq(d->base_io) != 0)
        {
            log_info("Error waiting for data");
            return -1;
        }

        for (int j = 0; j < 256; j++)
        {
            buf[i * 256 + j] = inw(d->base_io + ATA_REG_DATA);
        }
    }

    return 0;
}

int ata_write_sectors(const uint8_t drive, const uint32_t lba, const uint8_t sector_count, const void* buffer)
{
    if (drive >= 4 || !drives[drive].exists)
    {
        return -1;
    }

    if (sector_count == 0)
    {
        return 0;
    }

    const struct ata_drive* d = &drives[drive];
    const uint16_t* buf = (const uint16_t*)buffer;

    if (ata_wait_bsy(d->base_io) != 0)
    {
        log_info("Drive not ready for write");
        return -1;
    }

    outb(d->base_io + ATA_REG_DRIVE, 0xE0 | (d->drive_select << 4) | ((lba >> 24) & 0x0F));
    io_wait();

    outb(d->base_io + ATA_REG_SECCOUNT, sector_count);
    outb(d->base_io + ATA_REG_LBA_LO, (uint8_t)lba);
    outb(d->base_io + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(d->base_io + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));

    outb(d->base_io + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    for (int i = 0; i < sector_count; i++)
    {
        if (ata_wait_drq(d->base_io) != 0)
        {
            log_info("Error waiting for write ready");
            return -1;
        }

        for (int j = 0; j < 256; j++)
        {
            outw(d->base_io + ATA_REG_DATA, buf[i * 256 + j]);
        }
    }

    outb(d->base_io + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    if (ata_wait_bsy(d->base_io) != 0)
    {
        log_info("Cache flush timeout");
    }

    return 0;
}


bool ata_drive_exists(const uint8_t drive)
{
    if (drive >= 4)
    {
        return false;
    }
    return drives[drive].exists;
}

uint32_t ata_get_drive_size(const uint8_t drive)
{
    if (drive >= 4 || !drives[drive].exists)
    {
        return 0;
    }
    return drives[drive].size;
}
