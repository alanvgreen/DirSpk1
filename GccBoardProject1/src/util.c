/*
 * util.c
 */ 

#include <asf.h>

// Blink out a pattern in death.
void fatalBlink(short longBlinks, short shortBlinks) {
	
	for (;;) {
		// Long blinks
		for (short n = 0; n < longBlinks; n++) {
			ioport_set_pin_level(LED0_GPIO, false);
			delay_ms(200);
			ioport_set_pin_level(LED0_GPIO, true);
			delay_ms(800);
		}
		// Short blinks
		for (short n = 0; n < shortBlinks; n++) {
			ioport_set_pin_level(LED0_GPIO, false);
			delay_ms(800);
			ioport_set_pin_level(LED0_GPIO, true);
			delay_ms(200);
		}
	}
}
