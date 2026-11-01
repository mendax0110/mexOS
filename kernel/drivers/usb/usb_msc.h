#ifndef KERNEL_USB_MSC_H
#define KERNEL_USB_MSC_H

#include "../../../shared/types.h"

#define USB_MSC_CBW_SIGNATURE 0x43425355u // USBC on the wire
#define USB_MSC_CSW_SIGNATURE 0x53425355u // USBS on the wire

#define USB_MSC_CBW_LENGTH 31
#define USB_MSC_CSW_LENGTH 13

#define USB_MSC_SCSI_TEST_UNIT_READY 0x00
#define USB_MSC_SCSI_INQUIRY 0x12
#define USB_MSC_SCSI_READ_CAPACITY10 0x25
#define USB_MSC_SCSI_READ10 0x28
#define USB_MSC_SCSI_WRITE10 0x2A

#define USB_MSC_DIR_OUT 0x00
#define USB_MSC_DIR_IN 0x80

/**
 * @brief USB Mass Storage Class Command Block Wrapper (CBW) structure \struct usb_msc_cbw
 */
struct usb_msc_cbw
{
	uint32_t signature;
	uint32_t tag;
	uint32_t transfer_length;
	uint8_t flags;
	uint8_t lun;
	uint8_t command_length;
	uint8_t command[16];
} PACKED;

/**
 * @brief USB Mass Storage Class Command Status Wrapper (CSW) structure \struct usb_msc_csw
 */
struct usb_msc_csw
{
	uint32_t signature;
	uint32_t tag;
	uint32_t residue;
	uint8_t status;
} PACKED;

/**
 * @brief Build a USB Mass Storage Class READ(10) command
 * @param cbw The Command Block Wrapper to populate
 * @param tag The unique tag for this command
 * @param lun The Logical Unit Number to target
 * @param lba The Logical Block Address to read from
 * @param block_count The number of blocks to read
 * @param block_size The size of each block in bytes
 * @return true on success, false on failure
 */
bool usb_msc_build_read10(struct usb_msc_cbw* cbw, uint32_t tag, uint8_t lun, uint32_t lba, uint16_t block_count, uint16_t block_size);

/**
 * @brief Check if a USB Mass Storage Class Command Status Wrapper is successful
 * @param csw The Command Status Wrapper to check
 * @param expected_tag The expected tag for this command
 * @param expected_length The expected transfer length for this command
 * @return true if the command was successful, false otherwise
 */
bool usb_msc_csw_is_success(const struct usb_msc_csw* csw, uint32_t expected_tag, uint32_t expected_length);

#endif // KERNEL_USB_MSC_H