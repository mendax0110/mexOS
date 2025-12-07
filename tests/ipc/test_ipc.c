#include "test_ipc.h"
#include "../../kernel/ipc/ipc.h"
#include "../../kernel/include/string.h"

TEST_CASE(ipc_port_create_success)
{
    int port = port_create(1);
    TEST_ASSERT_GE(port, 0);
    port_destroy(port);
    return TEST_PASS;
}

TEST_CASE(ipc_port_create_multiple)
{
    int p1 = port_create(1);
    int p2 = port_create(1);
    int p3 = port_create(1);
    TEST_ASSERT_GE(p1, 0);
    TEST_ASSERT_GE(p2, 0);
    TEST_ASSERT_GE(p3, 0);
    TEST_ASSERT_NEQ(p1, p2);
    TEST_ASSERT_NEQ(p2, p3);
    TEST_ASSERT_NEQ(p1, p3);
    port_destroy(p1);
    port_destroy(p2);
    port_destroy(p3);
    return TEST_PASS;
}

TEST_CASE(ipc_port_destroy_success)
{
    int port = port_create(1);
    TEST_ASSERT_GE(port, 0);
    int ret = port_destroy(port);
    TEST_ASSERT_EQ(ret, 0);
    return TEST_PASS;
}

TEST_CASE(ipc_port_destroy_invalid)
{
    int ret = port_destroy(-1);
    TEST_ASSERT_EQ(ret, -1);
    return TEST_PASS;
}

TEST_CASE(ipc_port_destroy_invalid_high)
{
    int ret = port_destroy(9999);
    TEST_ASSERT_EQ(ret, -1);
    return TEST_PASS;
}

TEST_CASE(ipc_msg_send_nonblock_empty)
{
    int port = port_create(1);
    if (port < 0)
    {
        return TEST_SKIP;
    }
    struct message msg;
    memset(&msg, 0, sizeof(msg));
    msg.sender = 1;
    msg.receiver = 1;
    msg.type = MSG_SEND;
    msg.len = 4;
    memcpy(msg.data, "test", 4);
    int ret = msg_send(port, &msg, IPC_NONBLOCK);
    TEST_ASSERT_EQ(ret, 0);
    port_destroy(port);
    return TEST_PASS;
}

TEST_CASE(ipc_msg_receive_nonblock_empty)
{
    int port = port_create(1);
    if (port < 0)
    {
        return TEST_SKIP;
    }
    struct message msg;
    memset(&msg, 0, sizeof(msg));
    int ret = msg_receive(port, &msg, IPC_NONBLOCK);
    TEST_ASSERT_EQ(ret, -1);
    port_destroy(port);
    return TEST_PASS;
}

TEST_CASE(ipc_msg_send_receive_roundtrip)
{
    int port = port_create(1);
    if (port < 0)
    {
        return TEST_SKIP;
    }
    struct message send_msg;
    memset(&send_msg, 0, sizeof(send_msg));
    send_msg.sender = 1;
    send_msg.receiver = 1;
    send_msg.type = MSG_SEND;
    send_msg.len = 5;
    memcpy(send_msg.data, "hello", 5);
    int send_ret = msg_send(port, &send_msg, IPC_NONBLOCK);
    TEST_ASSERT_EQ(send_ret, 0);
    struct message recv_msg;
    memset(&recv_msg, 0, sizeof(recv_msg));
    int recv_ret = msg_receive(port, &recv_msg, IPC_NONBLOCK);
    TEST_ASSERT_EQ(recv_ret, 0);
    TEST_ASSERT_EQ(recv_msg.len, 5);
    TEST_ASSERT_EQ(memcmp(recv_msg.data, "hello", 5), 0);
    port_destroy(port);
    return TEST_PASS;
}

TEST_CASE(ipc_msg_send_invalid_port)
{
    struct message msg;
    memset(&msg, 0, sizeof(msg));
    int ret = msg_send(-1, &msg, IPC_NONBLOCK);
    TEST_ASSERT_EQ(ret, -1);
    return TEST_PASS;
}

TEST_CASE(ipc_msg_receive_invalid_port)
{
    struct message msg;
    memset(&msg, 0, sizeof(msg));
    int ret = msg_receive(-1, &msg, IPC_NONBLOCK);
    TEST_ASSERT_EQ(ret, -1);
    return TEST_PASS;
}

TEST_CASE(ipc_port_reuse_after_destroy)
{
    int p1 = port_create(1);
    TEST_ASSERT_GE(p1, 0);
    port_destroy(p1);
    int p2 = port_create(1);
    TEST_ASSERT_GE(p2, 0);
    port_destroy(p2);
    return TEST_PASS;
}

static struct test_case ipc_cases[] = {
        TEST_ENTRY(ipc_port_create_success),
        TEST_ENTRY(ipc_port_create_multiple),
        TEST_ENTRY(ipc_port_destroy_success),
        TEST_ENTRY(ipc_port_destroy_invalid),
        TEST_ENTRY(ipc_port_destroy_invalid_high),
        TEST_ENTRY(ipc_msg_send_nonblock_empty),
        TEST_ENTRY(ipc_msg_receive_nonblock_empty),
        TEST_ENTRY(ipc_msg_send_receive_roundtrip),
        TEST_ENTRY(ipc_msg_send_invalid_port),
        TEST_ENTRY(ipc_msg_receive_invalid_port),
        TEST_ENTRY(ipc_port_reuse_after_destroy),
        TEST_SUITE_END
};

static struct test_suite ipc_suite = {
        .name = "IPC Tests",
        .cases = ipc_cases,
        .count = 11
};

struct test_suite* test_ipc_get_suite(void)
{
    return &ipc_suite;
}
