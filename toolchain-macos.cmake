set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(CMAKE_C_COMPILER "i686-elf-gcc")
set(CMAKE_CXX_COMPILER "i686-elf-g++")
set(CMAKE_ASM_COMPILER "i686-elf-gcc")
set(CMAKE_AR "i686-elf-ar")
set(CMAKE_LINKER "i686-elf-ld")

set(CMAKE_C_FLAGS_INIT "-m32 -ffreestanding -nostdinc -nostdlib -fno-pie -fno-pic")
set(CMAKE_CXX_FLAGS_INIT "-m32 -ffreestanding -nostdinc -nostdlib -fno-exceptions -fno-rtti -fno-pie -fno-pic")
set(CMAKE_ASM_FLAGS_INIT "-m32")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-m32 -nostdlib -static -Wl,-m,elf_i386")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
