#include "../util.h"
#include "../scheduler.h"
#include "../interrupt.h"
#include "../common.h"
#include "uhci.h"
#include "uhci_pci.h"
#include "usb.h"
#include "usb_hub.h"
#include "allocator.h"
#include "error.h"
#include "debug.h"
#include "list.h"

DEBUG_NAME("UHCI");

/* Heads of free transfer transfer units */
static struct slist free_xfer_slist_head;
static struct slist free_tdc_slist_head;

/* Transfer descriptor container cache */
static struct uhci_td_container *tdc_alloc()
{
	struct uhci_td_container *tdc;

	SLIST_LOCK(&free_tdc_slist_head);
	LIST_FIRST(&free_tdc_slist_head, tdc);
	if ((void *)tdc != (void *)&free_tdc_slist_head) {
		LIST_UNLINK(tdc);
		SLIST_UNLOCK(&free_tdc_slist_head);

		return tdc;
	}
	SLIST_UNLOCK(&free_tdc_slist_head);

	tdc = kzalloc(sizeof(struct uhci_td_container));
	if (tdc == NULL)
		return NULL;
	tdc->td = kzalloc_align(sizeof(struct uhci_transfer_descriptor), 16);
	if (tdc->td == NULL) {
		kfree(tdc);
		return NULL;
	}

	return tdc;
}

static void tdc_free(struct uhci_td_container *tdc)
{
	SLIST_LOCK(&free_tdc_slist_head);
	LIST_LINK(&free_tdc_slist_head, tdc);
	SLIST_UNLOCK(&free_tdc_slist_head);
}

/*
 * Transfer descriptor helper functions
 */
static void td_reactivate(struct usb_pipe *pipe,
			  struct uhci_transfer_descriptor *td)
{
	/* Clear transfer length field */
	td->control_status &= ~UHCI_TD_ACTLEN;
	/* Clear transfer status */
	td->control_status &= ~UHCI_TD_STATUS_MASK;
	/* Set transfer active */
	td->control_status |= UHCI_TD_ACTIVE_BIT;
	/* Set toggle bit */
	td->token &= ~UHCI_TD_DATA_TOGGLE_BIT;
	td->token |= pipe->toggle_bit;
}

/*
 * Build transfer descriptor according to Intel
 * UHCI Design Guide p. 21-23
 */
static struct uhci_td_container *td_build(struct usb_pipe *pipe, uint8_t pid,
					  int size, char *data)
{

	struct uhci_transfer_descriptor *td;
	struct uhci_td_container *tdc;
	int usb_dev_speed;
	int interrupt_pipe;

	tdc = tdc_alloc();
	if (tdc == NULL)
		return NULL;

	td = tdc->td;

	usb_dev_speed = pipe->udev->usb_sc == USB_LS_DEV ? 1 : 0;
	interrupt_pipe = pipe->attributes == USB_PIPE_INTERRUPT ? 1 : 0;

	td->lp = UHCI_LP_TERMINATE;

	td->control_status = (usb_dev_speed << UHCI_TD_LS_DEV_OFF)
			     | /* USB device speed                          */
			     (3 << UHCI_TD_ERROR_COUNTDOWN_OFF)
			     | /* Allow up to three errors in transmission  */
			     (interrupt_pipe << UHCI_TD_IOC_OFF)
			     | /* Generate Interrupt on transfer complete
				* for data transmitted over interrupt pipes */
			     UHCI_TD_ACTIVE_BIT;

	td->token = (size - 1) << UHCI_TD_MAX_LEN_OFF | /* Transfer size    */
		    pipe->toggle_bit << UHCI_TD_DATA_TOGGLE_OFF
		    | /* Toggle bit       */
		    pipe->ep_address << UHCI_TD_END_POINT_OFF
		    | /* Endpoint address */
		    pipe->udev->address << UHCI_TD_DEV_ADDR_OFF
		    |    /* Device address   */
		    pid; /* Packet ID: SETUP, IN, or OUT */

	td->buffer_pointer = (uint32_t)data;

	return tdc;
}

/*
 * Helper functions to allocate and free transfer units. This function
 * does not permit to reuse a transfer unit that has been used in this
 * time frame. This is to ensure consistency in the UHCI schedule.
 * Otherwise, the following could go wrong:
 *  1. UHCI is processing some QH pointing to the QH related to a transfer
 *     unit which is about to be freed.
 *  2. CPU frees the transfer unit and the QH
 *  3. CPU allocates this QH for other transfer and modifies its content
 *  4. UHCI begins to process the QH with modified content.
 *
 * If we ensure that QH is left untouched in this frame we can safely
 * reuse it in the next frame, since it is removed from the UHCI schedule
 */
