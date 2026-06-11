#ifndef KERNEL_SERIAL_H
#define KERNEL_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the serial port COM1 (0x3F8).
 *
 * Configures baud rate, data bits, stop bits, parity, and FIFOs.
 * Must be called before any write operations.
 */
void serial_init(void);

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
 * @return Non-zero if a character is waiting, zero otherwise.
 */
int serial_has_data(void);

/**
 * @brief Read one character from COM1 (blocks until one arrives).
 *
 * @return The received byte.
 */
unsigned char serial_read_char(void);

#ifdef __cplusplus
}
#endif

#endif //KERNEL_SERIAL_H
