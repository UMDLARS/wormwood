#include <stdlib.h>
#include "reactor.h"
#include "console_win.h"
#include "common.h"

const char* const g_usermode_str[usermode_count] = {
	[usermode_none]		= "NA",
	[usermode_oper]		= "oper",
	[usermode_super]	= "super"
};

/* Default reactor state. */
static const reactor_state_t g_default_state = {
	.usermode		= usermode_none,
	.warns			= {0,0},
	.temp			= 70.0,
	.coolant_flow	= 10.0,
	.coolant_temp	= 70.0,
	.rod_depth		= REACTOR_UNSAFE_DEPTH - 1,
	.safety_enabled	= true,
	.safety_active	= false,
};

static float _float_up_to(int max) {
	// https://stackoverflow.com/questions/13408990/how-to-generate-random-float-number-in-c
	float fuzz = (float)rand()/(float)(RAND_MAX/max);
	return fuzz;
}

static float _get_fuzz(void) { return _float_up_to(1); }

static int _rand_sign(void) {
	int sign;
	int randval = rand();
	if (randval % 2 == 0) {
		sign = 1;
	}
	else {
		sign = -1;
	}

	return sign;
}

static void _print_sparks(void) {
	int i = CONSOLE_WIN_W * 3;
	int j = 0;
	while (i > 0) {
		for (j = (rand() % 10); j > 0 && i > 1; j--) {
			console_printf(" ");
			i--;
		}
		console_printf("*");
		i--;
	}

	console_printf("\n");
}

static void _update_safety(reactor_state_t* state) {
	/* If rod depth is below the upper limit, increment it. */
	unsigned char new_depth = state->rod_depth + 1;
	if (reactor_is_rod_depth_safe(new_depth)) {
		state->rod_depth = new_depth;
	}

	/* If flow rate is below the upper limit, increase it. */
	if (state->coolant_flow <= MAX_FLOW_RATE) {
		state->coolant_flow = state->coolant_flow + 1;
		if (state->coolant_flow > MAX_FLOW_RATE) {
			state->coolant_flow = MAX_FLOW_RATE;
		}
	}
}

bool reactor_is_rod_depth_safe(unsigned char depth) { return depth <= REACTOR_UNSAFE_DEPTH; }

void reactor_init(reactor_state_t* state) { *state = g_default_state; }

void reactor_update(reactor_state_t* state) {

	/* DO NOT UPDATE THIS CODE */

	/* This may shock you, but this is not a real nuclear reactor.
	 * the physics of this "simulation" are not even remotely accurate...
	 * in fact, I'm just making them up.
	 * But, for the purposes of this activity, pretend that there is
	 * real danger or stakes. It's more engaging and interesting that way!
	 */

	/* COOLANT FLOW HEAT REDUCTION. */
	/* Coolant flow reduces reactor temp. */
	if (state->temp > 70) {
		state->temp = state->temp - ((state->coolant_flow * _float_up_to(7)));
	}

	/* Coolant temp follows the reactor temp, but at a delay. */
	state->coolant_temp = state->coolant_temp + ((state->temp - state->coolant_temp) * .15);

	/* Reactor cannot get below room temp. */
	if (state->temp < 70) {
		state->temp = 70;
	}

	/* Fuzz the reactor temp a bit for realism. */
	state->temp = state->temp + _get_fuzz() * _rand_sign();

	/* RETRACTING THE RODS INCREASES THE TEMP */
	/* for each unit the rod is not fully extracted, add a random float up to 50. */
	float bump = 0;
	int i;
	int rod_factor = REACTOR_UNSAFE_DEPTH - 1 - state->rod_depth;

	for (i = rod_factor; i > 0; i--) {
		bump = bump + _float_up_to(20);
	}
	state->temp = state->temp + bump;

	/* SAFETY PROTOCOLS */
	if (state->safety_enabled == true && state->temp > REACTOR_SAFE_TEMP) {
		/* Activate and perform safety. */
		state->safety_active = true;
		_update_safety(state);
	}

	if (state->safety_active == true && state->temp <= REACTOR_SAFE_TEMP) {
		state->safety_active = false;
	}

	/* Fail/throw error if reactor temp goes over 5K. */
	if (state->temp >= REACTOR_EXPLODE_TEMP) {
		state->warns.temp_error = true;
		exit_reason = exit_reason_fail;
		console_interrupt();
		return;
	}

	/* Fail/throw error if rod depth is too high. */
	if(state->rod_depth >= REACTOR_UNSAFE_DEPTH) { // add <0 check if we go back to signed
		state->warns.rupture_error = true;
		exit_reason = exit_reason_fail;
		console_interrupt();
	}
}

void reactor_check_warns(reactor_state_t* state) {
	/* Check if we've overheated. */
	if(state->warns.temp_error) {
		console_clear();
		_print_sparks();
		console_printf("****** COOLANT VAPORIZATION *******\n");
		console_printf("****** CONTAINMENT VESSEL VENTING *******\n");
		console_printf("****** MAJOR RADIOACTIVITY LEAK!!! *******\n\n");
		_print_sparks();
		console_clear_interrupt();
		console_wait_until_press();
	}

	/* Check if a rupture has occurred. */
	if(state->warns.rupture_error) {
		console_clear();
		_print_sparks();
		console_printf("WARNING! WARNING! WARNING!\n");
		console_printf("CONTAINMENT VESSEL RUPTURE!\n");
		console_printf("CONTROL RODS EXTENDED THROUGH CONTAINMENT VESSEL!!!\n");
		console_printf("RADIATION LEAK - EVACUATE THE AREA!\n\n");
		_print_sparks();
		console_clear_interrupt();
		console_wait_until_press();
	}
}
