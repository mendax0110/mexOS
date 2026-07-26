#include "display.h"

static void launch_terminal(void)
{
    const int child = fork();
    if (child == 0)
    {
        const char* argv[] = { "terminal", NULL };
        execv("/bin/terminal", argv);
        exit(127);
    }
}

static void launch_calculator(void)
{
    const int child = fork();
    if (child == 0)
    {
        const char* argv[] = { "calc", NULL };
        execv("/bin/calc", argv);
        exit(127);
    }
}

int main(void)
{
    const int event_port = display_create_event_port();
    if (event_port < 0) return 1;

    struct display_packet registration;
    user_memset(&registration, 0, sizeof(registration));
    registration.type = DISPLAY_REGISTER_DESKTOP;
    registration.event_port = event_port;
    display_wait_send(&registration);

    while (1)
    {
        struct display_packet event;
        if (display_poll_event(event_port, &event))
        {
            if (event.type == DISPLAY_EVENT_ACTION)
            {
                if (event.a == DESKTOP_ACTION_TERMINAL) launch_terminal();
                if (event.a == DESKTOP_ACTION_REBOOT) power_control(POWER_REBOOT);
                if (event.a == DESKTOP_ACTION_SHUTDOWN) power_control(POWER_SHUTDOWN);
                if (event.a == DESKTOP_ACTION_CALCULATOR) launch_calculator();
            }
        }

        int status = 0;
        while (waitpid(-1, &status, WAIT_NOHANG) > 0)
        {
        }
        yield();
    }
}
