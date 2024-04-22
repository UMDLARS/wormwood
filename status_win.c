#include "status_win.h"
#include <assert.h>
#include <errno.h>
#include <ncurses.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "console_win.h"
#include "reactor.h"

static bool g_initialized = false;

static WINDOW* g_window = NULL;

static pthread_mutex_t g_status_mutex = PTHREAD_MUTEX_INITIALIZER;

static status_safety_t g_safety_type = status_safety_invalid;

static const char* const g_safety_enable_text[] = {
	[status_safety_invalid] = "Invalid (send bug report!)",
	[status_safety_enable]  = "[ENABLED]",
	[status_safety_disable] = "<<<DISABLED>>>",
	[status_safety_blank]   = "",
};

static const char* const g_safety_active_text[] = {
	[false] = "Inactive",
	[true]  = "Active"
};

static const int g_flash_rate = 1; // Seconds
static bool g_enable_flash = false;
static pthread_t g_flash_thread;
static pthread_cond_t g_flash_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_flash_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_flash_mutex = PTHREAD_MUTEX_INITIALIZER;

static char *_draw_rod_depth(char* out, unsigned char rod_depth) {
	/* Do nothing if rod_depth is invalid. */
	if(rod_depth > REACTOR_MAX_DEPTH) {
		rod_depth = REACTOR_MAX_DEPTH;
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

static void _print_safety_str(const reactor_state_t* state, int tick) {
	static const char* const safety_str[] = {
		[status_safety_invalid] = "Invalid (send bug report!)",
		[status_safety_enable]  = "[ENABLED]",
		[status_safety_disable] = "<<<DISABLED>>>",
		[status_safety_blank]   = "",
	};
	static const char* const safety_active_str[] = {
		[false] = "Inactive",
		[true]  = "Active"
	};

	/* Determine which safety state we're in. */
	status_safety_t safety_state = status_safety_invalid;
	if(state->safety_enabled) {
		safety_state = status_safety_enable;
	}
	else {
		safety_state = tick & 1 ? status_safety_disable : status_safety_blank;
	}

	/* Print safety string. */
	mvwprintw(g_window, 7, 1, "SAFETY PROTOCOLS: %14s", safety_str[safety_state]);

	/* If safety is enabled, also print whether safety is active. */
	if(state->safety_enabled) {
		/* The extra two spaces at the end are a hack to fully overwrite inactive when switching to active */
		wprintw(g_window, " (%s)  ", safety_active_str[state->safety_active]);
	}
	else {
		/* Write 12 space characters to overwrite active/inactive text. */
		for(int i = 0; i < 12; i++) waddch(g_window, ' ');
	}
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

	/* Destroy our window. */
	delwin(g_window);

	/* Set initialized flag. */
	g_initialized = false;
}

void status_update(const reactor_state_t* state, unsigned int tick) {
	/* Aquire lock, we only want one thread updating this at a time. */
	assert(pthread_mutex_lock(&g_status_mutex) == 0);

	/* Get current time. */
	struct tm tm;
	time_t t = time(NULL);
	localtime_r(&t, &tm);

	/* Draw rod depth. */
	char rod_depth_txt[REACTOR_MAX_DEPTH + 3]; // +3 for brackets + null term.
	_draw_rod_depth(rod_depth_txt, state->rod_depth);

	/* Print status message. */
	mvwprintw(g_window, 1, 1, "JERICHO NUCLEAR REACTOR STATUS PANEL");
	mvwprintw(g_window, 1, STATUS_WIN_W - 23, "(%d-%02d-%02d %02d:%02d:%02d)", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
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

	/* Print safety enabled/disabled string. */
	_print_safety_str(state, tick);

	/* Update window. */
	wrefresh(g_window);

	/* Move visible cursor back to console window. */
	console_refresh_cursor();

	assert(pthread_mutex_unlock(&g_status_mutex) == 0);
}
