#include "protocol.h"
#include "pci.h"
#include "acpi.h"
#include "../lib/ipc_client.h"
#include "../lib/memory.h"
#include "../lib/io_port.h"

/** Server heap memory */
static uint8_t server_heap[32768] __attribute__((aligned(4096)));

/** Server port ID */
static int server_port = -1;

/** Registered devices */
static struct devmgr_device devices[DEVMGR_MAX_DEVICES];
static int device_count = 0;
static int next_device_id = 1;

/** I/O port capabilities */
static struct devmgr_ioport_cap ioport_caps[DEVMGR_MAX_IOPORT_CAPS];
static int next_cap_id = 1;

/** MMIO capabilities */
static struct devmgr_mmio_cap mmio_caps[DEVMGR_MAX_MMIO_CAPS];

/** IRQ capabilities */
static struct devmgr_irq_cap irq_caps[DEVMGR_MAX_IRQ_CAPS];

/** PCI devices found during scan */
static struct devmgr_pci_device pci_devices[32];
static int pci_device_count = 0;

/**
 * @brief Scan PCI bus for devices
 */
static void scan_pci_bus(void)
{
    pci_device_count = 0;

    for (uint8_t bus = 0; bus < 8; bus++)
    {
        for (uint8_t device = 0; device < 32; device++)
        {
            for (uint8_t function = 0; function < 8; function++)
            {
                if (pci_device_count >= 32)
                {
                    return;
                }

                uint32_t addr = (1U << 31) | ((uint32_t)bus << 16) |
                                ((uint32_t)device << 11) | ((uint32_t)function << 8);

                io_outl(0xCF8, addr);
                uint32_t data = io_inl(0xCFC);

                uint16_t vendor_id = (uint16_t)(data & 0xFFFF);
                if (vendor_id == 0xFFFF || vendor_id == 0x0000)
                {
                    if (function == 0)
                    {
                        break;
                    }
                    continue;
                }

                struct devmgr_pci_device *pdev = &pci_devices[pci_device_count++];
                pdev->bus = bus;
                pdev->device = device;
                pdev->function = function;
                pdev->vendor_id = vendor_id;
                pdev->device_id = (uint16_t)(data >> 16);

                io_outl(0xCF8, addr | 0x08);
                data = io_inl(0xCFC);
                pdev->class_code = (uint8_t)(data >> 24);
                pdev->subclass = (uint8_t)(data >> 16);
                pdev->prog_if = (uint8_t)(data >> 8);

                io_outl(0xCF8, addr | 0x0C);
                data = io_inl(0xCFC);
                pdev->header_type = (uint8_t)(data >> 16);

                if (function == 0 && !(pdev->header_type & 0x80))
                {
                    break;
                }
            }
        }
    }
}

/**
 * @brief Handle device registration request
 * @param msg Incoming message
 */
