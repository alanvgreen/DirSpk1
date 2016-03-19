//
// Dirspk1 Main
//

#include "decls.h"

// The tick hook
extern void vApplicationTickHook(void);
void vApplicationTickHook(void) {
	// do nothing
}

int main(void) {
	// Initialize peripherals etc
	init();
	
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