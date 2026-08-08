#include "fs.h"

#include "assert.h"
#include "diskfs.h"
#include "lib/string.h"
#include "lib/log.h"
#include "sched/sched.h"

static struct fs_node fs_nodes[FS_MAX_FILES];
static int disk_enabled = 0;
static char cwd[FS_MAX_PATH];
static uint32_t cwd_idx;
static uint32_t cwd_diskfs_ino = 0;

/**
 * @brief Struct to represent filesystem open file \struct fs_open_file
 */
struct fs_open_file
{
    uint8_t used;
    uint8_t disk_backed;
    pid_t owner;
    int fd;
    uint8_t pipe_backed;
    uint8_t pipe_read_end;
    int pipe_id;
    int flags;
    uint32_t pos;
    int node_idx;
    int disk_ino;
    char path[FS_MAX_PATH];
};

static struct fs_open_file open_files[FS_MAX_OPEN_FILES];

#define FS_MAX_PIPES 8
#define FS_PIPE_BUFFER 1024

/**
 * @brief Struct to represent a pipe state \struct fs_pipe_state
 */
struct fs_pipe_state
{
    bool used;
    uint32_t readers;
    uint32_t writers;
    uint32_t head;
    uint32_t tail;
    char data[FS_PIPE_BUFFER];
};

static struct fs_pipe_state pipes[FS_MAX_PIPES];

#define FS_FIRST_USER_FD 3

static struct task* fs_current_task(void)
{
    return sched_get_current();
}

static char* active_cwd(void)
{
    struct task* task = fs_current_task();
    return task ? task->cwd : cwd;
}

static uint32_t active_cwd_node(void)
{
    const struct task* task = fs_current_task();
    return task ? task->cwd_node : cwd_idx;
}

static uint32_t active_cwd_disk_inode(void)
{
    const struct task* task = fs_current_task();
    return task ? task->cwd_disk_inode : cwd_diskfs_ino;
}

static void set_active_cwd(const char* path, const uint32_t node, const uint32_t disk_inode)
{
    struct task* task = fs_current_task();
    char* destination = task ? task->cwd : cwd;
    strncpy(destination, path, FS_MAX_PATH - 1);
    destination[FS_MAX_PATH - 1] = '\0';
    if (task)
    {
        task->cwd_node = node;
        task->cwd_disk_inode = disk_inode;
    }
    else
    {
        cwd_idx = node;
        cwd_diskfs_ino = disk_inode;
    }
}

static pid_t active_pid(void)
{
    const struct task* task = fs_current_task();
    return task ? task->pid : 0;
}

static int normalize_path(const char* path, char* out_path)
{
    if (!path || !out_path || path[0] == '\0')
    {
        return FS_ERR_INVALID;
    }

    char input[FS_MAX_PATH];
    if (path[0] == '/')
    {
        strncpy(input, path, FS_MAX_PATH - 1);
        input[FS_MAX_PATH - 1] = '\0';
    }
    else if (strcmp(active_cwd(), "/") == 0)
    {
        strncpy(input, "/", FS_MAX_PATH - 1);
        input[FS_MAX_PATH - 1] = '\0';
        strncat(input, path, FS_MAX_PATH - strlen(input) - 1);
    }
    else
    {
        strncpy(input, active_cwd(), FS_MAX_PATH - 1);
        input[FS_MAX_PATH - 1] = '\0';
        strncat(input, "/", FS_MAX_PATH - strlen(input) - 1);
        strncat(input, path, FS_MAX_PATH - strlen(input) - 1);
    }

    char parts[FS_MAX_PATH_DEPTH][FS_MAX_NAME];
    int depth = 0;
    char* p = input;

    while (*p)
    {
        while (*p == '/')
        {
            p++;
        }

        if (*p == '\0')
        {
            break;
        }

        char component[FS_MAX_NAME];
        uint32_t len = 0;
        while (*p && *p != '/')
        {
            if (len < FS_MAX_NAME - 1)
            {
                component[len++] = *p;
            }
            p++;
        }
        component[len] = '\0';

        if (strcmp(component, ".") == 0 || component[0] == '\0')
        {
            continue;
        }

        if (strcmp(component, "..") == 0)
        {
            if (depth > 0)
            {
                depth--;
            }
            continue;
        }

        if (depth >= FS_MAX_PATH_DEPTH)
        {
            return FS_ERR_INVALID;
        }

        strncpy(parts[depth], component, FS_MAX_NAME - 1);
        parts[depth][FS_MAX_NAME - 1] = '\0';
        depth++;
    }

    if (depth == 0)
    {
        strcpy(out_path, "/");
        return FS_ERR_OK;
    }

    out_path[0] = '\0';
    for (int i = 0; i < depth; i++)
    {
        if (strlen(out_path) + strlen(parts[i]) + 2 >= FS_MAX_PATH)
        {
            return FS_ERR_INVALID;
        }

        strcat(out_path, "/");
        strcat(out_path, parts[i]);
    }

    return FS_ERR_OK;
}

