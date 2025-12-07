#ifndef TEST_HEAP_H
#define TEST_HEAP_H

#include "../test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the heap test suite
 * @return Pointer to the heap test suite
 */
struct test_suite* test_heap_get_suite(void);

#ifdef __cplusplus
}
#endif

#endif
