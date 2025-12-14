#include "editor.h"
#include "../console/console.h"
#include "../input/keyboard.h"
#include "../../servers/vfs/fs.h"
#include "basic.h"
#include "include/string.h"

static editor_state_t editor_state;

void editor_init(void)
{
    memset(&editor_state, 0, sizeof(editor_state_t));
    editor_state.mode = EDITOR_MODE_TEXT;
    editor_state.running = 0;
    editor_state.modified = 0;
}

static void editor_clear_screen(void)
{
    console_clear();
}

static void editor_draw_header(void)
{
    console_write("=== mexOS Editor: ");
    console_write(editor_state.filename);

    if (editor_state.mode == EDITOR_MODE_TEXT)
    {
        console_write(" [TEXT]");
    }
    else if (editor_state.mode == EDITOR_MODE_BASIC)
    {
        console_write(" [BASIC]");
    }
    else if (editor_state.mode == EDITOR_MODE_HEX)
    {
        console_write(" [HEX]");
    }

    if (editor_state.modified)
    {
        console_write(" *");
    }

    console_write(" ===\n");
}

static void editor_draw_status(void)
{
    console_write("---\n");

    if (editor_state.mode == EDITOR_MODE_TEXT)
    {
        console_write(":q quit | :w save | :wq save+quit | :d delete line | :p print | :h help\n");
    }
    else if (editor_state.mode == EDITOR_MODE_BASIC)
    {
        console_write("Enter line numbers to add code | RUN | LIST | :q quit | :w save | :h help\n");
    }
    else if (editor_state.mode == EDITOR_MODE_HEX)
    {
        console_write(":q quit | :w save | :wq save+quit | :h help\n");
    }
}

void editor_show_help(void)
{
    console_write("\n=== Editor Help ===\n");

    if (editor_state.mode == EDITOR_MODE_TEXT)
    {
        console_write("TEXT MODE:\n");
        console_write("  :q          - Quit editor\n");
        console_write("  :w          - Save file\n");
        console_write("  :wq         - Save and quit\n");
        console_write("  :d          - Delete last line\n");
        console_write("  :p          - Print buffer\n");
        console_write("  :mode basic - Switch to BASIC mode\n");
        console_write("  :mode hex   - Switch to HEX mode\n");
        console_write("  :h          - Show this help\n");
        console_write("\nType text and press Enter to add lines\n");
    }
    else if (editor_state.mode == EDITOR_MODE_BASIC)
    {
        console_write("BASIC MODE:\n");
        console_write("  RUN         - Execute BASIC program\n");
        console_write("  LIST        - List program lines\n");
        console_write("  CLEAR       - Clear program\n");
        console_write("  PRINT expr  - Print expression\n");
        console_write("  LET var=val - Assign variable (A-Z)\n");
        console_write("  10 PRINT..  - Add numbered line\n");
        console_write("  :q          - Quit editor\n");
        console_write("  :w          - Save program\n");
        console_write("  :mode text  - Switch to TEXT mode\n");
        console_write("  :h          - Show this help\n");
    }
    else if (editor_state.mode == EDITOR_MODE_HEX)
    {
        console_write("HEX MODE:\n");
        console_write("  :q          - Quit editor\n");
        console_write("  :w          - Save file\n");
        console_write("  :wq         - Save and quit\n");
        console_write("  :mode text  - Switch to TEXT mode\n");
        console_write("  :h          - Show this help\n");
        console_write("\nHex viewer (read-only in this version)\n");
    }

    console_write("\nPress any key to continue...\n");
    keyboard_getchar();
}

int editor_open(const char* filename, const uint8_t mode)
{
    if (!filename)
    {
        return -1;
    }

    strncpy(editor_state.filename, filename, 127);
    editor_state.filename[127] = '\0';
    editor_state.mode = mode;
    editor_state.modified = 0;
    editor_state.buffer_size = 0;
    memset(editor_state.buffer, 0, EDITOR_MAX_FILE_SIZE);

    if (fs_exists(filename))
    {
        if (fs_is_dir(filename))
        {
            console_write("editor: is a directory\n");
            return -1;
        }

        int file_len = fs_read(filename, editor_state.buffer, EDITOR_MAX_FILE_SIZE - 1);
        if (file_len < 0)
        {
            file_len = 0;
        }
        editor_state.buffer[file_len] = '\0';
        editor_state.buffer_size = file_len;
    }
    else
    {
        const int ret = fs_create_file(filename);
        if (ret != FS_ERR_OK)
        {
            console_write("editor: cannot create file\n");
            return -1;
        }
    }

    return 0;
}

