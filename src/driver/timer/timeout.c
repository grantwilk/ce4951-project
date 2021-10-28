/* --------------------------------- Header --------------------------------- */


/**
 * @file    timeout.c
 * @brief   Contains functions for the idle/conflict timeout timer (TIM3)
 */


/* -------------------------------- Includes -------------------------------- */


# include <stdbool.h>
# include <stm32f446xx.h>


# include "state.h"
# include "timeout.h"
# include "uio.h"
# include "network.h"


/* --------------------------------- Defines -------------------------------- */


/**
 * The number of timeout timer ticks per second
 * (TIM3 is on APB1_TIMER)
 */
# define TIMEOUT_TIMER_TICKS_PER_SECOND ( 84000000L )


/**
 * The number of microseconds per second
 */
# define US_PER_SECOND  ( 1000000L )


/**
 * The number of timeout timer ticks per microsecond
 */
# define TIMEOUT_TIMER_TICKS_PER_US (   \
                TIMEOUT_TIMER_TICKS_PER_SECOND / US_PER_SECOND )


/* ---------------------------- Global Variables ---------------------------- */


/**
 * Timeout timer initialization flag
 */
static bool timeout_timer_is_init = false;


/**
 * Timeout timer running flag
 */
static bool timeout_timer_is_running = false;


/* ------------------------------- Functions -------------------------------- */


/**
 * Initializes the idle/conflict timeout timer
 *
 * @param   us  The timeout period in microseconds
 *
 * @return  Error code
 */
ERROR_CODE timeout_init( uint16_t us )
{
    // throw an error if the timeout timer is already initialized
    if ( timeout_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_ALREADY_INITIALIZED );
    }

    // set timeout timer init flag
    timeout_timer_is_init = true;

    // enable timeout timer (TIM3) in RCC
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // configure timeout timer
    TIM3->CR1 |= TIM_CR1_URS;
    TIM3->DIER |= TIM_DIER_UIE;
    TIM3->PSC = TIMEOUT_TIMER_TICKS_PER_US;

    // set timeout
    ELEVATE_IF_ERROR( timeout_set_timeout( us ) );

    // reset timer
    ELEVATE_IF_ERROR( timeout_reset() );

    // enable timeout timer interrupt in NVIC
    NVIC->ISER[0] |= ( 0b01 << 29U );

    // Enable the Capture Compare Register for Timer 3
    TIM3->CCMR1 &= ~(TIM_CCMR1_CC1S); // CC1 Channel configured as output
    TIM3->CCER |= TIM_CCER_CC1E; // Turn on OC1
    TIM3->DIER |= TIM_DIER_CC1IE;  // Enable CC Interrupt
    TIM3->CCR1 = 750U; // CCR value = 775

    RETURN_NO_ERROR();
}


/**
 * Starts the timeout timer
 *
 * @return Error code
 */
ERROR_CODE timeout_start()
{
    // throw an error if the timeout timer is not initialized
    if ( !timeout_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_NOT_INITIALIZED );
    }

    // throw an error if the timeout timer is already running
    if ( timeout_timer_is_running )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_ALREADY_RUNNING );
    }

    TIM3->CR1 |= TIM_CR1_CEN;
    timeout_timer_is_running = true;

    RETURN_NO_ERROR();
}


/**
 * Stops the timeout timer
 *
 * @return Error code
 */
ERROR_CODE timeout_stop()
{
    // throw an error if the timeout timer is not initialized
    if ( !timeout_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_NOT_INITIALIZED );
    }

    // throw an error if the timeout timer is not running
    if ( !timeout_timer_is_running )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_NOT_RUNNING );
    }

    TIM3->CR1 &= ~( TIM_CR1_CEN );
    timeout_timer_is_running = false;

    RETURN_NO_ERROR();
}


/**
 * Resets the idle/conflict timeout timer countdown
 *
 * @return  Error code
 */
ERROR_CODE timeout_reset()
{
    // throw an error if the timeout timer is not initialized
    if ( !timeout_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_NOT_INITIALIZED );
    }

    TIM3->CNT = 0;

    RETURN_NO_ERROR();
}

/**
 * Determines whether the timeout timer is running
 *
 * @return  True if the timeout timer is running, false otherwise
 */
bool timeout_is_running()
{
    // throw an error if the timeout timer is not initialized
    if ( !timeout_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_NOT_INITIALIZED );
    }

    return timeout_timer_is_running;
}


/**
 * Sets the idle/conflict timeout timer's timeout period
 *
 * @param   us  The timeout period in microseconds
 *
 * @return  Error code
 */
ERROR_CODE timeout_set_timeout( uint16_t us )
{
    // throw an error if the timeout timer is not initialized
    if ( !timeout_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_NOT_INITIALIZED );
    }

    TIM3->ARR = us;

    RETURN_NO_ERROR();
}


/* --------------------------- Interrupt Handlers --------------------------- */


/**
 * Timeout timer (TIM3) IRQ handler
 */
void TIM3_IRQHandler()
{


    if ( TIM3->SR & TIM_SR_UIF )
    {
        // clear update interrupt
        TIM3->SR &= ~( TIM_SR_UIF );

        // fetch GPIO PC12 (network input)
        bool network_input = GPIOC->IDR & GPIO_IDR_ID12;

        // if timeout occurs and the network input is 1, the line has idled
        // otherwise, the line has experienced a collision
        if ( network_input )
        {
            // uprintf("IDLE\n");
            ERROR_HANDLE_NON_FATAL( timeout_stop() );
            ERROR_HANDLE_NON_FATAL( state_set( IDLE ) );
            network_rx_queue_push(); // try to push the queue
        }
        else
        {
            // uprintf("COLLISION\n");
            ERROR_HANDLE_NON_FATAL( timeout_stop() );
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