static struct uhci_transfer_unit *xfer_alloc()
{
	struct uhci_transfer_unit *xfer;
	struct uhci *uh;
	uint32_t frame_num;

	/* To ensure consistency */
	SLIST_LOCK(&free_xfer_slist_head);
	/* Try to reuse already allocated xfer container */
	LIST_FOR_EACH(&free_xfer_slist_head, xfer)
	{
		/*
		 * Does this transfer unit belong to some UHCI? If so, check
		 * whether it has been released in this time frame. We do not
		 * reuse transfer units that has been released in this time
		 * frame, since this can be still processed by UHCI.
		 */
		if (xfer->uh != NULL) {
			uh = xfer->uh;
			frame_num = uhci_pci_read(uh->iobase, UHCI_FRAME_NUM);
			if (xfer->frame_num == frame_num)
				continue;
		}
		/* First transfer unit that matches our conditions */

		/* Unlink this transfer unit */
		LIST_UNLINK(xfer);

		SLIST_UNLOCK(&free_xfer_slist_head);

		/* Prepare and return it */
		bzero((char *)xfer, sizeof(struct uhci_transfer_unit));

		xfer->qh.horiz_lp = UHCI_LP_TERMINATE;
		xfer->qh.vert_lp = UHCI_LP_TERMINATE;
		LIST_INIT(&xfer->td_list_head);

		return xfer;
	}

	/* No free transfer units */
	SLIST_UNLOCK(&free_xfer_slist_head);

	/* Allocate a new one */
	xfer = kzalloc(sizeof(struct uhci_transfer_unit));

	xfer->qh.horiz_lp = UHCI_LP_TERMINATE;
	xfer->qh.vert_lp = UHCI_LP_TERMINATE;
	LIST_INIT(&xfer->td_list_head);

	return xfer;
}

/*
 * Link transfer unit back on the free transfer unit list
 * and frees all associated transfer descriptors containers
 */
static void xfer_free(struct uhci_transfer_unit *xfer)
{
	struct uhci_td_container *tdc, *tdc_helper;

	LIST_FOR_EACH_SAFE(&xfer->td_list_head, tdc, tdc_helper)
	{
		LIST_UNLINK(tdc);
		tdc_free(tdc);
	}

	/* Put this transfer unit onto the free list */
	SLIST_LOCK(&free_xfer_slist_head);
	LIST_LINK(&free_xfer_slist_head, xfer);
	SLIST_UNLOCK(&free_xfer_slist_head);
}

/*
 * Count transmitted bytes
 */
int xfer_count_bytes(struct uhci_transfer_unit *xfer)
{
	struct uhci_td_container *tdc;
	int xfer_bytes = 0;

	LIST_FOR_EACH(&xfer->td_list_head, tdc)
	xfer_bytes += (tdc->td->control_status + 1) & UHCI_TD_ACTLEN;

	return xfer_bytes;
}

/*
 * Returns the cumulative status from all TDs
 */
uint32_t xfer_get_status(struct uhci_transfer_unit *xfer)
{
	struct uhci_td_container *tdc;
	uint32_t status;

	status = 0;

	LIST_FOR_EACH(&xfer->td_list_head, tdc)
	/* Sum up all messages of this transfer */
	status |= tdc->td->control_status;

	status &= UHCI_TD_STATUS_MASK;

	return status;
}
/*
 * Adds TD to a transfer unit
 */
static void xfer_add_tdc(struct uhci_transfer_unit *xfer,
			 struct uhci_td_container *tdc)
{
	struct uhci_td_container *tdc_tmp;
	/*
	 * If this is the first TD in this transfer unit
	 * set the transfer unit QH vertical link pointer to
	 * point to this TD and record this TD as the first
	 * one
	 *  else
	 * Set the tail TD link pointer to point to
	 * this TD and link this TD on this transfer unit list
	 */
	if (xfer->qh.vert_lp == UHCI_LP_TERMINATE)
		xfer->qh.vert_lp = (uint32_t)tdc->td;
	else {
		LIST_LAST(&xfer->td_list_head, tdc_tmp);
		tdc_tmp->td->lp = (uint32_t)tdc->td;
	}

