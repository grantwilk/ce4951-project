/* ------------------------------------------ Header -------------------------------------------- */


/**
 * @file   error.c
 * @desc   Defines error codes and functions for handling errors
 */


/* ------------------------------------------ Includes ------------------------------------------ */


# include "error.h"
# include "uio.h"


/* ----------------------------------------- Functions ------------------------------------------ */


/**
 * @brief   Handles pre-fetch fault and memory access fault errors
 *
 * @param   void
 *
 * @return  void
 */
void errorBusFaultISR( void )
{
    ERROR_HANDLE_FAULT( "BUS_FAULT" );
}


/**
 * @brief   Handles hard fault errors
 *
 * @param   void
 *
 * @return  void
 */
void errorHardFaultISR( void )
{
    ERROR_HANDLE_FAULT( "HARD_FAULT" );
}


/**
 * @brief   Handles memory management fault errors
 *
 * @param   void
 *
 * @return  void
 */
void errorMemoryManagementFaultISR( void )
{
    ERROR_HANDLE_FAULT( "MEMORY_MANAGEMENT_FAULT" );
}


/**
 * @brief   Handles undefined instruction or illegal state errors
 *
 * @param   void
 *
 * @return  void
 */
void errorUsageFaultISR( void )
{
    ERROR_HANDLE_FAULT( "USAGE_FAULT" );
}


/* ---------------------------------------------------------------------------------------------- */