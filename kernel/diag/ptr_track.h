#ifndef KERNEL_PTR_TRACK_H
#define KERNEL_PTR_TRACK_H

#include "include/source_location.h"
#include "include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_TRACKED_PTRS 256

/**
 * @brief Struct to represent a tracked ptr entry \struct tracked_ptr_entry_t
 */
typedef struct
{
    void* ptr;
    const char* name;
    const char* file;
    int line;
    bool in_use;
} tracked_ptr_entry_t;

/**
 * @brief Method to register a pointer for tracking
 * @param ptr The pointer to track
 * @param name The name
 * @param file The file
 * @param line The line number
 * @param in_use Check if in use or not
 */
void ptr_track_register(void* ptr, const char* name, const char* file, int line, bool in_use);

/**
 * @brief Method to unregister a tracked ptr
 * @param ptr The pointer to unregister
 * @return True if worked, false otherwise
 */
bool ptr_track_unregister(void* ptr);

/**
 * @brief Lookup method for tracked pointers
 * @param ptr The pointer to lookup
 * @return A pointer to the tracked ptr entry, or NULL if not found
 */
const tracked_ptr_entry_t* ptr_track_lookup(void* ptr);

/**
 * @brief Method to dump tracked data
 */
void ptr_track_dump(void);

/**
 * @brief Method to track the location of a pointer
 * @param ptr The pointer to check
 * @return The location
 */
const char* ptr_track_location(void* ptr);

/**
 * @brief Helper macro to track pointer
 * @param p The pointer to track
 */
#define TRACK_PTR(p) \
    ptr_track_register((void*)(p), __func__, __FILENAME__, __LINE__, true)

/**
 * @brief Helper macro to untrack a pointer
 * @param p The pointer to untrack
 */
#define UNTRACK_PTR(p) \
    ptr_track_unregister((void*)(p))

/**
 * @brief Helper macro to get the location of a pointer
 * @param p The pointer to check
 */
#define POINTER_LOCATION_FROM(p) \
    ptr_track_location((void*)(p))

#ifdef __cplusplus
}
#endif

#endif // KERNEL_PTR_TRACK_H
