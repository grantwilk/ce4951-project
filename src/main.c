/* ------------------------------------------- Header ------------------------------------------- */


/**
 * @file    main.c
 * @brief   Firmware entry point
 */


/* ------------------------------------------ Includes ------------------------------------------ */


# include "main.h"
# include "sysclock.h"
# include "uio.h"


/* ----------------------------------------- Functions ------------------------------------------ */


/**
 * @brief  Firmware entry point
 *
 * @param  void
 *
 * @return Integer status
 */
int main( void )
{
    ERROR_CODE errorCode;

    // initialize STM32 HAL
    HAL_Init();

    // initialize system clocks
    errorCode = sysClockInit();
    ERROR_HANDLE_FATAL( errorCode );

    // initialize UIO for UART I/O
    errorCode = uinit( 115200, 10000 );
    ERROR_HANDLE_FATAL( errorCode );

    // print reset header
    // printResetHeader();

    // print hello world message
    uprintf("Hello, world!");

    // enter endless loop
    while(1);
}


/* ------------------------------------- Static Functions --------------------------------------- */


/**
 * @brief   Prints a reset header containing basic device information
 *
 * @param   void
 *
 * @return  Error code
 */
static ERROR_CODE printResetHeader( void )
{
    ERROR_CODE errorCode;

    errorCode = uprintf( "\n\n/* ================== DEVICE RESET ================== */\n" );
    ELEVATE_IF_ERROR( errorCode );

    errorCode = uprintf( "/* ---------------- BEGIN DEBUG PRINT --------------- */\n\n" );
    ELEVATE_IF_ERROR( errorCode );

    RETURN_NO_ERROR();
}


/* ---------------------------------------------------------------------------------------------- */