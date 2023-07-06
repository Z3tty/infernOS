#include "pci.h"
#include "ehci_pci.h"
#include "debug.h"
#include "../util.h"

DEBUG_NAME("EHCI");

/*
 * The only task of this initialisation is to take 
 * control over EHCI chip and to disable it. 
 * As a result all USB v2 devices will be redirected
 * to USB v1 ports
 */
int ehci_pci_init(struct pci_dev *pci) {
  uint8_t *hccreg_base;
  uint8_t *opreg_base;
  uint8_t cap_length;
  uint32_t hcc_params;
  uint32_t eecp;
  uint32_t *usbcmd;
  int i;

  DEBUG("Found EHCI controler");

  /* EHCI registers are located in memory address space */
  hccreg_base = (uint8_t *)(pci_read_dev_reg32(pci, EHCI_PCIREG_BASE) & 0xffffff00);
  DEBUG("   address %x", hccreg_base);

  cap_length = *((uint8_t *)(hccreg_base + EHCI_HCCREG_CAPLENGTH));
  DEBUG("   cap length %d", cap_length);

  hcc_params = *((uint32_t *)(hccreg_base + EHCI_HCCREG_HCCPARAMS));
  eecp = (hcc_params & 0x0000ff00) >> 16;   /* EHCI extended capability pointer */
  DEBUG("   EECP %x", eecp);
  
  opreg_base = hccreg_base + cap_length;
  usbcmd = (uint32_t *)(opreg_base + EHCI_OPREG_USBCMD);
  DEBUG("   USBCMD reg %x", *usbcmd);
  DEBUG("   Reseting the device...");
  *usbcmd = 0x2;

  i = 0;
  while (*((volatile uint32_t *)usbcmd) & 0x2)
    i++;

  DEBUG("   Device is reset and disable (it took %d count cycles)", i);

  return 1;
}

/* EHCI PCI interface */
static struct pci_dev_driver ehci_pci_driver = {
  .class_code = 0x0c,
  .subclass_code = 0x03,
  .prog_ifc = 0x20,
  .init = ehci_pci_init,
  .interrupt = NULL,
};

void ehci_pci_dev_driver_register() {
  pci_dev_driver_register(&ehci_pci_driver);
}


