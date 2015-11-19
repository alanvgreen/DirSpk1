
/*
* Include header files for all drivers that have been imported from
* Atmel Software Framework (ASF).
*/
/*
* Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
*/
#include <asf.h>
#include <string.h>

/**
 *  Configure UART console.
 */
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = 115200,
		.paritytype = UART_MR_PAR_NO
	};

	/* Configure console UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONSOLE_UART, &uart_serial_options);
}

int main (void)
{
	sysclk_init();
	board_init();
	configure_console();
	
	puts("A\n");
	
	// Define Due Pin 53 = GPIOB.14 to output high for one second
	pmc_enable_periph_clk(ID_PIOB);
	pio_set_output(PIOB, PIO_PB14, HIGH, DISABLE, DISABLE);
	puts("B\n");
	delay_s(1);
	puts("C\n");
	
	pio_set_output(PIOB, PIO_PB14, LOW, DISABLE, DISABLE);
	
	puts("D\n");
	
	// Now Define PB14 to peripheral B = PWMH2
	pmc_enable_periph_clk(ID_PWM);
	puts("E\n");
	pwm_channel_disable(PWM, PWM_CHANNEL_2);
	puts("F\n");
	
	if (pio_configure_pin(PIO_PB14_IDX, PIO_PERIPH_B)) puts( "OK");
	
	puts("G\n");
	
	pwm_clock_t clock_setting = {
		.ul_clka = 60000,
		.ul_clkb = 0,
		.ul_mck = 86000000
	};
	if (0 == pwm_init(PWM, &clock_setting)) puts("OK");
	puts("H\n");

	pwm_channel_t instance;
	memset(&instance, 0, sizeof(pwm_channel_t));
	instance.channel = PWM_CHANNEL_2;
	instance.ul_prescaler = PWM_CMR_CPRE_CLKA;
	instance.alignment = PWM_ALIGN_LEFT;
	instance.polarity = PWM_LOW;
	instance.ul_period = 600;
	instance.ul_duty = 300;
	
	if (0 == pwm_channel_init(PWM, &instance)) puts("OK");
	pwm_channel_enable(PWM, PWM_CHANNEL_2);
	
	puts("I\n");
	
	while (1) {
		delay_s(1);
		puts("J\n");
	}
}

