#ifndef KERNEL_MATH_H
#define KERNEL_MATH_H

#include "types.h"

/**
 * @brief Macro to find the minimum
 * @param a The first value
 * @param b The second value
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief Macro to find the maximum
 * @param a The first value
 * @param b The second value
 */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief Macro to clamp a value between two bounds
 * @param x The value to clamp
 * @param min The minimum value
 * @param max The maximum value
 */
#define CLAMP(x, min, max) (MAX((min), MIN((x), (max))))

/**
 * @brief Macro to find the absolute value
 * @param x The value to get the abs from
 */
#define ABS(x) ((x) < 0 ? -(x) : (x))

/**
 * @brief Macro to perform integer division with ceiling
 * @param a The first value
 * @param b The 2nd value
 */
#define DIV_CEIL(a, b) (((a) + (b) - 1) / (b))

/**
 * @brief Macro to check if a number is a power of two
 * @param x The number to check
 */
#define IS_POWER_OF_TWO(x) ((x) != 0 && ((x) & ((x) - 1)) == 0))

/**
 * @brief Macro to align a value up to the nearest multiple of a given alignment
 * @param x The value to align
 * @param align The alignment
 */
#define ALIGN_UP(x, align) (DIV_CEIL((x), (align)) * (align))

/**
 * @brief Macro to align a value down to the nearest multiple of a given alignment
 * @param x The value to align
 * @param align The alignment
 */
#define ALIGN_DOWN(x, align) ((x) / (align) * (align))

/**
 * @brief Macro to swap two values of the same type
 * @param a The first value
 * @param b The second value
 */
#define SWAP(a, b)              \
    do                          \
    {                           \
        typeof(a) _tmp = (a);   \
        (a) = (b);              \
        (b) = _tmp;             \
    } while (0)

/**
 * @brief Union representing f32 bits \union f32_bits_t
 */
typedef union
{
	float32_t f;
	uint32_t u;
} f32_bits_t;

/**
 * @brief Union representing f64 bits \union f64_bits_t
 */
typedef union
{
	float64_t f;
	uint64_t u;
} f64_bits_t;

/**
 * @brief Helper to get float 32 absolute
 * @param _x The value to check
 * @return The float 32 abs
 */
static inline float32_t fabsf(const float32_t _x)
{
	f32_bits_t v = { .f = _x };
	v.u &= 0x7FFFFFFFu;
	return v.f;
}

/**
 * @brief Helper to get float 64 absolute
 * @param _x The value to check
 * @return The float 64 abs
 */
static inline float64_t fabs(const float64_t _x)
{
	f64_bits_t v = { .f = _x };
	v.u &= 0x7FFFFFFFFFFFFFFFull;
	return v.f;
}

/**
 * @brief Helper to check if a value is Not a Number
 * @param _x The value to check
 * @return true is value is NaN, false otherwise
 */
static inline bool isNan(const float64_t _x)
{
	const f64_bits_t v = { .f = _x };
	return (v.u & 0x7FFFFFFFFFFFFFFFull) > 0x7FFFFFFFFFFFFFFFull;
}

/**
 * @brief Helper to check if a value is Infinite
 * @param _x The value to check
 * @return true if value if Inf, false otherwise
 */
static inline bool isInf(const float64_t _x)
{
	const f64_bits_t v = { .f = _x };
	return (v.u & 0x7FFFFFFFFFFFFFFFull) == 0x7FFFFFFFFFFFFFFFull;
}

/**
 * @brief Helper to get max value for float 32
 * @param _a The first value to check
 * @param _b The second value to check
 * @return The max float 32 value
 */
static inline float32_t fmaxf(const float32_t _a, const float32_t _b)
{
	return (_a > _b) ? _a : _b;
}

/**
 * @brief Helper to get the max value for float 64
 * @param _a The first value to check
 * @param _b The second value to check
 * @return The max float 64 value
 */
static inline float64_t fmax(const float64_t _a, const float64_t _b)
{
	return (_a > _b) ? _a : _b;
}

/**
 * @brief Helper to to fuzzy compare on float 32 values
 * @param _a The first value to check
 * @param _b The second value to check
 * @param _abs_eps Absolute value
 * @param _rel_eps Relative value
 * @return true if _a and _b are "equal", false otherwise
 */
static inline bool fuzzy_compare_f32(const float32_t _a, const float32_t _b, const float32_t _abs_eps, const float32_t _rel_eps)
{
	if (_a == _b) return true;
	const float32_t diff = fabsf(_a - _b);
	if (diff <= _abs_eps) return true;
	return diff <= _rel_eps * fmaxf(fabsf(_a), fabsf(_b));
}

/**
 * @brief Helper to to fuzzy compare on float 64 values
 * @param _a The first value to check
 * @param _b The second value to check
 * @param _abs_eps Absolute value
 * @param _rel_eps Relative value
 * @return true if _a and _b are "equal", false otherwise
 */
static inline bool fuzzy_compare_f64(const float64_t _a, const float64_t _b, const float64_t _abs_eps, const float64_t _rel_eps)
{
	if (_a == _b) return true;
	const float64_t diff = fabs(_a - _b);
	if (diff <= _abs_eps) return true;
	return diff <= _rel_eps * fmax(fabs(_a), fabs(_b));
}

/**
 * @brief Generic helper macro to do "fuzzy compare" on either float32 or float64 values
 * @param _num1 The first value to check
 * @param _num2 The second value to check
 * @param _abs_eps The absolute value
 * @param _rel_eps The relative value
 */
#define FUZZY_COMPARE(_num1, _num2, _abs_eps, _rel_eps)	\
	GENERIC((_num1) + (_num2),							\
		float32_t: fuzzy_compare_f32,					\
		float64_t: fuzzy_compare_f64,					\
	)((_num1), (_num2), (_abs_eps), (_rel_eps))

#endif // KERNEL_MATH_H
