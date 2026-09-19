#include "diag/panic.h"
#include "lib/string.h"
#include "include/addr.h"
#include "sched/sched.h"
#include "include/bitops.h"
#include "../../shared/asm.h"
#include "drivers/char/serial.h"
#include "lib/debug_utils.h"
#include "mm/alloc_track.h"
#include "sched/timer.h"

#define PANIC_VGA_MEMORY ((volatile uint16_t*)0xC00B8000)
#define PANIC_VGA_COLS    80
#define PANIC_VGA_ROWS    25

static int panic_vga_row = 0;
static int panic_vga_col = 0;
static uint8_t panic_current_attr = 0x0F;

static void panic_vga_clear(const uint8_t attr)
{
    volatile uint16_t* vga = PANIC_VGA_MEMORY;

    for (int i = 0; i < PANIC_VGA_COLS * PANIC_VGA_ROWS; i++)
    {
        vga[i] = ((uint16_t)attr << 8) | ' ';
    }

    panic_vga_row = 0;
    panic_vga_col = 0;
}

static void panic_vga_scroll(const uint8_t attr)
{
    volatile uint16_t* vga = PANIC_VGA_MEMORY;

    for (int row = 1; row < PANIC_VGA_ROWS; row++)
    {
        for (int col = 0; col < PANIC_VGA_COLS; col++)
        {
            vga[(row - 1) * PANIC_VGA_COLS + col] = vga[row * PANIC_VGA_COLS + col];
        }
    }

    for (int col = 0; col < PANIC_VGA_COLS; col++)
    {
        vga[(PANIC_VGA_ROWS - 1) * PANIC_VGA_COLS + col] = ((uint16_t)attr << 8) | ' ';
    }

    panic_vga_row = PANIC_VGA_ROWS - 1;
}

static void panic_vga_putc(const char c, const uint8_t attr)
{
    volatile uint16_t* vga = PANIC_VGA_MEMORY;

    if (c == '\n')
    {
        panic_vga_col = 0;
        panic_vga_row++;
    }
    else
    {
        vga[panic_vga_row * PANIC_VGA_COLS + panic_vga_col] = ((uint16_t)attr << 8) | (uint8_t)c;
        panic_vga_col++;

        if (panic_vga_col >= PANIC_VGA_COLS)
        {
            panic_vga_col = 0;
            panic_vga_row++;
        }
    }

    if (panic_vga_row >= PANIC_VGA_ROWS)
    {
        panic_vga_scroll(attr);
    }
}

static void panic_set_color(const uint8_t fg, const uint8_t bg)
{
    panic_current_attr = (uint8_t)((bg << 4) | (fg & 0x0F));
}

static void panic_write(const char* str)
{
    // use direct VGA output to avoid potential issues with vterm or console during panic....
    for (const char* p = str; *p; p++)
    {
        panic_vga_putc(*p, panic_current_attr);
        serial_write(*p);
    }
}

static void panic_write_hex(const uint32_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    char buf[11]; //"0x" + 8 hex digits + '\0'

    buf[0] = '0';
    buf[1] = 'x';

    for (int i = 0; i < 8; i++)
    {
        buf[2 + i] = digits[(value >> ((7 - i) * 4)) & 0xF];
    }

    buf[10] = '\0';
    panic_write(buf);
}

