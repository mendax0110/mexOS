#pragma once

#include "dataTypes.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

/// @brief Enumeration of system call numbers. \enum Syscalls
enum Syscalls
{
    SYS_WRITE = 0,
    SYS_READ,
    SYS_GET_TASK_COUNT,
    SYS_GET_TASK_INFO,
    SYS_GET_VERSION,
    SYS_YIELD
};


/**
 * @brief Structure representing the context of a process.
 */
typedef struct
{
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t user_esp;
    uint32_t ss;
} ProcessContext;


/// @brief Class representing a VGA terminal for output. \class VGATerminal
class VGATerminal
{
public:

    /**
     * @brief Initializes the VGA terminal.
     */
    void initialize();

    /**
     * @brief Writes a string to the VGA terminal.
     * @param str The string to write.
     */
    void write(const char* str);

    /**
     * @brief Writes a character to the VGA terminal.
     * @param c The character to write.
     */
    void putchar(char c);

private:
    uint16_t* buffer;
    uint32_t row;
    uint32_t column;
    uint8_t color;

    volatile uint32_t lock = 0;

    /**
     * @brief Spins until the lock is acquired.
     */
    void spin_lock();

    /**
     * @brief Releases the lock.
     */
    void spin_unlock();
};

/// @brief Class representing the system calls and their handling. \class System
class System
{
public:

    /**
     * @brief Handles a system call.
     * @param num The system call number.
     * @param arg1 The first argument for the system call.
     * @param arg2 The second argument for the system call.
     * @param arg3 The third argument for the system call.
     * @return An integer representing the return value of the system call.
     */
    static int syscall(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3);
};

/// @brief Class representing a real-time scheduler for managing tasks. \class RealTimeScheduler
class RealTimeScheduler
{
public:

    /**
     * @brief Structure representing a task in the scheduler.
     */
    struct Task
    {
        TaskFunc function;
        VoidFunc void_function;
        void* context;
        uint32_t priority;
        bool run_once = false;
    };

    /**
     * @brief Initializes the real-time scheduler.
     */
    void initialize();

    /**
     * @brief Adds a task to the scheduler.
     * @param task The function pointer for the task.
     * @param priority The priority of the task.
     * @param run_once Whether the task should run only once.
     */
    void addTask(VoidFunc task, uint32_t priority, bool run_once = false);

    /**
     * @brief Adds a task to the scheduler with a context.
     * @param task The function pointer for the task.
     * @param context The context to pass to the task
     * @param run_once Whether the task should run only once.
     * @param priority The priority of the task.
     */
    void addTask(TaskFunc task, void* context, uint32_t priority, bool run_once = false);

    /**
     * @brief Runs the scheduler, executing tasks in a loop.
     */
    void run();

    /**
     * @brief Gets the number of tasks in the scheduler.
     * @return The number of tasks.
     */
    uint32_t getTaskCount() const { return taskCount; }

    /**
     * @brief Gets a task by its index.
     * @param index The index of the task.
     * @return A reference to the task.
     */
    const Task& getTask(uint32_t index) const;

    /**
     * @brief Yields the current task, allowing the scheduler to switch to another task.
     */
    void yield();

    /**
     * @brief Stops the current task, removing it from the scheduler.
     */
    void stopTask();

    /**
     * @brief Relaxes the scheduler, putting it in a low-power state until an interrupt occurs.
     */
    void relax();

private:
    static const uint32_t MAX_TASKS = 16;
    Task tasks[MAX_TASKS];
    uint32_t taskCount = 0;
    uint32_t current_task = 0;
};

/// @brief Class representing the kernel of the operating system. \class Kernel
class Kernel
{
public:

    /**
     * @brief Gets the singleton instance of the Kernel.
     * @return A pointer to the Kernel instance.
     */
    static Kernel* instance();

    /**
     * @brief Initializes the kernel, setting up the scheduler and terminal.
     */
    void initialize();

    /**
     * @brief Runs the kernel, starting the real-time scheduler.
     */
    void run();

    /**
     * @brief Gets the user context for switching to user space.
     * @return A reference to the user context.
     */
    RealTimeScheduler& scheduler();

    /**
     * @brief Gets the VGA terminal for output.
     * @return A reference to the VGA terminal.
     */
    VGATerminal& terminal();

    /**
     * @brief Switches to user space, setting up the entry point and stack.
     * @param entry The entry point function for the user space.
     * @param stack_top The top of the user stack.
     */
    void switch_to_userspace(void (*entry)(), uint32_t stack_top);

    /**
     * @brief Stops the kernel
     */
    void stop();

private:
    Kernel() = default;
    RealTimeScheduler rtScheduler;
    VGATerminal vgaTerminal;
    static Kernel* s_instance;
    ProcessContext user_context;
};