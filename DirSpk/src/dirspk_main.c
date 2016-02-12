/*
 * Dirspk1 Main
 */ 

#include <asf.h>
#include "init.h"

int main(void) {
	// Initialze peripherals etc
	init();
	
	// Blink out a 5/5 patterns
	fatalBlink(5, 5);
}