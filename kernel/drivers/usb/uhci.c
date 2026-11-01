#include "uhci.h"

#include "addr.h"
#include "arch/i686/arch.h"
#include "lib/log.h"
#include "lib/string.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

#define UHCI_MAX_CONTROLLERS 4
#define UHCI_FRAME_COUNT 1024
#define UHCI_MAX_TRANSFER_SIZE 2048
#define UHCI_MAX_PACKET_SIZE 64
#define UHCI_TRANSFER_TIMEOUT_FRAMES 1000

#define UHCI_REG_COMMAND 0x00
#define UHCI_REG_STATUS 0x02
#define UHCI_REG_INTERRUPT 0x04
#define UHCI_REG_FRAME_NUMBER 0x06
#define UHCI_REG_FRAME_LIST_BASE 0x08
#define UHCI_REG_SOF_MOD 0x0C

#define UHCI_CMD_RUN 0x0001
#define UHCI_CMD_HOST_RESET 0x0002
#define UHCI_CMD_CONFIGURE 0x0040
#define UHCI_CMD_MAX_PACKET 0x0080

#define UHCI_STATUS_USBINT 0x0001
#define UHCI_STATUS_USBERRINT 0x0002
#define UHCI_STATUS_RESUME 0x0004
#define UHCI_STATUS_HOST_SYSTEM_ERROR 0x0008
#define UHCI_STATUS_PROCESS_ERROR 0x0010
#define UHCI_STATUS_HALTED 0x0020
#define UHCI_STATUS_W1C 0x001F

#define UHCI_LINK_TERMINATE 0x00000001
#define UHCI_LINK_QUEUE_HEAD 0x00000002

#define UHCI_TD_ACTIVE (1u << 23)
#define UHCI_TD_IOC (1u << 24)
#define UHCI_TD_LOW_SPEED (1u << 26)
#define UHCI_TD_ERROR_COUNT_3 (3u << 27)
#define UHCI_TD_SHORT_PACKET_DETECT (1u << 29)
#define UHCI_TD_ERROR_MASK (0x007E0000u)

#define UHCI_PID_OUT 0xE1
#define UHCI_PID_IN 0x69
#define UHCI_PID_SETUP 0x2D

/**
 * @brief Represents a UHCI Queue Head structure \struct uhci_queue_head
 */
struct uhci_queue_head
{
    uint32_t horizontal_link;
    uint32_t element_link;
} PACKED;

/**
 * @brief Represents a UHCI Transfer Descriptor structure \struct uhci_transfer_descriptor
 */
struct uhci_transfer_descriptor
{
    uint32_t link;
    uint32_t status;
    uint32_t token;
    uint32_t buffer;
} PACKED;

/**
 * @brief Represents a UHCI DMA Transfer structure, containing a Queue Head, Transfer Descriptors, and Setup Packet \struct uhci_dma_transfer
 */
struct uhci_dma_transfer
{
    struct uhci_queue_head qh;
    struct uhci_transfer_descriptor td[3];
    uint8_t setup[8];
} PACKED ALIGNED(16);

/**
 * @brief Represents a UHCI Controller structure, containing I/O base address, frame list, and status flags \struct uhci_controller
 */
struct uhci_controller
{
    uint16_t io_base;
    uint32_t* frame_list;
    uint32_t frame_list_phys;
    bool initialized;
    bool transfer_active;
    bool unusable;
};

static struct uhci_controller g_uhci[UHCI_MAX_CONTROLLERS];

/**
 * @brief Reads a 16-bit value from the specified UHCI controller register.
 * @param controller The UHCI controller from which to read the register.
 * @param reg The offset of the register to read.
 * @return The value read from the register.
 */
static uint16_t uhci_read16(const struct uhci_controller* controller, const uint16_t reg)
{
    return inw((uint16_t)(controller->io_base + reg));
}

/**
 * @brief Writes a 16-bit value to the specified UHCI controller register.
 * @param controller The UHCI controller to which to write the register.
 * @param reg The offset of the register to write.
 * @param value The value to write to the register.
 */
