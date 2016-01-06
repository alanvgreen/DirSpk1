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
	
	// Arduino Due Pin DAC0 = GPIO B.15
	pmc_enable_periph_clk(ID_PIOB);
	pio_configure_pin(PIO_PB15_IDX, PIO_PERIPH_B);
	// Arduino Due Pin D13 = GPIO B.27
	pio_configure_pin(PIO_PB27_IDX, PIO_OUTPUT_1);
		
	// Arduino Due Pin D38/39 = GPIO C.6/7: set to be PWM2 output L/H
	pmc_enable_periph_clk(ID_PIOC);
	pio_configure_pin(PIO_PC6_IDX, PIO_PERIPH_B);
	pio_configure_pin(PIO_PC7_IDX, PIO_PERIPH_B);
}

//
// PWM Configuration
//
static void configure_pwm(void) {
	// Channel 2: 40kHz period, 50% duty cycle
	pwm_channel_disable(PWM, PWM_CHANNEL_2);
	pmc_enable_periph_clk(ID_PWM);
	
	// Prescaling not required
	//pwm_clock_t clock_setting = {
		//.ul_clka = 0,
		//.ul_clkb = 0,
		//.ul_mck = sysclk_get_cpu_hz();
	//};
	//pwm_init(PWM, &clock_setting);
	
	// Configure channel 2 
	pwm_channel_t instance = {
		.channel = PWM_CHANNEL_2,
		.ul_prescaler = PWM_CMR_CPRE_MCK, // 84MHz
		.alignment = PWM_ALIGN_LEFT,
		.polarity = PWM_LOW,
		.ul_period = US_PERIOD,
		.ul_duty = US_PERIOD / 2,
	};
	pwm_channel_init(PWM, &instance);
	pwm_channel_enable(PWM, PWM_CHANNEL_2);
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
// Configure DAC0. PIO B.17 = AD0 = Arduino D66/DAC0
//
static void configure_dac(void) {
	pmc_enable_periph_clk(ID_DACC);
	dacc_reset(DACC);
	dacc_set_transfer_mode(DACC, 0);  // 16 bit transfer mode
	dacc_set_timing(DACC, 
	    0x08, // refresh = 8 - though we send new data more often than this
	    0, // not max speed
	    0x10); // startup = 640 periods
	dacc_set_channel_selection(DACC, 0); // Have to revisit when have dual inputs
	dacc_enable_channel(DACC, 0); 
	dacc_set_analog_control(DACC,
		DACC_ACR_IBCTLCH0(0x02) | 
		DACC_ACR_IBCTLCH1(0x02) |
		DACC_ACR_IBCTLDACCORE(0x01));
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
	configure_dac();
	log_msg("init done\r\n");
}