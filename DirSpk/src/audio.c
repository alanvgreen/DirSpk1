// audio.c
// Routines for Audio output.

#include "decls.h"

// 40kHz sampler.
//
// Runs at too high a priority to call FreeRTOS routines.
// This routine executes each (approximately) 2000 cycles. It is important that
// it is not slow.
void PWM_Handler(void) {
	// Read status to indicate that interrupt has been handled
	uint32_t isr = PWM->PWM_ISR1;
	if (isr != (1 << 2)) { // Must be interrupt two
		fatalBlink(6, 1); 
	}
	
	// Temporary code - read ADC, calculate result.
	// TODO: replace with DMA.
	// TODO: handle fading.
	uint32_t data0 = adc_get_channel_value(ADC, 0);
	uint32_t data1 = adc_get_channel_value(ADC, 1);
	uint32_t sum = data0 + data1; // sum is a 13 bit value
	
	// Calculate output value for DACC
	dacc_write_conversion_data(DACC, sum / 2);
	
	// Calculate duty cycle value for PWM Channel 2.
	// Calculated value must be between 0 and US_PERIOD-1, non inclusive.
	//int32_t duty = ((US_PERIOD - 2) * sum / 8192) + 1;
	// TODO: set properly
	int32_t duty = US_PERIOD / 2;
	PWM->PWM_CH_NUM[2].PWM_CDTYUPD = duty;
}