static void uhci_write16(const struct uhci_controller* controller, const uint16_t reg, const uint16_t value)
{
    outw((uint16_t)(controller->io_base + reg), value);
}

/**
 * @brief Waits for a short duration, typically used for timing-sensitive operations.
 */
static void uhci_wait_short(void)
{
    for (uint32_t i = 0; i < 1000; i++)
    {
        io_wait();
    }
}

/**
 * @brief Waits for the UHCI controller to halt.
 * @param controller The UHCI controller to wait for.
 * @return true if the controller halts within the timeout, false otherwise.
 */
static bool uhci_wait_halted(const struct uhci_controller* controller)
{
    for (uint32_t i = 0; i < 100000; i++)
    {
        if (uhci_read16(controller, UHCI_REG_STATUS) & UHCI_STATUS_HALTED)
        {
            return true;
        }
        io_wait();
    }
    return false;
}

/**
 * @brief Stops the UHCI controller by writing to the command register and waiting for it to halt.
 * @param controller The UHCI controller to stop.
 * @return true if the controller stops successfully, false otherwise.
 */
static bool uhci_stop(struct uhci_controller* controller)
{
    uhci_write16(controller, UHCI_REG_COMMAND, 0);
    if (!uhci_wait_halted(controller))
    {
        controller->unusable = true;
        return false;
    }
    return true;
}

/**
 * @brief Encodes a UHCI token for a transfer descriptor.
 * @param pid The packet identifier (PID) for the transfer (e.g., OUT, IN, SETUP).
 * @param address The USB device address.
 * @param endpoint The endpoint number.
 * @param toggle The data toggle value (0 or 1).
 * @param length The length of the data to transfer.
 * @return The encoded token value.
 */
static uint32_t uhci_token(const uint8_t pid, const uint8_t address, const uint8_t endpoint, const uint8_t toggle, const uint16_t length)
{
    const uint32_t encoded_length = length == 0 ? 0x7FFu : (uint32_t)(length - 1);
    return (uint32_t)pid |
           ((uint32_t)(address & 0x7F) << 8) |
           ((uint32_t)(endpoint & 0x0F) << 15) |
           ((uint32_t)(toggle & 1) << 19) |
           (encoded_length << 21);
}

/**
 * @brief Calculates the actual length of data transferred based on the status of a transfer descriptor.
 * @param td The transfer descriptor to check.
 * @return The actual length of data transferred, or 0 if there was an error.
 */
static uint32_t uhci_actual_length(const struct uhci_transfer_descriptor* td)
{
    const uint32_t encoded = td->status & 0x7FFu;
    return encoded == 0x7FFu ? 0 : encoded + 1;
}

/**
 * @brief Waits for a UHCI transfer to complete by monitoring the status of transfer descriptors.
 * @param controller The UHCI controller managing the transfer.
 * @param td The array of transfer descriptors to monitor.
 * @param td_count The number of transfer descriptors in the array.
 * @return true if the transfer completes successfully, false if there is an error or timeout.
 */
static bool uhci_wait_transfer(const struct uhci_controller* controller, const struct uhci_transfer_descriptor* td, const uint32_t td_count)
{
    uint16_t previous_frame = (uint16_t)(uhci_read16(controller, UHCI_REG_FRAME_NUMBER) & 0x03FF);
    uint32_t elapsed_frames = 0;

    while (elapsed_frames < UHCI_TRANSFER_TIMEOUT_FRAMES)
    {
        const uint16_t status = uhci_read16(controller, UHCI_REG_STATUS);
        if (status & (UHCI_STATUS_HOST_SYSTEM_ERROR | UHCI_STATUS_PROCESS_ERROR))
        {
            return false;
        }

        bool all_done = true;

        for (uint32_t i = 0; i < td_count; i++)
        {
            if (td[i].status & UHCI_TD_ACTIVE)
            {
                all_done = false;
            }
            else if (td[i].status & UHCI_TD_ERROR_MASK)
            {
                return false;
            }
        }

        if (all_done)
        {
            return true;
        }

        const uint16_t frame = (uint16_t)(uhci_read16(controller, UHCI_REG_FRAME_NUMBER) & 0x03FF);
        elapsed_frames += (frame - previous_frame) & 0x03FF;
        previous_frame = frame;
        io_wait();
    }

    return false;
}

