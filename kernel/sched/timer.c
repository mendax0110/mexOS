#include "timer.h"
#include "arch/i686/arch.h"
#include "arch/i686/idt.h"
#include "sched/sched.h"

#define PIT_FREQ 1193180
#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND 0x43
#define PIT_MODE 0x36
#define PIT_DISABLE 0x30

static volatile uint32_t tick_count = 0;

static void timer_callback(struct registers* regs)
{
    (void)regs; // TODO AdrGos -> handle regsisters properly in callback
    tick_count++;
    sched_tick();
}

void timer_init(const uint32_t frequency)
{
    register_interrupt_handler(32, timer_callback);

    const uint32_t divisor = PIT_FREQ / frequency;
    outb(PIT_COMMAND, PIT_MODE);
    outb(PIT_CHANNEL0_DATA, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0_DATA, (uint8_t)((divisor >> 8) & 0xFF));
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

uint32_t timer_get_seconds(void)
{
    return tick_count / 100;
}

uint32_t timer_get_minutes(void)
{
    return timer_get_seconds() / 60;
}

uint32_t timer_get_hours(void)
{
    return timer_get_minutes() / 60;
}

void timer_disable(void)
{
    outb(PIT_COMMAND, PIT_DISABLE);
}