	/* This transfer container is the last on the transfer list */
	LIST_LINK(&xfer->td_list_head, tdc);
}

/*
 * Safely enqueues a transfer unit for transaction
 *
 * When enqueuing a transfer unit we can experience two
 * race conditions:
 *  I) With UHCI managing transfer queues
 *    This UHCI schedule is modified with one atomic
 *    write operation updating QH
 *  II) With other processes enqueuing data transfers
 *    We use spinlock to achieve atomicity when accessing
 *    this UHCI schedule
 */
static void xfer_enqueue(struct uhci_transfer_unit *xfer, struct uhci *uh,
			 struct uhci_queue_head *uhci_qh)
{
	struct uhci_queue_head *next_qh;
	xfer->uh = uh;
	/*
	 * This function must be atomic with respect to
	 * multiple processes accessing it. Atomicity with
	 * respect to UHCI operation is achieved by atomic
	 * write operation to the UHCI queue head pointing to
	 * this transfer
	 */
	spinlock_acquire(&uh->xfer_qh_lock);

	/*
	 * Vertical link pointer of the UHCI queue head of NULL
	 * value indicates that there is no transfer queued
	 * on this UHCI queue. Otherwise, there are pending
	 * transfers and we insert this transfer in front.
	 */
	if ((uhci_qh->vert_lp & UHCI_LP_TERMINATE) != 0)
		xfer->qh.horiz_lp = uhci_qh->horiz_lp;
	else {
		xfer->qh.horiz_lp = uhci_qh->vert_lp;
		/*
		 * In this case we must update back reference of
		 * the next transfer unit
		 */
		next_qh = (struct uhci_queue_head *)(uhci_qh->vert_lp
						     & UHCI_LP_ADDR_MASK);
		next_qh->prev_qh = &xfer->qh;
	}

	/* Record the back reference */
	xfer->qh.prev_qh = uhci_qh;

	/*
	 * Transfer unit is ready to be put on the UHC schedule
	 * We do it with a single write operation
	 */
	uhci_qh->vert_lp = (uint32_t)&xfer->qh | UHCI_LP_QH;

	spinlock_release(&uh->xfer_qh_lock);
}

/*
 * Safely removes transfer unit from the UHC schedule
 */
static void xfer_dequeue(struct uhci_transfer_unit *xfer)
{
	struct uhci_queue_head *next_qh;
	struct uhci_queue_head *prev_qh;
	struct uhci_queue_head *qh = &xfer->qh;
	struct uhci *uh = xfer->uh;

	spinlock_acquire(&uh->xfer_qh_lock);
	/*
	 * Update the back reference of the next transfer unit
	 * (does not affect the UHCI schedule)
	 * If it is a top queue head it causes no effect.
	 */
	next_qh = (struct uhci_queue_head *)(qh->horiz_lp & UHCI_LP_ADDR_MASK);
	if (!next_qh->is_top_queue)
		next_qh->prev_qh = qh->prev_qh;
	/*
	 * Update the forward reference of the previous queue head
	 */
	prev_qh = qh->prev_qh;

	/*
	 * If the previous queue head and the next queue head
	 * are top queues, these are already linked and we only
	 * clear the vertical link pointer of the previous queue head
	 */
	if (prev_qh->is_top_queue && next_qh->is_top_queue)
		prev_qh->vert_lp = UHCI_LP_TERMINATE;
	else if (prev_qh->is_top_queue)
		prev_qh->vert_lp = qh->horiz_lp;
	else
		prev_qh->horiz_lp = qh->horiz_lp;

	/* Mark frame number when this transfer unit is freed */
	xfer->frame_num = uhci_pci_read(uh->iobase, UHCI_FRAME_NUM);
	spinlock_release(&uh->xfer_qh_lock);
}

/*
 * Print xfer status
 */
void xfer_print(struct uhci_transfer_unit *xfer)
{
	uint32_t status;
	char *status_text;
	int actlen;

	status = xfer_get_status(xfer);
	actlen = xfer_count_bytes(xfer);
	if (((status >> UHCI_TD_STATUS_OFF) & 0xff) == 0) {
		status_text = "OK";
	} else {
		if (status & UHCI_TD_ACTIVE_BIT)
			status_text = "ACTIVE";
		if (status & UHCI_TD_STALLED_BIT)
			status_text = "STALLED";
		if (status & UHCI_TD_BABBLE_BIT)
			status_text = "BABBLE";
		if (status & UHCI_TD_NAK_BIT)
			status_text = "NAK";
		if (status & UHCI_TD_TIMEOUT_BIT)
			status_text = "CRC/TIMEOUT";
	}
	DEBUG("TD actlen %2d, status %s", actlen, status_text);
}

