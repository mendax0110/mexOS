#ifndef SHARED_SYSCALL_NUMBERS_H
#define SHARED_SYSCALL_NUMBERS_H

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
#define SYS_MMAP_ANON    25
#define SYS_WAITPID      26
#define SYS_KILL         27
#define SYS_PTY_CREATE   28
#define SYS_PTY_ATTACH   29
#define SYS_PTY_READ     30
#define SYS_PTY_WRITE    31
#define SYS_PTY_DESTROY  32
#define SYS_DISPLAY_CLAIM 33
#define SYS_POWER        34
#define SYS_GETPROCS     35
#define SYS_GETUID       36
#define SYS_FS_MUTATE    37
#define SYS_UPTIME       38
#define SYS_SYSINFO      39
#define SYS_SHM_CREATE   40
#define SYS_SHM_MAP      41
#define SYS_SHM_DETACH   42
#define SYS_SHM_DESTROY  43
#define SYS_PIPE         44
#define SYS_DUP2         45
#define SYS_POLL_FD      46
#define SYS_SETPGID      47
#define SYS_GETPGID      48
#define SYS_MUNMAP       49

#endif // SHARED_SYSCALL_NUMBERS_H
