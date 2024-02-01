#pragma once
#include <stdbool.h>

#define DEBUG 0
#define FUNCNAME if (DEBUG) console_printf("In %s @ line %d...\n", __func__, __LINE__);

#define MAX_SAFE_DEPTH 16
#define MAX_FLOW_RATE 100.0

typedef enum {
	exit_reason_none = 0,
	exit_reason_fail,
	exit_reason_quit,
} exit_reason_t;

extern exit_reason_t exit_reason;
extern char rod_depth;
extern float coolant_flow;
extern float coolant_temp;
extern float reactor_temp;
extern int usermode;
extern bool safety_enabled;
extern bool safety_active;

extern const char *users[];
