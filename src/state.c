/*
 * Luke Schlavensky
 * 10/3/21
 * state.c
 *
 * Description: This file is responsible
 * for setting and getting the current state
 */

#include "state.h"
#include "leds.h"


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
        ELEVATE_IF_ERROR(leds_clear());
        ELEVATE_IF_ERROR(leds_set(LED_GREEN,true));
    }else if(state == BUSY)
    {
        current_state = BUSY;
        ELEVATE_IF_ERROR(leds_clear());
        ELEVATE_IF_ERROR(leds_set(LED_YELLOW,true));
    }else if(state == COLLISION)
    {
        current_state = COLLISION;
        ELEVATE_IF_ERROR(leds_clear());
        ELEVATE_IF_ERROR(leds_set(LED_RED,true));
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