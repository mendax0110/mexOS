.section .note.GNU-stack,"",%progbits
.equ KERNEL_CS_SEL, 0x08
.equ KERNEL_DS_SEL, 0x10
.equ TSS_SEL, 0x2B

.section .text
.global gdt_flush
.global tss_flush

gdt_flush:
    mov 4(%esp), %eax
    lgdt (%eax)
    mov $KERNEL_DS_SEL, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    jmp $KERNEL_CS_SEL, $.flush
.flush:
    ret

tss_flush:
    mov $TSS_SEL, %ax
    ltr %ax
    ret
