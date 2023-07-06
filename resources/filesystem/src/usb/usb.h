#ifndef USB_H
#define USB_H

#include "../util.h"
#include "list.h"

void usb_mass_storage_init();

enum usb_speed_class_e {
  USB_LS_DEV,
  USB_FS_DEV,
  USB_HS_DEV
};

typedef enum usb_speed_class_e usb_speed_class;

enum usb_device_state_e {
  USB_STATUS_ATTACHED,
  USB_STATUS_POWERED,
  USB_STATUS_DEFAULT,
  USB_STATUS_ADDRESS,
  USB_STATUS_CONFIGURED,
  USB_STATUS_SUSPENDED,
  USB_STATUS_NO_DRIVER
};

typedef enum usb_device_state_e usb_device_state;

struct usb_dev;
struct usb_hc_ops;
struct usb_dev_driver;

struct usb_pipe {
  struct usb_dev *udev;       /* USB device that owns this pipe */
  uint8_t ep_address;        /* Pipe endpoint address */
  uint8_t direction;         /* Pipe traffic direction */
  uint8_t attributes;        /* Pipe attributes like: tranfer, synch, and usage type */
  uint16_t max_packet_size;  /* Maximum packet size for this endpoint */
  uint8_t interval;          /* Interval for polling endpoint for data transfers */ 
  int toggle_bit;            /* Used for USB transmission sych */
  void *xfer;                /* Transfer related to this pipe, using void type 
                                since there may be various transfer types depending
                                on host controler implementation */
};

#define USB_PIPE_DATA_BULK 0x02
#define USB_PIPE_INTERRUPT 0x03
#define USB_PIPE_DIR_MASK 0x80
#define USB_PIPE_DIR_IN 0x01
#define USB_PIPE_DIR_OUT 0x00

/* 
 * USB device structure 
 */
struct usb_dev {
  /* USB device attachment */
  struct usb_hub *hub;
  int port;                  /* Port on a give USB hub */

  /* Host controler interface */
  struct usb_hc_ops *hc_ops;
  void *hc;
  uint8_t address;           /* Logical address of this USB device 
                              * on a give USB bus                 */

  /* General info */
  int vendor;
  int product;
  int configurations_num;

  usb_speed_class usb_sc;    /* USB device speed                  */

  /* Current interface settings */
  uint8_t class_code;
  uint8_t subclass_code;
  uint8_t protocol_code;
  uint8_t ifc_number;

  /* USB device drive related data */
  struct usb_dev_driver *udd;
  void *driver_data;

  usb_device_state op_state; /* describes the current USB device state */

  int pipe_count;
#define MAX_USB_PIPES 16
  struct usb_pipe pipe[MAX_USB_PIPES];  /* This usb device communication pipes */
};

/* 
 * Host controler operations 
 */
struct usb_dev_setup_request;
struct usb_interrupt;
struct usb_hc_ops {
  int (*read)(struct usb_pipe *, int size, char *data);
  int (*write)(struct usb_pipe *, int size, char *data);
  int (*setup)(struct usb_pipe *, struct usb_dev_setup_request *data);
  uint8_t (*get_next_addr)(struct usb_dev *);
  int (*register_interrupt_h)(struct usb_interrupt *);
  int (*remove_interrupt_h)(struct usb_interrupt *);
};

/*
 * Device driver struct and interface to register 
 */

struct usb_dev_driver {
  uint8_t class_code;
  uint8_t subclass_code;
  uint8_t protocol_code;
  int (*init)(struct usb_dev *);
  void (*free)(struct usb_dev *);
};

struct usb_driver_list {
  struct list list;
  struct usb_dev_driver *udd;
};

/*
 * Function prototypes for USB device drivers 
 */
int usb_read(struct usb_pipe *pipe, int size, char *data);
int usb_write(struct usb_pipe *pipe, int size, char *data);
/* Control direction */
#define USB_CREAD 1
#define USB_CWRITE 0
int usb_setup(struct usb_pipe *pipe, struct usb_dev_setup_request *req,
              int dir, int size, char *data);

/* 
 * USB interrupts 
 */
struct usb_interrupt {
  struct usb_pipe *pipe;
  char *buff;
  int size;
  int freq;
  /* Interrupt handler related data */
  void (*interrupt_h)(void *data);
  void *data;
};

int usb_dev_driver_register(struct usb_dev_driver *udd);
int usb_static_init();
int usb_configure_device(struct usb_dev *, int );
void usb_free_device(struct usb_dev *);

enum bRequest_e {
  GET_STATUS = 0,
  CLEAR_FEATURE = 1,
  SET_FEATURE = 3,
  SET_ADDRESS = 5,
  GET_DESCRIPTOR = 6,
  SET_DESCRIPTOR = 7,
  GET_CONFIGURATION = 8,
  SET_CONFIGURATION = 9,
  GET_INTERFACE = 10,
  SET_INTERFACE = 11,
  SYNCH_FRAME = 12
};

#define USB_RECIV_DEVICE 0
#define USB_RECIV_INTERFACE 1
#define USB_RECIV_ENDPOINT 2

/* Feature selectors */
#define USB_FEATURE_ENDPOINT_HALT 0
#define USB_FEATURE_DEVICE_REMOTE_WAKEUP 1
#define USB_FEATURE_TEST_MODE 2

/* 
 * USB descriptor request types
 *
 * Note: USB device must support only requests 
 * for the first three types of descriptors
 */
#define USB_DEVICE_DESCRIPTOR  0x01
#define USB_CONFIGURATION_DESCRIPTOR 0x02
#define USB_STRING_DESCRIPTOR 0x03
#define USB_INTERFACE_DESCRIPTOR 0x04
#define USB_ENDPOINT_DESCRIPTOR 0x05

struct usb_configuration_header {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
} __attribute__((packed));

struct usb_generic_header {
  uint8_t bLength;
  uint8_t bDescriptorType;
} __attribute__((packed));

struct usb_dev_setup_request {
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} __attribute__((packed));

struct usb_dev_descriptor {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdUSB;
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
  uint8_t bMaxPacketSize0;
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t iManufacturer;
  uint8_t iProduct;
  uint8_t iSerialNumber;
  uint8_t bNumConfigurations;
} __attribute__((packed));

struct usb_dev_configuration {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
  uint8_t bNumInterfaces;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower;
} __attribute__((packed));

struct usb_conf_interface {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndpoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
} __attribute__((packed));

struct usb_ifc_endpoint {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
} __attribute__((packed));

#endif
