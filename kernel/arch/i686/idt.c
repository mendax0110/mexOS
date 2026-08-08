#include "idt.h"
#include "arch.h"
#include "lib/string.h"
#include "include/config.h"
#include "lib/log.h"
#include "sched/sched.h"
#include "include/addr.h"
#include "include/bitops.h"

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
    // Start init sequences in cascade mode...
    outb(PIC1_CMD, PIC_ICW1); io_wait();
    outb(PIC2_CMD, PIC_ICW1); io_wait();

    // ICW2 sets vector offests...
    outb(PIC1_DATA, PIC1_VECTOR_OFFSET); io_wait();
    outb(PIC2_DATA, PIC2_VECTOR_OFFSET); io_wait();

    // ICW3 sets up cascade wirings...
    outb(PIC1_DATA, PIC1_CASCADE_IR2); io_wait();
    outb(PIC2_DATA, PIC2_CASCADE_ID); io_wait();

    // ICW4 sets up x86 mode...
    outb(PIC1_DATA, PIC_ICW4_8086); io_wait();
    outb(PIC2_DATA, PIC_ICW4_8086); io_wait();

    // Unmask all IRQs
    outb(PIC1_DATA, PIC_MASK_NONE);  io_wait();
    outb(PIC2_DATA, PIC_MASK_NONE);  io_wait();
}

