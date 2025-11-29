#include "fs.h"
#include "../include/string.h"

static struct fs_node fs_nodes[FS_MAX_FILES];
static char cwd[FS_MAX_PATH];
static uint32_t cwd_idx;

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

static int find_node_in_dir(uint32_t dir_idx, const char* name)
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

    uint32_t current = (buf[0] == '/') ? 0 : cwd_idx;

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
    char* name_part = last_slash + 1;

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

        int idx = find_node_in_dir(current, component);
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

    int ret = resolve_path(path, &parent_idx, basename);
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

void fs_init(void)
{
    memset(fs_nodes, 0, sizeof(fs_nodes));

    fs_nodes[0].used = 1;
    fs_nodes[0].type = FS_TYPE_DIR;
    strcpy(fs_nodes[0].name, "/");
    fs_nodes[0].parent_idx = 0;
    fs_nodes[0].size = 0;

    cwd_idx = 0;
    strcpy(cwd, "/");
}

int fs_create_file(const char* path)
{
    uint32_t parent_idx;
    char basename[FS_MAX_NAME];

    int ret = resolve_path(path, &parent_idx, basename);
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

    int idx = find_free_node();
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

int fs_create_dir(const char* path)
{
    uint32_t parent_idx;
    char basename[FS_MAX_NAME];

    int ret = resolve_path(path, &parent_idx, basename);
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

    int idx = find_free_node();
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
    int idx = resolve_full_path(path);
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

int fs_read(const char* path, char* buffer, uint32_t size)
{
    int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (fs_nodes[idx].type != FS_TYPE_FILE)
    {
        return FS_ERR_IS_DIR;
    }

    uint32_t to_read = (size < fs_nodes[idx].size) ? size : fs_nodes[idx].size;
    memcpy(buffer, fs_nodes[idx].data, to_read);

    return (int)to_read;
}

int fs_write(const char* path, const char* data, uint32_t size)
{
    int idx = resolve_full_path(path);
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

int fs_append(const char* path, const char* data, uint32_t size)
{
    int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (fs_nodes[idx].type != FS_TYPE_FILE)
    {
        return FS_ERR_IS_DIR;
    }

    uint32_t available = FS_MAX_FILE_SIZE - fs_nodes[idx].size;
    if (size > available)
    {
        size = available;
    }

    memcpy(fs_nodes[idx].data + fs_nodes[idx].size, data, size);
    fs_nodes[idx].size += size;

    return (int)size;
}

int fs_list_dir(const char* path, char* buffer, uint32_t size)
{
    int idx;

    if (path == NULL || path[0] == '\0' || strcmp(path, ".") == 0)
    {
        idx = (int)cwd_idx;
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
            uint32_t name_len = (uint32_t)strlen(fs_nodes[i].name);
            uint32_t entry_len = name_len + 2;

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
    if (path == NULL || path[0] == '\0')
    {
        cwd_idx = 0;
        strcpy(cwd, "/");
        return FS_ERR_OK;
    }

    int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return FS_ERR_NOT_FOUND;
    }

    if (fs_nodes[idx].type != FS_TYPE_DIR)
    {
        return FS_ERR_NOT_DIR;
    }

    cwd_idx = (uint32_t)idx;

    if (idx == 0)
    {
        strcpy(cwd, "/");
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

        cwd[0] = '\0';
        for (int i = depth - 1; i >= 0; i--)
        {
            strcat(cwd, "/");
            strcat(cwd, parts[i]);
        }
    }

    return FS_ERR_OK;
}

const char* fs_get_cwd(void)
{
    return cwd;
}

int fs_exists(const char* path)
{
    return resolve_full_path(path) >= 0;
}

int fs_is_dir(const char* path)
{
    int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return 0;
    }
    return fs_nodes[idx].type == FS_TYPE_DIR;
}

uint32_t fs_get_size(const char* path)
{
    int idx = resolve_full_path(path);
    if (idx < 0)
    {
        return 0;
    }
    return fs_nodes[idx].size;
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
