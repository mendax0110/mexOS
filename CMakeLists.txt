cmake_minimum_required(VERSION 3.10)
project(mexOS LANGUAGES C CXX ASM)

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker.ld")
set(USER_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/user/user.ld")
set(KCONFIG_CMAKE_FILE "${CMAKE_BINARY_DIR}/kconfig.cmake")

if(EXISTS "${KCONFIG_CMAKE_FILE}")
    message(STATUS "Loading Kconfig-derived settings from ${KCONFIG_CMAKE_FILE}")
    include("${KCONFIG_CMAKE_FILE}")
else()
    message(STATUS "No kconfig.cmake found -- using built-in defaults (run 'menuconfig Kconfig' to configure)")
endif()

if(NOT DEFINED CONFIG_MEXOS_TESTS)
    option(CONFIG_MEXOS_TESTS "Build kernel self-tests into the image" ON)
endif()
if(NOT DEFINED CONFIG_RUN_SELFTESTS)
    option(CONFIG_RUN_SELFTESTS "Run kernel self-tests automatically at boot" OFF)
endif()
if(NOT DEFINED CONFIG_INITRD)
    option(CONFIG_INITRD "Embed user programs into the initrd" ON)
endif()
if(NOT DEFINED CONFIG_BOOT_DISK_INSTALLER)
    option(CONFIG_BOOT_DISK_INSTALLER "Run interactive disk installer before user land" OFF)
endif()
if(NOT DEFINED CONFIG_DRIVER_ATA)
    option(CONFIG_DRIVER_ATA "Build ATA storage driver" ON)
endif()
if(NOT DEFINED CONFIG_DRIVER_AHCI)
    option(CONFIG_DRIVER_AHCI "Build AHCI storage driver" ON)
endif()
if(NOT DEFINED CONFIG_DRIVER_ACPI)
    option(CONFIG_DRIVER_ACPI "Build ACPI bus driver" ON)
endif()
if(NOT DEFINED CONFIG_DRIVER_PCI)
    option(CONFIG_DRIVER_PCI "Build PCI bus driver" ON)
endif()
if(NOT DEFINED CONFIG_DRIVER_VESA)
    option(CONFIG_DRIVER_VESA "Build VESA framebuffer driver" ON)
endif()
if(NOT DEFINED CONFIG_KERNEL_SHELL)
    option(CONFIG_KERNEL_SHELL "Build the recovery kernel shell" ON)
endif()
if(NOT DEFINED CONFIG_KERNEL_EDITOR)
    option(CONFIG_KERNEL_EDITOR "Build the kernel editor" OFF)
endif()
if(NOT DEFINED CONFIG_KERNEL_BASIC)
    option(CONFIG_KERNEL_BASIC "Build the BASIC interpreter" OFF)
endif()

if(NOT DEFINED CONFIG_KERNEL_HEAP_SIZE)
    set(CONFIG_KERNEL_HEAP_SIZE "0x00400000" CACHE STRING "Kernel heap size in bytes")
endif()
if(NOT DEFINED CONFIG_KERNEL_STACK_SIZE)
    set(CONFIG_KERNEL_STACK_SIZE "0x4000" CACHE STRING "Kernel stack size in bytes")
endif()
if(NOT DEFINED CONFIG_USER_STACK_SIZE)
    set(CONFIG_USER_STACK_SIZE "0x4000" CACHE STRING "User stack size in bytes")
endif()
if(NOT DEFINED CONFIG_FS_MAX_FILE_SIZE)
    set(CONFIG_FS_MAX_FILE_SIZE "32768" CACHE STRING "RAM filesystem maximum file size")
endif()
if(NOT DEFINED CONFIG_TICK_FREQUENCY_HZ)
    set(CONFIG_TICK_FREQUENCY_HZ "100" CACHE STRING "Scheduler tick frequency")
endif()
if(NOT DEFINED CONFIG_MAX_PORTS)
    set(CONFIG_MAX_PORTS "256" CACHE STRING "Maximum IPC ports")
endif()

message(STATUS "CONFIG_MEXOS_TESTS       = ${CONFIG_MEXOS_TESTS}")
message(STATUS "CONFIG_RUN_SELFTESTS     = ${CONFIG_RUN_SELFTESTS}")
message(STATUS "CONFIG_INITRD            = ${CONFIG_INITRD}")
message(STATUS "CONFIG_BOOT_DISK_INSTALLER = ${CONFIG_BOOT_DISK_INSTALLER}")
message(STATUS "CONFIG_DRIVER_ATA        = ${CONFIG_DRIVER_ATA}")
message(STATUS "CONFIG_DRIVER_AHCI       = ${CONFIG_DRIVER_AHCI}")
message(STATUS "CONFIG_DRIVER_ACPI       = ${CONFIG_DRIVER_ACPI}")
message(STATUS "CONFIG_DRIVER_PCI        = ${CONFIG_DRIVER_PCI}")
message(STATUS "CONFIG_DRIVER_VESA       = ${CONFIG_DRIVER_VESA}")
message(STATUS "CONFIG_KERNEL_SHELL      = ${CONFIG_KERNEL_SHELL}")
message(STATUS "CONFIG_KERNEL_EDITOR     = ${CONFIG_KERNEL_EDITOR}")
message(STATUS "CONFIG_KERNEL_BASIC      = ${CONFIG_KERNEL_BASIC}")
message(STATUS "CONFIG_KERNEL_HEAP_SIZE  = ${CONFIG_KERNEL_HEAP_SIZE}")
message(STATUS "CONFIG_KERNEL_STACK_SIZE = ${CONFIG_KERNEL_STACK_SIZE}")
message(STATUS "CONFIG_USER_STACK_SIZE   = ${CONFIG_USER_STACK_SIZE}")
message(STATUS "CONFIG_FS_MAX_FILE_SIZE  = ${CONFIG_FS_MAX_FILE_SIZE}")
message(STATUS "CONFIG_TICK_FREQUENCY_HZ = ${CONFIG_TICK_FREQUENCY_HZ}")
message(STATUS "CONFIG_MAX_PORTS         = ${CONFIG_MAX_PORTS}")

configure_file(
        "${CMAKE_SOURCE_DIR}/kernel/include/autoconf.h.in"
        "${CMAKE_BINARY_DIR}/generated/include/mexos/autoconf.h"
        @ONLY
)

if(NOT EXISTS "${LINKER_SCRIPT}")
    message(FATAL_ERROR "Linker script not found at: ${LINKER_SCRIPT}")
endif()

include_directories(
        ${CMAKE_SOURCE_DIR}/kernel
        ${CMAKE_SOURCE_DIR}/kernel/include
        ${CMAKE_BINARY_DIR}/generated/include
)

set_source_files_properties(
        user/lib/crt0.S
        PROPERTIES LANGUAGE ASM
        COMPILE_OPTIONS "-x;assembler-with-cpp"
)

function(add_user_program target output_name)
    add_executable(${target} user/lib/crt0.S ${ARGN})

    target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-ffreestanding -nostdlib -fno-builtin -Wall -Wextra -fno-stack-protector>
            $<$<COMPILE_LANGUAGE:ASM>:-ffreestanding>
    )

    target_include_directories(${target} PRIVATE
            ${CMAKE_SOURCE_DIR}/user/lib
            ${CMAKE_SOURCE_DIR}/user/init
            ${CMAKE_SOURCE_DIR}/user/bin
    )

    target_link_options(${target} PRIVATE
            "-T${USER_LINKER_SCRIPT}"
            "-nostdlib"
            "-static"
            "-Wl,--build-id=none"
            "-Wl,-z,noexecstack"
    )

    set_target_properties(${target} PROPERTIES
            OUTPUT_NAME "${output_name}"
            SUFFIX ".elf"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/user"
    )

    add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/initrd_${output_name}.o
            COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-i386 -B i386
            --rename-section .data=.initrd,alloc,load,readonly,data,contents
            ${output_name}.elf ${CMAKE_BINARY_DIR}/initrd_${output_name}.o
            DEPENDS ${target}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/user
            COMMENT "Embedding ${output_name}.elf into initrd"
    )

    set(USER_INITRD_OBJECTS ${USER_INITRD_OBJECTS} ${CMAKE_BINARY_DIR}/initrd_${output_name}.o PARENT_SCOPE)
