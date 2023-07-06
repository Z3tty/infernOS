#include "uhci.h"
#include "uhci_pci.h"
#include "pci.h"
#include "allocator.h"
#include "error.h"
#include "debug.h"

#undef DEBUG_NAME
#define DEBUG_NAME "PCI UH"

int uhci_pci_init(struct pci_dev *pci) {
  struct uhci *uh;
  int err;

  uh = kzalloc(sizeof(struct uhci));
  if (uh == NULL)
    return ERR_NO_MEM;

  /* Attach this PCI device to the UHCI */
  uh->pci = pci;
  /* Attach this UHCI state to the PCI device */
  pci->driver = (void *)uh;

  /* 
   * UHCI PCI registers are located in IO space 
   * defined by the PCI device header
   */
  uh->iobase = pci_read_dev_reg32(pci, UHCI_PCIREG_IO_BASE_ADDR) & 0xfffc;

//  pci_write_dev_reg16(pci, PCI_DEV_COMMAND, 0x0005);   /* Bus master & IO space enabled */

  /*
   * Enable interrupts
   */
  pci_write_dev_reg16(pci, 0x00c0, 0x2000);

  /*
   * Initialise UHCI for this device 
   * store interface and state of the driver
   */

  if ((err = uhci_init(uh)) < 0)
    return err;

  return 0;
}

uint32_t uhci_pci_read(uint16_t iobase, uhci_reg ureg) {
  switch (ureg) {
    case UHCI_COMMAND:
      return inw(iobase);
    case UHCI_STATUS:
      return inw(iobase + 0x02);
    case UHCI_INT_EN:
      return inw(iobase + 0x04);
    case UHCI_FRAME_NUM:
      return inw(iobase + 0x06);
    case UHCI_FRAME_BASE_ADDR:
      return inl(iobase + 0x08);
    case UHCI_START_OF_FRAME_MD:
      return inb(iobase + 0x0C);
    default:
      return 0;
  }
  return 0;
}

void uhci_pci_write(uint16_t iobase, uhci_reg ureg, uint32_t data) {
  switch (ureg) {
    case UHCI_COMMAND:
      outw(iobase + 0x00, (uint16_t)data);
      return;
    case UHCI_STATUS:
      outw(iobase + 0x02, (uint16_t)data);
      return;
    case UHCI_INT_EN:
      outw(iobase + 0x04, (uint16_t)data);
      return;
    case UHCI_FRAME_NUM:
      outw(iobase + 0x06, (uint16_t)data);
      return;
    case UHCI_FRAME_BASE_ADDR:
      outl(iobase + 0x08, data);
      return;
    case UHCI_START_OF_FRAME_MD:
      outb(iobase + 0x0C, (uint8_t)data);
    default:
      return;
  }
}

uint16_t uhci_pci_read_portsc(uint16_t iobase, int port) {
  if (port >= 2) 
    return 0;

  return inw(iobase + 0x10 + (port * 2));
}

void uhci_pci_write_portsc(uint16_t iobase, int port, uint16_t data) {
  if (port >= 2)
    return;

  outw(iobase + 0x10 + (port * 2), data);

  return;
}

/* Interrupt handler only forwards the interrupt to UHCI */
void uhci_pci_interrupt(void *driver) {
  struct uhci *uh = (struct uhci *)driver;

  uhci_interrupt(uh);

  return;
}

/* UHCI PCI interface */
static struct pci_dev_driver uhci_pci_driver = {
  .class_code = 0x0c,             /* Serial Bus Controllers */
  .subclass_code = 0x03,          /* USB Controller         */
  .prog_ifc = 0x00,               /* USB UHCI interface     */
  .init = uhci_pci_init,
  .interrupt = uhci_pci_interrupt
};

void uhci_pci_dev_driver_register() {
  pci_dev_driver_register(&uhci_pci_driver);
}
