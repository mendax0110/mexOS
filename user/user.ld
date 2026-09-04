/*
 * User-space linker script for mexOS
 * Programs start at 0x08048000 (standard Linux user base)
 */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    . = 0x08048000;

    /* The text section contains the executable code. */
    .text : ALIGN(4K)
    {
        *(.text.entry)
        *(.text)
        *(.text.*)
    }

    /* The read-only data section. */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* The data section contains initialized data. */
    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }

    /* The bss section contains uninitialized data. */
    .bss : ALIGN(4K)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* The discard section contains data that should be discarded. */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
        *(.eh_frame_hdr)
    }
}
