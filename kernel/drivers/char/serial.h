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

#ifdef __cplusplus
}
#endif

#endif //KERNEL_SERIAL_H
