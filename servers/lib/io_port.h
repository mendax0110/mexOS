#ifndef SERVERS_IO_PORT_H
#define SERVERS_IO_PORT_H

#include "../../kernel/include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read a byte from an I/O port
 * @param port The I/O port number
 * @return The byte read from the port
 */
static inline uint8_t io_inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * @brief Read a word (16-bit) from an I/O port
 * @param port The I/O port number
 * @return The word read from the port
 */
static inline uint16_t io_inw(uint16_t port)
{
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * @brief Read a double word (32-bit) from an I/O port
 * @param port The I/O port number
 * @return The double word read from the port
 */
static inline uint32_t io_inl(uint16_t port)
{
    uint32_t result;
    __asm__ volatile ("inl %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * @brief Write a byte to an I/O port
 * @param port The I/O port number
 * @param value The byte to write
 */
static inline void io_outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Write a word (16-bit) to an I/O port
 * @param port The I/O port number
 * @param value The word to write
 */
static inline void io_outw(uint16_t port, uint16_t value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Write a double word (32-bit) to an I/O port
 * @param port The I/O port number
 * @param value The double word to write
 */
static inline void io_outl(uint16_t port, uint32_t value)
{
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Wait for I/O operation to complete
 *
 * Performs a small delay by writing to an unused port.
 * Useful after certain I/O operations that need time to complete.
 */
static inline void io_wait(void)
{
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

/**
 * @brief Read multiple words from an I/O port
 * @param port The I/O port number
 * @param buffer Destination buffer
 * @param count Number of words to read
 */
static inline void io_insw(uint16_t port, void *buffer, uint32_t count)
{
    __asm__ volatile (
            "rep insw"
            : "+D"(buffer), "+c"(count)
            : "d"(port)
            : "memory"
            );
}

/**
 * @brief Write multiple words to an I/O port
 * @param port The I/O port number
 * @param buffer Source buffer
 * @param count Number of words to write
 */
static inline void io_outsw(uint16_t port, const void *buffer, uint32_t count)
{
    __asm__ volatile (
            "rep outsw"
            : "+S"(buffer), "+c"(count)
            : "d"(port)
            : "memory"
            );
}


#ifdef __cplusplus
}
#endif

#endif // SERVERS_IO_PORT_H