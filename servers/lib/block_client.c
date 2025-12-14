#include "block_client.h"
#include "ipc_client.h"
#include "memory.h"
#include "../../include/protocols/block_protocol.h"

static int block_server_port = -1;

int block_client_init(void)
{
    block_server_port = ipc_lookup_server(BLOCK_SERVER_PORT_NAME);
    if (block_server_port < 0)
    {
        block_server_port = PORT_BLOCK;
    }
    return ipc_client_init();
}

int block_drive_exists(uint8_t drive)
{
    struct message msg;
    struct block_info_request req;
    struct block_info_response resp;

    req.device_id = drive;

    ipc_msg_init(&msg, BLOCK_MSG_GET_INFO);
    ipc_msg_set_data(&msg, &req, sizeof(req));

    int ret = ipc_call(block_server_port, &msg);
    if (ret != IPC_SUCCESS)
    {
        return 0;
    }

    ipc_msg_get_data(&msg, &resp, sizeof(resp));
    return resp.status == 0 ? 1 : 0;
}

int block_read_sectors(uint8_t drive, uint32_t lba, uint8_t count, void* buffer)
{
    struct message msg;
    struct block_read_request req;
    struct block_response resp;

    req.device_id = drive;
    req.lba = lba;
    req.count = count;
    req.buffer_addr = (uint32_t)(uintptr_t)buffer;

    ipc_msg_init(&msg, BLOCK_MSG_READ);
    ipc_msg_set_data(&msg, &req, sizeof(req));

    int ret = ipc_call(block_server_port, &msg);
    if (ret != IPC_SUCCESS)
    {
        return -1;
    }

    ipc_msg_get_data(&msg, &resp, sizeof(resp));
    return resp.status;
}

int block_write_sectors(uint8_t drive, uint32_t lba, uint8_t count, const void* buffer)
{
    struct message msg;
    struct block_write_request req;
    struct block_response resp;

    req.device_id = drive;
    req.lba = lba;
    req.count = count;
    req.buffer_addr = (uint32_t)(uintptr_t)buffer;

    ipc_msg_init(&msg, BLOCK_MSG_WRITE);
    ipc_msg_set_data(&msg, &req, sizeof(req));

    int ret = ipc_call(block_server_port, &msg);
    if (ret != IPC_SUCCESS)
    {
        return -1;
    }

    ipc_msg_get_data(&msg, &resp, sizeof(resp));
    return resp.status;
}

int block_get_info(uint8_t drive, uint32_t* sector_size, uint32_t* sector_count)
{
    struct message msg;
    struct block_info_request req;
    struct block_info_response resp;

    req.device_id = drive;

    ipc_msg_init(&msg, BLOCK_MSG_GET_INFO);
    ipc_msg_set_data(&msg, &req, sizeof(req));

    int ret = ipc_call(block_server_port, &msg);
    if (ret != IPC_SUCCESS)
    {
        return -1;
    }

    ipc_msg_get_data(&msg, &resp, sizeof(resp));

    if (resp.status == 0)
    {
        if (sector_size) *sector_size = resp.sector_size;
        if (sector_count) *sector_count = resp.sector_count;
    }

    return resp.status;
}
