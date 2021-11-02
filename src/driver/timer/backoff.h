/* --------------------------------- Header --------------------------------- */



/**
 * @file    backoff.h
 * @brief   Contains functions for the backoff timer (TIM5)
 */


/* -------------------------------- Includes -------------------------------- */


# include <stdbool.h>
# include <stm32f446xx.h>


# include "state.h"
# include "backoff.h"
# include "uio.h"
# include "network.h"


/* --------------------------------- Defines -------------------------------- */


/**
 * The number of backoff timer ticks per second
 * (TIM3 is on APB1_TIMER)
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
 * @param   ms  The backoff period in milliseconds
 *
 * @return  Error code
 */
ERROR_CODE backoff_init( uint16_t ms )
{
    // throw an error if the backoff timer is already initialized
    if ( backoff_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_BACKOFF_ALREADY_INITIALIZED );
    }

    // set backoff timer init flag
    backoff_timer_is_init = true;

    // enable backoff timer (TIM3) in RCC
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // configure backoff timer
    TIM3->CR1 |= TIM_CR1_URS;
    TIM3->DIER |= TIM_DIER_UIE;
    TIM3->PSC = BACKOFF_TIMER_TICKS_PER_MS_TENTH;

    // set backoff
    ELEVATE_IF_ERROR( backoff_set_period( ms ) );

    // reset timer
    ELEVATE_IF_ERROR( backoff_reset() );

    // enable backoff timer interrupt in NVIC
    NVIC->ISER[0] |= ( 0b01 << 29U );

    // Enable the Capture Compare Register for Timer 3
    TIM3->CCMR1 &= ~(TIM_CCMR1_CC1S); // CC1 Channel configured as output
    TIM3->CCER |= TIM_CCER_CC1E; // Turn on OC1
    TIM3->DIER |= TIM_DIER_CC1IE;  // Enable CC Interrupt
    TIM3->CCR1 = 750U; // CCR value = 775

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

    TIM3->CR1 |= TIM_CR1_CEN;
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

    TIM3->CR1 &= ~( TIM_CR1_CEN );
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

    TIM3->CNT = 0;

    RETURN_NO_ERROR();
}


/**
 * Determines whether the backoff timer is running
 *
 * @return  True if the backoff timer is running, false otherwise
 */
bool backoff_is_running()
{
    // throw an error if the backoff timer is not initialized
    if ( !backoff_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_BACKOFF_NOT_INITIALIZED );
    }

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
    TIM3->ARR = ms * 10;

    RETURN_NO_ERROR();
}


/* --------------------------- Interrupt Handlers --------------------------- */


/**
 * Backoff timer (TIM3) IRQ handler
 */
void TIM3_IRQHandler()
{


    if ( TIM3->SR & TIM_SR_UIF )
    {
        // clear update interrupt
        TIM3->SR &= ~( TIM_SR_UIF );

        // fetch GPIO PC12 (network input)
        bool network_input = GPIOC->IDR & GPIO_IDR_ID12;

        // if backoff occurs and the network input is 1, the line has idled
        // otherwise, the line has experienced a collision
        if ( network_input )
        {
            // uprintf("IDLE\n");
            ERROR_HANDLE_NON_FATAL( backoff_stop() );
            ERROR_HANDLE_NON_FATAL( state_set( IDLE ) );
            network_rx_queue_push(); // try to push the queue
        }
        else
        {
            // uprintf("COLLISION\n");
            ERROR_HANDLE_NON_FATAL( backoff_stop() );
            ERROR_HANDLE_NON_FATAL( state_set( COLLISION ) );
            network_rx_queue_reset();
        }
    } else if ( TIM3->SR & TIM_SR_CC1IF )
    {
        // Clear the CC1IF Interrupt
        TIM3->SR &= ~( TIM_SR_CC1IF );

        // push last bit to the rx_queue
        bool last_bit = network_rx_queue_get_last_bit();
        network_rx_queue_push_bit(last_bit);
    }
}


/* -------------------------------------------------------------------------- */
