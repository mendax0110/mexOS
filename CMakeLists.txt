cmake_minimum_required(VERSION 3.10)
project(mexOS LANGUAGES C CXX ASM)

# ----------------------------
# Linker scripts
# ----------------------------
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker.ld")
set(USER_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/user/user.ld")

if(NOT EXISTS "${LINKER_SCRIPT}")
    message(FATAL_ERROR "Linker script not found at: ${LINKER_SCRIPT}")
endif()

# ----------------------------
# Include directories
# ----------------------------
include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/kernel
        ${CMAKE_SOURCE_DIR}/kernel/include
        ${CMAKE_SOURCE_DIR}/kernel/core
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/servers
        ${CMAKE_SOURCE_DIR}/shared
)

# ----------------------------
# Shared sources
# ----------------------------
set(SHARED_SOURCES
        shared/string.c
        shared/log.c
        shared/serial.c
)

set(SERVERS_LIB_SOURCES
        servers/lib/io_port.c
        servers/lib/ipc_client.c
        servers/lib/memory.c
        servers/lib/block_client.c
        ${SHARED_SOURCES}
)

# ----------------------------
# User init program
# ----------------------------
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
        "-T${USER_LINKER_SCRIPT}" "-nostdlib" "-static"
)
set_target_properties(init.elf PROPERTIES
        SUFFIX ""
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/user"
)

# ----------------------------
# Server ELF targets
# ----------------------------
set(SERVER_COMPILE_OPTIONS
        $<$<COMPILE_LANGUAGE:C>:-ffreestanding -nostdlib -fno-builtin -Wall -Wextra -fno-stack-protector>
        $<$<COMPILE_LANGUAGE:ASM>:-ffreestanding>
)
set(SERVER_LINK_OPTIONS
        "-T${USER_LINKER_SCRIPT}" "-nostdlib" "-static"
)

function(add_server name)
    set(sources ${ARGN})
    add_executable(${name}.elf user/lib/crt0.s ${sources})
    target_compile_options(${name}.elf PRIVATE ${SERVER_COMPILE_OPTIONS})
    target_link_options(${name}.elf PRIVATE ${SERVER_LINK_OPTIONS})
    set_target_properties(${name}.elf PROPERTIES
            SUFFIX ""
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/servers"
    )
endfunction()

add_server(console
        servers/console/main.c
        servers/console/console.c
        servers/console/vterm.c
        servers/console/vesa.c
        ${SERVERS_LIB_SOURCES}
)

add_server(input
        servers/input/main.c
        ${SERVERS_LIB_SOURCES}
)

add_server(vfs
        servers/vfs/main.c
        servers/vfs/fs.c
        servers/vfs/diskfs.c
        ${SERVERS_LIB_SOURCES}
)

add_server(block
        servers/block/main.c
        servers/block/ata.c
        servers/block/ahci.c
        ${SERVERS_LIB_SOURCES}
)

add_server(devmgr
        servers/devmgr/main.c
        servers/devmgr/pci.c
        servers/devmgr/acpi.c
        ${SERVERS_LIB_SOURCES}
)

add_server(shell
        servers/shell/main.c
        ${SERVERS_LIB_SOURCES}
)

# ----------------------------
# ELF binaries to embed in initrd
# ----------------------------
set(INITRD_ELFS
        ${CMAKE_BINARY_DIR}/user/init.elf
        ${CMAKE_BINARY_DIR}/servers/console.elf
        ${CMAKE_BINARY_DIR}/servers/input.elf
        ${CMAKE_BINARY_DIR}/servers/vfs.elf
        ${CMAKE_BINARY_DIR}/servers/block.elf
        ${CMAKE_BINARY_DIR}/servers/devmgr.elf
        ${CMAKE_BINARY_DIR}/servers/shell.elf
)

