#include <assert.h>
#include <ctype.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "console_win.h"
#include "reactor.h"
#include "status_win.h"

void get_string(char *dest) {
	char input_buffer[8192];
	console_read_strn(input_buffer, 8192);
	strcpy(dest, input_buffer);
}

void auth_user(void) {
	FUNCNAME

	char user_user[32];
	char user_pass[32];
	usermode_t userid = usermode_none;
	char *passes[] = {"NA", "HomerSimpson", "Artemisia1986"};

	/* Get username and put into user_user. */
	console_printf("WARNING: UNAUTHORIZED ACCESS IS PUNISHABLE BY LAW!\n");
    console_printf("Which role (%s or %s)?: ", users[1], users[2]);
	get_string(user_user);

	console_printf("Password for user '");
	console_printf(user_user);
	console_printf("': ");
	get_string(user_pass);

	/* Check if provided user is valid & set usermode accordingly. */
	if (strcmp(user_user, users[1]) == 0) {
		userid = usermode_oper;
	}
	else if (strcmp(user_user, users[2]) == 0) {
		userid = usermode_super;
	}
	else {
		console_printf("AUTHENTICATION FAILED (no such user)\n");
		reactor_set_usermode(usermode_none);
		return;
	}

	/* Check if provided password is correct. */
	if (strcmp(user_pass, passes[userid]) == 0) {
		console_printf("User '%s' AUTHENTICATED!\n", users[userid]);
		reactor_set_usermode(userid);
	} 
	else {
		console_printf("AUTHENTICATION FAILED (incorrect password)\n");
	}

	reactor_set_usermode(usermode_super);
	return;
}

void print_menu(void) {
	console_printf("Actions (choose one):\n");

	usermode_t userid = reactor_get_usermode();

	/* Print auth if we're not in supervisor mode. */
	if (userid != usermode_super) {
		console_printf("(A) - Authenticate\n");
	}

	if (userid != usermode_none) {
		/* Print all normal oper options. */
		console_printf("(R) - Set rod depth\n");
		console_printf("(F) - Set coolant flow rate\n");

		/* Allow enable/disable safety if we're in supervisor mode. */
		if(userid == usermode_super) {
			if (reactor_get_safety() == true) {
				console_printf("(D) - Disable automatic safety control (* SUPER ONLY *)\n");
			}
			else {
				console_printf("(E) - Enable automatic safety control (* SUPER ONLY *)\n");
			}
		}

		/* If authenticated, provide option to log out. */
		console_printf("(L) - Log out\n");

		console_printf("(?) - Any other choice - wait\n");
	}

	/* Always print Quit. */
	console_printf("(Q) - Quit\n");
}

void set_rod_depth(void) {
	int new = 0;
	char answer[256];
	char curr_depth = reactor_get_rod_depth();

	console_printf("What should the new rod depth be (0-16) [current: %d]?: ", curr_depth);
	get_string(answer);
	new = atoi(answer);

	console_printf("new: %d\n", new);
	if (new <= MAX_SAFE_DEPTH) {
		reactor_set_rod_depth(new);
	}
	else {
		console_printf("New depth value %d is greater than %d -- ignoring!", new, MAX_SAFE_DEPTH);
	}
}

void set_flow_rate(void) {
	float new = 0;
	char answer[256];
	usermode_t userid = reactor_get_usermode();
	float curr_flow = reactor_get_coolant_flow();

	console_printf("What should the new flow rate be (0.0-100.0) [current: %03.2f]?: ", curr_flow);
	get_string(answer);
	new = atof(answer);

	if (new > MAX_FLOW_RATE) {
		console_printf("New flow rate (%03.2f) is greater than max %03.2f -- ignoring!\n", new, MAX_FLOW_RATE);
	}

	if (new < 10 && userid < usermode_super) {
		console_printf("User 'oper' cannot set flow rate below 10! -- ignoring!\n");
	}
	else {
		console_printf("New coolant flow rate: %03.2f (was: %03.2f)\n", new, curr_flow);
		reactor_set_coolant_flow(new);
	}
}

void process_choice(char choice) {
	/* Clear console after the user enters an option. */
	console_clear();

	//console_printf("Your choice was: '%c'\n", choice);

	switch(choice) {
		case 'a':
			auth_user();
			break;
		case 'r':
			set_rod_depth();
			break;
		case 'f':
			set_flow_rate();
			break;
		case 'd':
			reactor_set_safety(false);
			console_printf("***** SAFETY PROTOCOLS DISABLED!!! *****\n");
			break;
		case 'e':
			reactor_set_safety(true);
			console_printf("***** SAFETY PROTOCOLS ENABLED!!! *****\n");
			break;
		case 'l':
			console_printf("Deauthenticating.\n");
			reactor_set_usermode(usermode_none);
			break;
		case 'q':
			console_printf("Quitting...\n");
			exit_reason = exit_reason_quit;
			break;
		default:
			console_printf("Doing nothing...\n");
			break;
	}

	return;
}

void reactor_status(void) {
	FUNCNAME

	/* Clear console. */
	console_clear();

	/* Print menu and get/perform operation. */
	print_menu();

	/* Read choice. */
	console_printf("Enter your selection (ARFDELQ) and then press ENTER.\n");
	char choice = tolower(console_read_chr());

	/* Process choice. */
	process_choice(choice);

	/* Perform reactor update. */
	reactor_update();

	/* Wait for keypress. */
	console_wait_until_press();

	return;
}

int main(int argc, char *argv[]) {
	/* Initialize ncurses. */
	initscr();

	/* Initialize windows. */
	status_init();
	console_init();

	/* Perform initial status update. */
	status_update();

	/* Enable realtime mode if the first argument is "realtime". */
	if(argc >= 2 && !strcmp(argv[1], "realtime")) {
		reactor_set_realtime_enabled(true);
	}

	/* Start realtime reactor loop. */
	if(reactor_is_realtime()) {
		reactor_start_realtime_update();
	}

	/* Seed RNG with time. */
	srand(time(NULL));

	while (1) {
		reactor_status();
		if (exit_reason != exit_reason_none) {
			break;
		}
	}

	/* End realtime reactor loop. */
	reactor_end_realtime_update(); // noop if non-realtime.

	/* Finalize windows. */
	status_end();
	console_end();

	/* Close ncurses window. */
	endwin();

	/* Return -1 on failure. */
	if(exit_reason == exit_reason_fail) {
		return -1;
	}

	return 0;
}
