#include "serial.h"
#include "../../include/types.h"

#define SERIAL_PORT 0x3F8

static inline void serial_out(uint16_t port, uint8_t value)
{
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t serial_in(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(void)
{
    serial_out(SERIAL_PORT + 1, 0x00); // Disable all interrupts
    serial_out(SERIAL_PORT + 3, 0x80); // Enable DLAB
    serial_out(SERIAL_PORT + 0, 0x03); // Baud rate divisor low byte (38400)
    serial_out(SERIAL_PORT + 1, 0x00); // Baud rate divisor high byte
    serial_out(SERIAL_PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    serial_out(SERIAL_PORT + 2, 0xC7); // FIFO, clear, 14-byte threshold
    serial_out(SERIAL_PORT + 4, 0x0B); // IRQs, RTS/DSR set
}

void serial_write(char c)
{
    while (!(serial_in(SERIAL_PORT + 5) & 0x20)) {} //transmit ready
    serial_out(SERIAL_PORT, c);
}

void serial_write_str(const char* str)
{
    while (*str != '\0')
    {
        serial_write(*str++);
    }
}
