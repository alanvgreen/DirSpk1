// init.c
//
// General device initialization

#include "decls.h"

//
// Initialize pins for GPIO or peripherals
//
static void initGpio(void) {
	ioport_init();
	
	// PA2 = A7 = ADC channel 0
	ioport_set_pin_dir(PIO_PA2_IDX, IOPORT_DIR_INPUT);
	
	// PA25,26,27,28,29 = MISO, MOSI, SCK, SPI0_CS0, SPI0_CS1 = D74, D75, D76, D77, D87
	// On the Due D77 and D10 share a pin, as Do D87 and D4
	const int spiPins = 0x1f << 25;
	ioport_set_port_mode(IOPORT_PIOA, spiPins, IOPORT_MODE_MUX_A);
	ioport_disable_port(IOPORT_PIOA, spiPins);
	
	// PB15 = DAC0
	ioport_set_pin_mode(PIO_PB15_IDX, IOPORT_MODE_MUX_B);
	ioport_disable_pin(PIO_PB15_IDX);
	
	// PIOB - PB27 = D13 = LED
	ioport_set_pin_level(LED0_GPIO, false);
	ioport_set_pin_dir(LED0_GPIO, IOPORT_DIR_OUTPUT);
	
	// PIOC - PC6,7 = D38,39 = PWML2 and PWMH2 
	const int pwmPins = 0x3 << 6;
	ioport_set_port_mode(IOPORT_PIOC, pwmPins, IOPORT_MODE_MUX_B);
	ioport_disable_port(IOPORT_PIOC, pwmPins);
	
	// PIOC - PC4 = D36 = SD H-bridge - pull high to turn on
	ioport_set_pin_dir(PIO_PC4_IDX, IOPORT_DIR_OUTPUT);
	
	// PIOC - PC12-PC19 = D51-D44 for encoders
	// Uses pio_ API since an interrupt handler is needed
	pio_set_input(PIOC, ENCODER_PINS, PIO_PULLUP);
	//pio_set_debounce_filter(PIOC, ENCODER_PINS, 500); // De-bounce at 500Hz
	pio_handler_set(PIOC, ID_PIOC, ENCODER_PINS, 0, encoderPIOCHandler);
	// Not quite the lowest priority
	pio_handler_set_priority(PIOC, ID_PIOC, configLIBRARY_LOWEST_INTERRUPT_PRIORITY - 1);
	pio_enable_interrupt(PIOC, ENCODER_PINS);
	
	// TODO: put current sense on NMI
	// TODO: add display wait, int
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
	
	// Enable channels 0 and 1
	adc_enable_channel(ADC, ADC_CHANNEL_0);
	adc_enable_channel(ADC, ADC_CHANNEL_1);
	
	// Turn on temp sensor (channel 15)
	// adc_enable_ts(ADC);
	
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
		.interrupt_priority = configLIBRARY_LOWEST_INTERRUPT_PRIORITY,
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
	ASSERT_BLINK(freeRTOSUART, 1, 7);
}

// Initialize TC0 channel 0 for tone generation
static void initTimers(void) {
 	pmc_enable_periph_clk(ID_TC0);
// 	
// 	uint32_t ul_sysclk = sysclk_get_cpu_hz(); 
// 	uint32_t ul_div, ul_tcclks;
	
	// TC0 channel 0 provides a 40kHz clock to power audio out.
	// on RC compare, triggers a high priority interrupt
	/* TODO: remove this when ready
	tc_find_mck_divisor(40000, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC0, 0, ul_tcclks | TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC);
	tc_write_rc(TC0, 0, (ul_sysclk / ul_div) / 40000);
	NVIC_SetPriority((IRQn_Type)ID_TC0, 1); // Highest priority (lowest number) except for NMI
	NVIC_EnableIRQ((IRQn_Type)ID_TC0);
	tc_enable_interrupt(TC0, 0, TC_IER_CPCS);
	tc_start(TC0, 0);
	*/
	
	// TC0 channel 1  provides a variable frequency clock for generating audible 
	// tones up to 10kHz. The audio generation task is responsible for configuring it,
	// and it is read via the MTIOA bit.
	/* TODO: figure out how we will generate tones.
	pmc_enable_periph_clk(ID_TC1);
	tc_find_mck_divisor(10000, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC0, 1, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC0, 1, (ul_sysclk / ul_div) / 10000);
	tc_start(TC0, 1);
	*/
}

