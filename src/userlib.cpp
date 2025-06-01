#include "userlib.h"
#include "mexKernel.h"

void print(const char* str)
{
    syscall(SYS_WRITE, (uint32_t)str);
}