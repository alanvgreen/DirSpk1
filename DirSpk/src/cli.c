// cli.c

#include "decls.h"

// Constant strings
static char const *WELCOME = " - DirSpk1 Command Server - \r\nType \"help\" for help.\r\n";
static char const *PROMPT = "\r\n> ";
static char const *END_COMMAND =  "\r\n*end*\r\n";
static char const *CRLF = "\r\n";
static char const *SPACE_LAST = "\b ";
static char const *TASKS_HEADER =  "Task          State  Priority  Stack	#\r\n************************************************\r\n";
static char const *MSG_CHANNEL_NOT_VALID = "Channel param must be between 0 and 15.\r\n";
static char const *MSG_SAMPLES_NOT_VALID = "Samples param must be between 1 and 10000.\r\n";
static char const *MSG_SECONDS_NOT_VALID = "Seconds param must be between 0 and 30.\r\n";
static char const *MSG_ENCODER_NOT_VALID = "Encoder param must be between 0 and 3.\r\n";
static char const *MSG_SPI_BUSY = "SPI mutex is busy. Please Try again.\r\n";
static char const *MSG_SPI_MUTEX_ERROR = "Could not release SPI mutex.\r\n";
static char const *MSG_POT_ADDR_NOT_VALID = "Addr param must be between 0 and 0xf.\r\n";
static char const *MSG_POT_VALUE_NOT_VALID = "Value param must be between 0 and 0x1ff.\r\n";
static char const *MSG_OK = "OK";
static char const *MSG_ERROR = "*** ERROR ***";
static char const *MSG_Y = "Y";
static char const *MSG_N = "N";

static char const POT_REG_NAMES[6][7] = {
	"R0    ",
	"R1    ",
	"NVR0  ",
	"NVR1  ",
	"TCON  ",
	"STATUS"
};

static char const *LABEL_ENC_NONE = "---";
static char const *LABEL_ENC_CW = "CW ";
static char const *LABEL_ENC_CCW = "CCW";

// Transmit buffer
#define txBufSize 1024
static uint8_t txBuf[txBufSize];

// Command buffer
// Assumed to be smaller than txBuf
#define cmdBufSize 81
static uint8_t cmdBuf[cmdBufSize];

// How long to wait for a transmit
#define IO_WAIT_MS 500

// Write whatever is in txBuf
static void consoleWriteTxBuf(void) {
	status_code_t rc = freertos_uart_write_packet(
	    freeRTOSUART, txBuf, strlen((char *) txBuf), MS_TO_TICKS(IO_WAIT_MS));
	ASSERT_BLINK(rc == STATUS_OK, 3, 2);
	txBuf[0] = '\0';
}

// Write str to CLI
static void consoleWrite(char const *str) {
	// TODO: deal with str > 100 chars
	// String copied to buffer since freertos_uart cannot write from flash
	strncpy((char *) txBuf, str, txBufSize - 1);
	txBuf[txBufSize - 1] = '\0';
	consoleWriteTxBuf();
}

// Fetches a command to execute
static void getCommand(void) {
	uint8_t charIn;
	uint8_t cIdx = 0;  // index into cmdBuf
	uint8_t lastIdx;
	while (1) {
		lastIdx = cIdx;
		// Wait until a character is available
		uint32_t readCount = freertos_uart_serial_read_packet(
		    freeRTOSUART, &charIn, 1, IO_WAIT_MS);
		if (readCount == 0) {
			continue;
		}
		// Handle the character
		if (charIn == '\r' || cIdx == (txBufSize - 1)) {
		    // Hitting empty on first char repeats the previous command
			return;
		} else if (charIn == '\n') {
		    // Ignore newlines
		} else if (charIn == '\b' || charIn == '\x7f') {
			if (cIdx > 0) {
			    cIdx--;
			}
		} else {
			// Get this character
			cmdBuf[cIdx++] = charIn;
		}
		cmdBuf[cIdx] = '\0';
		
		// Redisplay the line
		consoleWrite(SPACE_LAST);
		int i;
		for (i = 0; i < lastIdx; i++) {
			txBuf[i] = '\b';
		}
		txBuf[i] = '\0';
		consoleWriteTxBuf();
		for (i = 0; i < cIdx; i++) {
			txBuf[i] = cmdBuf[i];
		}
		txBuf[i] = '\0';
		consoleWriteTxBuf();
	}
}

