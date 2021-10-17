#include "hb_timer.h"
#include <stdbool.h>
#include "stm32f446xx.h"


/**
 * The number of hb timer ticks per second
 * (TIM4 is on APB1_TIMER)
 */
# define HB_TIMER_TICKS_PER_SECOND ( 84000000L )


/**
 * The number of microseconds per second
 */
# define US_PER_SECOND  ( 1000000L )


/**
 * The number of hb timer ticks per microsecond
 */
# define HB_TIMER_TICKS_PER_US (   \
                HB_TIMER_TICKS_PER_SECOND / US_PER_SECOND )


/**
 * Timeout timer initialization flag
 */
static bool hb_timer_is_init = false;


/**
 * Timeout timer running flag
 */
static bool hb_timer_is_running = false;


ERROR_CODE hb_timer_init(uint16_t us)
{
    // throw an error if the hb timer is already initialized
    if ( hb_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_ALREADY_INITIALIZED );
    }

    // set hb timer init flag
    hb_timer_is_init = true;

    // enable hb timer (TIM4) in RCC
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

    // configure hb timer
    TIM4->CR1 |= TIM_CR1_URS;
    TIM4->DIER |= TIM_DIER_UIE;
    TIM4->PSC = HB_TIMER_TICKS_PER_US;

    // set hb
    ELEVATE_IF_ERROR( hb_timer_set_timeout( us ) );

    // enable hb timer interrupt in NVIC
    NVIC->ISER[0] |= ( 0b01 << 30U );

    RETURN_NO_ERROR();
}

ERROR_CODE hb_timer_reset_and_start()
{
    // throw an error if the hb timer is not initialized
    if ( !hb_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_NOT_INITIALIZED );
    }

    TIM4->CNT = 0;

    if (!hb_timer_is_running)
    {
      TIM4->CR1 |= TIM_CR1_CEN;
      hb_timer_is_running = true;
    }

    RETURN_NO_ERROR();
}

ERROR_CODE hb_timer_stop()
{
    // throw an error if the hb timer is not initialized
    if ( !hb_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_NOT_INITIALIZED );
    }

    TIM4->CR1 &= ~( TIM_CR1_CEN );
    hb_timer_is_running = false;

    RETURN_NO_ERROR();
}

ERROR_CODE hb_timer_set_timeout(uint16_t us)
{
    // throw an error if the hb timer is not initialized
    if ( !hb_timer_is_init )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_TIMER_TIMEOUT_NOT_INITIALIZED );
    }

    TIM4->ARR = us;

    RETURN_NO_ERROR();
}
