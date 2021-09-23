/* ------------------------------------------ Header -------------------------------------------- */


/**
 * @file    uio.c
 * @brief   Contains functions for serial I/O via UART
 */


/* ------------------------------------------ Includes ------------------------------------------ */


# include <stdbool.h>
# include <stdint.h>
# include <string.h>
# include "uart.h"
# include "uio.h"


/* ----------------------------------- Static Global Variables ---------------------------------- */


/**
 * @brief   Buffer used to store incoming and outgoing bytes
 */
static uint8_t byteBuffer[1024];

/**
 * @brief   The transmit timeout in microseconds
 */
static uint32_t uioTimeout;

/**
 * @brief   UIO initialization flag
 */
static bool uioIsInit = false;


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
)
{
    ERROR_CODE errorCode;

    // throw an error if UIO is already initialized
    if (uioIsInit)
    {
        THROW_ERROR( ERROR_CODE_UTIL_UIO_ALREADY_INITIALIZED );
    }

    // initialize UART
    errorCode = uartInit( baudRate );
    ELEVATE_IF_ERROR( errorCode );

    // determine the timeout time based on baud rate
    uioTimeout = timeout;

    // set UIO init flag
    uioIsInit = true;

    RETURN_NO_ERROR();
}


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
)
{
    ERROR_CODE errorCode;

    // throw an error if UIO is not initialized
    if ( !uioIsInit )
    {
        THROW_ERROR( ERROR_CODE_UTIL_UIO_NOT_INITIALIZED );
    }

    // get variable arguments and create string
    va_list arg;
    va_start(arg, fmt);
    vsnprintf( ( char * ) byteBuffer, sizeof( byteBuffer ), fmt, arg );
    va_end(arg);

    uint32_t bufferLength = strlen( ( char * ) byteBuffer );

    // transmit the formatted string
    errorCode = uartTxBuffer(
        byteBuffer,
        bufferLength,
        uioTimeout
    );
    ELEVATE_IF_ERROR( errorCode );

    RETURN_NO_ERROR();
}


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
)
{
    ERROR_CODE errorCode;

    uint32_t i;
    uint8_t buff[17];
    const uint8_t *pc = ( const uint8_t * ) address;

    // check length
    if (size == 0)
    {
        RETURN_NO_ERROR();
    }

    for ( i = 0; i < size; i++ )
    {

        if ( ( i % 16 ) == 0 )
        {
            // don't print ascii buffer for the zeroth line
            if ( i != 0 )
            {
                errorCode = uprintf( "  %s\n", buff );
                ELEVATE_IF_ERROR( errorCode );
            }

            // output the offset
            errorCode = uprintf( "  %04X ", i );
            ELEVATE_IF_ERROR( errorCode );
        }

        // now the hex code for the specific character
        errorCode = uprintf(" %02x", pc[i]);
        ELEVATE_IF_ERROR( errorCode );

        // and buffer a printable ascii character for later
        if ( ( pc[i] < 0x20 ) || ( pc[i] > 0x7e ) )
        {
            buff[i % 16] = '.';
        }
        else
        {
            buff[i % 16] = pc[i];
        }
        buff[(i % 16) + 1] = '\0';
    }

    // pad out last line if not exactly 16 characters.
    while ( ( i % 16 ) != 0 )
    {
        errorCode = uprintf("   ");
        ELEVATE_IF_ERROR( errorCode );
        i++;
    }

    // print the final ascii buffer
    errorCode = uprintf("  %s\n", buff);
    ELEVATE_IF_ERROR( errorCode );

    RETURN_NO_ERROR();
}


/* ---------------------------------------------------------------------------------------------- */