static const char *encoderLabel(EncoderDirection d) {
	switch (d) {
		case ENC_CW: return LABEL_ENC_CW;
		case ENC_CCW: return LABEL_ENC_CCW;
		default: return LABEL_ENC_NONE;
	}
}

static void writeDetailedEncoderState(int n) {
	EncoderState *s = GLOBAL_STATE.encoders + n;
	snprintf((char*) txBuf, txBufSize, "%s, %d, %hhx\r\n", 
	    encoderLabel(s->dir), (int) s->dirTicks, s->state);
	consoleWriteTxBuf();
}

// Write the current global state
static void writeGlobalStateSummary(void) {
	snprintf((char*) txBuf, txBufSize, "UIQ(%s), Vol(%d, %d)\r\n",
		GLOBAL_STATE.uiQueueFullFlag ? MSG_ERROR : MSG_OK,
		GLOBAL_STATE.gain0,
		GLOBAL_STATE.gain1);
	consoleWriteTxBuf();
}

// The console task.
static void cliTask(void *pvParameters) {
	consoleWrite(WELCOME);
	cmdBuf[0] = '\0';
	while (1) {
		writeGlobalStateSummary();
		consoleWrite(PROMPT);
		getCommand();
		consoleWrite(CRLF);
		consoleWrite(CRLF);
		portBASE_TYPE more;
		do {
			more = FreeRTOS_CLIProcessCommand((int8_t *) cmdBuf, (int8_t *) txBuf, txBufSize);
			consoleWriteTxBuf();
		} while (more);
		consoleWrite(END_COMMAND);
	}
}

// Command to show stats about each task
static portBASE_TYPE tasksCommand(
    int8_t *pcWriteBuffer,
    size_t xWriteBufferLen,
    const int8_t *pcCommandString) {
	
	// First time through, show the header	
	static bool shownHeader = false;
	if (!shownHeader) {
		strcpy((char *) pcWriteBuffer, TASKS_HEADER);
		shownHeader = true;
		return pdTRUE;
	}
	vTaskList(pcWriteBuffer);
	shownHeader = false;
	return pdFALSE;
}

// Iterates forward through p to find the start of the next parameter
static int8_t const *findNextParam(int8_t const *p) {
	while (*p && *p != ' ') p++;
	while (*p && *p == ' ') p++;
	return p;
}

// Parse a positive integer from p. Return -1 on failure;
static int parseInt(int8_t const *p, int base) {
	int8_t *end;
	long val = strtol((char const *) p, (char **) &end, base);

	if (p == end) {
		return -1; // nothing to convert
	}
	if (val == LONG_MAX) {
		return -1; // Big number or conversion error
	}
	if (val < 0) {
		return -1; // negative number
	}
	if (*end != '\0' && !isspace(*end)) {
		return -1; // parameter not fully parsed
	}
	return (int) val;
}

// Command to dump values from the ADC
static portBASE_TYPE adcDumpCommand(
	int8_t *pcWriteBuffer,
	size_t xWriteBufferLen,
	const int8_t *pcCommandString) {
	
	// Channel param
	int8_t const *p = findNextParam(pcCommandString);
	int chan = parseInt(p, 0);
	if (chan < 0 || chan > 15) {
		consoleWrite(MSG_CHANNEL_NOT_VALID);
		return pdFALSE;
	}

	// Samples param
	p = findNextParam(p);
	int samples = parseInt(p, 0);
	if (samples < 1 || samples > 10000) {
		consoleWrite(MSG_SAMPLES_NOT_VALID);
		return pdFALSE;		
	}
	
	// Set test mode
	adc_disable_all_channel(ADC);
	adc_enable_channel(ADC, chan);
	
	int i;
	for (i = 0; i < samples; i++) {
		vTaskDelay(MS_TO_TICKS(2));
		uint16_t val = adc_get_channel_value(ADC, chan);
		snprintf((char *) txBuf, txBufSize, "%hd\r\n", val);
		consoleWriteTxBuf();
	}
	return pdFALSE;
}

