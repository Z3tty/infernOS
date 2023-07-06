#ifndef UHCI_H
#define UHCI_H

#include "../util.h"
#include "../thread.h"
#include "pci.h"
#include "usb.h"
#include "list.h"

struct usb_dev;

enum uhci_registers_enum {
  UHCI_COMMAND = 0,
  UHCI_STATUS,
  UHCI_INT_EN,
  UHCI_FRAME_NUM,
  UHCI_FRAME_BASE_ADDR,
  UHCI_START_OF_FRAME_MD,
};

typedef enum uhci_registers_enum uhci_reg;

/*
 * UHCI transfer structures 
 */
struct uhci_queue_head {
  uint32_t horiz_lp;               /* horizontal link pointer queue head link pointer */
  uint32_t vert_lp;                /* queue element link pointer */
  /* Non standard elements */
  struct uhci_queue_head *prev_qh; /* QH link pointer pointing to this transfer QH */
  char is_top_queue;               /* 1 for interrupt, control, bulk transfer queue heads */
} __attribute__((packed, aligned(16)));

struct uhci_transfer_unit; 


struct uhci_transfer_descriptor {
  uint32_t lp;              /* link pointer */
  uint32_t control_status;
  uint32_t token;
  uint32_t buffer_pointer;
} __attribute__((packed, aligned(16)));

struct uhci_td_container {
  struct list list;
  struct uhci_transfer_descriptor *td;
};

/* 
 * UHCI transfer description
 */
struct uhci_transfer_unit {
  struct list list;                         /* Transfer units are link together on the free list */
  struct uhci *uh;                          /* UHCI owning this trasnfer unit               */
  struct uhci_queue_head qh;                /* QH of this transfer                          */
  struct list td_list_head;
  uint32_t frame_num;                       /* Frame number assigned when this transfer unit 
                                               was removed from the schedule                */
};

/* Interrupt handler queue structure */
struct uhci_int_queue {
  struct list list;
  struct usb_interrupt *ui;
};

/* State of this UHC controller */
struct uhci {
  uint32_t *frame_base_addr;
  uint32_t current_frame_num;
  
  /* 
   * Pointers to transfer queues 
   * These are link during initialisation.
   */
  struct uhci_queue_head *interrupt_qh;
  struct uhci_queue_head *control_qh;
  struct uhci_queue_head *bulk_in_qh;
  struct uhci_queue_head *bulk_out_qh;
  /*
   * Spinlock to access transfer queues
   */
  spinlock_t xfer_qh_lock;

  /* Address allocator */
  int next_address;

  /* Resource related information */
  int avilable_bandwidth;

  /* UHCI PCI accesses */
  struct pci_dev *pci;
  int iobase;

  /* Queue head of registered interrupt handlers */
  struct list interrupt_handler_list_head;
};

/* Per UHC initialisation function */
int uhci_init(struct uhci *);

void uhci_interrupt(struct uhci *);

/*
 * Global UHC initialisation function. 
 * The main task is to set up transfer descriptor list.
 */
void uhci_static_init(); 

/* Frame List Pointer size */
#define UHCI_FRAME_LIST_SIZE 1024

/*
 * Link pointer related defines. These are use in:
 *  Frame Link Pointer, 
 *  Queue Head Link Pointer, and
 *  Queue Element Link Pointer.
 */
#define UHCI_LP_ADDR_MASK 0xfffffff0
#define UHCI_LP_VF (1 << 2)            /* Switch depth (1)/breadth first, valid only for TDs in queues */
#define UHCI_LP_QH (1 << 1)            /* Related Link Pointer points to either Queue Head (1) or Transfer Descriptor */
#define UHCI_LP_TERMINATE (1 << 0)     /* If set the link pointer is not valid */

/* Transfer Descriptor (TD) related defines */
 /* Token field related */
#define UHCI_TD_PID_IN          0x69
#define UHCI_TD_PID_OUT         0xE1
#define UHCI_TD_PID_SETUP       0x2D
#define UHCI_TD_DEV_ADDR_OFF    8
#define UHCI_TD_END_POINT_OFF   15
#define UHCI_TD_DATA_TOGGLE_OFF 19
#define UHCI_TD_DATA_TOGGLE_BIT (1 << 19)
#define UHCI_TD_MAX_LEN_OFF     21

 /* Control and status related */
#define UHCI_TD_ERROR_COUNTDOWN_OFF 27
#define UHCI_TD_LS_DEV_OFF      26
#define UHCI_TD_ISO_BIT         (1 << 25)
#define UHCI_TD_IOC_OFF         24
#define UHCI_TD_STATUS_OFF      16
#define UHCI_TD_ACTIVE_BIT      (1 << 23)
#define UHCI_TD_STALLED_BIT     (1 << 22)
#define UHCI_TD_BABBLE_BIT      (1 << 20)
#define UHCI_TD_NAK_BIT         (1 << 19)
#define UHCI_TD_TIMEOUT_BIT     (1 << 18)
#define UHCI_TD_STATUS_MASK     0x00fe0000
#define UHCI_TD_ACTLEN          0x000007ff

/* UHCI port status and control register */
#define UHCI_PORTSC_SUSPEND         (1 << 12)
#define UHCI_PORTSC_RESET           (1 << 9)
#define UHCI_PORTSC_LS_DEV          (1 << 8)
#define UHCI_PORTSC_RESUME          (1 << 6)
#define UHCI_PORTSC_ENABLE_CHANGE   (1 << 3)
#define UHCI_PORTSC_ENABLE          (1 << 2)
#define UHCI_PORTSC_CONNECT_CHANGE  (1 << 1)
#define UHCI_PORTSC_CURRENT_CONNECT (1 << 0)

/* UHCI status register */
#define UHCI_SS_USB_INT    (1 << 0)
#define UHCI_SS_ERR_INT    (1 << 1)
#define UHCI_SS_SYSTEM_ERR (1 << 3)
#define UHCI_SS_CONTRL_ERR (1 << 4)
#define UHCI_SS_HALTED     (1 << 5)

#endif
