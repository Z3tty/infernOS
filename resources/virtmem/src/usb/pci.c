#include "../util.h" 
#include "error.h"
#include "pci.h"
#include "debug.h"
#include "allocator.h"
#include "uhci_pci.h"
#include "ehci_pci.h"

DEBUG_NAME("PCI");

static struct list pci_drv_list_head;
static struct list pci_dev_list_head;

/* function prototypes */
static void pci_bus_probe();
static int pci_dev_exists(int bus, int slot, int func);

/* 
 * PCI initialisation function 
 *
 * It just clears the statically allocated memory area for PCI devices
 * and scans PCI slots querring the attached cards. 
 */
void pci_static_init() {
  DEBUG("Initialising PCI subsystem...");

  LIST_INIT(&pci_drv_list_head);
  LIST_INIT(&pci_dev_list_head);

  /* Register UHCI and EHCI device drivres */
  uhci_pci_dev_driver_register();
  ehci_pci_dev_driver_register();

  /* Scan PCI slots */
  pci_bus_probe();
}

static void pci_print_device_info(struct pci_dev *dev) {

  DEBUG("Found PCI device:");
  DEBUG("  Vendor: 0x%04x, Device: 0x%04x", 
      dev->vendor, dev->device);
  DEBUG("  Class: 0x%02x, Subclass: 0x%02x, ProgIF: 0x%02x", 
      dev->class_code, dev->subclass_code, dev->prog_ifc);
  DEBUG("  Location:  Bus: %d, Slot: %d", 
      dev->bus, dev->slot);
  DEBUG("  Interrupt line: %d", dev->interrupt_line);

  return;
}

static int pci_lookup_driver(struct pci_dev *dev) {
  struct pci_dev_driver *pdd;
  struct pci_driver_list *pdl;

  LIST_FOR_EACH(&pci_drv_list_head, pdl) { 
    pdd = pdl->drv;
    if ((dev->class_code == pdd->class_code) &&
        (dev->subclass_code == pdd->subclass_code) &&
        (dev->prog_ifc == pdd->prog_ifc)) {
      /* Bind the interrupt rutine */
      dev->interrupt = pdd->interrupt;
      /* Initialise the device driver and return */
      return pdd->init(dev);
    }
  }

  return ERR_NO_DRIVER; 
}

/*
 * Function initialises the PCI device
 * It returns a positive value when the device has multiple functions
 * otherwise it returns null
 */
static int pci_init_device(unsigned int bus, unsigned int slot,
                           unsigned int func) {
  struct pci_dev *dev;
  int multiple_functions;
  int err;

  dev = kzalloc(sizeof(struct pci_dev));
  if (dev == NULL)
    return 0;

  /* Populate the basic information about this PCI device */
  dev->bus = bus;
  dev->slot = slot;
  dev->func = func;
  dev->vendor = pci_read_dev_reg16(dev, PCI_DEV_VENDOR);
  dev->device = pci_read_dev_reg16(dev, PCI_DEV_DEVICE);
  dev->class_code = pci_read_dev_reg8(dev, PCI_DEV_CLASS);
  dev->subclass_code = pci_read_dev_reg8(dev, PCI_DEV_SUBCLASS);
  dev->prog_ifc = pci_read_dev_reg8(dev, PCI_DEV_PROG_IF);
  dev->header = pci_read_dev_reg8(dev, PCI_DEV_HEADER);
  dev->interrupt_line = pci_read_dev_reg8(dev, PCI_DEV_INT_LINE);
  
  /*
   * Linking procedure cannot be interrupted by PCI interrupt 
   * that traverses the list 
   */
  enter_critical();
  LIST_LINK(&pci_dev_list_head, dev);
  leave_critical();

  pci_print_device_info(dev);

  err = pci_lookup_driver(dev);
  if (err == 0)
    dev->op_state = DEV_STATUS_OPERATING;
  else if (err == ERR_NO_DRIVER)
    dev->op_state = DEV_STATUS_NO_DRIVER;
  else if (err == ERR_NO_MEM) 
    dev->op_state = DEV_STATUS_NO_DRIVER;
  else 
    dev->op_state = DEV_STATUS_UNKNOWN;

  /*
   * Check whether this is a PCI device with multiple functions
   * If a PCI device has multiple functions, it is indicated in 
   * the header field with MF bit. We return this information 
   * to the PCI bus probe.
   */
  if ((dev->header & PCI_DEV_HEADER_MF_BIT) != 0)
    multiple_functions = 1;
  else 
    multiple_functions = 0;

  return multiple_functions;  
}

