#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#include "types.h"
#include "shared/syscall_numbers.h"

/**
 * @brief Maximum message size for IPC
 */
#define MAX_MSG_SIZE 256

/**
 * @brief IPC flags
 */
#define IPC_BLOCK    0x01
#define IPC_NONBLOCK 0x02

/**
 * @brief IPC message structure for user-space \struct message
 */
struct message
{
    pid_t    sender;
    pid_t    receiver;
    uint32_t type;
    uint32_t len;
    uint8_t  data[MAX_MSG_SIZE];
};

/**
 * @brief Perform a system call with 0 arguments
 * @param num The system call number
 * @return The return value of the system call
 */
static inline int syscall0(int num)
{
    int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(num));
    return ret;
}

/**
 * @brief Perform a system call with 1 argument
 * @param num The system call number
 * @param arg1 The first argument
 * @return The return value of the system call
 */
static inline int syscall1(int num, int arg1)
{
    int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1));
    return ret;
}

/**
 * @brief Perform a system call with 2 arguments
 * @param num The system call number
 * @param arg1 The first argument
 * @param arg2 The second argument
 * @return The return value of the system call
 */
static inline int syscall2(int num, int arg1, int arg2)
{
    int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2));
    return ret;
}

/**
 * @brief Perform a system call with 3 arguments
 * @param num The system call number
 * @param arg1 The first argument
 * @param arg2 The second argument
 * @param arg3 The third argument
 * @return The return value of the system call
 */
static inline int syscall3(int num, int arg1, int arg2, int arg3)
{
    int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

/**
 * @brief Exit the current process
 * @param code The exit code
 */
static inline void exit(const int code)
{
    syscall1(SYS_EXIT, code);
}

/**
 * @brief Write data to the standard output
 * @param str The string to write
 * @param len The length of the string
 * @return The number of bytes written
 */
static inline int write(const char* str, const int len)
{
    return syscall2(SYS_WRITE, (int)str, len);
}

/**
 * @brief Read data from the standard input
 * @param buf The buffer to read into
 * @param len The maximum number of bytes to read
 * @return The number of bytes read
 */
static inline int read(char* buf, const int len)
{
    return syscall2(SYS_READ, (int)buf, len);
}

/**
 * @brief Yield the CPU to other processes
 */
static inline void yield(void)
{
    syscall0(SYS_YIELD);
}

/**
 * @brief Get the process ID of the current process
 * @return The process ID
 */
static inline int getpid(void)
{
    return syscall0(SYS_GETPID);
}

/**
 * @brief Send a message to a port
 * @param port The port to send the message to
 * @param msg The message to send
 * @param flags Message flags
 * @return 0 on success, or a negative error code
 */
static inline int send(const int port, struct message* msg, const int flags)
{
    return syscall3(SYS_SEND, port, (int)msg, flags);
}

/**
 * @brief Receive a message from a port
 * @param port The port to receive the message from
 * @param msg The message buffer to receive into
 * @param flags Message flags
 * @return 0 on success, or a negative error code
 */
static inline int recv(const int port, struct message* msg, const int flags)
{
    return syscall3(SYS_RECV, port, (int)msg, flags);
}

/**
 * @brief Fork the current process
 * @return Child PID in parent, 0 in child, -1 on error
 */
static inline int fork(void)
{
    return syscall0(SYS_FORK);
}

/**
 * @brief Wait for a child process to exit
 * @param pid Child PID to wait for, or -1 for any child
 * @param status Pointer to store exit status
 * @return PID of exited child, or -1 on error
 */
static inline int wait(const int pid, int* status)
{
    return syscall2(SYS_WAIT, pid, (int)status);
}

/**
 * @brief Execute a program
 * @param path Path to the executable
 * @return Does not return on success, -1 on error
 */
static inline int exec(const char* path)
{
    return syscall1(SYS_EXEC, (int)path);
}

/**
 * @brief Create a new port
 * @return Port ID on success, or -1 on error
 */
static inline int port_create(void)
{
    return syscall0(SYS_PORT_CREATE);
}

/**
 * @brief Destroy a port
 * @param port The port ID to destroy
 * @return 0 on success, or -1 on error
 */
static inline int port_destroy(const int port)
{
    return syscall1(SYS_PORT_DESTROY, port);
}

/**
 * @brief Open a file
 * @param path The path to the file
 * @param flags The flags for opening the file
 * @return The file descriptor on success, or -1 on error
 */
static inline int open(const char* path, const int flags)
{
    return syscall2(SYS_OPEN, (int)path, flags);
}

/**
 * @brief Closes a file descriptor
 * @param port The port (fd)
 * @return 0 on success, or -1 on error
 */
static inline int close(const int port)
{
    return syscall1(SYS_CLOSE, port);
}

/**
 * @brief Perform an I/O control operation
 * @param device The device to control
 * @param request The control request
 * @param argp The argument for the control request
 * @return 0 on success, or -1 on error
 */
static inline int ioctl(const int device, const int request, void* argp)
{
    return syscall3(SYS_IOCTL, device, request, (int)argp);
}

/**
 * @brief Map a framebuffer into user space
 * @param info The info vvoid ptr
 * @return 0 on success, or -1 on error
 */
static inline void* mmap_fb(void* info)
{
    return (void*)syscall1(SYS_MMAP, (int)info);
}

/**
 * @brief Getter for the time
 * @param time The time
 * @return 0 on success, or -1 on error
 */
static inline int gettime(struct rtc_time* time)
{
    return syscall1(SYS_GETTIME, (int)time);
}

/**
 * @brief Setter for the time
 * @param time The time
 * @return 0 on success, or -1 on error
 */
static inline int settime(const struct rtc_time* time)
{
    return syscall1(SYS_SETTIME, (int)time);
}

#endif
