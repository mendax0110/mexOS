#ifndef TEST_STRESS_H
#define TEST_STRESS_H

#include "../test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the stress test suite
 * @return Pointer to the stress test suite
 */
struct test_suite* test_stress_get_suite(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_STRESS_H
