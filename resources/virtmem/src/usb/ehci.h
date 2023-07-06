#ifndef EHCI_H
#define EHCI_H

#include "../util.h"

struct ehci_queue_transfer_descriptor {
  uint32_t next_qtd;
  uint32_t alternate_qtd;
  uint32_t control_status;
  uint32_t buffer_pointer[5];
} __attribute__((packed));

struct ehci_queue_head {
  uint32_t qh_horiz_lp;
  uint32_t ep_cap;
  uint32_t current_qtd;
}

#define EHCI_QH_RL_OFF              28
#define EHCI_QH_C_OFF               27
#define EHCI_QH_MAX_PACKET_LEN_OFF  16
#define EHCI_QH_H_OFF               15
#define EHCI_QH_DTC_OFF             14
#define EHCI_QH_EPS_OFF             12
#define EHCI_QH_ENDPT_OFF           8
#define EHCI_QH_I_OFF               7
#define EHCI_QH_ADDR_MASK       0x0000007F

#define EHCI_QH_MULT_OFF          30
#define EHCI_QH_PORT_NUM_OFF      23
#define EHCI_QH_HUB_ADDR_OFF      16
#define EHCI_QH_UFRAME_CMASK_OFF  8
#define EHCI_QH_UFRAME_SMASK_OFF  0

#define EHCI_TD_DATA_TOGGLE_OFF 31
#define EHCI_TD_TOTAL_SIZE_OFF  16
#define EHCI_TD_IOC_OFF         15
#define EHCI_TD_CERR_OFF        10
#define EHCI_TD_PID_OFF         8

#define EHCI_PID_OUT    0xE1
#define EHCI_PID_IN     0x69
#define EHCI_PID_SETUP  0x2D

#define EHCI_TD_STATUS_MASK     0x000000FF

#define EHCI_TD_BPOINTER_MASK   0xFFFFF000
#define EHCI_TD_POINTER_MASK    0xFFFFFFF0
#define EHCI_TD_TERMINATE       0x00000001


#define EHCI_STATUS_ACTIVE      0x80
#define EHCI_STATUS_HALTED      0x40
#define EHCI_STATUS_DATA_BUFFER 0x20
#define EHCI_STATUS_BABBLE      0x10
#define EHCI_STATUS_XACTERR     0x08
#define EHCI_STATUS_MISS_UF     0x04
#define EHCI_STATUS_SPLIT       0x02
#define EHCI_STATUS_PING        0x01




#endif
