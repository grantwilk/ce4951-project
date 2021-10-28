/* ------------------------------------------ Header -------------------------------------------- */


/**
 * @file    error.h
 * @brief   Defines error codes and functions for handling errors
 */


/* ---------------------------------------- Header Guard ---------------------------------------- */


# ifndef UTIL_ERROR_H
# define UTIL_ERROR_H


/* ---------------------------------------- Enumerations ---------------------------------------- */


/**
 * @brief   Error codes
 */
typedef enum
{
    ERROR_CODE_NO_ERROR,                                        // 0x00
    ERROR_CODE_UNKNOWN_ERROR,                                   // 0x01
    ERROR_CODE_MEMORY_ERROR,                                    // 0x02

    ERROR_CODE_HAL_ERROR,                                       // 0x03

    ERROR_CODE_DRIVER_SERIAL_UART_NOT_INITIALIZED,              // 0x04
    ERROR_CODE_DRIVER_SERIAL_UART_ALREADY_INITIALIZED,          // 0x05
    ERROR_CODE_DRIVER_SERIAL_UART_TIMEOUT,                      // 0x06

    ERROR_CODE_UTIL_UIO_NOT_INITIALIZED,                        // 0x07
    ERROR_CODE_UTIL_UIO_ALREADY_INITIALIZED,                    // 0x08

    ERROR_CODE_DRIVER_TIMER_TIMEOUT_NOT_INITIALIZED,            // 0x09
    ERROR_CODE_DRIVER_TIMER_TIMEOUT_ALREADY_INITIALIZED,        // 0x0A
    ERROR_CODE_DRIVER_TIMER_TIMEOUT_NOT_RUNNING,                // 0x0B
    ERROR_CODE_DRIVER_TIMER_TIMEOUT_ALREADY_RUNNING,            // 0x0C

    ERROR_CODE_DRIVER_LEDS_NOT_INITIALIZED,                     // 0x0D
    ERROR_CODE_DRIVER_LEDS_ALREADY_INITIALIZED,                 // 0x0E

    ERROR_CODE_SET_UNKNOWN_STATE,                               // 0x0F

    ERROR_CODE_NETWORK_NOT_INITIALIZED,                         // 0x10
    ERROR_CODE_NETWORK_ALREADY_INITIALIZED,                     // 0x11
    ERROR_CODE_NETWORK_MSG_QUEUE_FULL,                          // 0x12
    ERROR_CODE_NETWORK_MSG_POP_FAILURE,                         // 0x13
    
    ERROR_CODE_DRIVER_TIMER_HB_NOT_INITIALIZED,                 // 0x14
    ERROR_CODE_DRIVER_TIMER_HB_ALREADY_INITIALIZED,             // 0x15

    ERROR_CODE_INVALID_MANCHESTER_RECEIVED,                      // 0x16
    ERROR_CODE_MALFORMED_MESSAGE_RECEIVED,                       //0x17
    ERROR_CODE_WRONG_MESSAGE_VERSION_RECEIVED

} ERROR_CODE;


/* ------------------------------------------- Macros ------------------------------------------- */


/**
 * @brief   Returns a "no error" error code
 */
# define RETURN_NO_ERROR()                                              \
return ERROR_CODE_NO_ERROR


/**
 * Stops program flow and returns a user-specified error code
 */
# define THROW_WARNING( MSG )                                           \
uprintf("\n");                                                          \
uprintf("WARNING!    %s\n", MSG);                                       \
uprintf("\n");


/**
 * @brief   Stops program flow and returns a user-specified error code
 */
# define THROW_ERROR( ERROR_CODE )                                      \
return ERROR_CODE


/**
 * @brief   Elevates an error if one has occurred
 */
# define ELEVATE_IF_ERROR( ERROR_CODE )                                 \
if ( ( ERROR_CODE ) != ERROR_CODE_NO_ERROR )                            \
{                                                                       \
    return ERROR_CODE;                                                  \
}


/**
 * Handles an error by printing an error message
 */
# define ERROR_HANDLE_NON_FATAL( ERROR_CODE )                           \
if ( ( ERROR_CODE ) != ERROR_CODE_NO_ERROR )                            \
{                                                                       \
    uprintf("\n");                                                      \
    uprintf("ERROR!      A non-fatal error has occurred!\n");           \
    uprintf("            Error Code: 0x%08X\n", ERROR_CODE);            \
    uprintf("\n");                                                      \
}


/**
 * Handles a fatal error by printing an error message and halting
 */
# define ERROR_HANDLE_FATAL( ERROR_CODE )                               \
if ( ( ERROR_CODE ) != ERROR_CODE_NO_ERROR )                            \
{                                                                       \
    uprintf("\n");                                                      \
    uprintf("ERROR!      A fatal error has occurred!\n");               \
    uprintf("            Error Code: 0x%08X\n", ERROR_CODE);            \
    uprintf("\n");                                                      \
    uprintf("SYSTEM HALTED.");                                          \
    while (1);                                                          \
}


/**
 * @brief   Handles a fault by printing a warning and halting the program
 */
# define ERROR_HANDLE_FAULT( ERROR_TYPE )                               \
uprintf("\n");                                                          \
uprintf("ERROR!     A fatal fault has occurred!\n");                    \
uprintf("           Fault Type: %s\n", ERROR_TYPE);                     \
uprintf("\n");                                                          \
uprintf("SYSTEM HALTED.");                                              \
while (1);

/* ------------------------------------------- Footer ------------------------------------------- */


# endif // UTIL_ERROR_H


/* ---------------------------------------------------------------------------------------------- */
