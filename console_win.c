#include "console_win.h"
#include <assert.h>
#include <curses.h>
#include <limits.h>
#include <ncurses.h>
#include <pthread.h>

#include <stdlib.h>

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

static int _get_char_impl(void) {
	int out;
	do {
		out = wgetch(g_window);
		if(_get_and_clear_intrpt()) {
			return ERR;
		}
	} while(out == ERR);

	return out;
}

void _insert_char_into_str(char* str, int pos, int len) {
}

void _shift_str_backward(char* str, int start_pos, int len, int x, int y) {
	wmove(g_window, y, x + start_pos);

	int pos = start_pos;
	while(pos < len - 1 && str[pos] != 0) {
		str[pos] = str[pos + 1];
		if(str[pos] != 0) {
			waddch(g_window, str[pos]);
		}
		pos++;
	}
	waddch(g_window, ' ');

	wmove(g_window, y, x + start_pos);
}

void console_init(void) {
	if(g_initialized) {
		return;
	}

	/* Create our window. */
	g_window = newwin(CONSOLE_WIN_H, CONSOLE_WIN_W, CONSOLE_WIN_Y, CONSOLE_WIN_X);
	assert(g_window != NULL);

	/* Disable echoing. */
	noecho();

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
	char ch = (char)_get_char_impl();
	waddch(g_window, ch);
	return ch;
}

bool console_read_str(char* out) { return console_read_strn(out, INT_MAX); }

bool console_read_strn(char* out, int max_len) {
	/* TODO: Support backspace etc. */
	int ch;
	int pos = 0;
	int cur_size = 0;
	bool res = true;
	bool done = false;

	/* Get current position. */
	int start_x, start_y;
	getyx(g_window, start_y, start_x);

	while(pos < max_len - 1 && !done) {
		ch = _get_char_impl();

		/* Process char. */
		switch(ch) {
			case ERR: // We were interrupted, quit. 
				res = false;
			case '\n':
			case '\r': // I think this is a window-ism, but eh
				done = true;
				break;
			case KEY_BACKSPACE:
			case '\b':
			case 127:
				/* Remove char at previous position. */
				_shift_str_backward(out, pos - 1, max_len, start_x, start_y);
				--pos;
				break;
			default:
				/* Add char to buffer. */
				waddch(g_window, ch);
				out[pos++] = ch;
				cur_size++;
				break;
		}
	}

	out[cur_size] = 0;
	waddch(g_window, '\n');

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
