/* --------------------------------- Header --------------------------------- */


/**
 * @file    backoff.h
 * @brief   Contains functions for the backoff timer (TIM5)
 */


/* ------------------------------ Header Guard ------------------------------ */


# ifndef DRIVER_BACKOFF_H
# define DRIVER_BACKOFF_H


/* -------------------------------- Includes -------------------------------- */


# include "error.h"
#include <stdbool.h>


/* ------------------------------- Functions -------------------------------- */


ERROR_CODE backoff_init( uint16_t ms );

ERROR_CODE backoff_start();
ERROR_CODE backoff_stop();
ERROR_CODE backoff_reset();

ERROR_CODE backoff_set_period( uint16_t ms );

bool backoff_is_running();


/* --------------------------------- Footer --------------------------------- */


# endif // DRIVER_BACKOFF_H


/* -------------------------------------------------------------------------- */