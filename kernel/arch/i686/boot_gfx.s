.section .note.GNU-stack,"",%progbits

# Kernel virtual memory layout
.set KERNEL_VIRTUAL_BASE, 0xC0000000
.set KERNEL_PAGE_NUMBER, (KERNEL_VIRTUAL_BASE >> 22) # 768
.set BOOT_MAP_PDES, 16 # 16 * 4MB = 64MB of physical memory mapped to virtual memory

# Multiboot header flags
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set VIDEO,    1<<2
.set FLAGS,    ALIGN | MEMINFO | VIDEO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

# CR0/CR4 bits for FPU/SSE init
.set CR0_CLEAR_EM,   0xFFFFFFF3
.set CR0_SET_MP_NE,  0x22
.set CR4_SET_SSE,    0x600
.set CR4_SET_PSE,    0x00000010
.set CR0_SET_PG,     0x80000000

.section .multiboot, "a"
.align 4
multiboot_header:
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

    # These load-address fields are ignored for ELF kernels because bit 16 is
    # clear, but the video fields still live after them in the Multiboot layout.
    .long 0      # header_addr
    .long 0      # load_addr
    .long 0      # load_end_addr
    .long 0      # bss_end_addr
    .long 0      # entry_addr

    # Request a linear graphics framebuffer from a real bootloader.
    .long 0      # mode_type: linear graphics
    .long 1024   # width
    .long 768    # height
    .long 32     # depth

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
    # disable interrupts
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

    # if kernel returns, halt
    cli
.hang:
    hlt
    jmp .hang

.size _start, . - _start
