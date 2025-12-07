#ifndef TEST_IPC_H
#define TEST_IPC_H

#include "../test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the IPC test suite
 * @return Pointer to the IPC test suite
 */
struct test_suite* test_ipc_get_suite(void);

#ifdef __cplusplus
}
#endif

#endif
