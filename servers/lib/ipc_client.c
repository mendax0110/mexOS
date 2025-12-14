#include "ipc_client.h"
#include "memory.h"


static int reply_port = -1;

static int str_compare(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int ipc_client_init(void)
{
    reply_port = port_create();
    if (reply_port < 0)
    {
        return IPC_ERR_INVALID;
    }
    return IPC_SUCCESS;
}

int ipc_call(int port_id, struct message *msg)
{
    if (port_id < 0 || msg == NULL)
    {
        return IPC_ERR_INVALID;
    }

    if (reply_port < 0)
    {
        int ret = ipc_client_init();
        if (ret != IPC_SUCCESS)
        {
            return ret;
        }
    }

    msg->sender = (pid_t)reply_port;

    int ret = send(port_id, msg, IPC_BLOCK);
    if (ret != 0)
    {
        return IPC_ERR_FULL;
    }

    ret = recv(reply_port, msg, IPC_BLOCK);
    if (ret != 0)
    {
        return IPC_ERR_EMPTY;
    }

    return IPC_SUCCESS;
}

int ipc_send_async(int port_id, struct message *msg)
{
    if (port_id < 0 || msg == NULL)
    {
        return IPC_ERR_INVALID;
    }

    int ret = send(port_id, msg, IPC_NONBLOCK);
    if (ret == -2)
    {
        return IPC_ERR_FULL;
    }
    else if (ret != 0)
    {
        return IPC_ERR_INVALID;
    }

    return IPC_SUCCESS;
}

int ipc_receive(int port_id, struct message *msg, bool blocking)
{
    if (port_id < 0 || msg == NULL)
    {
        return IPC_ERR_INVALID;
    }

    int flags = blocking ? IPC_BLOCK : IPC_NONBLOCK;
    int ret = recv(port_id, msg, flags);

    if (ret == -2)
    {
        return IPC_ERR_EMPTY;
    }
    else if (ret != 0)
    {
        return IPC_ERR_INVALID;
    }

    return IPC_SUCCESS;
}

int ipc_reply(struct message *msg)
{
    if (msg == NULL)
    {
        return IPC_ERR_INVALID;
    }

    int reply_to = (int)msg->sender;

    pid_t tmp = msg->sender;
    msg->sender = msg->receiver;
    msg->receiver = tmp;

    return ipc_send_async(reply_to, msg);
}

int ipc_lookup_server(const char *name)
{
    if (name == NULL)
    {
        return IPC_ERR_INVALID;
    }

    /* TODO: Query nameserver */

    if (str_compare(name, "console") == 0)
    {
        return PORT_CONSOLE;
    }
    else if (str_compare(name, "input") == 0)
    {
        return PORT_INPUT;
    }
    else if (str_compare(name, "vfs") == 0)
    {
        return PORT_VFS;
    }
    else if (str_compare(name, "block") == 0)
    {
        return PORT_BLOCK;
    }
    else if (str_compare(name, "devmgr") == 0)
    {
        return PORT_DEVMGR;
    }

    return IPC_ERR_NO_PORT;
}

int ipc_register_server(const char *name, int port_id)
{
    /* TODO: Send registration to nameserver */
    (void)name;
    (void)port_id;
    return IPC_SUCCESS;
}

int ipc_msg_set_data(struct message *msg, const void *data, uint32_t len)
{
    if (msg == NULL || data == NULL)
    {
        return -1;
    }

    if (len > MAX_MSG_SIZE)
    {
        return -1;
    }

    mem_copy(msg->data, data, len);
    msg->len = len;
    return 0;
}

uint32_t ipc_msg_get_data(const struct message *msg, void *data, uint32_t max_len)
{
    if (msg == NULL || data == NULL)
    {
        return 0;
    }

    uint32_t copy_len = msg->len;
    if (copy_len > max_len)
    {
        copy_len = max_len;
    }

    mem_copy(data, msg->data, copy_len);
    return copy_len;
}