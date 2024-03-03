#pragma once
#include <stdbool.h>

#define CONSOLE_WIN_X 0
#define CONSOLE_WIN_Y 10
#define CONSOLE_WIN_W 80
#define CONSOLE_WIN_H 15

/**
 * Console printf.
 *
 * This is functionally equivilant to standard printf (see https://en.cppreference.com/w/c/io/fprintf)
 */
void console_printf(const char* fmt, ...);

/**
 * Read a character from the console.
 *
 * This may return early if a critical error occurs (e.g. the reactor blows up), in which
 * case `ERR` is returned.
 *
 * @return  single char or `ERR`. */
char console_read_chr(void);

/**
 * Reads a string from the console (no size limit).
 *
 * This may return early if a critical error occurs (e.g. the reactor blows up), in which
 * case false is returned.
 *
 * @param[out] out  Location to store string.
 * @return success. */
bool console_read_str(char* out);

/**
 * Reads a string of at most size `max_len` from the console.
 *
 * This may return early if a critical error occurs (e.g. the reactor blows up), in which
 * case false is returned.
 *
 * @param[out] out  Location to store string.
 * @param max_len  Max length string that should be read. 
 * @return success. */
bool console_read_strn(char* out, int max_len);

/**
 * Wait/Block until the user enters any key into the console.
 *
 * This may return early if a critical error occurs (e.g. the reactor blows up). */
void console_wait_until_press(void);

/** Clear the console. */
void console_clear(void);


/**************************************************************
 *          Don't worry about anything below here :)          *
 **************************************************************/

/** Init the console. */
void console_init(void);

/** Finalize the console. */
void console_end(void);

/** Reset the visible cursor back to the console. */
void console_refresh_cursor(void);

void console_interrupt(void);
void console_clear_interrupt(void);
