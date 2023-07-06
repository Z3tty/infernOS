#include "../util.h"
#include "usb.h"
#include "usb_hid.h"
#include "usb_keyboard.h"
#include "allocator.h"
#include "error.h"
#include "debug.h"

DEBUG_NAME("USB HID");

/*
 * Function to set the boot protocol for this HID device
 */
static int usb_hid_set_boot_proto(struct usb_hid_dev *hid)
{
	int rc;

	struct usb_dev_setup_request set_boot_proto = {
		.bmRequestType = 0x21,
		.bRequest = 0x0b, /* Set protocol request */
		.wValue = 0,
		.wIndex = hid->udev->ifc_number,
		.wLength = 0};

	rc = usb_setup(hid->pipe0, &set_boot_proto, USB_CWRITE, 0, NULL);

	if (rc != 0)
		return ERR_XFER;

	return 0;
}

static int usb_hid_set_idle(struct usb_hid_dev *hid)
{
	int rc;

	struct usb_dev_setup_request set_idle = {
		.bmRequestType = 0x21,
		.bRequest = 0x0a, /* Set idle request */
		.wValue = 5 << 8, /* Report every 5 * 4ms = 20ms*/
		.wIndex = hid->udev->ifc_number,
		.wLength = 0};

	rc = usb_setup(hid->pipe0, &set_idle, USB_CWRITE, 0, NULL);

	if (rc != 0)
		return ERR_XFER;

	return 0;
}

int usb_hid_init(struct usb_dev *udev)
{
	struct usb_hid_dev *hid;
	int rc;
	int pipe_i;

	hid = kzalloc(sizeof(struct usb_hid_dev));
	if (hid == NULL)
		return ERR_NO_MEM;

	hid->udev = udev;
	udev->driver_data = (void *)hid;

	DEBUG("Initialising USB human interface device");

	/* Assign interrupt pipe */
	for (pipe_i = 1; pipe_i < udev->pipe_count; pipe_i++)
		if ((udev->pipe[pipe_i].attributes == USB_PIPE_INTERRUPT)
		    && (udev->pipe[pipe_i].direction = USB_PIPE_DIR_IN))
			hid->interrupt_in = &udev->pipe[pipe_i];

	if (hid->interrupt_in == NULL)
		return ERR_NO_DRIVER;

	/* Assign control pipe */
	hid->pipe0 = &udev->pipe[0];

	/*
	 * Assign device driver
	 *
	 * At present we only support boot protocol
	 * for keyboards
	 */
	if (udev->subclass_code == 0x01) {
		/* Set Boot protocol */
		rc = usb_hid_set_boot_proto(hid);
		DEBUG("Setting boot protocol\t%s", DEBUG_STATUS(rc));
		rc -= usb_hid_set_idle(hid);
		DEBUG("Setting interrupt transmission idle\t%s\n",
		      DEBUG_STATUS(rc));

		if (rc < 0)
			return ERR_NO_DRIVER;

		return usb_keyboard_init(hid);
	} else
		return ERR_NO_DRIVER;

	return 0;
}

void usb_hid_free(struct usb_dev *udev)
{
	struct usb_hid_dev *hid;

	hid = (struct usb_hid_dev *)udev->driver_data;

	if (udev->subclass_code == 0x01) {
		usb_keyboard_free(hid);
		kfree(hid);
	}
}

static struct usb_dev_driver usb_human_interface_device = {
	.class_code = 0x03,    /* HID class     */
	.subclass_code = 0x01, /* Boot protocol */
	.protocol_code = 0x01, /* Keyboard      */
	.init = usb_hid_init,
	.free = usb_hid_free};

void usb_hid_static_init()
{
	usb_dev_driver_register(&usb_human_interface_device);
}
