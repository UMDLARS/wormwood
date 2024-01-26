#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <ncurses.h>

#define DEBUG 0
#define FUNCNAME if (DEBUG) printw("In %s @ line %d...\n", __func__, __LINE__);

#define MAX_SAFE_DEPTH 16
#define MAX_FLOW_RATE 100.0
#define TRUE 1
#define FALSE 0

typedef enum {
	exit_reason_none = 0,
	exit_reason_fail,
	exit_reason_quit,
} exit_reason_t;

exit_reason_t exit_reason = exit_reason_none;
char rod_depth = 16;
float coolant_flow = 10;
float coolant_temp = 70.0;
float reactor_temp = 70.0;
int usermode = 0;
int safety_enabled = TRUE;
int safety_active = 0;

const char *users[] = {"NA", "oper", "super"};

void wait_for_any_key() {
	printw("Press any key to continue.");
	refresh();
	getch();
}

char *draw_rod_depth(char rod_depth, char *depth_hist) {
	int idx = 0;
	int i;
	memset(depth_hist, '\0', 256);

	/* If depth is negative, draw to left of bracket. */
	if (depth_hist < 0) {
		i = rod_depth * -1;

		for (i; i > 0; i--) {
			depth_hist[idx] = '=';
			idx++;
		}
	}

	/* Draw brackets and whitespace starting at 0. */
	depth_hist[idx] = '[';
	depth_hist[idx + 17] = ']';
	idx++;
	for (i = 0; i < 16; i++) {
		depth_hist[idx + i] = ' ';
	}

	/* Depth is positive, draw to right of bracket. */
	for (i = 0; i < rod_depth; i++) {
		depth_hist[idx] = '=';
		idx++;
	}

	/* Draw final bracket at max depth. */

	return depth_hist;
}

void clear_screen_print_status() {
	char depth_hist[256];

	/* Generate timestamp string. */
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char timestring[64];
	sprintf(timestring, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	char *safetystring = "[ENABLED]";
	if (safety_enabled == 0) {
		safetystring = "<<<DISABLED>>>";
	}

	/* Clear screen first. */
	clear();

	/* Print status message. */
	printw("+------------------------------------------------------------------------+\n");
	printw("| JERICHO NUCLEAR REACTOR STATUS PANEL             (%s) |\n", timestring);
	printw("+------------------------------------------------------------------------+\n");
	printw("| reactor temp: %8.2f              coolant_temp: %8.2f             |\n", reactor_temp, coolant_temp); 
	printw("| rod_depth: %2d --[ %s ]--  coolant flow rate: %5.2f     |\n", rod_depth, draw_rod_depth(rod_depth, depth_hist), coolant_flow); 
	printw("| User: %-10s                                                       |\n", users[usermode]);
	printw("+------------------------------------------------------------------------+\n");
	printw("| SAFETY PROTOCOLS: %14s                                       |\n", safetystring);
	printw("+------------------------------------------------------------------------+\n");
}

void get_string(char *dest) {
	char input_buffer[8192];
	getstr(input_buffer); // Creates a new vuln, do we want this?
	strcpy(dest, input_buffer);
}

void print_sparks() {
	int i = 500;
	int j = 0;
	while (i > 0) {
		for (j = (rand() % 10); j > 0; j--) {
			printw(" ");
			i--;
		}
		printw("*");
		i--;
	}

	printw("\n");
}

void auth_user() {
	FUNCNAME

	char user_user[32];
	char user_pass[32];
	int userid = 0;
	char *passes[] = {"NA", "HomerSimpson", "Artemisia1986"};

	/* Get username and put into user_user. */
	printw("WARNING: UNAUTHORIZED ACCESS IS PUNISHABLE BY LAW!\n");
    printw("Which role (%s or %s)?: ", users[1], users[2]);
	refresh();
	get_string(user_user);

	printw("Password for user '");
	printw(user_user);
	printw("': ");
	refresh();

	get_string(user_pass);

	/* Check if provided user is valid & set usermode accordingly. */
	if (strcmp(user_user, users[1]) == 0) {
		userid = 1;
	}
	else if (strcmp(user_user, users[2]) == 0) {
		userid = 2;
	}
	else {
		printw("AUTHENTICATION FAILED (no such user)\n");
		usermode = 0;
		return;
	}

	/* Check if provided password is correct. */
	if (strcmp(user_pass, passes[userid]) == 0) {
		printw("User '%s' AUTHENTICATED!\n", users[userid]);
		usermode = userid;
	} 
	else {
		printw("AUTHENTICATION FAILED (incorrect password)\n");
	}

	usermode = 2;
	return;
}

void print_menu() {
	printw("Actions (choose one):\n");

	/* Print auth if we're not in supervisor mode. */
	if (usermode != 2) {
		printw("(A) - Authenticate\n");
	}

	if (usermode != 0) {
		/* Print all normal oper options. */
		printw("(R) - Set rod depth\n");
		printw("(F) - Set coolant flow rate\n");

		/* Allow enable/disable safety if we're in supervisor mode. */
		if(usermode == 2) {
			if (safety_enabled == TRUE) {
				printw("(D) - Disable automatic safety control (* SUPER ONLY *)\n");
			} else {
				printw("(E) - Enable automatic safety control (* SUPER ONLY *)\n");
			}
		}

		/* If authenticated, provide option to log out. */
		printw("(L) - Log out\n");

		printw("(?) - Any other choice - wait\n");
	}

	/* Always print Quit. */
	printw("(Q) - Quit\n");
}

void set_rod_depth() {
	int new = 0;
	char answer[256];
	printw("What should the new rod depth be (0-16) [current: %d]?: ", rod_depth);
	refresh();
	get_string(answer);
	new = atoi(answer);

	printw("new: %d\n", new);
	if (new <= MAX_SAFE_DEPTH) {
		rod_depth = new;
	}
	else {
		printw("New depth value %d is greater than %d -- ignoring!", new, MAX_SAFE_DEPTH);
	}
}

void set_flow_rate() {
	float new = 0;
	char answer[256];
	printw("What should the new flow rate be (0.0-100.0) [current: %03.2f]?: ", coolant_flow);
	get_string(answer);
	new = atof(answer);

	if (new > MAX_FLOW_RATE) {
		printw("New flow rate (%03.2f) is greater than max %03.2f -- ignoring!\n", new, MAX_FLOW_RATE);
	}

	if (new < 10 && usermode < 2) {
		printw("User 'oper' cannot set flow rate below 10! -- ignoring!\n");
	}
	else {
		printw("New coolant flow rate: %03.2f (was: %03.2f)\n", new, coolant_flow);
		coolant_flow = new;
	}
}

void get_and_do_choice() {
	char choice_string[256];
	printw("Enter your selection (ARFDEL) and then press ENTER.\n");
	refresh();

	char choice = getch();

	/* Clear screen after the user enters an option. */
	clear_screen_print_status();

	//printw("Your choice was: '%c'\n", choice);

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
			safety_enabled = FALSE;
			printw("***** SAFETY PROTOCOLS DISABLED!!! *****\n");
			break;
		case 'e':
			safety_enabled = TRUE;
			printw("***** SAFETY PROTOCOLS ENABLED!!! *****\n");
			break;
		case 'l':
			printw("Deauthenticating.\n");
			usermode = 0;
			break;
		case 'q':
			printw("Quitting...\n");
			exit_reason = exit_reason_quit;
			break;
		default:
			printw("Doing nothing...\n");
			break;
	}

	return;
}

