/* ------------------------------------------ Header -------------------------------------------- */


/**
 * @file    uio.h
 * @brief   Contains functions for serial I/O via UART
 */


/* ---------------------------------------- Header Guard ---------------------------------------- */


# ifndef UTIL_UIO_H
# define UTIL_UIO_H


/* ------------------------------------------ Includes ------------------------------------------ */


# include <stdio.h>
# include <stdarg.h>
# include "error.h"


/* ----------------------------------------- Functions ------------------------------------------ */


/**
 * @brief   Initializes the peripherals required for UART printf
 *
 * @param   [in]    baudRate        The baud rate of the internal UART peripheral
 * @param   [in]    timeout         The transmit timeout in microseconds
 *
 * @return  Error code
 */
ERROR_CODE
uinit
(
    uint32_t    baudRate,
    uint32_t    timeout
);


/**
 * @brief   Prints a formatted string via UART
 *
 * @param   [in]    *fmt    The formatted string
 * @param   [in]    ...     The string's arguments
 *
 * @return  Error code
 */
ERROR_CODE
uprintf
(
    const char *fmt,
    ...
);


/**
 * @brief   Prints a hex dump of the memory at the specified address via UART.
 *          Source code adapted from https://stackoverflow.com/a/7776146.
 *
 * @param   [in]    *address    The address to begin printing from
 * @param   [in]    size        The number of bytes to print
 *
 * @return  Error code
 */
ERROR_CODE
udump
(
    void        *address,
    uint32_t    size
);


/* ------------------------------------------- Footer ------------------------------------------- */


# endif // UTIL_UIO_H


/* ---------------------------------------------------------------------------------------------- */