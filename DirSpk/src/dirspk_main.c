//
// Dirspk1 Main
//

#include <asf.h>
#include "cli.h"
#include "encoder.h"
#include "init.h"
#include "spi.h"
#include "state.h"
#include "ui.h"
#include "util.h"

// The tick hook
extern void vApplicationTickHook(void);
void vApplicationTickHook(void) {
	// do nothing
}

int main(void) {
	// Initialize peripherals etc
	init();
	
	// Start each task/subsystem
	startCli();
	startEncoders();
	startSpi();
    startUi();
	
	// Start Task scheduler
	vTaskStartScheduler();
	
	// Should not get here - task scheduler ought not return
	fatalBlink(5, 1);
}