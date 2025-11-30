cmake_minimum_required(VERSION 3.10)
project(mexOS LANGUAGES C CXX ASM)

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker.ld")
if(NOT EXISTS "${LINKER_SCRIPT}")
    message(FATAL_ERROR "Linker script not found at: ${LINKER_SCRIPT}")
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/kernel
    ${CMAKE_SOURCE_DIR}/kernel/include
)

set(KERNEL_SOURCES
    kernel/arch/i686/boot.s
    kernel/arch/i686/gdt_asm.s
    kernel/arch/i686/idt_asm.s
    kernel/arch/i686/gdt.c
    kernel/arch/i686/idt.c
    kernel/core/string.c
    kernel/core/console.c
    kernel/core/timer.c
    kernel/core/syscall.c
    kernel/core/keyboard.c
    kernel/core/shell.c
    kernel/core/fs.c
    kernel/core/log.c
    kernel/mm/pmm.c
    kernel/mm/heap.c
    kernel/sched/sched.c
    kernel/sched/switch.s
    kernel/ipc/ipc.c
    kernel/kernel.c
    kernel/mm/vmm.c
    kernel/core/ata.c
)

add_executable(mexOS ${KERNEL_SOURCES})

target_compile_options(mexOS PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-ffreestanding -nostdlib -fno-builtin -Wall -Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-ffreestanding -nostdlib -fno-builtin -fno-exceptions -fno-rtti -Wall -Wextra>
)

target_link_options(mexOS PRIVATE "-T${LINKER_SCRIPT}" "-nostdlib" "-lgcc")
set_target_properties(mexOS PROPERTIES SUFFIX ".elf")

find_program(GRUB_MKRESCUE grub-mkrescue)
find_program(XORRISO xorriso)
if(GRUB_MKRESCUE AND XORRISO)
    configure_file(
        "${CMAKE_SOURCE_DIR}/grub.cfg.in"
        "${CMAKE_BINARY_DIR}/grub.cfg"
        @ONLY
    )
    add_custom_command(
        OUTPUT mexOS.iso
        COMMAND ${CMAKE_COMMAND} -E make_directory isodir/boot/grub
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/mexOS.elf isodir/boot/mexOS.elf
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/grub.cfg isodir/boot/grub/grub.cfg
        COMMAND ${GRUB_MKRESCUE} --xorriso=${XORRISO} -o mexOS.iso isodir -- -volid MEXOS
        DEPENDS mexOS.elf ${CMAKE_BINARY_DIR}/grub.cfg
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating bootable ISO"
    )
    add_custom_target(iso ALL DEPENDS mexOS.iso)
endif()

add_custom_target(run_iso
    COMMAND qemu-system-i386 -cdrom mexOS.iso -serial stdio -m 512M
    DEPENDS mexOS.iso
    COMMENT "Running in QEMU with ISO"
)

add_custom_target(run_kernel
    COMMAND qemu-system-i386 -kernel mexOS.elf -serial stdio -m 512M
    DEPENDS mexOS.elf
    COMMENT "Running in QEMU with kernel directly"
)