endfunction()

set(USER_INITRD_OBJECTS "")

add_user_program(user_init init user/init/init.c)
add_user_program(user_echo echo user/bin/echo.c)
add_user_program(user_cat cat user/bin/cat.c)
add_user_program(user_ls ls user/bin/ls.c)
add_user_program(user_sh sh user/bin/sh.c)
add_user_program(user_displayd displayd user/bin/displayd.c)
add_user_program(user_desktop desktop user/bin/desktop.c)
add_user_program(user_terminal terminal user/bin/terminal.c)
add_user_program(user_calc calc user/bin/calc.c)
add_user_program(user_edit edit user/bin/edit.c)

set(USER_CORE_UTILS
        clear
        ps
        kill
        mem
        uptime
        version
        pwd
        mkdir
        rm
        rmdir
        touch
        shutdown
        reboot
        sync
        date
        whoami
)

foreach(cmd IN LISTS USER_CORE_UTILS)
    add_user_program(user_cmd_${cmd} ${cmd} user/bin/coreutils.c)
    target_compile_definitions(user_cmd_${cmd} PRIVATE USER_APP_NAME="${cmd}")
endforeach()

add_custom_target(user_initrd_objects DEPENDS ${USER_INITRD_OBJECTS})

set(KERNEL_SOURCES
        kernel/arch/i686/boot.s
        kernel/arch/i686/gdt_asm.s
        kernel/arch/i686/idt_asm.s
        kernel/arch/i686/gdt.c
        kernel/arch/i686/idt.c
        kernel/lib/string.c
        kernel/lib/log.c
        kernel/lib/debug_utils.c
        kernel/lib/compiler_rt.c
        kernel/core/rollback.c
        kernel/core/ref.c
        kernel/core/syscall.c
        kernel/core/pty.c
        kernel/core/power.c
        kernel/core/shm.c
        kernel/core/userland.c
        kernel/core/initrd.c
        kernel/exec/elf.c
        kernel/diag/panic.c
        kernel/diag/ptr_track.c
        kernel/sync/spinlock.c
        kernel/fs/fs.c
        kernel/fs/diskfs.c
        kernel/mm/pmm.c
        kernel/mm/heap.c
        kernel/mm/alloc_track.c
        kernel/sched/sched.c
        kernel/sched/switch.s
        kernel/sched/timer.c
        kernel/ipc/ipc.c
        kernel/kernel.c
        kernel/mm/vmm.c
        kernel/sysmon/sysmon.c
        kernel/drivers/storage/ata.c
        kernel/drivers/storage/ahci.c
        kernel/drivers/char/serial.c
        kernel/drivers/input/keyboard.c
        kernel/drivers/input/mouse.c
        kernel/drivers/char/rtc.c
        kernel/drivers/bus/acpi.c
        kernel/drivers/bus/pci.c
        kernel/drivers/video/vesa.c
        kernel/apps/shell.c
        kernel/ui/console.c
        kernel/ui/basic.c
        kernel/ui/vterm.c
        kernel/ui/tui.c
        kernel/apps/disk_installer.c
        kernel/apps/editor.c
        kernel/perm/perm.c
        tests/test_framework.c
        tests/test_runner.c
        tests/sched/test_task.c
        tests/mm/test_pmm.c
        tests/mm/test_vmm.c
        tests/mm/test_heap.c
        tests/core/test_string.c
        tests/core/test_fs.c
        tests/rtc/test_rtc.c
        tests/ipc/test_ipc.c
        tests/sched/test_sched.c
        tests/types/test_types.c
        tests/types/test_rollback.c
        tests/stress/test_stress.c
)

