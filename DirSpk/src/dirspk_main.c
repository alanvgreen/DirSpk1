//
// Dirspk1 Main
//

#include "decls.h"

// The tick hook
extern void vApplicationTickHook(void);
void vApplicationTickHook(void) {
	// do nothing
}

// Liveness callback
static void aliveCallback(xTimerHandle pxTimer) {
	ioport_toggle_pin_level(LED0_GPIO);
}



int main(void) {
	// Initialize peripherals etc
	init();
	
	// Start a timer to blink D13 LED to confirm aliveness	
	xTimerHandle timer = xTimerCreate(USTR("ALIVE"), MS_TO_TICKS(2000), true, NULL, aliveCallback);
	xTimerStart(timer, 0);
	
	// Start each task/subsystem
	startEncoders();
	startSpi();
	startScreen();
	startUi();
	startCli();
	
	// Start Task scheduler
	vTaskStartScheduler();
	
	// Should not get here - task scheduler ought not return
	fatalBlink(1, 9);
}