#include "usb_keyboard.h"
#include "allocator.h"
#include "usb_hid.h"
#include "../keyboard.h"
#include "../util.h"
#include "../interrupt.h"
#include "error.h"
#include "debug.h"

DEBUG_NAME("USB KBD");

struct key_map {
	uint8_t key;
	uint8_t key1;
	void (*func0)(struct usb_keyboard *, uint8_t);
	void (*func1)(struct usb_keyboard *, uint8_t);
};

static void normal_h(struct usb_keyboard *, uint8_t);
static void escape_h(struct usb_keyboard *, uint8_t);
static void num_lock_h(struct usb_keyboard *, uint8_t);
static void caps_lock_h(struct usb_keyboard *, uint8_t);
static void func_h(struct usb_keyboard *, uint8_t);
static void usb_keyboard_put_char(char);

static const struct key_map usb_keyboard[] = {
	{0x00, 0x00, NULL, NULL},	 // 0x00 Reserved (no event indicated)
	{0x00, 0x00, NULL, NULL},	 // 0x01 Keyboard ErrorRollOver
	{0x00, 0x00, NULL, NULL},	 // 0x02 Keyboard POSTFail
	{0x00, 0x00, NULL, NULL},	 // 0x03 Keyboard ErrorUndefined
	{0x61, 0x41, normal_h, normal_h}, // 0x04 Keyboard a and A
	{0x62, 0x42, normal_h, normal_h}, // 0x05 Keyboard b and B
	{0x63, 0x43, normal_h, normal_h}, // 0x06 Keyboard c and C
	{0x64, 0x44, normal_h, normal_h}, // 0x07 Keyboard d and D
	{0x65, 0x45, normal_h, normal_h}, // 0x08 Keyboard e and E
	{0x66, 0x46, normal_h, normal_h}, // 0x09 Keyboard f and F
	{0x67, 0x47, normal_h, normal_h}, // 0x0A Keyboard g and G
	{0x68, 0x48, normal_h, normal_h}, // 0x0B Keyboard h and H
	{0x69, 0x49, normal_h, normal_h}, // 0x0C Keyboard i and I
	{0x6A, 0x4A, normal_h, normal_h}, // 0x0D Keyboard j and J
	{0x6B, 0x4B, normal_h, normal_h}, // 0x0E Keyboard k and K
	{0x6C, 0x4C, normal_h, normal_h}, // 0x0F Keyboard l and L
	{0x6D, 0x4D, normal_h, normal_h}, // 0x10 Keyboard m and M
	{0x6E, 0x4E, normal_h, normal_h}, // 0x11 Keyboard n and N
	{0x6F, 0x4F, normal_h, normal_h}, // 0x12 Keyboard o and O
	{0x70, 0x50, normal_h, normal_h}, // 0x13 Keyboard p and P
	{0x71, 0x51, normal_h, normal_h}, // 0x14 Keyboard q and Q
	{0x72, 0x52, normal_h, normal_h}, // 0x15 Keyboard r and R
	{0x73, 0x53, normal_h, normal_h}, // 0x16 Keyboard s and S
	{0x74, 0x54, normal_h, normal_h}, // 0x17 Keyboard t and T
	{0x75, 0x55, normal_h, normal_h}, // 0x18 Keyboard u and U
	{0x76, 0x56, normal_h, normal_h}, // 0x19 Keyboard v and V
	{0x77, 0x57, normal_h, normal_h}, // 0x1A Keyboard w and W
	{0x78, 0x58, normal_h, normal_h}, // 0x1B Keyboard x and X
	{0x79, 0x59, normal_h, normal_h}, // 0x1C Keyboard y and Y
	{0x7A, 0x5A, normal_h, normal_h}, // 0x1D Keyboard z and Z
	{0x31, 0x21, normal_h, normal_h}, // 0x1E Keyboard 1 and !
	{0x32, 0x40, normal_h, normal_h}, // 0x1F Keyboard 2 and @
	{0x33, 0x23, normal_h, normal_h}, // 0x20 Keyboard 3 and #
	{0x34, 0x24, normal_h, normal_h}, // 0x21 Keyboard 4 and $
	{0x35, 0x25, normal_h, normal_h}, // 0x22 Keyboard 5 and %
	{0x36, 0x5E, normal_h, normal_h}, // 0x23 Keyboard 6 and ^
	{0x37, 0x26, normal_h, normal_h}, // 0x24 Keyboard 7 and &
	{0x38, 0x52, normal_h, normal_h}, // 0x25 Keyboard 8 and *
	{0x39, 0x28, normal_h, normal_h}, // 0x26 Keyboard 9 and (
	{0x30, 0x29, normal_h, normal_h}, // 0x27 Keyboard 0 and )
	{0x0D, 0x0D, normal_h, normal_h}, // 0x28 Keyboard Return (ENTER)
	{0x1B, 0x1B, normal_h, normal_h}, // 0x29 Keyboard ESCAPE
	{0x08, 0x08, normal_h, normal_h}, // 0x2A Keyboard DELETE (Backspace)
	{0x09, 0x09, normal_h, normal_h}, // 0x2B Keyboard Tab
	{0x20, 0x20, normal_h, normal_h}, // 0x2C Keyboard Spacebar
	{0x2D, 0x5F, normal_h, normal_h}, // 0x2D Keyboard - and (underscore)
	{0x3D, 0x2B, normal_h, normal_h}, // 0x2E Keyboard = and +
	{0x5B, 0x7B, normal_h, normal_h}, // 0x2F Keyboard [ and {
	{0x5D, 0x7D, normal_h, normal_h}, // 0x30 Keyboard ] and }
	{0x5C, 0x7C, normal_h, normal_h}, // 0x31 Keyboard \ and |
	{0x23, 0x7E, normal_h, normal_h}, // 0x32 Keyboard Non-US # and ~
	{0x3B, 0x3A, normal_h, normal_h}, // 0x33 Keyboard ; and :
	{0x27, 0x22, normal_h, normal_h}, // 0x34 Keyboard ' and "
	{0x60, 0x7E, normal_h,
	 normal_h}, // 0x35 Keyboard Grave Accent and Tilde
	{0x2C, 0x3C, normal_h, normal_h},       // 0x36 Keyboard , and <
	{0x2E, 0x3E, normal_h, normal_h},       // 0x37 Keyboard . and >
	{0x2F, 0x3F, normal_h, normal_h},       // 0x38 Keyboard / and ?
	{0x00, 0x00, caps_lock_h, caps_lock_h}, // 0x39 Keyboard Caps Lock
	{0x01, 0x00, func_h, func_h},		// 0x3A Keyboard F1
	{0x02, 0x00, func_h, func_h},		// 0x3B Keyboard F2
	{0x03, 0x00, func_h, func_h},		// 0x3C Keyboard F3
	{0x04, 0x00, func_h, func_h},		// 0x3D Keyboard F4
	{0x05, 0x00, func_h, func_h},		// 0x3E Keyboard F5
	{0x06, 0x00, func_h, func_h},		// 0x3F Keyboard F6
	{0x07, 0x00, func_h, func_h},		// 0x40 Keyboard F7
	{0x08, 0x00, func_h, func_h},		// 0x41 Keyboard F8
	{0x09, 0x00, func_h, func_h},		// 0x42 Keyboard F9
	{0x0A, 0x00, func_h, func_h},		// 0x43 Keyboard F10
	{0x0B, 0x00, func_h, func_h},		// 0x44 Keyboard F11
	{0x0C, 0x00, func_h, func_h},		// 0x45 Keyboard F12
	{0x00, 0x00, NULL, NULL},		// 0x46 Keyboard PrintScreen
	{0x00, 0x00, NULL, NULL},		// 0x47 Keyboard Scroll Lock
	{0x00, 0x00, NULL, NULL},		// 0x48 Keyboard Pause
	{0x00, 0x00, NULL, NULL},		// 0x49 Keyboard Insert
	{0x48, 0x48, escape_h, escape_h},       // 0x4A Keyboard Home
	{0x00, 0x00, NULL, NULL},		// 0x4B Keyboard PageUp
	{0x00, 0x00, NULL, NULL},		// 0x4C Keyboard Delete Forward
	{0x46, 0x46, escape_h, escape_h},       // 0x4D Keyboard End
	{0x00, 0x00, NULL, NULL},		// 0x4E Keyboard PageDown
	{0x43, 0x43, escape_h, escape_h},       // 0x4F Keyboard RightArrow
	{0x44, 0x44, escape_h, escape_h},       // 0x50 Keyboard LeftArrow
	{0x42, 0x42, escape_h, escape_h},       // 0x51 Keyboard DownArrow
	{0x41, 0x41, escape_h, escape_h},       // 0x52 Keyboard UpArrow
	{0x00, 0x00, num_lock_h, num_lock_h}, // 0x53 Keypad Num Lock and Clear
	{0x2F, 0x2F, normal_h, normal_h},     // 0x54 Keypad /
	{0x2A, 0x2A, normal_h, normal_h},     // 0x55 Keypad *
	{0x2D, 0x2D, normal_h, normal_h},     // 0x56 Keypad -
	{0x2B, 0x2B, normal_h, normal_h},     // 0x57 Keypad +
	{0x0D, 0x0D, normal_h, normal_h},     // 0x58 Keypad ENTER
	{0x31, 0x46, normal_h, escape_h},     // 0x59 Keypad 1 and End
	{0x32, 0x42, normal_h, escape_h},     // 0x5A Keypad 2 and Down Arrow
	{0x33, 0x00, normal_h, NULL},	 // 0x5B Keypad 3 and PageDn
	{0x34, 0x44, normal_h, escape_h},     // 0x5C Keypad 4 and Left Arrow
	{0x35, 0x35, normal_h, normal_h},     // 0x5D Keypad 5
	{0x36, 0x43, normal_h, escape_h},     // 0x5E Keypad 6 and Right Arrow
	{0x37, 0x48, normal_h, escape_h},     // 0x5F Keypad 7 and Home
	{0x38, 0x41, normal_h, escape_h},     // 0x60 Keypad 8 and Up Arrow
	{0x39, 0x00, normal_h, NULL},	 // 0x61 Keypad 9 and PageUp
	{0x30, 0x00, normal_h, NULL},	 // 0x62 Keypad 0 and Insert
	{0x2E, 0x00, normal_h, NULL},	 // 0x63 Keypad . and Delete
	{0x00, 0x00, NULL, NULL},	     // 0x64 Keyboard Non-US \ and |
	{0x00, 0x00, NULL, NULL},	     // 0x65 Keyboard Application
	{0x00, 0x00, NULL, NULL},	     // 0x66 Keyboard Power
	{0x3D, 0x3D, normal_h, normal_h},     // 0x67 Keypad =
	{0x00, 0x00, NULL, NULL},	     // 0x68 Keyboard F13
	{0x00, 0x00, NULL, NULL},	     // 0x69 Keyboard F14
	{0x00, 0x00, NULL, NULL},	     // 0x6A Keyboard F15
	{0x00, 0x00, NULL, NULL},	     // 0x6B Keyboard F16
	{0x00, 0x00, NULL, NULL},	     // 0x6C Keyboard F17
	{0x00, 0x00, NULL, NULL},	     // 0x6D Keyboard F18
	{0x00, 0x00, NULL, NULL},	     // 0x6E Keyboard F19
	{0x00, 0x00, NULL, NULL},	     // 0x6F Keyboard F20
	{0x00, 0x00, NULL, NULL},	     // 0x70 Keyboard F21
	{0x00, 0x00, NULL, NULL},	     // 0x71 Keyboard F22
	{0x00, 0x00, NULL, NULL},	     // 0x72 Keyboard F23
	{0x00, 0x00, NULL, NULL},	     // 0x73 Keyboard F24
	{0x00, 0x00, NULL, NULL},	     // 0x74 Keyboard Execute
	{0x00, 0x00, NULL, NULL},	     // 0x75 Keyboard Help
	{0x00, 0x00, NULL, NULL},	     // 0x76 Keyboard Menu
	{0x00, 0x00, NULL, NULL},	     // 0x77 Keyboard Select
	{0x00, 0x00, NULL, NULL},	     // 0x78 Keyboard Stop
	{0x00, 0x00, NULL, NULL},	     // 0x79 Keyboard Again
	{0x00, 0x00, NULL, NULL},	     // 0x7A Keyboard Undo
	{0x00, 0x00, NULL, NULL},	     // 0x7B Keyboard Cut
	{0x00, 0x00, NULL, NULL},	     // 0x7C Keyboard Copy
	{0x00, 0x00, NULL, NULL},	     // 0x7D Keyboard Paste
	{0x00, 0x00, NULL, NULL},	     // 0x7E Keyboard Find
	{0x00, 0x00, NULL, NULL},	     // 0x7F Keyboard Mute
	{0x00, 0x00, NULL, NULL},	     // 0x80 Keyboard Volume Up
	{0x00, 0x00, NULL, NULL},	     // 0x81 Keyboard Volume Down
	{0x00, 0x00, NULL, NULL},	     // 0x82 Keyboard Locking Caps Lock
	{0x00, 0x00, num_lock_h, num_lock_h}, // 0x83 Keyboard Locking Num Lock
	{0x00, 0x00, NULL, NULL},	 // 0x84 Keyboard Locking Scroll Lock
	{0x27, 0x27, normal_h, normal_h}, // 0x85 Keypad Comma
	{0x3D, 0x3D, normal_h, normal_h}, // 0x86 Keypad Equal Sign
	{0x00, 0x00, NULL, NULL},	 // 0x9A Keyboard SysReq/Attention
	{0x00, 0x00, NULL, NULL},	 // 0x9B Keyboard Cancel
	{0x00, 0x00, NULL, NULL},	 // 0x9C Keyboard Clear
	{0x00, 0x00, NULL, NULL},	 // 0x9D Keyboard Prior
	{0x00, 0x00, NULL, NULL},	 // 0x9E Keyboard Return
	{0x00, 0x00, NULL, NULL},	 // 0x9F Keyboard Separator
	{0x00, 0x00, NULL, NULL},	 // 0xA2 Keyboard Clear/Again
	{0x00, 0x00, NULL, NULL},	 // 0xA3 Keyboard CrSel/Props
	{0x00, 0x00, NULL, NULL},	 // 0xA4 Keyboard ExSel
	{0x00, 0x00, NULL, NULL},	 // 0xE0 Keyboard LeftControl
	{0x00, 0x00, NULL, NULL},	 // 0xE1 Keyboard LeftShift
	{0x00, 0x00, NULL, NULL},	 // 0xE2 Keyboard LeftAlt
	{0x00, 0x00, NULL, NULL},	 // 0xE3 Keyboard Left GUI
	{0x00, 0x00, NULL, NULL},	 // 0xE4 Keyboard RightControl
	{0x00, 0x00, NULL, NULL},	 // 0xE5 Keyboard RightShift
	{0x00, 0x00, NULL, NULL},	 // 0xE6 Keyboard RightAlt
	{0x00, 0x00, NULL, NULL},	 // 0xE7 Keyboard Right GUI
};

