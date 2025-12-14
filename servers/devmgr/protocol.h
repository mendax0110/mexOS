#ifndef SERVERS_DEVMGR_PROTOCOL_H
#define SERVERS_DEVMGR_PROTOCOL_H

#include "../../include/protocols/devmgr_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum registered devices
 */
#define DEVMGR_MAX_DEVICES 32

/**
 * @brief Maximum I/O port capabilities
 */
#define DEVMGR_MAX_IOPORT_CAPS 16

/**
 * @brief Maximum MMIO capabilities
 */
#define DEVMGR_MAX_MMIO_CAPS 16

/**
 * @brief Maximum IRQ capabilities
 */
#define DEVMGR_MAX_IRQ_CAPS 16

/**
 * @brief Internal device registry entry \struct devmgr_device
 */
struct devmgr_device
{
    int32_t device_id;
    char name[DEVMGR_MAX_NAME];
    uint8_t device_class;
    uint8_t used;
    uint8_t reserved[2];
    pid_t driver_pid;
    int32_t server_port;
};

/**
 * @brief I/O port capability structure \struct devmgr_ioport_cap
 */
struct devmgr_ioport_cap
{
    uint32_t capability_id;
    pid_t owner;
    uint16_t port_base;
    uint16_t port_count;
    uint8_t used;
    uint8_t reserved[3];
};

/**
 * @brief MMIO capability structure \struct devmgr_mmio_cap
 */
struct devmgr_mmio_cap
{
    uint32_t capability_id;
    pid_t owner;
    uint32_t phys_addr;
    uint32_t size;
    uint32_t virt_addr;
    uint8_t used;
    uint8_t reserved[3];
};

/**
 * @brief IRQ capability structure \struct devmgr_irq_cap
 */
struct devmgr_irq_cap
{
    uint32_t capability_id;
    pid_t owner;
    int32_t port_id;
    uint8_t irq_num;
    uint8_t used;
    uint8_t reserved[2];
};

/**
 * @brief PCI device enumeration entry \struct devmgr_pci_device
 */
struct devmgr_pci_device
{
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t reserved;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t header_type;
};

#ifdef __cplusplus
}
#endif

#endif // SERVERS_DEVMGR_PROTOCOL_H
