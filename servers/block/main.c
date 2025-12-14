#include "protocol.h"
#include "ata.h"
#include "ahci.h"
#include "../lib/ipc_client.h"
#include "../lib/memory.h"
#include "../lib/io_port.h"

/** Server heap memory */
static uint8_t server_heap[65536] __attribute__((aligned(4096)));

/** Server port ID */
static int server_port = -1;

/** Registered block devices */
static struct block_device devices[BLOCK_MAX_DEVICES];

/** Number of detected devices */
static uint8_t device_count = 0;

/**
 * @brief Detect ATA devices
 */
static void detect_ata_devices(void)
{
    const uint16_t bases[] = { 0x1F0, 0x1F0, 0x170, 0x170 };
    const uint8_t drives[] = { 0, 1, 0, 1 };

    for (int i = 0; i < 4 && device_count < BLOCK_MAX_DEVICES; i++)
    {
        uint16_t base = bases[i];
        uint8_t drive = drives[i];

        io_outb(base + 6, (uint8_t)(0xA0 | (drive << 4)));
        io_wait();

        io_outb(base + 7, 0xEC);
        io_wait();

        uint8_t status = io_inb(base + 7);
        if (status == 0)
        {
            continue;
        }

        uint32_t timeout = 100000;
        while ((io_inb(base + 7) & 0x80) && timeout--)
        {
            io_wait();
        }

        if (timeout == 0)
        {
            continue;
        }

        if (io_inb(base + 4) != 0 || io_inb(base + 5) != 0)
        {
            continue;
        }

        timeout = 100000;
        while (!(io_inb(base + 7) & 0x08) && timeout--)
        {
            if (io_inb(base + 7) & 0x01)
            {
                break;
            }
        }

        if (timeout == 0)
        {
            continue;
        }

        uint16_t identify[256];
        for (int j = 0; j < 256; j++)
        {
            identify[j] = io_inw(base);
        }

        struct block_device *dev = &devices[device_count];
        dev->id = device_count;
        dev->type = BLOCK_DEV_ATA;
        dev->state = BLOCK_STATE_ONLINE;
        dev->sector_size = BLOCK_SECTOR_SIZE;

        dev->sector_count = (uint32_t)identify[60] | ((uint32_t)identify[61] << 16);

        for (int j = 0; j < 20; j++)
        {
            dev->model[j * 2] = (char)(identify[27 + j] >> 8);
            dev->model[j * 2 + 1] = (char)(identify[27 + j] & 0xFF);
        }
        dev->model[39] = '\0';

        device_count++;
    }
}

/**
 * @brief Handle read request
 * @param msg Incoming message
 */
static void handle_read(struct message *msg)
{
    struct block_read_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct block_response resp = { .status = -1, .bytes_transferred = 0 };

    if (req.device_id >= device_count)
    {
        msg->type = BLOCK_MSG_RESPONSE;
        ipc_msg_set_data(msg, &resp, sizeof(resp));
        ipc_reply(msg);
        return;
    }

    struct block_device *dev = &devices[req.device_id];
    if (dev->state != BLOCK_STATE_ONLINE)
    {
        msg->type = BLOCK_MSG_RESPONSE;
        ipc_msg_set_data(msg, &resp, sizeof(resp));
        ipc_reply(msg);
        return;
    }

    /* TODO: Perform actual ATA/AHCI read */
    resp.status = 0;
    resp.bytes_transferred = req.count * dev->sector_size;

    msg->type = BLOCK_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle write request
 * @param msg Incoming message
 */
static void handle_write(struct message *msg)
{
    struct block_write_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct block_response resp = { .status = -1, .bytes_transferred = 0 };

    if (req.device_id >= device_count)
    {
        msg->type = BLOCK_MSG_RESPONSE;
        ipc_msg_set_data(msg, &resp, sizeof(resp));
        ipc_reply(msg);
        return;
    }

    struct block_device *dev = &devices[req.device_id];
    if (dev->state != BLOCK_STATE_ONLINE)
    {
        msg->type = BLOCK_MSG_RESPONSE;
        ipc_msg_set_data(msg, &resp, sizeof(resp));
        ipc_reply(msg);
        return;
    }

    /* TODO: Perform actual ATA/AHCI write */
    resp.status = 0;
    resp.bytes_transferred = req.count * dev->sector_size;

    msg->type = BLOCK_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle get info request
 * @param msg Incoming message
 */
static void handle_get_info(struct message *msg)
{
    struct block_info_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct block_info_response resp;
    mem_set(&resp, 0, sizeof(resp));

    if (req.device_id >= device_count)
    {
        resp.status = -1;
        msg->type = BLOCK_MSG_RESPONSE;
        ipc_msg_set_data(msg, &resp, sizeof(resp));
        ipc_reply(msg);
        return;
    }

    struct block_device *dev = &devices[req.device_id];
    resp.status = 0;
    resp.device_type = dev->type;
    resp.sector_size = dev->sector_size;
    resp.sector_count = dev->sector_count;
    mem_copy(resp.model, dev->model, 40);

    msg->type = BLOCK_MSG_RESPONSE;
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
        case BLOCK_MSG_READ:
            handle_read(msg);
            break;
        case BLOCK_MSG_WRITE:
            handle_write(msg);
            break;
        case BLOCK_MSG_GET_INFO:
            handle_get_info(msg);
            break;
        case BLOCK_MSG_FLUSH:
        {
            struct block_response resp = { .status = 0, .bytes_transferred = 0 };
            msg->type = BLOCK_MSG_RESPONSE;
            ipc_msg_set_data(msg, &resp, sizeof(resp));
            ipc_reply(msg);
            break;
        }
        default:
        {
            struct block_response resp = { .status = -1, .bytes_transferred = 0 };
            msg->type = BLOCK_MSG_RESPONSE;
            ipc_msg_set_data(msg, &resp, sizeof(resp));
            ipc_reply(msg);
            break;
        }
    }
}

/**
 * @brief Block server main function
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

    ipc_register_server(BLOCK_SERVER_PORT_NAME, server_port);

    mem_set(devices, 0, sizeof(devices));
    detect_ata_devices();

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