# ----------------------------
# Generate .o blobs from ELF binaries
# ----------------------------
set(INITRD_OBJECTS "")
foreach(elf ${INITRD_ELFS})
    get_filename_component(name ${elf} NAME)
    get_filename_component(name_we ${elf} NAME_WE)

    set(temp_elf ${CMAKE_BINARY_DIR}/${name})
    add_custom_command(
            OUTPUT ${temp_elf}
            COMMAND ${CMAKE_COMMAND} -E copy ${elf} ${temp_elf}
            DEPENDS ${elf}
            COMMENT "Copying ${name} into build folder"
    )

    set(obj ${CMAKE_BINARY_DIR}/${name_we}.o)

    add_custom_command(
            OUTPUT ${obj}
            COMMAND ${CMAKE_OBJCOPY}
            -I binary -O elf32-i386 -B i386
            --rename-section .data=.initrd,alloc,load,readonly,data,contents
            --redefine-sym _binary_${name}_start=_binary_${name_we}_elf_start
            --redefine-sym _binary_${name}_end=_binary_${name_we}_elf_end
            --redefine-sym _binary_${name}_size=_binary_${name_we}_elf_size
            ${name} ${name_we}.o
            DEPENDS ${temp_elf}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Embedding ${name_we}.elf into initrd"
    )

    list(APPEND INITRD_OBJECTS ${obj})
endforeach()

add_custom_target(initrd_objects ALL DEPENDS ${INITRD_OBJECTS})

# ----------------------------
# Generate initrd_entries.c
# ----------------------------
set(INITRD_ENTRIES_C "${CMAKE_BINARY_DIR}/kernel/core/initrd_entries.c")

add_custom_command(
        OUTPUT ${INITRD_ENTRIES_C}
        COMMAND ${CMAKE_COMMAND}
        -DINITRD_ELFS="${INITRD_ELFS}"
        -DOUT_FILE=${INITRD_ENTRIES_C}
        -P "${CMAKE_SOURCE_DIR}/gen_initrd_entries.cmake"
        COMMENT "Generating initrd_entries.c for all embedded ELFs"
        DEPENDS ${INITRD_OBJECTS}
)

# ----------------------------
# Kernel sources
# ----------------------------
set(KERNEL_SOURCES
        kernel/arch/i686/boot.s
        kernel/arch/i686/gdt_asm.s
        kernel/arch/i686/idt_asm.s
        kernel/arch/i686/gdt.c
        kernel/arch/i686/idt.c
        kernel/lib/debug_utils.c
        kernel/core/syscall.c
        kernel/core/elf.c
        kernel/core/initrd.c
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
        ${INITRD_OBJECTS}
        ${INITRD_ENTRIES_C}
)

# ----------------------------
# Build kernel
# ----------------------------
add_executable(mexOS ${KERNEL_SOURCES} ${INITRD_OBJECTS} ${INITRD_ENTRIES_C})
add_dependencies(mexOS initrd_objects)

target_compile_definitions(mexOS PRIVATE __KERNEL__)
target_compile_options(mexOS PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-ffreestanding -nostdlib -fno-builtin -Wall -Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-ffreestanding -nostdlib -fno-builtin -fno-exceptions -fno-rtti -Wall -Wextra>
)
target_link_options(mexOS PRIVATE "-T${LINKER_SCRIPT}" "-nostdlib" "-lgcc")
set_target_properties(mexOS PROPERTIES SUFFIX ".elf")

# ----------------------------
# ISO and run targets
# ----------------------------
if(NOT DEFINED GRUB_MKRESCUE OR "${GRUB_MKRESCUE}" STREQUAL "")
    find_program(GRUB_MKRESCUE
            NAMES grub-mkrescue i686-elf-grub-mkrescue
            PATHS /opt/homebrew/bin /usr/local/bin /usr/bin
            NO_DEFAULT_PATH)
    if(NOT GRUB_MKRESCUE)
        find_program(GRUB_MKRESCUE NAMES grub-mkrescue i686-elf-grub-mkrescue)
    endif()
endif()

if(NOT DEFINED XORRISO OR "${XORRISO}" STREQUAL "")
    find_program(XORRISO
            NAMES xorriso
            PATHS /opt/homebrew/bin /usr/local/bin /usr/bin
            NO_DEFAULT_PATH)
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
    configure_file("${CMAKE_SOURCE_DIR}/grub.cfg.in" "${CMAKE_BINARY_DIR}/grub.cfg" @ONLY)

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
