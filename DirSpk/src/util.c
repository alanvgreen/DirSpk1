/*
* util.c
*/

#include <asf.h>
#include "util.h"

// Do a pattern of blinks
static void blinkPattern(short initDelay, short count, short offTime, short onTime) {
	ioport_set_pin_level(LED0_GPIO, false);
	delay_ms(initDelay);
	for (short i = 0; i < count ; i++) {
		ioport_set_pin_level(LED0_GPIO, false);
		delay_ms(offTime);
		ioport_set_pin_level(LED0_GPIO, true);
		delay_ms(onTime);
	}
}

// Blink out a pattern in death.
void fatalBlink(short longBlinks, short shortBlinks) {
	for (;;) {
		blinkPattern(1000, 20, 25, 25);
		blinkPattern(600, longBlinks, 400, 600);
		blinkPattern(1000, 15, 25, 25);
		blinkPattern(400, shortBlinks, 600, 100);
	}
}
