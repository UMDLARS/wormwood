#pragma once
#include <stddef.h>

#define CONSOLE_WIN_X 0
#define CONSOLE_WIN_Y 10
#define CONSOLE_WIN_W 80
#define CONSOLE_WIN_H 15

/* printf for the console, use in place of normal printf. */
void console_printf(const char* fmt, ...);

/* Read a single character from the console. */
char console_read_chr(void);

/* Read a string from the console. */
void console_read_str(char* out);

/* Read a string of length max_len or less from the console. */
void console_read_strn(char* out, int max_len);

/* Wait until the user presses any key in the console. */
void console_wait_until_press(void);

void console_init(void);

void console_end(void);

void console_clear(void);
