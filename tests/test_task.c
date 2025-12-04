#include "test_task.h"
#include "../core/console.h"
#include "../core/shell.h"
#include "../core/fs.h"
#include "../core/log.h"
#include "../core/sysmon.h"
#include "../core/debug_utils.h"
#include "../core/basic.h"
#include "../include/string.h"

static void execute_test_command(const char* cmd)
{
    console_write("> ");
    console_write(cmd);
    console_putchar('\n');

    char buffer[256];
    strncpy(buffer, cmd, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    execute_command(buffer);
}

void test_task(void)
{
    console_write("[test] Kernel self-test starting...\n");

    console_write("[test] Memory test:\n");
    execute_test_command("mem");

    console_write("[test] Filesystem test:\n");
    fs_init();
    fs_clear_cache();
    execute_test_command("mkdir testdir");
    execute_test_command("touch testdir/file1.txt");
    execute_test_command("write testdir/file1.txt Hello_world");
    execute_test_command("cat testdir/file1.txt");
    execute_test_command("ls testdir");
    execute_test_command("rm testdir/file1.txt");
    execute_test_command("ls testdir");
    execute_test_command("rmdir testdir");
    execute_test_command("ls");

    console_write("[test] Logging test:\n");
    log_info("Test log entry");
    execute_test_command("log");

    execute_test_command("echo Testing echo command");
    execute_test_command("ver");
    execute_test_command("uptime");
    execute_test_command("ps");
    execute_test_command("clear");

    execute_test_command("sysmon");
    execute_test_command("trace");
    execute_test_command("clrtrace");
    execute_test_command("memdump 0x1000 64");
    //execute_test_command("basic");

    console_write("[test] Kernel self-test completed!\n");
}
