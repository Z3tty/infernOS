#ifndef USB_HID_H
#define USB_HID_H

#include "usb.h"

struct usb_hid_dev {
	struct usb_dev *udev;

	struct usb_pipe *pipe0; /* Control pipe */
	struct usb_pipe *interrupt_in;

	void *driver_data;
};

void usb_hid_static_init();

#endif
