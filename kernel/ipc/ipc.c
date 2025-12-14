#include "ipc.h"
#include "../mm/heap.h"
#include "../../shared/string.h"
#include "../sched/sched.h"

#define MSG_QUEUE_SIZE 16

static struct port ports[MAX_PORTS];
static uint32_t port_count = 0;

void ipc_init(void)
{
    memset(ports, 0, sizeof(ports));
    port_count = 0;
}

int port_create(const pid_t owner)
{
    if (port_count >= MAX_PORTS) return -1;

    for (uint32_t i = 0; i < MAX_PORTS; i++)
    {
        if (ports[i].owner == 0)
        {
            ports[i].owner = owner;
            ports[i].id = i;
            ports[i].flags = 0;
            ports[i].queue = (struct message*)kmalloc(sizeof(struct message) * MSG_QUEUE_SIZE);
            if (!ports[i].queue) return -1;
            memset(ports[i].queue, 0, sizeof(struct message) * MSG_QUEUE_SIZE);
            ports[i].queue_head = 0;
            ports[i].queue_tail = 0;
            ports[i].queue_size = MSG_QUEUE_SIZE;
            ports[i].waiting_sender = 0;
            ports[i].waiting_receiver = 0;
            port_count++;
            return (int)i;
        }
    }
    return -1;
}

int port_destroy(const int port_id)
{
    if (port_id < 0 || (uint32_t)port_id >= MAX_PORTS) return -1;
    if (ports[port_id].owner == 0) return -1;

    if (ports[port_id].waiting_sender)
    {
        sched_unblock(ports[port_id].waiting_sender);
    }

    if (ports[port_id].waiting_receiver)
    {
        sched_unblock(ports[port_id].waiting_receiver);
    }

    if (ports[port_id].queue)
    {
        kfree(ports[port_id].queue);
    }
    memset(&ports[port_id], 0, sizeof(struct port));
    port_count--;
    return 0;
}

int msg_send(const int port_id, struct message* msg, const uint32_t flags)
{
    if (port_id < 0 || (uint32_t)port_id >= MAX_PORTS) return -1;
    if (ports[port_id].owner == 0) return -1;
    if (!msg) return -1;

    struct port* p = &ports[port_id];

    while (1)
    {
        const uint32_t next_tail = (p->queue_tail + 1) % p->queue_size;

        if (next_tail == p->queue_head)
        {
            if (flags & IPC_NONBLOCK) return -2;

            struct task* current = sched_get_current();
            if (current)
            {
                p->waiting_sender = current->id;
                sched_block(0);  // Block and reschedule
                continue;
            }
            return -2;
        }

        memcpy(&p->queue[p->queue_tail], msg, sizeof(struct message));
        p->queue_tail = next_tail;

        if (p->waiting_receiver)
        {
            sched_unblock(p->waiting_receiver);
            p->waiting_receiver = 0;
        }

        return 0;
    }
}

int msg_receive(const int port_id, struct message* msg, const uint32_t flags)
{
    if (port_id < 0 || (uint32_t)port_id >= MAX_PORTS) return -1;
    if (ports[port_id].owner == 0) return -1;
    if (!msg) return -1;

    struct port* p = &ports[port_id];

    while (1)
    {
        if (p->queue_head == p->queue_tail)
        {
            if (flags & IPC_NONBLOCK) return -2;

            struct task* current = sched_get_current();
            if (current)
            {
                p->waiting_receiver = current->id;
                sched_block(0);  // Block and reschedule
                continue;
            }
            return -2;
        }

        memcpy(msg, &p->queue[p->queue_head], sizeof(struct message));
        p->queue_head = (p->queue_head + 1) % p->queue_size;

        if (p->waiting_sender)
        {
            sched_unblock(p->waiting_sender);
            p->waiting_sender = 0;
        }

        return 0;
    }
}

int msg_reply(const pid_t dest, struct message* msg)
{
    // finds port owned by dest process
    for (uint32_t i = 0; i < MAX_PORTS; i++)
    {
        if (ports[i].owner == dest)
        {
            return msg_send((int)i, msg, IPC_NONBLOCK);
        }
    }
    return -1;
}
