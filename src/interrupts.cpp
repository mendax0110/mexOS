#include "interrupts.h"
#include "mexKernel.h"
#include "kernelUtils.h"

extern "C" {
IDTEntry idt[256] __attribute__((aligned(8)));
IDTPtr idt_ptr;

void init_interrupts(void)
{
    idt_ptr.limit = sizeof(IDTEntry) * 256 - 1;
    idt_ptr.base = (uint32_t)&idt;
    memset(&idt, 0, sizeof(IDTEntry) * 256);

    idt_set_gate(0, (uint32_t)divide_by_zero_handler, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)double_fault_handler, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)general_protection_fault_handler, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)page_fault_handler, 0x08, 0x8E);

    asm volatile("lidt %0" : : "m"(idt_ptr));
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].flags = flags;
}
}

extern "C" void exception_handler_wrapper(uint32_t exception_number)
{
    if (Kernel::instance())
    {
        Kernel::instance()->terminal().write("Exception! Code: 0x");
        Kernel::instance()->terminal().write(hex_to_str(exception_number));
        Kernel::instance()->terminal().write("\n");
    }
    while(1) asm("hlt");
}

extern "C" void yield_handler()
{
    asm volatile(
            "pusha\n\t"
            "call yield_handler_cpp\n\t"
            "popa\n\t"
            "iret\n\t"
            );
}

extern "C" void yield_handler_cpp()
{
    static bool in_handler = false;
    if (!in_handler && Kernel::instance())
    {
        in_handler = true;
        Kernel::instance()->scheduler().yield();
        in_handler = false;
    }
}