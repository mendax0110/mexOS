#include "../lib/ui/display.h"

/**
 * @brief Launch an application by forking a new process and executing the specified binary.
 * @param path The path to the binary to execute
 * @param name The name of the application (used as argv[0])
 */
static void do_launch_app(const char* path, const char* name)
{
    const int child = fork();
    if (child == 0)
    {
        const char* argv[] = { name, NULL };
        execv(path, argv);
        exit(127);
    }
}

/**
 * @brief Structure to map applet names to their corresponding binary paths and desktop actions. \struct applet_map
 */
struct applet_map
{
    const char* name;
    const char* path;
    enum desktop_action action;
};

/**
 * @brief Array of applet mappings, associating applet names with their binary paths and desktop actions. \var applet_map
 */
static const struct applet_map applet_map[] = {
	{ .name = "terminal",	.path = "/bin/terminal",	.action = DESKTOP_ACTION_TERMINAL },
	{ .name = "calculator",	.path = "/bin/calc",		.action = DESKTOP_ACTION_CALCULATOR },
	{ .name = "filemgr",		.path = "/bin/filemgr",		.action = DESKTOP_ACTION_FILE_MANAGER },
	{ .name = "taskman",		.path = "/bin/taskman",		.action = DESKTOP_ACTION_TASK_MANAGER },
	{ .name = "settings",	.path = "/bin/settings",	.action = DESKTOP_ACTION_SETTINGS },
};

#define APPLET_NUM sizeof(applet_map) / sizeof(applet_map[0])

/**
 * @brief The main entry point of the desktop application.
 * @return Exit status code.
 */
int main(void)
{
    const int event_port = display_create_event_port();
    if (event_port < 0) return 1;

    display_register_desktop(event_port);

    while (1)
    {
        struct display_packet event;
        if (display_poll_event(event_port, &event))
        {
            if (event.type == DISPLAY_EVENT_ACTION)
            {
                for (size_t i = 0; i < APPLET_NUM; i++)
                {
                    if (applet_map[i].action == event.a)
                    {
                        do_launch_app(applet_map[i].path, applet_map[i].name);
                        break;
                    }
                }
            }
        }

        int status = 0;
        while (waitpid(-1, &status, WAIT_NOHANG) > 0) { }
        yield();
    }
}
