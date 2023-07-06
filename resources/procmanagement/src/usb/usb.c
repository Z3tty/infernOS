#include "../util.h"
#include "../sleep.h"
#include "uhci.h"
#include "pci.h"
#include "error.h"
#include "debug.h"
#include "allocator.h"
#include "list.h"

#include "usb_hid.h"
#include "usb_msd.h"
#include "usb_hub.h"

DEBUG_NAME("USB");

static struct list usb_drv_list_head;

/*
 * Builds and executes a control transfer
 * Control transfers consists of up to three stages:
 *  1. SETUP
 *  2. IN/OUT
 *  3. STATUS
 * It is always executed via the control pipe (pipe 0)
 */
#define USB_CREAD 1
#define USB_CWRITE 0
int control_transfer(struct usb_dev *udev, struct usb_dev_setup_request *req,
		     int dir, int size, char *buff)
{
	struct usb_pipe *pipe = &udev->pipe[0];
	int rc;

	/* Setup stage (resets toggle bit) */
	rc = udev->hc_ops->setup(pipe, req);
	if (rc != 8)
		return ERR_XFER;
	/* Read/write stage if necessary */
	if (size > 0) {
		if (dir == USB_CREAD)
			rc = udev->hc_ops->read(pipe, size, buff);
		else
			rc = udev->hc_ops->write(pipe, size, buff);

		if (rc != size)
			return ERR_XFER;
	}
	/* Status stage (always uses toggle bit 1) */
	pipe->toggle_bit = 1;
	if (dir == USB_CREAD)
		rc = udev->hc_ops->write(pipe, 0, NULL);
	else
		rc = udev->hc_ops->read(pipe, 0, NULL);

	if (rc != 0)
		return ERR_XFER;

	return 0;
}

/*
 * Reads a descriptor from USB device to a buffer
 * Executed over a USB device control pipe
 */
int load_descriptor(struct usb_dev *udev, int desc_type, int desc_index,
		    int size, char *buff)
{
	int rc;

	struct usb_dev_setup_request get_descriptor_req = {
		.bmRequestType = 0x80, /* direction device-host */
		.bRequest = GET_DESCRIPTOR,
		.wValue = (desc_type << 8) | desc_index,
		.wIndex = 0,
		.wLength = size};

	rc = control_transfer(udev, &get_descriptor_req, USB_CREAD, size, buff);

	return rc;
}

/*
 * Reads device configuration with index i to buffer
 */
static int load_configuration(struct usb_dev *udev, int conf_index,
			      char *conf_buff)
{
	struct usb_dev_configuration udc;
	int size;
	int rc;

	size = sizeof(struct usb_dev_configuration);
	rc = load_descriptor(udev, USB_CONFIGURATION_DESCRIPTOR, conf_index,
			     size, (char *)&udc);

	if (rc < 0)
		return ERR_XFER;

	if (udc.bDescriptorType != USB_CONFIGURATION_DESCRIPTOR)
		return ERR_XFER;

	rc = load_descriptor(udev, USB_CONFIGURATION_DESCRIPTOR, conf_index,
			     udc.wTotalLength, conf_buff);

	if (rc == 0)
		return udc.wTotalLength;

	return ERR_XFER;
}

/*
 * Finds descriptor of a given type and index bypassing mismatching
 * descriptors.
 */
static char *get_descriptor_ptr(int descriptor_type, int index, int conf_size,
				char *conf_buff)
{

	struct usb_generic_header *ugh;
	int scanned_conf_bytes;

	scanned_conf_bytes = 0;

	do {
		/* Read only descriptor header */
		ugh = (struct usb_generic_header *)conf_buff;

		/* Descriptor type matches? */
		if (ugh->bDescriptorType == descriptor_type) {
			if (index == 0)
				/* Return the pointer to this descriptor */
				return conf_buff;
			index--;
		}

		/* If not skip over this descriptor */
		conf_buff += ugh->bLength;
		scanned_conf_bytes += ugh->bLength;
	} while (scanned_conf_bytes < conf_size);

	return NULL;
}

/*
 * Transmits SETUP request to USB device
 */
static int send_request(struct usb_pipe *pipe, uint8_t rec, uint8_t req_type,
			uint16_t value, uint16_t index)
{
	struct usb_dev *udev = pipe->udev;
	int rc;

	struct usb_dev_setup_request req = {
		.bmRequestType = 0 | rec, /* direction host-device, recipient*/
		.bRequest = req_type,
		.wValue = value,
		.wIndex = index,
		.wLength = 0};

	rc = control_transfer(udev, &req, USB_CWRITE, 0, NULL);

	return rc;
}


