#include "idt.h"
#include "arch.h"
#include "gdt.h"
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

    static void* isr_stub_table[32] = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };
    #define ISR_STUB_ENTRIES (sizeof(isr_stub_table) / sizeof(isr_stub_table[0]))

    // CPU exceptions (0-31)
    for (size_t i = 0; i < ISR_STUB_ENTRIES; i++)
    {
        idt_set_gate(i, PTR_TO_U32(isr_stub_table[i]), KERNEL_CS, 0x8E);
    }

    static void* irq_stub_table[16] = {
        irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7,
        irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15
    };
    #define IRQ_STUB_ENTRIES (sizeof(irq_stub_table) / sizeof(irq_stub_table[0]))

    // Hardware IRQs (32-47)
    for (size_t i = 0; i < IRQ_STUB_ENTRIES; i++)
    {
        idt_set_gate(32 + i, PTR_TO_U32(irq_stub_table[i]), KERNEL_CS, 0x8E);
    }

    // Double fault interrupt - uses dedicated TSS (DPL=0)
    idt_set_gate(8, 0, DF_TSS_SEGMENT * 8, 0x85);

    // Syscall interrupt - user accessible (DPL=3)
    idt_set_gate(128, PTR_TO_U32(isr128), KERNEL_CS, 0xEE);

    idt_flush(PTR_TO_U32(&idt_pointer));

    for (int i = 0; i < 32; i++)
    {
        if (i == 8) continue; // Skip double fault, handled separately
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
            PANIC_FMT("schedule() returned in page_fault_handler for user task (EIP=0x%x)", regs->eip);
        }

        PANIC_FMT("Page fault in user mode (CR2=0x%x, EIP=0x%x, %s%s%s%s), but no current task found",
                 faulting_address, regs->eip,
                 !present ? "not-present " : "",
                 write ? "write " : "",
                 reserved ? "reserved " : "",
                 fetch ? "fetch " : "");
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

    PANIC_FMT("Page fault in kernel mode (CR2=0x%x, EIP=0x%x, %s%s%s%s)",
             faulting_address, regs->eip,
             !present ? "not-present " : "",
             write ? "write " : "",
             reserved ? "reserved " : "",
             fetch ? "fetch " : "");
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
            PANIC_FMT("schedule() returned in exception_handler after user task exit (EIP=0x%x, exception=%s)", regs->eip, exception_names[regs->int_no]);
        }

        PANIC_FMT("Exception in user mode (EIP=0x%x, exception=%s), but no current task found", regs->eip, exception_names[regs->int_no]);
    }

    log_error("KERNEL PANIC: ");
    if (regs->int_no < 20)
    {
        log_error(exception_names[regs->int_no]);
        PANIC_FMT("Exception in kernel mode (EIP=0x%x, exception=%s)", regs->eip, exception_names[regs->int_no]);
    }

    log_error("Unknown Exception");
    log_error("\n");

    PANIC_FMT("Unknown Exception in kernel mode (EIP=0x%x, int_no=%d)", regs->eip, regs->int_no);
}

void isr_handler(struct registers* regs)
{
    if (handlers[regs->int_no])
    {
        handlers[regs->int_no](regs);
        return;
    }

    PANIC_FMT("Unhandled interrupt: %d", regs->int_no);
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
