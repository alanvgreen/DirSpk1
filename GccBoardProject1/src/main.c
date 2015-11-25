
/*
* Include header files for all drivers that have been imported from
* Atmel Software Framework (ASF).
*/
/*
* Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
*/
#include <asf.h>
#include <string.h>
#include "log.h"

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
	// Arduino Due Pin 53 = GPIO B.14: set to be PWM2 output
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
static void configure_timer() {
	
	pmc_enable_periph_clk(ID_TC0);
	
}

//
// Initialization
//
static void init(void) {
	sysclk_init();
	board_init();
	configure_console();
	configure_io_pins();
	configure_pwm();
	configure_timer();
}

int main(void)
{
	init();
	
	while (1) {
		delay_s(1);
		log_msg("1 second");
	}
}
