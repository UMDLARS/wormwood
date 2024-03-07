#include "reactor_thread.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include "status_win.h"
#include "console_win.h"
#include "common.h"
#include "reactor.h"

static bool g_initialized = false;

static pthread_mutex_t g_reactor_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Reactor config. */
static reactor_mode_t g_mode = reactor_mode_norealtime;

/* Reactor state. */
static reactor_state_t g_state;

/* Realtime mode variables. */
static const int g_realtime_update_rate = 1; // Seconds
static bool g_realtime_active;
static pthread_t g_realtime_thread;
static pthread_cond_t g_realtime_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_realtime_cond_mutex = PTHREAD_MUTEX_INITIALIZER;

static void _aquire_lock(void) { assert(pthread_mutex_lock(&g_reactor_mutex) == 0); }
static void _release_lock(void) { assert(pthread_mutex_unlock(&g_reactor_mutex) == 0); }

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

static void _update_impl(void) {
	reactor_impl_update(&g_state);
}

static void* _realtime_reactor_loop(__attribute__((unused)) void* p) {
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
	/* Set flag to enable updates. */
	assert(g_realtime_active == false);
	g_realtime_active = true;

	/* Start thread. */
	assert(pthread_create(&g_realtime_thread, NULL, _realtime_reactor_loop, NULL) == 0);
}

static void _end_realtime_update(void) {
	/* Set flag to disable updates in thread. */
	_aquire_lock();
	assert(g_realtime_active == true);
	g_realtime_active = false;
	_release_lock();

	/* Wake up thread. */
	assert(pthread_cond_signal(&g_realtime_cond) == 0);

	/* Wait for thread to finish. */
	assert(pthread_join(g_realtime_thread, NULL) == 0);
}

static void _reactor_process_warns(void) {
	reactor_impl_check_warns(&g_state);
}

usermode_t reactor_get_usermode(void) { _EXCL_RETURN(usermode_t, g_state.usermode); }
void reactor_set_usermode(usermode_t mode) { _EXCL_ACCESS(g_state.usermode = mode); }

bool reactor_get_safety(void) { _EXCL_RETURN(bool, g_state.safety_enabled); }
void reactor_set_safety(bool enabled) {
	_aquire_lock();

	/* Set safety enabled. */
	g_state.safety_enabled = enabled;
	
	/* If safety is being disabled, also deactivate safety. */
	if(!enabled) {
		g_state.safety_active = false;
	}

	/* If safety is being enabled and temp is above safe temp, activate safety. */
	else if(g_state.temp > REACTOR_SAFE_TEMP) {
		g_state.safety_active = true;
	}

	_release_lock();
}

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
	reactor_impl_init(&g_state);

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
