#ifndef KERNEL_CORE_POWER_H
#define KERNEL_CORE_POWER_H

#include "../../shared/types.h"

/**
 * @brief Shutdown the system
 */
NORETURN void kernel_power_shutdown(void);

/**
 * @brief Reboot the system
 */
NORETURN void kernel_power_reboot(void);

#endif
