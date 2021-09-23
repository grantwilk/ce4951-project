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

    // print hello world message
    uprintf("Hello, world!");

    // enter endless loop
    while(1);
}


/* ---------------------------------------------------------------------------------------------- */