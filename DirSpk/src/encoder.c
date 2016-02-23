// encoder.c
// 
// Handles rotary encoders on PC12-PC19

#include <asf.h>
#include "encoder.h"


// To turn on interrupts
#if 0

static void handler(int a, int b) {
	// read PIOC pins from scratch
}


static void setUp(void) {
	NVIC_SetPriority((IRQn_Type)ID_PIOC, 0x0e); // 2nd lowest interrupt priority
	NVIC_EnableIRQ((IRQn_Type)ID_PIOC);
	// Could not work around using the unhelpful PIOC_Handler that comes with ASF
	// At least it properly reads and clears status bits
    pio_handler_set(PIOC, ID_PIOC, pc1219, 0, handler);
}
#endif