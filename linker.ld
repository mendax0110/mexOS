/*
 * Kernel linker script for mexOS
 * Kernel-space start at virtual base address 0xC0000000,
 * with physical start at 0x00100000.
 */

ENTRY(_start)

KERNEL_VIRTUAL_BASE = 0xC0000000;
KERNEL_PHYSICAL_START = 0x00100000;

SECTIONS
{
    . = KERNEL_VIRTUAL_BASE + KERNEL_PHYSICAL_START;
    kernel_virtual_start = .;

    /* The text section contains the executable code. */
    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE)
    {
        *(.multiboot)
        *(.text)
    }

    /* The read-only data section. */
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VIRTUAL_BASE)
    {
        *(.rodata*)
    }

    /* The data section contains initialized data. */
    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VIRTUAL_BASE)
    {
        *(.data)
    }

    /* The bss section contains uninitialized data. */
    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VIRTUAL_BASE)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    _kernel_end = .;

    /* The discard section contains data that should be discarded. */
    /DISCARD/ :
    {
        *(.debug_*)
        *(.eh_frame)
        *(.comment)
        *(.note.gnu.build-id)
    }
}