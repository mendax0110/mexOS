#ifndef TEST_STRING_H
#define TEST_STRING_H

#include "../test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the string functions test suite
 * @return Pointer to the string test suite
 */
struct test_suite* test_string_get_suite(void);

#ifdef __cplusplus
}
#endif

#endif
