#include "display.h"
#include "runtime.h"

#define MAX_FILES 64

/**
 * @brief List the contents of a directory and display them in a window.
 * @param window_id The ID of the display window to show the directory contents
 * @param path The path to the directory to list
 */
static void list_directory(const uint32_t window_id, const char* path)
{
    display_clear_text(window_id);

    display_append_text(window_id, "PATH: ");
    display_append_text(window_id, path);
    display_append_text(window_id, "\n");
    display_append_text(window_id, "================================\n");

    struct fs_dirent entries[MAX_FILES];
    user_memset(entries, 0, sizeof(entries));

    const int count = readdir(path, entries, MAX_FILES);

    if (count > 0)
    {
        for (int i = 0; i < count; i++)
        {
            char line[256];
            user_memset(line, 0, sizeof(line));
            int pos = 0;

            if (entries[i].type == 1)
            {
                line[pos++] = '[';
            }

            int j = 0;
            while (entries[i].name[j] && pos < 240)
            {
                line[pos++] = entries[i].name[j++];
            }

            if (entries[i].type == 1)
            {
                line[pos++] = ']';
            }

            if (entries[i].type == 0 && entries[i].size > 0)
            {
                line[pos++] = ' ';
                line[pos++] = '(';

                const int size = entries[i].size;
                if (size >= 1000000)
                {
                    line[pos++] = '0' + (size / 1000000);
                    line[pos++] = 'M';
                }
                else if (size >= 1000)
                {
                    line[pos++] = '0' + (size / 1000);
                    line[pos++] = 'K';
                }
                else
                {
                    if (size >= 100) line[pos++] = '0' + (size / 100);
                    if (size >= 10) line[pos++] = '0' + ((size / 10) % 10);
                    line[pos++] = '0' + (size % 10);
                }
                line[pos++] = ')';
            }

            line[pos++] = '\n';
            display_append_text(window_id, line);
        }
    }
    else
    {
        display_append_text(window_id, "[Empty Directory]\n");
    }

    display_append_text(window_id, "\nCOMMANDS:\n");
    display_append_text(window_id, "  cd <num> - Enter directory\n");
    display_append_text(window_id, "  up - Go to parent\n");
}

/**
 * @brief The main entry point of the file manager application.
 * @return Exit status code.
 */
int main(void)
{
    const int port = display_create_event_port();
    if (port < 0) return 1;

    display_create_window(port, 680, 500, "FILE MANAGER");

    uint32_t window_id = 0;
    bool running = true;
    char current_path[256];
    user_memset(current_path, 0, sizeof(current_path));
    copy_string(current_path, sizeof(current_path), "/");

    while (running)
    {
        struct display_packet event;
        while (display_poll_event(port, &event))
        {
            if (event.type == DISPLAY_EVENT_CREATED)
            {
                window_id = event.window_id;
                list_directory(window_id, current_path);
            }
            else if (event.type == DISPLAY_EVENT_KEY)
            {
                if (event.a == 'u' || event.a == 'U')
                {
                    if (user_strcmp(current_path, "/") != 0)
                    {
                        int len = 0;
                        while (current_path[len])
                        {
                            len++;
                        }
                        while (len > 0 && current_path[len-1] != '/')
                        {
                            len--;
                        }
                        if (len > 1)
                        {
                            len--;
                        }
                        current_path[len] = '\0';
                        list_directory(window_id, current_path);
                    }
                }
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


