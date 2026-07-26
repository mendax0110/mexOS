#include "power.h"
#include "arch/i686/arch.h"
#include "drivers/bus/acpi.h"
#include "fs/fs.h"

NORETURN void kernel_power_shutdown(void)
{
    fs_sync();
    if (!acpi_shutdown())
    {
        outw(0x604, 0x2000);
        outw(0xB004, 0x2000);
        outw(0x4004, 0x3400);
    }
    cli();
    while (1)
    {
        hlt();
    }
}

NORETURN void kernel_power_reboot(void)
{
    fs_sync();
    cli();
    uint8_t status = 0x02;
    for (uint32_t timeout = 0; timeout < 100000 && (status & 0x02); timeout++)
    {
        status = inb(0x64);
    }
    outb(0x64, 0xFE);
    while (1)
    {
        hlt();
    }
}
