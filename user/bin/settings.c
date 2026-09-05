#include "display.h"
#include "runtime.h"

/**
 * @brief Append a label and a formatted number to the display window.
 * @param window_id The ID of the display window to append text to
 * @param label The label to display before the number
 * @param value The number to format and display
 */
static void append_number(const uint32_t window_id, const char* label, const int value)
{
    char text[128];
    user_memset(text, 0, sizeof(text));
    copy_string(text, sizeof(text), label);

    if (value >= 1000000)
    {
        append_uint_dec(text, sizeof(text), (uint32_t)(value / 1000000));
        append_string(text, sizeof(text), "M");
    }
    else if (value >= 1000)
    {
        append_uint_dec(text, sizeof(text), (uint32_t)(value / 1000));
        append_string(text, sizeof(text), "K");
    }
    else
    {
        append_uint_dec(text, sizeof(text), (uint32_t)value);
    }
    append_string(text, sizeof(text), "\n");

    display_append_text(window_id, text);
}

/**
 * @brief The main entry point of the settings application.
 * @return Exit status code.
 */
int main(void)
{
    const int port = display_create_event_port();
    if (port < 0) return 1;

    display_create_window(port, 640, 480, "SETTINGS");

    uint32_t window_id = 0;
    bool running = true;

    while (running)
    {
        struct display_packet event;
        while (display_poll_event(port, &event))
        {
            if (event.type == DISPLAY_EVENT_CREATED)
            {
                window_id = event.window_id;

                display_append_text(window_id, "===== SYSTEM SETTINGS =====\n\n");

                struct user_info user;
                user_memset(&user, 0, sizeof(user));
                if (getuser(&user) >= 0)
                {
                    display_append_text(window_id, "USER:\n");
                    display_append_text(window_id, user.username);
                    display_append_text(window_id, "\n");
                    if (user.is_admin)
                    {
                        display_append_text(window_id, "Admin: YES\n");
                    }
                    display_append_text(window_id, "\n");
                }

                struct system_info sys;
                user_memset(&sys, 0, sizeof(sys));
                sysinfo(&sys);

                display_append_text(window_id, "MEMORY:\n");
                append_number(window_id, "  Total: ", (int)sys.total_memory_kb);
                append_number(window_id, "  Used:  ", (int)sys.used_memory_kb);
                append_number(window_id, "  Free:  ", (int)sys.free_memory_kb);
                display_append_text(window_id, "\n");

                display_append_text(window_id, "UPTIME:\n");
                const uint32_t uptime_secs = sys.uptime_ticks / 100;
                const uint32_t hours = uptime_secs / 3600;
                const uint32_t mins = (uptime_secs % 3600) / 60;
                const uint32_t secs = uptime_secs % 60;

                char uptime_text[64];
                user_memset(uptime_text, 0, sizeof(uptime_text));
                append_uint_dec(uptime_text, sizeof(uptime_text), hours / 10);
                append_uint_dec(uptime_text, sizeof(uptime_text), hours % 10);
                append_string(uptime_text, sizeof(uptime_text), ":");
                append_uint_dec(uptime_text, sizeof(uptime_text), mins / 10);
                append_uint_dec(uptime_text, sizeof(uptime_text), mins % 10);
                append_string(uptime_text, sizeof(uptime_text), ":");
                append_uint_dec(uptime_text, sizeof(uptime_text), secs / 10);
                append_uint_dec(uptime_text, sizeof(uptime_text), secs % 10);
                append_string(uptime_text, sizeof(uptime_text), "\n");
                display_append_text(window_id, uptime_text);

                display_append_text(window_id, "\nDISPLAY:\n");
                display_append_text(window_id, "  Resolution: ");
                append_number(window_id, "", event.a);
                display_append_text(window_id, "x");
                append_number(window_id, "", event.b);
                display_append_text(window_id, "\n");
                display_append_text(window_id, "  Color Depth: ");
                append_number(window_id, "", event.c);
                display_append_text(window_id, "-bit\n");
            }
            else if (event.type == DISPLAY_EVENT_CLOSE)
            {
                running = false;
            }
        }
        yield();
    }

    return 0;
}

