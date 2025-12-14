#ifndef INCLUDE_DEVMGR_PROTOCOL_H
#define INCLUDE_DEVMGR_PROTOCOL_H

#include "../../kernel/include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device manager server port name for nameserver registration
 */
#define DEVMGR_SERVER_PORT_NAME "devmgr"

/**
 * @brief Maximum device name length
 */
#define DEVMGR_MAX_NAME 32

/**
 * @brief Device manager message types \enum devmgr_msg_type
 */
enum devmgr_msg_type
{
    DEVMGR_MSG_REGISTER = 0x0500,
    DEVMGR_MSG_UNREGISTER = 0x0501,
    DEVMGR_MSG_ENUMERATE = 0x0502,
    DEVMGR_MSG_GET_INFO = 0x0503,
    DEVMGR_MSG_IOPORT_REQ = 0x0510,
    DEVMGR_MSG_MMIO_REQ = 0x0511,
    DEVMGR_MSG_IRQ_REQ = 0x0512,
    DEVMGR_MSG_PCI_SCAN = 0x0520,
    DEVMGR_MSG_PCI_READ = 0x0521,
    DEVMGR_MSG_PCI_WRITE = 0x0522,
    DEVMGR_MSG_ACPI_QUERY = 0x0530,
    DEVMGR_MSG_RESPONSE = 0x05FF
};

/**
 * @brief Device classes for registration and enumeration \enum devmgr_device_class
 */
enum devmgr_device_class
{
    DEVMGR_CLASS_UNKNOWN = 0x00,
    DEVMGR_CLASS_STORAGE = 0x01,
    DEVMGR_CLASS_NETWORK = 0x02,
    DEVMGR_CLASS_DISPLAY = 0x03,
    DEVMGR_CLASS_INPUT = 0x04,
    DEVMGR_CLASS_SERIAL = 0x05,
    DEVMGR_CLASS_AUDIO = 0x06,
    DEVMGR_CLASS_USB = 0x07
};

/**
 * @brief Device register request structure \struct devmgr_register_request
 */
struct devmgr_register_request
{
    char name[DEVMGR_MAX_NAME];
    uint8_t device_class;
    uint8_t reserved[3];
    int32_t server_port;
};

/**
 * @brief Device register response structure \struct devmgr_register_response
 */
struct devmgr_register_response
{
    int32_t status;
    int32_t device_id;
};

/**
 * @brief Device enumeration request structure \struct devmgr_enumerate_request
 */
struct devmgr_enumerate_request
{
    uint8_t device_class;
    uint8_t start_index;
};

/**
 * @brief Device information structure \struct devmgr_device_info
 */
struct devmgr_device_info
{
    int32_t device_id;
    char name[DEVMGR_MAX_NAME];
    uint8_t device_class;
    uint8_t reserved[3];
    int32_t server_port;
};

/**
 * @brief Device enumeration response structure \struct devmgr_enumerate_response
 */
struct devmgr_enumerate_response
{
    int32_t status;
    uint8_t count;
    uint8_t more;
    uint8_t reserved[2];
    struct devmgr_device_info devices[2];
};

/**
 * @brief I/O port request structure \struct devmgr_ioport_request
 */
struct devmgr_ioport_request
{
    uint16_t port_base;
    uint16_t port_count;
};

/**
 * @brief I/O port response structure \struct devmgr_ioport_response
 */
struct devmgr_ioport_response
{
    int32_t  status;
    uint32_t capability;
};

/**
 * @brief MMIO request structure \struct devmgr_mmio_request
 */
struct devmgr_mmio_request
{
    uint32_t phys_addr;
    uint32_t size;
};

/**
 * @brief MMIO response structure \struct devmgr_mmio_response
 */
struct devmgr_mmio_response
{
    int32_t status;
    uint32_t virt_addr;
    uint32_t capability;
};

/**
 * @brief IRQ request structure \struct devmgr_irq_request
 */
struct devmgr_irq_request
{
    uint8_t irq_num;
    uint8_t reserved[3];
    int32_t port_id;
};

/**
 * @brief IRQ response structure \struct devmgr_irq_response
 */
struct devmgr_irq_response
{
    int32_t status;
    uint32_t capability;
};

/**
 * @brief PCI device location structure \struct devmgr_pci_location
 */
struct devmgr_pci_location
{
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t reserved;
};

/**
 * @brief PCI read request structure
 */
struct devmgr_pci_read_request
{
    struct devmgr_pci_location location;
    uint8_t reg;
    uint8_t size;
};

/**
 * @brief PCI read response structure \struct devmgr_pci_read_response
 */
struct devmgr_pci_read_response
{
    int32_t status;
    uint32_t value;
};

/**
 * @brief PCI write request structure \struct devmgr_pci_write_request
 */
struct devmgr_pci_write_request
{
    struct devmgr_pci_location location;
    uint8_t reg;
    uint8_t size;
    uint8_t reserved[2];
    uint32_t value;
};

/**
 * @brief Generic device manager response structure \struct devmgr_response
 */
struct devmgr_response
{
    int32_t status;
};

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_DEVMGR_PROTOCOL_H