/**
 * @brief Prepares a UHCI transfer descriptor with the specified parameters.
 * @param td The transfer descriptor to prepare.
 * @param physical The physical address of the data buffer.
 * @param token The encoded token for the transfer.
 * @param low_speed Whether the transfer is for a low-speed device.
 * @param short_packet_detect Whether to enable short packet detection.
 * @param interrupt_on_complete Whether to generate an interrupt on completion.
 */
static void uhci_prepare_td(struct uhci_transfer_descriptor* td,
                            const uint32_t physical,
                            const uint32_t token,
                            const bool low_speed,
                            const bool short_packet_detect,
                            const bool interrupt_on_complete)
{
    td->link = UHCI_LINK_TERMINATE;
    td->status = UHCI_TD_ACTIVE | UHCI_TD_ERROR_COUNT_3 |
                 (low_speed ? UHCI_TD_LOW_SPEED : 0) |
                 (short_packet_detect ? UHCI_TD_SHORT_PACKET_DETECT : 0) |
                 (interrupt_on_complete ? UHCI_TD_IOC : 0);
    td->token = token;
    td->buffer = physical;
}

/**
 * @brief Runs a UHCI transfer descriptor by configuring the controller and waiting for completion.
 * @param controller The UHCI controller managing the transfer.
 * @param transfer The DMA transfer structure containing the queue head and transfer descriptors.
 * @param qh_phys The physical address of the queue head.
 * @param td_phys The physical address of the first transfer descriptor.
 * @param td The array of transfer descriptors to run.
 * @param safe_to_free A pointer to a boolean that will be set to true if it is safe to free resources after the transfer.
 * @return true if the transfer completes successfully, false otherwise.
 */
static bool uhci_run_td(struct uhci_controller* controller,
                        struct uhci_dma_transfer* transfer,
                        const uint32_t qh_phys,
                        const uint32_t td_phys,
                        struct uhci_transfer_descriptor* td,
                        bool* safe_to_free)
{
    *safe_to_free = true;
    if (!uhci_stop(controller))
    {
        return false;
    }

    transfer->qh.horizontal_link = UHCI_LINK_TERMINATE;
    transfer->qh.element_link = td_phys;
    td->link = UHCI_LINK_TERMINATE;

    for (uint32_t i = 0; i < UHCI_FRAME_COUNT; i++)
    {
        controller->frame_list[i] = qh_phys | UHCI_LINK_QUEUE_HEAD;
    }

    ASM_V("" : : : "memory");
    uhci_write16(controller, UHCI_REG_STATUS, UHCI_STATUS_W1C);
    uhci_write16(controller, UHCI_REG_COMMAND, UHCI_CMD_RUN | UHCI_CMD_CONFIGURE | UHCI_CMD_MAX_PACKET);
    const bool completed = uhci_wait_transfer(controller, td, 1);
    const bool stopped = uhci_stop(controller);

    if (stopped)
    {
        for (uint32_t i = 0; i < UHCI_FRAME_COUNT; i++)
        {
            controller->frame_list[i] = UHCI_LINK_TERMINATE;
        }
    }
    else
    {
        *safe_to_free = false;
    }

    return completed && stopped;
}

/**
 * @brief Enters a UHCI transfer by checking the controller's state and setting the transfer_active flag.
 * @param controller The UHCI controller to enter the transfer.
 * @return true if the transfer can be entered, false otherwise.
 */
static bool uhci_enter_transfer(struct uhci_controller* controller)
{
    const uint32_t flags = irq_save();
    if (!controller->initialized || controller->unusable || controller->transfer_active)
    {
        irq_restore(flags);
        return false;
    }

    controller->transfer_active = true;
    irq_restore(flags);
    return true;
}

/**
 * @brief Leaves a UHCI transfer by clearing the transfer_active flag.
 * @param controller The UHCI controller to leave the transfer.
 */