static void panic_write_dec(uint32_t value)
{
    char buf[11];
    int i = 10;

    buf[i--] = '\0';

    if (value == 0)
    {
        buf[i--] = '0';
    }
    else
    {
        while (value > 0 && i >= 0)
        {
            buf[i--] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    panic_write(&buf[i + 1]);
}

static void panic_dump_registers(void)
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp, eip;

    arch_get_registers(&eax, &ebx, &ecx, &edx, &esi, &edi, &ebp, &esp, &eip);

    panic_write("\nCPU Registers:\n");

    panic_write("EAX: ");
    panic_write_hex(eax);

    panic_write("\nEBX: ");
    panic_write_hex(ebx);

    panic_write("\nECX: ");
    panic_write_hex(ecx);

    panic_write("\nEDX: ");
    panic_write_hex(edx);

    panic_write("\nESI: ");
    panic_write_hex(esi);

    panic_write("\nEDI: ");
    panic_write_hex(edi);

    panic_write("\nEBP: ");
    panic_write_hex(ebp);

    panic_write("\nESP: ");
    panic_write_hex(esp);

    panic_write("\nEIP: ");
    panic_write_hex(eip);

    panic_write("\n");
}

static void get_panic_dump_allocation(const uint32_t index, void* ptr, const size_t size, const alloc_src_t src, const char* file, const int line)
{
    panic_write("  [");
    panic_write_dec(index);

    panic_write("] ptr: ");
    panic_write_hex((uint32_t)ptr);

    panic_write(", size: ");
    panic_write_dec(size);

    panic_write(", src: ");
    panic_write_dec(src);

    panic_write(", location: ");
    panic_write(file);

    panic_write(":");
    panic_write_dec(line);

    panic_write("\n");
}

static void panic_dump_allocations(void)
{
    const uint32_t live_count = alloc_track_live_count();
    const uint32_t live_bytes = alloc_track_live_bytes();

    panic_write("Live allocations: ");
    panic_write_dec(live_count);
    panic_write(" (");
    panic_write_dec(live_bytes);
    panic_write(" bytes)\n");
    alloc_track_foreach(get_panic_dump_allocation);
}

static void map_address_to_symbol(const uint32_t addr, char* buffer, const size_t buffer_size)
{
    if (!buffer || buffer_size == 0)
    {
        return;
    }

    const char* symbol = debug_get_symbol(addr);
    if (!symbol)
    {
        symbol = "(no symbol)";
    }

    strncpy(buffer, symbol, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
}

static void panic_backtrace(void)
{
    uint32_t* ebp;
    ASM_V("mov %%ebp, %0" : "=r"(ebp));

    struct task* task = sched_get_current();

    panic_write("\nStack backtrace:\n");

    if (!task)
    {
        panic_write("  (no current task)\n");
        return;
    }

    const uint32_t stack_min = task->kernel_stack;
    const uint32_t stack_max = task->kernel_stack_top;

    for (int i = 0; ebp && i < 16; i++)
    {
        if ((uint32_t)ebp < stack_min ||
            (uint32_t)ebp >= stack_max)
        {
            break;
        }

        uint32_t return_addr = ebp[1];

        if (return_addr == 0)
        {
            break;
        }

        char symbol[64];
        map_address_to_symbol(return_addr, symbol, sizeof(symbol));

        panic_write("  #");
        panic_write_dec(i);
        panic_write(": ");
        panic_write_hex(return_addr);
        panic_write("  ");
        panic_write(symbol);
        panic_write("\n");

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
    panic_write("EFLAGS: ");
    panic_write_hex(eflags);
    panic_write("\n");

    const struct eflag_print_map eflag_map[] = {
        {0, "CF"},{2, "PF"},
        {4, "AF"},{6, "ZF"},
        {7, "SF"},{8, "TF"},
        {9, "IF"},{10, "DF"},
        {11, "OF"},{12, "IOPL(1)"},
        {13, "IOPL(2)"},{14, "NT"},
        {16, "RF"}, {17, "VM"}
    };
    #define EFLAG_MAP_ENTRY_COUNT (sizeof(eflag_map) / sizeof(eflag_map[0]))

    panic_write("Flags: ");
    for (size_t i = 0; i < EFLAG_MAP_ENTRY_COUNT; i++)
    {
        if (TEST_BIT(eflags, eflag_map[i].bit))
        {
            panic_write(eflag_map[i].name);
            panic_write(" ");
        }
    }

    panic_write("\n");
}

static void panic_dump_cr0(const uint32_t cr0)
{
    panic_write("CR0: ");
    panic_write_hex(cr0);
    panic_write("\n");

    panic_write("CR0 Flags: ");

    const struct cr0_print_map cr0_map[] = {
        {0, "PE"},{1, "MP"},
        {2, "EM"},{3, "TS"},
        {4, "ET"},{5, "NE"},
        {16, "WP"},{18, "AM"},
        {29, "NW"},{30, "CD"},
        {31, "PG"}
    };
    #define CR0_MAP_ENTRY_COUNT (sizeof(cr0_map) / sizeof(cr0_map[0]))

    for (size_t i = 0; i < CR0_MAP_ENTRY_COUNT; i++)
    {
        if (TEST_BIT(cr0, cr0_map[i].bit))
        {
            panic_write(cr0_map[i].name);
            panic_write(" ");
        }
    }

    panic_write("\n");
}

static void panic_dump_memory(void)
{
    const uint32_t free_blocks = pmm_get_free_block_count();
    const uint32_t used_blocks = pmm_get_used_block_count();

    panic_write("\nMemory:\n");
    panic_write("  Free: ");
    panic_write_dec(free_blocks * 4);
    panic_write(" KB (");
    panic_write_dec(free_blocks);
    panic_write(" blocks)\n");
    panic_write("  Used: ");
    panic_write_dec(used_blocks * 4);
    panic_write(" KB (");
    panic_write_dec(used_blocks);
    panic_write(" blocks)\n");
    panic_write("\n");
}

static void panic_write_time(void)
{
    const uint32_t seconds = timer_get_seconds();
    const uint32_t minutes = timer_get_minutes();
    const uint32_t hours = timer_get_hours();

    panic_write("Uptime: ");
    panic_write_dec(hours);
    panic_write("h ");
    panic_write_dec(minutes % 60);
    panic_write("m ");
    panic_write_dec(seconds % 60);
    panic_write("s\n");
}

NORETURN void kernel_panic(const char* msg)
{
    cli();

    panic_set_color(VGA_WHITE, VGA_RED);
    panic_vga_clear(panic_current_attr);

    panic_write("\n\n========================================\n");
    panic_write("*** KERNEL PANIC ***\n");
    panic_write("========================================\n");
    panic_write_time();
    panic_write("Error: ");
    panic_write(msg);
    panic_write("\n\n");

    const uint32_t eflags = read_eflags();
    panic_dump_eflags(eflags);
    panic_dump_cr0(read_cr0());
    panic_dump_registers();
    panic_dump_memory();
    panic_dump_allocations();
    panic_backtrace();

    panic_write("\n\n=========================================\n");
    panic_write("System halted.\n");
    panic_write("=========================================\n");
    while (1) hlt();
}