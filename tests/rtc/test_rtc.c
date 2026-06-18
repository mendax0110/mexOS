#include "test_rtc.h"
#include "../drivers/char/rtc.h"

TEST_CASE(rtc_read_time)
{
    rtc_init();
    uint32_t timeStamp = rtc_get_timestamp();
    TEST_ASSERT_GT(timeStamp, 0);
    return TEST_PASS;
}

TEST_CASE(rtc_write_time)
{
    struct rtc_time time;
    time.second = 30;
    time.minute = 45;
    time.hour = 12;
    time.day = 15;
    time.month = 6;
    time.year = 2024;

    rtc_write_time(&time);

    struct rtc_time read_time;
    rtc_read_time(&read_time);

    TEST_ASSERT_EQ(read_time.second, time.second);
    TEST_ASSERT_EQ(read_time.minute, time.minute);
    TEST_ASSERT_EQ(read_time.hour, time.hour);
    TEST_ASSERT_EQ(read_time.day, time.day);
    TEST_ASSERT_EQ(read_time.month, time.month);
    TEST_ASSERT_EQ(read_time.year, time.year);

    return TEST_PASS;
}

TEST_CASE(rtc_periodic_interrupt)
{
    rtc_disable_periodic_interrupt();
    TEST_ASSERT(rtc_is_updating() == false);
    rtc_enable_periodic_interrupt(6);
    for (volatile int i = 0; i < 1000000; i++);
    TEST_ASSERT(rtc_get_ticks() > 0);

    return TEST_PASS;
}

static struct test_case rtc_cases[] = {
    TEST_ENTRY(rtc_read_time),
    TEST_ENTRY(rtc_write_time),
    TEST_ENTRY(rtc_periodic_interrupt)
};

static struct test_suite rtc_suite = {
    .name = "RTC Tests",
    .cases = rtc_cases,
    .count = 3
};

struct test_suite* test_rtc_get_suite(void)
{
    return &rtc_suite;
}