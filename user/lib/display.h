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
 * @brief Creates an event port
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

/**
 * @brief Creates a display packet for the specified type and window ID.
 * @param type The type of the display packet
 * @param window_id The ID of the window
 * @return The created display packet
 */
static inline struct display_packet display_packet_for(const uint16_t type, const uint32_t window_id)
{
    struct display_packet packet;
    user_memset(&packet, 0, sizeof(packet));
    packet.type = type;
    packet.window_id = window_id;
    return packet;
}

/**
 * @brief Closes a display window
 * @param window_id The window to close
 */
static inline void display_close_window(const uint32_t window_id)
{
    const struct display_packet _msg = display_packet_for(DISPLAY_CLOSE_WINDOW, window_id);
    display_send(&_msg);
}

/**
 * @brief Clears a display window's text
 * @param window_id The window to clear
 */
static inline void display_clear_text(const uint32_t window_id)
{
    const struct display_packet _msg = display_packet_for(DISPLAY_CLEAR_TEXT, window_id);
    display_send(&_msg);
}

/**
 * @brief Appends text to a display window.
 * @param window_id The ID of the window to append text to
 * @param text The text to append
 */
static inline void display_append_text(const uint32_t window_id, const char* text)
{
    if (!text) return;
    struct display_packet _msg = display_packet_for(DISPLAY_APPEND_TEXT, window_id);
    user_strcpy(_msg.text, text);
    display_wait_send(&_msg);
}

/**
 * @brief Sets the title of a display window
 * @param event_port The event port of the window
 * @param width The width of the window
 * @param height The height of the window
 * @param title The title of the window
 */
static inline void display_create_window(const int event_port, const uint32_t width, const uint32_t height, const char* title)
{
    struct display_packet _msg = display_packet_for(DISPLAY_CREATE_WINDOW, 0);
    _msg.event_port = event_port;
    _msg.c = width;
    _msg.d = height;
    user_strcpy(_msg.text, title);
    display_wait_send(&_msg);
}

/**
 * @brief Registers a desktop with the display server.
 * @param event_port The event port to register for desktop events
 */
static inline void display_register_desktop(const int event_port)
{
    struct display_packet _msg = display_packet_for(DISPLAY_REGISTER_DESKTOP, 0);
    _msg.event_port = event_port;
    display_wait_send(&_msg);
}

#endif
