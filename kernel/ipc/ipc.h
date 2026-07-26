#ifndef KERNEL_IPC_H
#define KERNEL_IPC_H

#include "../../shared/types.h"
#include "include/config.h"

/**
 * @brief IPC message types
 */
#define MSG_SEND     0
#define MSG_RECEIVE  1
#define MSG_REPLY    2
#define MSG_NOTIFY   3

#define IPC_BLOCK    0x01
#define IPC_NONBLOCK 0x02

/// @brief IPC message structure \struct message
struct message
{
    pid_t   sender;
    pid_t   receiver;
    uint32_t type;
    uint32_t len;
    uint8_t  data[MAX_MSG_SIZE];
};

#define IPC_WAIT_QUEUE_SIZE 8

/// @brief IPC port structure \struct port
struct port
{
    bool     in_use;
    pid_t    owner;
    uint32_t id;
    uint32_t flags;
    struct message* queue;
    uint32_t queue_head;
    uint32_t queue_tail;
    uint32_t queue_size;
    tid_t waiting_senders[IPC_WAIT_QUEUE_SIZE];
    uint32_t waiting_sender_count;
    tid_t waiting_receivers[IPC_WAIT_QUEUE_SIZE];
    uint32_t waiting_receiver_count;
};

/**
 * @brief Initialize the IPC subsystem
 */
void ipc_init(void);

/**
 * @brief Create a new IPC port
 * @param owner The PID of the port owner
 * @return The port ID on success, -1 on failure
 */
int port_create(pid_t owner);

/**
 * @brief Destroy an IPC port
 * @param port_id The ID of the port to destroy
 * @return 0 on success, -1 on failure
 */
int port_destroy(int port_id);

/**
 * @brief Send a message to an IPC port
 * @param port_id The ID of the destination port
 * @param msg Pointer to the message to send
 * @param flags Message sending flags
 * @return 0 on success, -1 on failure
 */
int msg_send(int port_id, struct message* msg, uint32_t flags);

/**
 * @brief Receive a message from an IPC port
 * @param port_id The ID of the port to receive from
 * @param msg Pointer to the message structure to fill
 * @param flags Message receiving flags
 * @return 0 on success, -1 on failure
 */
int msg_receive(int port_id, struct message* msg, uint32_t flags);

/**
 * @brief Reply to a received message
 * @param dest The PID of the message sender
 * @param msg Pointer to the reply message
 * @return 0 on success, -1 on failure
 */
int msg_reply(pid_t dest, struct message* msg);

/**
 * @brief Helper to check if a given port is owned by a specific process
 * @param port_id The port to check
 * @param pid The process ID to check against
 * @return true if the port is owned by the process, false otherwise
 */
bool port_owned_by(int port_id, pid_t pid);

/**
 * @brief Performs process cleanup of a given process
 * @param pid The process to clean up
 */
void ipc_process_cleanup(pid_t pid);

#endif
