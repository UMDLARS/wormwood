#include <ctype.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "console_win.h"
#include "reactor.h"
#include "reactor_mgr.h"

bool get_string(char *dest, int max_len) {
	char input_buffer[8192];
	if(!console_read_strn(input_buffer, 8192)) {
		return false;
	}
	strcpy(dest, input_buffer);
	return true;
}

void auth_user(void) {
	FUNCNAME

	char user_user[32];
	char user_pass[32];
	usermode_t userid = usermode_none;
	const char* const passes[] = {
		[usermode_none]		= "NA",
		[usermode_oper]		= "HomerSimpson",
		[usermode_super]	= "Artemisia1986"
	};

	/* Get username and put into user_user. */
	console_printf("WARNING: UNAUTHORIZED ACCESS IS PUNISHABLE BY LAW!\n");
	console_printf("Which role (%s or %s)?: ", g_usermode_str[usermode_oper], g_usermode_str[usermode_super]);
	if(!get_string(user_user, sizeof(user_user))) {
		/* Return on failure. The reactor has blown up. */
		return;
	}
	user_user[sizeof(user_user) - 1] = 0;

	console_printf("Password for user '");
	console_printf(user_user);
	console_printf("': ");
	if(!get_string(user_pass, sizeof(user_pass))) {
		/* Return on failure. The reactor has blown up. */
		return;
	}
	user_pass[sizeof(user_pass) - 1] = 0;

	/* Initially set user mode to none. */
	reactor_mgr_set_usermode(usermode_none);

	/* Check if provided user is valid & set usermode accordingly. */
	if (strcmp(user_user, g_usermode_str[usermode_oper]) == 0) {
		userid = usermode_oper;
	}
	else if (strcmp(user_user, g_usermode_str[usermode_super]) == 0) {
		userid = usermode_super;
	}
	else {
		console_printf("AUTHENTICATION FAILED (no such user)\n");
		console_wait_until_press();
		return;
	}

	/* Check if provided password matches the one for the selected user. */
	if(strcmp(user_pass, passes[userid]) != 0) {
		console_printf("AUTHENTICATION FAILED (incorrect password)\n");
		console_wait_until_press();
		return;
	}

	reactor_mgr_set_usermode(userid);

	return;
}

void print_menu(void) {
	console_printf("Actions (choose one):\n");

	usermode_t userid = reactor_mgr_get_usermode();

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
			if (reactor_mgr_get_safety() == true) {
				console_printf("(D) - Disable automatic safety control (* SUPER ONLY *)\n");
			}
			else {
				console_printf("(E) - Enable automatic safety control (* SUPER ONLY *)\n");
			}
		}

		/* If authenticated, provide option to log out. */
		console_printf("(L) - Log out\n");
	}

	/* Always print Quit. */
	console_printf("(Q) - Quit\n");

	/* Display wait option if we're in norealtime mode. */
	if(reactor_mgr_get_mode() == reactor_mgr_mode_norealtime) {
		console_printf("(?) - Any other choice - wait\n");
	}
}

void set_rod_depth(void) {
	char new = 0;
	char answer[256];

	/* Ask user for rod depth. */
	console_printf("What should the new rod depth be (0-16)?: ");
	if(!get_string(answer, sizeof(answer))) {
		/* Return on failure. The reactor has blown up. */
		return;
	}
	answer[sizeof(answer) - 1] = 0;

	/* Convert rod depth to an integer. */
	new = atoi(answer);

	/* Make sure rod depth isn't too high. */
	if(!reactor_is_rod_depth_safe(new)) {
		console_printf("New depth value %d is greater than or equal to %d -- ignoring!", new, REACTOR_UNSAFE_DEPTH);
		console_wait_until_press();
		return;
	}

	reactor_mgr_set_rod_depth((unsigned char)new);
}

void set_flow_rate(void) {
	float new = 0;
	char answer[256];
	usermode_t userid = reactor_mgr_get_usermode();

	/* Ask user for flow rate. */
	console_printf("What should the new flow rate be (0.0-100.0)?: ");
	if(!get_string(answer, sizeof(answer))) {
		/* Return on failure. The reactor has blown up. */
		return;
	}
	answer[sizeof(answer) - 1] = 0;

	/* Convert flow rate to a float. */
	new = atof(answer);

	/* Make sure flow rate isn't too high. */
	if (new > MAX_FLOW_RATE) {
		console_printf("New flow rate (%03.2f) is greater than max %03.2f -- ignoring!\n", new, MAX_FLOW_RATE);
		console_wait_until_press();
	}

	/* Make sure flow rate doesn't go below 10 for oper user. */
	if (new < 10 && userid < usermode_super) {
		console_printf("User 'oper' cannot set flow rate below 10! -- ignoring!\n");
		console_wait_until_press();
	}
	else {
		reactor_mgr_set_coolant_flow(new);
	}
}

void confirm_quit(void) {
	/* Ask user to confirm that they want to quit. */
	console_printf("Are you sure you want to quit? (Y/N)");
	char choice = tolower(console_read_chr());
	if(choice == ERR) { // this check isn't needed but helps with clarity.
		return;
	}

	/* Exit if they said yes. */
	if(choice == 'y') {
		exit_reason = exit_reason_quit;
	}
}

void process_choice(char choice) {
	/* Clear console after the user enters an option. */
	console_clear();

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
			reactor_mgr_set_safety(false);
			break;
		case 'e':
			reactor_mgr_set_safety(true);
			break;
		case 'l':
			reactor_mgr_set_usermode(usermode_none);
			break;
		case 'q':
			confirm_quit();
			break;
		default:
			break;
	}

	return;
}

void wormwood_loop(void) {
	FUNCNAME

	/* Clear console. */
	console_clear();

	/* Print menu. */
	print_menu();

	/* Ask user for choice. */
	usermode_t userid = reactor_mgr_get_usermode();
	static const char* const arg_str[] = {
		[usermode_none]		= "AQ",
		[usermode_oper]		= "ARFLQ",
		[usermode_super]	= "RFDELQ"
	};
	console_printf("Enter your selection (%s) and then press ENTER.\n", arg_str[userid]);
	
	/* Read choice. */
	char choice = tolower(console_read_chr());
	if(choice != ERR) {
		/* Process choice if no error occurs. */
		process_choice(choice);
	}

	/* Perform reactor update. */
	reactor_mgr_update();

	return;
}
