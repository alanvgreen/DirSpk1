// state.c

#include <asf.h>
#include "state.h"
#include "util.h"

// Maximum amount of time to wait to send a command before giving up
#define MAX_QUEUE_WAIT_MS MS_TO_TICKS(1000)

// The Global State
GlobalState GLOBAL_STATE = {
	.lastCommandTicks = 0,
	.adcMode = ADC_NONE,
};

static xQueueHandle commandQueue;

// Command to modify the global state
void modifyGlobalState(GlobalModCommand *command) {
	portBASE_TYPE t = xQueueSend(commandQueue, command, MAX_QUEUE_WAIT_MS);
	ASSERT_BLINK(t == pdTRUE, 4, 4);
}

// Set up the ADC
static void setAdc(GlobalModCommand *command) {
	if (command->adcMode == ADC_NONE) {
		return;
	}
	
	GLOBAL_STATE.adcMode = command->adcMode;
	adc_disable_all_channel(ADC);
	adc_enable_channel(ADC, command->adcTestChannel);
}

// Set up the DACC
static void setDac(GlobalModCommand *command) {
	if (command->dacMode == OO_NONE) {
		return;
	}
	
	GLOBAL_STATE.dacMode = command->dacMode;
	dacc_enable(DACC);
}
	
// The global state task.
static void globalStateTask(void *pvParameters) {
	GlobalModCommand buf;
	
	for (;;) {
		portBASE_TYPE t = xQueueReceive(commandQueue, &buf, portMAX_DELAY);
		ASSERT_BLINK(t, 4, 3);
		GLOBAL_STATE.lastCommandTicks = xTaskGetTickCount();
		setAdc(&buf);
		setDac(&buf);
	}
}

// Start the task for dealing with global state
void startGlobalState(void) {
	commandQueue = xQueueCreate(5, sizeof(GlobalModCommand));
	ASSERT_BLINK(commandQueue, 4, 1);
	
	portBASE_TYPE t = xTaskCreate(
		globalStateTask,
		USTR("Glob"),
		configMINIMAL_STACK_SIZE * 4,
		NULL,
		configMAX_PRIORITIES - 1, // One less than the timer service task
		NULL);
	ASSERT_BLINK(t, 4, 2);
	
}