#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

#include "mexos/autoconf.h"

/**
 * @brief Kernel configuration constants
 */
#define KERNEL_STACK_SIZE   CONFIG_KERNEL_STACK_SIZE
#define USER_STACK_SIZE     CONFIG_USER_STACK_SIZE
#define MAX_PROCESSES       64
#define MAX_THREADS         256
#define MAX_PORTS           CONFIG_MAX_PORTS
#define MAX_MSG_SIZE        256
#define TICK_FREQUENCY_HZ   CONFIG_TICK_FREQUENCY_HZ

#define KERNEL_CS           0x08
#define KERNEL_DS           0x10
#define USER_CS             0x1B
#define USER_DS             0x23
#define TSS_SEG             0x28

#define KERNEL_HEAP_START   0x00400000
#define KERNEL_HEAP_SIZE    CONFIG_KERNEL_HEAP_SIZE

#define DEADCODE_MAGIC 0xDEADC0DE

#endif
