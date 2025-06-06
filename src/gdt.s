.section .data

.global gdt
.global gdt_ptr
.global tss

gdt:
    .quad 0x0000000000000000    # Null descriptor
    .quad 0x00CF9A000000FFFF    # Kernel code (0x08)
    .quad 0x00CF92000000FFFF    # Kernel data (0x10)
    .quad 0x00CFFA000000FFFF    # User code (0x18)
    .quad 0x00CFF2000000FFFF    # User data (0x20)
    .quad 0x0000890000000067    # TSS descriptor (0x28)
gdt_end:

gdt_ptr:
    .word gdt_end - gdt - 1
    .long gdt

.section .bss
.align 16
tss:
    .skip 104  # Size of TSS structure
tss_end:

.section .text
.global gdt_flush
.extern tss_flush

gdt_flush:
    lgdt [gdt_ptr]

    # Set kernel data segments
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Far jump to reload CS
    jmp $0x08, $.flush_cs
.flush_cs:
    # Initialize TSS
    mov $0x28, %ax
    ltr %ax
    ret