static void uhci_leave_transfer(struct uhci_controller* controller)
{
    const uint32_t flags = irq_save();
    controller->transfer_active = false;
    irq_restore(flags);
}

bool uhci_init(const uint32_t controller_index, const uint16_t io_base)
{
    if (controller_index >= UHCI_MAX_CONTROLLERS || io_base == 0)
    {
        return false;
    }

    struct uhci_controller* controller = &g_uhci[controller_index];
    if (controller->initialized)
    {
        uhci_shutdown(controller_index);
    }

    memset(controller, 0, sizeof(*controller));
    controller->io_base = io_base;

    void* frame_list_phys = pmm_alloc_block();
    if (!frame_list_phys)
    {
        return false;
    }
    controller->frame_list_phys = PTR_TO_U32(frame_list_phys);
    controller->frame_list = (uint32_t*)phys_to_virt(controller->frame_list_phys);
    for (uint32_t i = 0; i < UHCI_FRAME_COUNT; i++)
    {
        controller->frame_list[i] = UHCI_LINK_TERMINATE;
    }

    uhci_write16(controller, UHCI_REG_COMMAND, UHCI_CMD_HOST_RESET);
    bool reset_complete = false;
    for (uint32_t i = 0; i < 100000; i++)
    {
        if (!(uhci_read16(controller, UHCI_REG_COMMAND) & UHCI_CMD_HOST_RESET))
        {
            reset_complete = true;
            break;
        }
        io_wait();
    }
    if (!reset_complete)
    {
        pmm_free_block(frame_list_phys);
        memset(controller, 0, sizeof(*controller));
        return false;
    }

    uhci_write16(controller, UHCI_REG_COMMAND, 0);
    uhci_wait_short();
    uhci_write16(controller, UHCI_REG_STATUS, UHCI_STATUS_W1C);
    uhci_write16(controller, UHCI_REG_INTERRUPT, 0);
    uhci_write16(controller, UHCI_REG_FRAME_NUMBER, 0);
    outl((uint16_t)(io_base + UHCI_REG_FRAME_LIST_BASE), controller->frame_list_phys);
    outb((uint16_t)(io_base + UHCI_REG_SOF_MOD), 0x40);

    controller->initialized = true;
    uhci_write16(controller, UHCI_REG_COMMAND, UHCI_CMD_RUN | UHCI_CMD_CONFIGURE | UHCI_CMD_MAX_PACKET);

    for (uint32_t i = 0; i < 100000; i++)
    {
        if (!(uhci_read16(controller, UHCI_REG_STATUS) & UHCI_STATUS_HALTED))
        {
            log_info_fmt("UHCI controller %u started at I/O base 0x%x",
                         controller_index, io_base);
            return true;
        }
        io_wait();
    }

    uhci_stop(controller);
    pmm_free_block(frame_list_phys);
    memset(controller, 0, sizeof(*controller));
    return false;
}

void uhci_shutdown(const uint32_t controller_index)
{
    if (controller_index >= UHCI_MAX_CONTROLLERS)
    {
        return;
    }

    struct uhci_controller* controller = &g_uhci[controller_index];
    if (!controller->initialized)
    {
        return;
    }

    if (uhci_stop(controller))
    {
        if (controller->frame_list_phys)
        {
            pmm_free_block(PTR_FROM_U32(controller->frame_list_phys));
        }
        memset(controller, 0, sizeof(*controller));
    }
}

