//
// Dirspk1 Main
//

#include <asf.h>
#include "cli.h"
#include "init.h"
#include "util.h"

// The tick hook
extern void vApplicationTickHook(void);
void vApplicationTickHook(void) {
	// do nothing
}

int main(void) {
	// Initialize peripherals etc
	init();
	
	// Start the CLI
	startCli();
	
	// Start Task scheduler
	vTaskStartScheduler();
	
	// Should not get here - task scheduler ought not return
	fatalBlink(5, 1);
}