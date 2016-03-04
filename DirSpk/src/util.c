/*
* util.c
*/

#include "decls.h"

// Stack overflow - blink out a pattern
extern void vApplicationStackOverflowHook( xTaskHandle, signed char *);
void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName) {
	fatalBlink(10, 1);
}

// Malloc failed - blink out a pattern
extern void vApplicationMallocFailedHook( void );
void vApplicationMallocFailedHook(void) {
	fatalBlink(10, 2);
}

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

// Blink out a pattern for warnings
void errorBlink(short longBlinks, short shortBlinks) {
	blinkPattern(1000, 20, 25, 25);
	blinkPattern(600, longBlinks, 400, 600);
	blinkPattern(700, 10, 25, 25);
	blinkPattern(400, shortBlinks, 600, 100);
	
	ioport_set_pin_level(LED0_GPIO, false);
}

// Blink out a pattern in death.
void fatalBlink(short longBlinks, short shortBlinks) {
	// Turn off all interrupt handling
	__set_FAULTMASK(1);
	
	// Blink the lights
	for (;;) {
		errorBlink(longBlinks, shortBlinks);
	}
}