int editor_save(void)
{
    const int ret = fs_write(editor_state.filename, editor_state.buffer,
                       (uint32_t)strlen(editor_state.buffer));

    if (ret == FS_ERR_OK)
    {
        editor_state.modified = 0;
        console_write("Saved\n");
        return 0;
    }
    else
    {
        console_write("Error: Failed to save file\n");
        return -1;
    }
}

static void editor_delete_last_line(void)
{
    const uint32_t len = (uint32_t)strlen(editor_state.buffer);

    if (len > 0)
    {
        uint32_t last_nl = len;
        if (editor_state.buffer[len - 1] == '\n' && len > 1)
        {
            last_nl = len - 1;
        }

        while (last_nl > 0 && editor_state.buffer[last_nl - 1] != '\n')
        {
            last_nl--;
        }

        editor_state.buffer[last_nl] = '\0';
        editor_state.buffer_size = last_nl;
        editor_state.modified = 1;
        console_write("Line deleted\n");
    }
    else
    {
        console_write("Buffer empty\n");
    }
}

static void editor_print_buffer(void)
{
    console_write("---\n");
    if (strlen(editor_state.buffer) > 0)
    {
        console_write(editor_state.buffer);
        if (editor_state.buffer[strlen(editor_state.buffer) - 1] != '\n')
        {
            console_write("\n");
        }
    }
    console_write("---\n");
}

static void editor_add_line(const char* line)
{
    const uint32_t buf_len = (uint32_t)strlen(editor_state.buffer);
    const uint32_t line_len = (uint32_t)strlen(line);

    if (buf_len + line_len + 2 < EDITOR_MAX_FILE_SIZE)
    {
        strcat(editor_state.buffer, line);
        strcat(editor_state.buffer, "\n");
        editor_state.buffer_size = buf_len + line_len + 1;
        editor_state.modified = 1;
    }
    else
    {
        console_write("Buffer full\n");
    }
}

void editor_set_mode(const uint8_t mode)
{
    if (mode != editor_state.mode)
    {
        editor_state.mode = mode;
        editor_state.modified = 1;

        if (mode == EDITOR_MODE_BASIC)
        {
            basic_clear_program();

            const char* line = editor_state.buffer;
            char temp_line[EDITOR_LINE_SIZE];

            while (*line)
            {
                uint32_t i = 0;
                while (*line && *line != '\n' && i < EDITOR_LINE_SIZE - 1)
                {
                    temp_line[i++] = *line++;
                }
                temp_line[i] = '\0';

                if (*line == '\n')
                {
                    line++;
                }

                const char* ptr = temp_line;
                while (*ptr == ' ') ptr++;

                if (*ptr >= '0' && *ptr <= '9')
                {
                    uint32_t line_num = 0;
                    while (*ptr >= '0' && *ptr <= '9')
                    {
                        line_num = line_num * 10 + (*ptr - '0');
                        ptr++;
                    }

                    while (*ptr == ' ') ptr++;
                    basic_add_line(line_num, ptr);
                }
            }

            console_write("Switched to BASIC mode\n");
        }
        else if (mode == EDITOR_MODE_TEXT)
        {
            console_write("Switched to TEXT mode\n");
        }
        else if (mode == EDITOR_MODE_HEX)
        {
            console_write("Switched to HEX mode\n");
        }
    }
}

void editor_run_basic(void)
{
    if (editor_state.mode != EDITOR_MODE_BASIC)
    {
        console_write("Not in BASIC mode\n");
        return;
    }

    console_write("\n=== Running BASIC Program ===\n");
    basic_run_program();
    console_write("\n=== Program Finished ===\n");
}

void editor_list_basic(void)
{
    if (editor_state.mode != EDITOR_MODE_BASIC)
    {
        console_write("Not in BASIC mode\n");
        return;
    }

    basic_list_program();
}

