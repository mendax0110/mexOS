#ifndef TEST_ROLLBACK_H
#define TEST_ROLLBACK_H

#include "../test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the rollback test suite
 * @return Pointer to the rollback test suite
 */
struct test_suite* test_rollback_get_suite(void);

#ifdef __cplusplus
}
#endif


#endif // TEST_ROLLBACK_H