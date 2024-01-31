#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include <ncurses.h>
#include "common.h"
#include "console_win.h"
#include "status_win.h"

#define DEBUG 0
#define FUNCNAME if (DEBUG) console_printf("In %s @ line %d...\n", __func__, __LINE__);

#define MAX_SAFE_DEPTH 16
#define MAX_FLOW_RATE 100.0

typedef enum {
	exit_reason_none = 0,
	exit_reason_fail,
	exit_reason_quit,
} exit_reason_t;

exit_reason_t exit_reason = exit_reason_none;

void get_string(char *dest) {
	char input_buffer[8192];
	console_read_str(input_buffer); // Creates a new vuln, do we want this?
	strcpy(dest, input_buffer);
}

void print_sparks(void) {
	int i = CONSOLE_WIN_W * 3;
	int j = 0;
	while (i > 0) {
		for (j = (rand() % 10); j > 0 && i > 1; j--) {
			console_printf(" ");
			i--;
		}
		console_printf("*");
		i--;
	}

	console_printf("\n");
}

void auth_user(void) {
	FUNCNAME

	char user_user[32];
	char user_pass[32];
	int userid = 0;
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
		userid = 1;
	}
	else if (strcmp(user_user, users[2]) == 0) {
		userid = 2;
	}
	else {
		console_printf("AUTHENTICATION FAILED (no such user)\n");
		usermode = 0;
		return;
	}

	/* Check if provided password is correct. */
	if (strcmp(user_pass, passes[userid]) == 0) {
		console_printf("User '%s' AUTHENTICATED!\n", users[userid]);
		usermode = userid;
	} 
	else {
		console_printf("AUTHENTICATION FAILED (incorrect password)\n");
	}

	usermode = 2;
	return;
}

