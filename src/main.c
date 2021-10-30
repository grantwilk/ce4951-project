/* ------------------------------------------- Header ------------------------------------------- */


/**
 * @file    main.c
 * @brief   Firmware entry point
 */


/* ------------------------------------------ Includes ------------------------------------------ */


# include <stdio.h>
# include <string.h>
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
    unsigned int rxBufferSize;

    // TODO: These line is a temporary fix. The first transmission after reset
    //       causes a collision. By transmitting one byte at startup, we collide
    //       on reset, which is more OK. Ideally this doesn't happen though.
    GPIOC->ODR &= ~(GPIO_ODR_OD11);
    GPIOC->ODR |= GPIO_ODR_OD11;
    network_rx_queue_reset();

    uint8_t buf[8] = {0x55, 0x01, 0x08, 0x52, 0x01, 0x01, 0x41, 0x00};

    uint8_t result = crc8_calculate(buf, 8, 0x55);
    uprintf("crc calculation: %x\n", result);

    buf[7] = result;

    result = crc8_calculate(buf, 8, 0x55);
    uprintf("inverse: %x\n", result);

    while(1){}


    // while(1)
    // {
    //     //try a network read to check buffer.
    //     if(network_rx((uint8_t *) networkRxBuffer, &receiveAddr))
    //     {
    //         //print message
    //         uprintf("[ From 0x%02X: %s ]\n", receiveAddr, networkRxBuffer);
    //         uartRxReprint();
    //     }
    //     //if uart has full string get it and place it in transmit buffer.
    //     if(uartRxReady())
    //     {
    //         //get user message to transmit
    //         fgets(uartRxBuffer, CE4981_NETWORK_MAX_MESSAGE_SIZE, stdin);
    //         fflush(stdin);

    //         // remove newline from message
    //         uartRxBuffer[strlen(uartRxBuffer) - 1] = 0x00;
    //         rxBufferSize = strlen(uartRxBuffer);

    //         //check for preset transmissions
    //         if(!strcmp(uartRxBuffer,"/zeros")) {
    //             memset(uartRxBuffer, 0x00, 8);
    //             rxBufferSize = 8;
    //         }
    //         else if (!strcmp(uartRxBuffer, "/ones")) {
    //             memset(uartRxBuffer, 0xFF, 8);
    //             rxBufferSize = 8;
    //         }

    //         uprintf("[ To 0x%02X: %s ]\n", 0x00, uartRxBuffer);
    //         ERROR_HANDLE_FATAL(network_tx(0x00, (uint8_t *) uartRxBuffer, rxBufferSize));
    //     }
    // }
}


/* ---------------------------------------------------------------------------------------------- */