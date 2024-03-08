#include "status_win.h"
#include <assert.h>
#include <errno.h>
#include <ncurses.h>
#include <pthread.h>
#include <string.h>
#include "common.h"
#include "console_win.h"
#include "reactor.h"

static bool g_initialized = false;

static WINDOW* g_window = NULL;

static pthread_mutex_t g_status_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool g_last_safety_state = false; // default safety is true.
static bool g_last_safety_active = false;

static const int g_flash_rate = 1; // Seconds
static bool g_enable_flash = false;
static pthread_t g_flash_thread;
static pthread_cond_t g_flash_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_flash_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_flash_mutex = PTHREAD_MUTEX_INITIALIZER;

static char *_draw_rod_depth(char* out, unsigned char rod_depth) {
	/* Do nothing if rod_depth is invalid. */
	if(rod_depth > REACTOR_MAX_DEPTH) {
		out[0] = 0;
		return out;
	}

	/* Draw first bracket. */
	int idx = 0;
	out[idx++] = '[';

    /* Draw rod. */
    for(int i = 0; i < rod_depth; i++) {
        out[idx++] = '=';
    }

	/* Draw whitespace. */
	for(int i = 0; i < REACTOR_MAX_DEPTH - rod_depth; i++) {
		out[idx++] = ' ';
	}

	/* Draw second bracket + null term. */
	out[idx++] = ']';
	out[idx++] = 0;

	return out;
}

/*
 * 0: Disabled
 * 1: Enabled
 * 2: Blank/Empty
*/
static void _set_safety_str(int state, bool active) {
	static const char* const safety_str[] = {
		"<<<DISABLED>>>",
		"[ENABLED]",
		"",
	};
	static const char* const safety_active_str[] = {
		"Inactive",
		"Active"
	};

	/* Print safety string. */
	mvwprintw(g_window, 7, 1, "SAFETY PROTOCOLS: %14s", safety_str[state]);

	/* If safety is enabled, also print whether safety is active. */
	if(state == 1) {
		/* The extra two spaces at the end are a hack to fully overwrite inactive when switching to active */
		wprintw(g_window, " (%s)  ", safety_active_str[active]);
	}

	/* Update safety text. */
	wrefresh(g_window);
}

static void* _safety_flash_loop(__attribute__((unused)) void* p) {
	struct timespec timeout;
	bool state = true;
	bool done = false;
	while(!done) {
		/* Get current time and add [g_flash_rate] sec. */
		timespec_get(&timeout, TIME_UTC);
		timeout.tv_sec += g_flash_rate;

		/* Wait until we've either been signalled or 1 second passes. */
		int res = 0;
		while(res != ETIMEDOUT && g_enable_flash) {
			res = pthread_cond_timedwait(&g_flash_cond, &g_flash_cond_mutex, &timeout);
			assert(res == 0 || res == ETIMEDOUT);
		}

		/* Aquire lock for enable flag. */
		assert(pthread_mutex_lock(&g_flash_mutex) == 0);

		/* Toggle safety state if we're suppose to still be running, otherwise quit. */
		if(g_enable_flash) {
			_set_safety_str(state ? 2 : 0, false);
			console_refresh_cursor();
			state = !state;
		}
		else {
			done = true;
		}

		assert(pthread_mutex_unlock(&g_flash_mutex) == 0);
	}

	return NULL;
}

static void _start_safety_flash(void) {
	if(g_enable_flash) {
		return;
	}

	g_enable_flash = true;
	assert(pthread_create(&g_flash_thread, NULL, _safety_flash_loop, NULL) == 0);
}

static void _end_safety_flash(void) {
	if(!g_enable_flash) {
		return;
	}

	/* Set flag to disable flash. */
	assert(pthread_mutex_lock(&g_flash_mutex) == 0);
	g_enable_flash = false;
	assert(pthread_mutex_unlock(&g_flash_mutex) == 0);

	/* Try to signal thread. */
	assert(pthread_cond_signal(&g_flash_cond) == 0);
	assert(pthread_join(g_flash_thread, NULL) == 0);
}