static int split_path(const char* path, char* parent_path, char* basename)
{
    ASSERT(path != NULL && parent_path != NULL && basename != NULL);
    char normalized[FS_MAX_PATH];
    const int ret = normalize_path(path, normalized);
    if (ret != FS_ERR_OK)
    {
        return ret;
    }

    if (strcmp(normalized, "/") == 0)
    {
        return FS_ERR_INVALID;
    }

    char* last_slash = normalized;
    for (char* p = normalized; *p; p++)
    {
        if (*p == '/')
        {
            last_slash = p;
        }
    }

    if (last_slash == normalized)
    {
        strcpy(parent_path, "/");
        strncpy(basename, normalized + 1, FS_MAX_NAME - 1);
        basename[FS_MAX_NAME - 1] = '\0';
        return basename[0] == '\0' ? FS_ERR_INVALID : FS_ERR_OK;
    }

    *last_slash = '\0';
    strncpy(parent_path, normalized, FS_MAX_PATH - 1);
    parent_path[FS_MAX_PATH - 1] = '\0';
    strncpy(basename, last_slash + 1, FS_MAX_NAME - 1);
    basename[FS_MAX_NAME - 1] = '\0';

    return basename[0] == '\0' ? FS_ERR_INVALID : FS_ERR_OK;
}

static int find_free_node(void)
{
    for (int i = 0; i < FS_MAX_FILES; i++)
    {
        if (!fs_nodes[i].used)
        {
            return i;
        }
    }
    return -1;
}

