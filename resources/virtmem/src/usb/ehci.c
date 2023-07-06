#include "ehci.h"
#include "usb_hub.h"
#include "debug.h"

#undef DEBUG_NAME
#define DEBUG_NAME "EHCI"

int ehci_read_write() {

}

/* Hub operations */
struct usb_hub_ops ehci_hub_ops = {
  .port_speed = ehci_port_speed,
  .port_command = ehci_port_command,
  .port_status = ehci_port_status
};

/* Host controler operations */
struct usb_hc_ops ehci_hc_ops = {
  .read = ehci_read,
  .write = ehci_write,
  .setup = ehci_setup,
  .get_next_addr = ehci_get_next_address,
  .register_interrupt_h = ehci_register_interrupt_h
};

int ehci_init(struct ehci *eh) {
  struct usb_hub *root_hub;

  /* Initialise root hub */
  root_hub = kzalloc(sizeof(struct usb_hub));
  if (root_hub == NULL)
    return ERR_NO_MEM;

  root_hub->port_num = 2;           /* The total number of ports on this hub */
  root_hub->hc = (void *)eh;        /* Host controler of this hub */
  root_hub->hc_ops = &ehci_hc_ops;  /* Host controler operations  */
  root_hub->upstream_hub = NULL;    /* This is root hub */
  root_hub->hub_ops = &ehci_hub_ops;/* Hub operations   */
  
  usb_hub_register(root_hub);

  return 0;
}