static void handle_register(struct message *msg)
{
    struct devmgr_register_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct devmgr_register_response resp = { .status = -1, .device_id = -1 };

    if (device_count < DEVMGR_MAX_DEVICES)
    {
        struct devmgr_device *dev = &devices[device_count++];
        dev->device_id = next_device_id++;
        dev->used = 1;
        dev->device_class = req.device_class;
        dev->driver_pid = msg->sender;
        dev->server_port = req.server_port;
        mem_copy(dev->name, req.name, DEVMGR_MAX_NAME);

        resp.status = 0;
        resp.device_id = dev->device_id;
    }

    msg->type = DEVMGR_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle enumeration request
 * @param msg Incoming message
 */
static void handle_enumerate(struct message *msg)
{
    struct devmgr_enumerate_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct devmgr_enumerate_response resp;
    mem_set(&resp, 0, sizeof(resp));
    resp.status = 0;

    int added = 0;
    for (int i = req.start_index; i < device_count && added < 2; i++)
    {
        if (devices[i].used)
        {
            if (req.device_class == 0 || devices[i].device_class == req.device_class)
            {
                resp.devices[added].device_id = devices[i].device_id;
                resp.devices[added].device_class = devices[i].device_class;
                resp.devices[added].server_port = devices[i].server_port;
                mem_copy(resp.devices[added].name, devices[i].name, DEVMGR_MAX_NAME);
                added++;
            }
        }
    }

    resp.count = (uint8_t)added;
    resp.more = (req.start_index + added < device_count) ? 1 : 0;

    msg->type = DEVMGR_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle I/O port request
 * @param msg Incoming message
 */
static void handle_ioport_request(struct message *msg)
{
    struct devmgr_ioport_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct devmgr_ioport_response resp = { .status = -1, .capability = 0 };

    for (int i = 0; i < DEVMGR_MAX_IOPORT_CAPS; i++)
    {
        if (!ioport_caps[i].used)
        {
            ioport_caps[i].capability_id = (uint32_t)next_cap_id++;
            ioport_caps[i].owner = msg->sender;
            ioport_caps[i].port_base = req.port_base;
            ioport_caps[i].port_count = req.port_count;
            ioport_caps[i].used = 1;

            resp.status = 0;
            resp.capability = ioport_caps[i].capability_id;
            break;
        }
    }

    msg->type = DEVMGR_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle PCI read request
 * @param msg Incoming message
 */
static void handle_pci_read(struct message *msg)
{
    struct devmgr_pci_read_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct devmgr_pci_read_response resp = { .status = 0, .value = 0 };

    uint32_t addr = (1U << 31) |
                    ((uint32_t)req.location.bus << 16) |
                    ((uint32_t)req.location.device << 11) |
                    ((uint32_t)req.location.function << 8) |
                    (req.reg & 0xFC);

    io_outl(0xCF8, addr);

    switch (req.size)
    {
        case 1:
            resp.value = io_inb(0xCFC + (req.reg & 3));
            break;
        case 2:
            resp.value = io_inw(0xCFC + (req.reg & 2));
            break;
        case 4:
            resp.value = io_inl(0xCFC);
            break;
        default:
            resp.status = -1;
            break;
    }

    msg->type = DEVMGR_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle PCI scan request
 * @param msg Incoming message
 */
static void handle_pci_scan(struct message *msg)
{
    scan_pci_bus();

    struct devmgr_response resp = { .status = pci_device_count };

    msg->type = DEVMGR_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Process a message from a client
 * @param msg The received message
 */
static void process_message(struct message *msg)
{
    switch (msg->type)
    {
        case DEVMGR_MSG_REGISTER:
            handle_register(msg);
            break;
        case DEVMGR_MSG_ENUMERATE:
            handle_enumerate(msg);
            break;
        case DEVMGR_MSG_IOPORT_REQ:
            handle_ioport_request(msg);
            break;
        case DEVMGR_MSG_PCI_READ:
            handle_pci_read(msg);
            break;
        case DEVMGR_MSG_PCI_SCAN:
            handle_pci_scan(msg);
            break;
        default:
        {
            struct devmgr_response resp = { .status = -1 };
            msg->type = DEVMGR_MSG_RESPONSE;
            ipc_msg_set_data(msg, &resp, sizeof(resp));
            ipc_reply(msg);
            break;
        }
    }
}

/**
 * @brief Device Manager server main function
 * @return Does not return
 */
int main(void)
{
    mem_init(server_heap, sizeof(server_heap));

    ipc_client_init();

    server_port = port_create();
    if (server_port < 0)
    {
        return -1;
    }

    ipc_register_server(DEVMGR_SERVER_PORT_NAME, server_port);

    mem_set(devices, 0, sizeof(devices));
    mem_set(ioport_caps, 0, sizeof(ioport_caps));
    mem_set(mmio_caps, 0, sizeof(mmio_caps));
    mem_set(irq_caps, 0, sizeof(irq_caps));

    scan_pci_bus();

    struct message msg;
    while (1)
    {
        int ret = ipc_receive(server_port, &msg, true);
        if (ret == IPC_SUCCESS)
        {
            process_message(&msg);
        }
    }

    return 0;
}
