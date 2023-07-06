#ifndef PCI_H
#define PCI_H

#include "../util.h"
#include "list.h"

/*
 * This is just a skeleton of the PCI support written to 
 * provide access to USB device 
 */

void pci_init();

struct pci_dev;

struct pci_dev_driver {
  unsigned char class_code;
  unsigned char subclass_code;
  unsigned char prog_ifc;
  int (*init)(struct pci_dev *);
  void (*interrupt)(void *driver);
};

struct pci_driver_list {
  struct list list;
  struct pci_dev_driver *drv;
};

int pci_dev_driver_register(struct pci_dev_driver *);
void pci_interrupt(int num);
void pci_static_init();

uint32_t pci_read_dev_reg32(struct pci_dev *, uint32_t offset);
void pci_write_dev_reg32(struct pci_dev *, uint32_t offset, uint32_t data);
uint16_t pci_read_dev_reg16(struct pci_dev *, uint32_t offset);
void pci_write_dev_reg16(struct pci_dev *, uint32_t offset, uint16_t data);
uint8_t pci_read_dev_reg8(struct pci_dev *, uint32_t offset);
void pci_write_dev_reg8(struct pci_dev *, uint32_t offset, uint8_t data);

enum pci_dev_state_e {
  DEV_STATUS_OPERATING = 0,
  DEV_STATUS_NO_DRIVER,
  DEV_STATUS_UNKNOWN
};

typedef enum pci_dev_state_e pci_dev_state;

struct pci_dev {
  struct list list;
  /* device location on the PCI bus */
  unsigned short bus;
  unsigned short slot;
  unsigned short func;
  /* device operational state */
  pci_dev_state op_state;
  /* device parameters */
  unsigned short vendor;
  unsigned short device;
  unsigned char class_code;
  unsigned char subclass_code;
  unsigned char prog_ifc;
  unsigned char header;
  unsigned char interrupt_line;

  /* pointer to the device driver state */
  void *driver;
  /* pointer to interrupt rutine */
  void (*interrupt)(void *driver);
};

/* 
 * PCI API IO address space for the host bridge 
 * as defined in the PCI 2.x specification 
 */
#define PCI_IO_CONFIG_ADDRESS 0x0CF8
#define PCI_IO_CONFIG_DATA 0x0CFC

/* Offsets in the PCI header */
#define PCI_DEV_VENDOR 0x00
#define PCI_DEV_DEVICE 0x02
#define PCI_DEV_COMMAND 0x04
#define PCI_DEV_STATUS 0x06
#define PCI_DEV_REV_ID 0x08
#define PCI_DEV_PROG_IF 0x09
#define PCI_DEV_SUBCLASS 0x0A
#define PCI_DEV_CLASS 0x0B
#define PCI_DEV_HEADER 0x0E
#define PCI_DEV_INT_LINE 0x3C
#define PCI_DEV_INT_PIN 0x3D

/* PCI dev command register bits */
#define PCI_COMMAND_DISABLE_INT_BIT (1 << 10)

/* PCI dev status register bits */
#define PCI_STATUS_INT_BIT (1 << 2)

/* PCI dev header types */
#define PCI_DEV_HEADER_STANDARD 0
#define PCI_DEV_HEADER_PCI_TO_PCI_BRIDGE 1
#define PCI_DEV_HEADER_CARDBUS_BRIDGE 2
#define PCI_DEV_HEADER_MF_BIT 0x80  /* Device has multiple functions */


#endif

