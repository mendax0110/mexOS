#include "mexKernel.h"
#include "kernelUtils.h"
#include "memory.h"
#include "interrupts.h"

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

static uint32_t next_task_id = 1;

void RealTimeScheduler::addTask(VoidFunc task, uint32_t priority, bool run_once)
{
    if (taskCount < MAX_TASKS)
    {
        Task newTask;
        newTask.id = next_task_id++;
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
        newTask.id = next_task_id++;
        newTask.function = task;
        newTask.void_function = nullptr;
        newTask.context = context;
        newTask.priority = priority;
        newTask.run_once = run_once;
        tasks[taskCount] = newTask;
        taskCount++;
    }
}

void RealTimeScheduler::removeTask(uint32_t id)
{
    for (uint32_t i = 0; i < taskCount; i++)
    {
        if (tasks[i].id == id)
        {
            // Shift remaining tasks
            for (uint32_t j = i; j < taskCount - 1; j++)
            {
                tasks[j] = tasks[j + 1];
            }
            taskCount--;

            // Adjust current_task if needed
            if (current_task >= i && current_task > 0)
            {
                current_task--;
            }
            break;
        }
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
    uint32_t last_switch = 0;

    while (1)
    {
        if (taskCount == 0)
            //relax();
            continue; // No tasks

        Task& task = tasks[current_task];
        uint32_t current_id = task.id;

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
                Kernel::instance()->terminal().write("Switching context...\n");                ProcessContext* old_context = current_context;
                current_context = next_context;
                context_switch(old_context, current_context);
            }
        }
        else if (task.void_function)
        {
            Kernel::instance()->terminal().write("Running task with void function...\n");
            task.void_function();
        }

        if (task.run_once)
        {
            removeTask(current_id);
            if (current_task >= taskCount)
            {
                current_task = 0;
            }
        }
        else
        {
            current_task = (current_task + 1) % taskCount;
        }

        current_task = (current_task + 1) % taskCount;
    }
}

void Kernel::initialize()
{
    extern uint32_t tss;
    uint32_t* tss_esp0 = (uint32_t*)((uint8_t*)&tss + 4);
    asm volatile("mov %%esp, %0" : "=m"(*tss_esp0));

    extern void tss_flush();

    ::init_interrupts();
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
    stack_top &= ~0xF;  // 16-byte alignment

    ProcessContext* ctx = (ProcessContext*)kmalloc(sizeof(ProcessContext));
    memset(ctx, 0, sizeof(ProcessContext));

    // Set up iret stack frame
    uint32_t* user_stack = (uint32_t*)stack_top;
    user_stack -= 5;

    user_stack[0] = 0x23;      // User data segment
    user_stack[1] = stack_top; // ESP
    user_stack[2] = 0x202;     // EFLAGS (IF=1)
    user_stack[3] = 0x1B;      // User code segment
    user_stack[4] = (uint32_t)entry; // EIP

    // Initialize context
    ctx->eip = (uint32_t)entry;
    ctx->user_esp = (uint32_t)user_stack;
    ctx->eflags = 0x202;
    ctx->cs = 0x1B;
    ctx->ss = 0x23;
    ctx->esp = stack_top;

    scheduler().addTask((TaskFunc)entry, ctx, 2);
}