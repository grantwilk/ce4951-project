/* ------------------------------------------- Header ------------------------------------------- */


/**
 * @file    main.c
 * @brief   Firmware entry point
 */


/* ------------------------------------------ Includes ------------------------------------------ */


# include <stdio.h>
# include <string.h>
#include <stdlib.h>
# include "main.h"
# include "sysclock.h"
# include "uio.h"
# include "uart.h"

# include "leds.h"
# include "network.h"
# include "channel_monitor.h"
# include "timeout.h"
# include "state.h"



/* ------------------------------------------ Defines ------------------------------------------- */


# define CE4981_NETWORK_TIMEOUT_PERIOD_US   ( 1100U )
# define CE4981_NETWORK_MAX_MESSAGE_SIZE    ( 256 )


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
    ERROR_HANDLE_FATAL( network_init() );
    ERROR_HANDLE_FATAL( channel_monitor_init() );

    // start timeout timer
    ERROR_HANDLE_FATAL( timeout_init( CE4981_NETWORK_TIMEOUT_PERIOD_US ) );

    // initialize leds
    ERROR_HANDLE_FATAL( leds_init() );

    // set initial state to IDLE
    ERROR_HANDLE_FATAL( state_set( IDLE ) );

    // UART buffer
    char uartRxBuffer[CE4981_NETWORK_MAX_MESSAGE_SIZE];
    // network recive buffer
    char networkRxBuffer[CE4981_NETWORK_MAX_MESSAGE_SIZE];

    uint8_t receiveAddr;
    uint8_t destinationAddr;
    unsigned int rxBufferSize;

    // Local Machine Address
    uint8_t localMachineAddress = 0xFF;

    // TODO: These line is a temporary fix. The first transmission after reset
    //       causes a collision. By transmitting one byte at startup, we collide
    //       on reset, which is more OK. Ideally this doesn't happen though.
    GPIOC->ODR &= ~(GPIO_ODR_OD11);
    GPIOC->ODR |= GPIO_ODR_OD11;
    network_rx_queue_reset();

    while(1)
    {
        //try a network read to check buffer.
        if(network_rx((uint8_t *) networkRxBuffer, &receiveAddr, &destinationAddr))
        {
            if(localMachineAddress == destinationAddr)
            {
                //print message
                uprintf("[ From 0x%02X: %s ]\n", receiveAddr, networkRxBuffer);
                uartRxReprint();
            }


        }
        //if uart has full string get it and place it in transmit buffer.
        if(uartRxReady())
        {
            //get user message to transmit
            fgets(uartRxBuffer, CE4981_NETWORK_MAX_MESSAGE_SIZE, stdin);
            fflush(stdin);

            // remove newline from message
            uartRxBuffer[strlen(uartRxBuffer) - 1] = 0x00;
            rxBufferSize = strlen(uartRxBuffer);

            //check for preset transmissions
            if(!strcmp(uartRxBuffer,"/zeros")) {
                memset(uartRxBuffer, 0x00, 8);
                rxBufferSize = 8;
            }
            else if (!strcmp(uartRxBuffer, "/ones")) {
                memset(uartRxBuffer, 0xFF, 8);
                rxBufferSize = 8;
            }

            // Get Address from input
            char address[2] = {uartRxBuffer[2], uartRxBuffer[3]};
            unsigned int addressHEX = (int)strtol(address, NULL, 16);

            // Get Message from input
            char message[CE4981_NETWORK_MAX_MESSAGE_SIZE];
            memcpy(message, uartRxBuffer+5, CE4981_NETWORK_MAX_MESSAGE_SIZE);
            // Size of the message
            unsigned int messageSize = rxBufferSize - 5;

            uprintf("[ To 0x%02X: %s ]\n", addressHEX, message);
            ERROR_HANDLE_FATAL(network_tx(addressHEX, (uint8_t *) message, messageSize, localMachineAddress));
        }
    }
}


/* ---------------------------------------------------------------------------------------------- */