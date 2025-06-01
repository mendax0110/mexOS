ENTRY(_start)

SECTIONS {
    . = 1M;

    .text : {
        *(.multiboot)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }

    .user : {
        user_start = .;
        *(.user*)
        user_end = .;
    }

    /DISCARD/ : {
        *(.eh_frame)
    }
}