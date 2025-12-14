/**
 * @file shell_ipc.c
 * @brief Shell IPC client implementation
 *
 * Implements IPC-based wrappers for console, input, VFS and system services.
 */

#include "shell_ipc.h"
#include "../lib/ipc_client.h"
#include "../lib/memory.h"
#include "../../include/protocols/console_protocol.h"
#include "../../include/protocols/input_protocol.h"
#include "../../include/protocols/vfs_protocol.h"
#include "../../user/lib/syscall.h"

static int console_port = -1;
static int input_port = -1;
static int vfs_port = -1;

static char cwd_buffer[VFS_MAX_PATH] = "/";

int shell_ipc_init(void)
{
    ipc_client_init();

    console_port = ipc_lookup_server(CONSOLE_SERVER_PORT_NAME);
    input_port = ipc_lookup_server(INPUT_SERVER_PORT_NAME);
    vfs_port = ipc_lookup_server(VFS_SERVER_PORT_NAME);

    return (console_port >= 0 && input_port >= 0) ? 0 : -1;
}

void console_write(const char *str)
{
    if (console_port < 0 || !str)
    {
        return;
    }

    struct message msg;
    struct console_write_request req;

    while (*str)
    {
        ipc_msg_init(&msg, CONSOLE_MSG_WRITE);

        int len = 0;
        while (str[len] && len < CONSOLE_MAX_WRITE_SIZE)
        {
            len++;
        }

        req.length = (uint8_t)len;
        mem_copy(req.data, str, (uint32_t)len);
        ipc_msg_set_data(&msg, &req, sizeof(req));
        ipc_call(console_port, &msg);

        str += len;
    }
}

void console_putchar(char c)
{
    char str[2] = { c, '\0' };
    console_write(str);
}

void console_write_dec(int32_t num)
{
    char buf[16];
    int i = 0;
    int neg = 0;

    if (num < 0)
    {
        neg = 1;
        num = -num;
    }

    if (num == 0)
    {
        console_putchar('0');
        return;
    }

    while (num > 0)
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    if (neg)
    {
        console_putchar('-');
    }

    while (i > 0)
    {
        console_putchar(buf[--i]);
    }
}

void console_write_hex(uint32_t num)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[9];
    int i;

    for (i = 7; i >= 0; i--)
    {
        buf[i] = hex[num & 0xF];
        num >>= 4;
    }
    buf[8] = '\0';

    console_write("0x");
    console_write(buf);
}

void console_clear(void)
{
    if (console_port < 0)
    {
        return;
    }

    struct message msg;
    ipc_msg_init(&msg, CONSOLE_MSG_CLEAR);
    ipc_call(console_port, &msg);
}

void console_set_color(uint8_t fg, uint8_t bg)
{
    if (console_port < 0)
    {
        return;
    }

    struct message msg;
    struct console_set_color_request req = { .foreground = fg, .background = bg };

    ipc_msg_init(&msg, CONSOLE_MSG_SET_COLOR);
    ipc_msg_set_data(&msg, &req, sizeof(req));
    ipc_call(console_port, &msg);
}

int console_get_size(uint16_t *width, uint16_t *height)
{
    if (console_port < 0)
    {
        return -1;
    }

    struct message msg;
    ipc_msg_init(&msg, CONSOLE_MSG_GET_SIZE);

    if (ipc_call(console_port, &msg) == IPC_SUCCESS)
    {
        struct console_size_response resp;
        ipc_msg_get_data(&msg, &resp, sizeof(resp));
        if (width) *width = resp.width;
        if (height) *height = resp.height;
        return resp.status;
    }

    return -1;
}

void console_set_pos(uint16_t x, uint16_t y)
{
    if (console_port < 0)
    {
        return;
    }

    struct message msg;
    struct console_position pos = { .x = x, .y = y };

    ipc_msg_init(&msg, CONSOLE_MSG_SET_POS);
    ipc_msg_set_data(&msg, &pos, sizeof(pos));
    ipc_call(console_port, &msg);
}

void console_get_pos(uint16_t *x, uint16_t *y)
{
    if (console_port < 0)
    {
        return;
    }

    struct message msg;
    ipc_msg_init(&msg, CONSOLE_MSG_GET_POS);

    if (ipc_call(console_port, &msg) == IPC_SUCCESS)
    {
        struct console_position pos;
        ipc_msg_get_data(&msg, &pos, sizeof(pos));
        if (x) *x = pos.x;
        if (y) *y = pos.y;
    }
}

