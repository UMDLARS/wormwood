#include "reactor.h"
#include <stdlib.h>
#include "console_win.h"
#include "common.h"

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

void update_reactor(void) {
	FUNCNAME

	/* DO NOT UPDATE THIS CODE */

	/* This may shock you, but this is not a real nuclear reactor.
	 * the physics of this "simulation" are not even remotely accurate...
	 * in fact, I'm just making them up.
	 * But, for the purposes of this activity, pretend that there is
	 * real danger or stakes. It's more engaging and interesting that way!
	 */

	/* WARNINGS */
	if (reactor_temp > 3000) {
		console_printf("\n***** WARNING: REACTOR COOLANT WILL VAPORIZE AT 5000 DEGREES ******\n");
		if(reactor_temp > 4000) {
			console_printf("***** WARNING: IMMINENT BREACH! IMMINENT BREACH! ******\n");
		}
		console_printf("\n");
	}

	/* Fail/throw error if reactor temp goes over 5K. */
	if (reactor_temp >= 5000) {
		console_clear();
		_print_sparks();
		console_printf("****** COOLANT VAPORIZATION *******\n");
		console_printf("****** CONTAINMENT VESSEL VENTING *******\n");
		console_printf("****** MAJOR RADIOACTIVITY LEAK!!! *******\n\n");
		_print_sparks();
		exit_reason = exit_reason_fail;
		return;
	}

	/* COOLANT FLOW HEAT REDUCTION. */
	/* Coolant flow reduces reactor temp. */
	if (reactor_temp > 70) {
		reactor_temp = reactor_temp - ((coolant_flow * _float_up_to(7)));
	}

	/* Coolant temp follows the reactor temp, but at a delay. */
	coolant_temp = coolant_temp + ((reactor_temp - coolant_temp) * .15);

	/* Reactor cannot get below room temp. */
	if (reactor_temp < 70) {
		reactor_temp = 70;
	}

	/* Fuzz the reactor temp a bit for realism. */
	reactor_temp = reactor_temp + _get_fuzz() * _rand_sign();

	/* RETRACTING THE RODS INCREASES THE TEMP */
	/* for each unit the rod is not fully extracted, add a random float up to 50. */
	float bump = 0;
	int i;
	int rod_factor = MAX_SAFE_DEPTH - rod_depth;

	for (i = rod_factor; i > 0; i--) {
		bump = bump + _float_up_to(20);
	}
	reactor_temp = reactor_temp + bump;

	/* SAFETY PROTOCOLS */
	if (safety_enabled == true && reactor_temp > 2000) {
		safety_active = 1;
		console_printf("\n ****** SAFETY PROTOCOLS ENGAGED: Extending control rods! *******\n\n");
		if (rod_depth <= MAX_SAFE_DEPTH) {
			/* Automatically increment rod_depth to cool reactor. */
			rod_depth++;
		}

		console_printf("\n ****** SAFETY PROTOCOLS ENGAGED: Increasing coolant flow! *******\n\n");
		if (coolant_flow <= MAX_FLOW_RATE) {
			coolant_flow = coolant_flow + 1;
			if (coolant_flow > MAX_FLOW_RATE) {
				coolant_flow = MAX_FLOW_RATE;
			}
		}
	}

	if (safety_active == 1 && reactor_temp < 2000) {
		safety_active = 0;
		console_printf("\n\n******* NORMAL OPERATING TEMPERATURE ACHIEVED ********\n\n");
	}

	/* Check if rod depth is safe. */
	if (rod_depth < 0 || rod_depth > MAX_SAFE_DEPTH) {
		console_clear();
		_print_sparks();
		console_printf("WARNING! WARNING! WARNING!\n");
		console_printf("CONTAINMENT VESSEL RUPTURE!\n");
		console_printf("CONTROL RODS EXTENDED THROUGH CONTAINMENT VESSEL!!!\n");
	    console_printf("RADIATION LEAK - EVACUATE THE AREA!\n\n");
		_print_sparks();
		exit_reason = exit_reason_fail;
	}
}
