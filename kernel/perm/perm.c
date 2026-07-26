#include "perm/perm.h"
#include "lib/string.h"
#include "sched/sched.h"

static kernel_user_id* current_user = NULL;
static kernel_group_id* current_group = NULL;
static kernel_user_map* active_map = NULL;

void set_user_id(kernel_user_id* user)
{
    struct task* task = sched_get_current();
    if (task)
    {
        task->uid = user ? user->uid : LIMIT_UNSIGNED;
        return;
    }
    if (current_user == user)
    {
        return;
    }
    current_user = user;
}

kernel_user_id* get_current_user(void)
{
    const struct task* task = sched_get_current();
    if (task && active_map)
    {
        return user_map_find_user_by_uid(active_map, task->uid);
    }
    return current_user;
}

void set_group_id(kernel_group_id* group)
{
    struct task* task = sched_get_current();
    if (task)
    {
        task->gid = group ? group->gid : LIMIT_UNSIGNED;
        return;
    }
    if (current_group == group)
    {
        return;
    }
    current_group = group;
}

kernel_group_id* get_current_group(void)
{
    const struct task* task = sched_get_current();
    if (task && active_map)
    {
        return user_map_find_group_by_gid(active_map, task->gid);
    }
    return current_group;
}

void create_user_map(kernel_user_map* user_map)
{
    if (user_map == NULL)
    {
        return;
    }

    for (uint32_t i = 0; i < KERNEL_MAX_USERS; i++)
    {
        user_map->users[i] = NULL;
        user_map->groups[i] = NULL;
        user_map->creds[i] = NULL;
    }

    user_map->user_count = 0;
    user_map->group_count = 0;
    user_map->creds_count = 0;
}

int user_map_add_user(kernel_user_map* user_map, kernel_user_id* user)
{
    if (!user_map || !user) return -1;
    if (user_map->user_count >= KERNEL_MAX_USERS) return -1;

    user_map->users[user_map->user_count++] = user;
    return 0;
}

int user_map_add_group(kernel_user_map* user_map, kernel_group_id* group)
{
    if (!user_map || !group) return -1;
    if (user_map->group_count >= KERNEL_MAX_USERS) return -1;

    user_map->groups[user_map->group_count++] = group;
    return 0;
}

int user_map_add_cred(kernel_user_map* user_map, kernel_cred_t* cred)
{
    if (!user_map || !cred) return -1;
    if (user_map->creds_count >= KERNEL_MAX_USERS) return -1;

    user_map->creds[user_map->creds_count++] = cred;
    return 0;
}

int user_map_remove_user(kernel_user_map* user_map, const uint32_t uid)
{
    if (!user_map) return -1;

    for (uint32_t i = 0; i < user_map->user_count; i++)
    {
        if (user_map->users[i] && user_map->users[i]->uid == uid)
        {
            if (current_user == user_map->users[i])
            {
                current_user = NULL;
            }

            const uint32_t last = user_map->user_count - 1;
            user_map->users[i] = user_map->users[last];
            user_map->users[last] = NULL;
            user_map->user_count--;
            return 0;
        }
    }

    return -1;
}

int user_map_remove_group(kernel_user_map* user_map, const uint32_t gid)
{
    if (!user_map) return -1;

    for (uint32_t i = 0; i < user_map->group_count; i++)
    {
        if (user_map->groups[i] && user_map->groups[i]->gid == gid)
        {
            if (current_group == user_map->groups[i])
            {
                current_group = NULL;
            }

            const uint32_t last = user_map->group_count - 1;
            user_map->groups[i] = user_map->groups[last];
            user_map->groups[last] = NULL;
            user_map->group_count--;
            return 0;
        }
    }

    return -1;
}

int user_map_remove_cred(kernel_user_map* user_map, const uint32_t uid, const uint32_t gid)
{
    if (!user_map) return -1;

    for (uint32_t i = 0; i < user_map->creds_count; i++)
    {
        const kernel_cred_t* cred = user_map->creds[i];

        if (cred &&
            cred->user &&
            cred->group &&
            cred->user->uid == uid &&
            cred->group->gid == gid
        )
        {
            const uint32_t last = user_map->creds_count - 1;
            user_map->creds[i] = user_map->creds[last];
            user_map->creds[last] = NULL;
            user_map->creds_count--;
            return 0;
        }
    }

    return -1;
}

kernel_user_id* user_map_find_user_by_uid(kernel_user_map* user_map, const uint32_t uid)
{
    if (!user_map) return NULL;

    for (uint32_t i = 0; i < user_map->user_count; i++)
    {
        if (user_map->users[i] && user_map->users[i]->uid == uid)
        {
            return user_map->users[i];
        }
    }

    return NULL;
}

kernel_group_id* user_map_find_group_by_gid(kernel_user_map* user_map, const uint32_t gid)
{
    if (!user_map) return NULL;

    for (uint32_t i = 0; i < user_map->group_count; i++)
    {
        if (user_map->groups[i] && user_map->groups[i]->gid == gid)
        {
            return user_map->groups[i];
        }
    }

    return NULL;
}

kernel_cred_t* user_map_find_cred(kernel_user_map* user_map, const uint32_t uid, const uint32_t gid)
{
    if (!user_map) return NULL;

    for (uint32_t i = 0; i < user_map->creds_count; i++)
    {
        kernel_cred_t* cred = user_map->creds[i];

        if (cred &&
            cred->user &&
            cred->group &&
            cred->user->uid == uid &&
            cred->group->gid == gid
        )
        {
            return cred;
        }
    }

    return NULL;
}

void perm_grant(kernel_user_id* user, const uint32_t perm_mask)
{
    if (!user) return;
    BIT_SET(user->type, perm_mask);
}

void perm_revoke(kernel_user_id* user, const uint32_t perm_mask)
{
    if (!user) return;
    BIT_CLEAR(user->type, perm_mask);
}

int perm_check(const kernel_user_id* user, const uint32_t perm_mask)
{
    if (!user) return 0;
    return BIT_MASK(user->type, perm_mask) != 0 ? 1 : 0;
}

int current_user_has_perm(const uint32_t perm_mask)
{
    return perm_check(get_current_user(), perm_mask);
}

void perm_set_active_map(kernel_user_map* user_map)
{
    if (active_map == user_map)
    {
        return;
    }
    active_map = user_map;
}

kernel_user_map* perm_get_active_map(void)
{
    return active_map;
}

kernel_user_id* user_map_find_user_by_name(kernel_user_map* user_map, const char* username)
{
    if (!user_map || !username) return NULL;

    for (uint32_t i = 0; i < user_map->user_count; i++)
    {
        if (user_map->users[i] && strcmp(user_map->users[i]->username, username) == 0)
        {
            return user_map->users[i];
        }
    }

    return NULL;
}

kernel_group_id* user_map_find_group_by_name(kernel_user_map* user_map, const char* groupname)
{
    if (!user_map || !groupname) return NULL;

    for (uint32_t i = 0; i < user_map->group_count; i++)
    {
        if (user_map->groups[i] && strcmp(user_map->groups[i]->name, groupname) == 0)
        {
            return user_map->groups[i];
        }
    }

    return NULL;
}