void idt_init(void)
{
    idt_pointer.limit = sizeof(struct idt_entry) * 256 - 1;
    idt_pointer.base  = PTR_TO_U32(idt_entries);

    memset(&idt_entries, 0, sizeof(struct idt_entry) * 256);
    memset(&handlers, 0, sizeof(handlers));

    pic_remap();

    // CPU exceptions (0-31)
    idt_set_gate(0,  PTR_TO_U32(isr0),  KERNEL_CS, 0x8E);
    idt_set_gate(1,  PTR_TO_U32(isr1),  KERNEL_CS, 0x8E);
    idt_set_gate(2,  PTR_TO_U32(isr2),  KERNEL_CS, 0x8E);
    idt_set_gate(3,  PTR_TO_U32(isr3),  KERNEL_CS, 0x8E);
    idt_set_gate(4,  PTR_TO_U32(isr4),  KERNEL_CS, 0x8E);
    idt_set_gate(5,  PTR_TO_U32(isr5),  KERNEL_CS, 0x8E);
    idt_set_gate(6,  PTR_TO_U32(isr6),  KERNEL_CS, 0x8E);
    idt_set_gate(7,  PTR_TO_U32(isr7),  KERNEL_CS, 0x8E);
    idt_set_gate(8,  PTR_TO_U32(isr8),  KERNEL_CS, 0x8E);
    idt_set_gate(9,  PTR_TO_U32(isr9),  KERNEL_CS, 0x8E);
    idt_set_gate(10, PTR_TO_U32(isr10), KERNEL_CS, 0x8E);
    idt_set_gate(11, PTR_TO_U32(isr11), KERNEL_CS, 0x8E);
    idt_set_gate(12, PTR_TO_U32(isr12), KERNEL_CS, 0x8E);
    idt_set_gate(13, PTR_TO_U32(isr13), KERNEL_CS, 0x8E);
    idt_set_gate(14, PTR_TO_U32(isr14), KERNEL_CS, 0x8E);
    idt_set_gate(15, PTR_TO_U32(isr15), KERNEL_CS, 0x8E);
    idt_set_gate(16, PTR_TO_U32(isr16), KERNEL_CS, 0x8E);
    idt_set_gate(17, PTR_TO_U32(isr17), KERNEL_CS, 0x8E);
    idt_set_gate(18, PTR_TO_U32(isr18), KERNEL_CS, 0x8E);
    idt_set_gate(19, PTR_TO_U32(isr19), KERNEL_CS, 0x8E);
    idt_set_gate(20, PTR_TO_U32(isr20), KERNEL_CS, 0x8E);
    idt_set_gate(21, PTR_TO_U32(isr21), KERNEL_CS, 0x8E);
    idt_set_gate(22, PTR_TO_U32(isr22), KERNEL_CS, 0x8E);
    idt_set_gate(23, PTR_TO_U32(isr23), KERNEL_CS, 0x8E);
    idt_set_gate(24, PTR_TO_U32(isr24), KERNEL_CS, 0x8E);
    idt_set_gate(25, PTR_TO_U32(isr25), KERNEL_CS, 0x8E);
    idt_set_gate(26, PTR_TO_U32(isr26), KERNEL_CS, 0x8E);
    idt_set_gate(27, PTR_TO_U32(isr27), KERNEL_CS, 0x8E);
    idt_set_gate(28, PTR_TO_U32(isr28), KERNEL_CS, 0x8E);
    idt_set_gate(29, PTR_TO_U32(isr29), KERNEL_CS, 0x8E);
    idt_set_gate(30, PTR_TO_U32(isr30), KERNEL_CS, 0x8E);
    idt_set_gate(31, PTR_TO_U32(isr31), KERNEL_CS, 0x8E);

    // Hardware IRQs (32-47)
    idt_set_gate(32, PTR_TO_U32(irq0),  KERNEL_CS, 0x8E);
    idt_set_gate(33, PTR_TO_U32(irq1),  KERNEL_CS, 0x8E);
    idt_set_gate(34, PTR_TO_U32(irq2),  KERNEL_CS, 0x8E);
    idt_set_gate(35, PTR_TO_U32(irq3),  KERNEL_CS, 0x8E);
    idt_set_gate(36, PTR_TO_U32(irq4),  KERNEL_CS, 0x8E);
    idt_set_gate(37, PTR_TO_U32(irq5),  KERNEL_CS, 0x8E);
    idt_set_gate(38, PTR_TO_U32(irq6),  KERNEL_CS, 0x8E);
    idt_set_gate(39, PTR_TO_U32(irq7),  KERNEL_CS, 0x8E);
    idt_set_gate(40, PTR_TO_U32(irq8),  KERNEL_CS, 0x8E);
    idt_set_gate(41, PTR_TO_U32(irq9),  KERNEL_CS, 0x8E);
    idt_set_gate(42, PTR_TO_U32(irq10), KERNEL_CS, 0x8E);
    idt_set_gate(43, PTR_TO_U32(irq11), KERNEL_CS, 0x8E);
    idt_set_gate(44, PTR_TO_U32(irq12), KERNEL_CS, 0x8E);
    idt_set_gate(45, PTR_TO_U32(irq13), KERNEL_CS, 0x8E);
    idt_set_gate(46, PTR_TO_U32(irq14), KERNEL_CS, 0x8E);
    idt_set_gate(47, PTR_TO_U32(irq15), KERNEL_CS, 0x8E);

    // Syscall interrupt - user accessible (DPL=3)
    idt_set_gate(128, PTR_TO_U32(isr128), KERNEL_CS, 0xEE);

    idt_flush(PTR_TO_U32(&idt_pointer));

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

    const int present = BIT_FLAG(regs->err_code, 0x1);
    const int write = BIT_FLAG(regs->err_code, 0x2);
    const int user = BIT_FLAG(regs->err_code, 0x4);
    const int reserved = BIT_FLAG(regs->err_code, 0x8);
    const int fetch = BIT_FLAG(regs->err_code, 0x10);

    if (user)
    {
        const struct task* current = sched_get_current();
        if (current)
        {
            task_exit(current->id, -1);
            schedule();
            char msg[128];
            snprintf(msg, sizeof(msg), "schedule() returned in page_fault_handler for user task (EIP=0x%x)", regs->eip);
            kernel_panic(msg);
        }

        char msg[128];
        snprintf(msg, sizeof(msg), "Page fault in user mode (CR2=0x%x, EIP=0x%x, %s%s%s%s), but no current task found",
                 faulting_address, regs->eip,
                 !present ? "not-present " : "",
                 write ? "write " : "",
                 reserved ? "reserved " : "",
                 fetch ? "fetch " : "");
        kernel_panic(msg);
    }

    log_error("KERNEL PANIC: Page fault in kernel mode!\n");
    log_error("Faulting address: 0x");

    char hex[9];
    for (int i = 7; i >= 0; i--)
    {
        const uint8_t nibble = (faulting_address >> (i * 4)) & 0xF;
        hex[7 - i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    hex[8] = '\0';
    log_error(hex);
    log_error("\n");

    log_error("Error: ");
    if (!present) log_error("page-not-present ");
    if (write) log_error("write ");
    if (reserved) log_error("reserved-bits ");
    if (fetch) log_error("instruction-fetch ");
    log_error("\n");

    log_error("EIP: 0x");
    for (int i = 7; i >= 0; i--)
    {
        const uint8_t nibble = (regs->eip >> (i * 4)) & 0xF;
        hex[7 - i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    hex[8] = '\0';
    log_error(hex);
    log_error("\n");

    char panic_msg[96];
    snprintf(panic_msg, sizeof(panic_msg),
             "Page fault in kernel mode (CR2=0x%x, EIP=0x%x, %s%s%s%s)",
             faulting_address, regs->eip,
             !present ? "not-present " : "",
             write ? "write " : "",
             reserved ? "reserved " : "",
             fetch ? "fetch " : "");
    kernel_panic(panic_msg);
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
            char msg[128];
            snprintf(msg, sizeof(msg), "schedule() returned in exception_handler after user task exit (EIP=0x%x, exception=%s)", regs->eip, exception_names[regs->int_no]);
            kernel_panic(msg);
        }

        char msg[128];
        snprintf(msg, sizeof(msg), "Exception in user mode (EIP=0x%x, exception=%s), but no current task found", regs->eip, exception_names[regs->int_no]);
        kernel_panic(msg);
    }

    log_error("KERNEL PANIC: ");
    if (regs->int_no < 20)
    {
        log_error(exception_names[regs->int_no]);
        kernel_panic(exception_names[regs->int_no]);
    }

    log_error("Unknown Exception");
    log_error("\n");

    char msg[128];
    snprintf(msg, sizeof(msg), "Unknown Exception in kernel mode (EIP=0x%x, int_no=%d)", regs->eip, regs->int_no);
    kernel_panic(msg);
}

void isr_handler(struct registers* regs)
{
    if (handlers[regs->int_no])
    {
        handlers[regs->int_no](regs);
        return;
    }

    char msg[64];
    snprintf(msg, sizeof(msg), "Unhandled interrupt: %d", regs->int_no);
    kernel_panic(msg);
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