/*
 * Sets up a free transfer descriptor list
 */
void uhci_static_init()
{
	SLIST_INIT(&free_xfer_slist_head);
	SLIST_INIT(&free_tdc_slist_head);

	return;
}

/*
 * Read/write operation over USB pipes
 *
 * This function support the RW operation even if data transfer
 * exceeds the maximum packet size. In such case the transfer
 * is split into multiple packets that carry subsequent data
 * chunks.
 */
static int uhci_read_write(struct usb_pipe *pipe, int pid, int size, char *data)
{
	struct uhci_td_container *tdc;
	struct uhci_transfer_unit *xfer_container;
	struct uhci *uh;
	uint32_t status;
	int remaining_size = size;
	int packet_size;
	int err = 0;

	uh = (struct uhci *)pipe->udev->hc;

	/*
	 * SETUP packet always sets the toggle bit to 0
	 * for host controller and device
	 */
	if (pid == UHCI_TD_PID_SETUP)
		pipe->toggle_bit = 0;

	if ((xfer_container = xfer_alloc()) == NULL)
		return ERR_NO_MEM;


	/* Transfers must also handle packets with size 0 */
	do {
		packet_size = remaining_size > pipe->max_packet_size
				      ? pipe->max_packet_size
				      : remaining_size;
		remaining_size -= packet_size;

		tdc = td_build(pipe, pid, packet_size, data);
		data += packet_size;

		if (tdc == NULL) {
			err = ERR_NO_MEM;
			goto error_uhci_rw;
		}

		pipe->toggle_bit = 1 - pipe->toggle_bit;
		xfer_add_tdc(xfer_container, tdc);

	} while (remaining_size > 0);


	/*
	 * Enqueue this transfer unit on the UHCI schedule
	 */
	switch (pid) {
	case UHCI_TD_PID_IN:
		xfer_enqueue(xfer_container, uh, uh->bulk_in_qh);
		break;
	case UHCI_TD_PID_OUT:
		xfer_enqueue(xfer_container, uh, uh->bulk_out_qh);
		break;
	case UHCI_TD_PID_SETUP:
		xfer_enqueue(xfer_container, uh, uh->control_qh);
		break;
	default:
		err = ERR_PROTO;
		goto error_uhci_rw;
	}

	/* Wait until all TD are executed */
	while ((status = xfer_get_status(xfer_container)) != 0) {
		/* If status bits are != 0 this indicates either:
		 *  there are active TD pending for execution
		 * or
		 *  there was transmission error which results
		 *  in device being stalled
		 */
		if (status & UHCI_TD_STALLED_BIT) {
			err = ERR_DEV_STALLED;
			break;
		}
		/* Wait */
		yield();
	}

	xfer_dequeue(xfer_container);

	/*
	 * If transmission was successful we return the number of
	 * transmitted bytes
	 */
	if (err == 0)
		err = xfer_count_bytes(xfer_container);

error_uhci_rw:
	xfer_print(xfer_container);
	xfer_free(xfer_container);

	return err;
}

/* Standard read write UHCI functions */
static int uhci_read(struct usb_pipe *pipe, int size, char *data)
{
	return uhci_read_write(pipe, UHCI_TD_PID_IN, size, data);
}

static int uhci_write(struct usb_pipe *pipe, int size, char *data)
{
	return uhci_read_write(pipe, UHCI_TD_PID_OUT, size, data);
}

static int uhci_setup(struct usb_pipe *pipe, struct usb_dev_setup_request *data)
{
	return uhci_read_write(pipe, UHCI_TD_PID_SETUP, 8, (char *)data);
	/* Setup packet size is always 8B */
}

static uint8_t uhci_get_next_address(struct usb_dev *udev)
{
	struct uhci *uh = (struct uhci *)udev->hc;
	uint8_t address;

	address = uh->next_address;
	uh->next_address++;

	return address;
}