void print_menu(void) {
	console_printf("Actions (choose one):\n");

	/* Print auth if we're not in supervisor mode. */
	if (usermode != 2) {
		console_printf("(A) - Authenticate\n");
	}

	if (usermode != 0) {
		/* Print all normal oper options. */
		console_printf("(R) - Set rod depth\n");
		console_printf("(F) - Set coolant flow rate\n");

		/* Allow enable/disable safety if we're in supervisor mode. */
		if(usermode == 2) {
			if (safety_enabled == true) {
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
	console_printf("What should the new rod depth be (0-16) [current: %d]?: ", rod_depth);
	get_string(answer);
	new = atoi(answer);

	console_printf("new: %d\n", new);
	if (new <= MAX_SAFE_DEPTH) {
		rod_depth = new;
	}
	else {
		console_printf("New depth value %d is greater than %d -- ignoring!", new, MAX_SAFE_DEPTH);
	}
}

void set_flow_rate(void) {
	float new = 0;
	char answer[256];
	console_printf("What should the new flow rate be (0.0-100.0) [current: %03.2f]?: ", coolant_flow);
	get_string(answer);
	new = atof(answer);

	if (new > MAX_FLOW_RATE) {
		console_printf("New flow rate (%03.2f) is greater than max %03.2f -- ignoring!\n", new, MAX_FLOW_RATE);
	}

	if (new < 10 && usermode < 2) {
		console_printf("User 'oper' cannot set flow rate below 10! -- ignoring!\n");
	}
	else {
		console_printf("New coolant flow rate: %03.2f (was: %03.2f)\n", new, coolant_flow);
		coolant_flow = new;
	}
}

void get_and_do_choice(void) {
	char choice_string[256];

	console_printf("Enter your selection (ARFDEL) and then press ENTER.\n");
	char choice = console_read_chr();

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
			safety_enabled = false;
			console_printf("***** SAFETY PROTOCOLS DISABLED!!! *****\n");
			break;
		case 'e':
			safety_enabled = true;
			console_printf("***** SAFETY PROTOCOLS ENABLED!!! *****\n");
			break;
		case 'l':
			console_printf("Deauthenticating.\n");
			usermode = 0;
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

float float_up_to(int max) {
	// https://stackoverflow.com/questions/13408990/how-to-generate-random-float-number-in-c
	float fuzz = (float)rand()/(float)(RAND_MAX/max);
	return fuzz;
}

float get_fuzz(void) {
	return float_up_to(1);
}

int rand_sign(void) {
	int sign;
	int randval = rand();
	if (randval % 2 == 0) {
		sign = 1;
	}
	else {
		sign = -1;
	}

	return sign;
}

void update_reactor(void) {
	FUNCNAME

	/* DO NOT UPDATE THIS CODE */

	/* This may shock you, but this is not a real nuclear reactor.
	 * the physics of this "simulation" are not even remotely accurate...
	 * in fact, I'm just making them up.
	 * But, for the purposes of this activity, pretend that there is
	 * real danger or stakes. It's more engaging and interesting that way!
	 */

	/* WARNINGS */
	if (reactor_temp > 3000) {
		console_printf("\n***** WARNING: REACTOR COOLANT WILL VAPORIZE AT 5000 DEGREES ******\n");
		if(reactor_temp > 4000) {
			console_printf("***** WARNING: IMMINENT BREACH! IMMINENT BREACH! ******\n");
		}
		console_printf("\n");
	}

	/* Fail/throw error if reactor temp goes over 5K. */
	if (reactor_temp >= 5000) {
		console_clear();
		print_sparks();
		console_printf("****** COOLANT VAPORIZATION *******\n");
		console_printf("****** CONTAINMENT VESSEL VENTING *******\n");
		console_printf("****** MAJOR RADIOACTIVITY LEAK!!! *******\n\n");
		print_sparks();
		exit_reason = exit_reason_fail;
		return;
	}

	/* COOLANT FLOW HEAT REDUCTION. */
	/* Coolant flow reduces reactor temp. */
	if (reactor_temp > 70) {
		reactor_temp = reactor_temp - ((coolant_flow * float_up_to(7)));
	}

	/* Coolant temp follows the reactor temp, but at a delay. */
	coolant_temp = coolant_temp + ((reactor_temp - coolant_temp) * .15);

	/* Reactor cannot get below room temp. */
	if (reactor_temp < 70) {
		reactor_temp = 70;
	}

	/* Fuzz the reactor temp a bit for realism. */
	reactor_temp = reactor_temp + get_fuzz() * rand_sign();

	/* RETRACTING THE RODS INCREASES THE TEMP */
	/* for each unit the rod is not fully extracted, add a random float up to 50. */
	float bump = 0;
	int i;
	int rod_factor = MAX_SAFE_DEPTH - rod_depth;

	for (i = rod_factor; i > 0; i--) {
		bump = bump + float_up_to(20);
	}
	reactor_temp = reactor_temp + bump;

	/* SAFETY PROTOCOLS */
	if (safety_enabled == true && reactor_temp > 2000) {
		safety_active = 1;
		console_printf("\n ****** SAFETY PROTOCOLS ENGAGED: Extending control rods! *******\n\n");
		if (rod_depth <= MAX_SAFE_DEPTH) {
			/* Automatically increment rod_depth to cool reactor. */
			rod_depth++;
		}

		console_printf("\n ****** SAFETY PROTOCOLS ENGAGED: Increasing coolant flow! *******\n\n");
		if (coolant_flow <= MAX_FLOW_RATE) {
			coolant_flow = coolant_flow + 1;
			if (coolant_flow > MAX_FLOW_RATE) {
				coolant_flow = MAX_FLOW_RATE;
			}
		}
	}

	if (safety_active == 1 && reactor_temp < 2000) {
		safety_active = 0;
		console_printf("\n\n******* NORMAL OPERATING TEMPERATURE ACHIEVED ********\n\n");
	}
}

void reactor_status(void) {
	FUNCNAME

	/* Update status. */
	status_update();

	/* Clear console. */
	console_clear();

	print_menu();
	get_and_do_choice();
	update_reactor();

	/* Check if rod depth is safe. */
	if (rod_depth < 0 || rod_depth > MAX_SAFE_DEPTH) {
		console_clear();
		print_sparks();
		console_printf("WARNING! WARNING! WARNING!\n");
		console_printf("CONTAINMENT VESSEL RUPTURE!\n");
		console_printf("CONTROL RODS EXTENDED THROUGH CONTAINMENT VESSEL!!!\n");
	    console_printf("RADIATION LEAK - EVACUATE THE AREA!\n\n");
		print_sparks();
		exit_reason = exit_reason_fail;
	}

	console_wait_until_press();

	return;
}


int main(int argc, char *argv[]) {
	/* Initialize ncurses. */
	initscr();

	/* Initialize windows. */
	status_init();
	console_init();

	/* Seed RNG with time. */
	srand(time(NULL));

	while (1) {
		reactor_status();
		if (exit_reason != exit_reason_none) {
			break;
		}
	}

	/* Close ncurses window. */
	endwin();

	/* Return -1 on failure. */
	if(exit_reason == exit_reason_fail) {
		return -1;
	}

	return 0;
}