/* Modifier bits (the first byte of Report) */
#define LEFT_CTRL (1 << 0)
#define LEFT_SHIFT (1 << 1)
#define LEFT_ALT (1 << 2)
#define LEFT_GUI (1 << 3)
#define RIGHT_CTRL (1 << 4)
#define RIGHT_SHIFT (1 << 5)
#define RIGHT_ALT (1 << 6)
#define RIGHT_GUI (1 << 7)

static void normal_h(struct usb_keyboard *uk, uint8_t key_code)
{
	char c;

	if (((uk->shift_key + uk->caps_lock_key) % 2) == 0)
		c = usb_keyboard[key_code].key;
	else
		c = usb_keyboard[key_code].key1;

	usb_keyboard_put_char(c);
}

static void escape_h(struct usb_keyboard *uk, uint8_t key_code)
{
	/* Put escape character */
	usb_keyboard_put_char(0x1B);
	usb_keyboard_put_char(']');
	usb_keyboard_put_char(usb_keyboard[key_code].key1);
	return;
}

static void num_lock_h(struct usb_keyboard *uk, uint8_t key_code)
{
	uk->num_lock_key = 1 - uk->num_lock_key;
}

static void caps_lock_h(struct usb_keyboard *uk, uint8_t key_code)
{
	uk->caps_lock_key = 1 - uk->caps_lock_key;
}

