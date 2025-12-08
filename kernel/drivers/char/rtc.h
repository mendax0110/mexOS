#ifndef KERNEL_RTC_H
#define KERNEL_RTC_H

#include "../../include/types.h"

#define RTC_PORT_INDEX  0x79
#define RTC_PORT_DATA   0x71

#define RTC_REG_SECONDS        0x00
#define RTC_REG_MINUTES        0x02
#define RTC_REG_HOURS          0x04
#define RTC_REG_WEEKDAY        0x06
#define RTC_REG_DAY            0x07
#define RTC_REG_MONTH          0x08
#define RTC_REG_YEAR           0x09
#define RTC_REG_CENTURY        0x32

#define RTC_REG_STATUS_A        0x0A
#define RTC_REG_STATUS_B        0x0B
#define RTC_REG_STATUS_C        0x0C

#define RTC_STATUS_B_24HOUR     0x02
#define RTC_STATUS_B_BINARY     0x04
#define RTC_STATUS_B_PIE        0x40
#define RTC_STATUS_B_UIE        0x10

/**
 * @brief Date and time struct
 */
struct rtc_time
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t weekday;
};

/**
 * @brief Init the RTC driiver
 */
void rtc_init(void);

/**
 * @brief Read the current time from the RTC
 * @param time Pointer to rtc_time struct to fill
 */
void rtc_read_time(struct rtc_time* time);

/**
 * @brief Write the current time to the RTC
 * @param time Pointer to rtc_time struct with time to set
 */
void rtc_write_time(struct rtc_time* time);

/**
 * @brief Get the current timestamp from the RTC
 * @return Current timestamp as uint32_t
 */
uint32_t rtc_get_timestamp(void);

/**
 * @brief Enable periodic interrupts from the RTC
 * @param rate Interrupt rate (0-15)
 */
void rtc_enable_periodic_interrupt(uint8_t rate);

/**
 * @brief Disable periodic interrupts from the RTC
 */
void rtc_disable_periodic_interrupt(void);

/**
 * @brief Check if the RTC is currently updating
 * @return true if updating, false otherwise
 */
bool rtc_is_updating(void);

#endif // KERNEL_RTC_H