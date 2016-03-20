// audio.c
// Routines for Audio output.

#include "decls.h"

// The current mode
static volatile AudioMode mode;

// The volume (used to scale output in AM_HZ mode)
volatile uint8_t audioVolume;

// The current frequency (or zero for off). Used in AM_HZ mode.
static uint32_t currFreq;


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
	
	if (mode == AM_ADC) {
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
		
		// Get TIOA value from status register
		bool mtioa = TC0->TC_CHANNEL[0].TC_SR & TC_SR_MTIOA;
		
		// Write to DACC
		int16_t dDelta = 2048 * audioVolume / 255;
		if (!mtioa) {
			dDelta = -dDelta;
		}
		dacc_write_conversion_data(DACC, dDelta + 2048);
		
		// Set PWM
		int16_t pDelta = (US_PERIOD - 2) / 2 * audioVolume / 255;
		if (!mtioa) {
			pDelta = -pDelta;
		}
		PWM->PWM_CH_NUM[2].PWM_CDTYUPD = pDelta + (US_PERIOD / 2);
	}
}


// Set the frequency of the generated tone. 0 means off.
void audioFrequencySet(uint32_t freq) {
	// In order to avoid audio hiccups, don't do anything if setting 
	// same frequency. 
	if (currFreq == freq) {
		return;
	}
	
	tc_stop(TC0, 0);
	if (freq == 0) {
		return;
	}
	
	// Find the best divisor for this frequency
	uint32_t ul_div, ul_tcclks;
	tc_find_mck_divisor(freq, SystemCoreClock, 
		&ul_div, &ul_tcclks, SystemCoreClock);
		
	// Put Timer into wavesel up RC mode with TIOA at 50% duty cycle
	// Clear TIOA at CPC match and set at CPA match
	tc_init(TC0, 0, ul_tcclks | TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_ACPC_CLEAR | TC_CMR_ACPA_SET);
	uint16_t rcVal = (SystemCoreClock / ul_div) / freq;
	tc_write_rc(TC0, 0, rcVal);
	tc_write_ra(TC0, 0, rcVal / 2);  // 50% duty cycle
	
	// Start the thing
	tc_start(TC0, 0);
	
	currFreq = freq;
}
	

// Set the audio mode
void audioModeSet(AudioMode m) {
	// Don't do anything if mode is not changed
	if (mode == m) {
		return;
	}
	
	// Set new mode
	mode = m;
	audioFrequencySet(0); // Always reset currFreq on mode change
	switch (m) {
		case AM_OFF:
			pwm_channel_disable(PWM, PWM_CHANNEL_2);
			break;
		case AM_ADC:
			pwm_channel_enable(PWM, PWM_CHANNEL_2);
		case AM_HZ:
			pwm_channel_enable(PWM, PWM_CHANNEL_2);
			break;
	}
}
