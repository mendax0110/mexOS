#ifndef KERNEL_SYSCALL_NUMBERS_H
#define KERNEL_SYSCALL_NUMBERS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Syscall numbers constants
 */
#define SYS_EXIT         0
#define SYS_WRITE        1
#define SYS_READ         2
#define SYS_YIELD        3
#define SYS_GETPID       4
#define SYS_FORK         5
#define SYS_WAIT         6
#define SYS_EXEC         7
#define SYS_OPEN         8
#define SYS_CLOSE        9
#define SYS_SEND         10
#define SYS_RECV         11
#define SYS_PORT_CREATE  12
#define SYS_PORT_DESTROY 13
#define SYS_IOCTL        14
#define SYS_MMAP         15
#define SYS_GETTIME      16
#define SYS_SETTIME      17

#ifdef __cplusplus
}
#endif

#endif // KERNEL_SYSCALL_NUMBERS_H