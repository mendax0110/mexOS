#include "panic.h"
#include "string.h"
#include "../include/cast.h"
#include "../lib/debug_utils.h"

static void panic_dump_registers(void)
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp, eip;

    arch_get_registers(&eax, &ebx, &ecx, &edx, &esi, &edi, &ebp, &esp, &eip);

    console_write("\nCPU Registers:\n");

    console_write("EAX: ");
    console_write_hex(eax);

    console_write("\nEBX: ");
    console_write_hex(ebx);

    console_write("\nECX: ");
    console_write_hex(ecx);

    console_write("\nEDX: ");
    console_write_hex(edx);

    console_write("\nESI: ");
    console_write_hex(esi);

    console_write("\nEDI: ");
    console_write_hex(edi);

    console_write("\nEBP: ");
    console_write_hex(ebp);

    console_write("\nESP: ");
    console_write_hex(esp);

    console_write("\nEIP: ");
    console_write_hex(eip);

    console_write("\n");
}

static void map_address_to_symbol(const uint32_t addr, char* buffer, const size_t buffer_size)
{
    if (!buffer || buffer_size == 0)
    {
        return;
    }

    const char* symbol = debug_get_symbol(addr);
    strncpy(buffer, symbol, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
}

static void panic_backtrace(void)
{
    uint32_t* ebp;
    __asm__ volatile ("mov %%ebp, %0" : "=r"(ebp));

    console_write("Stack backtrace:\n");

    for (int i = 0; ebp && i < 16; i++)
    {
        const uint32_t stack_max = 0x02000000;
        const uint32_t stack_min = 0x00100000;

        if (!ebp) break;
        if ((uint32_t)ebp < stack_min || (uint32_t)ebp >= stack_max) break;

        uint32_t return_addr = ebp[1];
        if (return_addr == 0) break;
        if (return_addr < stack_min || return_addr >= stack_max) break;
        const char* sym = debug_get_symbol(return_addr);
        if (!sym) break;

        char symbol[64];
        map_address_to_symbol(return_addr, symbol, sizeof(symbol));

        console_write("  #");
        console_write_dec(i);
        console_write(": ");
        console_write_hex(return_addr);
        console_write("  ");
        console_write(symbol);
        console_write("\n");

        uint32_t* next = (uint32_t*)ebp[0];

        if (!next || next <= ebp)
        {
            break;
        }

        ebp = next;
    }
}

static void panic_dump_eflags(const uint32_t eflags)
{
    console_write("EFLAGS: ");
    console_write_hex(eflags);
    console_write("\n");

    console_write("Flags: ");
    if (BIT_FLAG(eflags, 0)) console_write("CF ");
    if (BIT_FLAG(eflags, 2)) console_write("PF ");
    if (BIT_FLAG(eflags, 4)) console_write("AF ");
    if (BIT_FLAG(eflags, 6)) console_write("ZF ");
    if (BIT_FLAG(eflags, 7)) console_write("SF ");
    if (BIT_FLAG(eflags, 8)) console_write("TF ");
    if (BIT_FLAG(eflags, 9)) console_write("IF ");
    if (BIT_FLAG(eflags, 10)) console_write("DF ");
    if (BIT_FLAG(eflags, 11)) console_write("OF ");
    if (BIT_FLAG(eflags, 12)) console_write("IOPL(1) ");
    if (BIT_FLAG(eflags, 13)) console_write("IOPL(2) ");
    if (BIT_FLAG(eflags, 14)) console_write("NT ");
    if (BIT_FLAG(eflags, 16)) console_write("RF ");
    if (BIT_FLAG(eflags, 17)) console_write("VM ");
    console_write("\n");
}

static void panic_dump_cr0(const uint32_t cr0)
{
    console_write("CR0: ");
    console_write_hex(cr0);
    console_write("\n");

    console_write("CR0 Flags: ");
    if (BIT_FLAG(cr0, 0)) console_write("PE ");
    if (BIT_FLAG(cr0, 1)) console_write("MP ");
    if (BIT_FLAG(cr0, 2)) console_write("EM ");
    if (BIT_FLAG(cr0, 3)) console_write("TS ");
    if (BIT_FLAG(cr0, 4)) console_write("ET ");
    if (BIT_FLAG(cr0, 5)) console_write("NE ");
    if (BIT_FLAG(cr0, 16)) console_write("WP ");
    if (BIT_FLAG(cr0, 18)) console_write("AM ");
    if (BIT_FLAG(cr0, 29)) console_write("NW ");
    if (BIT_FLAG(cr0, 30)) console_write("CD ");
    if (BIT_FLAG(cr0, 31)) console_write("PG ");
    console_write("\n");
}

static void panic_dump_memory(void)
{
    const uint32_t free_blocks = pmm_get_free_block_count();
    const uint32_t used_blocks = pmm_get_used_block_count();

    console_write("Memory:\n");
    console_write("  Free: ");
    console_write_dec(free_blocks * 4);
    console_write(" KB (");
    console_write_dec(free_blocks);
    console_write(" blocks)\n");
    console_write("  Used: ");
    console_write_dec(used_blocks * 4);
    console_write(" KB (");
    console_write_dec(used_blocks);
    console_write(" blocks)\n");
}

_Noreturn void kernel_panic(const char* msg)
{
    cli();
    console_set_color(VGA_WHITE, VGA_RED);
    console_write("\n\n========================================\n");
    console_write("*** KERNEL PANIC ***\n");
    console_write("========================================\n");
    console_write("Error: ");
    console_write(msg);
    console_write("\n\n");

    const uint32_t eflags = read_eflags();
    panic_dump_eflags(eflags);
    panic_dump_cr0(read_cr0());
    panic_dump_registers();
    panic_dump_memory();
    panic_backtrace();


    console_write("\n\n=========================================\n");
    console_write("System halted.\n");
    console_write("=========================================\n");
    while (1) hlt();
}