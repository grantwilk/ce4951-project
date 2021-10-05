/*
 * Luke Schlavensky
 * 10/3/21
 * state.h
 */

#ifndef STATE_H
#define STATE_H

#include "error.h"


/**
 * @brief   The states
 */
typedef enum { BUSY, IDLE, COLLISION } STATE_TYPE;


/**
 * @brief   Sets the current state
 */
ERROR_CODE state_set(STATE_TYPE state);


/**
 * @brief   Gets the current state
 */
STATE_TYPE state_get();


#endif // STATE_H