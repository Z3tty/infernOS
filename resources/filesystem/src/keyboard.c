#include "interrupt.h"
#include "kernel.h"
#include "keyboard.h"
#include "mbox.h"
#include "scheduler.h"
#include "util.h"

/*
 * This is the keyboard device driver.  It catches irq1 and read data
 * from the keyboard controller, convert the data into ascii and
 * buffer the character
 */

static void normal_handler(unsigned char scan);
static void escape_handler(unsigned char c);
static void right_shift_handler(unsigned char c);
static void left_shift_handler(unsigned char c);
static void control_handler(unsigned char c);
static void alt_handler(unsigned char c);
static void caps_lock_handler(unsigned char c);
static void num_lock_handler(unsigned char c);
static void scroll_lock_handler(unsigned char c);

/*
 * Shift status bits:
 * [0] Right shift
 * [1] Left shift
 * [2] Caps Lock
 * [3] Control
 * [4] Alt
 * [5..7] Unused
 *
 * These bits are set by the assosiated handler.
 */

static char shift_status = FALSE;
static char key_release = FALSE;
static char multiread = FALSE;

/*
 * scan_to_ascii[SCAN CODE] = {no_shift, shift, control, handler}
 *
 * Assume Q is pressed on the keyboard, the scancode for Q is 0x10
 * scan_to_ascii[10] = {0x71,0x51,0x11,normal_handler}
 * The assosiated handler is normal_handler(),
 * 0x71 = ASCII(q), 0x51 = ASCII(Q), 0x11 = ASCII(something).
 *
 * The Keyboard Codes can be found in The Undocumented PC p.332
 */
