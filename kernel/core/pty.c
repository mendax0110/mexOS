#include "pty.h"
#include "sched/sched.h"
#include "lib/string.h"

#define PTY_MAX 8
#define PTY_BUFFER_SIZE 2048

/**
 * @brief Ring buffer structure for pseudo-terminal input/output. \struct byte_ring
 */
struct byte_ring
{
    char data[PTY_BUFFER_SIZE];
    uint32_t head;
    uint32_t tail;
};

/**
 * @brief Pseudo-terminal structure representing a master-slave pair. \struct pty
 */
struct pty
{
    bool used;
    pid_t owner;
    struct byte_ring input;
    struct byte_ring output;
};

static struct pty ptys[PTY_MAX];

static int ring_write(struct byte_ring* ring, const char* data, const uint32_t size)
{
    uint32_t written = 0;
    while (written < size)
    {
        const uint32_t next = (ring->tail + 1U) % PTY_BUFFER_SIZE;
        if (next == ring->head)
        {
            break;
        }
        ring->data[ring->tail] = data[written++];
        ring->tail = next;
    }
    return (int)written;
}

static int ring_read(struct byte_ring* ring, char* data, const uint32_t size)
{
    uint32_t read_count = 0;
    while (read_count < size && ring->head != ring->tail)
    {
        data[read_count++] = ring->data[ring->head];
        ring->head = (ring->head + 1U) % PTY_BUFFER_SIZE;
    }
    return (int)read_count;
}

void pty_init(void)
{
    memset(ptys, 0, sizeof(ptys));
}

int pty_create(const pid_t owner)
{
    for (int i = 0; i < PTY_MAX; i++)
    {
        if (!ptys[i].used)
        {
            memset(&ptys[i], 0, sizeof(ptys[i]));
            ptys[i].used = true;
            ptys[i].owner = owner;
            return i;
        }
    }
    return -1;
}

int pty_destroy(const int id, const pid_t caller)
{
    if (id < 0 || id >= PTY_MAX || !ptys[id].used || ptys[id].owner != caller)
    {
        return -1;
    }
    memset(&ptys[id], 0, sizeof(ptys[id]));
    return 0;
}

int pty_attach_slave(const int id)
{
    struct task* task = sched_get_current();
    if (!task || id < 0 || id >= PTY_MAX || !ptys[id].used)
    {
        return -1;
    }
    task->stdin_pty = id;
    task->stdout_pty = id;
    task->session_id = ptys[id].owner;
    task->process_group = task->pid;
    return 0;
}

int pty_master_read(const int id, char* buffer, const uint32_t size, const pid_t caller)
{
    if (id < 0 || id >= PTY_MAX || !ptys[id].used || ptys[id].owner != caller || !buffer)
    {
        return -1;
    }
    return ring_read(&ptys[id].output, buffer, size);
}

int pty_master_write(const int id, const char* buffer, const uint32_t size, const pid_t caller)
{
    if (id < 0 || id >= PTY_MAX || !ptys[id].used || ptys[id].owner != caller || !buffer)
    {
        return -1;
    }
    return ring_write(&ptys[id].input, buffer, size);
}

int pty_slave_read(const int id, char* buffer, const uint32_t size)
{
    if (id < 0 || id >= PTY_MAX || !ptys[id].used || !buffer || size == 0)
    {
        return -1;
    }
    int result = 0;
    while (result == 0)
    {
        result = ring_read(&ptys[id].input, buffer, size);
        if (result == 0)
        {
            sched_yield();
        }
    }
    return result;
}

int pty_slave_write(const int id, const char* buffer, const uint32_t size)
{
    if (id < 0 || id >= PTY_MAX || !ptys[id].used || !buffer)
    {
        return -1;
    }
    uint32_t total = 0;
    while (total < size)
    {
        const int written = ring_write(&ptys[id].output, buffer + total, size - total);
        if (written <= 0)
        {
            sched_yield();
            continue;
        }
        total += (uint32_t)written;
    }
    return (int)total;
}

void pty_process_cleanup(const pid_t pid)
{
    for (int i = 0; i < PTY_MAX; i++)
    {
        if (ptys[i].used && ptys[i].owner == pid)
        {
            memset(&ptys[i], 0, sizeof(ptys[i]));
        }
    }
}
