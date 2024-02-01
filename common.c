#include "common.h"

exit_reason_t exit_reason = exit_reason_none;
char rod_depth = 16;
float coolant_flow = 10;
float coolant_temp = 70.0;
float reactor_temp = 70.0;
int usermode = 0;
bool safety_enabled = true;
bool safety_active = false;

const char *users[] = {"NA", "oper", "super"};
