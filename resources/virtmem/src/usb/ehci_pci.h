#ifndef EHCI_PCI_H
#define EHCI_PCI_H

void ehci_pci_dev_driver_register();

#define EHCI_PCIREG_BASE 0x10

/* 
 * Host controler capability registers (HCCREG) 
 * are located in address memory starting 
 * from address pointed to by EHCI_PCIREG_BASE
 */
#define EHCI_HCCREG_CAPLENGTH 0x00  /* 8 bits reg */
#define EHCI_HCCREG_HCCPARAMS 0x08  /* 32 bits reg */
/*
 * Host controler operational registers (OPREG)
 * are located in address memory starting from 
 * address pointed to by 
 *  EHCI_PCIREG_BASE + EHCI_HCCREG_CAPLENGTH
 */
#define EHCI_OPREG_USBCMD     0x00  /* 32 bits reg */

#endif
