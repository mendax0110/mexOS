#include "usb_msc.h"
#include "lib/string.h"

bool usb_msc_build_read10(struct usb_msc_cbw* cbw, const uint32_t tag, const uint8_t lun, const uint32_t lba, const uint16_t block_count, const uint16_t block_size)
{
	if (!cbw || block_count == 0 || block_size == 0 || block_count > UINT32_MAX / block_size)
	{
		return false;
	}

	memset(cbw, 0, sizeof(*cbw));

	cbw->signature = USB_MSC_CBW_SIGNATURE;
	cbw->tag = tag;
	cbw->transfer_length = (uint32_t)block_count * block_size;
	cbw->flags = USB_MSC_DIR_IN;
	cbw->lun = lun;
	cbw->command_length = 10;

	cbw->command[0] = USB_MSC_SCSI_READ10;
	cbw->command[2] = (uint8_t)(lba >> 24);
	cbw->command[3] = (uint8_t)(lba >> 16);
	cbw->command[4] = (uint8_t)(lba >> 8);
	cbw->command[5] = (uint8_t)(lba);
	cbw->command[7] = (uint8_t)(block_count >> 8);
	cbw->command[8] = (uint8_t)(block_count);

	return true;
}

bool usb_msc_csw_is_success(const struct usb_msc_csw* csw, const uint32_t expected_tag, const uint32_t expected_length)
{
	if (!csw)
	{
		return false;
	}

	if (csw->signature != USB_MSC_CSW_SIGNATURE)
	{
		return false;
	}

	if (csw->tag != expected_tag)
	{
		return false;
	}

	if (csw->residue > expected_length)
	{
		return false;
	}

	if (csw->status != 0)
	{
		return false;
	}

	return true;
}