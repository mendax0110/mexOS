#include "rtc.h"
#include "../../lib/log.h"
#include "string.h"
#include "../../arch/i686/arch.h"
#include "../../arch/i686/idt.h"


#define RTC_NMI_DISABLE  0x80
#define IO_DELAY_PORT 0x80
#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

static volatile uint32_t rtc_ticks = 0;

static inline void rtc_io_delay(void)
{
#if defined(io_wait)
    io_wait();
#else
    outb(IO_DELAY_PORT, 0);
#endif
}

static inline void rtc_select_register_nmi(const uint8_t reg)
{
    outb(RTC_PORT_INDEX, (uint8_t)(reg | RTC_NMI_DISABLE));
    rtc_io_delay();
}

static uint8_t rtc_read_register(const uint8_t reg)
{
    rtc_select_register_nmi(reg);
    return inb(RTC_PORT_DATA);
}

static void rtc_write_register(const uint8_t reg, const uint8_t value)
{
    rtc_select_register_nmi(reg);
    outb(RTC_PORT_DATA, value);
    rtc_io_delay();
}

static uint8_t bcd_to_binary(const uint8_t bcd)
{
    return (uint8_t)(((bcd >> 4) * 10) + (bcd & 0x0F));
}

static uint8_t binary_to_bcd(const uint8_t binary)
{
    return (uint8_t)(((binary / 10) << 4) | (binary % 10));
}

bool rtc_is_updating(void)
{
    return (rtc_read_register(RTC_REG_STATUS_A) & 0x80) != 0;
}

static bool rtc_wait_uip_clear(const unsigned int max_loops)
{
    unsigned int i = 0;
    while (i < max_loops)
    {
        if (!rtc_is_updating())
            return true;
        rtc_io_delay();
        i++;
    }
    return false;
}

void rtc_interrupt_handler(struct registers* regs)
{
    (void)regs;
    rtc_read_register(RTC_REG_STATUS_C);
    rtc_ticks++;
}

static void rtc_unmask_irq(void)
{
    uint8_t mask2 = inb(PIC2_DATA);
    mask2 &= ~(1 << 0);
    outb(PIC2_DATA, mask2);

    uint8_t mask1 = inb(PIC1_DATA);
    mask1 &= ~(1 << 2);
    outb(PIC1_DATA, mask1);
}

