ENTRY(_start)

KERNEL_VIRTUAL_BASE = 0xC0000000;
KERNEL_PHYSICAL_START = 0x00100000;

SECTIONS
{
    . = KERNEL_VIRTUAL_BASE + KERNEL_PHYSICAL_START;
    kernel_virtual_start = .;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE)
    {
        *(.multiboot)
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VIRTUAL_BASE)
    {
        *(.rodata*)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VIRTUAL_BASE)
    {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VIRTUAL_BASE)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    _kernel_end = .;

    /DISCARD/ :
    {
        *(.debug_*)
        *(.eh_frame)
        *(.comment)
        *(.note.gnu.build-id)
    }
}