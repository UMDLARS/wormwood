#pragma once
#include <stdbool.h>
#include <pthread.h>

extern pthread_mutex_t g_reactor_mutex;

void reactor_update(void);

void reactor_process_warns(void);

bool reactor_is_realtime(void);

void reactor_start_realtime_update(void);
void reactor_end_realtime_update(void);
