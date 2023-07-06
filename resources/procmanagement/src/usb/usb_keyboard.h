#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include "usb_hid.h"
#include "../util.h"

int usb_keyboard_init(struct usb_hid_dev *);
void usb_keyboard_free(struct usb_hid_dev *);

#define BOOT_REPORT_SIZE 8

struct usb_keyboard {
	struct usb_hid_dev *hid;
	uint8_t report[BOOT_REPORT_SIZE];
	uint8_t last_report[BOOT_REPORT_SIZE];

	struct usb_interrupt usb_kbd_interrupt;

	int caps_lock_key;
	int num_lock_key;
	int shift_key;
	int ctrl_key;
	int alt_key;
	int mbox; /* mbox number to send keycodes to */
};

#endif
