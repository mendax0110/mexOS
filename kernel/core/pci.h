#ifndef KERNEL_PCI_H
#define KERNEL_PCI_H

#include "../include/types.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_VENDOR_INVALID 0xFFFF

#define PCI_MAX_BUS 256
#define PCI_MAX_DEVICE 32
#define PCI_MAX_FUNCTION 8

#define PCI_REG_VENDOR_ID 0x00
#define PCI_REG_DEVICE_ID 0x02
#define PCI_REG_COMMAND 0x04
#define PCI_REG_STATUS 0x06
#define PCI_REG_REVISION 0x08
#define PCI_REG_PROG_IF 0x09
#define PCI_REG_SUBCLASS 0x0A
#define PCI_REG_CLASS 0x0B
#define PCI_REG_CACHE_LINE 0x0C
#define PCI_REG_LATENCY 0x0D
#define PCI_REG_HEADER_TYPE 0x0E
#define PCI_REG_BIST 0x0F
#define PCI_REG_BAR0 0x10
#define PCI_REG_BAR1 0x14
#define PCI_REG_BAR2 0x18
#define PCI_REG_BAR3 0x1C
#define PCI_REG_BAR4 0x20
#define PCI_REG_BAR5 0x24
#define PCI_REG_INTERRUPT_LINE 0x3C
#define PCI_REG_INTERRUPT_PIN 0x3D

#define PCI_CLASS_UNCLASSIFIED 0x00
#define PCI_CLASS_MASS_STORAGE 0x01
#define PCI_CLASS_NETWORK 0x02
#define PCI_CLASS_DISPLAY 0x03
#define PCI_CLASS_MULTIMEDIA 0x04
#define PCI_CLASS_MEMORY 0x05
#define PCI_CLASS_BRIDGE 0x06
#define PCI_CLASS_COMMUNICATION 0x07
#define PCI_CLASS_PERIPHERAL 0x08
#define PCI_CLASS_INPUT 0x09
#define PCI_CLASS_DOCKING 0x0A
#define PCI_CLASS_PROCESSOR 0x0B
#define PCI_CLASS_SERIAL_BUS 0x0C

#define PCI_BAR_TYPE_MEMORY 0
#define PCI_BAR_TYPE_IO 1

/**
 * @brief PCI device structure
 */
struct pci_device
{
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint32_t bar[6];
    struct pci_device* next;
};

/**
 * @brief Initialize PCI subsystem and enumerate all devices
 */
void pci_init(void);

/**
 * @brief Read 8-bit value from PCI configuration space
 *
 * @param bus PCI bus number (0-255)
 * @param device PCI device number (0-31)
 * @param function PCI function number (0-7)
 * @param offset Register offset (0-255)
 * @return uint8_t Configuration space value
 */
uint8_t pci_config_read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/**
 * @brief Read 16-bit value from PCI configuration space
 *
 * @param bus PCI bus number (0-255)
 * @param device PCI device number (0-31)
 * @param function PCI function number (0-7)
 * @param offset Register offset (0-254, must be 2-byte aligned)
 * @return uint16_t Configuration space value
 */
uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/**
 * @brief Read 32-bit value from PCI configuration space
 *
 * @param bus PCI bus number (0-255)
 * @param device PCI device number (0-31)
 * @param function PCI function number (0-7)
 * @param offset Register offset (0-252, must be 4-byte aligned)
 * @return uint32_t Configuration space value
 */
uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/**
 * @brief Write 8-bit value to PCI configuration space
 *
 * @param bus PCI bus number (0-255)
 * @param device PCI device number (0-31)
 * @param function PCI function number (0-7)
 * @param offset Register offset (0-255)
 * @param value Value to write
 */
void pci_config_write_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value);

/**
 * @brief Write 16-bit value to PCI configuration space
 *
 * @param bus PCI bus number (0-255)
 * @param device PCI device number (0-31)
 * @param function PCI function number (0-7)
 * @param offset Register offset (0-254, must be 2-byte aligned)
 * @param value Value to write
 */
void pci_config_write_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);

/**
 * @brief Write 32-bit value to PCI configuration space
 *
 * @param bus PCI bus number (0-255)
 * @param device PCI device number (0-31)
 * @param function PCI function number (0-7)
 * @param offset Register offset (0-252, must be 4-byte aligned)
 * @param value Value to write
 */
void pci_config_write_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

/**
 * @brief Get linked list of all detected PCI devices
 *
 * @return struct pci_device* Pointer to first device in list, NULL if none
 */
struct pci_device* pci_get_devices(void);

/**
 * @brief Find first PCI device matching class and subclass
 *
 * @param class_code PCI class code
 * @param subclass PCI subclass code
 * @return struct pci_device* Pointer to device, NULL if not found
 */
struct pci_device* pci_find_device_by_class(uint8_t class_code, uint8_t subclass);

/**
 * @brief Find first PCI device matching vendor and device ID
 *
 * @param vendor_id PCI vendor ID
 * @param device_id PCI device ID
 * @return struct pci_device* Pointer to device, NULL if not found
 */
struct pci_device* pci_find_device_by_id(uint16_t vendor_id, uint16_t device_id);

/**
 * @brief Get Base Address Register (BAR) value and type
 *
 * @param dev PCI device pointer
 * @param bar_index BAR index (0-5)
 * @param is_io Output parameter: 1 if I/O BAR, 0 if memory BAR
 * @return uint32_t BAR base address (I/O port or physical memory address)
 */
uint32_t pci_get_bar(struct pci_device* dev, uint8_t bar_index, uint8_t* is_io);

/**
 * @brief Enable PCI bus mastering for device
 *
 * @param dev PCI device pointer
 */
void pci_enable_bus_mastering(struct pci_device* dev);

/**
 * @brief Print all detected PCI devices to console
 */
void pci_list_devices(void);

#endif // KERNEL_PCI_H

