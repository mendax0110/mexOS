#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include "../../shared/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the system timer
 * @param frequency The frequency in Hz
 */
void timer_init(uint32_t frequency);

/**
 * @brief Get the number of timer ticks since boot
 * @return The number of ticks
 */
uint32_t timer_get_ticks(void);

/**
 * @brief Get the number of seconds since boot
 * @return The number of seconds
 */
uint32_t timer_get_seconds(void);

/**
 * @brief Get the number of minutes since boot
 * @return The number of minutes
 */
uint32_t timer_get_minutes(void);

/**
 * @brief Get the number of hours since boot
 * @return The number of hours
 */
uint32_t timer_get_hours(void);

/**
 * @brief Wait for a specified number of timer ticks
 * @param ticks The number of ticks to wait
 */
void timer_wait(uint32_t ticks);

/**
 * @brief Disable the system timer
 */
void timer_disable(void);

#ifdef __cplusplus
}
#endif

#endif
