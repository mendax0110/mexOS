cmake_minimum_required(VERSION 3.10)
project(mexOS LANGUAGES C CXX ASM)

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker.ld")
set(USER_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/user/user.ld")

if(NOT EXISTS "${LINKER_SCRIPT}")
    message(FATAL_ERROR "Linker script not found at: ${LINKER_SCRIPT}")
endif()

# Include directories for kernel, protocols, and shared code
include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/kernel
        ${CMAKE_SOURCE_DIR}/kernel/include
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/servers
        ${CMAKE_SOURCE_DIR}/shared
)

# ============================================================================
# User-space init program
# ============================================================================
set(USER_INIT_SOURCES
        user/lib/crt0.s
        user/init/init.c
)

add_executable(init.elf ${USER_INIT_SOURCES})

target_compile_options(init.elf PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-ffreestanding -nostdlib -fno-builtin -Wall -Wextra -fno-stack-protector>
        $<$<COMPILE_LANGUAGE:ASM>:-ffreestanding>
)

target_link_options(init.elf PRIVATE
        "-T${USER_LINKER_SCRIPT}"
        "-nostdlib"
        "-static"
)

set_target_properties(init.elf PROPERTIES
        SUFFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/user"
)

add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/initrd.o
        COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-i386 -B i386
        --rename-section .data=.initrd,alloc,load,readonly,data,contents
        init.elf ${CMAKE_BINARY_DIR}/initrd.o
        DEPENDS init.elf
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/user
        COMMENT "Creating initrd object from user programs"
)

# ============================================================================
# Shared sources (used by both kernel and servers)
# ============================================================================
set(SHARED_SOURCES
        shared/string.c
        shared/log.c
        shared/serial.c
        #shared/kernel_stubs.c
)

# ============================================================================
# Server library sources (compiled with each server)
# ============================================================================
set(SERVERS_LIB_SOURCES
        servers/lib/io_port.c
        servers/lib/ipc_client.c
        servers/lib/memory.c
        ${SHARED_SOURCES}
)

# ============================================================================
# Kernel sources
# ============================================================================
set(KERNEL_SOURCES
        kernel/arch/i686/boot.s
        kernel/arch/i686/gdt_asm.s
        kernel/arch/i686/idt_asm.s
        kernel/arch/i686/gdt.c
        kernel/arch/i686/idt.c
        #kernel/lib/string.c
        #kernel/lib/log.c
        kernel/lib/debug_utils.c
        kernel/core/syscall.c
        kernel/core/elf.c
        servers/vfs/fs.c
        servers/vfs/diskfs.c
        kernel/mm/pmm.c
        kernel/mm/heap.c
        kernel/sched/sched.c
        kernel/sched/switch.s
        kernel/ipc/ipc.c
        kernel/kernel.c
        kernel/mm/vmm.c
        kernel/sys/sysmon.c
        kernel/sys/timer.c
        servers/block/ata.c
        servers/block/ahci.c
        #kernel/drivers/char/serial.c
        servers/input/keyboard.c
        kernel/drivers/char/rtc.c
        servers/devmgr/acpi.c
        servers/devmgr/pci.c
        servers/console/vesa.c
        servers/shell/shell.c
        servers/console/console.c
        servers/shell/basic.c
        servers/console/vterm.c
        servers/shell/tui.c
        kernel/ui/disk_installer.c
        servers/shell/editor.c
        tests/test_framework.c
        tests/test_runner.c
        tests/test_task.c
        tests/mm/test_pmm.c
        tests/mm/test_heap.c
        tests/core/test_string.c
        tests/core/test_fs.c
        tests/ipc/test_ipc.c
        tests/sched/test_sched.c
        tests/types/test_types.c
        ${SHARED_SOURCES}
)

# ============================================================================
# Kernel executable
# ============================================================================
add_executable(mexOS ${KERNEL_SOURCES} ${CMAKE_BINARY_DIR}/initrd.o)

target_compile_options(mexOS PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-ffreestanding -nostdlib -fno-builtin -Wall -Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-ffreestanding -nostdlib -fno-builtin -fno-exceptions -fno-rtti -Wall -Wextra>
)

target_link_options(mexOS PRIVATE "-T${LINKER_SCRIPT}" "-nostdlib" "-lgcc")
set_target_properties(mexOS PROPERTIES SUFFIX ".elf")

if(NOT DEFINED GRUB_MKRESCUE OR "${GRUB_MKRESCUE}" STREQUAL "")
    find_program(GRUB_MKRESCUE
            NAMES grub-mkrescue i686-elf-grub-mkrescue
            PATHS /opt/homebrew/bin /usr/local/bin /usr/bin
            NO_DEFAULT_PATH
    )
    if(NOT GRUB_MKRESCUE)
        find_program(GRUB_MKRESCUE NAMES grub-mkrescue i686-elf-grub-mkrescue)
    endif()
endif()

if(NOT DEFINED XORRISO OR "${XORRISO}" STREQUAL "")
    find_program(XORRISO
            NAMES xorriso
            PATHS /opt/homebrew/bin /usr/local/bin /usr/bin
            NO_DEFAULT_PATH
    )
    if(NOT XORRISO)
        find_program(XORRISO NAMES xorriso)
    endif()