static struct usb_dev_driver *usb_lookup_driver(struct usb_dev *udev)
{
	struct usb_driver_list *udl;

	LIST_FOR_EACH(&usb_drv_list_head, udl)
	if (udev->class_code == udl->udd->class_code
	    && udev->protocol_code == udl->udd->protocol_code) {
		/*
		 * We do not initialise the USB device driver before
		 * USB device was not initialised completely
		 */
		return udl->udd;
	}
	return NULL;
}

/*
 * This function configures USB device, this involves:
 *  - assigning configuration
 *  - assigning unique address
 *  - attaching USB device driver
 * Function assumes that there is only one device
 * responding to address 0 on this host controller.
 */
int usb_configure_device(struct usb_dev *udev, int address)
{
	/* Configuration and interface iterators */
	struct usb_dev_descriptor *udev_desc = NULL;
	struct usb_dev_configuration *udev_conf = NULL;
	struct usb_conf_interface *conf_ifc = NULL;
	struct usb_ifc_endpoint *ifc_ep = NULL;
	char buff[512];
	char *buff_ptr;
	char *ptr;
	int conf_size = 0;
	int conf_i;
	int ifc_i;
	int ep_i;
	int rc;
	/* USB device driver (if found) */
	struct usb_dev_driver *udd = NULL;
	/* USB pipes allocator */
	int pipe_i = 0;

	DEBUG("Configuring USB device");
	/* After reset all devices are reachable at address 0 */
	udev->address = 0;
	udev->op_state = USB_STATUS_NO_DRIVER;

	/* Setup control pipe, i.e., pipe 0 */
	bzero((char *)&udev->pipe[0], sizeof(struct usb_pipe));
	udev->pipe[0].max_packet_size =
		udev->usb_sc == USB_LS_DEV
			? 8
			: 64; /* Default max packet size for LS and FS devices
			       */
	udev->pipe[0].udev = udev;

	/* Read device descriptor */
	/* Function loads device descriptor to the buff */
	buff_ptr = buff;
	rc = load_descriptor(udev, USB_DEVICE_DESCRIPTOR,
			     0, /* device descriptor index is always 0 */
			     sizeof(struct usb_dev_descriptor), buff_ptr);

	DEBUG("reading device descriptor\t%s", DEBUG_STATUS(rc));
	udev_desc = (struct usb_dev_descriptor *)buff_ptr;
	buff_ptr += sizeof(struct usb_dev_descriptor);

	/* Update the maximum packet size for the control pipe */
	udev->pipe[0].max_packet_size = udev_desc->bMaxPacketSize0;
	DEBUG("MAX packet size %d", udev_desc->bMaxPacketSize0);

	/* Populate some general information about this USB device */
	udev->vendor = udev_desc->idVendor;
	udev->product = udev_desc->idProduct;
	udev->configurations_num = udev_desc->bNumConfigurations;

	/*
	 * Device has n configurations (usually only one), we scan
	 * through these configurations and choose one. We choose
	 * the configuration that can be handled by register drivers.
	 */
	for (conf_i = 0; conf_i < udev->configurations_num; conf_i++) {
		DEBUG("reading device configuration ");
		/*
		 * Function loads the complete device configuration with
		 * interfaces, endpoints descriptors and strings to buff.
		 */
		conf_size = load_configuration(udev, conf_i, buff_ptr);
		udev_conf = (struct usb_dev_configuration *)buff_ptr;

		DEBUG("interface count %d, reading..",
		      udev_conf->bNumInterfaces);

		/* Scan this configuration */
		for (ifc_i = 0; ifc_i < udev_conf->bNumInterfaces; ifc_i++) {
			ptr = get_descriptor_ptr(USB_INTERFACE_DESCRIPTOR,
						 ifc_i, conf_size, buff_ptr);
			conf_ifc = (struct usb_conf_interface *)ptr;

			/* If broken configuration then check the next one */
			if (conf_ifc == NULL)
				break;

			udev->class_code = conf_ifc->bInterfaceClass;
			udev->subclass_code = conf_ifc->bInterfaceSubClass;
			udev->protocol_code = conf_ifc->bInterfaceProtocol;
			udev->ifc_number = conf_ifc->bInterfaceNumber;

			DEBUG("  class: %02x, subclass: %02x", udev->class_code,
			      udev->subclass_code);

			udd = usb_lookup_driver(udev);

			/* If driver found then stop the scanning process */
			if (udd != NULL)
				goto exit_scan;
		}
	}

exit_scan:

	/* If no driver was found */
	if (udd == NULL) {
		DEBUG("device driver not found");
		return ERR_NO_DRIVER;
	}
	DEBUG("device driver found");
	udev->udd = udd;

	/* Scan and record endpoints related to this interface */
	pipe_i = 1; /* Pipe 0 already configured */
	for (ep_i = 0; ep_i < conf_ifc->bNumEndpoints; ep_i++) {
		ptr = get_descriptor_ptr(USB_ENDPOINT_DESCRIPTOR, ep_i,
					 conf_size, buff_ptr);
		ifc_ep = (struct usb_ifc_endpoint *)ptr;

		DEBUG("Looking up endpoint addr %x attr %x",
		      ifc_ep->bEndpointAddress, ifc_ep->bmAttributes);

		udev->pipe[pipe_i].udev = udev;
		udev->pipe[pipe_i].ep_address = ifc_ep->bEndpointAddress & 0x0f;
		udev->pipe[pipe_i].direction =
			(ifc_ep->bEndpointAddress & USB_PIPE_DIR_MASK) != 0
				? USB_PIPE_DIR_IN
				: USB_PIPE_DIR_OUT;
		udev->pipe[pipe_i].attributes = ifc_ep->bmAttributes;
		udev->pipe[pipe_i].max_packet_size = ifc_ep->wMaxPacketSize;
		udev->pipe[pipe_i].interval = ifc_ep->bInterval;
		udev->pipe[pipe_i].toggle_bit = 0;
		pipe_i++;
	}

	udev->pipe_count = pipe_i;

	/* Configure address  */
	rc = send_request(&udev->pipe[0], USB_RECIV_DEVICE, SET_ADDRESS,
			  (uint16_t)address, 0);
	DEBUG("Setting up address %d\t%s", address, DEBUG_STATUS(rc));
	udev->address = address;

	/* Choose configuration */
	rc = send_request(&udev->pipe[0], USB_RECIV_DEVICE, SET_CONFIGURATION,
			  udev_conf->bConfigurationValue, 0);
	DEBUG("Choosing configuration %d\t%s", udev_conf->bConfigurationValue,
	      DEBUG_STATUS(rc));

	/* Choose interface */
	rc = send_request(&udev->pipe[0], USB_RECIV_INTERFACE, SET_INTERFACE,
			  conf_ifc->bAlternateSetting,
			  conf_ifc->bInterfaceNumber);
	DEBUG("Choosing interface %d alt %d\t%s", conf_ifc->bInterfaceNumber,
	      conf_ifc->bAlternateSetting, DEBUG_STATUS(rc));

	udev->op_state = USB_STATUS_CONFIGURED;

	/* Initialise device driver for this USB device */
	udd->init(udev);

	return 0;
}

