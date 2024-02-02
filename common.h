#pragma once
#include <stdbool.h>

#define DEBUG 0
#define FUNCNAME if (DEBUG) console_printf("In %s @ line %d...\n", __func__, __LINE__);

typedef enum {
	exit_reason_none = 0,
	exit_reason_fail,
	exit_reason_quit,
} exit_reason_t;

extern exit_reason_t exit_reason;

extern const char *users[];