// Command to track encoder behavior
static portBASE_TYPE encoderTrackCommand(
	int8_t *pcWriteBuffer,
	size_t xWriteBufferLen,
	const int8_t *pcCommandString) {
		
	// encoder
	int8_t const *p = findNextParam(pcCommandString);
	
	int enc = parseInt(p, 0);
	if (enc < 0 || enc > 3) {
		consoleWrite(MSG_ENCODER_NOT_VALID);
		return pdFALSE;
	}	
	
	// scan through encoder then whitespace to seconds param
	p = findNextParam(p);
	int seconds = parseInt(p, 0);
	if (seconds < 0 || seconds > 30) {
		consoleWrite(MSG_SECONDS_NOT_VALID);
		return pdFALSE;
	}
	uint32_t endTicks = xTaskGetTickCount() + (seconds * 1000);
	
	while (xTaskGetTickCount() < endTicks) {
		writeDetailedEncoderState(enc);
		vTaskDelay(20);
	}
	return pdFALSE;
}


// Command to listen to the encoders
static portBASE_TYPE encoderListenCommand(
int8_t *pcWriteBuffer,
size_t xWriteBufferLen,
const int8_t *pcCommandString) {
	
	// scan through command then whitespace to seconds param
	int8_t const *p = findNextParam(pcCommandString);
	int limitSeconds = parseInt(p, 0);
	if (limitSeconds < 0 || limitSeconds > 30) {
		consoleWrite(MSG_SECONDS_NOT_VALID);
		return pdFALSE;
	}
	
	EncoderMove move;
	for (int seconds = 0; seconds < limitSeconds; seconds++) {
		snprintf((char *) txBuf, txBufSize, "%d:\r\n", seconds);
		consoleWriteTxBuf();
		uint32_t endTicks = xTaskGetTickCount() + 1000;
		while (xTaskGetTickCount() < endTicks) {
			uint32_t ticks = endTicks - xTaskGetTickCount();
		    if (xQueueReceive(GLOBAL_STATE.encoderDebugQueue, &move, ticks)) {
				snprintf((char *) txBuf, txBufSize, "  %d - %s\r\n", 
				    move.num, encoderLabel(move.dir));
				consoleWriteTxBuf();
			}
		}
	}
	
	return pdFALSE;
}

// Command to dump spi pot
static portBASE_TYPE potDumpCommand(
int8_t *pcWriteBuffer,
size_t xWriteBufferLen,
const int8_t *pcCommandString) {
	// Take the mutex
	if (!xSemaphoreTake(GLOBAL_STATE.spiMutex, 1000)) {
		consoleWrite(MSG_SPI_BUSY);
		return pdFALSE;
	}
	
	// Iterate through all addresses
	for (int8_t addr = 0; addr < 0x10 ; addr++) {
		// PCS = 0 so no bits to set. Put Addr in high four bits, followed by 11.
		uint32_t datum = (addr << 12) + (3 << 10);
		uint32_t result = spiSendReceive(datum);
		if (result & 0x200) {
			// Show buffer name and value
			snprintf((char *) txBuf, txBufSize, "0x%02x: 0x%03lx (0x%04lx)", 
			    addr, result & 0x1ff, result);
			if (addr <= 5) {
				int len = strnlen((char *) txBuf, txBufSize);
				char *txp = (char *) txBuf + len;
				int txlen = txBufSize - len;
				if (addr < 4) {
					snprintf(txp, txlen, " %s: %3ld", POT_REG_NAMES[addr], result & 0x1ff);
				} else if (addr == 4) {
					snprintf(txp, txlen, " %s: R1 %s, R0 %s",
					    POT_REG_NAMES[addr],
						(result & 0xf0) == 0xf0 ? MSG_OK : MSG_ERROR,
						(result & 0x0f) == 0x0f ? MSG_OK : MSG_ERROR);
				} else if (addr == 5) {
					snprintf(txp, txlen, " %s: EEWA=%s, WL1=%s, WL0=%s, SHDN=%s, WP=%s",
						POT_REG_NAMES[addr],
						result & 0x10 ? MSG_Y : MSG_N,
						result & 0x08 ? MSG_Y : MSG_N,
						result & 0x04 ? MSG_Y : MSG_N,
						result & 0x02 ? MSG_Y : MSG_N,
						result & 0x01 ? MSG_Y : MSG_N);
				}
			}
			strncat((char *) txBuf, CRLF, txBufSize);
		} else {
			snprintf((char *) txBuf, txBufSize, "0x%02x: ERR (0x%04lx)\r\n", addr, result);
		}
		consoleWriteTxBuf();
	}
	
	if (!xSemaphoreGive(GLOBAL_STATE.spiMutex)) {
		consoleWrite(MSG_SPI_MUTEX_ERROR);
		return pdFALSE;
	}
	
	return pdFALSE;
}

