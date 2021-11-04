/* --------------------------------- Header --------------------------------- */


/**
 * @file    backoff.h
 * @brief   Contains functions for the backoff timer (TIM5)
 */


/* -------------------------------- Includes -------------------------------- */


# include <stdbool.h>
# include <stm32f446xx.h>

# include "error.h"
# include "backoff.h"
# include "network.h"
# include "uio.h"


/* --------------------------------- Defines -------------------------------- */


/**
 * The number of backoff timer ticks per second
 * (TIM5 is on APB1_TIMER)
 */
# define BACKOFF_TIMER_TICKS_PER_SECOND ( 84000000L )


/**
 * The number of microseconds tenths (0.1ms) per second
 */
# define MS_TENTHS_PER_SECOND  ( 10000L )


/**
 * The number of backoff timer ticks per microsecond
 */
# define BACKOFF_TIMER_TICKS_PER_MS_TENTH (   \
                BACKOFF_TIMER_TICKS_PER_SECOND / MS_TENTHS_PER_SECOND )


/* ---------------------------- Global Variables ---------------------------- */


/**
 * Backoff timer initialization flag
 */
static bool backoff_timer_is_init = false;


/**
 * Backoff timer running flag
 */
static bool backoff_timer_is_running = false;


/* ------------------------------- Functions -------------------------------- */


/**
 * Initializes the backoff timer
 *
 * @return  Error code
 */
ERROR_CODE backoff_init()
{
    // throw an error if the backoff timer is already initialized
    if ( backoff_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_BACKOFF_ALREADY_INITIALIZED );
    }

    // set backoff timer init flag
    backoff_timer_is_init = true;

    // enable backoff timer (TIM5) in RCC
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;

    // configure backoff timer
    TIM5->CR1 |= TIM_CR1_URS;
    TIM5->DIER |= TIM_DIER_UIE;
    TIM5->PSC = BACKOFF_TIMER_TICKS_PER_MS_TENTH;

    // set backoff
    ELEVATE_IF_ERROR( backoff_set_period( 100 ) );

    // reset timer
    ELEVATE_IF_ERROR( backoff_reset() );

    // enable backoff timer interrupt in NVIC
    NVIC->ISER[1] |= ( 0b01 << (50U - 32U) );

    RETURN_NO_ERROR();
}


/**
 * Starts the backoff timer
 *
 * @return Error code
 */
ERROR_CODE backoff_start()
{
    // throw an error if the backoff timer is not initialized
    if ( !backoff_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_BACKOFF_NOT_INITIALIZED );
    }

    // throw an error if the backoff timer is already running
    if ( backoff_timer_is_running )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_BACKOFF_ALREADY_RUNNING );
    }

    TIM5->CR1 |= TIM_CR1_CEN;
    backoff_timer_is_running = true;

    RETURN_NO_ERROR();
}


/**
 * Stops the backoff timer
 *
 * @return Error code
 */
ERROR_CODE backoff_stop()
{
    // throw an error if the backoff timer is not initialized
    if ( !backoff_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_BACKOFF_NOT_INITIALIZED );
    }

    // throw an error if the backoff timer is not running
    if ( !backoff_timer_is_running )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_BACKOFF_NOT_RUNNING );
    }

    TIM5->CR1 &= ~( TIM_CR1_CEN );
    backoff_timer_is_running = false;

    RETURN_NO_ERROR();
}


/**
 * Resets the backoff timer countdown
 *
 * @return  Error code
 */
ERROR_CODE backoff_reset()
{
    // throw an error if the backoff timer is not initialized
    if ( !backoff_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_BACKOFF_NOT_INITIALIZED );
    }

    TIM5->CNT = 0;

    RETURN_NO_ERROR();
}


/**
 * Determines whether the backoff timer is running
 *
 * @return  True if the backoff timer is running, false otherwise
 */
bool backoff_is_running()
{
    return backoff_timer_is_running;
}


/**
 * Sets the backoff timer's backoff period
 *
 * @param   ms  The backoff period in milliseconds
 *
 * @return  Error code
 */
ERROR_CODE backoff_set_period( uint16_t ms )
{
    // throw an error if the backoff timer is not initialized
    if ( !backoff_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_BACKOFF_NOT_INITIALIZED );
    }

    // multiply by 10 because PSC sets the timer to run in millisecond tenths
    TIM5->ARR = ms * 10;

    RETURN_NO_ERROR();
}


/* --------------------------- Interrupt Handlers --------------------------- */


/**
 * Backoff timer (TIM5) IRQ handler
 */
void TIM5_IRQHandler()
{
    if (TIM5->SR & TIM_SR_UIF)
    {
        backoff_stop();
        TIM5->SR &= ~(TIM_SR_UIF);
        ERROR_HANDLE_NON_FATAL(network_start_tx());
    }
}


/* -------------------------------------------------------------------------- */

