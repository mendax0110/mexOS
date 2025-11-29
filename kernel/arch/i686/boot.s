.section .note.GNU-stack,"",%progbits

# Multiboot header flags
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot, "a"
.align 4
multiboot_header:
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

.section .bss, "aw", @nobits
.align 16
stack_bottom:
    .skip 16384
stack_top:

.section .text
.global _start
.extern kernel_main
.type _start, @function

_start:
    # disable interrupts
    cli

    # Set up stack
    mov $stack_top, %esp
    xor %ebp, %ebp

    # Reset EFLAGS to a known state, ensures a clean processor state regardless of bootloader settings
    pushl $0
    popf

    # Push Multiboot info ptr and magic number
    push %ebx
    push %eax

    call kernel_main

    # if kernel returns, halt
    cli
.hang:
    hlt
    jmp .hang

.size _start, . - _start
