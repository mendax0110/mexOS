#ifndef TEST_PMM_H
#define TEST_PMM_H

#include "../test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the PMM test suite
 * @return Pointer to the PMM test suite
 */
struct test_suite* test_pmm_get_suite(void);

#ifdef __cplusplus
}
#endif

#endif
