
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

// Main loop. Executed every tick of TC1.
static void loop(void) {
	static uint16_t last_min = 0;
	static uint16_t last_max = 0;
	if (ticks % 10 == 0) {
		printf("min = %4d, max = %4d, range = %d, min_diff = %4d, max_diff = %4d\r\n", 
			min_adc, max_adc, max_adc - min_adc, min_adc - last_min, max_adc - last_max);
		last_min = min_adc;
		last_max = max_adc;
		max_adc = 0;
		min_adc = 0xffff;
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
