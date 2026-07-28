#ifndef SHARED_DISPLAY_ABI_H
#define SHARED_DISPLAY_ABI_H

#include "types.h"

#define DISPLAY_SERVER_PORT 0
#define DISPLAY_TEXT_MAX 196

/**
 * @brief Display server ABI definitions. \enum display_command
 */
enum display_command
{
    DISPLAY_REGISTER_DESKTOP = 1,
    DISPLAY_CREATE_WINDOW = 2,
    DISPLAY_APPEND_TEXT = 3,
    DISPLAY_SET_TITLE = 4,
    DISPLAY_CLOSE_WINDOW = 5,
    DISPLAY_COMMIT_SURFACE = 6,
    DISPLAY_CLEAR_TEXT = 7,
};

/**
 * @brief Display server event definitions. \enum display_event
 */
enum display_event
{
    DISPLAY_EVENT_CREATED = 100,
    DISPLAY_EVENT_KEY = 101,
    DISPLAY_EVENT_CLOSE = 102,
    DISPLAY_EVENT_ACTION = 103,
    DISPLAY_EVENT_CLICK = 104,
};

/**
 * @brief Desktop action definitions. \enum desktop_action
 */
enum desktop_action
{
    DESKTOP_ACTION_TERMINAL = 1,
    DESKTOP_ACTION_SHUTDOWN = 2,
    DESKTOP_ACTION_REBOOT = 3,
    DESKTOP_ACTION_CALCULATOR = 4
};

/**
 * @brief Display packet structure for communication with the display server. \struct display_packet
 */
struct display_packet
{
    uint32_t type;
    uint32_t window_id;
    int32_t event_port;
    int32_t a;
    int32_t b;
    int32_t c;
    int32_t d;
    char text[DISPLAY_TEXT_MAX];
};

#endif