static void _draw_horiz_line(int y) {
	mvwaddch(g_window, y, 0, ACS_LTEE);
	mvwhline(g_window, y, 1, 0, STATUS_WIN_W - 2);
	mvwaddch(g_window, y, STATUS_WIN_W - 1, ACS_RTEE);
}

static void _draw_vert_line(int x) { // untested
	mvwaddch(g_window, 0, x, ACS_TTEE);
	mvwvline(g_window, 1, x, 0, STATUS_WIN_H - 2);
	mvwaddch(g_window, STATUS_WIN_H - 1, x, ACS_BTEE);
}

void status_init(void) {
	if(g_initialized) {
		return;
	}

	/* Create our window. */
	g_window = newwin(STATUS_WIN_H, STATUS_WIN_W, STATUS_WIN_Y, STATUS_WIN_X);
	assert(g_window != NULL);

	/* Create box & lines. */
	box(g_window, 0, 0);
	_draw_horiz_line(2);
	_draw_horiz_line(6);

	/* Set initialized flag. */
	g_initialized = true;
}

void status_end(void) {
	if(!g_initialized) {
		return;
	}

	/* End safety flash. */
	_end_safety_flash();

	/* Destroy our window. */
	delwin(g_window);

	/* Set initialized flag. */
	g_initialized = false;
}

void status_update(const reactor_state_t* state) {
	/* Aquire lock, we only want one thread updating this at a time. */
	assert(pthread_mutex_lock(&g_status_mutex) == 0);

	/* Generate timestamp string. */
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char timestring[64];
	sprintf(timestring, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	/* Draw rod depth. */
	char rod_depth_txt[REACTOR_MAX_DEPTH + 3]; // +3 for brackets + null term.
	_draw_rod_depth(rod_depth_txt, state->rod_depth);

	/* Print status message. */
	mvwprintw(g_window, 1, 1, "JERICHO NUCLEAR REACTOR STATUS PANEL			 (%s)", timestring);
	mvwprintw(g_window, 4, 1, "rod_depth: %2d --[ %s ]--  coolant flow rate: %5.2f", state->rod_depth, rod_depth_txt, state->coolant_flow); 
	mvwprintw(g_window, 5, 1, "User: %-10s", g_usermode_str[state->usermode]);

	/* Print start of temperature line. */
	mvwprintw(g_window, 3, 1, "reactor temp: ");
	wrefresh(g_window);

	/* Update color if the reactor is getting hot. */
	if(state->temp >= REACTOR_WARNING_TEMP_2) {
		wattron(g_window, COLOR_PAIR(1));
	}
	else if(state->temp >= REACTOR_WARNING_TEMP) {
		wattron(g_window, COLOR_PAIR(2));
	}

	/* Print temp. */
	wprintw(g_window, "%8.2f", state->temp);

	/* Clear colors. */
	if(state->temp >= REACTOR_WARNING_TEMP_2) {
		wattroff(g_window, COLOR_PAIR(1));
	}
	else if(state->temp >= REACTOR_WARNING_TEMP) {
		wattroff(g_window, COLOR_PAIR(2));
	}

	/* Print coolant temp. */
	mvwprintw(g_window, 3, 34, "coolant_temp: %8.2f", state->coolant_temp);

	/* Switch safety string if safety has been enabled or disabled since the last update. */
	if(state->safety_enabled != g_last_safety_state) {
		if(state->safety_enabled) {
			_end_safety_flash();
			_set_safety_str(1, state->safety_active);
			g_last_safety_active = state->safety_active;
		}
		else {
			_set_safety_str(0, false);
			_start_safety_flash();
		}

		g_last_safety_state = state->safety_enabled;
	}

	/* Update safety string if safety is enabled and active state has changed. */
	if(state->safety_enabled && state->safety_active != g_last_safety_active) {
		_set_safety_str(1, state->safety_active);
		g_last_safety_active = state->safety_active;
	}

	/* Update window. */
	wrefresh(g_window);

	/* Move visible cursor back to console window. */
	console_refresh_cursor();

	assert(pthread_mutex_unlock(&g_status_mutex) == 0);
}