void usb_free_device(struct usb_dev *udev)
{
	udev->udd->free(udev);
	kfree(udev);
}

/*
 * Functions for USB device drivers
 */
int usb_read(struct usb_pipe *pipe, int size, char *data)
{
	struct usb_dev *udev = pipe->udev;

	return udev->hc_ops->read(pipe, size, data);
}

int usb_write(struct usb_pipe *pipe, int size, char *data)
{
	struct usb_dev *udev = pipe->udev;

	return udev->hc_ops->write(pipe, size, data);
}

int usb_setup(struct usb_pipe *pipe, struct usb_dev_setup_request *req, int dir,
	      int size, char *data)
{
	struct usb_dev *udev = pipe->udev;
	int rc;

	rc = control_transfer(udev, req, dir, size, data);

	return rc;
}

/*
 * Used to register USB drivers
 */
int usb_dev_driver_register(struct usb_dev_driver *udd)
{
	struct usb_driver_list *udl;

	udl = kzalloc(sizeof(struct usb_driver_list));
	if (udl == NULL)
		return ERR_NO_MEM;

	udl->udd = udd;

	LIST_LINK(&usb_drv_list_head, udl);

	return 0;
}

int usb_static_init()
{
	LIST_INIT(&usb_drv_list_head);

	allocator_init();
	usb_hub_static_init();
	usb_msd_static_init();
	usb_hid_static_init();
	uhci_static_init();
	pci_static_init();

	return 0;
}
