#pragma once
#include <stdbool.h>
#include "reactor.h"


usermode_t reactor_get_usermode(void);
void reactor_set_usermode(usermode_t mode);

bool reactor_get_safety(void);
void reactor_set_safety(bool enabled);

bool reactor_get_safety_active(void);

unsigned char reactor_get_rod_depth(void);
void reactor_set_rod_depth(unsigned char depth);

float reactor_get_coolant_flow(void);
void reactor_set_coolant_flow(float flow);

float reactor_get_temp(void);
float reactor_get_coolant_temp(void);


/**************************************************************
 *          Don't worry about anything below here :)          *
 **************************************************************/

reactor_state_t reactor_get_state(void);

void reactor_update(void);

typedef enum {
	reactor_mode_norealtime,
	reactor_mode_realtime,
	reactor_mode_count
} reactor_mode_t;

void reactor_init(reactor_mode_t mode);
void reactor_end(void);
reactor_mode_t reactor_get_mode(void);
