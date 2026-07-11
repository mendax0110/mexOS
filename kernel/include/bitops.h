#ifndef KERNEL_BITOPS_H
#define KERNEL_BITOPS_H

#include "../../shared/types.h"

/**
 * @brief Macro to make bit mask
 * @param val The value
 * @param mask The mask
 */
#define BIT_MASK(val, mask) ((uint32_t)((val) & (mask)))

/**
 * @brief Macro to make bit flag
 * @param val The value
 * @param bit The bit
 */
#define BIT_FLAG(val, bit) (((val) & (1U << (bit))) != 0)

/**
 * @brief Macro for bit
 * @param bit The bit
 */
#define BIT(bit) (1U << (bit))

/**
 * @brief Macro to test bit
 * @param val The value
 * @param bit The bit
 */
#define TEST_BIT(val, bit) (((val) & BIT(bit)) != 0)

/**
 * @brief Macro to set bit
 * @param word The word
 * @param mask The mask
 */
#define BIT_SET(word, mask) ((word) |= (mask))

/**
 * @brief Macro to clear bit
 * @param word The word
 * @param mask The mask
 */
#define BIT_CLEAR(word, mask) ((word) &= ~(mask))

/**
 * @brief Macro to toggle bit
 * @param word The word
 * @param mask The mask
 */
#define BIT_TOGGLE(word, mask) ((word) ^= (mask))



#endif // KERNEL_BITOPS_H
