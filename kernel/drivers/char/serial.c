#include "serial.h"
#include "include/asm.h"
#include "include/types.h"


static char serial_buffer[SERIAL_BUFFER_SIZE];
static uint32_t serial_buf_pos = 0;

static void serial_out(uint16_t port, uint8_t value)
{
    ASM_V("outb %0, %1" : : "a"(value), "Nd"(port));
}

static uint8_t serial_in(uint16_t port)
{
    uint8_t ret;
    ASM_V("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

bool serial_init(void)
{
    serial_out(SERIAL_PORT + SERIAL_REG_IER, 0x00); // Disable all interrupts
    serial_out(SERIAL_PORT + SERIAL_REG_LCR, LCR_DLAB); // Enable DLAB
    serial_out(SERIAL_PORT + SERIAL_REG_DATA, SERIAL_BAUD_DIVISOR_LO); // Baud rate divisor low byte (38400)
    serial_out(SERIAL_PORT + SERIAL_REG_IER, SERIAL_BAUD_DIVISOR_HI); // Baud rate divisor high byte
    serial_out(SERIAL_PORT + SERIAL_REG_LCR, LCR_8N1); // 8 bits, no parity, one stop bit
    serial_out(SERIAL_PORT + SERIAL_REG_FCR, FCR_INIT); // FIFO, clear, 14-byte threshold
    serial_out(SERIAL_PORT + SERIAL_REG_MCR, MCR_INIT); // IRQs, RTS/DSR set

    serial_out(SERIAL_PORT + SERIAL_REG_MCR, MCR_LOOPBACK);
    serial_out(SERIAL_PORT + SERIAL_REG_DATA, SERIAL_TEST_BYTE);

    if (serial_in(SERIAL_PORT + SERIAL_REG_DATA) != SERIAL_TEST_BYTE)
    {
        return false;
    }

    serial_out(SERIAL_PORT + SERIAL_REG_MCR, MCR_INIT);
    return true;
}

static void serial_flush_buffer(void)
{
    for (uint32_t i = 0; i < serial_buf_pos; i++)
    {
        while (!(serial_in(SERIAL_PORT + SERIAL_REG_LSR) & LSR_TX_EMPTY)) {}
        serial_out(SERIAL_PORT + SERIAL_REG_DATA, (uint8_t)serial_buffer[i]);
    }
    serial_buf_pos = 0;
}

void serial_write(const char c)
{
    if (serial_buf_pos >= SERIAL_BUFFER_SIZE)
    {
        serial_flush_buffer();
    }
    serial_buffer[serial_buf_pos++] = c;

    if (c == '\n')
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

bool serial_has_data(void)
{
    return (serial_in(SERIAL_PORT + SERIAL_REG_LSR) & LSR_DATA_READY) != 0;
}

unsigned char serial_read_char(void)
{
    while (!serial_has_data()) {}
    return serial_in(SERIAL_PORT + SERIAL_REG_DATA);
}