if(NOT CONFIG_MEXOS_TESTS)
    list(FILTER KERNEL_SOURCES EXCLUDE REGEX "^tests/")
endif()

if(NOT CONFIG_KERNEL_SHELL)
    list(REMOVE_ITEM KERNEL_SOURCES
            kernel/apps/shell.c
            kernel/apps/editor.c
            kernel/ui/basic.c
            kernel/ui/tui.c)
endif()

set(KERNEL_GFX_SOURCES ${KERNEL_SOURCES})
list(REMOVE_ITEM KERNEL_GFX_SOURCES kernel/arch/i686/boot.s)
set(KERNEL_GFX_SOURCES kernel/arch/i686/boot_gfx.s ${KERNEL_GFX_SOURCES})

add_executable(mexOS ${KERNEL_SOURCES} ${USER_INITRD_OBJECTS})
add_dependencies(mexOS user_initrd_objects)

function(configure_kernel_target target map_name)
target_compile_options(${target} PRIVATE
        $<$<COMPILE_LANGUAGE:C>:
        -ffreestanding -nostdlib -fno-builtin
        -Wall -Wextra
        -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
        -Wno-unused-function
        -fno-omit-frame-pointer
        -g
        >
        $<$<COMPILE_LANGUAGE:CXX>:
        -ffreestanding -nostdlib -fno-builtin
        -fno-exceptions -fno-rtti
        -Wall -Wextra
        -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
        -Wno-unused-function
        -fno-omit-frame-pointer
        -g
        >
)

