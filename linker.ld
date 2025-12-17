ENTRY(_start)

SECTIONS {
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text*)
    }

    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata*)
    }

    .data BLOCK(4K) : ALIGN(4K) {
        *(.data*)
    }

    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss*)
    }

    .initrd.entries BLOCK(4K) : ALIGN(4K) {
        __initrd_start = .;
        KEEP(*(.initrd.entries))
        __initrd_end = .;
    }

    .initrd.data BLOCK(4K) : ALIGN(4K) {
        KEEP(*(.initrd))
    }

    _kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
        *(.note.gnu.build-id)
    }
}