int uhci_control_transfer(struct usb_device* device, const uint8_t setup[8], void* data, const uint16_t length, uint16_t* actual_length)
{
    if (actual_length)
    {
        *actual_length = 0;
    }
    if (!device || !setup || (length && !data) || length > UHCI_MAX_TRANSFER_SIZE ||
        length != (uint16_t)(setup[6] | ((uint16_t)setup[7] << 8)) ||
        device->endpoint0_max_packet_size == 0 ||
        device->endpoint0_max_packet_size > UHCI_MAX_PACKET_SIZE ||
        device->address > 127 || device->controller_index >= UHCI_MAX_CONTROLLERS)
    {
        return -1;
    }

    struct uhci_controller* controller = &g_uhci[device->controller_index];
    if (!uhci_enter_transfer(controller))
    {
        return -1;
    }

    void* transfer_phys = pmm_alloc_block();
    uint32_t data_pages = 0;
    void* data_phys = NULL;
    if (length)
    {
        data_pages = (length + PMM_BLOCK_SIZE - 1) / PMM_BLOCK_SIZE;
        data_phys = pmm_alloc_blocks(data_pages);
    }
    if (!transfer_phys || (length && !data_phys))
    {
        if (transfer_phys)
        {
            pmm_free_block(transfer_phys);
        }
        if (data_phys)
        {
            pmm_free_blocks(data_phys, data_pages);
        }
        uhci_leave_transfer(controller);
        return -1;
    }

    struct uhci_dma_transfer* transfer = phys_to_virt(PTR_TO_U32(transfer_phys));
    uint32_t data_phys_addr = 0;
    uint8_t* data_virt = NULL;
    if (length)
    {
        data_phys_addr = PTR_TO_U32(data_phys);
        data_virt = (uint8_t*)phys_to_virt(data_phys_addr);
        if (!(setup[0] & 0x80))
        {
            memcpy(data_virt, data, length);
        }
    }
    memcpy(transfer->setup, setup, 8);

    const uint32_t qh_phys = PTR_TO_U32(transfer_phys);
    const uint32_t td_phys = qh_phys + (uint32_t)OFFSET_OF(struct uhci_dma_transfer, td);
    const uint32_t setup_phys = qh_phys + (uint32_t)OFFSET_OF(struct uhci_dma_transfer, setup);
    bool safe_to_free = true;

    uhci_prepare_td(&transfer->td[0], setup_phys,
                    uhci_token(UHCI_PID_SETUP, device->address, 0, 0, 8),
                    device->low_speed, false, true);
    bool success = uhci_run_td(controller, transfer, qh_phys, td_phys, &transfer->td[0], &safe_to_free);

    uint32_t transferred = 0;
    const bool data_in = (setup[0] & 0x80) != 0;
    const uint8_t data_pid = data_in ? UHCI_PID_IN : UHCI_PID_OUT;
    uint8_t data_toggle = 1;
    while (success && transferred < length)
    {
        uint16_t packet_length = (uint16_t)(length - transferred);
        if (packet_length > device->endpoint0_max_packet_size)
        {
            packet_length = device->endpoint0_max_packet_size;
        }

        uhci_prepare_td(&transfer->td[0], data_phys_addr + transferred,
                        uhci_token(data_pid, device->address, 0, data_toggle, packet_length),
                        device->low_speed, data_in, true);
        success = uhci_run_td(controller, transfer, qh_phys, td_phys, &transfer->td[0], &safe_to_free);
        if (!success)
        {
            break;
        }

        const uint32_t packet_actual = uhci_actual_length(&transfer->td[0]);
        if (packet_actual > packet_length || (!data_in && packet_actual != packet_length))
        {
            success = false;
            break;
        }
        if (data_in && packet_actual)
        {
            memcpy((uint8_t*)data + transferred, data_virt + transferred, packet_actual);
        }
        transferred += packet_actual;
        data_toggle ^= 1;

        if (data_in && packet_actual < packet_length)
        {
            break;
        }
    }

    if (success)
    {
        const uint8_t status_pid = data_in || length == 0 ? UHCI_PID_OUT : UHCI_PID_IN;
        uhci_prepare_td(&transfer->td[0], 0,
                        uhci_token(status_pid, device->address, 0, 1, 0),
                        device->low_speed, false, true);
        success = uhci_run_td(controller, transfer, qh_phys, td_phys, &transfer->td[0], &safe_to_free);
    }

    if (success && actual_length)
    {
        *actual_length = (uint16_t)transferred;
    }

    if (safe_to_free && data_phys)
    {
        pmm_free_blocks(data_phys, data_pages);
    }
    if (safe_to_free)
    {
        pmm_free_block(transfer_phys);
    }
    uhci_leave_transfer(controller);
    return success ? 0 : -1;
}

