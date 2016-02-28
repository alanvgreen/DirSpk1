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
	
	// Temporary code - read ADC, write DAC
	uint32_t data0 = adc_get_channel_value(ADC, 0);
	uint32_t data1 = adc_get_channel_value(ADC, 1);
	dacc_write_conversion_data(DACC, (data1 + data0) / 2);
	
	// Get a value
	// Look at audio mode
	
	// Always output to both PWM and DAC
}