#ifndef USB_MSD_H
#define USB_MSD_H

#include "usb.h"
#include "../util.h"

void usb_msd_static_init();

struct usb_msd_dev {
	struct usb_dev *udev;

	struct usb_pipe *bulk_in;
	struct usb_pipe *bulk_out;
	struct usb_pipe *pipe0;

	uint32_t CBWTag;
	int max_lun;
};

struct command_block_wrapper {
	uint32_t dCBWSignature;
#define CBW_SIGNATURE 0x43425355
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
#define CBW_FLAGS_IN 0x80
#define CBW_FLAGS_OUT 0x00
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
#define CB_MAX_SIZE 16
} __attribute__((packed));

struct command_status_wrapper {
	uint32_t dCSWSignature;
#define CSW_SIGNATURE 0x53425355
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
#define CSW_COMMAND_PASSED 0
#define CSW_COMMAND_FAILED 1
#define CSW_PHASE_ERROR 2
} __attribute__((packed));


#endif
