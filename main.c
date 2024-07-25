#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "console_win.h"
#include "reactor_mgr.h"
#include "wormwood.h"

static bool g_fault_occurred = false;

static void _error_handler(int signal) {
    if(g_fault_occurred) {
        exit(1);
    }

    g_fault_occurred = true;

	/* Close ncurses window. */
	endwin();

    fputs("Congrats! You broke it!\n", stderr);
}

static void _exit_handler(int signal) {
	/* Close ncurses window. */
	endwin();

    /* Exit. */
    exit(0);
}

// https://www.gnu.org/software/libc/manual/html_node/Termination-Signals.html
// https://man7.org/linux/man-pages/man2/sigaction.2.html
static void _setup_signal_handlers(void) {
    /* Setup handler struct. */
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = _error_handler;

    /* Handle SIGSEGV and SIGIOT. */
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGIOT, &act, NULL);

    /* Handle SIGTERM, SIGINT, and SIGQUIT*/
    act.sa_handler = _exit_handler;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
}

int main(int argc, char *argv[]) {
	/* Get reactor mode. */
	reactor_mgr_mode_t mode = reactor_mgr_mode_realtime; // default to realtime
	if(argc >= 2) {
		if(!strcmp(argv[1], "realtime")) {
			mode = reactor_mgr_mode_realtime;
		}
		else if(!strcmp(argv[1], "norealtime")) {
			mode = reactor_mgr_mode_norealtime;
		}
		else {
			printf("Invalid mode %s.\n", argv[1]);
			return -1;
		}
	}

	/* Setup signal handlers. */
	_setup_signal_handlers();

	/* Initialize ncurses. */
	initscr();
	start_color();
	halfdelay(1);

	/* Setup colors. */
	init_pair(1, COLOR_WHITE, COLOR_RED);
	init_pair(2, COLOR_RED, COLOR_BLACK);

	/* Initialize windows. */
	console_init();

	/* Initialize reactor with previously obtained mode. */
	reactor_mgr_init(mode);

	/* Seed RNG with time. */
	srand(time(NULL));

	while (1) {
		wormwood_loop();
		if (exit_reason != exit_reason_none) {
			break;
		}
	}

	/* Finalize reactor. */
	reactor_mgr_end();

	/* Finalize windows. */
	console_end();

	/* Close ncurses window. */
	endwin();

	/* Return -1 on failure. */
	if(exit_reason == exit_reason_fail) {
		return -1;
	}

	return 0;
}
