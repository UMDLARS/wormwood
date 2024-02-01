#include "status_win.h"
#include "common.h"
#include "console_win.h"
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

static WINDOW* g_window = NULL;

static pthread_mutex_t g_status_mutex;

static char g_depth_hist[256];

static char* g_safety_string[] = {
    "<<<DISABLED>>>",
    "[ENABLED]",
    "",
};

static const int g_flash_rate = 1; // Seconds
static bool g_enable_flash = false;
static pthread_t g_flash_thread;
static pthread_cond_t g_flash_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_flash_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_flash_mutex = PTHREAD_MUTEX_INITIALIZER;

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

static void __set_safety_state(int state) {
    /* Update safety text. */
    wmove(g_window, 7, 0);
    wprintw(g_window, "| SAFETY PROTOCOLS: %14s                                       |\n", g_safety_string[state]);
    wrefresh(g_window);
    console_reset_cursor();
}

static void* __safety_flash_updater(void*) {
    struct timespec timeout;
    bool state = true;
    bool done = false;
    while(!done) {
        /* Get current time and add [g_flash_rate] sec. */
        timespec_get(&timeout, TIME_UTC);
        timeout.tv_sec += g_flash_rate;

        /* Wait until we've either been signalled or 1 second passes. */
        pthread_cond_timedwait(&g_flash_cond, &g_flash_cond_mutex, &timeout);

        /* Aquire lock on enable flag. */
        pthread_mutex_lock(&g_flash_mutex);

        /* Toggle safety state if we're suppose to still be running, otherwise quit. */
        if(g_enable_flash) {
            __set_safety_state(state ? 2 : 0);
            state = !state;
        }
        else {
            done = true;
        }

        pthread_mutex_unlock(&g_flash_mutex);
    }

    return NULL;
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

    /* Aquire lock, we only want one thread updating this at a time. */
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
	wprintw(g_window, "\n"); // Placeholder, see __set_safety_state.
	wprintw(g_window, "+------------------------------------------------------------------------+\n");

    /* If safety is disabled, the safety message should flash. */
    if(!safety_enabled) {
        __set_safety_state(0);
        g_enable_flash = true;
        pthread_create(&g_flash_thread, NULL, __safety_flash_updater, NULL);
    }
    else {
        if (g_enable_flash) {
            /* Set flag to disable flash. */
            pthread_mutex_lock(&g_flash_mutex);
            g_enable_flash = false;
            pthread_mutex_unlock(&g_flash_mutex);

            /* Try to signal thread. */
            pthread_cond_signal(&g_flash_cond);
            pthread_join(g_flash_thread, NULL);
        }
        __set_safety_state(1);
    }

    /* Update window. */
    //wrefresh(g_window);

    pthread_mutex_unlock(&g_status_mutex);
}
