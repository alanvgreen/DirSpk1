// init.c
//
// General device initialization

#include <asf.h>
#include "init.h"
#include "util.h"

//
// Initialize pins for GPIO or peripherals
//
static void initGpio(void) {
	ioport_init();
	
	// PIOA - PA2 = A7 = ADC channel 0
	ioport_set_pin_dir(PIO_PA2_IDX, IOPORT_DIR_INPUT);
	
	// PIOB - PB27 = D13 = LED
	ioport_set_pin_level(LED0_GPIO, false);
	ioport_set_pin_dir(LED0_GPIO, IOPORT_DIR_OUTPUT);
}

//
// Initialize ADC
// See http://www.djerickson.com/arduino/ for information about timing
//
static void initAdc(void) {
	pmc_enable_periph_clk(ID_ADC);
	
	// Set 20MHz ADC clock = 50ns period. 640 cycle startup time ~= 32ns
	adc_init(ADC, sysclk_get_cpu_hz(), 20 * 1000 * 1000, ADC_STARTUP_TIME_10);
	
	// From datasheet, TRACKTIM = 0, TRANSFER = 1
	adc_configure_timing(ADC, 0, 0, 1);
	
	// Turn on bias current, since we're doing many conversions
	adc_set_bias_current(ADC, 1);
	
	// Not enabling any channels just yet, but here's how to do it.
	// adc_enable_channel(ADC, ADC_CHANNEL_0);
	
	// Turn on temp sensor (channel 15)
	adc_enable_ts(ADC);
	
	// Start free run mode
	adc_configure_trigger(ADC, ADC_TRIG_SW, 1);
	adc_start(ADC);
}


//
// Initialize the FreeRTOS UART for the USB CLI
//
freertos_uart_if freeRTOSUART;

static void initUart(void) {
#define rxBufSize 100
	static uint8_t rxBuf[rxBufSize];
	
	// UART already initialized by board_init()
	freertos_peripheral_options_t driver_options = {
		.receive_buffer = rxBuf,
		.receive_buffer_size = rxBufSize,
		.interrupt_priority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,
		.operation_mode = UART_RS232,
		.options_flags = WAIT_TX_COMPLETE | USE_TX_ACCESS_MUTEX,
	};

	sam_uart_opt_t uart_settings = {
		.ul_mck = sysclk_get_peripheral_hz(),
		.ul_baudrate = 115200,
		.ul_mode = UART_MR_PAR_NO,
	};

	/* Initialise the UART interface. */
	freeRTOSUART = freertos_uart_serial_init(
		UART,
		&uart_settings,
		&driver_options);
	ASSERT_BLINK(freeRTOSUART, 2, 1);
}

void init(void) {
	sysclk_init();
	NVIC_SetPriorityGrouping(0);
	board_init();
	
	initGpio();
	initAdc();
	initUart();
}