static int editor_handle_command(void)
{
    const char* cmd = editor_state.line_buffer;

    if (*cmd == ':')
    {
        cmd++;
    }

    if (strcmp(cmd, "q") == 0)
    {
        if (editor_state.modified)
        {
            console_write("Warning: unsaved changes (use :q! to force quit)\n");
            return 0;
        }
        return EDITOR_CMD_QUIT;
    }
    else if (strcmp(cmd, "q!") == 0)
    {
        return EDITOR_CMD_QUIT;
    }
    else if (strcmp(cmd, "w") == 0)
    {
        editor_save();
        return 0;
    }
    else if (strcmp(cmd, "wq") == 0)
    {
        editor_save();
        return EDITOR_CMD_QUIT;
    }
    else if (strcmp(cmd, "d") == 0)
    {
        editor_delete_last_line();
        return 0;
    }
    else if (strcmp(cmd, "p") == 0)
    {
        editor_print_buffer();
        return 0;
    }
    else if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0)
    {
        editor_show_help();
        return 0;
    }
    else if (strncmp(cmd, "mode ", 5) == 0)
    {
        cmd += 5;
        while (*cmd == ' ') cmd++;

        if (strcmp(cmd, "text") == 0)
        {
            editor_set_mode(EDITOR_MODE_TEXT);
        }
        else if (strcmp(cmd, "basic") == 0)
        {
            editor_set_mode(EDITOR_MODE_BASIC);
        }
        else if (strcmp(cmd, "hex") == 0)
        {
            editor_set_mode(EDITOR_MODE_HEX);
        }
        else
        {
            console_write("Unknown mode\n");
        }
        return 0;
    }
    else
    {
        console_write("Unknown command (type :h for help)\n");
        return 0;
    }
}

void editor_run(void)
{
    editor_state.running = 1;

    editor_clear_screen();
    editor_draw_header();
    editor_draw_status();
    console_write("---\n");

    if (editor_state.buffer_size > 0)
    {
        console_write(editor_state.buffer);
        if (editor_state.buffer[editor_state.buffer_size - 1] != '\n')
        {
            console_write("\n");
        }
    }

    console_write("---\n> ");

    while (editor_state.running)
    {
        uint32_t pos = 0;
        memset(editor_state.line_buffer, 0, EDITOR_LINE_SIZE);

        while (1)
        {
            const char c = keyboard_getchar();

            if (c == '\n')
            {
                console_putchar('\n');
                editor_state.line_buffer[pos] = '\0';
                break;
            }
            else if (c == '\b')
            {
                if (pos > 0)
                {
                    pos--;
                    console_putchar('\b');
                    console_putchar(' ');
                    console_putchar('\b');
                }
            }
            else if (c >= 0x20 && c < 0x7F && pos < EDITOR_LINE_SIZE - 1)
            {
                editor_state.line_buffer[pos++] = c;
                console_putchar(c);
            }
        }

        if (editor_state.line_buffer[0] == ':')
        {
            const int cmd_result = editor_handle_command();
            if (cmd_result == EDITOR_CMD_QUIT)
            {
                editor_state.running = 0;
                break;
            }
        }
        else if (editor_state.mode == EDITOR_MODE_BASIC)
        {
            if (strncmp(editor_state.line_buffer, "RUN", 3) == 0)
            {
                editor_run_basic();
            }
            else if (strncmp(editor_state.line_buffer, "LIST", 4) == 0)
            {
                editor_list_basic();
            }
            else if (strncmp(editor_state.line_buffer, "CLEAR", 5) == 0)
            {
                basic_clear_program();
                memset(editor_state.buffer, 0, EDITOR_MAX_FILE_SIZE);
                editor_state.buffer_size = 0;
                editor_state.modified = 1;
                console_write("Program cleared\n");
            }
            else
            {
                const char* ptr = editor_state.line_buffer;
                while (*ptr == ' ') ptr++;

                if (*ptr >= '0' && *ptr <= '9')
                {
                    uint32_t line_num = 0;
                    while (*ptr >= '0' && *ptr <= '9')
                    {
                        line_num = line_num * 10 + (*ptr - '0');
                        ptr++;
                    }

                    while (*ptr == ' ') ptr++;

                    if (basic_add_line(line_num, ptr) == 0)
                    {
                        editor_add_line(editor_state.line_buffer);
                    }
                    else
                    {
                        console_write("Error: Program full\n");
                    }
                }
                else
                {
                    if (basic_execute_line(editor_state.line_buffer) < 0)
                    {
                        console_write("Syntax error\n");
                    }
                }
            }
        }
        else
        {
            editor_add_line(editor_state.line_buffer);
        }

        console_write("> ");
    }

    console_clear();
}

editor_state_t* editor_get_state(void)
{
    return &editor_state;
}
