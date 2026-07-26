#ifndef KERNEL_SERIAL_H
#define KERNEL_SERIAL_H

#include "../../../shared/types.h"

#define SERIAL_PORT 0x3F8
#define SERIAL_BUFFER_SIZE 256

#define SERIAL_REG_DATA 0 // Data Register (DLAB=0)
#define SERIAL_REG_IER 1 // Interrupt enable
#define SERIAL_REG_FCR 2 // FIFO control (write) , ISR (read)
#define SERIAL_REG_LCR 3 // Line control
#define SERIAL_REG_MCR 4 // Modem control
#define SERIAL_REG_LSR 5 // Line status

#define LCR_DLAB 0x80 // Divisor Latch Access Bit
#define LCR_8N1 0x03 // 8 bits, no parity, 1 stop bit

#define FCR_ENABLE 0x01 // Enable FIFO
#define FCR_CLEAR_RX 0x02 // Clear RX FIFO
#define FCR_CLEAR_TX 0x04 // Clear TX FIFO
#define FCR_TRIGGER_14 0xC0 // Trigger level 14 bytes
#define FCR_INIT (FCR_ENABLE | FCR_CLEAR_RX | FCR_CLEAR_TX | FCR_TRIGGER_14)

#define MCR_DTR 0x01 // Data Terminal Ready
#define MCR_RTS 0x02 // Request to Send
#define MCR_OUT2 0x08 // Out2 (used to enable interrupts)
#define MCR_INIT (MCR_DTR | MCR_RTS | MCR_OUT2)

#define LSR_DATA_READY 0x01 // Data Ready
#define LSR_TX_EMPTY 0x20 // Transmitter Holding Register Empty

#define SERIAL_BAUD_DIVISOR_LO 0x03 // Divisor low byte for 38400 baud
#define SERIAL_BAUD_DIVISOR_HI 0x00 // Divisor high byte for 38400 baud

#define MCR_LOOPBACK 0x10 // Loopback mode
#define SERIAL_TEST_BYTE 0xAE // Test byte for loopback test

/**
 * @brief Initialize the serial port COM1 (0x3F8).
 *
 * Configures baud rate, data bits, stop bits, parity, and FIFOs.
 * Must be called before any write operations.
 * @return true if initialization succeeded, false otherwise (e.g., if the loopback test fails).
 */
bool serial_init(void);

/**
 * @brief Write a single character to the serial port.
 *
 * @param c The character to send.
 */
void serial_write(char c);

/**
 * @brief Write a null-terminated string to the serial port.
 *
 * @param str The string to send.
 */
void serial_write_str(const char* str);

/**
 * @brief Flush the serial output buffer, ensuring all data is sent.
 */
void serial_flush(void);

/**
 * @brief Check whether a byte is available on COM1 RX.
 *
 * @return true if data is available, false otherwise.
 */
bool serial_has_data(void);

/**
 * @brief Read one character from COM1 (blocks until one arrives).
 *
 * @return The received byte.
 */
unsigned char serial_read_char(void);

#endif //KERNEL_SERIAL_H
