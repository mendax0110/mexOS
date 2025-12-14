#ifndef KERNEL_BASIC_H
#define KERNEL_BASIC_H

#include "include/types.h"

#define BASIC_MAX_VARS 26
#define BASIC_MAX_LINE_LEN 128
#define BASIC_MAX_PROGRAM_LINES 100
#define BASIC_STACK_SIZE 32

/**
 * @brief BASIC interpreter state structure
 */
typedef struct basic_state
{
    int32_t variables[BASIC_MAX_VARS];
    char program[BASIC_MAX_PROGRAM_LINES][BASIC_MAX_LINE_LEN];
    uint32_t line_numbers[BASIC_MAX_PROGRAM_LINES];
    uint32_t line_count;
    uint32_t pc;
    int32_t stack[BASIC_STACK_SIZE];
    uint32_t stack_ptr;
    uint8_t running;
} basic_state_t;

/**
 * @brief Initialize BASIC interpreter
 */
void basic_init(void);

/**
 * @brief Execute a single BASIC line
 * @param line Line to execute
 * @return 0 on success, negative on error
 */
int32_t basic_execute_line(const char* line);

/**
 * @brief Run stored BASIC program
 */
void basic_run_program(void);

/**
 * @brief List stored program
 */
void basic_list_program(void);

/**
 * @brief Clear program memory
 */
void basic_clear_program(void);

/**
 * @brief Add line to program
 * @param line_num Line number
 * @param line Line content
 * @return 0 on success, negative on error
 */
int32_t basic_add_line(uint32_t line_num, const char* line);

/**
 * @brief Enter interactive BASIC mode
 */
void basic_interactive_mode(void);

#endif
