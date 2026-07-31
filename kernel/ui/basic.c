#include "basic.h"
#include "console.h"
#include "../../user/lib/syscall.h"
#include "drivers/input/keyboard.h"
#include "lib/string.h"

static basic_state_t state;
static const char* g_expr_ptr;

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

static const char* match_keyword(const char* str, const char* keyword)
{
    uint32_t i = 0;

    while (keyword[i] != '\0')
    {
        if (str[i] != keyword[i])
        {
            return NULL;
        }
        i++;
    }

    const char next = str[i];
    if (next == '\0' ||
        next == ' '  ||
        next == '\t' ||
        next == '\r' ||
        next == '\n')
    {
        return str + i;
    }

    return NULL;
}

static void set_error(const char* msg)
{
    uint32_t i = 0;
    while (msg[i] != '\0' && i < BASIC_MAX_ERROR_LEN - 1)
    {
        state.error_msg[i] = msg[i];
        i++;
    }
    state.error_msg[i] = '\0';
}

const char* basic_get_error(void)
{
    return state.error_msg;
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
        result = (result * 10) + (*str - '0');
        str++;
    }
    
    return result * sign;
}

static int32_t find_line_index(uint32_t line_num)
{
    for (uint32_t i = 0; i < state.line_count; i++)
    {
        if (state.line_numbers[i] == line_num)
        {
            return (int32_t)i;
        }
    }
    return -1;
}

static int32_t parse_expression(void);

static int32_t parse_factor(void)
{
    g_expr_ptr = skip_spaces(g_expr_ptr);

    if (*g_expr_ptr == '(')
    {
        g_expr_ptr++;
        const int32_t value = parse_expression();
        g_expr_ptr = skip_spaces(g_expr_ptr);
        if (*g_expr_ptr == ')')
        {
            g_expr_ptr++;
        }
        return value;
    }

    if (*g_expr_ptr == '-')
    {
        g_expr_ptr++;
        return -parse_factor();
    }

    if (*g_expr_ptr >= 'A' && *g_expr_ptr <= 'Z')
    {
        return state.variables[*g_expr_ptr++ - 'A'];
    }

    int32_t result = 0;
    while (*g_expr_ptr >= '0' && *g_expr_ptr <= '9')
    {
        result = result * 10 + (*g_expr_ptr++ - '0');
    }

    return result;
}

static int32_t parse_term(void)
{
    int32_t value = parse_factor();
    g_expr_ptr = skip_spaces(g_expr_ptr);

    while (*g_expr_ptr == '*' || *g_expr_ptr == '/')
    {
        const char op = *g_expr_ptr++;
        const int32_t rhs = parse_factor();
        value = (op == '*') ? value * rhs : (rhs != 0 ? value / rhs : 0);
        g_expr_ptr = skip_spaces(g_expr_ptr);
    }

    return value;
}

static int32_t parse_expression(void)
{
    int32_t value = parse_term();
    g_expr_ptr = skip_spaces(g_expr_ptr);

    while (*g_expr_ptr == '+' || *g_expr_ptr == '-')
    {
        const char op = *g_expr_ptr++;
        const int32_t rhs = parse_factor();
        value = (op == '+') ? value + rhs : value - rhs;
        g_expr_ptr = skip_spaces(g_expr_ptr);
    }

    return value;
}

static int32_t evaluate_expression(const char* expr)
{
    g_expr_ptr = expr;
    return parse_expression();
}

static int32_t parse_condition(const char* cond)
{
    g_expr_ptr = cond;
    const int32_t lhs = parse_expression();
    g_expr_ptr = skip_spaces(g_expr_ptr);

    const char op1 = g_expr_ptr[0];
    const char op2 = g_expr_ptr[1];
    int32_t result;

    if (op1 == '<' && op2 == '>')
    {
        g_expr_ptr += 2;
        result = (lhs != parse_expression());
    }
    else if (op1 == '<' && op2 == '=')
    {
        g_expr_ptr += 2;
        result = (lhs <= parse_expression());
    }
    else if (op1 == '>' && op2 == '=')
    {
        g_expr_ptr += 2;
        result = (lhs >= parse_expression());
    }
    else if (op1 == '<')
    {
        g_expr_ptr += 1;
        result = (lhs < parse_expression());
    }
    else if (op1 == '>')
    {
        g_expr_ptr += 1;
        result = (lhs > parse_expression());
    }
    else
    {
        result = (lhs != 0);
    }

    return result;
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
    state.jump_pending = 0;
    state.jump_target = 0;
    state.error_msg[0] = '\0';
}

