#ifndef KERNEL_MOUSE_H
#define KERNEL_MOUSE_H

#include "../shared/types.h"
#include "../shared/video_abi.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MOUSE_DATA_PORT 0x60
#define MOUSE_STATUS_PORT 0x64

/**
 * @brief Initializes the PS2 mouse
 */
void mouse_init(void);

/**
 * @brief Non-blocking read of current state of cursor/button state
 * @param state Pointer to the mouse state structure
 * @return 1 always (state is always valid)
 */
int mouse_try_get_state(struct mouse_state* state);

/**
 * @brief Shutdown the mouse driver
 */
void mouse_shutdown(void);

#ifdef __cplusplus
}
#endif


#endif // KERNEL_MOUSE_H