static void func_h(struct usb_keyboard *uk, uint8_t key_code)
{
	return;
}

static void usb_kbd_ihandler(void *data)
{
	struct usb_keyboard *uk = (struct usb_keyboard *)data;
	void (*func)(struct usb_keyboard *, uint8_t);
	int key_pressed;
	int key_code;
	int i, j;
	uint8_t *c;

	/* Parse the modifier bits located in the first byte */
	uk->shift_key =
		(uk->report[0] & (LEFT_SHIFT | RIGHT_SHIFT)) != 0 ? 1 : 0;
	uk->alt_key = (uk->report[0] & (LEFT_ALT | RIGHT_ALT)) != 0 ? 1 : 0;
	uk->ctrl_key = (uk->report[0] & (LEFT_CTRL | RIGHT_CTRL)) != 0 ? 1 : 0;

	for (i = 2; i < BOOT_REPORT_SIZE; i++) {
		/* Find new keys */
		key_code = uk->report[i];
		key_pressed = 1;
		for (j = 2; j < BOOT_REPORT_SIZE; j++)
			/* If the key was in the previous set skip over it */
			if (key_code == uk->last_report[j]) {
				key_pressed = 0;
				break;
			}
		if (key_pressed && key_code < 0x64) {
			/* Keyboard keys use only funcion 0 */
			func = usb_keyboard[key_code].func0;
			/* Keypad keys use also function 1 when NUM LOCK is
			 * released */
			if (key_code >= 0x54 && key_code <= 0x63)
				if (!uk->num_lock_key)
					func = usb_keyboard[key_code].func1;

			if (func != NULL)
				func(uk, key_code);
		}
	}

	for (i = 0; i < BOOT_REPORT_SIZE; i++)
		uk->last_report[i] = uk->report[i];
	c = uk->report;
}

