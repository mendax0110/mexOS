#ifndef USER_DISPLAY_H
#define USER_DISPLAY_H

#include "runtime.h"
#include "../shared/display_abi.h"

/**
 * @brief Sends a display packet to the display server.
 * @param packet The packet to send
 * @return 0 on success, or a negative error code on failure
 */
static inline int display_send(const struct display_packet* packet)
{
    struct message message;
    user_memset(&message, 0, sizeof(message));
    message.sender = getpid();
    message.type = packet->type;
    message.len = sizeof(*packet);
    user_memcpy(message.data, packet, sizeof(*packet));
    return send(DISPLAY_SERVER_PORT, &message, IPC_NONBLOCK);
}

/**
 * @brief Polls for a display event from the display server.
 * @param event_port The event port to poll
 * @param event The display packet
 * @return 0 on success, or a negative error code on failure
 */
static inline int display_poll_event(const int event_port, struct display_packet* event)
{
    struct message message;
    if (!event || recv(event_port, &message, IPC_NONBLOCK) < 0 ||
        message.len < sizeof(*event))
    {
        return 0;
    }
    user_memcpy(event, message.data, sizeof(*event));
    return 1;
}

/**
 * @brief Creates a event port
 * @return 0 on success, or a negative error code on failure
 */
static inline int display_create_event_port(void)
{
    return port_create();
}

/**
 * @brief Checks display send, if it's smaller than zero, we yield.
 * @param packet The packet to send
 */
static inline void display_wait_send(const struct display_packet* packet)
{
    while (display_send(packet) < 0) yield();
}

#endif
