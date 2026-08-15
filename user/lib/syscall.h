#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#include "../shared/types.h"
#include "../shared/syscall_numbers.h"
#include "../shared/fs_abi.h"
#include "../shared/video_abi.h"
#include "../shared/process_abi.h"
#include "../shared/pty_abi.h"
#include "../shared/system_abi.h"
#include "../shared/io_abi.h"
#include "../shared/asm.h"

/**
 * @brief Maximum message size for IPC
 */
#define MAX_MSG_SIZE 256

/**
 * @brief IPC flags
 */
#define IPC_BLOCK    0x01
#define IPC_NONBLOCK 0x02

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define O_RDONLY 0x01
#define O_WRONLY 0x02
#define O_RDWR   (O_RDONLY | O_WRONLY)

/**
 * @brief Struct representing the rct_time \struct rtc_time
 */
struct rtc_time
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t weekday;
};

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
    ASM_V("int $0x80" : "=a"(ret) : "a"(num));
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
    ASM_V("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1));
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
    ASM_V("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2));
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
    ASM_V("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3));
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
 * @brief Write data to a file descriptor
 * @param fd The file descriptor
 * @param str The buffer to write
 * @param len The length of the string
 * @return The number of bytes written
 */
static inline int write(const int fd, const void* str, const int len)
{
    return syscall3(SYS_WRITE, fd, (int)str, len);
}

/**
 * @brief Read data from a file descriptor
 * @param fd The file descriptor
 * @param buf The buffer to read into
 * @param len The maximum number of bytes to read
 * @return The number of bytes read
 */
static inline int read(const int fd, void* buf, const int len)
{
    return syscall3(SYS_READ, fd, (int)buf, len);
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
 * @brief Wait for a specific child process to exit with options
 * @param pid The PID to wait for
 * @param status The status of the PID
 * @param flags Message flags
 * @return
 */
static inline int waitpid(const int pid, int* status, const int flags)
{
    return syscall3(SYS_WAITPID, pid, (int)status, flags);
}

/**
 * @brief Kill a given PID
 * @param pid
 * @param status
 * @return
 */
static inline int kill(const int pid, const int status)
{
    return syscall2(SYS_KILL, pid, status);
}

/**
 * @brief Execute a program
 * @param path Path to the executable
 * @param argv Argument vector
 * @return Does not return on success, -1 on error
 */
static inline int execv(const char* path, const char* const argv[])
{
    int argc = 0;
    if (argv)
    {
        while (argv[argc])
        {
            argc++;
        }
    }

    return syscall3(SYS_EXEC, (int)path, (int)argv, argc);
}

/**
 * @brief Execute a program with argv[0] defaulted to the path
 * @param path Path to the executable
 * @return Does not return on success, -1 on error
 */
static inline int exec(const char* path)
{
    const char* argv[] = { path, NULL };
    return execv(path, argv);
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
 * @brief Read structured directory entries
 * @param path The directory path
 * @param entries The destination entry array
 * @param max_entries Maximum number of entries to read
 * @return Number of entries read, or -1 on error
 */
static inline int readdir(const char* path, struct fs_dirent* entries, const int max_entries)
{
    return syscall3(SYS_READDIR, (int)path, (int)entries, max_entries);
}

/**
 * @brief Get metadata for a file or directory
 * @param path The path to inspect
 * @param info The destination metadata buffer
 * @return 0 on success, or -1 on error
 */
static inline int stat(const char* path, struct fs_stat* info)
{
    return syscall2(SYS_STAT, (int)path, (int)info);
}

/**
 * @brief Change the current working directory
 * @param path Target directory
 * @return 0 on success, or -1 on error
 */
static inline int chdir(const char* path)
{
    return syscall1(SYS_CHDIR, (int)path);
}

/**
 * @brief Copy the current working directory into a buffer
 * @param buffer Destination buffer
 * @param size Size of the destination buffer
 * @return Number of bytes copied, or -1 on error
 */
static inline int getcwd(char* buffer, const int size)
{
    return syscall2(SYS_GETCWD, (int)buffer, size);
}

/**
 * @brief Execute a kernel-backed shell command line
 * @param line Full command line
 * @return 0 on success, or -1 on error
 */
static inline int shell_exec(const char* line)
{
    return syscall1(SYS_SHELL_EXEC, (int)line);
}

/**
 * @brief Poll one keyboard/serial input byte without blocking
 * @param key Destination byte
 * @return 1 if a key was read, 0 if no key is pending, or -1 on error
 */
static inline int poll_key(unsigned char* key)
{
    return syscall1(SYS_POLL_KEY, (int)key);
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

/**
 * @brief Poll current mouse position and button state wihtout blocking
 * @param state Destination mouse state structure
 * @return 1 on success, or negative on error
 */
static inline int poll_mouse(struct mouse_state* state)
{
    return syscall1(SYS_POLL_MOUSE, (int)state);
}

/**
 * @brief Map anonymous zeroed memory into the calling process address space
 * @param size Number of bytes requrested
 * @return Virtual address of the mapped region, or NULL on failure
 */
static inline void* mmap_anon(const size_t size)
{
    return (void*)syscall1(SYS_MMAP_ANON, size);
}

/**
 * @brief Create a new pseudo-terminal master
 * @return File descriptor of the new master, or -1 on error
 */
static inline int pty_create(void)
{
    return syscall0(SYS_PTY_CREATE);
}

/**
 * @brief Attach a pseudo-terminal slave to a master
 * @param id The master pseudo-terminal ID
 * @return 0 on success, or -1 on error
 */
static inline int pty_attach_slave(const int id)
{
    return syscall2(SYS_PTY_ATTACH, id, PTY_ATTACH_SLAVE);
}

/**
 * @brief Detach a pseudo-terminal slave from a master
 * @param id The master pseudo-terminal ID
 * @param buffer The buffer to read from the slave
 * @param size The number of bytes to read
 * @return 0 on success, or -1 on error
 */
static inline int pty_read(const int id, void* buffer, const int size)
{
    return syscall3(SYS_PTY_READ, id, (int)buffer, size);
}

/**
 * @brief Write data to a pseudo-terminal master
 * @param id The master pseudo-terminal ID
 * @param buffer The data to write
 * @param size The number of bytes to write
 * @return Number of bytes written, or -1 on error
 */
static inline int pty_write(const int id, const void* buffer, const int size)
{
    return syscall3(SYS_PTY_WRITE, id, (int)buffer, size);
}

/**
 * @brief Destroy a pseudo-terminal
 * @param id The pseudo-terminal ID
 * @return 0 on success, or -1 on error
 */
static inline int pty_destroy(const int id)
{
    return syscall1(SYS_PTY_DESTROY, id);
}

/**
 * @brief Claim the display server port
 * @return The display server port ID, or -1 on error
 */
static inline int display_claim(void)
{
    return syscall0(SYS_DISPLAY_CLAIM);
}

/**
 * @brief Power control
 * @param action The power action to perform
 * @return 0 on success, or -1 on error
 */
static inline int power_control(const int action)
{
    return syscall1(SYS_POWER, action);
}

/**
 * @brief Get the list of processes
 * @param output The destination array for process info
 * @param capacity The maximum number of entries to write
 * @return The number of processes written, or -1 on error
 */
static inline int getprocs(struct process_info* output, const int capacity)
{
    return syscall2(SYS_GETPROCS, (int)output, capacity);
}

/**
 * @brief Get the user ID of the current process
 * @return The user ID
 */
static inline int getuid(void)
{
    return syscall0(SYS_GETUID);
}

/**
 * @brief File system mutation operations (create, delete, etc.)
 * @param operation The mutation operation to perform
 * @param path The path to the file or directory to mutate
 * @return 0 on success, or -1 on error
 */
static inline int fs_mutate(const int operation, const char* path)
{
    return syscall2(SYS_FS_MUTATE, operation, (int)path);
}

/**
 * @brief Get the uptime in ticks
 * @return The number of ticks since system start
 */
static inline int uptime_ticks(void)
{
    return syscall0(SYS_UPTIME);
}

/**
 * @brief Get system information
 * @param info The destination structure for system info
 * @return 0 on success, or -1 on error
 */
static inline int sysinfo(struct system_info* info)
{
    return syscall1(SYS_SYSINFO, (int)info);
}

/**
 * @brief Create a shared memory segment
 * @param size The size of the shared memory segment
 * @return The ID of the created shared memory segment, or -1 on error
 */
static inline int shm_create(const size_t size)
{
    return syscall1(SYS_SHM_CREATE, size);
}

/**
 * @brief Map a shared memory segment into the calling process address space
 * @param id The ID of the shared memory segment to map
 * @return Virtual address of the mapped region, or NULL on failure
 */
static inline void* shm_map(const int id)
{
    return (void*)syscall1(SYS_SHM_MAP, id);
}

/**
 * @brief Detach a shared memory segment from the calling process address space
 * @param id The ID of the shared memory segment to detach
 * @return 0 on success, or -1 on error
 */
static inline int shm_detach(const int id)
{
    return syscall1(SYS_SHM_DETACH, id);
}

/**
 * @brief Destroy a shared memory segment
 * @param id The ID of the shared memory segment to destroy
 * @return 0 on success, or -1 on error
 */
static inline int shm_destroy(const int id)
{
    return syscall1(SYS_SHM_DESTROY, id);
}

/**
 * @brief Create a pipe
 * @param fds An array of two integers to hold the read and write file descriptors
 * @return 0 on success, or -1 on error
 */
static inline int pipe(int fds[2])
{
    return syscall1(SYS_PIPE, (int)fds);
}

/**
 * @brief Duplicate a file descriptor to a new file descriptor
 * @param old_fd The old file descriptor to duplicate
 * @param new_fd The new file descriptor to duplicate to
 * @return 0 on success, or -1 on error
 */
static inline int dup2(const int old_fd, const int new_fd)
{
    return syscall2(SYS_DUP2, old_fd, new_fd);
}

/**
 * @brief Poll a file descriptor for events
 * @param fd The file descriptor to poll
 * @param events The events to poll for (e.g., read, write)
 * @return 0 if no events, >0 if events occurred, -1 on error
 */
static inline int poll_fd(const int fd, const int events)
{
    return syscall2(SYS_POLL_FD, fd, events);
}

/**
 * @brief Set the process group ID of a process
 * @param pid The PID of the process
 * @param pgid The new process group ID
 * @return 0 on success, or -1 on error
 */
static inline int setpgid(const int pid, const int pgid)
{
    return syscall2(SYS_SETPGID, pid, pgid);
}

/**
 * @brief Get the process group ID of a process
 * @param pid The PID of the process to query
 * @return The process group ID, or -1 on error
 */
static inline int getpgid(const int pid)
{
    return syscall1(SYS_GETPGID, pid);
}

/**
 * @brief Unmap a memory region from the calling process address space
 * @param address The starting address of the region to unmap
 * @param size The size of the region to unmap
 * @return 0 on success, or -1 on error
 */
static inline int munmap(void* address, const size_t size)
{
    return syscall2(SYS_MUNMAP, (int)address, size);
}

#endif
