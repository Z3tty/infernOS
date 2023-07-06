#include "../util.h"
#include "uhci_pci.h"
#include "ehci_pci.h"
#include "usb.h"
#include "usb_msd.h"
#include "allocator.h"
#include "scsi.h"
#include "error.h"
#include "debug.h"

DEBUG_NAME("USB MSD");

/* Function prototypes */
static int usb_msd_read(void *, int, char *, int, char *);
static int usb_msd_write(void *, int, char *, int, char *);

/*
 * Function resets USB Mass Storage Device 
 */
static int usb_msd_reset(struct usb_msd_dev *msd) {
  int rc;

  struct usb_dev_setup_request reset_dev = {
    .bmRequestType = 0x21,
    .bRequest = 0xff,
    .wValue = 0,
    .wIndex = msd->udev->ifc_number,
    .wLength = 0
  };

  rc = usb_setup(msd->pipe0, &reset_dev, USB_CWRITE, 0, NULL);
  
  if (rc != 0)
    return ERR_XFER;

  return 0;
}

/* 
 * Function clears endpoint HALT feature 
 */
static int usb_msd_clear_halt(struct usb_pipe *pipe) {
  int rc;
  struct usb_dev_setup_request clear_halt = {
    .bmRequestType = 0 | USB_RECIV_ENDPOINT,
    .bRequest = CLEAR_FEATURE,
    .wValue = USB_FEATURE_ENDPOINT_HALT,
    .wIndex = ((pipe->direction == USB_PIPE_DIR_IN) ? 0x80 : 0x00) | 
      pipe->ep_address,
    .wLength = 0
  };

  rc = usb_setup(pipe, &clear_halt, USB_CWRITE, 0, NULL);
  
  if (rc != 0)
    return ERR_XFER;

  return 0;
}

/*
 * Function reads the number of logical units supported
 * by this device. 
 *
 * Abbrev.: LUN - logical unit number
 */

static int usb_msd_get_max_lun(struct usb_msd_dev *msd) {
  char max_lun;
  int rc;

  struct usb_dev_setup_request get_max_lun = {
    .bmRequestType = 0xa1,
    .bRequest = 0xfe,
    .wValue = 0,
    .wIndex = msd->udev->ifc_number,
    .wLength = 1
  };

  /* 
   * Device should transfer 1 byte representing 
   * the maximum number of logical units 
   */
  rc = usb_setup(msd->pipe0, &get_max_lun, USB_CREAD, 1, &max_lun);
  
  /* Devices that do not support this command may STALL */
  if (rc != 0)
    return 0;

  return (int)max_lun;
}

int usb_msd_init(struct usb_dev *udev) {
  struct usb_msd_dev *usb_msd;
  struct scsi_ifc scsi_if;
  int pipe_i;

  usb_msd = kzalloc(sizeof(struct usb_msd_dev));

  if (usb_msd == NULL)
    return ERR_NO_MEM;

  usb_msd->udev = udev;
  udev->driver_data = (void *)usb_msd;

  DEBUG("Initialising USB mass storage device");

  /* 
   * Scan through available pipes and assign 
   * bulk-in and bulk-out pipes 
   */
  for (pipe_i = 1; pipe_i < udev->pipe_count; pipe_i++) {
    if (udev->pipe[pipe_i].attributes == USB_PIPE_DATA_BULK) {
      if (udev->pipe[pipe_i].direction == USB_PIPE_DIR_IN)
        usb_msd->bulk_in = &udev->pipe[pipe_i];
      else
        usb_msd->bulk_out = &udev->pipe[pipe_i];
    }
  }

  /* If no suitable bulk-in and bulk-out pipe was found */
  if ((usb_msd->bulk_in == NULL) || (usb_msd->bulk_out == NULL))
    return ERR_NO_DRIVER;

  /* Assigin control pipe */
  usb_msd->pipe0 = &udev->pipe[0];

  /* Read maximum number of logical units */
  usb_msd->max_lun = usb_msd_get_max_lun(usb_msd);

  /* Reset the device */
  usb_msd_reset(usb_msd);

  /* 
   * Assign mass storage device driver 
   *
   * At present we support only SCSI interface for
   * Mass Storage Devices 
   */
  if (udev->subclass_code == 0x06) {
    DEBUG("found mass storage device");
    DEBUG("max logical units (LUN): %d", usb_msd->max_lun);

    scsi_if.driver = (void *)usb_msd;
    scsi_if.read = usb_msd_read;
    scsi_if.write = usb_msd_write;

    return scsi_init(&scsi_if);
  } else
    return ERR_NO_DRIVER;
    
  return 0;
}

