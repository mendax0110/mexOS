#include "display.h"
#include "runtime.h"

static void send_key_to_pty(const int pty, const int key)
{
    if (key == 0x80 || key == 0x81 || key == 0x82 || key == 0x83)
    {
        char sequence[3] = { 27, '[', 'A' };
        if (key == 0x81) sequence[2] = 'B';
        if (key == 0x83) sequence[2] = 'C';
        if (key == 0x82) sequence[2] = 'D';
        pty_write(pty, sequence, sizeof(sequence));
        return;
    }
    const char character = (char)key;
    pty_write(pty, &character, 1);
}

int main(void)
{
    const int event_port = display_create_event_port();
    if (event_port < 0) return 1;

    const int pty = pty_create();
    if (pty < 0) return 1;

    const int shell = fork();
    if (shell == 0)
    {
        if (pty_attach_slave(pty) < 0) exit(126);
        const char* argv[] = { "sh", NULL };
        execv("/bin/sh", argv);
        exit(127);
    }
    if (shell < 0) return 1;

    struct display_packet create;
    user_memset(&create, 0, sizeof(create));
    create.type = DISPLAY_CREATE_WINDOW;
    create.event_port = event_port;
    create.a = -1;
    create.c = 680;
    create.d = 420;
    char title[DISPLAY_TEXT_MAX];
    struct user_info info;
    user_memset(&info, 0, sizeof(info));

    if (getuser(&info) >= 0)
    {
        user_strcpy(title, info.username);
        user_strcat(title, ": ");
        user_strcat(title, "mexOS");
    }
    else
    {
        user_strcpy(title, "mexOS");
    }
    user_memcpy(create.text, title, user_strlen(title) + 1);
    display_wait_send(&create);

    uint32_t window_id = 0;
    bool running = true;
    while (running)
    {
        struct display_packet event;
        while (display_poll_event(event_port, &event))
        {
            if (event.type == DISPLAY_EVENT_CREATED)
            {
                window_id = event.window_id;
            }
            else if (event.type == DISPLAY_EVENT_KEY)
            {
                send_key_to_pty(pty, event.a);
            }
            else if (event.type == DISPLAY_EVENT_CLOSE)
            {
                running = false;
            }
        }

        char output[DISPLAY_TEXT_MAX];
        const int bytes = window_id ? pty_read(pty, output, sizeof(output) - 1) : 0;
        if (bytes > 0 && window_id)
        {
            int start = 0;
            for (int i = 0; i < bytes; i++)
            {
                if (output[i] == '\f')
                {
                    if (i > start)
                    {
                        struct display_packet append;
                        user_memset(&append, 0, sizeof(append));
                        append.type = DISPLAY_APPEND_TEXT;
                        append.window_id = window_id;
                        user_memcpy(append.text, output + start, (size_t)(i - start));
                        append.text[i - start] = '\0';
                        display_send(&append);
                    }

                    display_clear_text(window_id);
                    start = i + 1;
                }
            }

            if (start < bytes)
            {
                output[bytes] = '\0';
                struct display_packet append;
                user_memset(&append, 0, sizeof(append));
                append.type = DISPLAY_APPEND_TEXT;
                append.window_id = window_id;
                user_memcpy(append.text, output + start, (size_t)(bytes - start) + 1);
                display_send(&append);

            }
        }

        int status = 0;
        if (waitpid(shell, &status, WAIT_NOHANG) == shell)
        {
            running = false;
        }
        yield();
    }

    kill(shell, 143);
    int status = 0;
    waitpid(shell, &status, 0);
    if (window_id)
    {
        display_close_window(window_id);
    }
    pty_destroy(pty);
    port_destroy(event_port);
    return 0;
}
