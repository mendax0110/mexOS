#ifndef TEST_RTC_H
#define TEST_RTC_H

#include "../test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the RTC test suite
 * @return Pointer to the RTC test suite
 */
struct test_suite* test_rtc_get_suite(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_RTC_H