// put a value into the pot memory
static portBASE_TYPE potSetCommand(
int8_t *pcWriteBuffer,
size_t xWriteBufferLen,
const int8_t *pcCommandString) {
	
	// scan through command, then through whitespace to address
	int8_t const *p = findNextParam(pcCommandString);
	int addr = parseInt(p, 16);
	if (addr < 0 || addr > 15) {
		consoleWrite(MSG_POT_ADDR_NOT_VALID);
		return pdFALSE;
	}
	
	// scan through address then whitespace to value
	p = findNextParam(p);
	int val = parseInt(p, 16);
	if (val < 0 || val > 0x1ff) {
		consoleWrite(MSG_POT_VALUE_NOT_VALID);
		return pdFALSE;
	}
	
	// Take the mutex
	if (!xSemaphoreTake(GLOBAL_STATE.spiMutex, 1000)) {
		consoleWrite(MSG_SPI_BUSY);
		return pdFALSE;
	}
	
	// Put Addr in high four bits of packet, 
	uint32_t datum = (addr << 12) + val;
	uint32_t result = spiSendReceive(datum);
	snprintf((char *) txBuf, txBufSize, "Write 0x%04lx (0x%02x = 0x%03x: %04lx)\r\n", 
	    datum, addr, val, result);
	consoleWriteTxBuf();
	
	// Return Mutex
	if (!xSemaphoreGive(GLOBAL_STATE.spiMutex)) {
		consoleWrite(MSG_SPI_MUTEX_ERROR);
		return pdFALSE;
	}
	
	return pdFALSE;
}

// All the commands to register
static CLI_Command_Definition_t allCommands[] = {
	{
		USTR("adcdump"), 
		USTR("adcdump c n: Dump n samples from ADC channel c.\r\n"),
		adcDumpCommand,
		2
	},
	{
		USTR("enctrack"),
		USTR("enctrack n s: Track encoder n for s seconds.\r\n"),
		encoderTrackCommand,
		2
	},
	{
		USTR("enc"),
		USTR("enc s: Listen to debug encoders for s seconds.\r\n"),
		encoderListenCommand,
		1
	},
	{
		USTR("pot"),
		USTR("pot: Lists value of all pot registers.\r\n"),
		potDumpCommand,
		0
	},
	{
		USTR("potset"),
		USTR("potset r v: Sets pot register r (hex) to value v (hex).\r\n"),
		potSetCommand,
		2
	},
	{
		USTR("tasks"),
		USTR("tasks: Lists each task, along with basic stats about the task.\r\n"),
		tasksCommand,
		0
    },
	{ NULL, NULL, NULL, -1 },
};

// Kick off the CLI
void startCli(void) {
	CLI_Command_Definition_t *c = allCommands;
	for (;c->pcCommand != NULL; c++) {
		FreeRTOS_CLIRegisterCommand(c);	
	}
	
	portBASE_TYPE t = xTaskCreate(
	    cliTask,
		USTR("CLI"), 
		configMINIMAL_STACK_SIZE * 4, 
		NULL,
		tskIDLE_PRIORITY + 1, 
		NULL);
	ASSERT_BLINK(t, 3, 1);
}