.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.section .text
.global _start
.extern kernel_main
.extern init_interrupts

_start:
    # Set up stack
    mov $stack_top, %esp
    xor %ebp, %ebp  # Clear frame pointer

    # Initialize critical systems
    call gdt_flush
    call init_interrupts

    # Verify initialization
    mov $0xDEADBEEF, %eax
    cmp $0xDEADBEEF, %eax
    jne .hang

    # Call kernel main
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
