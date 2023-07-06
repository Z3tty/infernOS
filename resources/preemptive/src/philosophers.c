#include "scheduler.h"
#include "screen.h"
#include "thread.h"
#include "util.h"

/* Dining philosphers threads. */

enum
{
	THINK_TIME = 9999,
	EAT_TIME = THINK_TIME,
};

volatile int forks_initialized = 0;
semaphore_t fork[3];
int num_eating = 0;
int scroll_eating = 0;
int caps_eating = 0;
/* Set to true if status should be printed to screen */
int print_to_screen;

enum
{
	LED_NONE = 0x00,
	LED_SCROLL = 0x01,
	LED_NUM = 0x02,
	LED_CAPS = 0x04,
	LED_ALL = 0x07
};

/* Turns keyboard LEDs on or off according to bitmask.
 *
 * Bitmask is composed of the following three flags:
 * 0x01 -- SCROLL LOCK LED enable flag
 * 0x02 -- NUM LOCK LED enable flag
 * 0x04 -- CAPS LOCK LED enable flag
 *
 * Bitmask = 0x00 thus disables all LEDS, while 0x07
 * enables all LEDS.
 *
 * See http://www.computer-engineering.org/ps2keyboard/
 * and http://forum.osdev.org/viewtopic.php?t=10053
 */
static void update_keyboard_LED(unsigned char bitmask) {
	/* Make sure that bitmask only contains bits for status LEDs  */
	bitmask &= 0x07;

	/* Wait for keyboard buffer to be empty */
	while (inb(0x64) & 0x02)
		;
	/* Tells the keyboard to update LEDs */
	outb(0x60, 0xed);
	/* Wait for the keyboard to acknowledge LED change message */
	while (inb(0x60) != 0xfa)
		;
	/* Write bitmask to keyboard */
	outb(0x60, bitmask);

	ms_delay(100);
}

static void think_for_a_random_time(void) {
	volatile int foo;
	int i, n;

	n = rand() % THINK_TIME;
	for (i = 0; i < n; i++)
		if (foo % 2 == 0)
			foo++;
}

static void eat_for_a_random_time(void) {
	volatile int foo;
	int i, n;

	n = rand() % EAT_TIME;
	for (i = 0; i < n; i++)
		if (foo % 2 == 0)
			foo++;
}

/* Odd philosopher */
void num(void) {
	print_to_screen = 1;

	/* Initialize semaphores */
	semaphore_init(&fork[0], 1);
	semaphore_init(&fork[1], 1);
	semaphore_init(&fork[2], 1);
	forks_initialized = 1;
	if (print_to_screen) {
		print_str(PHIL_LINE, PHIL_COL, "Phil.");
		print_str(PHIL_LINE + 1, PHIL_COL, "Running");
	}

	while (1) {
		think_for_a_random_time();

		/* Grab left fork... */
		semaphore_down(&fork[0]);
		/* ... and grab right fork */
		semaphore_down(&fork[1]);

		/* Enable NUM-LOCK LED and disable the others */
		update_keyboard_LED(LED_NUM);

		num_eating = 1;
		/* With three forks only one philosopher at a time can eat */
		ASSERT(scroll_eating + caps_eating == 0);

		if (print_to_screen) {
			print_str(PHIL_LINE, PHIL_COL, "Phil.");
			print_str(PHIL_LINE + 1, PHIL_COL, "Num    ");
		}

		eat_for_a_random_time();

		num_eating = 0;

		/* Release forks */
		semaphore_up(&fork[0]);
		semaphore_up(&fork[1]);
	}
}

void caps(void) {
	/* Wait until num hasd initialized forks */
	while (forks_initialized == 0)
		yield();

	while (1) {
		think_for_a_random_time();

		/* Grab right fork... */
		semaphore_down(&fork[2]);
		/* ... and grab left fork */
		semaphore_down(&fork[1]);

		/* Enable CAPS-LOCK LED and disable the others */
		update_keyboard_LED(LED_CAPS);

		caps_eating = 1;
		/* With three forks only one philosopher at a time can eat */
		ASSERT(scroll_eating + num_eating == 0);

		if (print_to_screen) {
			print_str(PHIL_LINE, PHIL_COL, "Phil.");
			print_str(PHIL_LINE + 1, PHIL_COL, "Caps   ");
		}

		eat_for_a_random_time();

		caps_eating = 0;

		/* Release forks */
		semaphore_up(&fork[2]);
		semaphore_up(&fork[1]);
	}
}

void scroll_th(void) {
	/* Wait until num hasd initialized forks */
	while (forks_initialized == 0)
		yield();

	while (1) {
		think_for_a_random_time();

		/* Grab right fork... */
		semaphore_down(&fork[0]);
		/* ... and grab left fork */
		semaphore_down(&fork[2]);

		/* Enable SCROLL-LOCK LED and disable the others */
		update_keyboard_LED(LED_SCROLL);

		scroll_eating = 1;
		/* With three forks only one philosopher at a time can eat */
		ASSERT(caps_eating + num_eating == 0);

		if (print_to_screen) {
			print_str(PHIL_LINE, PHIL_COL, "Phil.");
			print_str(PHIL_LINE + 1, PHIL_COL, "Scroll ");
		}

		eat_for_a_random_time();

		scroll_eating = 0;

		/* Release forks */
		semaphore_up(&fork[0]);
		semaphore_up(&fork[2]);
	}
}