static int32_t execute_print(const char* line)
{
    int32_t suppress_newline = 0;
    line = skip_spaces(line);

    while (1)
    {
        suppress_newline = 0;
        line = skip_spaces(line);

        if (*line == '"')
        {
            line++;
            while (*line && *line != '"')
            {
                console_putchar(*line);
                line++;
            }
            if (*line == '"')
            {
                line++;
            }
        }
        else if (*line && *line != ';' && *line != ',')
        {
            const int32_t value = evaluate_expression(line);
            console_write_dec(value);
            line = g_expr_ptr;
        }
        else if (*line == '\0')
        {
            break;
        }

        line = skip_spaces(line);

        if (*line == ';' || *line == ',')
        {
            suppress_newline = 1;
            line++;
        }
        else
        {
            break;
        }
    }

    if (!suppress_newline)
    {
        console_putchar('\n');
    }

    return 0;
}

static int32_t execute_let(const char* line)
{
    line = skip_spaces(line);
    
    if (*line < 'A' || *line > 'Z')
    {
        set_error("LET statement must start with a variable (A-Z)");
        return -1;
    }
    
    const char var = *line;
    line++;
    line = skip_spaces(line);
    
    if (*line != '=')
    {
        set_error("LET statement must have an assignment operator (=)");
        return -1;
    }
    
    line++;
    const int32_t value = evaluate_expression(line);
    state.variables[var - 'A'] = value;
    
    return 0;
}

static int32_t execute_goto(const char* line)
{
    line = skip_spaces(line);
    const int32_t target_line = evaluate_expression(line);
    const int32_t idx = find_line_index((uint32_t)target_line);

    if (idx < 0)
    {
        set_error("GOTO target line not found");
        return -1;
    }

    state.jump_pending = 1;
    state.jump_target = (uint32_t)idx;
    return 0;
}

static int32_t execute_if(const char* line)
{
    line = skip_spaces(line);
    const int32_t cond_result = parse_condition(line);

    const char* rest = skip_spaces(g_expr_ptr);
    const char* then_body = match_keyword(rest, "THEN");

    if (!then_body)
    {
        set_error("IF statement must have THEN");
        return -1;
    }

    then_body = skip_spaces(then_body);

    if (!cond_result)
    {
        return 0;
    }

    if (*then_body >= '0' && *then_body <= '9')
    {
        const uint32_t target_line = (uint32_t)str_to_int(then_body);
        const int32_t idx = find_line_index(target_line);

        if (idx < 0)
        {
            set_error("IF THEN target line not found");
            return -1;
        }

        state.jump_pending = 1;
        state.jump_target = (uint32_t)idx;
        return 0;
    }

    return basic_execute_line(then_body);
}

static int32_t execute_for(const char* line)
{
    line = skip_spaces(line);

    if (*line < 'A' || *line > 'Z')
    {
        set_error("Expected variable after FOR");
        return -1;
    }

    const char var = *line;
    line++;
    line = skip_spaces(line);

    if (*line != '=')
    {
        set_error("Expected '=' in FOR");
        return -1;
    }

    line++;

    const int32_t start_value = evaluate_expression(line);
    line = skip_spaces(g_expr_ptr);

    const char* after_to = match_keyword(line, "TO");
    if (!after_to)
    {
        set_error("Expected TO in FOR");
        return -1;
    }

    const int32_t limit_value = evaluate_expression(after_to);
    line = skip_spaces(g_expr_ptr);

    int32_t step_value = 1;
    const char* after_step = match_keyword(line, "STEP");
    if (after_step)
    {
        step_value = evaluate_expression(after_step);
    }

    if (state.for_stack_ptr >= BASIC_STACK_SIZE)
    {
        set_error("FOR stack overflow");
        return -1;
    }

    state.variables[var - 'A'] = start_value;

    basic_for_loop_t* loop = &state.for_stack[state.for_stack_ptr++];
    loop->var = var;
    loop->limit = limit_value;
    loop->step = step_value;
    loop->line_index = state.pc + 1;

    return 0;
}

