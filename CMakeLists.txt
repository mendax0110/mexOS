cmake_minimum_required(VERSION 3.10)
project(mexOS LANGUAGES C CXX ASM)

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker.ld")
if(NOT EXISTS "${LINKER_SCRIPT}")
    message(FATAL_ERROR "Linker script not found at: ${LINKER_SCRIPT}")
endif()

include_directories(
        ${CMAKE_SOURCE_DIR}/src
)

add_executable(mexOS
        src/loader.s
        src/switch.s
        src/gdt.s
        src/exception.s
        src/interrupts.cpp
        src/mexKernel.cpp
        src/mexSystem.cpp
        src/kernelUtils.cpp
        src/memory.cpp
        src/cxxSupport.cpp
        src/shell.cpp
        src/mexOS.cpp
        src/userlib.cpp
)

target_link_options(mexOS PRIVATE "-T${LINKER_SCRIPT}")
set_target_properties(mexOS PROPERTIES SUFFIX ".elf")

find_program(GRUB_MKRESCUE grub-mkrescue)
if(GRUB_MKRESCUE)
    add_custom_command(
            OUTPUT mexOS.iso
            COMMAND mkdir -p isodir/boot/grub
            COMMAND cp ${CMAKE_BINARY_DIR}/mexOS.elf isodir/boot/mexOS.elf
            COMMAND echo "menuentry 'mexOS' { multiboot /boot/mexOS.elf }" > isodir/boot/grub/grub.cfg
            COMMAND ${GRUB_MKRESCUE} -o mexOS.iso isodir
            DEPENDS mexOS.elf
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