int uhci_bulk_transfer(struct usb_device* device, const uint8_t endpoint_address, void* data, const uint32_t length, uint32_t* actual_length)
{
    if (actual_length)
    {
        *actual_length = 0;
    }
    if (!device || (length && !data) || length > UHCI_MAX_TRANSFER_SIZE ||
        device->address > 127 || device->controller_index >= UHCI_MAX_CONTROLLERS)
    {
        return -1;
    }

    const bool in = (endpoint_address & 0x80) != 0;
    const uint8_t endpoint = endpoint_address & 0x0F;
    uint8_t* toggle;
    uint8_t max_packet;
    if (in && endpoint_address == device->bulk_in_endpoint)
    {
        toggle = &device->bulk_in_toggle;
        max_packet = device->bulk_in_max_packet_size;
    }
    else if (!in && endpoint_address == device->bulk_out_endpoint)
    {
        toggle = &device->bulk_out_toggle;
        max_packet = device->bulk_out_max_packet_size;
    }
    else
    {
        return -1;
    }
    if (max_packet == 0 || max_packet > UHCI_MAX_PACKET_SIZE)
    {
        return -1;
    }

    struct uhci_controller* controller = &g_uhci[device->controller_index];
    if (!uhci_enter_transfer(controller))
    {
        return -1;
    }

    void* transfer_phys = pmm_alloc_block();
    uint32_t data_pages = 0;
    void* data_phys = NULL;
    if (length)
    {
        data_pages = (length + PMM_BLOCK_SIZE - 1) / PMM_BLOCK_SIZE;
        data_phys = pmm_alloc_blocks(data_pages);
    }
    if (!transfer_phys || (length && !data_phys))
    {
        if (transfer_phys)
        {
            pmm_free_block(transfer_phys);
        }
        if (data_phys)
        {
            pmm_free_blocks(data_phys, data_pages);
        }
        uhci_leave_transfer(controller);
        return -1;
    }

    struct uhci_dma_transfer* transfer = phys_to_virt(PTR_TO_U32(transfer_phys));
    uint32_t data_phys_addr = 0;
    uint8_t* data_virt = NULL;
    if (length)
    {
        data_phys_addr = PTR_TO_U32(data_phys);
        data_virt = (uint8_t*)phys_to_virt(data_phys_addr);
        if (!in)
        {
            memcpy(data_virt, data, length);
        }
    }

    const uint32_t qh_phys = PTR_TO_U32(transfer_phys);
    const uint32_t td_phys = qh_phys + (uint32_t)OFFSET_OF(struct uhci_dma_transfer, td);
    bool safe_to_free = true;
    bool success = true;
    uint32_t transferred = 0;
    bool send_zero_length_packet = length == 0;

    while (success && (transferred < length || send_zero_length_packet))
    {
        uint16_t packet_length = (uint16_t)(length - transferred);
        if (packet_length > max_packet)
        {
            packet_length = max_packet;
        }

        uhci_prepare_td(&transfer->td[0], data_phys_addr + transferred,
                        uhci_token(in ? UHCI_PID_IN : UHCI_PID_OUT,
                                   device->address, endpoint, *toggle, packet_length),
                        device->low_speed, in, true);
        success = uhci_run_td(controller, transfer, qh_phys, td_phys, &transfer->td[0], &safe_to_free);
        if (!success)
        {
            break;
        }

        const uint32_t packet_actual = uhci_actual_length(&transfer->td[0]);
        if (packet_actual > packet_length || (!in && packet_actual != packet_length))
        {
            success = false;
            break;
        }
        if (in && packet_actual)
        {
            memcpy((uint8_t*)data + transferred, data_virt + transferred, packet_actual);
        }
        transferred += packet_actual;
        *toggle ^= 1;
        send_zero_length_packet = false;

        if (in && packet_actual < packet_length)
        {
            break;
        }
    }

    if (success && actual_length)
    {
        *actual_length = transferred;
    }

    if (safe_to_free && data_phys)
    {
        pmm_free_blocks(data_phys, data_pages);
    }
    if (safe_to_free)
    {
        pmm_free_block(transfer_phys);
    }
    uhci_leave_transfer(controller);
    return success ? 0 : -1;
}
