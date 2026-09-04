#include "display.h"
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
                    char pid_str[16];
                    user_memset(pid_str, 0, sizeof(pid_str));
                    int i = 0;
                    while (i < 15 && event.a >= '0' && event.a <= '9')
                    {
                        pid_str[i++] = event.a;
                        break;
                    }
                    pid_str[i] = '\0';

                    int pid = 0;
                    for (int j = 0; pid_str[j]; j++)
                    {
                        pid = pid * 10 + (pid_str[j] - '0');
                    }

                    if (pid > 0)
                    {
                        kill(pid, 0);
                        char msg_text[64];
                        user_memset(msg_text, 0, sizeof(msg_text));
                        user_strcpy(msg_text, "Killed PID ");
                        int pos = 11;
                        if (pid >= 100) msg_text[pos++] = '0' + (pid / 100);
                        if (pid >= 10) msg_text[pos++] = '0' + ((pid / 10) % 10);
                        msg_text[pos++] = '0' + (pid % 10);
                        msg_text[pos++] = '\n';
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

                    int pos = 0;
                    int pid = procs[i].pid;
                    if (pid >= 10000)
                    {
                        proc_line[pos++] = '0' + (pid / 10000);
                    }
                    if (pid >= 1000)
                    {
                        proc_line[pos++] = '0' + ((pid / 1000) % 10);
                    }
                    if (pid >= 100)
                    {
                        proc_line[pos++] = '0' + ((pid / 100) % 10);
                    }
                    if (pid >= 10)
                    {
                        proc_line[pos++] = '0' + ((pid / 10) % 10);
                    }
                    proc_line[pos++] = '0' + (pid % 10);
                    proc_line[pos++] = ' ';

                    int name_idx = 0;
                    while (procs[i].name[name_idx] && pos < 120)
                    {
                        proc_line[pos++] = procs[i].name[name_idx++];
                    }

                    proc_line[pos++] = '\n';
                    display_append_text(window_id, proc_line);
                }
            }
        }

        yield();
    }

    return 0;
}


