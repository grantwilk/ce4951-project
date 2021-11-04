/* ------------------------------------------- Header ------------------------------------------- */


/**
 * @file    main.c
 * @brief   Firmware entry point
 */


/* ------------------------------------------ Includes ------------------------------------------ */


# include <stdio.h>
# include <string.h>
#include <stdlib.h>
#include <ctype.h>
# include "main.h"
# include "sysclock.h"
# include "uio.h"
# include "uart.h"

# include "leds.h"
# include "network.h"
# include "channel_monitor.h"
# include "timeout.h"
# include "state.h"
# include "error.h"


/* ------------------------------------------ Defines ------------------------------------------- */


# define CE4981_NETWORK_TIMEOUT_PERIOD_US   ( 1100U )
# define CE4981_NETWORK_MAX_MESSAGE_SIZE    ( 256 + 5) // I added 5 bytes due to Address Size in the UART




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
            if(destinationAddr == 0x00)
            {
                uprintf("[ Broadcast from 0x%02X: %s ]\n", receiveAddr, networkRxBuffer);
                uartRxReprint();
            }
            else if(destinationAddr == get_local_machine_address())
            {
                //print message
                uprintf("[ From 0x%02X: %s ]\n", receiveAddr, networkRxBuffer);
                uartRxReprint();
            }


        }
        //if uart has full string get it and place it in transmit buffer.
        else if (uartRxReady())
        {
            //get user message to transmit
            fgets(uartRxBuffer, CE4981_NETWORK_MAX_MESSAGE_SIZE, stdin);
            fflush(stdin);

            // remove newline from message
            uartRxBuffer[strlen(uartRxBuffer) - 1] = 0x00;
            rxBufferSize = strlen(uartRxBuffer);

            //check if setting address
            if(!strncmp(uartRxBuffer, "/setaddr", 8))
            {
                char newAddress[3] = {uartRxBuffer[11], uartRxBuffer[12], '\0'};
                set_local_machine_address((uint8_t)strtol(newAddress, NULL, 16));
                uprintf("[ Local address set to 0x%02X ]\n", get_local_machine_address());
            }
            else if(uartRxBuffer[0] != '0' || (uartRxBuffer[1] != 'x' && uartRxBuffer[1] != 'X') ||
                !isxdigit(uartRxBuffer[2]) || !isxdigit(uartRxBuffer[3]) || uartRxBuffer[4] != ' ')
            {
                ERROR_HANDLE_NON_FATAL(ERROR_CODE_INVALID_UART_INPUT);
            }
            else
            {
                // Get Address from input
                char address[2] = {uartRxBuffer[2], uartRxBuffer[3]};
                uint8_t destinationAddress = (uint8_t)strtol(address, NULL, 16);

                // Get Message from input
                char message[CE4981_NETWORK_MAX_MESSAGE_SIZE];
                memcpy(message, uartRxBuffer+5, CE4981_NETWORK_MAX_MESSAGE_SIZE);
                // Size of the message
                unsigned int messageSize = rxBufferSize - 5;

                //check for preset transmissions
                if(!strcmp(message,".zeros")) {
                    memset(message, 0x00, 8);
                    messageSize = 8;
                }
                else if (!strcmp(message, ".ones")) {
                    memset(message, 0xFF, 8);
                    messageSize = 8;
                }

                if(destinationAddress == 0x00)
                {
                    uprintf("[ Broadcast: %s ]\n", message);
                } else {
                    uprintf("[ To 0x%02X: %s ]\n", destinationAddress, message);
                }

                ERROR_HANDLE_FATAL(network_tx(destinationAddress, (uint8_t *) message, messageSize));
            }
        }
    }
}


/* ---------------------------------------------------------------------------------------------- */