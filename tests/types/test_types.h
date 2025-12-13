#ifndef TEST_TYPES_H
#define TEST_TYPES_H

#include "../test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the types test suite
 * @return Pointer to the types test suite
 */
struct test_suite* test_types_get_suite(void);

#ifdef __cplusplus
}
#endif

#endif
