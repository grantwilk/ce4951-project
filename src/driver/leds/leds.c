#include "leds.h"

#define GPIO_MODE_OUTPUT    ( 0b01U )

bool leds_initialized = false;

ERROR_CODE leds_init()
{
    // throw an error if the LEDs are already initialized
    if (leds_initialized)
    {
        THROW_ERROR( ERROR_CODE_DRIVER_LEDS_ALREADY_INITIALIZED );
    }

    // SET UP PC8(red), PC6(yellow), PC5(green) in output pull up resistor mode
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // GPIO B enabled

    // configure GPIO MODER
    GPIOB->MODER &= ~(GPIO_MODER_MODER5);
    GPIOB->MODER &= ~(GPIO_MODER_MODER6);
    GPIOB->MODER &= ~(GPIO_MODER_MODER7);

    GPIOB->MODER |= (GPIO_MODE_OUTPUT << GPIO_MODER_MODER5_Pos);
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << GPIO_MODER_MODER6_Pos);
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << GPIO_MODER_MODER7_Pos);

    // set initialization flag
    leds_initialized = true;

    // clear the LEDs
    leds_clear();

    RETURN_NO_ERROR();
}

ERROR_CODE leds_clear()
{
    if ( !leds_initialized )
    {
        THROW_ERROR( ERROR_CODE_DRIVER_LEDS_NOT_INITIALIZED );
    }

    GPIOB->ODR &= ~(GPIO_ODR_OD5 | GPIO_ODR_OD6 | GPIO_ODR_OD7);

    RETURN_NO_ERROR();
}

ERROR_CODE leds_set(leds_t led, bool set)
{
    if (!leds_initialized)
    {
        THROW_ERROR( ERROR_CODE_DRIVER_LEDS_NOT_INITIALIZED );
    }

   if( set )
   {
       switch(led)
       {
           case LED_RED:
               GPIOB->ODR |= GPIO_ODR_OD5;
               break;
           case LED_YELLOW:
               GPIOB->ODR |= GPIO_ODR_OD6;
               break;
           case LED_GREEN:
               GPIOB->ODR |= GPIO_ODR_OD7;
               break;
       }
   }

   else
   {
       switch(led)
       {
           case LED_RED:
               GPIOB->ODR &= ~GPIO_ODR_OD5;
               break;
           case LED_YELLOW:
               GPIOB->ODR &= ~GPIO_ODR_OD6;
               break;
           case LED_GREEN:
               GPIOB->ODR &= ~GPIO_ODR_OD7;
               break;
       }
   }

    RETURN_NO_ERROR();
}
