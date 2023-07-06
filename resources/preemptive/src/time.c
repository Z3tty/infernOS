/*
 * time.c
 * Adapted from linux kernel sources:
 * /usr/src/linux/arch/i386/kernel/time.c Note: A number of comments
 * irrelevant to the OS projects (but certainly relevant to Linux!)
 * have been removed from this adaptation, which originally came from
 * Stanislav Sokolov (who got the code from the Linux kernel).
 */

#include "time.h"
#include "util.h"

uint32_t cpu_mhz = 0;

/*
 * ------ Calibrate the TSC -------
 * Return 2^32 * (1 / (TSC clocks per usec)) for
 * do_fast_gettimeoffset().  Too much 64-bit arithmetic here to do
 * this cleanly in C, and for accuracy's sake we want to keep the
 * overhead on the CTC speaker (channel 2) output busy loop as low as
 * possible. We avoid reading the CTC registers directly because of
 * the awkward 8-bit access mechanism of the 82C54 device.
 */
static uint32_t calibrate_tsc(void) {
	uint32_t last_tsc_low; /* lsb 32 bits of Time Stamp Counter */
	if (last_tsc_low && 0) {
		; /* Hack for defined but not used warning */
	}

	/* Set the Gate high, disable speaker */
	outb(0x61, (inb(0x61) & ~0x02) | 0x01);

	/*
	 * Now let's take care of CTC channel 2
	 *
	 * Set the Gate high, program CTC channel 2 for mode 0,
	 * (interrupt on terminal count mode), binary count,
	 * load 5 * LATCH count, (LSB and MSB) to begin countdown.
	 */
	outb(0x43, 0xb0);                   /* binary, mode 0, LSB/MSB, Ch 2 */
	outb(0x42, CALIBRATE_LATCH & 0xff); /* LSB of count */
	outb(0x42, CALIBRATE_LATCH >> 8);   /* MSB of count */

	{
		unsigned long startlow, starthigh;
		unsigned long endlow, endhigh;
		unsigned long count;

		rdtsc(startlow, starthigh);
		count = 0;
		do {
			count++;
		} while ((inb(0x61) & 0x20) == 0);
		rdtsc(endlow, endhigh);

		last_tsc_low = endlow;

		/* Error: ECTCNEVERSET */
		if (count <= 1) {
			print_str(0, 0, "ectneverset");
			goto bad_ctc;
		}

		/* 64-bit subtract - gcc just messes up with long longs */
		__asm__("subl %2,%0\n\t"
		        "sbbl %3,%1"
		        : "=a"(endlow), "=d"(endhigh)
		        : "g"(startlow), "g"(starthigh), "0"(endlow), "1"(endhigh));

		/* Error: ECPUTOOFAST */
		if (endhigh) {
			print_str(0, 0, "too fast");
			goto bad_ctc;
		}

		/* Error: ECPUTOOSLOW */
		if (endlow <= CALIBRATE_TIME) {
			print_str(0, 0, "too slow");
			goto bad_ctc;
		}

		__asm__("divl %2" : "=a"(endlow), "=d"(endhigh) : "r"(endlow), "0"(0), "1"(CALIBRATE_TIME));
		return endlow;
	}

	/*
	 * The CTC wasn't reliable: we got a hit on the very first read,
	 * or the CPU was so fast/slow that the quotient wouldn't fit in
	 * 32 bits..
	 */
bad_ctc:
	return 0;
}

void time_init(void) {
	uint32_t cpu_khz, /* Detected as we calibrate the TSC */
	    tsc_quotient = 0;

	tsc_quotient = calibrate_tsc();
	if (tsc_quotient) {
		/*
		 * report CPU clock rate in Hz.
		 * The formula is (10^6 * 2^32) / (2^32 * 1 / (clocks/us)) =
		 * clock/second. Our precision is about 100 ppm.
		 */
		unsigned long eax = 0, edx = 1000;
		__asm__("divl %2" : "=a"(cpu_khz), "=d"(edx) : "r"(tsc_quotient), "0"(eax), "1"(edx));
		cpu_mhz = cpu_khz / 1000;
	}
	else
		cpu_mhz = 500; /* Default to 500 MHz */
}
