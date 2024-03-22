#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "console_win.h"
#include "reactor_mgr.h"
#include "signal.h"
#include "wormwood.h"

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

	/* Setup exception handlers. */
	/* These are needed to cleanup ncurses when a fault occurs. */
	signal_setup_handlers();

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
		reactor_mgr_status();
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