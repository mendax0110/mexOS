#ifndef KERNEL_DISK_INSTALLER_H
#define KERNEL_DISK_INSTALLER_H

#include "../include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Show interactive installer dialog for disk selection
 * @return Selected drive number (0-3), or -1 if user cancels/no disks
 */
int disk_installer_dialog(void);

#ifdef __cplusplus
}
#endif

#endif // KERNEL_DISK_INSTALLER_H
