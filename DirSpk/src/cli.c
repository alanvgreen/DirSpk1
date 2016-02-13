// cli.c

#include <string.h>
#include <asf.h>
#include "cli.h"
#include "init.h"
#include "util.h"

// Constant strings
static char const *WELCOME = " - DirSpk1 Command Server - \r\nType \"help\" for help.\r\n";
static char const *PROMPT = "\r\n> ";
static char const *CRLF = "\r\n";
static char const *SPACE_LAST = "\b ";

// Transmit buffer
#define txBufSize 101
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


// The console task.
static void cliTask(void *pvParameters) {
	consoleWrite(WELCOME);
	cmdBuf[0] = '\0';
	while (1) {
		consoleWrite(PROMPT);
		getCommand();
		consoleWrite(CRLF);
		consoleWrite((char *) cmdBuf);
		consoleWrite(CRLF);
	}
}

//
// Command to show each task
static portBASE_TYPE tasksCommand(
    int8_t *pcWriteBuffer,
    size_t xWriteBufferLen,
    const int8_t *pcCommandString) {
	strcpy((char *)pcWriteBuffer, "Hello\r\n");	
	return pdFALSE;
}

// All the commands to register
static CLI_Command_Definition_t allCommands[] = {
	{
		USTR("tasks"), 
		USTR("tasks:\r\n Lists each task, along with basic stats about the task.\r\n\r\n"),
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