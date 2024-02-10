#include "console_win.h"
#include <assert.h>
#include <limits.h>
#include <ncurses.h>
#include <pthread.h>

static bool g_initialized = false;

static WINDOW* g_window = NULL;

static bool g_intrpt_flag = false;
static pthread_mutex_t g_intrpt_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool _get_and_clear_intrpt(void) {
    bool out;
    pthread_mutex_lock(&g_intrpt_flag_mutex);
    out = g_intrpt_flag;
    g_intrpt_flag = false;
    pthread_mutex_unlock(&g_intrpt_flag_mutex);
    return out;
}

void console_init(void) {
    if(g_initialized) {
        return;
    }

    /* Create our window. */
    g_window = newwin(CONSOLE_WIN_H, CONSOLE_WIN_W, CONSOLE_WIN_Y, CONSOLE_WIN_X);
    assert(g_window != NULL);

    /* Set initialized flag. */
    g_initialized = true;
}

void console_end(void) {
    if(!g_initialized) {
        return;
    }

    /* Destroy our window. */
    delwin(g_window);

    /* Set initialized flag. */
    g_initialized = false;
}

void console_clear(void) { wclear(g_window); }

void console_printf(const char* fmt, ...) {
    /* Print to console then refresh console. */
    va_list lst;
    va_start(lst, fmt);
    vw_printw(g_window, fmt, lst);
    wrefresh(g_window);
}

char console_read_chr(void) {
    char out;
    do {
        out = wgetch(g_window);
        if(_get_and_clear_intrpt()) {
            return ERR;
        }
    } while(out == ERR);

    return out;
}

bool console_read_str(char* out) { return console_read_strn(out, INT_MAX); }

bool console_read_strn(char* out, int max_len) {
    /* TODO: Support backspace etc. */
    int idx = 0;
    char ch;
    while(idx < max_len - 1) {
        ch = console_read_chr();
        if(ch == ERR) {
            wprintw(g_window, "\n");
            return false;
        }
        else if(ch == '\n') {
            break;
        }

        out[idx++] = ch;
    }

    /* Move down one line. */
    wmove(g_window, getcury(g_window) + 1, 0);

    /* Clear last character. */
    out[idx] = 0;

    return true;
}

void console_wait_until_press(void) {
    console_printf("Press any key to continue.");
    console_read_chr();
}

void console_refresh_cursor(void) {
    int y, x;
    getyx(g_window, y, x);
    wmove(g_window, y, x);
    wrefresh(g_window);
}

void console_interrupt(void) {
    pthread_mutex_lock(&g_intrpt_flag_mutex);
    g_intrpt_flag = true;
    pthread_mutex_unlock(&g_intrpt_flag_mutex);
}

void console_clear_interrupt(void) {
    pthread_mutex_lock(&g_intrpt_flag_mutex);
    g_intrpt_flag = false;
    pthread_mutex_unlock(&g_intrpt_flag_mutex);
}
