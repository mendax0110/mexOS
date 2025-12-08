#include "basic.h"
#include "console.h"
#include "../drivers/input/keyboard.h"
#include "../include/string.h"

static basic_state_t state;

static const char* skip_spaces(const char* str)
{
    while (*str == ' ' || *str == '\t')
    {
        str++;
    }
    return str;
}

static int32_t str_starts_with(const char* str, const char* prefix)
{
    while (*prefix != '\0')
    {
        if (*str != *prefix)
        {
            return 0;
        }
        str++;
        prefix++;
    }
    return 1;
}

static int32_t str_to_int(const char* str)
{
    int32_t result = 0;
    int32_t sign = 1;
    
    str = skip_spaces(str);
    
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    
    while (*str >= '0' && *str <= '9')
    {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

static int32_t evaluate_expression(const char* expr)
{
    expr = skip_spaces(expr);
    
    if (*expr >= 'A' && *expr <= 'Z')
    {
        return state.variables[*expr - 'A'];
    }
    
    if (*expr == '-' || (*expr >= '0' && *expr <= '9'))
    {
        return str_to_int(expr);
    }
    
    return 0;
}

void basic_init(void)
{
    for (uint32_t i = 0; i < BASIC_MAX_VARS; i++)
    {
        state.variables[i] = 0;
    }
    
    state.line_count = 0;
    state.pc = 0;
    state.stack_ptr = 0;
    state.running = 0;
}

static int32_t execute_print(const char* line)
{
    line = skip_spaces(line);
    
    if (*line == '"')
    {
        line++;
        while (*line && *line != '"')
        {
            console_putchar(*line);
            line++;
        }
    }
    else if (*line)
    {
        int32_t value = evaluate_expression(line);
        console_write_dec(value);
    }
    
    console_putchar('\n');
    return 0;
}

static int32_t execute_let(const char* line)
{
    line = skip_spaces(line);
    
    if (*line < 'A' || *line > 'Z')
    {
        return -1;
    }
    
    char var = *line;
    line++;
    line = skip_spaces(line);
    
    if (*line != '=')
    {
        return -1;
    }
    
    line++;
    int32_t value = evaluate_expression(line);
    state.variables[var - 'A'] = value;
    
    return 0;
}

int32_t basic_execute_line(const char* line)
{
    if (!line)
    {
        return -1;
    }
    
    line = skip_spaces(line);
    
    if (*line == '\0')
    {
        return 0;
    }
    
    if (str_starts_with(line, "PRINT"))
    {
        return execute_print(line + 5);
    }
    
    if (str_starts_with(line, "LET"))
    {
        return execute_let(line + 3);
    }
    
    if (*line >= 'A' && *line <= 'Z')
    {
        const char* next = line + 1;
        next = skip_spaces(next);
        if (*next == '=')
        {
            return execute_let(line);
        }
    }
    
    if (str_starts_with(line, "RUN"))
    {
        basic_run_program();
        return 0;
    }
    
    if (str_starts_with(line, "LIST"))
    {
        basic_list_program();
        return 0;
    }
    
    if (str_starts_with(line, "CLEAR"))
    {
        basic_clear_program();
        return 0;
    }
    
    return -1;
}

int32_t basic_add_line(uint32_t line_num, const char* line)
{
    if (state.line_count >= BASIC_MAX_PROGRAM_LINES)
    {
        return -1;
    }
    
    uint32_t insert_idx = state.line_count;
    
    for (uint32_t i = 0; i < state.line_count; i++)
    {
        if (state.line_numbers[i] == line_num)
        {
            strncpy(state.program[i], line, BASIC_MAX_LINE_LEN - 1);
            state.program[i][BASIC_MAX_LINE_LEN - 1] = '\0';
            return 0;
        }
        else if (state.line_numbers[i] > line_num)
        {
            insert_idx = i;
            break;
        }
    }
    
    for (uint32_t i = state.line_count; i > insert_idx; i--)
    {
        state.line_numbers[i] = state.line_numbers[i - 1];
        strncpy(state.program[i], state.program[i - 1], BASIC_MAX_LINE_LEN);
    }
    
    state.line_numbers[insert_idx] = line_num;
    strncpy(state.program[insert_idx], line, BASIC_MAX_LINE_LEN - 1);
    state.program[insert_idx][BASIC_MAX_LINE_LEN - 1] = '\0';
    state.line_count++;
    
    return 0;
}

void basic_run_program(void)
{
    state.running = 1;
    
    for (state.pc = 0; state.pc < state.line_count && state.running; state.pc++)
    {
        if (basic_execute_line(state.program[state.pc]) < 0)
        {
            console_write("Error at line ");
            console_write_dec(state.line_numbers[state.pc]);
            console_write("\n");
            break;
        }
    }
    
    state.running = 0;
}

void basic_list_program(void)
{
    console_write("\n=== Program Listing ===\n");
    
    if (state.line_count == 0)
    {
        console_write("(empty)\n");
        return;
    }
    
    for (uint32_t i = 0; i < state.line_count; i++)
    {
        console_write_dec(state.line_numbers[i]);
        console_write(" ");
        console_write(state.program[i]);
        console_write("\n");
    }
}

void basic_clear_program(void)
{
    state.line_count = 0;
    console_write("Program cleared\n");
}

void basic_interactive_mode(void)
{
    char input_buffer[BASIC_MAX_LINE_LEN];
    uint32_t input_pos = 0;
    
    console_write("\nmexOS BASIC Interpreter\n");
    console_write("Commands: PRINT, LET, RUN, LIST, CLEAR\n");
    console_write("Type 'EXIT' to quit\n\n");
    
    while (1)
    {
        console_write("] ");
        input_pos = 0;
        memset(input_buffer, 0, BASIC_MAX_LINE_LEN);
        
        while (1)
        {
            char c = keyboard_getchar();
            
            if (c == '\n')
            {
                input_buffer[input_pos] = '\0';
                console_putchar('\n');
                break;
            }
            else if (c == '\b')
            {
                if (input_pos > 0)
                {
                    input_pos--;
                    console_putchar('\b');
                    console_putchar(' ');
                    console_putchar('\b');
                }
            }
            else if (c >= 0x20 && c < 0x7F && input_pos < BASIC_MAX_LINE_LEN - 1)
            {
                input_buffer[input_pos++] = c;
                console_putchar(c);
            }
        }
        
        if (str_starts_with(input_buffer, "EXIT"))
        {
            break;
        }
        
        const char* ptr = skip_spaces(input_buffer);
        if (*ptr >= '0' && *ptr <= '9')
        {
            uint32_t line_num = str_to_int(ptr);
            while (*ptr >= '0' && *ptr <= '9')
            {
                ptr++;
            }
            ptr = skip_spaces(ptr);
            
            if (basic_add_line(line_num, ptr) < 0)
            {
                console_write("Error: Program full\n");
            }
        }
        else
        {
            if (basic_execute_line(input_buffer) < 0)
            {
                console_write("Syntax error\n");
            }
        }
    }
}
