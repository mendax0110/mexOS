#include "initrd.h"
#include "fs/fs.h"
#include "lib/log.h"
#include "lib/string.h"
#include "ui/console.h"

#define INITRD_PATH_ENTRY(name, path) path,
    static const char* initrd_paths[] =
    {
        INITRD_PROGRAMS(INITRD_PATH_ENTRY)
    };
#undef INITRD_PATH_ENTRY

#define INITRD_START_ENTRY(name, path) INITRD_SYMBOL_START(name),
    static const uint8_t* initrd_starts[] =
    {
        INITRD_PROGRAMS(INITRD_START_ENTRY)
    };
#undef INITRD_START_ENTRY

#define INITRD_END_ENTRY(name, path) INITRD_SYMBOL_END(name),
    static const uint8_t* initrd_ends[] =
    {
        INITRD_PROGRAMS(INITRD_END_ENTRY)
    };
#undef INITRD_END_ENTRY

size_t initrd_file_count(void)
{
    return sizeof(initrd_paths) / sizeof(initrd_paths[0]);
}

int initrd_get_file(const size_t index, struct initrd_file* out)
{
    if (!out || index >= initrd_file_count())
    {
        return -1;
    }

    out->path = initrd_paths[index];
    out->data = initrd_starts[index];
    out->size = (size_t)(initrd_ends[index] - initrd_starts[index]);
    return 0;
}

int initrd_install(void)
{
    if (!fs_exists("/bin"))
    {
        const int dir_ret = fs_create_dir("/bin");
        if (dir_ret != FS_ERR_OK && dir_ret != FS_ERR_EXISTS)
        {
            log_warn("initrd_install: failed to create /bin");
            return -1;
        }
    }

    for (size_t i = 0; i < initrd_file_count(); i++)
    {
        struct initrd_file file;
        if (initrd_get_file(i, &file) != 0 || !file.path || !file.data || file.size == 0)
        {
            char num_buf[16];
            console_write("[initrd] invalid embedded file at index ");
            console_write(itoa((int)i, num_buf, 10));
            console_write("\n");
            return -1;
        }

        if (!fs_exists(file.path))
        {
            const int create_ret = fs_create_file(file.path);
            if (create_ret != FS_ERR_OK && create_ret != FS_ERR_EXISTS)
            {
                char num_buf[16];
                console_write("[initrd] failed to create ");
                console_write(file.path);
                console_write(" ret ");
                console_write(itoa(create_ret, num_buf, 10));
                console_write("\n");
                log_warn_fmt("initrd_install: failed to create %s", file.path);
                return -1;
            }
        }

        const int write_ret = fs_write(file.path, (const char*)file.data, file.size);
        if (write_ret != (int)file.size)
        {
            char num_buf[16];
            console_write("[initrd] failed to install ");
            console_write(file.path);
            console_write(" wrote ");
            console_write(itoa(write_ret, num_buf, 10));
            console_write(" of ");
            console_write(itoa((int)file.size, num_buf, 10));
            console_write(" bytes\n");
            log_warn_fmt("initrd_install: failed to install %s", file.path);
            return -1;
        }
    }

    log_info("initrd_install: installed embedded user programs");
    return 0;
}
