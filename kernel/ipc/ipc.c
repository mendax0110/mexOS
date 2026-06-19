#include "ipc.h"
#include "../mm/heap.h"
#include "../lib/string.h"
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
        if (!ports[i].in_use)
        {
            ports[i].in_use  = true;
            ports[i].owner   = owner;
            ports[i].id      = i;
            ports[i].flags   = 0;
            ports[i].queue   = (struct message*)kmalloc(sizeof(struct message) * MSG_QUEUE_SIZE);
            if (!ports[i].queue) return -1;
            memset(ports[i].queue, 0, sizeof(struct message) * MSG_QUEUE_SIZE);
            ports[i].queue_head = 0;
            ports[i].queue_tail = 0;
            ports[i].queue_size = MSG_QUEUE_SIZE;
            memset(ports[i].waiting_senders,   0, sizeof(ports[i].waiting_senders));
            memset(ports[i].waiting_receivers, 0, sizeof(ports[i].waiting_receivers));
            ports[i].waiting_sender_count   = 0;
            ports[i].waiting_receiver_count = 0;
            port_count++;
            return (int)i;
        }
    }
    return -1;
}

int port_destroy(const int port_id)
{
    if (port_id < 0 || (uint32_t)port_id >= MAX_PORTS) return -1;
    if (!ports[port_id].in_use) return -1;

    for (uint32_t i = 0; i < ports[port_id].waiting_sender_count; i++)
    {
        sched_unblock(ports[port_id].waiting_senders[i]);
    }
    for (uint32_t i = 0; i < ports[port_id].waiting_receiver_count; i++)
    {
        sched_unblock(ports[port_id].waiting_receivers[i]);
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
    if (!ports[port_id].in_use) return -1;
    if (!msg) return -1;

    struct port* p = &ports[port_id];

    while (1)
    {
        if (!p->in_use) return -1;

        const uint32_t next_tail = (p->queue_tail + 1) % p->queue_size;

        if (next_tail == p->queue_head)
        {
            if (flags & IPC_NONBLOCK) return -1;

            struct task* current = sched_get_current();
            if (!current) return -1;
            if (p->waiting_sender_count < IPC_WAIT_QUEUE_SIZE)
            {
                p->waiting_senders[p->waiting_sender_count++] = current->id;
            }
            sched_block(BLOCK_WAITING);
            continue;
        }

        memcpy(&p->queue[p->queue_tail], msg, sizeof(struct message));
        p->queue_tail = next_tail;

        if (p->waiting_receiver_count > 0)
        {
            sched_unblock(p->waiting_receivers[0]);
            p->waiting_receiver_count--;
            for (uint32_t i = 0; i < p->waiting_receiver_count; i++)
            {
                p->waiting_receivers[i] = p->waiting_receivers[i + 1];
            }
        }

        return 0;
    }
}

int msg_receive(const int port_id, struct message* msg, const uint32_t flags)
{
    if (port_id < 0 || (uint32_t)port_id >= MAX_PORTS) return -1;
    if (!ports[port_id].in_use) return -1;
    if (!msg) return -1;

    struct port* p = &ports[port_id];

    while (1)
    {
        if (!p->in_use) return -1;

        if (p->queue_head == p->queue_tail)
        {
            if (flags & IPC_NONBLOCK) return -1;

            struct task* current = sched_get_current();
            if (!current) return -1;
            if (p->waiting_receiver_count < IPC_WAIT_QUEUE_SIZE)
            {
                p->waiting_receivers[p->waiting_receiver_count++] = current->id;
            }
            sched_block(BLOCK_WAITING);
            continue;
        }

        memcpy(msg, &p->queue[p->queue_head], sizeof(struct message));
        p->queue_head = (p->queue_head + 1) % p->queue_size;

        if (p->waiting_sender_count > 0)
        {
            sched_unblock(p->waiting_senders[0]);
            p->waiting_sender_count--;
            for (uint32_t i = 0; i < p->waiting_sender_count; i++)
            {
                p->waiting_senders[i] = p->waiting_senders[i + 1];
            }
        }

        return 0;
    }
}

int msg_reply(const pid_t dest, struct message* msg)
{
    for (uint32_t i = 0; i < MAX_PORTS; i++)
    {
        if (ports[i].in_use && ports[i].owner == dest)
        {
            return msg_send((int)i, msg, IPC_NONBLOCK);
        }
    }
    return -1;
}
