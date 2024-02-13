#include "reactor.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include "status_win.h"
#include "console_win.h"
#include "common.h"

const char* g_usermode_str[usermode_count] = {
	[usermode_none]		= "NA",
	[usermode_oper]		= "oper",
	[usermode_super]	= "super"
};

static bool g_initialized = false;

static pthread_mutex_t g_reactor_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Reactor config. */
static reactor_mode_t g_mode = reactor_mode_norealtime;

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

/* Reactor state. */
static reactor_state_t g_state;

/* Realtime mode variables. */
static const int g_realtime_update_rate = 2; // Seconds
static bool g_realtime_active;
static pthread_t g_realtime_thread;
static pthread_cond_t g_realtime_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_realtime_cond_mutex = PTHREAD_MUTEX_INITIALIZER;

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
	if (g_state.temp >= REACTOR_EXPLODE_TEMP) {
        g_state.warns.temp_error = true;
        exit_reason = exit_reason_fail;
		console_interrupt();
		return;
	}

	/* COOLANT FLOW HEAT REDUCTION. */
	/* Coolant flow reduces reactor temp. */
	if (g_state.temp > 70) {
		g_state.temp = g_state.temp - ((g_state.coolant_flow * _float_up_to(7)));
	}

	/* Coolant temp follows the reactor temp, but at a delay. */
	g_state.coolant_temp = g_state.coolant_temp + ((g_state.temp - g_state.coolant_temp) * .15);

	/* Reactor cannot get below room temp. */
	if (g_state.temp < 70) {
		g_state.temp = 70;
	}

	/* Fuzz the reactor temp a bit for realism. */
	g_state.temp = g_state.temp + _get_fuzz() * _rand_sign();

	/* RETRACTING THE RODS INCREASES THE TEMP */
	/* for each unit the rod is not fully extracted, add a random float up to 50. */
	float bump = 0;
	int i;
	int rod_factor = REACTOR_UNSAFE_DEPTH - 1 - g_state.rod_depth;

	for (i = rod_factor; i > 0; i--) {
		bump = bump + _float_up_to(20);
	}
	g_state.temp = g_state.temp + bump;

	/* SAFETY PROTOCOLS */
	if (g_state.safety_enabled == true && g_state.temp > 2000) {
		g_state.safety_active = 1;
		if (g_state.rod_depth < REACTOR_UNSAFE_DEPTH - 1) {
			/* Automatically increment g_state.rod_depth to cool reactor. */
			g_state.rod_depth++;
		}

		if (g_state.coolant_flow <= MAX_FLOW_RATE) {
			g_state.coolant_flow = g_state.coolant_flow + 1;
			if (g_state.coolant_flow > MAX_FLOW_RATE) {
				g_state.coolant_flow = MAX_FLOW_RATE;
			}
		}
	}

	if (g_state.safety_active == 1 && g_state.temp < 2000) {
		g_state.safety_active = 0;
	}

	if(g_state.rod_depth < 0 || g_state.rod_depth >= REACTOR_UNSAFE_DEPTH) {
		g_state.warns.rupture_error = true;
		exit_reason = exit_reason_fail;
		console_interrupt();
	}
}

static void* _realtime_reactor_loop(void*) {
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
        _aquire_lock();

        /* Perform update if we're suppose to still be running, otherwise quit. */
        if(g_realtime_active) {
            _update_impl();
            if(exit_reason != exit_reason_none) {
                done = true;
            }
        }
        else {
            done = true;
        }

        _release_lock();

        /* Update status. */
        status_update();
    }

    return NULL;
}

static void _start_realtime_update(void) {
	/* Do nothing if already started or not in realtime mode. */
    if(g_realtime_active || g_mode != reactor_mode_realtime) {
        return;
    }

    /* Set flag to enable updates. */
    g_realtime_active = true;

    /* Start thread. */
    assert(pthread_create(&g_realtime_thread, NULL, _realtime_reactor_loop, NULL) == 0);
}

static void _end_realtime_update(void) {
    if(!g_realtime_active) {
        return;
    }

    /* Set flag to disable updates in thread. */
    _aquire_lock();
    g_realtime_active = false;
    _release_lock();

    /* Wake up thread. */
    assert(pthread_cond_signal(&g_realtime_cond) == 0);

    /* Wait for thread to finish. */
    assert(pthread_join(g_realtime_thread, NULL) == 0);
}

static void _reactor_process_warns(void) {
    /* Check if we've overheated. */
    if(g_state.warns.temp_error) {
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
    if(g_state.warns.rupture_error) {
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

usermode_t reactor_get_usermode(void) { _EXCL_RETURN(usermode_t, g_state.usermode); }
void reactor_set_usermode(usermode_t mode) { _EXCL_ACCESS(g_state.usermode = mode); }

bool reactor_get_safety(void) { _EXCL_RETURN(bool, g_state.safety_enabled); }
void reactor_set_safety(bool enabled) { _EXCL_ACCESS(g_state.safety_enabled = enabled); }

bool reactor_get_safety_active(void) { _EXCL_RETURN(bool, g_state.safety_active); }

unsigned char reactor_get_rod_depth(void) { _EXCL_RETURN(unsigned char, g_state.rod_depth); }
void reactor_set_rod_depth(unsigned char depth) { _EXCL_ACCESS(g_state.rod_depth = depth); }

float reactor_get_coolant_flow(void) { _EXCL_RETURN(float, g_state.coolant_flow); }
void reactor_set_coolant_flow(float flow) { _EXCL_ACCESS(g_state.coolant_flow = flow); }

float reactor_get_temp(void) { _EXCL_RETURN(float, g_state.temp); }
float reactor_get_coolant_temp(void) { _EXCL_RETURN(float, g_state.coolant_temp); }

reactor_state_t reactor_get_state(void) {
	_aquire_lock();
	reactor_state_t state = g_state;
	_release_lock();
	return state;
}

void reactor_update(void) {
    _aquire_lock();

    /* If we're in norealtime mode, perform an update. */
    if(g_mode == reactor_mode_norealtime) {
        _update_impl();
    }

    /* Process any warnings. */
    _reactor_process_warns();

    _release_lock();

    /* Update status window. */
    status_update();
}

void reactor_init(reactor_mode_t mode) {
	/* Do nothing if we're initialized. */
	if(g_initialized) {
		return;
	}

	/* Set default state. */
	g_state = g_default_state;

	/* Set mode. */
	g_mode = mode;

	/* Perform mode specific init. */
	switch(mode) {
		case reactor_mode_norealtime:
			break;
		case reactor_mode_realtime:
			_start_realtime_update();
			break;
		default:
			break;
	}

	g_initialized = true;
}

void reactor_end(void) {
	/* Do nothing if we aren't initialized. */
	if(!g_initialized) {
		return;
	}

	/* Perform mode specific deinit. */
	switch(g_mode) {
		case reactor_mode_norealtime:
			break;
		case reactor_mode_realtime:
			_end_realtime_update();
			break;
		default:
			break;
	}

	g_initialized = false;
}

reactor_mode_t reactor_get_mode(void) { return g_mode; }
