/**
 * @file ipc_client.h
 * @brief IPC client library for user-space servers
 *
 * This header provides convenience functions for IPC communication
 * between user-space servers in the mexOS microkernel architecture.
 */

#ifndef SERVER_IPC_CLIENT_H
#define SERVER_IPC_CLIENT_H

#include "../../kernel/include/types.h"
#include "../../user/lib/syscall.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Well-known server port IDs \enum server_port_ids
 *
 * These are fixed port IDs for core system servers.
 * Servers register these ports during initialization.
 */
enum server_port_ids
{
    PORT_NAMESERVER = 1,
    PORT_CONSOLE = 2,
    PORT_INPUT = 3,
    PORT_VFS = 4,
    PORT_BLOCK = 5,
    PORT_DEVMGR = 6
};

/**
 * @brief IPC error codes \enum ipc_error
 */
enum ipc_error
{
    IPC_SUCCESS =  0,
    IPC_ERR_INVALID= -1,
    IPC_ERR_FULL = -2,
    IPC_ERR_EMPTY = -3,
    IPC_ERR_TIMEOUT = -4,
    IPC_ERR_NO_PORT = -5,
    IPC_ERR_DENIED = -6
};

/**
 * @brief Initialize the IPC client library
 * @return 0 on success, negative error code on failure
 *
 * This should be called once during server initialization.
 */
int ipc_client_init(void);

/**
 * @brief Send a message and wait for a reply
 * @param port_id Destination port ID
 * @param msg Message to send (will be modified with reply)
 * @return 0 on success, negative error code on failure
 *
 * This is a synchronous call that blocks until a reply is received.
 */
int ipc_call(int port_id, struct message *msg);

/**
 * @brief Send a message without waiting for reply
 * @param port_id Destination port ID
 * @param msg Message to send
 * @return 0 on success, negative error code on failure
 */
int ipc_send_async(int port_id, struct message *msg);

/**
 * @brief Receive a message from a port
 * @param port_id Port ID to receive from
 * @param msg Buffer to store received message
 * @param blocking If true, block until message available
 * @return 0 on success, negative error code on failure
 */
int ipc_receive(int port_id, struct message *msg, bool blocking);

/**
 * @brief Reply to a received message
 * @param msg Reply message (sender field is the destination)
 * @return 0 on success, negative error code on failure
 */
int ipc_reply(struct message *msg);

/**
 * @brief Lookup a server port by name
 * @param name Server name (e.g., "console", "vfs")
 * @return Port ID on success, negative error code on failure
 *
 * @note Currently returns well-known port IDs. In the future,
 *       this will query the nameserver.
 */
int ipc_lookup_server(const char *name);

/**
 * @brief Register this server with the nameserver
 * @param name Server name to register
 * @param port_id Port ID to register
 * @return 0 on success, negative error code on failure
 */
int ipc_register_server(const char *name, int port_id);

/**
 * @brief Create a message with specified type
 * @param msg Message structure to initialize
 * @param type Message type
 */
static inline void ipc_msg_init(struct message *msg, uint32_t type)
{
    msg->sender = 0;
    msg->receiver = 0;
    msg->type = type;
    msg->len = 0;
}

/**
 * @brief Copy data into message payload
 * @param msg Message structure
 * @param data Data to copy
 * @param len Length of data
 * @return 0 on success, -1 if data too large
 */
int ipc_msg_set_data(struct message *msg, const void *data, uint32_t len);

/**
 * @brief Copy data from message payload
 * @param msg Message structure
 * @param data Destination buffer
 * @param max_len Maximum length to copy
 * @return Number of bytes copied
 */
uint32_t ipc_msg_get_data(const struct message *msg, void *data, uint32_t max_len);


#ifdef __cplusplus
}
#endif

#endif // SERVER_IPC_CLIENT_H