/*
 * This function is used to register an interrupt handler
 *
 *  Registered interrupt handlers are called when UHCI
 *  receives the interrupt on complete (IOC) signal
 *
 */
static int uhci_register_interrupt_h(struct usb_interrupt *ui)
{
	struct uhci_transfer_unit *xfer_container;
	struct uhci_td_container *tdc;
	struct uhci *uh = (struct uhci *)ui->pipe->udev->hc;
	struct uhci_int_queue *iq_element;

	iq_element = kzalloc(sizeof(struct uhci_int_queue));

	if (iq_element == NULL)
		return ERR_NO_MEM;

	iq_element->ui = ui;

	/*
	 * Enqueue interrupt transfer on UHCI schedule
	 * We assume that data transfer fits one packet
	 */
	if ((xfer_container = xfer_alloc()) == NULL)
		return ERR_NO_MEM;

	tdc = td_build(ui->pipe, UHCI_TD_PID_IN, ui->size, ui->buff);
	if (!tdc) {
		xfer_free(xfer_container);
		kfree(iq_element);
		return ERR_NO_MEM;
	}

	xfer_add_tdc(xfer_container, tdc);
	ui->pipe->xfer = xfer_container;

	/*
	 * We cannot be interrupted when modifying the interrupt
	 * handler list, since then the UHCI interrupt handler
	 * would traverse a broken list.
	 *
	 * We also cannot modify the list when the UHCI interrupt
	 * handler traverses it. However, to protect against this
	 * threat it is required to use rw locks that are not
	 * implemented (TODO)
	 */
	enter_critical();
	LIST_LINK(&uh->interrupt_handler_list_head, iq_element);
	leave_critical();

	/* From now on transfer can trigger interrupts */
	xfer_enqueue(xfer_container, uh, uh->interrupt_qh);

	return 0;
}

/*
 * Function removes an interrupt handler
 */
int uhci_remove_interrupt_h(struct usb_interrupt *ui)
{
	struct uhci_transfer_unit *xfer_container;
	struct uhci_int_queue *iq_i;
	struct uhci *uh;

	uh = (struct uhci *)ui->pipe->udev->hc;

	LIST_FOR_EACH(&uh->interrupt_handler_list_head, iq_i)
	if (iq_i->ui == ui) {
		/* Interrupt handler found, unlink it */
		enter_critical();
		LIST_UNLINK(iq_i);
		leave_critical();

		/*
		 * Remove transfer container describing USB transfers
		 * for this interrupt
		 */
		xfer_container = iq_i->ui->pipe->xfer;
		xfer_dequeue(xfer_container);
		xfer_free(xfer_container);

		kfree(iq_i);

		return 0;
	}

	/* Interrupt handler was not found */

	return 0;
}

/*
 * This function gets called when the UHCI issues an interrupt
 */
void uhci_interrupt(struct uhci *uh)
{
	struct uhci_transfer_unit *xfer_container;
	struct uhci_td_container *tdc;
	struct uhci_int_queue *iq_i;
	struct usb_pipe *pipe;
	uint16_t int_status;
	uint16_t xfer_status;

	int_status = uhci_pci_read(uh->iobase, UHCI_STATUS);
	/*
	 * Clear Interrupt Status Register (ISR) to re-enable interrupts
	 */
	uhci_pci_write(uh->iobase, UHCI_STATUS, int_status);
	/*
	 * Interrupt on status complete
	 *   This may indicate that some data transfer is
	 *   ready to be processed by interrupt handlers
	 */
	if ((int_status & UHCI_SS_USB_INT) != 0) {
		/* Scan through all registered interrupt handlers */
		LIST_FOR_EACH(&uh->interrupt_handler_list_head, iq_i)
		{
			/* Is this transfer the source of interrupt? */
			pipe = iq_i->ui->pipe;
			xfer_container =
				(struct uhci_transfer_unit *)pipe->xfer;
			xfer_status = xfer_get_status(xfer_container);
			if ((xfer_status & UHCI_TD_ACTIVE_BIT) != 0)
				/* No, check the next interrupt source */
				continue;

			/* Handle this interrupt */
			iq_i->ui->interrupt_h(iq_i->ui->data);

			/* Reactivate this transfer (this ensures at least 2ms
			 * freq) */
			pipe->toggle_bit = 1 - pipe->toggle_bit;
			LIST_FIRST(&xfer_container->td_list_head, tdc);
			td_reactivate(pipe, tdc->td);
			/* Put back on schedule! */
			xfer_container->qh.vert_lp = (uint32_t)tdc->td;
		}
	}

	return;
}