float float_up_to(int max) {
	// https://stackoverflow.com/questions/13408990/how-to-generate-random-float-number-in-c
	float fuzz = (float)rand()/(float)(RAND_MAX/max);
	return fuzz;
}

float get_fuzz() {
	return float_up_to(1);
}

int rand_sign() {
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

void update_reactor() {
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
		printw("\n***** WARNING: REACTOR COOLANT WILL VAPORIZE AT 5000 DEGREES ******\n");
		if(reactor_temp > 4000) {
			printw("***** WARNING: IMMINENT BREACH! IMMINENT BREACH! ******\n");
		}
		printw("\n");
	}

	/* Fail/throw error if reactor temp goes over 5K. */
	if (reactor_temp >= 5000) {
		clear();
		print_sparks();
		printw("****** COOLANT VAPORIZATION *******\n");
		printw("****** CONTAINMENT VESSEL VENTING *******\n");
		printw("****** MAJOR RADIOACTIVITY LEAK!!! *******\n\n");
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
	if (safety_enabled == TRUE && reactor_temp > 2000) {
		safety_active = 1;
		printw("\n ****** SAFETY PROTOCOLS ENGAGED: Extending control rods! *******\n\n");
		if (rod_depth <= MAX_SAFE_DEPTH) {
			/* Automatically increment rod_depth to cool reactor. */
			rod_depth++;
		}

		printw("\n ****** SAFETY PROTOCOLS ENGAGED: Increasing coolant flow! *******\n\n");
		if (coolant_flow <= MAX_FLOW_RATE) {
			coolant_flow = coolant_flow + 1;
			if (coolant_flow > MAX_FLOW_RATE) {
				coolant_flow = MAX_FLOW_RATE;
			}
		}
	}

	if (safety_active == 1 && reactor_temp < 2000) {
		safety_active = 0;
		printw("\n\n******* NORMAL OPERATING TEMPERATURE ACHIEVED ********\n\n");
	}
}

void reactor_status() {
	FUNCNAME

	/* Clear screen. */
	clear_screen_print_status();

	print_menu();
	get_and_do_choice();
	update_reactor();

	/* Check if rod depth is safe. */
	if (rod_depth < 0 || rod_depth > MAX_SAFE_DEPTH) {
		clear();
		print_sparks();
		printw("WARNING! WARNING! WARNING!\n");
		printw("CONTAINMENT VESSEL RUPTURE!\n");
		printw("CONTROL RODS EXTENDED THROUGH CONTAINMENT VESSEL!!!\n");
	    printw("RADIATION LEAK - EVACUATE THE AREA!\n\n");
		print_sparks();
		refresh();
		exit_reason = exit_reason_fail;
	}

	wait_for_any_key();

	return;
}


int main(int argc, char *argv[]) {
	/* Initialize ncurses. */
	initscr();

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
