/*
 * Luke Schlavensky
 * 10/3/21
 * state.c
 *
 * Description: This file is responsible
 * for setting and getting the current state
 */

#include "state.h"


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
        //Set only IDLE LED
    }else if(state == BUSY)
    {
        current_state = BUSY;
        //Set only BUSY LED
    }else if(state == COLLISION)
    {
        current_state = COLLISION;
        //Set only COLLISION LED
    }else
    {
        THROW_ERROR(ERROR_CODE_SET_UNKNOWN_STATE);
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