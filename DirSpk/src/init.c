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
	
	// PA2 = A7 = ADC channel 0
	ioport_set_pin_dir(PIO_PA2_IDX, IOPORT_DIR_INPUT);
	
	// PB15 = DAC0
	ioport_set_pin_mode(PIO_PB15_IDX, IOPORT_MODE_MUX_B);
	
	// PIOB - PB27 = D13 = LED
	ioport_set_pin_level(LED0_GPIO, false);
	ioport_set_pin_dir(LED0_GPIO, IOPORT_DIR_OUTPUT);
	
	// PIOC - PC12-PC19 = D51-D44 for encoders
	uint32_t pc1219 = 0x000ff000;
	PIOC->PIO_PUER = pc1219; // enable pullups
	PIOC->PIO_PER = pc1219; // GPIO, no peripheral
	PIOC->PIO_ODR = pc1219; // Not driven
	PIOC->PIO_OWDR = pc1219; // Do not allow writes to ODSR
	PIOC->PIO_DIFSR = pc1219; // Choose debounce filter
	PIOC->PIO_SCDR = 32768 / 500; // Debounce = 500Hz stops a lot of slop.
	PIOC->PIO_IFER = pc1219; // enable debounce
	PIOC->PIO_IER = pc1219; // Standard level change interrupt
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

// Initialize TC0 channel 0 and 1
static void initTimers() {
	pmc_enable_periph_clk(ID_TC0);
	
	uint32_t ul_sysclk = sysclk_get_cpu_hz(); 
	uint32_t ul_div, ul_tcclks;
	
	// TC0 channel 0 provides a 40kHz clock to power audio out.
	// on RC compare, triggers a high priority interrupt
	tc_find_mck_divisor(40000, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC0, 0, ul_tcclks | TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC);
	tc_write_rc(TC0, 0, (ul_sysclk / ul_div) / 40000);
	NVIC_SetPriority((IRQn_Type)ID_TC0, 1); // Most important (lowest number)
	NVIC_EnableIRQ((IRQn_Type)ID_TC0);
	tc_enable_interrupt(TC0, 0, TC_IER_CPCS);
	tc_start(TC0, 0);
	
	// TC0 channel 1  provides a variable frequency clock for generating audible 
	// tones up to 10kHz. The audio generation task is responsible for configuring it,
	// and it is read via the MTIOA bit.
	pmc_enable_periph_clk(ID_TC1);
	tc_find_mck_divisor(10000, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC0, 1, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC0, 1, (ul_sysclk / ul_div) / 10000);
	tc_start(TC0, 1);
}


// Deal with the DAC
void initDac(void) {
	pmc_enable_periph_clk(ID_DACC);
	dacc_reset(DACC);
    
	// 16 bit transfer mode
	dacc_set_transfer_mode(DACC, 0);  
	dacc_set_timing(DACC,
		0x08, // refresh = 8 - though we send new data more often than this
		0, // not max speed
		0x10); // startup = 640 periods
	dacc_set_channel_selection(DACC, 0); // Have to revisit when have dual inputs
	dacc_set_analog_control(DACC,
		DACC_ACR_IBCTLCH0(0x02) |
		DACC_ACR_IBCTLCH1(0x02) |
		DACC_ACR_IBCTLDACCORE(0x01));
	dacc_disable_channel(DACC, 0);
}

void init(void) {
	sysclk_init();
	NVIC_SetPriorityGrouping(0);
	board_init();
	
	initGpio();
	initAdc();
	initUart();
	initTimers();
	initDac();
}