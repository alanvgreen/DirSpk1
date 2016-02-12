//
// Dirspk1 Main
//

#include <asf.h>
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
	
	// Blink out pattern
	fatalBlink(5, 1);
}