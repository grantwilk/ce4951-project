#ifndef STATE_H
#define STATE_H

#include "error.h"

typedef enum state { BUSY, IDLE, COLLISION } state_t;

ERROR_CODE state_set(state_t state);
state_t state_get();

#endif