char keyboard_getchar(void)
{
    if (input_port < 0)
    {
        return 0;
    }

    struct message msg;
    ipc_msg_init(&msg, INPUT_MSG_READ);

    if (ipc_call(input_port, &msg) == IPC_SUCCESS)
    {
        struct input_read_response resp;
        ipc_msg_get_data(&msg, &resp, sizeof(resp));

        if (resp.status == 0 && resp.event_count > 0)
        {
            if (resp.events[0].type == INPUT_EVENT_KEY_PRESS)
            {
                return (char)resp.events[0].keychar;
            }
        }
    }

    return 0;
}

int keyboard_poll(void)
{
    if (input_port < 0)
    {
        return 0;
    }

    struct message msg;
    ipc_msg_init(&msg, INPUT_MSG_POLL);

    if (ipc_call(input_port, &msg) == IPC_SUCCESS)
    {
        struct input_poll_response resp;
        ipc_msg_get_data(&msg, &resp, sizeof(resp));
        return (int)resp.events_pending;
    }

    return 0;
}

uint32_t timer_get_ticks(void)
{
    return sys_get_ticks();
}

void timer_wait(uint32_t ms)
{
    uint32_t start = sys_get_ticks();
    uint32_t ticks = ms;

    while ((sys_get_ticks() - start) < ticks)
    {
        for (volatile int i = 0; i < 1000; i++);
    }
}

int32_t fs_read(const char *path, void *buffer, uint32_t size)
{
    if (vfs_port < 0 || !path || !buffer)
    {
        return -1;
    }

    struct message msg;
    struct vfs_open_request open_req;
    struct vfs_open_response open_resp;

    ipc_msg_init(&msg, VFS_MSG_OPEN);
    open_req.flags = VFS_O_RDONLY;
    open_req.mode = 0;

    int i = 0;
    while (path[i] && i < VFS_MAX_PATH - 1)
    {
        open_req.path[i] = path[i];
        i++;
    }
    open_req.path[i] = '\0';

    ipc_msg_set_data(&msg, &open_req, sizeof(open_req));

    if (ipc_call(vfs_port, &msg) != IPC_SUCCESS)
    {
        return -1;
    }

    ipc_msg_get_data(&msg, &open_resp, sizeof(open_resp));

    if (open_resp.status < 0)
    {
        return -1;
    }

    int32_t fd = open_resp.fd;
    uint8_t *buf = (uint8_t *)buffer;
    int32_t total = 0;
    uint32_t offset = 0;

    while (offset < size)
    {
        struct vfs_read_request read_req;
        struct vfs_read_response read_resp;

        ipc_msg_init(&msg, VFS_MSG_READ);
        read_req.fd = fd;
        read_req.size = (size - offset < VFS_MAX_DATA) ? (size - offset) : VFS_MAX_DATA;
        read_req.offset = offset;
        ipc_msg_set_data(&msg, &read_req, sizeof(read_req));

        if (ipc_call(vfs_port, &msg) != IPC_SUCCESS)
        {
            break;
        }

        ipc_msg_get_data(&msg, &read_resp, sizeof(read_resp));

        if (read_resp.status <= 0)
        {
            break;
        }

        mem_copy(buf + offset, read_resp.data, (uint32_t)read_resp.status);
        offset += (uint32_t)read_resp.status;
        total += read_resp.status;

        if ((uint32_t)read_resp.status < read_req.size)
        {
            break;
        }
    }

    struct vfs_close_request close_req = { .fd = fd };
    ipc_msg_init(&msg, VFS_MSG_CLOSE);
    ipc_msg_set_data(&msg, &close_req, sizeof(close_req));
    ipc_call(vfs_port, &msg);

    return total;
}

