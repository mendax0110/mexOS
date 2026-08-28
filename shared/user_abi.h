#ifndef SHARED_USER_ABI_H
#define SHARED_USER_ABI_H

#include "types.h"

#define USER_NAME_MAX 32

/**
 * @brief Struct to represent user information for userland queries. \struct user_info
 */
struct user_info
{
    uint32_t uid;
    uint32_t is_admin;
    char username[USER_NAME_MAX];
};

#endif // SHARED_USER_ABI_H