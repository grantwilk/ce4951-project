/* ------------------------------------------ Header -------------------------------------------- */


/**
 * @file    uart.c
 * @brief   Contains functions for transmitting and receiving data via UART
 */


/* ------------------------------------------ Includes ------------------------------------------ */


# include <stdbool.h>
# include <stm32f446xx.h>
# include "error.h"
# include "uart.h"
# include "uio.h"
# include "circular_queue.h"


/* ------------------------------------------ Defines ------------------------------------------- */


/**
 * @brief   The USART module clock's number of ticks per second (USART2 is on APB1_PERIPHERAL)
 */
# define USART_CLOCK_TICKS_PER_SECOND           ( 42000000U )


/**
 * @brief   The timeout timer clock's number of ticks per second (TIM7 is on APB1_TIMER)
 */
# define TIMEOUT_TIMER_CLOCK_TICKS_PER_SECOND   ( 84000000U )


/**
 * @brief   The desired number of ticks per second in the timeout timer
 */
# define TIMEOUT_TIMER_TICKS_PER_SECOND         ( 1000000U )


/* ----------------------------------- Static Global Variables ---------------------------------- */


/**
 * @brief   UART initialization flag
 */
static bool uartIsInit = false;

static circular_queue input_buffer;


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
)
{
	// throw an error if the UART is already initialized
	if ( uartIsInit )
    {
	    THROW_ERROR( ERROR_CODE_DRIVER_SERIAL_UART_ALREADY_INITIALIZED );
    }

    // circular queue
    input_buffer = cq_init();

    // enable USART module (USART2), timeout timer (TIM7), and GPIO port (GPIOA) in RCC
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;

    // configure GPIO pins (GPIOA)
    GPIOA->PUPDR |= ( 0b01 << GPIO_PUPDR_PUPD2_Pos ) |
                    ( 0b01 << GPIO_PUPDR_PUPD3_Pos );

    GPIOA->MODER &= ~( GPIO_MODER_MODER2 | GPIO_MODER_MODER3 );
    GPIOA->MODER |=  ( 0b10 << GPIO_MODER_MODER2_Pos ) |
                     ( 0b10 << GPIO_MODER_MODER3_Pos );

    GPIOA->AFR[0] &= ~( GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3 );
    GPIOA->AFR[0] |= ( 0x07U << GPIO_AFRL_AFSEL2_Pos ) |
                     ( 0x07U << GPIO_AFRL_AFSEL3_Pos );

    // configure timeout timer (TIM7)
    TIM7->CR1 |= TIM_CR1_URS;
    TIM7->SR  &= ~( TIM_SR_UIF );
    TIM7->PSC = TIMEOUT_TIMER_CLOCK_TICKS_PER_SECOND / TIMEOUT_TIMER_TICKS_PER_SECOND;

    // configure USART module (USART2)
    uint32_t mantissa = USART_CLOCK_TICKS_PER_SECOND / ( 16 * baudRate );
    USART2->BRR = ( mantissa << USART_BRR_DIV_Mantissa_Pos );
    USART2->SR = 0;
    USART2->CR1 |= USART_CR1_RE;
    USART2->CR1 |= USART_CR1_TE;
    USART2->CR1 |= USART_CR1_UE;

    // enable interrupts
    USART2->CR1 |= USART_CR1_RXNEIE;
    NVIC->ISER[1] |= (1 << 6);

    // set buffer source
    setvbuf(stdout, NULL, _IONBF, 0);

    // set initialization flag
    uartIsInit = true;

	RETURN_NO_ERROR();
}


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
)
{
	ERROR_CODE errorCode;

    // throw an error if the UART is not initialized
    if ( !uartIsInit )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_SERIAL_UART_NOT_INITIALIZED );
    }

    // transmit buffer
	uint32_t txByteIndex = 0;
	while ( txByteIndex < txBufferSize )
	{
		errorCode = uartTxByte( txBuffer[txByteIndex++], timeout );
		ELEVATE_IF_ERROR( errorCode );
	}

	RETURN_NO_ERROR();
}


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
)
{
	ERROR_CODE errorCode;

    // throw an error if the UART is not initialized
    if ( !uartIsInit )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_SERIAL_UART_NOT_INITIALIZED );
    }

    // receive buffer
    uint32_t rxByteIndex = 0;
    while ( rxByteIndex < rxBufferSize )
    {
        errorCode = uartRxByte( &rxBuffer[rxByteIndex++], timeout );
        ELEVATE_IF_ERROR( errorCode );
    }

	RETURN_NO_ERROR();
}


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
)
{
    // start timeout timer
    TIM7->CNT = 0;
    TIM7->ARR = timeout;
    TIM7->SR  = ~( TIM_SR_UIF );
    if ( timeout != DRIVER_SERIAL_UART_NO_TIMEOUT )
    {
        TIM7->CR1 |= TIM_CR1_CEN;
    }

    // wait for transmit data register empty or timeout
    while ( !( USART2->SR & USART_SR_TXE ) && !( TIM7->SR & TIM_SR_UIF ) );

    // stop the timeout timer and check for timeout
    TIM7->CR1 &= ~( TIM_CR1_CEN );
    if ( TIM7->SR & TIM_SR_UIF )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_SERIAL_UART_TIMEOUT );
    }

    // transmit the byte
    USART2->DR = txByte;

    RETURN_NO_ERROR();
}


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
)
{
    // start timeout timer
    TIM7->CNT = 0;
    TIM7->ARR = timeout;
    TIM7->SR  = ~( TIM_SR_UIF );
    if ( timeout != DRIVER_SERIAL_UART_NO_TIMEOUT )
    {
        TIM7->CR1 |= TIM_CR1_CEN;
    }

    // wait for receive data register not empty or timeout
    while ( !( USART2->SR & USART_SR_RXNE ) && !( TIM7->SR & TIM_SR_UIF ) );

    // stop the timeout timer and check for timeout
    TIM7->CR1 &= ~( TIM_CR1_CEN );
    if ( TIM7->SR & TIM_SR_UIF )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_SERIAL_UART_TIMEOUT );
    }

    // receive the byte
    *rxByte = USART2->DR;

    RETURN_NO_ERROR();
}

/* ------------------------------------- Override Functions ------------------------------------- */


/**
 * Reads a string from the UART's input buffer
 * @param file - not implemented (ignored)
 * @param ptr - where the read data should be put
 * @param len - the number of characters to read
 * @return the number of characters read
 */
int _read(int file, char * ptr, int len) {

    // wait until the input buffer receives some data
    while (cq_isempty(&input_buffer));

    int char_count = 0;

    // pull from the circular queue until it is empty
    while (!cq_isempty(&input_buffer)) {
        char_count++;
        *ptr = cq_pull(&input_buffer);
        ptr++;
    }

    if (*ptr == '\r') *ptr = '\n';

    return char_count;

}


/* ---------------------------------------- IRQ Handlers ---------------------------------------- */


/**
 * USART2 interrupt request handler
 */
void USART2_IRQHandler(void) {
    if ((USART2->SR & USART_SR_RXNE) && !cq_isfull(&input_buffer)) {
        // read the RDR
        char c = USART2->DR;

        // push the char in the RDR into the input buffer
        cq_push(&input_buffer, c);

        // echo the character to the output buffer
        ERROR_HANDLE_NON_FATAL(uartTxByte(c, 1000));
    }
}


/* ---------------------------------------------------------------------------------------------- */