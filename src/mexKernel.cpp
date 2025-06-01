#include "mexKernel.h"
#include "kernelUtils.h"
#include "memory.h"

Kernel* Kernel::s_instance = nullptr;

Kernel* Kernel::instance()
{
    if (!s_instance)
    {
        static Kernel instance;
        s_instance = &instance;
    }
    return s_instance;
}

void RealTimeScheduler::initialize()
{
    taskCount = 0;
}

void RealTimeScheduler::addTask(VoidFunc task, uint32_t priority, bool run_once)
{
    if (taskCount < MAX_TASKS)
    {
        Task newTask;
        newTask.void_function = task;
        newTask.function = nullptr;
        newTask.context = nullptr;
        newTask.priority = priority;
        newTask.run_once = run_once;
        tasks[taskCount] = newTask;
        taskCount++;
    }
}

void RealTimeScheduler::addTask(TaskFunc task, void* context, uint32_t priority, bool run_once)
{
    if (taskCount < MAX_TASKS)
    {
        Task newTask;
        newTask.function = task;
        newTask.void_function = nullptr;
        newTask.context = context;
        newTask.priority = priority;
        newTask.run_once = run_once;
        tasks[taskCount] = newTask;
        taskCount++;
    }
}

void RealTimeScheduler::relax()
{
    asm volatile("hlt");
}

extern "C" void context_switch(ProcessContext* old_ctx, ProcessContext* new_ctx);

void RealTimeScheduler::run()
{
    Kernel::instance()->terminal().write("Scheduler started\n");

    while (1)
    {
        if (taskCount == 0)
            //relax();
            continue; // No tasks

        Task& task = tasks[current_task];

        if (task.context)
        {
            ProcessContext* next_context = (ProcessContext*)task.context;
            static ProcessContext* current_context = nullptr;

            if (current_context == nullptr)
            {
                current_context = next_context;
                // Switch directly into user mode
                asm volatile(
                        "mov %0, %%esp\n\t"
                        "pushl $0x23\n\t"
                        "pushl %1\n\t"
                        "pushf\n\t"
                        "pushl $0x1B\n\t"
                        "pushl %2\n\t"
                        "iret\n\t"
                        :
                        : "r"(next_context->user_esp), "r"(next_context->user_esp), "r"(next_context->eip)
                        : "memory"
                        );
            }
            else
            {
                Kernel::instance()->terminal().write("Switching context...\n");
                context_switch(current_context, next_context);
                current_context = next_context;
            }
        }
        else if (task.void_function)
        {
            Kernel::instance()->terminal().write("Running task with void function...\n");
            task.void_function();
        }

        if (task.run_once)
        {
            Kernel::instance()->terminal().write("Task completed, removing...\n");
            stopTask();
        }
        else
        {
            current_task = (current_task + 1) % taskCount;
        }
    }
}

void Kernel::initialize()
{
    rtScheduler.initialize();
    vgaTerminal.initialize();
}

void Kernel::run()
{
    rtScheduler.run();
}

void Kernel::stop()
{
    rtScheduler.stopTask();
}

RealTimeScheduler& Kernel::scheduler()
{
    return rtScheduler;
}

const RealTimeScheduler::Task& RealTimeScheduler::getTask(uint32_t index) const
{
    if (index < taskCount)
    {
        return tasks[index];
    }
    return tasks[0];
}

void RealTimeScheduler::stopTask()
{
    if (taskCount > 0)
    {
        Kernel::instance()->terminal().write("Stopping current task...\n");

        for (uint32_t i = current_task; i + 1 < taskCount; i++)
        {
            tasks[i] = tasks[i + 1];
        }
        taskCount--;

        if (current_task >= taskCount)
        {
            current_task = 0;
        }
    }
    else
    {
        Kernel::instance()->terminal().write("No tasks to stop.\n");
    }
}

extern "C" void yield_handler()
{
    Kernel::instance()->scheduler().yield();
}

void RealTimeScheduler::yield()
{
    if (taskCount == 0)
    {
        Kernel::instance()->terminal().write("No tasks to yield to.\n");
        return;
    }

    if (taskCount == 1)
    {
        Kernel::instance()->terminal().write("Only one task running, yielding to itself.\n");
        return;
    }

    if (taskCount > 1)
    {
        Kernel::instance()->terminal().write("Switching to next task...\n");
        asm volatile ("int $0x20");
    }
    else
    {
        Kernel::instance()->terminal().write("No tasks available to yield to.\n");
    }
}

void Kernel::switch_to_userspace(void (*entry)(), uint32_t stack_top)
{
    // Aglipn stack to 16 bytes
    stack_top &= ~0xF;

    ProcessContext* ctx = (ProcessContext*)kmalloc(sizeof(ProcessContext));
    memset(ctx, 0, sizeof(ProcessContext));

    ctx->eip = (uint32_t)entry;
    ctx->user_esp = stack_top - 12;
    ctx->eflags = 0x202;  // IF=1

    Kernel::instance()->terminal().write("Switching to user space at entry point: 0x");
    Kernel::instance()->terminal().write(hex_to_str((uint32_t)entry));
    Kernel::instance()->terminal().write("\n");

    Kernel::instance()->terminal().write("User stack top: 0x");
    Kernel::instance()->terminal().write(hex_to_str(stack_top));
    Kernel::instance()->terminal().write("\n");

    ctx->cs = 0x1B;
    ctx->ss = 0x23;
    ctx->esp = stack_top - sizeof(ProcessContext);
    ctx->edi = 0;
    ctx->esi = 0;
    ctx->ebp = 0;
    ctx->ebx = 0;
    ctx->edx = 0;
    ctx->ecx = 0;
    ctx->eax = 0;
    Kernel::instance()->terminal().write("User context initialized.\n");

    scheduler().addTask((TaskFunc)entry, ctx, 2);
}
