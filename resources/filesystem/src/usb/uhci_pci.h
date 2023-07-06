#ifndef UHCI_PCI_H
#define UHCI_PCI_H

#include "../util.h"
#include "pci.h"
#include "uhci.h"

void uhci_pci_dev_driver_register();
uint32_t uhci_pci_read(uint16_t iobase, uhci_reg ureg);
uint16_t uhci_pci_read_portsc(uint16_t iobase, int port);
void uhci_pci_write(uint16_t iobase, uhci_reg ureg, uint32_t data);
void uhci_pci_write_portsc(uint16_t iobase, int port, uint16_t data);

#define UHCI_PCIREG_COMMAND 0x04
#define UHCI_PCIREG_IO_BASE_ADDR 0x20

#endif
