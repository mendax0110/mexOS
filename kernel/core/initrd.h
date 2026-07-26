#ifndef INITRD_H
#define INITRD_H

#include "../../shared/types.h"

/**
 * @brief X-Macro Helper
 * @param X The param
 */
#define INITRD_PROGRAMS(X) \
    X(init, "/bin/init") \
    X(echo, "/bin/echo") \
    X(cat, "/bin/cat") \
    X(ls, "/bin/ls") \
    X(sh, "/bin/sh") \
    X(displayd, "/bin/displayd") \
    X(desktop, "/bin/desktop") \
    X(terminal, "/bin/terminal") \
    X(clear, "/bin/clear") \
    X(ps, "/bin/ps") \
    X(calc, "/bin/calc") \
    X(kill, "/bin/kill") \
    X(mem, "/bin/mem") \
    X(uptime, "/bin/uptime") \
    X(version, "/bin/version") \
    X(pwd, "/bin/pwd") \
    X(mkdir, "/bin/mkdir") \
    X(rm, "/bin/rm") \
    X(rmdir, "/bin/rmdir") \
    X(touch, "/bin/touch") \
    X(shutdown, "/bin/shutdown") \
    X(reboot, "/bin/reboot") \
    X(sync, "/bin/sync") \
    X(date, "/bin/date") \
    X(whoami, "/bin/whoami")

/**
 * @brief Helper Macro to create the symbol start
 * @param name The name of the symbol start
 */
#define INITRD_SYMBOL_START(name) _binary_##name##_elf_start

/**
 * @brief Helper Macro to create the symbol end
 * @param name The name of the symbol end
 */
#define INITRD_SYMBOL_END(name) _binary_##name##_elf_end

/**
 * @brief Helper Macro to declare the symbols for each initrd file
 * @param name The name of the file
 * @param path The path of the file
 */
#define INITRD_DECLARE_SYMBOLS(name, path)              \
    extern const uint8_t INITRD_SYMBOL_START(name)[];   \
    extern const uint8_t INITRD_SYMBOL_END(name)[];

/**
 * @brief Helper Macro to create the variables
 */
INITRD_PROGRAMS(INITRD_DECLARE_SYMBOLS)

#undef INITRD_DECLARE_SYMBOLS

/**
 * @brief Path to the init binary
 */
#define INITRD_INIT_PATH "/bin/init"

/**
 * @brief Struct to represent the initrd file \struct initrd_file
 */
struct initrd_file
{
    const char* path;
    const uint8_t* data;
    size_t size;
};

/**
 * @brief Get the number of files embedded in the initrd.
 * @return Return the number of files embedded in the initrd.
 */
size_t initrd_file_count(void);

/**
 * @brief Copy an embedded initrd file descriptor into out.
 * @param index The index of the file to copy
 * @param out The structure to copy the file descriptor into
 * @return Return 0 on success, -1 on failure
 */
int initrd_get_file(size_t index, struct initrd_file* out);

/**
 * @brief Install embedded user programs into the active VFS.
 * @return Return 0 on success, -1 on failure
 */
int initrd_install(void);

/**
 * @brief Get pointer to the embedded init binary
 * @return Pointer to the start of the init.elf data
 */
static inline const void* initrd_get_init(void)
{
    return _binary_init_elf_start;
}

/**
 * @brief Get size of the embedded init binary
 * @return Size in bytes
 */
static inline size_t initrd_get_init_size(void)
{
    return (size_t)(_binary_init_elf_end - _binary_init_elf_start);
}

#endif
