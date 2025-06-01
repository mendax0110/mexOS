#include "mexKernel.h"

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

void RealTimeScheduler::addTask(VoidFunc task, uint32_t priority)
{
    if (taskCount < MAX_TASKS)
    {
        Task newTask;
        newTask.void_function = task;
        newTask.function = nullptr;
        newTask.context = nullptr;
        newTask.priority = priority;
        tasks[taskCount] = newTask;
        taskCount++;
    }
}

void RealTimeScheduler::addTask(TaskFunc task, void* context, uint32_t priority)
{
    if (taskCount < MAX_TASKS)
    {
        Task newTask;
        newTask.function = task;
        newTask.void_function = nullptr;
        newTask.context = context;
        newTask.priority = priority;
        tasks[taskCount] = newTask;
        taskCount++;
    }
}

void RealTimeScheduler::run()
{
    while (1)
    {
        for (uint32_t i = 0; i < taskCount; i++)
        {
            if (tasks[i].function)
            {
                tasks[i].function(tasks[i].context);
            }
            else if (tasks[i].void_function)
            {
                tasks[i].void_function();
            }
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

/*extern "C" void switch_to_userspace_asm(uint32_t eip, uint32_t esp);

void Kernel::switch_to_userspace(void (*entry)(), uint32_t stack_top)
{
    user_context.eip = (uint32_t)entry;
    user_context.esp = stack_top;
    user_context.ebp = stack_top;

    switch_to_userspace_asm(user_context.eip, user_context.esp);
}*/