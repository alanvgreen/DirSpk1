// cli.c

#include <ctype.h>
#include <string.h>
#include <asf.h>
#include "cli.h"
#include "init.h"
#include "state.h"
#include "util.h"

// Constant strings
static char const *WELCOME = " - DirSpk1 Command Server - \r\nType \"help\" for help.\r\n";
static char const *PROMPT = "\r\n> ";
static char const *END_COMMAND =  "\r\n*end*\r\n";
static char const *CRLF = "\r\n";
static char const *SPACE_LAST = "\b ";
static char const *TASKS_HEADER =  "Task          State  Priority  Stack	#\r\n************************************************\r\n";
static char const *CHANNEL_NOT_VALID = "Channel must be between 0 and 15\r\n";
static char const *SAMPLES_NOT_VALID = "Samples must be between 1 and 10000\r\n";
static char const *SECONDS_NOT_VALID = "Seconds must be between 0 and 30\r\n";
static char const *ENCODER_NOT_VALID = "Encoder must be between 0 and 3\r\n";

static char const *LABEL_ENC_NONE = "NUN";
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
	    encoderLabel(s->dir), (int) s->dirTicks, s->machine);
	consoleWriteTxBuf();
}

// Write the current global state
static void writeGlobalStateSummary(void) {
	snprintf((char*) txBuf, txBufSize, "\r\n");
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

// Parse a positive intenter from p. Return -1 on failure;
static int parseInt(int8_t const *p) {
	if (!isdigit(*p)) {
		return -1;
	}
	return atoi((char const *)p);
}

// Command to dump values from the ADC
static portBASE_TYPE adcDumpCommand(
	int8_t *pcWriteBuffer,
	size_t xWriteBufferLen,
	const int8_t *pcCommandString) {
	
	int8_t const *p = pcCommandString;
	
	// scan through command, then through whitespace to channel param
	while (*p && !isspace(*p)) p++;
	while (*p && isspace(*p)) p++;
	int chan = parseInt(p);
	if (chan < 0 || chan > 15) {
		consoleWrite(CHANNEL_NOT_VALID);
		return pdFALSE;
	}
	
	// scan through channel then through whitespace to n param
	while (*p && !isspace(*p)) p++;
	while (*p && isspace(*p)) p++;
	int samples = parseInt(p);
	if (samples < 1 || samples > 10000) {
		consoleWrite(SAMPLES_NOT_VALID);
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
		
	int8_t const *p = pcCommandString;
	
	// scan through command, then through whitespace to encoder
	while (*p && !isspace(*p)) p++;
	while (*p && isspace(*p)) p++;
	int enc = parseInt(p);
	if (enc < 0 || enc > 3) {
		consoleWrite(ENCODER_NOT_VALID);
		return pdFALSE;
	}	
	
	// scan through encoder then whitespace to seconds param
	while (*p && !isspace(*p)) p++;
	while (*p && isspace(*p)) p++;
	int seconds = parseInt(p);
	if (seconds < 0 || seconds > 30) {
		consoleWrite(SECONDS_NOT_VALID);
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
	int8_t const *p = pcCommandString;
	
	// scan through command then whitespace to seconds param
	while (*p && !isspace(*p)) p++;
	while (*p && isspace(*p)) p++;
	int limitSeconds = parseInt(p);
	if (limitSeconds < 0 || limitSeconds > 30) {
		consoleWrite(SECONDS_NOT_VALID);
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