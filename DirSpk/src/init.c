// init.c
//
// General device initialization

#include <asf.h>
#include "init.h"
#include "util.h"


static void initGpio(void) {
	ioport_init();
	
	// PIOB - PB27
	ioport_set_pin_level(LED0_GPIO, false);
	ioport_set_pin_dir(LED0_GPIO, IOPORT_DIR_OUTPUT);
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
	initUart();
}