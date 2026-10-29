#include "../lib/ui/display.h"
#include "runtime.h"

/**
 * @brief The main function of the task manager application.
 * It creates a window to display running processes and allows the user to refresh the list or kill processes by PID.
 * @return Exit status code
 */
int main(void)
{
    const int port = display_create_event_port();
    if (port < 0) return 1;

    display_create_window(port, 680, 520, "TASK MANAGER");

    uint32_t window_id = 0;
    bool running = true;
    uint64_t last_update = 0;

    while (running)
    {
        struct display_packet event;
        while (display_poll_event(port, &event))
        {
            if (event.type == DISPLAY_EVENT_CREATED)
            {
                window_id = event.window_id;
                display_append_text(window_id, "RUNNING PROCESSES (PID NAME CPU%)\n");
                display_append_text(window_id, "================================\n");
            }
            else if (event.type == DISPLAY_EVENT_KEY)
            {
                if (event.a == 'r' || event.a == 'R')
                {
                    display_clear_text(window_id);

                    display_append_text(window_id, "RUNNING PROCESSES (PID NAME CPU%)\n");
                    display_append_text(window_id, "================================\n");
                }
                else if (event.a >= '0' && event.a <= '9')
                {
                    int pid = event.a - '0';

                    if (pid > 0)
                    {
                        kill(pid, 0);
                        char msg_text[64];
                        user_memset(msg_text, 0, sizeof(msg_text));
                        copy_string(msg_text, sizeof(msg_text), "Killed PID ");
                        append_uint_dec(msg_text, sizeof(msg_text), (uint32_t)pid);
                        append_string(msg_text, sizeof(msg_text), "\n");
                        display_append_text(window_id, msg_text);
                    }
                }
            }
            else if (event.type == DISPLAY_EVENT_CLOSE)
            {
                running = false;
            }
        }

        uint32_t ticks_low;
        uint32_t ticks_high;
        ASM_V("rdtsc" : "=a"(ticks_low), "=d"(ticks_high));
        uint64_t ticks = ((uint64_t)ticks_high << 32) | ticks_low;

        if (window_id > 0 && ticks - last_update > 3000000000UL)
        {
            last_update = ticks;

            struct process_info procs[64];
            int count = getprocs(procs, 64);

            if (count > 0)
            {
                display_clear_text(window_id);

                display_append_text(window_id, "RUNNING PROCESSES (PID NAME CPU%)\n");
                display_append_text(window_id, "================================\n");

                for (int i = 0; i < count; i++)
                {
                    char proc_line[128];
                    user_memset(proc_line, 0, sizeof(proc_line));

                    int pid = procs[i].pid;
                    append_uint_dec(proc_line, sizeof(proc_line), (uint32_t)pid);
                    append_string(proc_line, sizeof(proc_line), " ");

                    size_t pos = user_strlen(proc_line);
                    int name_idx = 0;
                    while (procs[i].name[name_idx] && pos + 1 < sizeof(proc_line))
                    {
                        proc_line[pos++] = procs[i].name[name_idx++];
                    }

                    proc_line[pos++] = '\n';
                    proc_line[pos] = '\0';
                    display_append_text(window_id, proc_line);
                }
            }
        }

        yield();
    }

    return 0;
}