void rtc_read_time(struct rtc_time* time)
{
    if (!time) return;

    struct rtc_time a, b;
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        if (!rtc_wait_uip_clear(20000))
        {
            char warn[96], tmp[8];
            strcpy(warn, "RTC: UIP stuck while reading; continuing (attempt ");
            int_to_str_pad(attempt + 1, tmp, 1);
            strcat(warn, tmp);
            strcat(warn, ")");
            log_warn(warn);
        }

        const uint8_t status_b = rtc_read_register(RTC_REG_STATUS_B);
        const bool binary_mode = (status_b & RTC_STATUS_B_BINARY) != 0;

        a.second = rtc_read_register(RTC_REG_SECONDS);
        a.minute = rtc_read_register(RTC_REG_MINUTES);
        a.hour = rtc_read_register(RTC_REG_HOURS);
        a.day = rtc_read_register(RTC_REG_DAY);
        a.month = rtc_read_register(RTC_REG_MONTH);
        a.year  = rtc_read_register(RTC_REG_YEAR);
        a.weekday = rtc_read_register(RTC_REG_WEEKDAY);
        uint8_t century = rtc_read_register(RTC_REG_CENTURY);

        b.second = rtc_read_register(RTC_REG_SECONDS);
        b.minute = rtc_read_register(RTC_REG_MINUTES);
        b.hour = rtc_read_register(RTC_REG_HOURS);
        b.day = rtc_read_register(RTC_REG_DAY);
        b.month = rtc_read_register(RTC_REG_MONTH);
        b.year = rtc_read_register(RTC_REG_YEAR);
        b.weekday = rtc_read_register(RTC_REG_WEEKDAY);

        if (!binary_mode)
        {
            a.second = bcd_to_binary(a.second);
            a.minute = bcd_to_binary(a.minute);
            a.hour = bcd_to_binary(a.hour);
            a.day = bcd_to_binary(a.day);
            a.month = bcd_to_binary(a.month);
            a.year = bcd_to_binary(a.year);
            a.weekday = bcd_to_binary(a.weekday);

            b.second= bcd_to_binary(b.second);
            b.minute = bcd_to_binary(b.minute);
            b.hour = bcd_to_binary(b.hour);
            b.day = bcd_to_binary(b.day);
            b.month = bcd_to_binary(b.month);
            b.year = bcd_to_binary(b.year);
            b.weekday = bcd_to_binary(b.weekday);

            if (century != 0xFF) century = bcd_to_binary(century);
        }

        const uint32_t year_a = (uint32_t)a.year + ((century != 0 && century != 0xFF) ? (century * 100) : 2000u);

        if (a.second == b.second &&
            a.minute == b.minute &&
            a.hour == b.hour &&
            a.day == b.day &&
            a.month == b.month &&
            a.year == b.year)
        {
            time->second = a.second;
            time->minute = a.minute;
            time->hour = a.hour;
            time->day = a.day;
            time->month = a.month;
            time->year = (uint16_t)year_a;
            time->weekday = a.weekday;
            return;
        }

    }

    {
        const uint8_t status_b = rtc_read_register(RTC_REG_STATUS_B);
        const bool binary_mode = (status_b & RTC_STATUS_B_BINARY) != 0;

        time->second = rtc_read_register(RTC_REG_SECONDS);
        time->minute = rtc_read_register(RTC_REG_MINUTES);
        time->hour = rtc_read_register(RTC_REG_HOURS);
        time->day = rtc_read_register(RTC_REG_DAY);
        time->month = rtc_read_register(RTC_REG_MONTH);
        time->year = rtc_read_register(RTC_REG_YEAR);
        time->weekday = rtc_read_register(RTC_REG_WEEKDAY);
        uint8_t century = rtc_read_register(RTC_REG_CENTURY);

        if (!binary_mode)
        {
            time->second = bcd_to_binary(time->second);
            time->minute = bcd_to_binary(time->minute);
            time->hour = bcd_to_binary(time->hour);
            time->day = bcd_to_binary(time->day);
            time->month = bcd_to_binary(time->month);
            time->year = bcd_to_binary(time->year);
            if (century != 0xFF) century = bcd_to_binary(century);
        }

        time->year += (century != 0 && century != 0xFF) ? (century * 100) : 2000u;
    }
}

void rtc_write_time(struct rtc_time* time)
{
    if (!time) return;

    const uint8_t status_b = rtc_read_register(RTC_REG_STATUS_B);
    const bool binary_mode = (status_b & RTC_STATUS_B_BINARY) != 0;

    uint8_t second = time->second;
    uint8_t minute = time->minute;
    uint8_t hour = time->hour;
    uint8_t day = time->day;
    uint8_t month = time->month;
    uint8_t year = (uint8_t)(time->year % 100);
    uint8_t century= (uint8_t)(time->year / 100);

    if (!binary_mode)
    {
        second = binary_to_bcd(second);
        minute = binary_to_bcd(minute);
        hour = binary_to_bcd(hour);
        day = binary_to_bcd(day);
        month = binary_to_bcd(month);
        year = binary_to_bcd(year);
        century = binary_to_bcd(century);
    }

    const uint8_t prev_b = rtc_read_register(RTC_REG_STATUS_B);
    rtc_write_register(RTC_REG_STATUS_B, (uint8_t)(prev_b | 0x80));
    rtc_io_delay();

    rtc_write_register(RTC_REG_SECONDS, second);
    rtc_write_register(RTC_REG_MINUTES, minute);
    rtc_write_register(RTC_REG_HOURS, hour);
    rtc_write_register(RTC_REG_DAY, day);
    rtc_write_register(RTC_REG_MONTH, month);
    rtc_write_register(RTC_REG_YEAR, year);
    rtc_write_register(RTC_REG_CENTURY, century);

    rtc_write_register(RTC_REG_STATUS_B, (uint8_t)(prev_b & ~0x80));

    char year_str[6], m[3], d[3], hh[3], mm[3], ss[3];
    int_to_str_pad(time->year, year_str, 4);
    int_to_str_pad(time->month, m, 2);
    int_to_str_pad(time->day, d, 2);
    int_to_str_pad(time->hour, hh, 2);
    int_to_str_pad(time->minute, mm, 2);
    int_to_str_pad(time->second, ss, 2);

    char msg[128];
    strcpy(msg, "RTC: Time set to: ");
    strcat(msg, year_str); strcat(msg, "-");
    strcat(msg, m); strcat(msg, "-");
    strcat(msg, d); strcat(msg, " ");
    strcat(msg, hh); strcat(msg, ":");
    strcat(msg, mm); strcat(msg, ":");
    strcat(msg, ss);
    log_info(msg);
}