static int find_node_in_dir(const uint32_t dir_idx, const char* name)
{
    for (int i = 0; i < FS_MAX_FILES; i++)
    {
        if (fs_nodes[i].used && fs_nodes[i].parent_idx == dir_idx)
        {
            if (strcmp(fs_nodes[i].name, name) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

static int resolve_path(const char* path, uint32_t* parent_idx, char* basename)
{
    if (path == NULL || path[0] == '\0')
    {
        return FS_ERR_INVALID;
    }

    char buf[FS_MAX_PATH];
    strncpy(buf, path, FS_MAX_PATH - 1);
    buf[FS_MAX_PATH - 1] = '\0';

    uint32_t current = (buf[0] == '/') ? 0 : active_cwd_node();

    char* p = buf;
    if (*p == '/')
    {
        p++;
    }

    char* last_slash = NULL;
    for (char* s = p; *s; s++)
    {
        if (*s == '/')
        {
            last_slash = s;
        }
    }

    if (last_slash == NULL)
    {
        *parent_idx = current;
        strncpy(basename, p, FS_MAX_NAME - 1);
        basename[FS_MAX_NAME - 1] = '\0';
        return FS_ERR_OK;
    }

    *last_slash = '\0';
    char* dir_part = p;
    const char* name_part = last_slash + 1;

    char* token = dir_part;
    while (*token)
    {
        char component[FS_MAX_NAME];
        char* slash = NULL;

        for (char* s = token; *s; s++)
        {
            if (*s == '/')
            {
                slash = s;
                break;
            }
        }

        if (slash)
        {
            size_t len = (size_t)(slash - token);
            if (len >= FS_MAX_NAME) len = FS_MAX_NAME - 1;
            strncpy(component, token, len);
            component[len] = '\0';
            token = slash + 1;
        }
        else
        {
            strncpy(component, token, FS_MAX_NAME - 1);
            component[FS_MAX_NAME - 1] = '\0';
            token = token + strlen(token);
        }

        if (component[0] == '\0')
        {
            continue;
        }

        if (strcmp(component, ".") == 0)
        {
            continue;
        }

        if (strcmp(component, "..") == 0)
        {
            if (current != 0)
            {
                current = fs_nodes[current].parent_idx;
            }
            continue;
        }

        const int idx = find_node_in_dir(current, component);
        if (idx < 0 || fs_nodes[idx].type != FS_TYPE_DIR)
        {
            return FS_ERR_NOT_FOUND;
        }
        current = (uint32_t)idx;
    }

    *parent_idx = current;
    strncpy(basename, name_part, FS_MAX_NAME - 1);
    basename[FS_MAX_NAME - 1] = '\0';

    return FS_ERR_OK;
}

static int resolve_full_path(const char* path)
{
    if (path == NULL || path[0] == '\0')
    {
        return FS_ERR_INVALID;
    }

    if (strcmp(path, "/") == 0)
    {
        return 0;
    }

    uint32_t parent_idx;
    char basename[FS_MAX_NAME];

    const int ret = resolve_path(path, &parent_idx, basename);
    if (ret != FS_ERR_OK)
    {
        return ret;
    }

    if (basename[0] == '\0')
    {
        return (int)parent_idx;
    }

    if (strcmp(basename, ".") == 0)
    {
        return (int)parent_idx;
    }

    if (strcmp(basename, "..") == 0)
    {
        if (parent_idx == 0)
        {
            return 0;
        }
        return (int)fs_nodes[parent_idx].parent_idx;
    }

    return find_node_in_dir(parent_idx, basename);
}

static int resolve_to_diskfs_inode(const char* path)
{
    if (!disk_enabled || !path)
    {
        return -1;
    }

    char normalized[FS_MAX_PATH];
    if (normalize_path(path, normalized) != FS_ERR_OK)
    {
        return -1;
    }

    if (strcmp(normalized, "/") == 0)
    {
        return 0;
    }

    int current_ino = 0;

    char* p = normalized;
    if (*p == '/')
    {
        p++;
    }

    while (*p)
    {
        char component[DISKFS_MAX_FILENAME];
        char* slash = NULL;

        for (char* s = p; *s; s++)
        {
            if (*s == '/')
            {
                slash = s;
                break;
            }
        }

        if (slash)
        {
            size_t len = (size_t) (slash - p);
            if (len >= DISKFS_MAX_FILENAME) len = DISKFS_MAX_FILENAME - 1;
            strncpy(component, p, len);
            component[len] = '\0';
            p = slash + 1;
        }
        else
        {
            strncpy(component, p, DISKFS_MAX_FILENAME - 1);
            component[DISKFS_MAX_FILENAME - 1] = '\0';
            p = p + strlen(p);
        }

        if (component[0] == '\0')
        {
            continue;
        }

        if (strcmp(component, ".") == 0)
        {
            continue;
        }

        if (strcmp(component, "..") == 0)
        {
            struct diskfs_inode inode;
            if (diskfs_stat((uint32_t)current_ino, &inode) != 0)
            {
                return -1;
            }

            current_ino = (int)inode.parent_inode;
            continue;
        }

        current_ino = diskfs_lookup((uint32_t)current_ino, component);
        if (current_ino < 0)
        {
            return -1;
        }
    }

    return current_ino;
}

void fs_init(void)
{
    memset(fs_nodes, 0, sizeof(fs_nodes));
    memset(open_files, 0, sizeof(open_files));

    fs_nodes[0].used = 1;
    fs_nodes[0].type = FS_TYPE_DIR;
    strcpy(fs_nodes[0].name, "/");
    fs_nodes[0].parent_idx = 0;
    fs_nodes[0].size = 0;

    cwd_idx = 0;
    strcpy(cwd, "/");
    disk_enabled = 0;
}

int fs_create_file(const char* path)
{
    if (disk_enabled)
    {
        char parent_path[FS_MAX_PATH];
        char basename[FS_MAX_NAME];

        const int split_ret = split_path(path, parent_path, basename);
        if (split_ret != FS_ERR_OK)
        {
            return split_ret;
        }

        const int parent_ino = resolve_to_diskfs_inode(parent_path);
        if (parent_ino < 0)
        {
            return FS_ERR_NOT_FOUND;
        }

        const int ino = diskfs_create((uint32_t)parent_ino, basename, DISKFS_TYPE_FILE);
        if (ino == -2) return FS_ERR_EXISTS;
        if (ino < 0) return FS_ERR_FULL;
        return FS_ERR_OK;
    }

    uint32_t parent_idx;
    char basename[FS_MAX_NAME];

    const int ret = resolve_path(path, &parent_idx, basename);
    if (ret != FS_ERR_OK)
    {
        return ret;
    }

    if (basename[0] == '\0')
    {
        return FS_ERR_INVALID;
    }

    if (find_node_in_dir(parent_idx, basename) >= 0)
    {
        return FS_ERR_EXISTS;
    }

    const int idx = find_free_node();
    if (idx < 0)
    {
        return FS_ERR_FULL;
    }

    fs_nodes[idx].used = 1;
    fs_nodes[idx].type = FS_TYPE_FILE;
    strncpy(fs_nodes[idx].name, basename, FS_MAX_NAME - 1);
    fs_nodes[idx].name[FS_MAX_NAME - 1] = '\0';
    fs_nodes[idx].parent_idx = parent_idx;
    fs_nodes[idx].size = 0;
    memset(fs_nodes[idx].data, 0, FS_MAX_FILE_SIZE);

    return FS_ERR_OK;
}

static int disk_mode(bool enabled, const bool isRemove, const char* path)
{
    if (enabled)
    {
        char parent_path[FS_MAX_PATH];
        char basename[FS_MAX_NAME];
        const int split_ret = split_path(path, parent_path, basename);
        if (split_ret != FS_ERR_OK)
        {
            return split_ret;
        }

        const int parent_ino = resolve_to_diskfs_inode(parent_path);
        if (parent_ino < 0) return FS_ERR_NOT_FOUND;

        int ino;
        if (isRemove)
        {
            ino = diskfs_delete((uint32_t)parent_ino, basename);
            if (ino == -2) return FS_ERR_NOT_EMPTY;
            if (ino < 0) return FS_ERR_NOT_FOUND;
            diskfs_sync();
            return FS_ERR_OK;
        }

        ino = diskfs_create((uint32_t)parent_ino, basename, DISKFS_TYPE_DIR);
        if (ino == -2) return FS_ERR_EXISTS;
        if (ino < 0) return FS_ERR_FULL;
        return FS_ERR_OK;
    }

    return FS_ERR_INVALID;
}

int fs_create_dir(const char* path)
{
    if (disk_enabled)
    {
        const int result = disk_mode(true, false, path);
        return result;
    }

    uint32_t parent_idx;
    char basename[FS_MAX_NAME];

    const int ret = resolve_path(path, &parent_idx, basename);
    if (ret != FS_ERR_OK)
    {
        return ret;
    }

    if (basename[0] == '\0')
    {
        return FS_ERR_INVALID;
    }

    if (find_node_in_dir(parent_idx, basename) >= 0)
    {
        return FS_ERR_EXISTS;
    }

    const int idx = find_free_node();
    if (idx < 0)
    {
        return FS_ERR_FULL;
    }

    fs_nodes[idx].used = 1;
    fs_nodes[idx].type = FS_TYPE_DIR;
    strncpy(fs_nodes[idx].name, basename, FS_MAX_NAME - 1);
    fs_nodes[idx].name[FS_MAX_NAME - 1] = '\0';
    fs_nodes[idx].parent_idx = parent_idx;
    fs_nodes[idx].size = 0;

    return FS_ERR_OK;
}

int fs_remove(const char* path)
{
    if (disk_enabled)
    {
        const int result = disk_mode(true, true, path);
        return result;
    }

    const int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (idx == 0)
    {
        return FS_ERR_INVALID;
    }

    if (fs_nodes[idx].type == FS_TYPE_DIR)
    {
        for (int i = 0; i < FS_MAX_FILES; i++)
        {
            if (fs_nodes[i].used && fs_nodes[i].parent_idx == (uint32_t)idx)
            {
                return FS_ERR_NOT_EMPTY;
            }
        }
    }

    fs_nodes[idx].used = 0;
    memset(&fs_nodes[idx], 0, sizeof(struct fs_node));

    return FS_ERR_OK;
}

int fs_read(const char* path, char* buffer, const uint32_t size)
{
    if (disk_enabled)
    {
        const int ino = resolve_to_diskfs_inode(path);
        if (ino < 0)
        {
            return FS_ERR_NOT_FOUND;
        }

        return diskfs_read((uint32_t)ino, buffer, 0, size);
    }

    const int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (fs_nodes[idx].type != FS_TYPE_FILE)
    {
        return FS_ERR_IS_DIR;
    }

    const uint32_t to_read = (size < fs_nodes[idx].size) ? size : fs_nodes[idx].size;
    memcpy(buffer, fs_nodes[idx].data, to_read);

    return (int)to_read;
}

int fs_write(const char* path, const char* data, uint32_t size)
{
    if (disk_enabled)
    {
        if (size > DISKFS_MAX_FILE_SIZE)
        {
            return FS_ERR_FULL;
        }

        const int ino = resolve_to_diskfs_inode(path);
        if (ino < 0)
        {
            return FS_ERR_NOT_FOUND;
        }

        if (diskfs_truncate((uint32_t)ino) != 0)
        {
            return FS_ERR_INVALID;
        }

        const int ret = diskfs_write((uint32_t)ino, data, 0, size);
        diskfs_sync();
        return ret;
    }

    const int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (fs_nodes[idx].type != FS_TYPE_FILE)
    {
        return FS_ERR_IS_DIR;
    }

    if (size > FS_MAX_FILE_SIZE)
    {
        size = FS_MAX_FILE_SIZE;
    }

    memcpy(fs_nodes[idx].data, data, size);
    fs_nodes[idx].size = size;

    return (int)size;
}

static int fd_to_slot(const int fd)
{
    if (fd < 0)
    {
        return FS_ERR_INVALID;
    }

    const pid_t owner = active_pid();
    for (int slot = 0; slot < FS_MAX_OPEN_FILES; slot++)
    {
        if (open_files[slot].used && open_files[slot].owner == owner && open_files[slot].fd == fd)
        {
            return slot;
        }
    }
    return FS_ERR_INVALID;
}

int fs_read_fd(const int fd, char* buffer, uint32_t size)
{
    const int slot = fd_to_slot(fd);
    if (slot < 0) return slot;

    struct fs_open_file* file = &open_files[slot];
    if (!(file->flags & FS_OPEN_READ)) return FS_ERR_INVALID;

    if (file->pipe_backed)
    {
        struct fs_pipe_state* pipe = &pipes[file->pipe_id];
        uint32_t count = 0;
        while (count == 0)
        {
            while (count < size && pipe->head != pipe->tail)
            {
                buffer[count++] = pipe->data[pipe->head];
                pipe->head = (pipe->head + 1U) % FS_PIPE_BUFFER;
            }
            if (count || pipe->writers == 0) break;
            sched_yield();
        }
        return (int)count;
    }

    if (file->disk_backed)
    {
        const int ret = diskfs_read((uint32_t)file->disk_ino, buffer, file->pos, size);
        if (ret > 0)
        {
            file->pos += (uint32_t)ret;
        }
        return ret;
    }

    if (file->node_idx < 0 || file->node_idx >= FS_MAX_FILES ||
        !fs_nodes[file->node_idx].used || fs_nodes[file->node_idx].type != FS_TYPE_FILE)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (file->pos >= fs_nodes[file->node_idx].size)
    {
        return 0;
    }

    const uint32_t available = fs_nodes[file->node_idx].size - file->pos;
    if (size > available)
    {
        size = available;
    }

    memcpy(buffer, fs_nodes[file->node_idx].data + file->pos, size);
    file->pos += size;

    return (int)size;
}

int fs_write_fd(const int fd, const char* data, uint32_t size)
{
    const int slot = fd_to_slot(fd);
    if (slot < 0) return slot;

    struct fs_open_file* file = &open_files[slot];
    if (!(file->flags & FS_OPEN_WRITE)) return FS_ERR_INVALID;

    if (file->pipe_backed)
    {
        struct fs_pipe_state* pipe = &pipes[file->pipe_id];
        if (pipe->readers == 0) return FS_ERR_INVALID;
        uint32_t count = 0;
        while (count < size)
        {
            const uint32_t next = (pipe->tail + 1U) % FS_PIPE_BUFFER;
            if (next == pipe->head)
            {
                sched_yield();
                continue;
            }
            pipe->data[pipe->tail] = data[count++];
            pipe->tail = next;
        }
        return (int)count;
    }

    if (file->disk_backed)
    {
        const int ret = diskfs_write((uint32_t)file->disk_ino, data, file->pos, size);
        if (ret > 0)
        {
            file->pos += (uint32_t)ret;
            diskfs_sync();
        }
        return ret;
    }

    if (file->node_idx < 0 || file->node_idx >= FS_MAX_FILES ||
        !fs_nodes[file->node_idx].used || fs_nodes[file->node_idx].type != FS_TYPE_FILE)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (file->pos >= FS_MAX_FILE_SIZE)
    {
        return 0;
    }

    const uint32_t available = FS_MAX_FILE_SIZE - file->pos;
    if (size > available)
    {
        size = available;
    }

    memcpy(fs_nodes[file->node_idx].data + file->pos, data, size);
    file->pos += size;
    if (file->pos > fs_nodes[file->node_idx].size)
    {
        fs_nodes[file->node_idx].size = file->pos;
    }

    return (int)size;
}

int fs_append(const char* path, const char* data, uint32_t size)
{
    if (disk_enabled)
    {
        const int ino = resolve_to_diskfs_inode(path);
        if (ino < 0)
        {
            return FS_ERR_NOT_FOUND;
        }

        struct diskfs_inode inode;
        if (diskfs_stat((uint32_t)ino, &inode) != 0)
        {
            return FS_ERR_NOT_FOUND;
        }

        const uint32_t available = DISKFS_MAX_FILE_SIZE - inode.size;
        if (size > available)
        {
            size = available;
        }

        const int ret = diskfs_write((uint32_t)ino, data, inode.size, size);
        diskfs_sync();
        return ret;
    }

    const int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (fs_nodes[idx].type != FS_TYPE_FILE)
    {
        return FS_ERR_IS_DIR;
    }

    const uint32_t available = FS_MAX_FILE_SIZE - fs_nodes[idx].size;
    if (size > available)
    {
        size = available;
    }

    memcpy(fs_nodes[idx].data + fs_nodes[idx].size, data, size);
    fs_nodes[idx].size += size;

    return (int)size;
}

int fs_list_dir(const char* path, char* buffer, const uint32_t size)
{
    if (disk_enabled)
    {
        const char* resolved_path = (path == NULL || path[0] == '\0') ? "." : path;
        const int dir_ino = resolve_to_diskfs_inode(resolved_path);
        if (dir_ino < 0) return FS_ERR_NOT_FOUND;

        struct diskfs_dirent entries[FS_MAX_FILES];
        const int count = diskfs_readdir((uint32_t)dir_ino, entries, FS_MAX_FILES);
        if (count < 0) return FS_ERR_NOT_DIR;

        uint32_t pos = 0;
        buffer[0] = '\0';
        for (int i = 0; i < count; i++)
        {
            if (entries[i].inode == 0) continue;

            struct diskfs_inode inode;
            if (diskfs_stat(entries[i].inode, &inode) != 0) continue;

            const uint32_t name_len = strlen(entries[i].name);
            if (pos + name_len + 4 >= size) break;

            if (inode.type == DISKFS_TYPE_DIR) buffer[pos++] = '[';
            memcpy(buffer + pos, entries[i].name, name_len);
            pos += name_len;
            if (inode.type == DISKFS_TYPE_DIR) buffer[pos++] = ']';
            buffer[pos++] = '\n';
            buffer[pos] = '\0';
        }
        return (int)pos;
    }

    int idx;

    if (path == NULL || path[0] == '\0' || strcmp(path, ".") == 0)
    {
        idx = (int)active_cwd_node();
    }
    else
    {
        idx = resolve_full_path(path);
    }

    if (idx < 0)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (fs_nodes[idx].type != FS_TYPE_DIR)
    {
        return FS_ERR_NOT_DIR;
    }

    uint32_t pos = 0;
    buffer[0] = '\0';

    for (int i = 0; i < FS_MAX_FILES; i++)
    {
        if (fs_nodes[i].used && fs_nodes[i].parent_idx == (uint32_t)idx && i != idx)
        {
            const uint32_t name_len = strlen(fs_nodes[i].name);
            const uint32_t entry_len = name_len + 2;

            if (pos + entry_len >= size)
            {
                break;
            }

            if (fs_nodes[i].type == FS_TYPE_DIR)
            {
                buffer[pos++] = '[';
            }

            memcpy(buffer + pos, fs_nodes[i].name, name_len);
            pos += name_len;

            if (fs_nodes[i].type == FS_TYPE_DIR)
            {
                buffer[pos++] = ']';
            }

            buffer[pos++] = '\n';
            buffer[pos] = '\0';
        }
    }

    return (int)pos;
}

int fs_change_dir(const char* path)
{
    if (disk_enabled)
    {
        if (path == NULL || path[0] == '\0')
        {
            set_active_cwd("/", 0, 0);
            return FS_ERR_OK;
        }

        char normalized[FS_MAX_PATH];
        if (normalize_path(path, normalized) != FS_ERR_OK)
        {
            return FS_ERR_INVALID;
        }

        const int ino = resolve_to_diskfs_inode(normalized);
        if (ino < 0) return FS_ERR_NOT_FOUND;

        struct diskfs_inode inode;
        if (diskfs_stat((uint32_t)ino, &inode) != 0) return FS_ERR_NOT_FOUND;
        if (inode.type != DISKFS_TYPE_DIR) return FS_ERR_NOT_DIR;

        set_active_cwd(normalized, 0, (uint32_t)ino);

        return FS_ERR_OK;
    }

    if (path == NULL || path[0] == '\0')
    {
        set_active_cwd("/", 0, 0);
        return FS_ERR_OK;
    }

    const int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (fs_nodes[idx].type != FS_TYPE_DIR)
    {
        return FS_ERR_NOT_DIR;
    }

    if (idx == 0)
    {
        set_active_cwd("/", 0, 0);
    }
    else
    {
        char parts[FS_MAX_PATH_DEPTH][FS_MAX_NAME];
        int depth = 0;
        uint32_t current = (uint32_t)idx;

        while (current != 0 && depth < FS_MAX_PATH_DEPTH)
        {
            strncpy(parts[depth], fs_nodes[current].name, FS_MAX_NAME - 1);
            parts[depth][FS_MAX_NAME - 1] = '\0';
            depth++;
            current = fs_nodes[current].parent_idx;
        }

        char new_cwd[FS_MAX_PATH];
        new_cwd[0] = '\0';
        for (int i = depth - 1; i >= 0; i--)
        {
            strcat(new_cwd, "/");
            strcat(new_cwd, parts[i]);
        }
        set_active_cwd(new_cwd, (uint32_t)idx, 0);
    }

    return FS_ERR_OK;
}

const char* fs_get_cwd(void)
{
    return active_cwd();
}

int fs_get_cwd_copy(char* buffer, const uint32_t size)
{
    if (!buffer || size == 0)
    {
        return FS_ERR_INVALID;
    }

    strncpy(buffer, active_cwd(), size - 1);
    buffer[size - 1] = '\0';
    return (int)strlen(buffer);
}

int fs_exists(const char* path)
{
    if (disk_enabled)
    {
        return resolve_to_diskfs_inode(path) >= 0;
    }
    return resolve_full_path(path) >= 0;
}

int fs_is_dir(const char* path)
{
    if (disk_enabled)
    {
        const int ino = resolve_to_diskfs_inode(path);
        if (ino < 0)
        {
            return 0;
        }

        struct diskfs_inode inode;
        if (diskfs_stat((uint32_t)ino, &inode) != 0)
        {
            return 0;
        }

        return inode.type == DISKFS_TYPE_DIR;
    }

    const int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return 0;
    }
    return fs_nodes[idx].type == FS_TYPE_DIR;
}

uint32_t fs_get_size(const char* path)
{
    if (disk_enabled)
    {
        const int ino = resolve_to_diskfs_inode(path);
        if (ino < 0)
        {
            return 0;
        }

        struct diskfs_inode inode;
        if (diskfs_stat((uint32_t)ino, &inode) != 0)
        {
            return 0;
        }

        return inode.size;
    }

    const int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return 0;
    }
    return fs_nodes[idx].size;
}

int fs_stat(const char* path, struct fs_stat* info)
{
    if (!info)
    {
        return FS_ERR_INVALID;
    }

    if (disk_enabled)
    {
        const int ino = resolve_to_diskfs_inode(path);
        if (ino < 0)
        {
            return FS_ERR_NOT_FOUND;
        }

        struct diskfs_inode inode;
        if (diskfs_stat((uint32_t)ino, &inode) != 0)
        {
            return FS_ERR_NOT_FOUND;
        }

        info->type = (inode.type == DISKFS_TYPE_DIR) ? FS_ABI_TYPE_DIR : FS_ABI_TYPE_FILE;
        info->size = inode.size;
        return FS_ERR_OK;
    }

    const int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return FS_ERR_NOT_FOUND;
    }

    info->type = (fs_nodes[idx].type == FS_TYPE_DIR) ? FS_ABI_TYPE_DIR : FS_ABI_TYPE_FILE;
    info->size = fs_nodes[idx].size;
    return FS_ERR_OK;
}

int fs_readdir(const char* path, struct fs_dirent* entries, const uint32_t max_entries)
{
    if (!entries || max_entries == 0)
    {
        return FS_ERR_INVALID;
    }

    if (disk_enabled)
    {
        const char* resolved_path = (path == NULL || path[0] == '\0') ? "." : path;
        const int dir_ino = resolve_to_diskfs_inode(resolved_path);
        if (dir_ino < 0)
        {
            return FS_ERR_NOT_FOUND;
        }

        struct diskfs_dirent disk_entries[FS_MAX_FILES];
        const int count = diskfs_readdir((uint32_t)dir_ino, disk_entries, FS_MAX_FILES);
        if (count < 0)
        {
            return FS_ERR_NOT_DIR;
        }

        uint32_t out_count = 0;
        for (int i = 0; i < count && out_count < max_entries; i++)
        {
            if (disk_entries[i].inode == 0)
            {
                continue;
            }

            struct diskfs_inode inode;
            if (diskfs_stat(disk_entries[i].inode, &inode) != 0)
            {
                continue;
            }

            strncpy(entries[out_count].name, disk_entries[i].name, FS_ABI_NAME_MAX - 1);
            entries[out_count].name[FS_ABI_NAME_MAX - 1] = '\0';
            entries[out_count].type = (inode.type == DISKFS_TYPE_DIR) ? FS_ABI_TYPE_DIR : FS_ABI_TYPE_FILE;
            entries[out_count].size = inode.size;
            out_count++;
        }

        return (int)out_count;
    }

    int dir_idx;
    if (path == NULL || path[0] == '\0' || strcmp(path, ".") == 0)
    {
        dir_idx = (int)active_cwd_node();
    }
    else
    {
        dir_idx = resolve_full_path(path);
    }

    if (dir_idx < 0)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (fs_nodes[dir_idx].type != FS_TYPE_DIR)
    {
        return FS_ERR_NOT_DIR;
    }

    uint32_t out_count = 0;
    for (int i = 0; i < FS_MAX_FILES && out_count < max_entries; i++)
    {
        if (!fs_nodes[i].used || fs_nodes[i].parent_idx != (uint32_t)dir_idx || i == dir_idx)
        {
            continue;
        }

        strncpy(entries[out_count].name, fs_nodes[i].name, FS_ABI_NAME_MAX - 1);
        entries[out_count].name[FS_ABI_NAME_MAX - 1] = '\0';
        entries[out_count].type = (fs_nodes[i].type == FS_TYPE_DIR) ? FS_ABI_TYPE_DIR : FS_ABI_TYPE_FILE;
        entries[out_count].size = fs_nodes[i].size;
        out_count++;
    }

    return (int)out_count;
}

void fs_clear_cache(void)
{
    for (int i = 1; i < FS_MAX_FILES; i++)
    {
        fs_nodes[i].used = 0;
        memset(&fs_nodes[i], 0, sizeof(struct fs_node));
    }

    cwd_idx = 0;
    strcpy(cwd, "/");
}

int fs_enable_disk(const uint8_t drive)
{
    const int ret = diskfs_init(drive);
    cwd_diskfs_ino = 0;
    if (ret == 0)
    {
        disk_enabled = 1;
        log_info("Disk filesystem enabled");

        for (int i = 1; i < FS_MAX_FILES; i++)
        {
            if (fs_nodes[i].used)
            {
                const int ino = diskfs_create(0, fs_nodes[i].name,
                                            fs_nodes[i].type == FS_TYPE_FILE ?
                                            DISKFS_TYPE_FILE : DISKFS_TYPE_DIR);
                if (ino >= 0 && fs_nodes[i].type == FS_TYPE_FILE)
                {
                    diskfs_write((uint32_t)ino, fs_nodes[i].data, 0, fs_nodes[i].size);
                }
            }
        }
        diskfs_sync();
    }
    return ret;
}

int fs_sync(void)
{
    if (disk_enabled)
    {
        return diskfs_sync();
    }
    return 0;
}

int fs_is_disk_enabled(void)
{
    return disk_enabled;
}

int fs_open(const char* path, const int flags)
{
    if (path == NULL || path[0] == '\0') return FS_ERR_INVALID;
    if (flags & ~(FS_OPEN_READ | FS_OPEN_WRITE)) return FS_ERR_INVALID;
    if ((flags & (FS_OPEN_READ | FS_OPEN_WRITE)) == 0) return FS_ERR_INVALID;

    int slot = -1;
    int local_fd = FS_FIRST_USER_FD;
    const pid_t owner = active_pid();
    for (;; local_fd++)
    {
        bool used = false;
        for (int i = 0; i < FS_MAX_OPEN_FILES; i++)
        {
            if (open_files[i].used && open_files[i].owner == owner && open_files[i].fd == local_fd)
            {
                used = true;
                break;
            }
        }
        if (!used) break;
        if (local_fd >= FS_FIRST_USER_FD + FS_MAX_OPEN_FILES) return FS_ERR_FULL;
    }
    for (int i = 0; i < FS_MAX_OPEN_FILES; i++)
    {
        if (!open_files[i].used)
        {
            slot = i;
            break;
        }
    }
    if (slot < 0) return FS_ERR_FULL;

    struct fs_open_file* file = &open_files[slot];
    memset(file, 0, sizeof(*file));

    if (disk_enabled)
    {
        fs_sync();
        const int ino = resolve_to_diskfs_inode(path);
        if (ino < 0) return FS_ERR_NOT_FOUND;

        struct diskfs_inode inode;
        if (diskfs_stat((uint32_t)ino, &inode) != 0)
        {
            return FS_ERR_NOT_FOUND;
        }
        if (inode.type != DISKFS_TYPE_FILE)
        {
            return FS_ERR_IS_DIR;
        }

        file->disk_backed = 1;
        file->disk_ino = ino;
    }
    else
    {
        const int idx = resolve_full_path(path);
        if (idx < 0) return FS_ERR_NOT_FOUND;
        if (fs_nodes[idx].type != FS_TYPE_FILE) return FS_ERR_IS_DIR;

        file->disk_backed = 0;
        file->node_idx = idx;
    }

    file->used = 1;
    file->owner = owner;
    file->fd = local_fd;
    file->flags = flags;
    file->pos = 0;
    strncpy(file->path, path, FS_MAX_PATH - 1);
    file->path[FS_MAX_PATH - 1] = '\0';

    return local_fd;
}

int fs_close(const int fd)
{
    const int slot = fd_to_slot(fd);
    if (slot < 0)
    {
        return slot;
    }

    if (open_files[slot].pipe_backed)
    {
        struct fs_pipe_state* pipe = &pipes[open_files[slot].pipe_id];
        if (open_files[slot].pipe_read_end)
        {
            if (pipe->readers) pipe->readers--;
        }
        else if (pipe->writers)
        {
            pipe->writers--;
        }
        if (pipe->readers == 0 && pipe->writers == 0)
        {
            memset(pipe, 0, sizeof(*pipe));
        }
    }
    else if (open_files[slot].disk_backed)
    {
        diskfs_sync();
    }
    memset(&open_files[slot], 0, sizeof(open_files[slot]));
    return FS_ERR_OK;
}

void fs_process_fork(const pid_t parent, const pid_t child)
{
    for (int i = 0; i < FS_MAX_OPEN_FILES; i++)
    {
        if (!open_files[i].used || open_files[i].owner != parent)
        {
            continue;
        }
        for (int j = 0; j < FS_MAX_OPEN_FILES; j++)
        {
            if (!open_files[j].used)
            {
                open_files[j] = open_files[i];
                open_files[j].owner = child;
                if (open_files[j].pipe_backed)
                {
                    if (open_files[j].pipe_read_end)
                    {
                        pipes[open_files[j].pipe_id].readers++;
                    }
                    else
                    {
                        pipes[open_files[j].pipe_id].writers++;
                    }
                }
                break;
            }
        }
    }
}

void fs_process_cleanup(const pid_t pid)
{
    for (int i = 0; i < FS_MAX_OPEN_FILES; i++)
    {
        if (open_files[i].used && open_files[i].owner == pid)
        {
            if (open_files[i].pipe_backed)
            {
                struct fs_pipe_state* pipe = &pipes[open_files[i].pipe_id];
                if (open_files[i].pipe_read_end)
                {
                    if (pipe->readers) pipe->readers--;
                }
                else if (pipe->writers)
                {
                    pipe->writers--;
                }
                if (!pipe->readers && !pipe->writers) memset(pipe, 0, sizeof(*pipe));
            }
            memset(&open_files[i], 0, sizeof(open_files[i]));
        }
    }
}

static int next_local_fd(const pid_t owner, const int skip)
{
    for (int fd = FS_FIRST_USER_FD; fd < FS_FIRST_USER_FD + FS_MAX_OPEN_FILES; fd++)
    {
        if (fd == skip) continue;
        bool used = false;
        for (int i = 0; i < FS_MAX_OPEN_FILES; i++)
        {
            if (open_files[i].used && open_files[i].owner == owner && open_files[i].fd == fd)
            {
                used = true;
            }
        }
        if (!used) return fd;
    }
    return -1;
}

int fs_pipe(int fds[2])
{
    if (!fds) return FS_ERR_INVALID;
    int pipe_id = -1;
    int slots[2] = { -1, -1 };
    for (int i = 0; i < FS_MAX_PIPES; i++)
    {
        if (!pipes[i].used)
        {
            pipe_id = i;
            break;
        }
    }
    for (int i = 0; i < FS_MAX_OPEN_FILES && slots[1] < 0; i++)
    {
        if (!open_files[i].used)
        {
            if (slots[0] < 0)
            {
                slots[0] = i;
            }
            else
            {
                slots[1] = i;
            }
        }
    }
    if (pipe_id < 0 || slots[1] < 0) return FS_ERR_FULL;

    const pid_t owner = active_pid();
    const int read_fd = next_local_fd(owner, -1);
    const int write_fd = next_local_fd(owner, read_fd);
    if (read_fd < 0 || write_fd < 0) return FS_ERR_FULL;
    memset(&pipes[pipe_id], 0, sizeof(pipes[pipe_id]));
    pipes[pipe_id].used = true;
    pipes[pipe_id].readers = 1;
    pipes[pipe_id].writers = 1;

    memset(&open_files[slots[0]], 0, sizeof(open_files[slots[0]]));
    open_files[slots[0]].used = 1;
    open_files[slots[0]].owner = owner;
    open_files[slots[0]].fd = read_fd;
    open_files[slots[0]].flags = FS_OPEN_READ;
    open_files[slots[0]].pipe_backed = 1;
    open_files[slots[0]].pipe_read_end = 1;
    open_files[slots[0]].pipe_id = pipe_id;

    open_files[slots[1]] = open_files[slots[0]];
    open_files[slots[1]].fd = write_fd;
    open_files[slots[1]].flags = FS_OPEN_WRITE;
    open_files[slots[1]].pipe_read_end = 0;
    fds[0] = read_fd;
    fds[1] = write_fd;
    return 0;
}

int fs_dup2(const int old_fd, const int new_fd)
{
    const int old_slot = fd_to_slot(old_fd);
    if (old_slot < 0 || new_fd < 0) return FS_ERR_INVALID;
    if (old_fd == new_fd) return new_fd;
    const int existing = fd_to_slot(new_fd);
    if (existing >= 0) fs_close(new_fd);
    int slot = -1;
    for (int i = 0; i < FS_MAX_OPEN_FILES; i++)
    {
        if (!open_files[i].used)
        {
            slot = i;
            break;
        }
    }
    if (slot < 0) return FS_ERR_FULL;
    open_files[slot] = open_files[old_slot];
    open_files[slot].fd = new_fd;
    if (open_files[slot].pipe_backed)
    {
        if (open_files[slot].pipe_read_end)
        {
            pipes[open_files[slot].pipe_id].readers++;
        }
        else
        {
            pipes[open_files[slot].pipe_id].writers++;
        }
    }
    return new_fd;
}

int fs_poll_fd(const int fd, const int events)
{
    const int slot = fd_to_slot(fd);
    if (slot < 0) return FS_ERR_INVALID;
    if (!open_files[slot].pipe_backed) return events;
    const struct fs_pipe_state* pipe = &pipes[open_files[slot].pipe_id];
    int ready = 0;
    if ((events & 1) && (pipe->head != pipe->tail || pipe->writers == 0)) ready |= 1;
    if ((events & 2) && ((pipe->tail + 1U) % FS_PIPE_BUFFER != pipe->head) && pipe->readers) ready |= 2;
    return ready;
}

int fs_fd_is_open(const int fd)
{
    return fd_to_slot(fd) >= 0;
}
