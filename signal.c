#include "signal.h"
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <ncurses.h>

static void _error_handler(int sig) {
	/* Close ncurses window. */
	endwin();

    fputs("Congrats! You broke it!\n", stderr);
}

static void _exit_handler(int sig) {
	/* Close ncurses window. */
	endwin();

    /* Exit. */
    exit(0);
}

// https://www.gnu.org/software/libc/manual/html_node/Termination-Signals.html
// https://man7.org/linux/man-pages/man2/sigaction.2.html
void signal_setup_handlers(void) {
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