void usb_msd_free(struct usb_dev *udev) {
  struct usb_msd_dev *usb_msd;
  
  usb_msd = (struct usb_msd_dev *)udev->driver_data;

  if (udev->subclass_code == 0x06) {
    scsi_free();
    kfree(usb_msd);
  }
}

/*
 * Read write to/from USB Mass Storage Device 
 *
 *  This function encapsulates a command block for 
 *  a destination device into a command block wrapper
 *  according to USB Mass Storage Device specification.
 */
#define MSD_READ 0
#define MSD_WRITE 1

static int usb_msd_read_write(struct usb_msd_dev *umd, int dir,
                              int cb_size, char *cb_data,
                              int length, char *data) {
  int cbw_size;
  int csw_size;
  int rc;

  /* 
   * Prepare standard Command Block Wrapper for 
   * Comand Block that is issued to this mass storage device
   */
  struct command_block_wrapper cbw = {
    .dCBWSignature = CBW_SIGNATURE,
    .dCBWTag = umd->CBWTag++,
    .dCBWDataTransferLength = length,
    .bmCBWFlags = (dir == MSD_READ) ? CBW_FLAGS_IN : CBW_FLAGS_OUT,
    .bCBWLUN = 0,               /* We address only logical unit number 0 */
    .bCBWCBLength = cb_size
  };

  struct command_status_wrapper csw;

  /* 
   * Verify whether Command Block size is in 
   * the permissible range 
   */
  if (cb_size > CB_MAX_SIZE)
    return ERR_PROTO;

  /* Copy Command Block data to the wrapper */
  bcopy(cb_data, (char *)cbw.CBWCB, cb_size);
  
  /* Send the command to the USB device */
  cbw_size = sizeof(struct command_block_wrapper);
  rc = usb_write(umd->bulk_out, cbw_size, (char *)&cbw);

  if (rc != cbw_size)
    goto reset_dev;
  
  /* Read/write data from/to the mass storage device */
  if (length > 0) {
    if (dir == MSD_READ) 
      rc = usb_read(umd->bulk_in, length, data);
    else
      rc = usb_write(umd->bulk_out, length, data);

    if (rc < length)
      goto reset_dev;
  }

  /*
   * Read the operation status, any error is handled 
   * by device reset 
   */
  csw_size = sizeof(struct command_status_wrapper);
  rc = usb_read(umd->bulk_in, csw_size, (char *)&csw);

  if (rc != csw_size)
    goto reset_dev;

  if ((csw.dCSWSignature != CSW_SIGNATURE) ||
      (csw.dCSWTag != cbw.dCBWTag) ||
      (csw.bCSWStatus == CSW_PHASE_ERROR))
    goto reset_dev;

  return csw.bCSWStatus;

reset_dev:
  /* Standard USB mass storage device reset involves
   * tree steps:
   *  - mass storage reset,
   *  - clearing feature HALT to the bulk-in endpoint
   *  - clearing feature HALT to the bulk-out endpoint
   */
  usb_msd_reset(umd);
  usb_msd_clear_halt(umd->bulk_in);
  usb_msd_clear_halt(umd->bulk_out);

  return ERR_XFER;
}

/*
 * Interface functions presented to the device driver 
 */
static int usb_msd_read(void *driver, int cb_size, char *cb_data,
                 int size, char *data) {
  struct usb_msd_dev *umd = (struct usb_msd_dev *)driver;
  
  return usb_msd_read_write(umd, MSD_READ, cb_size, cb_data, size, data);
}

static int usb_msd_write(void *driver, int cb_size, char *cb_data,
                 int size, char *data) {
  struct usb_msd_dev *umd = (struct usb_msd_dev *)driver;
  
  return usb_msd_read_write(umd, MSD_WRITE, cb_size, cb_data, size, data);
}

static struct usb_dev_driver usb_mass_storage_driver = {
  .class_code = 0x08,           /* Mass storage class  */
  .subclass_code = 0x06,        /* SCSI device         */
  .protocol_code = 0x50,        /* Bulk-only transport */
  .init = usb_msd_init,
  .free = usb_msd_free
};

void usb_msd_static_init() {
    usb_dev_driver_register(&usb_mass_storage_driver);
}

