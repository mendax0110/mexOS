#include "timer.h"
#include "../arch/i686/arch.h"
#include "../arch/i686/idt.h"
#include "../sched/sched.h"

#define PIT_FREQ 1193180

static uint32_t tick_count = 0;

static void timer_callback(struct registers* regs)
{
    (void)regs;
    tick_count++;
    sched_tick();
}

void timer_init(const uint32_t frequency)
{
    register_interrupt_handler(32, timer_callback);

    const uint32_t divisor = PIT_FREQ / frequency;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

uint32_t timer_get_ticks(void)
{
    return tick_count;
}

void timer_wait(const uint32_t ticks)
{
    const uint32_t end = tick_count + ticks;
    while (tick_count < end)
    {
        hlt();
    }
}
