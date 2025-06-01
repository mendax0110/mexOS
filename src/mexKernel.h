#pragma once

// Data types and constants for the MEX kernel
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

#define SYS_WRITE 0
#define SYS_READ 1

/**
 * @brief Function pointer types for tasks in the real-time scheduler.
 */
typedef void (*TaskFunc)(void*);

/**
 * @brief Function pointer type for tasks that do not require a context.
 */
typedef void (*VoidFunc)();

/**
 * @brief Structure representing the context of a process.
 */
typedef struct
{
    uint32_t eip;
    uint32_t esp;
    uint32_t ebp;
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
     */
    static void syscall(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3);
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
    };

    /**
     * @brief Initializes the real-time scheduler.
     */
    void initialize();

    /**
     * @brief Adds a task to the scheduler.
     * @param task The function pointer for the task.
     * @param priority The priority of the task.
     */
    void addTask(VoidFunc task, uint32_t priority);

    /**
     * @brief Adds a task to the scheduler with a context.
     * @param task The function pointer for the task.
     * @param context The context to pass to the task
     * @param priority The priority of the task.
     */
    void addTask(TaskFunc task, void* context, uint32_t priority);

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

private:

    static const uint32_t MAX_TASKS = 16;
    Task tasks[MAX_TASKS];
    uint32_t taskCount;
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

private:
    Kernel() = default;
    RealTimeScheduler rtScheduler;
    VGATerminal vgaTerminal;
    static Kernel* s_instance;
    ProcessContext user_context;
};