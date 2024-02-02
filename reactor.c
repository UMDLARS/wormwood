#include "reactor.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "status_win.h"
#include "console_win.h"
#include "common.h"

pthread_mutex_t g_reactor_mutex = PTHREAD_MUTEX_INITIALIZER;

static usermode_t g_usermode = usermode_none;

static bool g_safety_enabled = true;
static bool g_safety_active = false;
static char g_rod_depth = 16;
static float g_coolant_flow = 10;
static float g_temp = 70.0;
static float g_coolant_temp = 70.0;

static bool g_is_realtime = false;

/* Realtime mode variables. */
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

void _aquire_lock(void) { assert(pthread_mutex_lock(&g_reactor_mutex) == 0); }
void _release_lock(void) { assert(pthread_mutex_unlock(&g_reactor_mutex) == 0); }

#define _EXCL_ACCESS(expr) \
{ \
    _aquire_lock(); \
    expr; \
    _release_lock(); \
}

#define _EXCL_RETURN(type, var) \
{ \
    _aquire_lock(); \
    type _val = var; \
    _release_lock(); \
    return _val; \
}

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

static void _update_impl(void) {
	FUNCNAME

	/* DO NOT UPDATE THIS CODE */

	/* This may shock you, but this is not a real nuclear reactor.
	 * the physics of this "simulation" are not even remotely accurate...
	 * in fact, I'm just making them up.
	 * But, for the purposes of this activity, pretend that there is
	 * real danger or stakes. It's more engaging and interesting that way!
	 */

	/* Fail/throw error if reactor temp goes over 5K. */
	if (g_temp >= 5000) {
        g_warnings.temp_error = true;
        exit_reason = exit_reason_fail;
		return;
	}

	/* COOLANT FLOW HEAT REDUCTION. */
	/* Coolant flow reduces reactor temp. */
	if (g_temp > 70) {
		g_temp = g_temp - ((g_coolant_flow * _float_up_to(7)));
	}

	/* Coolant temp follows the reactor temp, but at a delay. */
	g_coolant_temp = g_coolant_temp + ((g_temp - g_coolant_temp) * .15);

	/* Reactor cannot get below room temp. */
	if (g_temp < 70) {
		g_temp = 70;
	}

	/* Fuzz the reactor temp a bit for realism. */
	g_temp = g_temp + _get_fuzz() * _rand_sign();

	/* RETRACTING THE RODS INCREASES THE TEMP */
	/* for each unit the rod is not fully extracted, add a random float up to 50. */
	float bump = 0;
	int i;
	int rod_factor = MAX_SAFE_DEPTH - g_rod_depth;

	for (i = rod_factor; i > 0; i--) {
		bump = bump + _float_up_to(20);
	}
	g_temp = g_temp + bump;

	/* SAFETY PROTOCOLS */
	if (g_safety_enabled == true && g_temp > 2000) {
        g_warnings.safety_enable = true;
		g_safety_active = 1;
		if (g_rod_depth <= MAX_SAFE_DEPTH) {
			/* Automatically increment g_rod_depth to cool reactor. */
			g_rod_depth++;
		}

		if (g_coolant_flow <= MAX_FLOW_RATE) {
			g_coolant_flow = g_coolant_flow + 1;
			if (g_coolant_flow > MAX_FLOW_RATE) {
				g_coolant_flow = MAX_FLOW_RATE;
			}
		}
	}

	if (g_safety_active == 1 && g_temp < 2000) {
        g_warnings.reach_norm = true;
        g_warnings.safety_enable = false;
		g_safety_active = 0;
	}

	/* Check if rod depth is safe. */
	if (g_rod_depth < 0 || g_rod_depth > MAX_SAFE_DEPTH) {
        g_warnings.rupture = true;
        exit_reason = exit_reason_fail;
	}
}

