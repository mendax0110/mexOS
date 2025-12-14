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

    .initrd ALIGN(16) : {
        __initrd_start = .;
        KEEP(*(.initrd.entries))
        __initrd_end = .;
    }

    .initrd.data ALIGN(16) : {
        KEEP(*(.initrd.data))
    }

    _kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
        *(.note.gnu.build-id)
    }
}
