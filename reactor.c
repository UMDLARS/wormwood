#include "reactor.h"
#include <stdlib.h>
#include <stdint.h>
#include "status_win.h"
#include "console_win.h"
#include "common.h"

pthread_mutex_t g_reactor_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool g_is_realtime = false;

static const int g_realtime_update_rate = 2; // Seconds
static bool g_realtime_active;
static pthread_t g_realtime_thread;
static pthread_cond_t g_realtime_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_realtime_cond_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct {
    uint temp_error : 1;
    uint safety_enable :1;
    uint reach_norm : 1;
    uint rupture : 1;
} g_warnings;

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

void* _realtime_reactor_loop(void*) {
    struct timespec timeout;
    bool done = false;

    while(!done) {
        /* Get current time and add [g_realtime_update_rate] sec. */
        timespec_get(&timeout, TIME_UTC);
        timeout.tv_sec += g_realtime_update_rate;

        /* Wait until we're interrupted or X seconds pass. */
        pthread_cond_timedwait(&g_realtime_cond, &g_realtime_cond_mutex, &timeout);

        /* Aquire lock. */
        pthread_mutex_lock(&g_reactor_mutex);

        /* Perform update if we're suppose to still be running, otherwise quit. */
        if(g_realtime_active) {
            reactor_update();
        }
        else {
            done = true;
        }

        pthread_mutex_unlock(&g_reactor_mutex);
    }

    return NULL;
}

void reactor_update(void) {
	FUNCNAME

	/* DO NOT UPDATE THIS CODE */

	/* This may shock you, but this is not a real nuclear reactor.
	 * the physics of this "simulation" are not even remotely accurate...
	 * in fact, I'm just making them up.
	 * But, for the purposes of this activity, pretend that there is
	 * real danger or stakes. It's more engaging and interesting that way!
	 */

	/* Fail/throw error if reactor temp goes over 5K. */
	if (reactor_temp >= 5000) {
        g_warnings.temp_error = true;
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
        g_warnings.safety_enable = true;
		safety_active = 1;
		if (rod_depth <= MAX_SAFE_DEPTH) {
			/* Automatically increment rod_depth to cool reactor. */
			rod_depth++;
		}

		if (coolant_flow <= MAX_FLOW_RATE) {
			coolant_flow = coolant_flow + 1;
			if (coolant_flow > MAX_FLOW_RATE) {
				coolant_flow = MAX_FLOW_RATE;
			}
		}
	}

	if (safety_active == 1 && reactor_temp < 2000) {
        g_warnings.reach_norm = true;
        g_warnings.safety_enable = false;
		safety_active = 0;
	}

	/* Check if rod depth is safe. */
	if (rod_depth < 0 || rod_depth > MAX_SAFE_DEPTH) {
        g_warnings.rupture = true;
        exit_reason = exit_reason_fail;
	}

    /* Update status window. */
    status_update();
}

void reactor_process_warns(void) {
    /* Check if we've overheated. */
    if(g_warnings.temp_error) {
		console_clear();
		_print_sparks();
		console_printf("****** COOLANT VAPORIZATION *******\n");
		console_printf("****** CONTAINMENT VESSEL VENTING *******\n");
		console_printf("****** MAJOR RADIOACTIVITY LEAK!!! *******\n\n");
		_print_sparks();
        return;
    }

    /* Check if a rupture has occurred. */
    if(g_warnings.rupture) {
		console_clear();
		_print_sparks();
		console_printf("WARNING! WARNING! WARNING!\n");
		console_printf("CONTAINMENT VESSEL RUPTURE!\n");
		console_printf("CONTROL RODS EXTENDED THROUGH CONTAINMENT VESSEL!!!\n");
	    console_printf("RADIATION LEAK - EVACUATE THE AREA!\n\n");
		_print_sparks();
        return;
    }

    /* Check if temp is getting dangerously high. */
	if (reactor_temp > 3000) {
		console_printf("\n***** WARNING: REACTOR COOLANT WILL VAPORIZE AT 5000 DEGREES ******\n");
		if(reactor_temp > 4000) {
			console_printf("***** WARNING: IMMINENT BREACH! IMMINENT BREACH! ******\n");
		}
        console_printf("\n");
	}

    /* Check if safety had to be enabled. */
    if(g_warnings.safety_enable) {
        console_printf("\n ****** SAFETY PROTOCOLS ENGAGED: Extending control rods! *******\n\n");
        console_printf("\n ****** SAFETY PROTOCOLS ENGAGED: Increasing coolant flow! *******\n\n");
    }

    /* Check if we've reached normal temps again. */
    if(g_warnings.reach_norm) {
        console_printf("\n\n******* NORMAL OPERATING TEMPERATURE ACHIEVED ********\n\n");

        /* We only need to print this once. */
        g_warnings.reach_norm = false;
    }
}

bool reactor_is_realtime(void) { return g_is_realtime; }

void reactor_start_realtime_update(void) {
    /* Set flag to enable updates. */
    g_realtime_active = true;

    /* Start thread. */
    pthread_create(&g_realtime_thread, NULL, _realtime_reactor_loop, NULL);
}

void reactor_end_realtime_update(void) {
    /* Set flag to disable updates in thread. */
    pthread_mutex_lock(&g_reactor_mutex);
    g_realtime_active = false;
    pthread_mutex_unlock(&g_reactor_mutex);

    /* Wake up thread. */
    pthread_cond_signal(&g_realtime_cond);

    /* Wait for thread to finish. */
    pthread_join(g_realtime_thread, NULL);
}
