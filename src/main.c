/* ------------------------------------------- Header ------------------------------------------- */


/**
 * @file    main.c
 * @brief   Firmware entry point
 */


/* ------------------------------------------ Includes ------------------------------------------ */


#include <string.h>
# include "main.h"
# include "sysclock.h"
# include "uio.h"

# include "leds.h"
# include "network.h"
# include "channel_monitor.h"
# include "timeout.h"
# include "state.h"


/* ------------------------------------------ Defines ------------------------------------------- */


# define CE4981_NETWORK_TIMEOUT_PERIOD_US  ( 1100U )


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
    uprintf("/* ---------- DEVICE RESET ---------- */\n\n");

    // start network
    ERROR_HANDLE_FATAL( channel_monitor_init() );
    ERROR_HANDLE_FATAL( network_init() );

    // start timeout timer
    ERROR_HANDLE_FATAL( timeout_init( CE4981_NETWORK_TIMEOUT_PERIOD_US ) );

    // initialize leds
    ERROR_HANDLE_FATAL( leds_init() );

    // set initial state to IDLE
    ERROR_HANDLE_FATAL( state_set( IDLE ) );

    char * txBuf = "This is my tx buffer!";

    ERROR_HANDLE_FATAL( network_tx((void * ) txBuf, strlen(txBuf)));

    // enter endless loop
    //todo implement UART program, reading lines of text from user and sending via network_tx
    while(1);
}


/* ---------------------------------------------------------------------------------------------- */