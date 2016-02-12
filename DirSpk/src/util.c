/*
* util.c
*/

#include <asf.h>
#include "util.h"

// Blink out a pattern in death.
void fatalBlink(short longBlinks, short shortBlinks) {
	
	for (;;) {
		ioport_set_pin_level(LED0_GPIO, false);
		delay_ms(500);
		// Long blinks
		for (short n = 0; n < longBlinks; n++) {
			ioport_set_pin_level(LED0_GPIO, false);
			delay_ms(100);
			ioport_set_pin_level(LED0_GPIO, true);
			delay_ms(400);
		}
		ioport_set_pin_level(LED0_GPIO, false);
		delay_ms(500);
		// Short blinks
		for (short n = 0; n < shortBlinks; n++) {
			ioport_set_pin_level(LED0_GPIO, false);
			delay_ms(400);
			ioport_set_pin_level(LED0_GPIO, true);
			delay_ms(100);
		}
	}
}
