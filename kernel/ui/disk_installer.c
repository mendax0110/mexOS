#include "disk_installer.h"
#include "../../servers/console/console.h"
#include "../../servers/input/keyboard.h"
#include "../../servers/block/ata.h"
#include "../../servers/block/ahci.h"
#include "../../servers/vfs/diskfs.h"
#include "../include/string.h"

int disk_installer_dialog(void)
{
    uint8_t drive_exists[36];
    uint32_t drive_size_mb[36];
    bool has_diskfs[36];
    const char* drive_type[36];
    int num_disks = 0;


    for (uint8_t drive = 0; drive < 4; drive++)
    {
        drive_exists[drive] = ata_drive_exists(drive);

        if (drive_exists[drive])
        {
            num_disks++;
            const uint32_t size_sectors = ata_get_drive_size(drive);
            drive_size_mb[drive] = (size_sectors * 512) / (1024 * 1024);
            drive_type[drive] = "IDE";

            uint8_t sector_buf[512];
            has_diskfs[drive] = false;
            if (ata_read_sectors(drive, 0, 1, sector_buf) == 0)
            {
                const uint32_t* magic = (uint32_t*)sector_buf;
                if (*magic == 0x6D786673U)
                {
                    has_diskfs[drive] = true;
                }
            }
        }
    }

    for (uint8_t port = 0; port < 32; port++)
    {
        const uint8_t idx = 4 + port;
        drive_exists[idx] = ahci_port_exists(port);

        if (drive_exists[idx])
        {
            num_disks++;
            const uint64_t size_sectors = ahci_get_port_size(port);
            drive_size_mb[idx] = (uint32_t)((size_sectors * 512) / (1024 * 1024));
            drive_type[idx] = "SATA";

            uint8_t sector_buf[512];
            has_diskfs[idx] = false;
            if (ahci_read_sectors(port, 0, 1, sector_buf) == 0)
            {
                const uint32_t* magic = (uint32_t*)sector_buf;
                if (*magic == DISKFS_MAGIC)  // <DISKFS_MAGIC
                {
                    has_diskfs[idx] = true;
                }
            }
        }
    }

    if (num_disks == 0)
    {
        console_clear();
        console_set_color(0x0C, 0x00);
        console_write("No ATA drives detected!\n");
        console_set_color(0x07, 0x00);
        console_write("Continue in RAM-only mode...\n");
        for (volatile int i = 0; i < 50000000; i++);
        return -1;
    }

    console_clear();
    console_set_color(0x0F, 0x01);
    console_write("mexOS Disk Installer\n");
    console_set_color(0x07, 0x00);
    console_write("\n");

    console_write("Available disks:\n\n");

    int drive_num = 0;
    for (int i = 0; i < 36; i++)
    {
        if (!drive_exists[i])
        {
            continue;
        }

        console_set_color(0x0E, 0x00);
        console_write("  [");
        console_putchar('0' + drive_num);
        console_write("] ");
        console_write(drive_type[i]);
        console_write(" Drive: ");

        char size_buf[16];
        console_write(itoa(drive_size_mb[i], size_buf, 10));
        console_write(" MB");

        if (has_diskfs[i])
        {
            console_set_color(0x0A, 0x00);
            console_write(" [Has mexFS]");
        }
        else
        {
            console_set_color(0x08, 0x00);
            console_write(" [Unformatted]");
        }

        console_set_color(0x07, 0x00);
        console_write("\n");
        drive_num++;
    }

    console_write("\n");
    console_write("Options:\n");
    console_write("  [0-3] - Select drive and mount (will format if needed)\n");
    console_write("  [S]   - Skip disk support (RAM-only mode)\n");
    console_write("  [R]   - Rescan drives\n");
    console_write("\n");
    console_write("Your choice: ");

    while (1)
    {
        const uint8_t key = keyboard_getchar();

        if (key >= '0' && key <= '3')
        {
            const uint8_t drive = key - '0';

            if (!drive_exists[drive])
            {
                console_set_color(0x0C, 0x00);
                console_write("\n\nDrive does not exist!\n");
                console_set_color(0x07, 0x00);
                console_write("Press any key...");
                keyboard_getchar();
                return disk_installer_dialog();
            }

            console_putchar(key);
            console_write("\n\n");

            if (has_diskfs[drive])
            {
                console_set_color(0x0A, 0x00);
                console_write("Mounting existing mexFS on drive ");
                console_putchar(key);
                console_write("...\n");
                console_set_color(0x07, 0x00);

                if (diskfs_mount(drive) == 0)
                {
                    console_write("Mount successful!\n");
                    keyboard_getchar();
                    return drive;
                }
                else
                {
                    console_set_color(0x0C, 0x00);
                    console_write("Mount failed!\n");
                    console_set_color(0x07, 0x00);
                    keyboard_getchar();
                    return -1;
                }
            }
            else
            {
                console_set_color(0x0E, 0x00);
                console_write("Drive ");
                console_putchar(key);
                console_write(" is not formatted. Format now? [Y/N]: ");
                console_set_color(0x07, 0x00);

                const uint8_t confirm = keyboard_getchar();
                console_putchar(confirm);
                console_write("\n");

                if (confirm == 'y' || confirm == 'Y')
                {
                    console_write("Formatting drive ");
                    console_putchar(key);
                    console_write("...\n");

                    if (diskfs_format(drive) == 0 && diskfs_mount(drive) == 0)
                    {
                        console_set_color(0x0A, 0x00);
                        console_write("Format and mount successful!\n");
                        console_set_color(0x07, 0x00);
                        keyboard_getchar();
                        return drive;
                    }
                    else
                    {
                        console_set_color(0x0C, 0x00);
                        console_write("Format failed!\n");
                        console_set_color(0x07, 0x00);
                        keyboard_getchar();
                        return -1;
                    }
                }
                else
                {
                    return disk_installer_dialog();
                }
            }
        }
        else if (key == 's' || key == 'S')
        {
            console_putchar(key);
            console_write("\n\n");
            console_set_color(0x08, 0x00);
            console_write("Skipping disk support. Running in RAM-only mode.\n");
            console_set_color(0x07, 0x00);
            keyboard_getchar();
            return -1;
        }
        else if (key == 'r' || key == 'R')
        {
            return disk_installer_dialog();
        }
    }
}