/*
 * A device driver calls this function to register on
 * the PCI device drivers list. Later, the registered device
 * drivers are matched against devices found on the PCI bus.
 */
int pci_dev_driver_register(struct pci_dev_driver *pdd) {
  struct pci_driver_list *pdl;

  pdl = kzalloc(sizeof(struct pci_driver_list));
  if(pdl == NULL)
    return ERR_NO_MEM;

  DEBUG("Registering a driver for the device of"
        "class %02x, subclass %02x, interface %02x",
      pdd->class_code, pdd->subclass_code, pdd->prog_ifc);

  pdl->drv = pdd;
  LIST_LINK(&pci_drv_list_head, pdl);

  return 0;
}

static void pci_bus_probe() {
  int bus;
  int slot;
  int func;
  int mf;
  
  /*
   * Each PCI bus can have up to 32 slots and each PCI device
   * can have up to 8 functions.  
   */
  for (bus = 0; bus < 1; bus++)
    for (slot = 0; slot < 32; slot++) 
      if(pci_dev_exists(bus, slot, 0)) {
        mf = pci_init_device(bus, slot, 0);
      
        /* 
         * If a device has multiple functions. 
         * We register each function as a PCI device
         */
        if (mf == 1)
          for (func = 1; func < 8; func++)
            if (pci_dev_exists(bus, slot, func))
              mf = pci_init_device(bus, slot, func);
      }

  return;
}

/* 
 * PCI interrupt handler
 *
 * A PCI device interrupt pin is connected to one of four PCI interrupt
 * lines: PIRQA, PIRQB, PIRQC, or PIRQD. These PCI interrupt lines are 
 * routed via PIR to the 5th line of the master PIC and the 9th, 10th, 
 * or 11th line of the slave PIC.  When the PCI interrupt fires, 
 * this function is called with an argument containing the PIC line number
 * that caused this interrupt call. 
 * 
 * The PIC line the PCI device is connected to is stored in the PCI device
 * header. This mapping is done by BIOS during initialisation process. 
 * Many PCI devices can use the same interrupt line. However, we leave verification 
 * of the interrupt event for the device driver.
 */
void pci_interrupt(int int_line) {
  struct pci_dev *dev;

  /* Find PCI devices that could trigger this interrupt */
  LIST_FOR_EACH(&pci_dev_list_head, dev)
    if ((dev->op_state == DEV_STATUS_OPERATING) &&
        (dev->interrupt_line == int_line)) 
        dev->interrupt(dev->driver);

  return;
}

/*
 * Sets the Configuration Address Register value of a host bridge 
 * to enable access to a register at the given offset in 
 * the configuration space of a PCI device. The PCI device is identified 
 * by the bus, slot, and func parameters. 
 */
static inline void
set_configuration_address(int bus, int slot, int func, int offset) {
  uint32_t address;

  address = (1 << 31) | 
          (bus << 16) |
         (slot << 11) |
          (func << 8) | 
      (offset & 0xfc);

  outl(PCI_IO_CONFIG_ADDRESS, address);

  return;
}

/*
 * Function check whether a PCI device identified by 
 * the bus, slot, and func parameters exists.
 */
static int pci_dev_exists(int bus, int slot, int func) {
  uint16_t vendor_id;

  set_configuration_address(bus, slot, func, PCI_DEV_VENDOR);
  vendor_id = inw(PCI_IO_CONFIG_DATA + (PCI_DEV_VENDOR & 0x02));

  /* Vendor ID value of 0xffff indicates empty slot */
  if (vendor_id == 0xffff) 
    return 0;

  return 1;
}

/*
 * Read PCI device header register
 */
#define pci_read_dev_reg(x, in) \
  uint##x##_t pci_read_dev_reg##x(struct pci_dev *dev, uint32_t offset) { \
    set_configuration_address(dev->bus, dev->slot, dev->func, offset); \
    return in(PCI_IO_CONFIG_DATA + (offset & 0x03)); \
  }

pci_read_dev_reg(32, inl);
pci_read_dev_reg(16, inw);
pci_read_dev_reg(8, inb);

/* 
 * Write PCI device header register
 */
#define pci_write_dev_reg(x, out) \
  void pci_write_dev_reg##x(struct pci_dev *dev, uint32_t offset, uint##x##_t data) { \
    set_configuration_address(dev->bus, dev->slot, dev->func, offset); \
    out(PCI_IO_CONFIG_DATA + (offset & 0x03), data); \
  }

pci_write_dev_reg(32, outl);
pci_write_dev_reg(16, outw);
pci_write_dev_reg(8, outb);