endif()

message(STATUS "GRUB_MKRESCUE = ${GRUB_MKRESCUE}")
message(STATUS "XORRISO       = ${XORRISO}")

if(APPLE)
    set(GRUB_XORRISO_ARG "--xorriso=${XORRISO}")
else()
    set(GRUB_XORRISO_ARG "")
endif()

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
            COMMAND ${GRUB_MKRESCUE} ${GRUB_XORRISO_ARG} -o mexOS.iso isodir -- -volid MEXOS
            DEPENDS mexOS.elf ${CMAKE_BINARY_DIR}/grub.cfg
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Creating bootable ISO"
    )

    add_custom_target(iso ALL DEPENDS mexOS.iso)

else()
    message(WARNING "grub-mkrescue or xorriso not found. ISO target will not be available.")
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

# ============================================================================
# User-space server executables
# These are built as separate ELF files for the microkernel architecture
# ============================================================================

# Common compile options for servers
set(SERVER_COMPILE_OPTIONS
        $<$<COMPILE_LANGUAGE:C>:-ffreestanding -nostdlib -fno-builtin -Wall -Wextra -fno-stack-protector>
        $<$<COMPILE_LANGUAGE:ASM>:-ffreestanding>
)

set(SERVER_LINK_OPTIONS
        "-T${USER_LINKER_SCRIPT}"
        "-nostdlib"
        "-static"
)

# Console Server
set(CONSOLE_SERVER_SOURCES
        servers/console/main.c
        servers/console/console.c
        servers/console/vterm.c
        servers/console/vesa.c
        ${SERVERS_LIB_SOURCES}
        ${SHARED_SOURCES}
)
add_executable(console.elf user/lib/crt0.s ${CONSOLE_SERVER_SOURCES})
target_compile_options(console.elf PRIVATE ${SERVER_COMPILE_OPTIONS})
target_link_options(console.elf PRIVATE ${SERVER_LINK_OPTIONS})
set_target_properties(console.elf PROPERTIES
        SUFFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/servers"
)

# Input Server
set(INPUT_SERVER_SOURCES
        servers/input/main.c
        servers/input/keyboard.c
        ${SERVERS_LIB_SOURCES}
        ${SHARED_SOURCES}
)
add_executable(input.elf user/lib/crt0.s ${INPUT_SERVER_SOURCES})
target_compile_options(input.elf PRIVATE ${SERVER_COMPILE_OPTIONS})
target_link_options(input.elf PRIVATE ${SERVER_LINK_OPTIONS})
set_target_properties(input.elf PROPERTIES
        SUFFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/servers"
)

# VFS Server
set(VFS_SERVER_SOURCES
        servers/vfs/main.c
        servers/vfs/fs.c
        servers/vfs/diskfs.c
        ${SERVERS_LIB_SOURCES}
)
add_executable(vfs.elf user/lib/crt0.s ${VFS_SERVER_SOURCES})
target_compile_options(vfs.elf PRIVATE ${SERVER_COMPILE_OPTIONS})
target_link_options(vfs.elf PRIVATE ${SERVER_LINK_OPTIONS})
set_target_properties(vfs.elf PROPERTIES
        SUFFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/servers"
)

# Block Device Server
set(BLOCK_SERVER_SOURCES
        servers/block/main.c
        servers/block/ata.c
        servers/block/ahci.c
        ${SERVERS_LIB_SOURCES}
)
add_executable(block.elf user/lib/crt0.s ${BLOCK_SERVER_SOURCES})
target_compile_options(block.elf PRIVATE ${SERVER_COMPILE_OPTIONS})
target_link_options(block.elf PRIVATE ${SERVER_LINK_OPTIONS})
set_target_properties(block.elf PROPERTIES
        SUFFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/servers"
)

# Device Manager Server
set(DEVMGR_SERVER_SOURCES
        servers/devmgr/main.c
        servers/devmgr/pci.c
        servers/devmgr/acpi.c
        ${SERVERS_LIB_SOURCES}
)
add_executable(devmgr.elf user/lib/crt0.s ${DEVMGR_SERVER_SOURCES})
target_compile_options(devmgr.elf PRIVATE ${SERVER_COMPILE_OPTIONS})
target_link_options(devmgr.elf PRIVATE ${SERVER_LINK_OPTIONS})
set_target_properties(devmgr.elf PROPERTIES
        SUFFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/servers"
)

# Shell Application
set(SHELL_SOURCES
        servers/shell/main.c
        servers/shell/shell.c
        servers/shell/basic.c
        servers/shell/editor.c
        servers/shell/tui.c
        ${SERVERS_LIB_SOURCES}
)
add_executable(shell.elf user/lib/crt0.s ${SHELL_SOURCES})
target_compile_options(shell.elf PRIVATE ${SERVER_COMPILE_OPTIONS})
target_link_options(shell.elf PRIVATE ${SERVER_LINK_OPTIONS})
set_target_properties(shell.elf PROPERTIES
        SUFFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/servers"
)