// Init PWM
static void initPwm(void) {
	pmc_enable_periph_clk(ID_PWM);
	
	// Channel 2: 40kHz period, 50% duty cycle
	pwm_channel_disable(PWM, PWM_CHANNEL_2);
	pmc_enable_periph_clk(ID_PWM);
		
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
	// TODO: ensure explicitly enabled during ui bringup, and remove this
	pwm_channel_enable(PWM, PWM_CHANNEL_2);
		
	// Enable interrupts
	PWM->PWM_IER1 = (1 << 2);
	NVIC_SetPriority((IRQn_Type)ID_PWM, 1); // Highest priority (lowest number) except for NMI
	NVIC_EnableIRQ((IRQn_Type)ID_PWM);
}


// Deal with the DAC
static void initDac(void) {
	pmc_enable_periph_clk(ID_DACC);
	dacc_reset(DACC);
    
	// 16 bit transfer mode
	dacc_set_transfer_mode(DACC, 0);  
	dacc_set_timing(DACC,
		0x08, // refresh = 8 - though we send new data more often than this
		0, // not max speed
		0x10); // startup = 640 periods
	dacc_set_channel_selection(DACC, 0); // Have to revisit if have dual outputs
	dacc_set_analog_control(DACC,
		DACC_ACR_IBCTLCH0(0x02) |
		DACC_ACR_IBCTLCH1(0x02) |
		DACC_ACR_IBCTLDACCORE(0x01));
	dacc_enable_channel(DACC, 0);
}

// Set up SPI controller 0
static void initSpi0(void) {
	pmc_enable_periph_clk(ID_SPI0);
	// Enable
	SPI0->SPI_CR = 0x1; 
	
	// Master, variable select, no decode chip select, fault detection,
	// no wait read before transfer.
	// DLYBCS = 25 MCK cycles. FOR RA8825, must be > 5 RA8825 clock cycles, which
	// could be as slow as 20MHz
	SPI0->SPI_MR = (25 << 24) + 0x23; 
	
	// SPI Chip 0 (NCP4261)
	// No delay between consecutive transfers. 
	// (Though keep in mind MCP4261 requires ~10ms delay after writing NVRAM)
	// 16 bit transfers.
	// TODO: set to 10MHz
	uint32_t f = 1 * 1000 * 1000; // 1MHz
	int32_t clk = div_ceil(sysclk_get_cpu_hz(), f);
	int32_t dlybct = div_ceil(sysclk_get_cpu_hz(), f * 32); // wait for a clock cycle before release cs
	// DLYBCT = dlybct * 32, DLYBS = 0, SCBR = clk, 16 bit transfers,  NCPHA = 1, CPOL = 0
	SPI0->SPI_CSR[0] = (dlybct << 24) + (clk << 8) + (8 << 4) + 2;
	
	// SPI Chip 1 (RA8875)
	// TODO: set to 20MHz for write and 10MHz for read. 
	// See RA8875 datasheet section 6-1-2-2 for timing info. Will need to set
	// RA8875 clock to 60MHz first
	f = 1 * 1000 * 1000; // 1MHz
	clk = div_ceil(sysclk_get_cpu_hz(), f); // 1MHz (approx)
	// NCPHA = 1, CPOL = 1
	SPI0->SPI_CSR[1] = (clk << 8) + (8 << 4) + 1;
}

void init(void) {
	sysclk_init();
	NVIC_SetPriorityGrouping(0);
	board_init();
	
	initGpio();
	initAdc();
	initUart();
	initTimers();
	initPwm();
	initDac();
	initSpi0();
}