int32_t fs_write(const char *path, const void *buffer, uint32_t size)
{
    if (vfs_port < 0 || !path || !buffer)
    {
        return -1;
    }

    struct message msg;
    struct vfs_open_request open_req;
    struct vfs_open_response open_resp;

    ipc_msg_init(&msg, VFS_MSG_OPEN);
    open_req.flags = VFS_O_WRONLY | VFS_O_CREATE | VFS_O_TRUNC;
    open_req.mode = 0;

    int i = 0;
    while (path[i] && i < VFS_MAX_PATH - 1)
    {
        open_req.path[i] = path[i];
        i++;
    }
    open_req.path[i] = '\0';

    ipc_msg_set_data(&msg, &open_req, sizeof(open_req));

    if (ipc_call(vfs_port, &msg) != IPC_SUCCESS)
    {
        return -1;
    }

    ipc_msg_get_data(&msg, &open_resp, sizeof(open_resp));

    if (open_resp.status < 0)
    {
        return -1;
    }

    int32_t fd = open_resp.fd;
    const uint8_t *buf = (const uint8_t *)buffer;
    int32_t total = 0;
    uint32_t offset = 0;

    while (offset < size)
    {
        struct vfs_write_request write_req;
        struct vfs_write_response write_resp;

        ipc_msg_init(&msg, VFS_MSG_WRITE);
        write_req.fd = fd;
        write_req.size = (size - offset < VFS_MAX_DATA) ? (size - offset) : VFS_MAX_DATA;
        mem_copy(write_req.data, buf + offset, write_req.size);
        ipc_msg_set_data(&msg, &write_req, sizeof(write_req));

        if (ipc_call(vfs_port, &msg) != IPC_SUCCESS)
        {
            break;
        }

        ipc_msg_get_data(&msg, &write_resp, sizeof(write_resp));

        if (write_resp.status < 0)
        {
            break;
        }

        offset += write_req.size;
        total += (int32_t)write_req.size;
    }

    struct vfs_close_request close_req = { .fd = fd };
    ipc_msg_init(&msg, VFS_MSG_CLOSE);
    ipc_msg_set_data(&msg, &close_req, sizeof(close_req));
    ipc_call(vfs_port, &msg);

    return total;
}

int fs_exists(const char *path)
{
    if (vfs_port < 0 || !path)
    {
        return 0;
    }

    struct message msg;
    struct vfs_stat_request stat_req;
    struct vfs_stat_response stat_resp;

    ipc_msg_init(&msg, VFS_MSG_STAT);

    int i = 0;
    while (path[i] && i < VFS_MAX_PATH - 1)
    {
        stat_req.path[i] = path[i];
        i++;
    }
    stat_req.path[i] = '\0';

    ipc_msg_set_data(&msg, &stat_req, sizeof(stat_req));

    if (ipc_call(vfs_port, &msg) == IPC_SUCCESS)
    {
        ipc_msg_get_data(&msg, &stat_resp, sizeof(stat_resp));
        return (stat_resp.status >= 0) ? 1 : 0;
    }

    return 0;
}

int fs_is_dir(const char *path)
{
    if (vfs_port < 0 || !path)
    {
        return 0;
    }

    struct message msg;
    struct vfs_stat_request stat_req;
    struct vfs_stat_response stat_resp;

    ipc_msg_init(&msg, VFS_MSG_STAT);

    int i = 0;
    while (path[i] && i < VFS_MAX_PATH - 1)
    {
        stat_req.path[i] = path[i];
        i++;
    }
    stat_req.path[i] = '\0';

    ipc_msg_set_data(&msg, &stat_req, sizeof(stat_req));

    if (ipc_call(vfs_port, &msg) == IPC_SUCCESS)
    {
        ipc_msg_get_data(&msg, &stat_resp, sizeof(stat_resp));
        return (stat_resp.status >= 0 && stat_resp.info.type == VFS_TYPE_DIR) ? 1 : 0;
    }

    return 0;
}

int fs_create_file(const char *path)
{
    if (vfs_port < 0 || !path)
    {
        return -1;
    }

    struct message msg;
    struct vfs_open_request open_req;
    struct vfs_open_response open_resp;

    ipc_msg_init(&msg, VFS_MSG_OPEN);
    open_req.flags = VFS_O_CREATE | VFS_O_WRONLY;
    open_req.mode = 0;

    int i = 0;
    while (path[i] && i < VFS_MAX_PATH - 1)
    {
        open_req.path[i] = path[i];
        i++;
    }
    open_req.path[i] = '\0';

    ipc_msg_set_data(&msg, &open_req, sizeof(open_req));

    if (ipc_call(vfs_port, &msg) != IPC_SUCCESS)
    {
        return -1;
    }

    ipc_msg_get_data(&msg, &open_resp, sizeof(open_resp));

    if (open_resp.status >= 0)
    {
        struct vfs_close_request close_req = { .fd = open_resp.fd };
        ipc_msg_init(&msg, VFS_MSG_CLOSE);
        ipc_msg_set_data(&msg, &close_req, sizeof(close_req));
        ipc_call(vfs_port, &msg);
        return 0;
    }

    return -1;
}

