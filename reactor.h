#pragma once
#include <stdbool.h>
#include <pthread.h>

extern pthread_mutex_t g_reactor_mutex;
extern bool g_is_reactor_realtime;

void update_reactor(void);

void process_reactor_warns(void);

void start_periodic_reactor_update(void);
void end_periodic_reactor_update(void);
