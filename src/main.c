/* ------------------------------------------- Header ------------------------------------------- */


/**
 * @file    main.c
 * @brief   Firmware entry point
 */


/* ------------------------------------------ Includes ------------------------------------------ */


# include "main.h"
# include "sysclock.h"
# include "uio.h"
# include "uart.h"

# include "leds.h"
# include "network.h"
# include "channel_monitor.h"
# include "timeout.h"
#include "state.h"


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

    // Initialize uart.c for reading and writing
    ERROR_HANDLE_FATAL(uartInit( 115200 ) );

    // start network
    ERROR_HANDLE_FATAL( channel_monitor_init() );
    ERROR_HANDLE_FATAL( network_init() );

    // start timeout timer
    ERROR_HANDLE_FATAL( timeout_init( CE4981_NETWORK_TIMEOUT_PERIOD_US ) );

    // initialize leds
    ERROR_HANDLE_FATAL( leds_init() );

    // set initial state to IDLE
    ERROR_HANDLE_FATAL( state_set( IDLE ) );

    // enter endless loop
    //todo implement UART program, reading lines of text from user and sending via network_tx

    // make buffer of max message size
    //char *buffer = new char[max_message_size];
    while(1)
    {
        //int msize = 0;
        //if(!network_tx_isFull()) {
        //  for(uint i = 1; i < buffer.size && i > 0 && buffer[i] != NULL; ++i) {
        //      if(buffer[i-2] == NULL) {
        //          //IF last character was null end loop with one less total character
        //          i = 0;
        //          --msize;
        //      } elsif(ERROR_CODE_DRIVER_SERIAL_UART_TIMEOUT == uartRxByte(*buffer[i-1], timeout)) {
        //          //IF pulling gives us a timeout end loop.
        //          i = 0; //end for loop
        //      } else {
        //          ++msize;
        //      }
        //  }
        //  network_tx(buffer, msize)
        //}
    }
}


/* ---------------------------------------------------------------------------------------------- */