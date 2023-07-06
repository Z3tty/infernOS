#include "usb.h"
#include "usb_hub.h"
#include "uhci.h"
#include "allocator.h"
#include "debug.h"
#include "list.h"
#include "../sleep.h"

DEBUG_NAME("USB HUB");

static struct list uhub_list_head;

int usb_hub_static_init() {
  LIST_INIT(&uhub_list_head);

  return 0;
}

/* 
 * USB HUB register functions
 *
 * Registered HUBs are scanned every 1s for the port
 * status change
 */
int usb_hub_register(struct usb_hub *uhub) {
  LIST_LINK(&uhub_list_head, uhub)

  return 0;
}

/* 
 * Function scans all ports of all registered hubs
 * in the system.If a new device is found it is
 * initialised. 
 *
 * USB devices are initialised sequentially one by one,
 * thus only one device at a moment responses to 
 * the default address 0.
 */
void usb_hub_scan_ports() {
  struct usb_hub *uhub_i;
  struct usb_dev *udev;
  enum port_status_e current_ps;    /* Current port status */
  enum port_status_e previous_ps;   /* Previous port status */
  int port_i;
  int address;

  /* Iterate over all registered hubs */
  LIST_FOR_EACH(&uhub_list_head, uhub_i) 
    /* Iterate over all ports of the hub */
    for (port_i = 0; port_i < uhub_i->port_num; port_i++) {
      current_ps = uhub_i->hub_ops->port_status(uhub_i, port_i);
      previous_ps = uhub_i->port[port_i].port_status;
      /* Update port status */
      uhub_i->port[port_i].port_status = current_ps;

      /* If a device has been disconnected */
      if (previous_ps != USB_PORT_DISCONNECTED && 
          current_ps == USB_PORT_DISCONNECTED) {
        udev = uhub_i->port[port_i].udev;

        if (udev != NULL) {
          usb_free_device(udev);
          kfree(udev);
          uhub_i->port[port_i].udev = NULL;
        }

        continue;
      }

      /* If a device has been connected */
      if (previous_ps == USB_PORT_DISCONNECTED &&
          current_ps != USB_PORT_DISCONNECTED) {
        DEBUG("found a USB device, port status %d", (int)current_ps);

       /* Build new USB device */
        udev = kzalloc(sizeof(struct usb_dev));
        if (udev == NULL)
          break;

        udev->hc = uhub_i->hc;    /* USB device is reachable via
                                   * the same host controller as this hub
                                   */
        udev->hc_ops = uhub_i->hc_ops;

        udev->hub = uhub_i;
        udev->port = port_i;
        uhub_i->port[port_i].udev = udev;

        /* Perform standard reset routine before device initialisation */
        DEBUG("resetting...");
        uhub_i->hub_ops->port_command(uhub_i, port_i, USB_PORT_CLEAR_SUSPEND);
        uhub_i->hub_ops->port_command(uhub_i, port_i, USB_PORT_RESET);
        ms_delay(30);
        uhub_i->hub_ops->port_command(uhub_i, port_i, USB_PORT_CLEAR_RESET);
        ms_delay(1);
        uhub_i->hub_ops->port_command(uhub_i, port_i, USB_PORT_ENABLE);

        address = (int)uhub_i->hc_ops->get_next_addr(udev);
        /* Check USB device speed */
        udev->usb_sc = uhub_i->hub_ops->port_speed(uhub_i, port_i);
        DEBUG("port status %d device speed %d address %d",
            (int)uhub_i->hub_ops->port_status(uhub_i, port_i),
            (int)udev->usb_sc, address);

         /* Continue with USB configuration */
        usb_configure_device(udev, address);
      }
    }
  
}

