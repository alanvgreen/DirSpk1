
/*
* Include header files for all drivers that have been imported from
* Atmel Software Framework (ASF).
*/
#include <asf.h>
#include <string.h>
#include "log.h"
#include "init.h"

// global vars

// 20Hz ticks
static uint32_t ticks = 0;
static uint16_t min_adc = 0;
static uint16_t max_adc = 0;
static uint16_t min_dac = 0;
static uint16_t max_dac = 0;

// Main loop. Executed every tick of TC1.
static void loop(void) {
	if (ticks % 10 == 0) {
		printf("adc min/max = %4d/%4d, dac min/max = %4d/%4d ,range = %d\r\n", 
			min_adc, max_adc, min_dac, max_dac, max_dac - min_dac);
		max_adc = max_dac = 0;
		min_adc = min_dac = 0xffff;
	}
	if (ticks % 10 == 0) {
		pio_toggle_pin(PIO_PB27_IDX);
	}
}

//
// Interrupts
//
// TC0: 40kHz Highest priority ticker
void TC0_Handler(void) {
	// Read ID_TC0 Status to indicate that interrupt has been handled
	tc_get_status(TC0, 0);
	
	// Get the ADC value - record max and min
	uint16_t ch0 = (uint16_t) adc_get_channel_value(ADC, ADC_CHANNEL_0);
	if (ch0 < min_adc) {
		min_adc = ch0;
	}
	if (ch0 > max_adc) {
		max_adc = ch0;
	}
	
	// Write DAC value for audio output
	// Assume 3300 is max ADC input
	const uint32_t adc_limit = 4095;
	uint16_t dac_value = ((uint32_t) ch0) * 4096 / adc_limit;
	if (dac_value < min_dac) {
		min_dac = dac_value;
	}
	if (dac_value > max_dac) {
		max_dac = dac_value;
	}
	dacc_write_conversion_data(DACC, dac_value);
	
	// Ultrasonic output
	// TODO: think about this some more
	int32_t duty = ((int32_t) ch0) * US_PERIOD / adc_limit;
	if (duty >= US_PERIOD) {
		duty = US_PERIOD - 1;
	}
	PWM->PWM_CH_NUM[2].PWM_CDTYUPD = duty;
}


// TC1: 20Hz ticker
void TC1_Handler(void) {
	// Read ID_TC1 Status to indicate that interrupt has been handled
	tc_get_status(TC0, 1);
	ticks++;
	loop();
}


int main(void)
{
	init();
	
	printf("init done\r\n");
	while (1) {
	}
}