void* _realtime_reactor_loop(void*) {
    struct timespec timeout;
    bool done = false;
    while(!done) {
        /* Get current time and add [g_realtime_update_rate] sec. */
        timespec_get(&timeout, TIME_UTC);
        timeout.tv_sec += g_realtime_update_rate;

        /* Wait until we're interrupted or X seconds pass. */
        int res = 0;
        while(res != ETIMEDOUT && g_realtime_active) {
            res = pthread_cond_timedwait(&g_realtime_cond, &g_realtime_cond_mutex, &timeout);
            assert(res == 0 || res == ETIMEDOUT);
        }

        /* Aquire lock. */
        assert(pthread_mutex_lock(&g_reactor_mutex) == 0);

        /* Perform update if we're suppose to still be running, otherwise quit. */
        if(g_realtime_active) {
            _update_impl();
        }
        else {
            done = true;
        }

        assert(pthread_mutex_unlock(&g_reactor_mutex) == 0);

        /* Update status. */
        status_update();
    }

    return NULL;
}

usermode_t reactor_get_usermode(void) { _EXCL_RETURN(usermode_t, g_usermode); }
void reactor_set_usermode(usermode_t mode) { _EXCL_ACCESS(g_usermode = mode); }

bool reactor_get_safety(void) { _EXCL_RETURN(bool, g_safety_enabled); }
void reactor_set_safety(bool enabled) { _EXCL_ACCESS(g_safety_enabled = enabled); }

char reactor_get_rod_depth(void) { _EXCL_RETURN(char, g_rod_depth); }
void reactor_set_rod_depth(char depth) { _EXCL_ACCESS(g_rod_depth = depth); }

float reactor_get_coolant_flow(void) { _EXCL_RETURN(float, g_coolant_flow); }
void reactor_set_coolant_flow(float flow) { _EXCL_ACCESS(g_coolant_flow = flow); }

float reactor_get_temp(void) { _EXCL_RETURN(float, g_temp); }
float reactor_get_coolant_temp(void) { _EXCL_RETURN(float, g_coolant_temp); }

void reactor_update(void) {
    /* If we're not in realtime mode, perform an update. */
    if(!reactor_is_realtime()) {
        _update_impl();
    }

    /* Update status window. */
    status_update();

    /* Process any warnings. */
    reactor_process_warns();
}

void reactor_process_warns(void) {
    _aquire_lock();

    /* Check if temp is getting dangerously high. */
	if (g_temp > 3000) {
		console_printf("\n***** WARNING: REACTOR COOLANT WILL VAPORIZE AT 5000 DEGREES ******\n");
		if(g_temp > 4000) {
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

    /* Check if we've overheated. */
    if(g_warnings.temp_error) {
		console_clear();
		_print_sparks();
		console_printf("****** COOLANT VAPORIZATION *******\n");
		console_printf("****** CONTAINMENT VESSEL VENTING *******\n");
		console_printf("****** MAJOR RADIOACTIVITY LEAK!!! *******\n\n");
		_print_sparks();
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
    }

    _release_lock();
}

void reactor_set_realtime_enabled(bool enabled) { g_is_realtime = enabled; }
bool reactor_is_realtime(void) { return g_is_realtime; }

void reactor_start_realtime_update(void) {
    if(g_realtime_active) {
        return;
    }

    /* Set flag to enable updates. */
    g_realtime_active = true;

    /* Start thread. */
    assert(pthread_create(&g_realtime_thread, NULL, _realtime_reactor_loop, NULL) == 0);
}

void reactor_end_realtime_update(void) {
    if(!g_realtime_active) {
        return;
    }

    /* Set flag to disable updates in thread. */
    assert(pthread_mutex_lock(&g_reactor_mutex) == 0);
    g_realtime_active = false;
    assert(pthread_mutex_unlock(&g_reactor_mutex) == 0);

    /* Wake up thread. */
    assert(pthread_cond_signal(&g_realtime_cond) == 0);

    /* Wait for thread to finish. */
    assert(pthread_join(g_realtime_thread, NULL) == 0);
}
