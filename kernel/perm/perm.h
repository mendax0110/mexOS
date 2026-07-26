#ifndef KERNEL_PERM_H
#define KERNEL_PERM_H

#include "../../shared/types.h"
#include "include/config.h"
#include "include/bitops.h"

#define KERNEL_MAX_USERS 16

#define KERNEL_PERM_READ BIT(0)
#define KERNEL_PERM_WRITE BIT(1)
#define KERNEL_PERM_EXEC BIT(2)
#define KERNEL_PERM_ADMIN BIT(3)


/**
 * @brief Struct to represent the user id \struct kernel_user_id
 */
typedef struct kernel_user_id
{
    const char* username;
    const char* password;
    uint32_t uid;
    uint32_t type;
} kernel_user_id;

/**
 * @brief Struct to represent the group id \struct kernel_group_id
 */
typedef struct kernel_group_id
{
    const char* name;
    uint32_t gid;
} kernel_group_id;

/**
 * @brief Struct to represent the credentials \struct kernel_cred_t
 */
typedef struct kernel_cred_t
{
    kernel_user_id* user;
    kernel_group_id* group;
} kernel_cred_t;

/**
 * @brief Struct to represent the user map \struct kernel_user_map
 */
typedef struct
{
    // list of all users
    kernel_user_id* users[KERNEL_MAX_USERS];
    kernel_group_id* groups[KERNEL_MAX_USERS];
    kernel_cred_t* creds[KERNEL_MAX_USERS];
    uint32_t user_count;
    uint32_t group_count;
    uint32_t creds_count;
} kernel_user_map;

/**
 * @brief Sets the user id
 * @param user The user id struct
 */
void set_user_id(kernel_user_id* user);

/**
 * @brief Gets the current user
 * @return user id ptr
 */
kernel_user_id* get_current_user(void);

/**
 * @brief Sets the group id
 * @param group The group id struct
 */
void set_group_id(kernel_group_id* group);

/**
 * @brief Gets the current group
 * @return group id ptr
 */
kernel_group_id* get_current_group(void);

/**
 * @brief Creates a new user map
 * @param user_map The user map struct
 */
void create_user_map(kernel_user_map* user_map);

/**
 * @brief Adds user to the user map
 * @param user_map The user map
 * @param user The user to add
 * @return 0 on success, -1 on failure
 */
int user_map_add_user(kernel_user_map* user_map, kernel_user_id* user);

/**
 * @brief Adds user group to the user map
 * @param user_map The user map
 * @param group  The group to add
 * @return 0 on success, -1 on failure
 */
int user_map_add_group(kernel_user_map* user_map, kernel_group_id* group);

/**
 * @brief Adds credentials to the user map
 * @param user_map The user map
 * @param cred The credentials to add
 * @return 0 on success, -1 on failure
 */
int user_map_add_cred(kernel_user_map* user_map, kernel_cred_t* cred);

/**
 * @brief Removes user from the user map
 * @param user_map The user map
 * @param uid The user id to remove
 * @return 0 on success, -1 on failure
 */
int user_map_remove_user(kernel_user_map* user_map, uint32_t uid);

/**
 * @brief Removes the group from the user map
 * @param user_map The user map
 * @param gid The group id to remove
 * @return 0 on success, -1 on failure
 */
int user_map_remove_group(kernel_user_map* user_map, uint32_t gid);

/**
 * @brief Removes the credentials from the user map
 * @param user_map The user map
 * @param uid The user id
 * @param gid The group id
 * @return 0 on success, -1 on failure
 */
int user_map_remove_creds(kernel_user_map* user_map, uint32_t uid, uint32_t gid);

/**
 * @brief Finds user by the given uid
 * @param user_map The user map to lookup
 * @param uid The user it to check
 * @return Ptr kernel user id
 */
kernel_user_id* user_map_find_user_by_uid(kernel_user_map* user_map, uint32_t uid);

/**
 * @brief Finds group by the given gid
 * @param user_map The user map to lookup
 * @param gid The group id to check
 * @return Ptr kernel group id
 */
kernel_group_id* user_map_find_group_by_gid(kernel_user_map* user_map, uint32_t gid);

/**
 * @brief Finds credentials by the given uid and gid
 * @param user_map The user map to lookup
 * @param uid The user id to check
 * @param gid The group id to check
 * @return Ptr kernel credentials
 */
kernel_cred_t* user_map_find_cred(kernel_user_map* user_map, uint32_t uid, uint32_t gid);

/**
 * @brief Grants permissions
 * @param user The user to grant permissions
 * @param perm_mask The permissions mask
 */
void perm_grant(kernel_user_id* user, uint32_t perm_mask);

/**
 * @brief Revokes permissions
 * @param user The user to revoke permissions
 * @param perm_mask The permissions mask
 */
void perm_revoke(kernel_user_id* user, uint32_t perm_mask);

/**
 * @brief Checks if the user has the given permissions
 * @param user The user to check
 * @param perm_mask The permissions mask
 * @return 0 if the user has the permissions, -1 if not
 */
int perm_check(const kernel_user_id* user, uint32_t perm_mask);

/**
 * @brief Checks if the current user has the given permissions
 * @param perm_mask The permissions mask
 * @return 0 if the current user has the permissions, -1 if not
 */
int current_user_has_perm(uint32_t perm_mask);

/**
 * @brief Setter for the active map
 * @param user_map The user map to set as active
 */
void perm_set_active_map(kernel_user_map* user_map);

/**
 * @brief Getter for the active map
 * @return The active map
 */
kernel_user_map* perm_get_active_map(void);

/**
 * @brief Finds user by the username in the map
 * @param user_map The user map
 * @param username the username
 * @return The ptr user id struct
 */
kernel_user_id* user_map_find_user_by_name(kernel_user_map* user_map, const char* username);

/**
 * @brief Finds group by the groupname in the map
 * @param user_map The user map
 * @param groupname the groupname
 * @return The ptr group id struct
 */
kernel_group_id* user_map_find_group_by_name(kernel_user_map* user_map, const char* groupname);

#endif // KERNEL_PERM_H