void rtc_enable_periodic_interrupt(const uint8_t rate)
{
    if (rate < 3 || rate > 15)
    {
        char err[64], tmp[8];
        strcpy(err, "RTC: Invalid periodic interrupt rate: ");
        int_to_str_pad(rate, tmp, 2);
        strcat(err, tmp);
        log_error(err);
        return;
    }

    rtc_unmask_irq();

    const uint8_t prev_a = rtc_read_register(RTC_REG_STATUS_A);
    rtc_write_register(RTC_REG_STATUS_A, (uint8_t)((prev_a & 0xF0) | (rate & 0x0F)));

    const uint8_t prev_b = rtc_read_register(RTC_REG_STATUS_B);
    rtc_write_register(RTC_REG_STATUS_B, (uint8_t)(prev_b | RTC_STATUS_B_PIE));

    const uint32_t freq = 32768u >> (rate - 1);
    char msg[128], tmp[16];
    strcpy(msg, "RTC: Enabled periodic interrupt at ");
    int_to_str_pad(freq, tmp, 5);
    strcat(msg, tmp);
    strcat(msg, " Hz");
    log_info(msg);
}

void rtc_disable_periodic_interrupt(void)
{
    const uint8_t prev_b = rtc_read_register(RTC_REG_STATUS_B);
    rtc_write_register(RTC_REG_STATUS_B, (uint8_t)(prev_b & ~RTC_STATUS_B_PIE));
    log_info("RTC: Disabled periodic interrupt");
}

void rtc_init(void)
{
    char tmp[64];

    strcpy(tmp, "RTC: Initializing Real-Time Clock");
    log_info(tmp);

    const uint8_t status_b = rtc_read_register(RTC_REG_STATUS_B);
    rtc_write_register(RTC_REG_STATUS_B, (uint8_t)(status_b | RTC_STATUS_B_24HOUR));

    rtc_unmask_irq();
    register_interrupt_handler(40, rtc_interrupt_handler);

    struct rtc_time t;
    rtc_read_time(&t);

    char year[6], month[3], day_s[3], hour[3], minute[3], second[3];
    int_to_str_pad(t.year, year, 4);
    int_to_str_pad(t.month, month, 2);
    int_to_str_pad(t.day, day_s, 2);
    int_to_str_pad(t.hour, hour, 2);
    int_to_str_pad(t.minute, minute, 2);
    int_to_str_pad(t.second, second, 2);

    char out[128];
    strcpy(out, "RTC: Current time: ");
    strcat(out, year); strcat(out, "-");
    strcat(out, month); strcat(out, "-");
    strcat(out, day_s); strcat(out, " ");
    strcat(out, hour); strcat(out, ":");
    strcat(out, minute); strcat(out, ":");
    strcat(out, second);
    log_info(out);
}

uint32_t rtc_get_ticks(void)
{
    return rtc_ticks;
}
