#ifndef MEXOS_TEST_TASK_H
#define MEXOS_TEST_TASK_H

/**
 * @brief Automatic kernel self-test task.
 *
 * This task runs at boot and exercises:
 *  - Memory reporting
 *  - Filesystem operations
 *  - Logging
 *  - Shell commands
 *  - IPC basics
 */
void test_task(void);

#endif //MEXOS_TEST_TASK_H
