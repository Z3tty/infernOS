#ifndef USB_HUB_H
#define USB_HUB_H

#include "usb.h"
#include "list.h"

enum port_status_e {
  USB_PORT_DISCONNECTED = 0,
  USB_PORT_DISABLED,
  USB_PORT_ENABLED,
  USB_PORT_SUSPENDED
};

struct usb_hub_port {
  enum port_status_e port_status; /* Port status */
  struct usb_dev *udev;           /* Device attached to this port */
  int power_consumption;          /* Power consumption of this port */
};

#define MAX_USB_HUBS 16
#define MAX_PORTS_PER_HUB 16
struct usb_hub {
  struct list list;
  int port_num;       /* Number of port on this hub */
  struct usb_hub_port port[MAX_PORTS_PER_HUB];

  /* Host controller associated with this hub */
  void *hc;                       /* host controller state */
  struct usb_hc_ops *hc_ops;      /* host controller operations */

  /* This hub operations */
  struct usb_hub_ops *hub_ops;

  struct usb_hub *upstream_hub; 
  struct usb_hub *downstram_hub[MAX_PORTS_PER_HUB];

  int power_provided;
};


enum uhub_port_command_e {
  USB_PORT_RESET,
  USB_PORT_CLEAR_RESET,
  USB_PORT_ENABLE,
  USB_PORT_DISABLE,
  USB_PORT_CLEAR_SUSPEND,
};

/* Functions to export */
int usb_hub_static_init();
int usb_hub_register(struct usb_hub *);
void usb_hub_scan_ports();

/* USB hub operations */
struct usb_hub_ops {
  enum usb_speed_class_e (*port_speed)(struct usb_hub *, int port);
  enum port_status_e     (*port_status)(struct usb_hub *, int port);
  void                   (*port_command)(struct usb_hub *, int port, enum uhub_port_command_e);
};

#endif
