#include "test_task.h"
#include "test_runner.h"
#include "../servers/console/vterm.h"
#include "../shared/log.h"

void test_task(void)
{
    run_all_tests();

    struct vterm* vt = vterm_get(VTERM_USER1);
    if (vt)
    {
        vterm_write(vt, "\nPress Alt+F1 to return to shell\n");
    }

    log_info("Kernel self-test completed");
}