/*
 *
 * Implementation of hub operations
 *
 */
static void uhci_port_set_bit(struct uhci *uh, int port, uint16_t bit)
{
	uint16_t status;

	status = uhci_pci_read_portsc(uh->iobase, port);
	status |= bit;
	uhci_pci_write_portsc(uh->iobase, port, status);
}

static void uhci_port_clear_bit(struct uhci *uh, int port, uint16_t bit)
{
	uint16_t status;

	status = uhci_pci_read_portsc(uh->iobase, port);
	status &= ~bit;
	uhci_pci_write_portsc(uh->iobase, port, status);
}

enum port_status_e uhci_port_status(struct usb_hub *uhub, int port)
{
	struct uhci *uh = (struct uhci *)uhub->hc;
	enum port_status_e rc;
	uint16_t status;

	status = uhci_pci_read_portsc(uh->iobase, port);
	if ((status & UHCI_PORTSC_CURRENT_CONNECT) == 0)
		rc = USB_PORT_DISCONNECTED;
	else if ((status & UHCI_PORTSC_ENABLE) == 0)
		rc = USB_PORT_DISABLED;
	else if ((status & UHCI_PORTSC_SUSPEND) == 0)
		rc = USB_PORT_ENABLED;
	else
		rc = USB_PORT_SUSPENDED;

	return rc;
}

void uhci_port_command(struct usb_hub *uhub, int port,
		       enum uhub_port_command_e command)
{
	struct uhci *uh = (struct uhci *)uhub->hc;

	switch (command) {
	case USB_PORT_RESET:
		uhci_port_set_bit(uh, port, UHCI_PORTSC_RESET);
		break;
	case USB_PORT_CLEAR_RESET:
		uhci_port_clear_bit(uh, port, UHCI_PORTSC_RESET);
		break;
	case USB_PORT_ENABLE: {
		uint16_t value;
		uhci_port_set_bit(uh, port, UHCI_PORTSC_ENABLE);

		// Make sure port is actually enabled
		do {
			value = uhci_pci_read_portsc(uh->iobase, port);
			ms_delay(1);
		} while (((value & UHCI_PORTSC_ENABLE) == 0)
			 && (value & UHCI_PORTSC_CURRENT_CONNECT));

		break;
	}
	case USB_PORT_DISABLE:
		uhci_port_clear_bit(uh, port, UHCI_PORTSC_ENABLE);
		break;
	case USB_PORT_CLEAR_SUSPEND:
		uhci_port_clear_bit(uh, port, UHCI_PORTSC_SUSPEND);
		break;
	default:
		break;
	}
	return;
}

/* Function checks the USB device speed */
enum usb_speed_class_e uhci_port_speed(struct usb_hub *uhub, int port)
{
	struct uhci *uh = (struct uhci *)uhub->hc;

	return ((uhci_pci_read_portsc(uh->iobase, port) & UHCI_PORTSC_LS_DEV)
		!= 0)
		       ? USB_LS_DEV
		       : USB_FS_DEV;
}

/* Hub operations */
struct usb_hub_ops uhci_hub_ops = {.port_speed = uhci_port_speed,
				   .port_command = uhci_port_command,
				   .port_status = uhci_port_status};

/* Host controler operations */
struct usb_hc_ops uhci_hc_ops = {.read = uhci_read,
				 .write = uhci_write,
				 .setup = uhci_setup,
				 .get_next_addr = uhci_get_next_address,
				 .register_interrupt_h =
					 uhci_register_interrupt_h,
				 .remove_interrupt_h = uhci_remove_interrupt_h};

/*
 * Initialisation of UHCI driver
 * UHCI performs two tasks:
 *  1) manages transmission over USB
 *  2) acts as a root hub
 *
 * Before calling this function the PCI UHCI
 * setups iobase vector to point to IO registers
 * of this UHCI.
 */

