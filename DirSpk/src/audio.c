// audio.c
// Routines for Audio output.

#include "decls.h"

// TODO: implement Audio Mode / Value
// The current mode
AudioMode audioMode;

// The value to output - range from 1 to US_PERIOD - 2, inclusive
volatile uint16_t audioValue;

// 40kHz sampler.
//
// Runs at too high a priority to call FreeRTOS routines.
// This routine executes each (approximately) 2000 cycles. It is important that
// it is not slow.
void PWM_Handler(void) {
	// Read status to indicate that interrupt has been handled
	uint32_t isr = PWM->PWM_ISR1;
	if (isr != (1 << 2)) { // Must be interrupt two
		fatalBlink(1, 6);
	}
	
	if (audioMode == AM_ADC) {
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
	
		int32_t duty = ((US_PERIOD - 2) * sum / 8192) + 1;
		// For testing with silence
		// int32_t duty = US_PERIOD / 2;
		PWM->PWM_CH_NUM[2].PWM_CDTYUPD = duty;	
	} else { // Must be AM_VALUE
		dacc_write_conversion_data(DACC, audioValue);
		int32_t duty = ((US_PERIOD - 2) * audioValue / 4096) + 1;
		PWM->PWM_CH_NUM[2].PWM_CDTYUPD = duty;
	}
}

// Set the audio mode
void audioModeSet(AudioMode m) {
	audioMode = m;
	if (m == AM_OFF) {
		// Shutdown PWM
		pwm_channel_disable(PWM, PWM_CHANNEL_2);
	} else {
		// Ensure PWM is on
		pwm_channel_enable(PWM, PWM_CHANNEL_2);
	}
}
