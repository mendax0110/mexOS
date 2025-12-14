#include "serial.h"
#include "include/types.h"

#define SERIAL_PORT 0x3F8
#define SERIAL_BUFFER_SIZE 256

static char serial_buffer[SERIAL_BUFFER_SIZE];
static uint32_t serial_buf_pos = 0;

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

static void serial_flush_buffer(void)
{
    for (uint32_t i = 0; i < serial_buf_pos; i++)
    {
        while (!(serial_in(SERIAL_PORT + 5) & 0x20)) {}
        serial_out(SERIAL_PORT, serial_buffer[i]);
    }
    serial_buf_pos = 0;
}

void serial_write(const char c)
{
    serial_buffer[serial_buf_pos++] = c;

    if (serial_buf_pos >= SERIAL_BUFFER_SIZE || c == '\n')
    {
        serial_flush_buffer();
    }
}

void serial_write_str(const char* str)
{
    while (*str != '\0')
    {
        serial_write(*str++);
    }
}

void serial_flush(void)
{
    if (serial_buf_pos > 0)
    {
        serial_flush_buffer();
    }
}