int fs_create_dir(const char *path)
{
    if (vfs_port < 0 || !path)
    {
        return -1;
    }

    struct message msg;
    struct vfs_path_request path_req;

    ipc_msg_init(&msg, VFS_MSG_MKDIR);

    int i = 0;
    while (path[i] && i < VFS_MAX_PATH - 1)
    {
        path_req.path[i] = path[i];
        i++;
    }
    path_req.path[i] = '\0';

    ipc_msg_set_data(&msg, &path_req, sizeof(path_req));

    if (ipc_call(vfs_port, &msg) == IPC_SUCCESS)
    {
        struct vfs_response resp;
        ipc_msg_get_data(&msg, &resp, sizeof(resp));
        return resp.status;
    }

    return -1;
}

int fs_remove(const char *path)
{
    if (vfs_port < 0 || !path)
    {
        return -1;
    }

    struct message msg;
    struct vfs_path_request path_req;

    ipc_msg_init(&msg, VFS_MSG_UNLINK);

    int i = 0;
    while (path[i] && i < VFS_MAX_PATH - 1)
    {
        path_req.path[i] = path[i];
        i++;
    }
    path_req.path[i] = '\0';

    ipc_msg_set_data(&msg, &path_req, sizeof(path_req));

    if (ipc_call(vfs_port, &msg) == IPC_SUCCESS)
    {
        struct vfs_response resp;
        ipc_msg_get_data(&msg, &resp, sizeof(resp));
        return resp.status;
    }

    return -1;
}

int fs_list_dir(const char *path, void (*callback)(const char *name, int is_dir))
{
    if (vfs_port < 0 || !path || !callback)
    {
        return -1;
    }

    struct message msg;
    struct vfs_path_request path_req;

    int i = 0;
    while (path[i] && i < VFS_MAX_PATH - 1)
    {
        path_req.path[i] = path[i];
        i++;
    }
    path_req.path[i] = '\0';

    int count = 0;
    int more = 1;

    while (more)
    {
        ipc_msg_init(&msg, VFS_MSG_READDIR);
        ipc_msg_set_data(&msg, &path_req, sizeof(path_req));

        if (ipc_call(vfs_port, &msg) != IPC_SUCCESS)
        {
            return -1;
        }

        struct vfs_readdir_response resp;
        ipc_msg_get_data(&msg, &resp, sizeof(resp));

        if (resp.status < 0)
        {
            return -1;
        }

        for (int j = 0; j < resp.count; j++)
        {
            callback(resp.entries[j].name, resp.entries[j].type == VFS_TYPE_DIR);
            count++;
        }

        more = resp.more;
    }

    return count;
}

int fs_change_dir(const char *path)
{
    if (vfs_port < 0 || !path)
    {
        return -1;
    }

    struct message msg;
    struct vfs_path_request path_req;

    ipc_msg_init(&msg, VFS_MSG_CHDIR);

    int i = 0;
    while (path[i] && i < VFS_MAX_PATH - 1)
    {
        path_req.path[i] = path[i];
        i++;
    }
    path_req.path[i] = '\0';

    ipc_msg_set_data(&msg, &path_req, sizeof(path_req));

    if (ipc_call(vfs_port, &msg) == IPC_SUCCESS)
    {
        struct vfs_response resp;
        ipc_msg_get_data(&msg, &resp, sizeof(resp));

        if (resp.status >= 0)
        {
            i = 0;
            while (path[i] && i < VFS_MAX_PATH - 1)
            {
                cwd_buffer[i] = path[i];
                i++;
            }
            cwd_buffer[i] = '\0';
            return 0;
        }
    }

    return -1;
}

const char *fs_get_cwd(void)
{
    return cwd_buffer;
}

void fs_init(void)
{
    cwd_buffer[0] = '/';
    cwd_buffer[1] = '\0';
}

void fs_clear_cache(void)
{
    /* VFS server handles caching internally */
}

void fs_sync(void)
{
    /* VFS server handles syncing internally */
}

int fs_is_disk_enabled(void)
{
    return (vfs_port >= 0) ? 1 : 0;
}

void fs_enable_disk(int enable)
{
    (void)enable;
}
