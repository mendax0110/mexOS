#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

/**
 * @brief Kernel configuration constants
 */
#define KERNEL_STACK_SIZE   0x4000
#define USER_STACK_SIZE     0x4000
#define PAGE_SIZE           0x1000
#define MAX_PROCESSES       64
#define MAX_THREADS         256
#define MAX_PORTS           256
#define MAX_MSG_SIZE        256
#define TICK_FREQUENCY_HZ   100

#define KERNEL_CS           0x08
#define KERNEL_DS           0x10
#define USER_CS             0x1B
#define USER_DS             0x23
#define TSS_SEG             0x28

#define KERNEL_HEAP_START   0x00400000
#define KERNEL_HEAP_SIZE    0x00400000

#endif
