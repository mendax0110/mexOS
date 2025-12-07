#ifndef TEST_SCHED_H
#define TEST_SCHED_H

#include "../test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the scheduler test suite
 * @return Pointer to the scheduler test suite
 */
struct test_suite* test_sched_get_suite(void);

#ifdef __cplusplus
}
#endif

#endif
