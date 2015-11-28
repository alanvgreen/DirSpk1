/*
 * Initialization and configuration functions
 */ 

#include <asf.h>
#include "log.h"
#include "init.h"

//
//  Configure UART console.
//
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = 115200,
		.paritytype = UART_MR_PAR_NO
	};

	/* Configure console UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONSOLE_UART, &uart_serial_options);
	log_msg("console configured");
}

// Configure the IO pins
static void configure_io_pins(void) {
	// Arduino Due Pin A7 = GPIO A.2: set to be input for AD0
	pmc_enable_periph_clk(ID_PIOA);
	pio_configure_pin(PIO_PA2_IDX, PIO_INPUT);
	
	// Arduino Due Pin D53 = GPIO B.14: set to be PWM2 output
	pmc_enable_periph_clk(ID_PIOB);
	pio_configure_pin(PIO_PB14_IDX, PIO_PERIPH_B);
	log_msg("io_pin_configuration done");
}

//
// PWM Configuration
//
static void configure_pwm(void) {
	// Channel 2: 1 second period, 50% duty cycle
	pwm_channel_disable(PWM, PWM_CHANNEL_2);
	pmc_enable_periph_clk(ID_PWM);
	
	pwm_clock_t clock_setting = {
		.ul_clka = 60000,
		.ul_clkb = 0,
		.ul_mck = 86000000
	};
	pwm_init(PWM, &clock_setting);
	
	pwm_channel_t instance = {
		.channel = PWM_CHANNEL_2,
		.ul_prescaler = PWM_CMR_CPRE_CLKA,
		.alignment = PWM_ALIGN_LEFT,
		.polarity = PWM_LOW,
		.ul_period = 60000,
		.ul_duty = 30000,
	};
	pwm_channel_init(PWM, &instance);
	pwm_channel_enable(PWM, PWM_CHANNEL_2);
	
	log_msg("configure_pwm done");
}

//
// Configure the timer/counter
//
static void configure_timer(void) {
	
	static uint32_t ul_sysclk;
	ul_sysclk = sysclk_get_cpu_hz();
	uint32_t ul_div, ul_tcclks;
	
	// TC0 channel 0 provides a 40kHz clock
	pmc_enable_periph_clk(ID_TC0);
	tc_find_mck_divisor(40000, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC0, 0, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC0, 0, (ul_sysclk / ul_div) / 40000);
	NVIC_SetPriority((IRQn_Type)ID_TC0, 0); // More important (lower number)
	NVIC_EnableIRQ((IRQn_Type)ID_TC0);
	tc_enable_interrupt(TC0, 0, TC_IER_CPCS);
	tc_start(TC0, 0);
	
	// TC0 channel 1  provides a 20Hz clock
	pmc_enable_periph_clk(ID_TC1);
	tc_find_mck_divisor(20, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC0, 1, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC0, 1, (ul_sysclk / ul_div) / 20);
	NVIC_SetPriority((IRQn_Type)ID_TC1, 8); // Less important (higher number)
	NVIC_EnableIRQ((IRQn_Type)ID_TC1);
	tc_enable_interrupt(TC0, 1, TC_IER_CPCS);
	tc_start(TC0, 1);
	
	log_msg("configure_timer done");
}


//
// Configure ADC. PIO A.2 pin = AD0 = Arduino A7
//
static void configure_adc(void) {
	pmc_enable_periph_clk(ID_ADC);
	
	// Set 20MHz ADC clock = 50ns period. 640 cycle startup time ~= 32ns
	// adc_init(ADC, sysclk_get_cpu_hz(), 20 * 1000 * 1000, ADC_STARTUP_TIME_10);
	// Hack : 5 MHz
	adc_init(ADC, sysclk_get_cpu_hz(), 5 * 1000 * 1000, ADC_STARTUP_TIME_10);
	
	adc_configure_timing(
		ADC, 
		4, // Tracking time = (x + 1) periods = 250ns
	    0, // Settling time is unused 
	    1); // Transfer time = (2x + 3) periods = 250ns
		
	// Turn on bias current, since we're doing many conversions
	adc_set_bias_current(ADC, 1);
	
	// Just the one channel
	adc_enable_channel(ADC, ADC_CHANNEL_0);
		
	// Start free run mode
	adc_configure_trigger(ADC, ADC_TRIG_SW, 1);
	adc_start(ADC);
}


//
// Initialization
//
void init(void) {
	sysclk_init();
	board_init();
	configure_console();
	configure_io_pins();
	configure_pwm();
	configure_timer();
	configure_adc();
}