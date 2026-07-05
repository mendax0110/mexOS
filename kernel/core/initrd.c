#include "initrd.h"
#include "fs/fs.h"
#include "lib/log.h"

static const char* initrd_paths[] =
{
    INITRD_INIT_PATH,
    "/bin/echo",
    "/bin/cat",
    "/bin/ls",
    "/bin/sh"
};

static const uint8_t* initrd_starts[] =
{
    _binary_init_elf_start,
    _binary_echo_elf_start,
    _binary_cat_elf_start,
    _binary_ls_elf_start,
    _binary_sh_elf_start
};

static const uint8_t* initrd_ends[] =
{
    _binary_init_elf_end,
    _binary_echo_elf_end,
    _binary_cat_elf_end,
    _binary_ls_elf_end,
    _binary_sh_elf_end
};

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
            return -1;
        }

        if (!fs_exists(file.path))
        {
            const int create_ret = fs_create_file(file.path);
            if (create_ret != FS_ERR_OK && create_ret != FS_ERR_EXISTS)
            {
                log_warn_fmt("initrd_install: failed to create %s", file.path);
                return -1;
            }
        }

        const int write_ret = fs_write(file.path, (const char*)file.data, (uint32_t)file.size);
        if (write_ret != (int)file.size)
        {
            log_warn_fmt("initrd_install: failed to install %s", file.path);
            return -1;
        }
    }

    log_info("initrd_install: installed embedded user programs");
    return 0;
}