static int32_t execute_next(const char* line)
{
    line = skip_spaces(line);

    if (state.for_stack_ptr == 0)
    {
        set_error("NEXT without FOR");
        return -1;
    }

    const basic_for_loop_t* loop = &state.for_stack[state.for_stack_ptr - 1];

    if (*line >= 'A' && *line <= 'Z' && *line != loop->var)
    {
        set_error("NEXT variable does not match FOR variable");
        return -1;
    }

    state.variables[loop->var - 'A'] += loop->step;

    const int32_t current = state.variables[loop->var - 'A'];
    const int32_t keep_looping = (loop->step >= 0) ? (current <= loop->limit) : (current >= loop->limit);

    if (keep_looping)
    {
        state.jump_pending = 1;
        state.jump_target = loop->line_index;
    }
    else
    {
        state.for_stack_ptr--;
    }

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

    const char* rest;
    
    if ((rest = match_keyword(line, "REM")) != NULL)
    {
        UNUSED(rest);
        return 0;
    }

    if ((rest = match_keyword(line, "PRINT")) != NULL)
    {
        return execute_print(rest);
    }
    
    if ((rest = match_keyword(line, "LET")) != NULL)
    {
        return execute_let(rest);
    }

    if ((rest = match_keyword(line, "GOTO")) != NULL)
    {
        return execute_goto(rest);
    }

    if ((rest = match_keyword(line, "IF")) != NULL)
    {
        return execute_if(rest);
    }

    if ((rest = match_keyword(line, "FOR")) != NULL)
    {
        return execute_for(rest);
    }

    if ((rest = match_keyword(line, "NEXT")) != NULL)
    {
        return execute_next(rest);
    }
    if (match_keyword(line, "END") != NULL)
    {
        state.running = 0;
        return 0;
    }

    if (match_keyword(line, "RUN") != NULL)
    {
        if (state.running)
        {
            set_error("Cannot RUN from within a running program");
            return -1;
        }

        basic_run_program();
        return 0;
    }
    
    if (match_keyword(line, "LIST") != NULL)
    {
        basic_list_program();
        return 0;
    }
    
    if (match_keyword(line, "CLEAR") != NULL)
    {
        basic_clear_program();
        return 0;
    }

    if (*line >= 'A' && *line <= 'Z')
    {
        const char* next = skip_spaces(line + 1);
        if (*next == '=')
        {
            return execute_let(line);
        }
    }

    set_error("Unknown command or syntax error");
    return -1;
}

int32_t basic_add_line(const uint32_t line_num, const char* line)
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

        if (state.line_numbers[i] > line_num)
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
    state.jump_pending = 0;
    state.jump_target = 0;
    
    for (state.pc = 0; state.pc < state.line_count && state.running; state.pc++)
    {
        if (basic_execute_line(state.program[state.pc]) < 0)
        {
            console_write("Error at line ");
            console_write_dec(state.line_numbers[state.pc]);
            console_write(": ");
            console_write(basic_get_error());
            console_write("\n");
            break;
        }

        if (state.jump_pending)
        {
            state.jump_pending = 0;
            state.pc = state.jump_target - 1;
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
    console_write("Commands: PRINT, LET, IF/THEN, FOR/NEXT, GOTO, END, REM, RUN, LIST, CLEAR\n");
    console_write("Type 'EXIT' to quit\n\n");
    
    while (1)
    {
        console_write("] ");
        input_pos = 0;
        memset(input_buffer, 0, BASIC_MAX_LINE_LEN);
        
        while (1)
        {
            const char c = keyboard_getchar();
            
            if (c == '\n')
            {
                input_buffer[input_pos] = '\0';
                console_putchar('\n');
                break;
            }

            if (c == '\b')
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
            const uint32_t line_num = str_to_int(ptr);
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
                console_write("Syntax error: ");
                console_write(basic_get_error());
                console_write("\n");
            }
        }
    }
}