int uhci_init(struct uhci *uh)
{
	uint32_t *frame_list_pointer;
	struct usb_hub *root_hub;
	int i;

	DEBUG("Initialising UHCI with iobase %04x", uh->iobase);
	/*
	 * Setup and link transfer queues
	 */
	uh->interrupt_qh = kzalloc_align(sizeof(struct uhci_queue_head), 16);
	uh->control_qh = kzalloc_align(sizeof(struct uhci_queue_head), 16);
	uh->bulk_in_qh = kzalloc_align(sizeof(struct uhci_queue_head), 16);
	uh->bulk_out_qh = kzalloc_align(sizeof(struct uhci_queue_head), 16);

	uh->interrupt_qh->horiz_lp = ((uint32_t)uh->control_qh) | UHCI_LP_QH;
	uh->interrupt_qh->vert_lp = UHCI_LP_TERMINATE;
	uh->interrupt_qh->is_top_queue = 1;
	uh->control_qh->horiz_lp = ((uint32_t)uh->bulk_in_qh) | UHCI_LP_QH;
	uh->control_qh->vert_lp = UHCI_LP_TERMINATE;
	uh->control_qh->is_top_queue = 1;
	uh->bulk_in_qh->horiz_lp = ((uint32_t)uh->bulk_out_qh) | UHCI_LP_QH;
	uh->bulk_in_qh->vert_lp = UHCI_LP_TERMINATE;
	uh->bulk_in_qh->is_top_queue = 1;
	uh->bulk_out_qh->horiz_lp = UHCI_LP_TERMINATE;
	//  uh->bulk_out_qh->horiz_lp = ((uint32_t)uh->control_qh) | UHCI_LP_QH;
	uh->bulk_out_qh->vert_lp = UHCI_LP_TERMINATE;
	uh->bulk_out_qh->is_top_queue = 1;

	/* Initialise spin lock */
	spinlock_init(&uh->xfer_qh_lock);

	/* Setup address enumeration */
	uh->next_address = 1;

	/* Interrupt handlers queue head */
	LIST_INIT(&uh->interrupt_handler_list_head);

	/*
	 * Prepare empty Frame List:
	 *  - 1024 entries of Frame List Pointers (FRP) 4 bytes long,
	 *    i.e. 1 4kB page, 4kB page aligned
	 *  - each FRP entry points to the interrupt queue head
	 *    which is the first queue for asynchronous transfers
	 */

	uh->frame_base_addr =
		kzalloc_align(UHCI_FRAME_LIST_SIZE * 4, (1 << 12));
	frame_list_pointer = uh->frame_base_addr;

	for (i = 0; i < UHCI_FRAME_LIST_SIZE; i++)
		*frame_list_pointer++ = UHCI_LP_TERMINATE;

	frame_list_pointer = uh->frame_base_addr;
	for (i = 0; i < UHCI_FRAME_LIST_SIZE; i++)
		*frame_list_pointer++ =
			((uint32_t)uh->interrupt_qh) | UHCI_LP_QH;

	/* Global reset and UHCI specific */
	uhci_pci_write(uh->iobase, UHCI_COMMAND, 0x0004);
	while ((uhci_pci_read(uh->iobase, UHCI_COMMAND) & 0x0002) != 0)
		;

	ms_delay(20);
	uhci_pci_write(uh->iobase, UHCI_COMMAND, 0x0000);


	/* Set UHCI frame base pointer to this empty frame list */
	uhci_pci_write(uh->iobase, UHCI_FRAME_BASE_ADDR,
		       (uint32_t)uh->frame_base_addr);

	/* Clear frame number counter */
	uhci_pci_write(uh->iobase, UHCI_FRAME_NUM, 0);

	/* Enable interrupts on complete */
	uhci_pci_write(uh->iobase, UHCI_INT_EN, 0x0004);

	/* Enable UHCI */
	uhci_pci_write(uh->iobase, UHCI_COMMAND,
		       0x00C1); /* 0x80 64B packets allowed at SOF
				 * 0x40 Configured Flag
				 * 0x01 Run HC
				 */

	/* Initialise root hub */
	root_hub = kzalloc(sizeof(struct usb_hub));
	if (root_hub == NULL)
		return ERR_NO_MEM;

	root_hub->port_num = 2;    /* The total number of ports on this hub */
	root_hub->hc = (void *)uh; /* Host controller of this hub */
	root_hub->hc_ops = &uhci_hc_ops;   /* Host controller operations  */
	root_hub->upstream_hub = NULL;     /* This is root hub */
	root_hub->hub_ops = &uhci_hub_ops; /* Hub operations   */

	usb_hub_register(root_hub);

	DEBUG("UHCI root hub ports %d", root_hub->port_num);

	return 0;
}
