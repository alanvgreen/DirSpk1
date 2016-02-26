// state.c

#include <asf.h>
#include "state.h"
#include "util.h"

// Maximum amount of time to wait to send a command before giving up
#define MAX_QUEUE_WAIT_MS MS_TO_TICKS(1000)

// The Global State
GlobalState GLOBAL_STATE;

