#include "status_win.h"
#include "common.h"
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include <pthread.h>

static WINDOW* g_window = NULL;

static pthread_mutex_t g_status_mutex;

static char g_depth_hist[256];

static char* g_safety_string[] = {
    "<<<DISABLED>>>",
    "[ENABLED]",
};

static char *__draw_rod_depth(char rod_depth) {
	int idx = 0;
	int i;
	memset(g_depth_hist, '\0', 256);

	/* If depth is negative, draw to left of bracket. */
	if (g_depth_hist < 0) {
		i = rod_depth * -1;

		for (i; i > 0; i--) {
			g_depth_hist[idx] = '=';
			idx++;
		}
	}

	/* Draw brackets and whitespace starting at 0. */
	g_depth_hist[idx] = '[';
	g_depth_hist[idx + 17] = ']';
	idx++;
	for (i = 0; i < 16; i++) {
		g_depth_hist[idx + i] = ' ';
	}

	/* Depth is positive, draw to right of bracket. */
	for (i = 0; i < rod_depth; i++) {
		g_depth_hist[idx] = '=';
		idx++;
	}

	/* Draw final bracket at max depth. */

	return g_depth_hist;
}

void status_init(void) {
    /* Init our mutex. */
    pthread_mutex_init(&g_status_mutex, NULL);

    /* Create our window. */
    g_window = newwin(STATUS_WIN_H, STATUS_WIN_W, STATUS_WIN_Y, STATUS_WIN_X);
}

void status_end(void) {
    /* Destroy our mutex. */
    pthread_mutex_destroy(&g_status_mutex);

    /* Destroy our window. */
    delwin(g_window);
}

void status_update(void) {
	char depth_hist[256];

    /* Lock mutex, we only want one thread updating this at a time. */
    pthread_mutex_lock(&g_status_mutex);

	/* Generate timestamp string. */
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char timestring[64];
	sprintf(timestring, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	/* Clear screen first. */
	wclear(g_window);

	/* Print status message. */
	wprintw(g_window, "+------------------------------------------------------------------------+\n");
	wprintw(g_window, "| JERICHO NUCLEAR REACTOR STATUS PANEL             (%s) |\n", timestring);
	wprintw(g_window, "+------------------------------------------------------------------------+\n");
	wprintw(g_window, "| reactor temp: %8.2f              coolant_temp: %8.2f             |\n", reactor_temp, coolant_temp); 
	wprintw(g_window, "| rod_depth: %2d --[ %s ]--  coolant flow rate: %5.2f     |\n", rod_depth, __draw_rod_depth(rod_depth), coolant_flow); 
	wprintw(g_window, "| User: %-10s                                                       |\n", users[usermode]);
	wprintw(g_window, "+------------------------------------------------------------------------+\n");
	wprintw(g_window, "| SAFETY PROTOCOLS: %14s                                       |\n", g_safety_string[safety_enabled]);
	wprintw(g_window, "+------------------------------------------------------------------------+\n");

    /* Update window. */
    wrefresh(g_window);

    pthread_mutex_unlock(&g_status_mutex);
}