static void usb_keyboard_put_char(char c)
{
	struct character char_s;

	char_s.character = c;
	/* Call the original putchar function in keyboard.c */
	putchar(&char_s);
}

static void usb_keyboard_reg_interrupt(struct usb_keyboard *uk)
{
	struct usb_dev *udev = uk->hid->udev;
	struct usb_interrupt *ui;

	/*
	 * Build interrupt interface
	 */
	ui = &uk->usb_kbd_interrupt;

	ui->pipe = uk->hid->interrupt_in; /* interrupt pipe */
	ui->buff = (char *)uk->report;    /* buffer to read interrupt data to    */
	ui->size = BOOT_REPORT_SIZE;      /* data size      */
	ui->freq = 1; /* interrupt frequency (not implemented) */
	ui->interrupt_h =
		usb_kbd_ihandler; /* function to be called on interrupt    */
	ui->data = uk; /* data to be passed to the usb_kbd_ihandler function */

	udev->hc_ops->register_interrupt_h(ui);
}

/*
 * Initialisation function allocates USB keyboard
 * structure and registers interrupts for the keyboard
 */
int usb_keyboard_init(struct usb_hid_dev *hid)
{
	struct usb_keyboard *uk;
	DEBUG("Initialising boot keyboard");

	uk = kzalloc(sizeof(struct usb_keyboard));
	if (uk == NULL)
		return ERR_NO_MEM;

	uk->hid = hid;
	hid->driver_data = (void *)uk;

	usb_keyboard_reg_interrupt(uk);

	return 0;
}

void usb_keyboard_free(struct usb_hid_dev *hid)
{
	struct usb_keyboard *uk;
	struct usb_dev *udev;

	uk = (struct usb_keyboard *)hid->driver_data;
	udev = hid->udev;

	udev->hc_ops->remove_interrupt_h(&uk->usb_kbd_interrupt);

	kfree(uk);
}
