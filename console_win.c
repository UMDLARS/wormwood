#include "console_win.h"
#include <ncurses.h>

static WINDOW* g_window = NULL;

void console_init(void) {
    /* Create our window. */
    g_window = newwin(CONSOLE_WIN_H, CONSOLE_WIN_W, CONSOLE_WIN_Y, CONSOLE_WIN_X);
}

void console_end(void) {
    /* Destroy our window. */
    delwin(g_window);
}

void console_clear(void) { wclear(g_window); }

void console_printf(const char* fmt, ...) {
    /* Print to console then refresh console. */
    va_list lst;
    va_start(lst, fmt);
    vw_printw(g_window, fmt, lst);
    wrefresh(g_window);
}

char console_read_chr(void) { return wgetch(g_window); }
void console_read_str(char* out) { wgetstr(g_window, out); }
void console_read_strn(char* out, int max_len) { wgetnstr(g_window, out, max_len); }

void console_wait_until_press(void) {
    console_printf("Press any key to continue.");
    wgetch(g_window);
}
