// audio.c
// Routines for Audio output.
#include <asf.h>

//
// Utility function to indicate interrupt handled
static __attribute__((optimize("O0"))) inline void clearTcBit(void) {
	TC0->TC_CHANNEL[0].TC_SR;
}

// 40kHz sampler.
//
// Runs at too high a priority to call FreeRTOS routines.
// This routine executes each (approximately) 2000 cycles. It is important that
// it is not slow.
void TC0_Handler(void) {
	// Read status to indicate that interrupt has been handled
	clearTcBit();
	
	// Get a value
	// Look at audio mode
	
	// Always output to both PWM and DAC
}