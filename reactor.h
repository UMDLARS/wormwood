#pragma once
#include <limits.h>
#include <stdbool.h>

#define MAX_FLOW_RATE 100.0

#define REACTOR_MAX_DEPTH 16
#define REACTOR_UNSAFE_DEPTH REACTOR_MAX_DEPTH + 1
_Static_assert(REACTOR_MAX_DEPTH >= 1, "Max depth must be at least 1.");
_Static_assert(REACTOR_MAX_DEPTH <= CHAR_MAX, "Max depth cannot be greater than CHAR_MAX.");

#define REACTOR_SAFE_TEMP 2000
#define REACTOR_WARNING_TEMP 3000
#define REACTOR_WARNING_TEMP_2 4000
#define REACTOR_EXPLODE_TEMP 5000

typedef enum {
	usermode_none = 0,
	usermode_oper,
	usermode_super,
	usermode_count
} usermode_t;

typedef struct {
	unsigned int temp_error : 1;
	unsigned int rupture_error :1;
} warnings_t;

typedef struct {
	usermode_t usermode;
	warnings_t warns;
	float temp;
	float coolant_flow;
	float coolant_temp;
	unsigned char rod_depth;
	bool safety_enabled;
	bool safety_active;
} reactor_state_t;

/*
 * [usermode_none]		= "NA"
 * [usermode_oper]		= "oper"
 * [usermode_super]		= "super"
 */
extern const char* const g_usermode_str[usermode_count];

bool reactor_is_rod_depth_safe(char depth);

void reactor_init(reactor_state_t* state);

void reactor_update(reactor_state_t* state);

void reactor_check_warns(reactor_state_t* state);
