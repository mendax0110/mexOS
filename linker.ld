ENTRY(_start)

/* Define memory regions if needed */
MEMORY {
    ram (rwx) : ORIGIN = 1M, LENGTH = 128M
}

/* Program headers for segment permissions */
PHDRS {
    text PT_LOAD FLAGS(5); /* R-X (read + execute) */
    rodata PT_LOAD FLAGS(4); /* R-- (read only) */
    data PT_LOAD FLAGS(6); /* RW- (read + write) */
    user PT_LOAD FLAGS(7); /* RWX for user sections (if absolutely needed) */
}

SECTIONS {
    . = 1M;

    /* Text section (executable code) */
    .text : {
        *(.multiboot)
        *(.text)
    } :text /* Assign to text program header */

    /* Read-only data */
    .rodata : {
        *(.rodata)
    } :rodata

    /* Read-write data (initialized) */
    .data : {
        *(.data)
    } :data

    /* Read-write data (uninitialized) */
    .bss : {
        *(COMMON)
        *(.bss)
    } :data

    /* User sections (if they need special permissions) */
    .user : {
        user_start = .;
        *(.user*)
        user_end = .;
    } :user

    /* Discard unwanted sections */
    /DISCARD/ : {
        *(.eh_frame)
    }
}