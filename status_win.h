#pragma once
#include "reactor.h"

#define STATUS_WIN_X 0
#define STATUS_WIN_Y 0
#define STATUS_WIN_W 80
#define STATUS_WIN_H 9

void status_init(void);

void status_end(void);

void status_update(const reactor_state_t* state);
