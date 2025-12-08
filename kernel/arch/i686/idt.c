#include "idt.h"
#include "arch.h"
#include "../include/string.h"
#include "../include/config.h"
#include "../../ui/console.h"
#include "../../lib/log.h"
#include "../sched/sched.h"

static struct idt_entry idt_entries[256];
static struct idt_ptr   idt_pointer;
static isr_handler_t    handlers[256];
static void exception_handler(struct registers* regs);
static void page_fault_handler(const struct registers* regs);

void idt_set_gate(const uint8_t num, const uint32_t base, const uint16_t sel, const uint8_t flags)
{
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
}

static void pic_remap(void)
{
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
    outb(0x21, 0x0);  io_wait();
    outb(0xA1, 0x0);  io_wait();
}

void idt_init(void)
{
    idt_pointer.limit = sizeof(struct idt_entry) * 256 - 1;
    idt_pointer.base  = (uint32_t)&idt_entries;

    memset(&idt_entries, 0, sizeof(struct idt_entry) * 256);
    memset(&handlers, 0, sizeof(handlers));

    pic_remap();

    // CPU exceptions (0-31)
    idt_set_gate(0,  (uint32_t)isr0,  KERNEL_CS, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  KERNEL_CS, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  KERNEL_CS, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  KERNEL_CS, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  KERNEL_CS, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  KERNEL_CS, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  KERNEL_CS, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  KERNEL_CS, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  KERNEL_CS, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  KERNEL_CS, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, KERNEL_CS, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, KERNEL_CS, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, KERNEL_CS, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, KERNEL_CS, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, KERNEL_CS, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, KERNEL_CS, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, KERNEL_CS, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, KERNEL_CS, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, KERNEL_CS, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, KERNEL_CS, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, KERNEL_CS, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, KERNEL_CS, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, KERNEL_CS, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, KERNEL_CS, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, KERNEL_CS, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, KERNEL_CS, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, KERNEL_CS, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, KERNEL_CS, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, KERNEL_CS, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, KERNEL_CS, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, KERNEL_CS, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, KERNEL_CS, 0x8E);

    // Hardware IRQs (32-47)
    idt_set_gate(32, (uint32_t)irq0,  KERNEL_CS, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  KERNEL_CS, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  KERNEL_CS, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  KERNEL_CS, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  KERNEL_CS, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  KERNEL_CS, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  KERNEL_CS, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  KERNEL_CS, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  KERNEL_CS, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  KERNEL_CS, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, KERNEL_CS, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, KERNEL_CS, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, KERNEL_CS, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, KERNEL_CS, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, KERNEL_CS, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, KERNEL_CS, 0x8E);

    // Syscall interrupt - user accessible (DPL=3)
    idt_set_gate(128, (uint32_t)isr128, KERNEL_CS, 0xEE);

    idt_flush((uint32_t)&idt_pointer);

    for (int i = 0; i < 32; i++)
    {
        register_interrupt_handler(i, exception_handler);
    }
}

void register_interrupt_handler(const uint8_t n, const isr_handler_t handler)
{
    handlers[n] = handler;
}

static void page_fault_handler(const struct registers* regs)
{
    const uint32_t faulting_address = read_cr2();

    const int present = regs->err_code & 0x1;
    const int write = regs->err_code & 0x2;
    const int user = regs->err_code & 0x4;
    const int reserved = regs->err_code & 0x8;
    const int fetch = regs->err_code & 0x10;

    if (user)
    {
        const struct task* current = sched_get_current();
        if (current)
        {
            task_exit(current->id, -1);
            schedule();
        }
        return;
    }

    console_write("KERNEL PANIC: Page fault in kernel mode!\n");
    console_write("Faulting address: 0x");

    char hex[9];
    for (int i = 7; i >= 0; i--)
    {
        const uint8_t nibble = (faulting_address >> (i * 4)) & 0xF;
        hex[7 - i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    hex[8] = '\0';
    console_write(hex);
    console_write("\n");

    console_write("Error: ");
    if (!present) console_write("page-not-present ");
    if (write) console_write("write ");
    if (reserved) console_write("reserved-bits ");
    if (fetch) console_write("instruction-fetch ");
    console_write("\n");

    console_write("EIP: 0x");
    for (int i = 7; i >= 0; i--)
    {
        const uint8_t nibble = (regs->eip >> (i * 4)) & 0xF;
        hex[7 - i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    hex[8] = '\0';
    console_write(hex);
    console_write("\n");

    cli();
    for (;;)
    {
        hlt();
    }
}

static void exception_handler(struct registers* regs)
{
    static const char* exception_names[] = {
            "Division By Zero",
            "Debug",
            "Non Maskable Interrupt",
            "Breakpoint",
            "Overflow",
            "Bound Range Exceeded",
            "Invalid Opcode",
            "Device Not Available",
            "Double Fault",
            "Coprocessor Segment Overrun",
            "Invalid TSS",
            "Segment Not Present",
            "Stack-Segment Fault",
            "General Protection Fault",
            "Page Fault",
            "Reserved",
            "x87 Floating-Point Exception",
            "Alignment Check",
            "Machine Check",
            "SIMD Floating-Point Exception"
    };

    if (regs->int_no == 14)
    {
        page_fault_handler(regs);
        return;
    }

    const bool user_mode = (regs->cs & 0x3) == 3;

    if (user_mode)
    {
        const struct task* current = sched_get_current();
        if (current)
        {
            task_exit(current->id, -1);
            schedule();
        }
        return;
    }

    console_write("KERNEL PANIC: ");
    if (regs->int_no < 20)
    {
        console_write(exception_names[regs->int_no]);
    }
    else
    {
        console_write("Unknown Exception");
    }
    console_write("\n");

    cli();
    for (;;)
    {
        hlt();
    }
}

void isr_handler(struct registers* regs)
{
    if (handlers[regs->int_no])
    {
        handlers[regs->int_no](regs);
    }
}

void irq_handler(struct registers* regs)
{
    // Send EOI
    if (regs->int_no >= 40)
    {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);

    if (handlers[regs->int_no])
    {
        handlers[regs->int_no](regs);
    }
}
