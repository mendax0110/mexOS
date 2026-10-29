#include "boot_defines.h"

.section .note.GNU-stack,"",%progbits

.section .multiboot, "a"
.align 4
multiboot_header:
    .long MB_MAGIC
    .long MB_FLAGS
    .long MB_CHECKSUM

.section .data
.align 4096
.global boot_page_directory
boot_page_directory:
    .set i, 0
    .rept BOOT_MAP_PDES
        .long (i << 22) | 0x83
        .set i, i+1
    .endr
    .rept (KERNEL_PAGE_NUMBER - BOOT_MAP_PDES)
        .long 0
    .endr
    .set i, 0
    .rept BOOT_MAP_PDES
        .long (i << 22) | 0x83
        .set i, i+1
    .endr
    .rept (1024 - KERNEL_PAGE_NUMBER - BOOT_MAP_PDES)
        .long 0
    .endr

.global stack_top
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
    # Disable interrupts
    cli

    mov $(boot_page_directory - KERNEL_VIRTUAL_BASE), %ecx
    mov %ecx, %cr3

    mov %cr4, %ecx
    or $CR4_SET_PSE, %ecx
    mov %ecx, %cr4

    mov %cr0, %ecx
    or $CR0_SET_PG, %ecx
    mov %ecx, %cr0

    lea .higher_half, %ecx
    jmp *%ecx

.higher_half:
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

    # If kernel returns, halt
    cli
.hang:
    hlt
    jmp .hang

.size _start, . - _start
