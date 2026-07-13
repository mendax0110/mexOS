#ifndef SHARED_SYSCALL_NUMBERS_H
#define SHARED_SYSCALL_NUMBERS_H

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
#define SYS_READDIR      10
#define SYS_STAT         11
#define SYS_CHDIR        12
#define SYS_GETCWD       13
#define SYS_SEND         14
#define SYS_RECV         15
#define SYS_PORT_CREATE  16
#define SYS_PORT_DESTROY 17
#define SYS_IOCTL        18
#define SYS_MMAP         19
#define SYS_GETTIME      20
#define SYS_SETTIME      21
#define SYS_SHELL_EXEC   22
#define SYS_POLL_KEY     23
#define SYS_POLL_MOUSE   24

#ifdef __cplusplus
}
#endif

#endif // SHARED_SYSCALL_NUMBERS_H
