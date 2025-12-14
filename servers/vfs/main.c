#include "protocol.h"
#include "fs.h"
#include "diskfs.h"
#include "../lib/ipc_client.h"
#include "../lib/memory.h"

/** Server heap memory */
static uint8_t server_heap[131072] __attribute__((aligned(4096)));

/** Server port ID */
static int server_port = -1;

/** Filesystem nodes */
static struct vfs_node nodes[VFS_MAX_NODES];

/** File descriptors */
static struct vfs_fd fds[VFS_MAX_FDS * 16];

/** Next file descriptor number */
static int next_fd = 3;

/**
 * @brief Find a free node
 * @return Node index, or -1 if none available
 */
static int find_free_node(void)
{
    for (int i = 0; i < VFS_MAX_NODES; i++)
    {
        if (!nodes[i].used)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Find a node by path
 * @param path File path
 * @param parent_idx Output: parent directory index
 * @return Node index, or -1 if not found
 */
static int find_node(const char *path, uint32_t *parent_idx)
{
    if (path == NULL || path[0] == '\0')
    {
        return -1;
    }

    uint32_t current = 0;
    if (parent_idx)
    {
        *parent_idx = 0;
    }

    const char *p = path;
    if (*p == '/')
    {
        p++;
    }

    if (*p == '\0')
    {
        return 0;
    }

    while (*p)
    {
        char component[VFS_MAX_NAME];
        int len = 0;

        while (*p && *p != '/' && len < VFS_MAX_NAME - 1)
        {
            component[len++] = *p++;
        }
        component[len] = '\0';

        if (*p == '/')
        {
            p++;
        }

        if (parent_idx)
        {
            *parent_idx = current;
        }

        int found = -1;
        for (int i = 0; i < VFS_MAX_NODES; i++)
        {
            if (nodes[i].used && nodes[i].parent_idx == current)
            {
                const char *n1 = nodes[i].name;
                const char *n2 = component;
                while (*n1 && *n1 == *n2)
                {
                    n1++;
                    n2++;
                }
                if (*n1 == '\0' && *n2 == '\0')
                {
                    found = i;
                    break;
                }
            }
        }

        if (found < 0)
        {
            return -1;
        }

        current = (uint32_t)found;
    }

    return (int)current;
}

/**
 * @brief Allocate a file descriptor
 * @param owner Owner process ID
 * @param node_idx Node index
 * @param flags Open flags
 * @return FD number, or -1 on error
 */
static int alloc_fd(pid_t owner, int node_idx, uint16_t flags)
{
    for (int i = 0; i < (int)(sizeof(fds) / sizeof(fds[0])); i++)
    {
        if (!fds[i].used)
        {
            fds[i].owner = owner;
            fds[i].node_idx = node_idx;
            fds[i].position = 0;
            fds[i].flags = flags;
            fds[i].used = 1;
            return next_fd++;
        }
    }
    return -1;
}

/**
 * @brief Find FD by number and owner
 * @param fd FD number
 * @param owner Owner process ID
 * @return FD structure pointer, or NULL
 */
static struct vfs_fd *find_fd(int fd, pid_t owner)
{
    for (int i = 0; i < (int)(sizeof(fds) / sizeof(fds[0])); i++)
    {
        if (fds[i].used && fds[i].owner == owner)
        {
            static int fd_map = 0;
            fd_map++;
            if (fd_map == fd)
            {
                return &fds[i];
            }
        }
    }
    return NULL;
}

/**
 * @brief Handle open request
 * @param msg Incoming message
 */
static void handle_open(struct message *msg)
{
    struct vfs_open_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct vfs_open_response resp = { .status = VFS_ERR_NOTFOUND, .fd = -1 };

    uint32_t parent_idx;
    int node_idx = find_node(req.path, &parent_idx);

    if (node_idx < 0)
    {
        if (req.flags & VFS_O_CREATE)
        {
            node_idx = find_free_node();
            if (node_idx >= 0)
            {
                struct vfs_node *node = &nodes[node_idx];
                node->used = 1;
                node->type = VFS_TYPE_FILE;
                node->size = 0;
                node->parent_idx = parent_idx;
                node->data = NULL;

                const char *name = req.path;
                const char *slash = req.path;
                while (*slash)
                {
                    if (*slash == '/')
                    {
                        name = slash + 1;
                    }
                    slash++;
                }

                int i = 0;
                while (*name && i < VFS_MAX_NAME - 1)
                {
                    node->name[i++] = *name++;
                }
                node->name[i] = '\0';
            }
        }
    }

    if (node_idx >= 0)
    {
        int fd = alloc_fd(msg->sender, node_idx, req.flags);
        if (fd >= 0)
        {
            resp.status = 0;
            resp.fd = fd;
        }
        else
        {
            resp.status = VFS_ERR_NOMEM;
        }
    }

    msg->type = VFS_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle close request
 * @param msg Incoming message
 */
static void handle_close(struct message *msg)
{
    struct vfs_close_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct vfs_response resp = { .status = VFS_ERR_INVALID };

    for (int i = 0; i < (int)(sizeof(fds) / sizeof(fds[0])); i++)
    {
        if (fds[i].used && fds[i].owner == msg->sender)
        {
            fds[i].used = 0;
            resp.status = 0;
            break;
        }
    }

    msg->type = VFS_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle read request
 * @param msg Incoming message
 */
static void handle_read(struct message *msg)
{
    struct vfs_read_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct vfs_read_response resp;
    mem_set(&resp, 0, sizeof(resp));
    resp.status = VFS_ERR_INVALID;

    struct vfs_fd *fd = find_fd(req.fd, msg->sender);
    if (fd && fd->node_idx >= 0 && fd->node_idx < VFS_MAX_NODES)
    {
        struct vfs_node *node = &nodes[fd->node_idx];
        if (node->used && node->type == VFS_TYPE_FILE)
        {
            uint32_t available = node->size - fd->position;
            uint32_t to_read = req.size;
            if (to_read > available)
            {
                to_read = available;
            }
            if (to_read > VFS_MAX_DATA)
            {
                to_read = VFS_MAX_DATA;
            }

            if (node->data && to_read > 0)
            {
                mem_copy(resp.data, node->data + fd->position, to_read);
            }

            fd->position += to_read;
            resp.status = (int32_t)to_read;
        }
    }

    msg->type = VFS_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle write request
 * @param msg Incoming message
 */
static void handle_write(struct message *msg)
{
    struct vfs_write_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct vfs_write_response resp = { .status = VFS_ERR_INVALID };

    struct vfs_fd *fd = find_fd(req.fd, msg->sender);
    if (fd && fd->node_idx >= 0 && fd->node_idx < VFS_MAX_NODES)
    {
        struct vfs_node *node = &nodes[fd->node_idx];
        if (node->used && node->type == VFS_TYPE_FILE)
        {
            uint32_t new_size = fd->position + req.size;
            if (new_size > VFS_MAX_FILE_SIZE)
            {
                resp.status = VFS_ERR_NOSPACE;
            }
            else
            {
                if (new_size > node->size)
                {
                    uint8_t *new_data = mem_alloc(new_size);
                    if (new_data)
                    {
                        if (node->data)
                        {
                            mem_copy(new_data, node->data, node->size);
                            mem_free(node->data);
                        }
                        node->data = new_data;
                        node->size = new_size;
                    }
                    else
                    {
                        resp.status = VFS_ERR_NOMEM;
                        goto done;
                    }
                }

                mem_copy(node->data + fd->position, req.data, req.size);
                fd->position += req.size;
                resp.status = (int32_t)req.size;
            }
        }
    }

    done:
    msg->type = VFS_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle mkdir request
 * @param msg Incoming message
 */
static void handle_mkdir(struct message *msg)
{
    struct vfs_path_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct vfs_response resp = { .status = VFS_ERR_NOSPACE };

    uint32_t parent_idx;
    int existing = find_node(req.path, &parent_idx);

    if (existing >= 0)
    {
        resp.status = VFS_ERR_EXISTS;
    }
    else
    {
        int node_idx = find_free_node();
        if (node_idx >= 0)
        {
            struct vfs_node *node = &nodes[node_idx];
            node->used = 1;
            node->type = VFS_TYPE_DIR;
            node->size = 0;
            node->parent_idx = parent_idx;
            node->data = NULL;

            const char *name = req.path;
            const char *slash = req.path;
            while (*slash)
            {
                if (*slash == '/')
                {
                    name = slash + 1;
                }
                slash++;
            }

            int i = 0;
            while (*name && i < VFS_MAX_NAME - 1)
            {
                node->name[i++] = *name++;
            }
            node->name[i] = '\0';

            resp.status = 0;
        }
    }

    msg->type = VFS_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle stat request
 * @param msg Incoming message
 */
static void handle_stat(struct message *msg)
{
    struct vfs_stat_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct vfs_stat_response resp;
    mem_set(&resp, 0, sizeof(resp));

    uint32_t parent_idx;
    int node_idx = find_node(req.path, &parent_idx);

    if (node_idx >= 0)
    {
        struct vfs_node *node = &nodes[node_idx];
        resp.status = 0;
        resp.info.type = node->type;
        resp.info.size = node->size;
        resp.info.created = node->created;
        resp.info.modified = node->modified;
    }
    else
    {
        resp.status = VFS_ERR_NOTFOUND;
    }

    msg->type = VFS_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Process a message from a client
 * @param msg The received message
 */
static void process_message(struct message *msg)
{
    switch (msg->type)
    {
        case VFS_MSG_OPEN:
            handle_open(msg);
            break;
        case VFS_MSG_CLOSE:
            handle_close(msg);
            break;
        case VFS_MSG_READ:
            handle_read(msg);
            break;
        case VFS_MSG_WRITE:
            handle_write(msg);
            break;
        case VFS_MSG_MKDIR:
            handle_mkdir(msg);
            break;
        case VFS_MSG_STAT:
            handle_stat(msg);
            break;
        default:
        {
            struct vfs_response resp = { .status = VFS_ERR_INVALID };
            msg->type = VFS_MSG_RESPONSE;
            ipc_msg_set_data(msg, &resp, sizeof(resp));
            ipc_reply(msg);
            break;
        }
    }
}

/**
 * @brief Initialize filesystem
 */
static void init_filesystem(void)
{
    mem_set(nodes, 0, sizeof(nodes));
    mem_set(fds, 0, sizeof(fds));

    nodes[0].used = 1;
    nodes[0].type = VFS_TYPE_DIR;
    nodes[0].name[0] = '/';
    nodes[0].name[1] = '\0';
    nodes[0].parent_idx = 0;
}

/**
 * @brief VFS server main function
 * @return Does not return
 */
int main(void)
{
    mem_init(server_heap, sizeof(server_heap));

    ipc_client_init();

    server_port = port_create();
    if (server_port < 0)
    {
        return -1;
    }

    ipc_register_server(VFS_SERVER_PORT_NAME, server_port);

    init_filesystem();

    struct message msg;
    while (1)
    {
        int ret = ipc_receive(server_port, &msg, true);
        if (ret == IPC_SUCCESS)
        {
            process_message(&msg);
        }
    }

    return 0;
}
