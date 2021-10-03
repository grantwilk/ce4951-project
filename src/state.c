/*
 * Luke Schlavensky
 * 10/3/21
 * state.c
 *
 * Description: This file is responsible
 * for setting and getting the current state
 */

#include "state.h"
//#include "error.h"


//Current State
STATE_TYPE current_state = IDLE;


/**
 * @brief   This function sets the current state
 *
 * @param   STATE_TYPE state - Try to switch to that state
 *
 * @return  ERROR_CODE - Throws no error unless the param is not a state
 */
ERROR_CODE state_set(STATE_TYPE state)
{
    if(state == IDLE)
    {
        current_state = IDLE;
    }else if(state == BUSY)
    {
        current_state = BUSY;
    }else if(state == COLLISION)
    {
        current_state = COLLISION;
    }else
    {
        //Throw an Error here
    }

    RETURN_NO_ERROR();
}


/**
 * @brief   This function returns the current state
 *
 * @param   void
 *
 * @return  STATE_TYPE - The current state
 */
STATE_TYPE state_get()
{
    return current_state;
}