struct ascii scan_to_ascii[] = {
    {0x00, 0x00, 0x00, normal_handler},                                                                                                                                                                                               /* 0 */
    {0x1b, 0x1b, 0x1b, escape_handler},      {0x31, 0x21, 0x00, normal_handler}, {0x32, 0x40, 0x00, normal_handler},  {0x33, 0x23, 0x00, normal_handler}, {0x34, 0x24, 0x00, normal_handler},                                         /* 5 */
    {0x35, 0x25, 0x00, normal_handler},      {0x36, 0x5e, 0x1e, normal_handler}, {0x37, 0x26, 0x00, normal_handler},  {0x38, 0x2a, 0x00, normal_handler}, {0x39, 0x28, 0x00, normal_handler},                                         /* a */
    {0x30, 0x29, 0x00, normal_handler},      {0x2d, 0x5f, 0x1f, normal_handler}, {0x3d, 0x2b, 0x00, normal_handler},  {0x08, 0x08, 0x7f, normal_handler}, {0x09, 0x00, 0x00, normal_handler},     {0x71, 0x51, 0x11, normal_handler}, /* 10 */
    {0x77, 0x57, 0x17, normal_handler},      {0x65, 0x45, 0x05, normal_handler}, {0x72, 0x52, 0x12, normal_handler},  {0x74, 0x54, 0x14, normal_handler}, {0x79, 0x59, 0x19, normal_handler},                                         /* 15 */
    {0x75, 0x55, 0x15, normal_handler},      {0x69, 0x49, 0x09, normal_handler}, {0x6f, 0x4f, 0x0f, normal_handler},  {0x70, 0x50, 0x10, normal_handler}, {0x5b, 0x7b, 0x1b, normal_handler},                                         /* 1a */
    {0x5d, 0x7d, 0x1d, normal_handler},      {0x0d, 0x0d, 0x0a, normal_handler}, {0x00, 0x00, 0x00, control_handler}, {0x61, 0x41, 0x01, normal_handler}, {0x73, 0x53, 0x13, normal_handler},     {0x64, 0x44, 0x04, normal_handler}, /* 20 */
    {0x66, 0x46, 0x06, normal_handler},      {0x67, 0x47, 0x07, normal_handler}, {0x68, 0x48, 0x08, normal_handler},  {0x6a, 0x4a, 0x0a, normal_handler}, {0x6b, 0x4b, 0x0b, normal_handler},                                         /* 25 */
    {0x6c, 0x4c, 0x0c, normal_handler},      {0x3b, 0x3a, 0x00, normal_handler}, {0x27, 0x22, 0x00, normal_handler},  {0x60, 0x7e, 0x00, normal_handler}, {0x00, 0x00, 0x00, left_shift_handler},                                     /* 2a */
    {0x5c, 0x7c, 0x00, normal_handler},      {0x7a, 0x5a, 0x1a, normal_handler}, {0x78, 0x58, 0x18, normal_handler},  {0x63, 0x43, 0x03, normal_handler}, {0x76, 0x56, 0x16, normal_handler},     {0x62, 0x42, 0x02, normal_handler}, /* 30 */
    {0x6e, 0x4e, 0x0e, normal_handler},      {0x6d, 0x4d, 0x0d, normal_handler}, {0x2c, 0x3c, 0x00, normal_handler},  {0x2e, 0x2e, 0x00, normal_handler}, {0x2f, 0x3f, 0x00, normal_handler},                                         /* 35 */
    {0x00, 0x00, 0x00, right_shift_handler}, {0x2a, 0x2a, 0x00, normal_handler}, {0x00, 0x00, 0x00, alt_handler},     {0x20, 0x20, 0x02, normal_handler}, {0x00, 0x00, 0x00, caps_lock_handler},                                      /* 3a */
    {0x00, 0x00, 0x00, normal_handler},      {0x00, 0x00, 0x00, normal_handler}, {0x00, 0x00, 0x00, normal_handler},  {0x00, 0x00, 0x00, normal_handler}, {0x00, 0x00, 0x00, normal_handler},     {0x00, 0x00, 0x00, normal_handler}, /* 40 */
    {0x00, 0x00, 0x00, normal_handler},      {0x00, 0x00, 0x00, normal_handler}, {0x00, 0x00, 0x00, normal_handler},  {0x00, 0x00, 0x00, normal_handler}, {0x00, 0x00, 0x00, num_lock_handler},                                       /* 45 */
    {0x00, 0x00, 0x00, scroll_lock_handler}, {0x00, 0x37, 0x00, normal_handler}, {0x00, 0x38, 0x00, normal_handler},  {0x00, 0x39, 0x00, normal_handler}, {0x2d, 0x2d, 0x00, normal_handler},                                         /* 4a */
    {0x00, 0x34, 0x00, normal_handler},      {0x00, 0x35, 0x00, normal_handler}, {0x00, 0x36, 0x00, normal_handler},  {0x2b, 0x2b, 0x00, normal_handler}, {0x00, 0x31, 0x00, normal_handler},     {0x00, 0x32, 0x00, normal_handler}, /* 50 */
    {0x00, 0x33, 0x00, normal_handler},      {0x00, 0x30, 0x00, normal_handler}, {0x00, 0x2e, 0x00, normal_handler},
};

void normal_handler(unsigned char scan) {
	struct character char_read;

	if (key_release == FALSE) {
		char_read.scancode = scan;
		char_read.attribute = shift_status;

		/*
		 * Check shift_status to decide what to return in the
		 * character buffer.
		 * Note: this will not work if two or more "shift" keys are
		 * pressed. Caps Lock + Shift + K returns ASCII(0) not 'k'.
		 */
		switch (shift_status) {
		case LEFT_SHIFT:
		case RIGHT_SHIFT:
		case CAPS_SHIFT:
			char_read.character = scan_to_ascii[scan].shift;
			break;
		case CONTROL:
			char_read.character = scan_to_ascii[scan].control;
			break;
		case 0:
			char_read.character = scan_to_ascii[scan].no_shift;
			break;
		default:
			char_read.character = 0;
			break;
		}
		putchar(&char_read);
	}
}

void escape_handler(unsigned char c) {
	/* Emtpy, we ignore this key */
}

/* Sets right shift bit in shift_status */
void right_shift_handler(unsigned char c) {
	if (key_release == FALSE)
		shift_status |= RIGHT_SHIFT;
	else
		shift_status &= ~RIGHT_SHIFT;
}

/* Sets left shift bit in shift_status */
void left_shift_handler(unsigned char c) {
	if (key_release == FALSE)
		shift_status |= LEFT_SHIFT;
	else
		shift_status &= ~LEFT_SHIFT;
}

