/* ------------------------------------------- Header ------------------------------------------- */


/**
 * @file    main.h
 * @brief   Firmware entry point
 */


/* ---------------------------------------- Header Guard ---------------------------------------- */


# ifndef MAIN_H
# define MAIN_H


/* ------------------------------------------ Includes ------------------------------------------ */


# include <stm32f4xx.h>
# include "error.h"


/* ------------------------------------- Static Functions --------------------------------------- */


/**
 * @brief   Prints a reset header containing basic device information
 *
 * @param   void
 *
 * @return  Error code
 */
static ERROR_CODE printResetHeader( void );


/* ------------------------------------------- Footer ------------------------------------------- */


# endif // MAIN_H


/* ---------------------------------------------------------------------------------------------- */