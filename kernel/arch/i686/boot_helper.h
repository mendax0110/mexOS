#ifndef ARCH_I686_BOOT_HELPER_H
#define ARCH_I686_BOOT_HELPER_H

#include "../../../shared/types.h"
#include "../../include/addr.h"
#include "../../include/source_location.h"
#include "../../lib/string.h"
#include "../../lib/log.h"
#include "../../lib/debug_utils.h"
#include "../../apps/disk_installer.h"
#include "../../ui/console.h"
#include "../../fs/fs.h"
#include "../../drivers/storage/ata.h"
#include "../../drivers/storage/ahci.h"

#define MULTIBOOT_INFO_CMDLINE  (1U << 2)

/**
 * @brief Check for a whitespace-delimited token in the Multiboot command line.
 */
static bool boot_has_option(const uint32_t mboot_info, const char* option)
{
    if (!mboot_info || !option || !*option)
    {
        return false;
    }

    const uint32_t* info = PTR_FROM_U32_TYPED(const uint32_t, mboot_info);
    if (!(info[0] & MULTIBOOT_INFO_CMDLINE) || info[4] == 0)
    {
        return false;
    }

    const char* command_line = PTR_FROM_U32_TYPED(const char, info[4]);
    const size_t option_length = strlen(option);

    while (*command_line)
    {
        while (*command_line == ' ' || *command_line == '\t')
        {
            command_line++;
        }

        const char* token = command_line;
        while (*command_line && *command_line != ' ' && *command_line != '\t')
        {
            command_line++;
        }

        if ((size_t)(command_line - token) == option_length &&
            strncmp(token, option, option_length) == 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Scan for storage drives and invoke the disk installer if any are found.
 * If no drives are found, the system will continue in RAM-only mode.
 */
static void scan_drives(void)
{
    bool has_drives = false;
    for (uint8_t i = 0; i < 4; i++)
    {
        if (ata_drive_exists(i))
        {
            has_drives = true;
            break;
        }
    }

    if (!has_drives)
    {
        for (uint8_t i = 0; i < 32; i++)
        {
            if (ahci_port_exists(i))
            {
                has_drives = true;
                break;
            }
        }
    }

    if (has_drives)
    {
        console_write("[boot] Starting disk installer...\n");
        const int selected_drive = disk_installer_dialog();

        if (selected_drive >= 0)
        {
            if (fs_enable_disk((uint8_t)selected_drive) == 0)
            {
                log_info_fmt("Persistent filesystem enabled on drive %d", selected_drive);
                console_clear();
                log_load("/var/log/kernel.log");
            }
            else
            {
                log_warn("Failed to enable disk filesystem, using RAM-only mode");
            }
        }
        else
        {
            log_info("Running in RAM-only filesystem mode");
            console_clear();
        }
    }
    else
    {
        console_write("[boot] No storage drives detected\n");
        console_write("[boot] Continuing in RAM-only mode...\n");
        log_warn("No ATA drives found, using RAM-only filesystem");
        DEBUG_BUSY_WAIT_LOG(50000000);
    }
}

#endif