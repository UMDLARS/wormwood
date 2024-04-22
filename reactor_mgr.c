#include "reactor_mgr.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include "status_win.h"
#include "console_win.h"
#include "common.h"
#include "reactor.h"

static bool g_initialized = false;

static unsigned int g_tick = 0;

static pthread_mutex_t g_reactor_mgr_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Reactor config. */
static reactor_mgr_mode_t g_mode = reactor_mgr_mode_norealtime;

/* Reactor state.. */
static reactor_state_t g_state;

/* Realtime mode variables. */
static const int g_realtime_update_rate = 1; // Seconds
static bool g_realtime_active;
static pthread_t g_realtime_thread;
static pthread_cond_t g_realtime_cond = PTHREAD_COND_INITIALIZER;

static void _aquire_lock(void) { assert(pthread_mutex_lock(&g_reactor_mgr_mutex) == 0); }
static void _release_lock(void) { assert(pthread_mutex_unlock(&g_reactor_mgr_mutex) == 0); }

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

static void* _realtime_reactor_mgr_loop(__attribute__((unused)) void* p) {
	struct timespec timeout;
	bool done = false;
	while(!done) {
		/* Get current time and add [g_realtime_update_rate] sec. */
		timespec_get(&timeout, TIME_UTC);
		timeout.tv_sec += g_realtime_update_rate;

		/* Aquire lock. */
		_aquire_lock();

		/* Wait until we're interrupted or X seconds pass. */
		int res = 0;
		while(res != ETIMEDOUT && g_realtime_active) {
			res = pthread_cond_timedwait(&g_realtime_cond, &g_reactor_mgr_mutex, &timeout);
			assert(res == 0 || res == ETIMEDOUT);
		}

		/* Perform update if we're suppose to still be running, otherwise quit. */
		if(g_realtime_active) {
			reactor_update(&g_state);
			if(exit_reason != exit_reason_none) {
				done = true;
			}
		}
		else {
			done = true;
		}

		/* Update status. */
		status_update(&g_state);

		/* Update tick. */
		g_tick++;

		_release_lock();
	}

	return NULL;
}

static void _start_realtime_update(void) {
	/* Set flag to enable updates. */
	assert(g_realtime_active == false);
	g_realtime_active = true;

	/* Start thread. */
	assert(pthread_create(&g_realtime_thread, NULL, _realtime_reactor_mgr_loop, NULL) == 0);
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

usermode_t reactor_mgr_get_usermode(void) { _EXCL_RETURN(usermode_t, g_state.usermode); }
void reactor_mgr_set_usermode(usermode_t mode) { _EXCL_ACCESS(g_state.usermode = mode); }

bool reactor_mgr_get_safety(void) { _EXCL_RETURN(bool, g_state.safety_enabled); }
void reactor_mgr_set_safety(bool enabled) {
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

bool reactor_mgr_get_safety_active(void) { _EXCL_RETURN(bool, g_state.safety_active); }

unsigned char reactor_mgr_get_rod_depth(void) { _EXCL_RETURN(unsigned char, g_state.rod_depth); }
void reactor_mgr_set_rod_depth(unsigned char depth) { _EXCL_ACCESS(g_state.rod_depth = depth); }

float reactor_mgr_get_coolant_flow(void) { _EXCL_RETURN(float, g_state.coolant_flow); }
void reactor_mgr_set_coolant_flow(float flow) { _EXCL_ACCESS(g_state.coolant_flow = flow); }

float reactor_mgr_get_temp(void) { _EXCL_RETURN(float, g_state.temp); }
float reactor_mgr_get_coolant_temp(void) { _EXCL_RETURN(float, g_state.coolant_temp); }

void reactor_mgr_update(void) {
	_aquire_lock();

	/* If we're in norealtime mode, perform an update. */
	if(g_mode == reactor_mgr_mode_norealtime) {
		reactor_update(&g_state);
	}

	/* Process any warnings. */
	reactor_check_warns(&g_state);

	/* Update status window. */
	status_update(&g_state);

	_release_lock();
}

void reactor_mgr_init(reactor_mgr_mode_t mode) {
	/* Do nothing if we're initialized. */
	if(g_initialized) {
		return;
	}

	/* Set default state. */
	reactor_init(&g_state);

	/* Set mode. */
	g_mode = mode;

	/* Perform mode specific init. */
	switch(mode) {
		case reactor_mgr_mode_norealtime:
			break;
		case reactor_mgr_mode_realtime:
			_start_realtime_update();
			break;
		default:
			break;
	}

	/* Init status window. */
	status_init();

	/* Update status window. */
	status_update(&g_state);

	g_initialized = true;
}

void reactor_mgr_end(void) {
	/* Do nothing if we aren't initialized. */
	if(!g_initialized) {
		return;
	}

	/* Perform mode specific deinit. */
	switch(g_mode) {
		case reactor_mgr_mode_norealtime:
			break;
		case reactor_mgr_mode_realtime:
			_end_realtime_update();
			break;
		default:
			break;
	}

	/* Finalize status window. */
	status_end();

	g_initialized = false;
}

reactor_mgr_mode_t reactor_mgr_get_mode(void) { return g_mode; }