/* Sets control bit in shift_status */
void control_handler(unsigned char c) {
	if (key_release == FALSE)
		shift_status |= CONTROL;
	else
		shift_status &= ~CONTROL;
}

/* Sets alt bit in shift_status */
void alt_handler(unsigned char c) {
	if (key_release == FALSE)
		shift_status |= ALT;
	else
		shift_status &= ~ALT;
}

/* Sets caps lock bit in shift_status */
void caps_lock_handler(unsigned char c) {
	if (key_release == FALSE) {
		if (shift_status & CAPS_SHIFT)
			shift_status &= ~CAPS_SHIFT;
		else
			shift_status |= CAPS_SHIFT;
	}
}

void num_lock_handler(unsigned char c) {
	/* Emtpy, we ignore this key */
}

void scroll_lock_handler(unsigned char c) {
	/* Emtpy, we ignore this key */
}

/*
 * This function is called when a keyboard interrupt arrive.  For
 * details refer to The Undocumented PC p. 332
 */
void keyboard_interrupt(void) {
	unsigned char key;

	/* Read key */
	key = inb(0x60);

	/* check if this is a key release or press */
	if (key & 0x80) /* bit 7 set */
		key_release = TRUE;
	else
		key_release = FALSE;

	/* Mask of the release bit */
	key &= 0x7f; /* set bit 7 = zero */

	/* Check if this is a multiread */
	if (key == 0xe0) {
		multiread = TRUE;
		key = inb(0x60);
	}

	/* Call the handler for the key */
	if (key < 0x54) {
		(*scan_to_ascii[key].handler)(key);
	}
}

static int mbox;

void keyboard_init(void) {
	/* Open mailbox QUEUE */
	mbox = mbox_open(QUEUE);
	ASSERT2(mbox >= 0, "mbox_open failed");
}

/*
 * Function to read a character from the keyboard queue.
 * Writes an ASCII character to *c.
 *
 * We need a critical section around getchar because this could happend:
 * Process X has called getchar()
 *
 * getchar() {
 *   ...
 *   mbox_recv() {
 *   lock_acquire(&l);
 *  ...                no context switch
 *  IRQ 1 =================================> irq1() {
 *      ...
 *      putchar() {
 *      ...
 *           mbox_stat() {
 *               lock_acquire(&l)
 *            Process X is blocked while holding lock l.
 */
int getchar(int *c) {
	char space[CHAR_MSG_SIZE];
	msg_t *m = (msg_t *)space;

	/* CS to avoid deadlocks */
	enter_critical();
	mbox_recv(mbox, m);
	leave_critical();

	ASSERT2(m->size == 1, "Invalid message");
	*c = m->body[0];
	return 1;
}

/*
 * Called by the keyboard interrupt (irq1) handler (through
 * normal_handler) to insert an ASCII character in the keyboard
 * queue.
 *
 * We need a C_S because:
 * 1. Process X has acquired the lock L when a keyboard interrupt occurs.
 * 2. Process X is now running inside putchar(), mbox_stat() reports
 *    that there is enough room for one more message.
 * 3. Timer interrupt, process Y is scheduled to run.
 * 4. Keyboard interrupt, process Y is now in putchar(), mbox_stat() reports
 *    that there is enough room for one more mesage, so process Y sends message
 * 5. Timer interrupt, process X is scheduled to run.
 * 6. Mailbox is full, but process X sends a message and is blocked.
 * 7. Process S (the consumer) is scheduled to run, but before it calls
 *    getchar(), it acquires lock L, and is blocked (because Process X holds
 *    lock L).
 * ==> Deadlock
 */
void putchar(struct character *c) {
	int count, available_space;
	char space[CHAR_MSG_SIZE];
	msg_t *m = (msg_t *)space;

	m->size = 1;
	m->body[0] = c->character;

	/* CS to avoid deadlocks */
	enter_critical();
	/*
	 * producer can't block discard character if buffer is full
	 * (assume the user won't type too fast)
	 */
	mbox_stat(mbox, &count, &available_space);
	if (available_space >= CHAR_MSG_SIZE) {
		mbox_send(mbox, m);
	}
	leave_critical();
}