target_link_options(${target} PRIVATE
        "-T${LINKER_SCRIPT}"
        "-nostdlib"
        "-lgcc"
        "-Wl,--build-id=none"
        "-Wl,-z,noexecstack"
        "-Wl,-Map=${CMAKE_BINARY_DIR}/${map_name}"
)
set_target_properties(${target} PROPERTIES SUFFIX ".elf")
endfunction()

configure_kernel_target(mexOS "mexOS.map")

add_executable(mexOS_gfx ${KERNEL_GFX_SOURCES} ${USER_INITRD_OBJECTS})
add_dependencies(mexOS_gfx user_initrd_objects)
configure_kernel_target(mexOS_gfx "mexOS-gfx.map")
set_target_properties(mexOS_gfx PROPERTIES OUTPUT_NAME "mexOS-gfx")

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
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mexOS> isodir/boot/mexOS.elf
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mexOS_gfx> isodir/boot/mexOS-gfx.elf
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/grub.cfg isodir/boot/grub/grub.cfg
            COMMAND ${GRUB_MKRESCUE} ${GRUB_XORRISO_ARG} -o mexOS.iso isodir -- -volid MEXOS
            DEPENDS mexOS mexOS_gfx ${CMAKE_BINARY_DIR}/grub.cfg
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Creating bootable ISO"
    )

    add_custom_target(iso ALL DEPENDS mexOS.iso)

else()
    message(WARNING "grub-mkrescue or xorriso not found. ISO target will not be available.")
endif()

add_custom_target(run_iso
        COMMAND qemu-system-i386
        -cdrom mexOS.iso
        -serial stdio
        -m 128M
        -drive file=${CMAKE_BINARY_DIR}/mexOS.img,format=raw,if=ide,index=0,media=disk
        -net nic,model=e1000
        -net user
        -vga std
        -d cpu_reset,guest_errors
        -D ${CMAKE_BINARY_DIR}/qemu.log
        DEPENDS mexOS.iso
        COMMENT "Running in QEMU with ISO"
)

add_custom_target(run_kernel
        COMMAND qemu-system-i386
        -kernel $<TARGET_FILE:mexOS>
        -serial stdio
        -m 128M
        -drive file=${CMAKE_BINARY_DIR}/mexOS.img,format=raw,if=ide,index=0,media=disk
        -net nic,model=e1000
        -net user
        -vga std
        -d cpu_reset,guest_errors
        -D ${CMAKE_BINARY_DIR}/qemu.log
        DEPENDS mexOS
        COMMENT "Running in QEMU with kernel directly"
)
add_custom_target(create_disk
        COMMAND ${CMAKE_COMMAND} -E echo "Creating 64MB disk image..."
        COMMAND qemu-img create -f raw ${CMAKE_BINARY_DIR}/mexOS.img 64M
        COMMENT "Creating blank disk image"
)
