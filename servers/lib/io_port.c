#include "io_port.h"

/**
 * @brief Validate I/O port range
 * @param port_base Base port number
 * @param count Number of ports
 * @return true if valid range, false otherwise
 */
bool io_validate_port_range(uint16_t port_base, uint16_t count)
{
    /* Currently allow all ports for privileged servers */
    /* TODO: Implement capability checking */
    (void)port_base;
    (void)count;
    return true;
}

/**
 * @brief Read a block of bytes from sequential I/O ports
 * @param port_base Starting I/O port
 * @param buffer Destination buffer
 * @param count Number of bytes to read
 */
void io_read_bytes(uint16_t port_base, uint8_t *buffer, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        buffer[i] = io_inb(port_base + (uint16_t)i);
    }
}

/**
 * @brief Write a block of bytes to sequential I/O ports
 * @param port_base Starting I/O port
 * @param buffer Source buffer
 * @param count Number of bytes to write
 */
void io_write_bytes(uint16_t port_base, const uint8_t *buffer, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        io_outb(port_base + (uint16_t)i, buffer[i]);
    }
}