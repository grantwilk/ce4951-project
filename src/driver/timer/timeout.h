/* --------------------------------- Header --------------------------------- */


/**
 * @file    timeout.h
 * @brief   Contains functions for the idle/conflict timeout timer (TIM3)
 */


/* ------------------------------ Header Guard ------------------------------ */


# ifndef DRIVER_TIMEOUT_H
# define DRIVER_TIMEOUT_H


/* -------------------------------- Includes -------------------------------- */


# include "error.h"
#include <stdbool.h>


/* ------------------------------- Functions -------------------------------- */


ERROR_CODE timeout_init( uint16_t us );

ERROR_CODE timeout_start();
ERROR_CODE timeout_stop();
ERROR_CODE timeout_reset();

ERROR_CODE timeout_set_timeout( uint16_t us );

bool timeout_is_running();

/* --------------------------------- Footer --------------------------------- */


# endif // DRIVER_TIMEOUT_H


/* -------------------------------------------------------------------------- */
