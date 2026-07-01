#ifndef KERNEL_SOURCE_LOCATION_H
#define KERNEL_SOURCE_LOCATION_H

/**
 * @brief Macro to get a representation of the current source file name (without the path)
 */
#define __FILENAME__ (__builtin_strchr(__FILE__, '/') ?  \
    __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#endif // KERNEL_SOURCE_LOCATION_H
