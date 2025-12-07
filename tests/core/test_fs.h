#ifndef TEST_FS_H
#define TEST_FS_H

#include "../test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the filesystem test suite
 * @return Pointer to the filesystem test suite
 */
struct test_suite* test_fs_get_suite(void);

#ifdef __cplusplus
}
#endif

#endif
