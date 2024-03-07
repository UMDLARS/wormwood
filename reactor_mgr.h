#pragma once
#include <stdbool.h>
#include "reactor.h"


usermode_t reactor_mgr_get_usermode(void);
void reactor_mgr_set_usermode(usermode_t mode);

bool reactor_mgr_get_safety(void);
void reactor_mgr_set_safety(bool enabled);

bool reactor_mgr_get_safety_active(void);

unsigned char reactor_mgr_get_rod_depth(void);
void reactor_mgr_set_rod_depth(unsigned char depth);

float reactor_mgr_get_coolant_flow(void);
void reactor_mgr_set_coolant_flow(float flow);

float reactor_mgr_get_temp(void);
float reactor_mgr_get_coolant_temp(void);


/**************************************************************
 *          Don't worry about anything below here :)          *
 **************************************************************/

void reactor_mgr_update(void);

typedef enum {
	reactor_mgr_mode_norealtime,
	reactor_mgr_mode_realtime,
	reactor_mgr_mode_count
} reactor_mgr_mode_t;

void reactor_mgr_init(reactor_mgr_mode_t mode);
void reactor_mgr_end(void);
reactor_mgr_mode_t reactor_mgr_get_mode(void);
