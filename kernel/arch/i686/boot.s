.section .note.GNU-stack,"",%progbits

# Multiboot header flags
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

# CR0/CR4 bits for FPU/SSE init
.set CR0_CLEAR_EM,   0xFFFFFFF3
.set CR0_SET_MP_NE,  0x22
.set CR4_SET_SSE,    0x600

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

    # FPU/SSE init
    mov %cr0, %ecx
    and $CR0_CLEAR_EM, %ecx  # Clear EM bit (bit 2) to enable FPU
    or $CR0_SET_MP_NE, %ecx  # Set MP bit (bit 1) to enable FPU exceptions
    mov %ecx, %cr0

    # Enable SSE by setting CR4.OSFXSR (bit 9) and CR4.OSXMMEXCPT (bit 10)
    mov %cr4, %ecx
    or $CR4_SET_SSE, %ecx # Set bits 9 and 10 (OSFXSR and OSXMMEXCPT)
    mov %ecx, %cr4

    fninit # init FPU

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
