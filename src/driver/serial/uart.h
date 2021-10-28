/* ------------------------------------------ Header -------------------------------------------- */


/**
 * @file    uart.h
 * @brief   Contains functions for transmitting and receiving data via UART
 */


/* ---------------------------------------- Header Guard ---------------------------------------- */


# ifndef DRIVER_SERIAL_UART_H
# define DRIVER_SERIAL_UART_H


/* ------------------------------------------ Includes ------------------------------------------ */


# include <stdint.h>
# include <stdbool.h>
# include "error.h"


/* ------------------------------------------ Defines ------------------------------------------- */


/**
 * @brief   Value to indicate that UART byte exchanges should not timeout
 */
# define DRIVER_SERIAL_UART_NO_TIMEOUT 0


/* ---------------------------------- Constructors / Destructors -------------------------------- */


/**
 * @brief	Initializes UART for serial communication
 *
 * @param	[in]    baudRate        The baud rate of the UART peripheral
 *
 * @return 	Error code
 */
ERROR_CODE
uartInit
(
    uint32_t    baudRate
);


/* ----------------------------------------- Functions ------------------------------------------ */


/**
 * @brief	Transmits a buffer via UART
 *
 * @param	[in]	*txBuffer	    The buffer to transmit bytes from
 * @param	[in]	txBufferSize	The number of bytes to transmit
 * @param	[in]	timeout	        The transmit timeout in microseconds
 *
 * @return 	Error code
 */
ERROR_CODE
uartTxBuffer
(
    uint8_t	    *txBuffer,
    uint32_t	txBufferSize,
    uint16_t	timeout
);


/**
 * @brief	Receives a buffer via UART
 *
 * @param	[in]	*rxBuffer		The buffer to receive bytes to
 * @param	[in]	rxBufferSize    The number of bytes to receive
 * @param	[in]	timeout	        The receive timeout in microseconds
 *
 * @return 	Error code
 */
ERROR_CODE
uartRxBuffer
(
    uint8_t		*rxBuffer,
    uint32_t	rxBufferSize,
    uint16_t	timeout
);


/**
 * @brief   checks if last char recived is \n
 * @return  bool if a string ending in a newline exists in the buffer
 */
bool 
uartRxReady();

/**
 * allows for reprinting of message when message received in the middle of typing
 */
void uartRxReprint();

/* -------------------------------------- Static Functions -------------------------------------- */


/**
 * @brief	Transmits a byte via UART
 *
 * @param	[in]	txByte		The byte to transmit
 * @param	[in]	timeout	    The transmit timeout in microseconds
 *
 * @return 	Error code
 */
static ERROR_CODE
uartTxByte
(
    uint8_t     txByte,
    uint16_t	timeout
);


/**
 * @brief	Receives a byte via UART
 *
 * @param	[in]	*rxByte		The byte to receive
 * @param	[in]	timeout	    The receive timeout in microseconds
 *
 * @return 	Error code
 */
static ERROR_CODE
uartRxByte
(
    uint8_t 	*rxByte,
    uint16_t	timeout
);





/* ------------------------------------------- Footer ------------------------------------------- */


# endif // DRIVER_SERIAL_UART_H


/* ---------------------------------------------------------------------------------------------- */