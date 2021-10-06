#include "leds.h"

ERROR_CODE led_init()
{
    // SET UP PC8(red), PC6(yellow), PC5(green) in output pull up resistor mode
    rcc->AHB2RSTR |= RCC_APB2ENR_SYSCFGEN; // System config enabled
    rcc->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; // GPIO C enabled

    gpioc->MODER |= GPIO_MODER_MODER8; // GPIO c8 Output
    gpioc->MODER |= GPIO_MODER_MODER6; // GPIO c6 Output
    gpioc->MODER |= GPIO_MODER_MODER5; // GPIO c5 Output

    gpioc->PUPDR &= ~GPIO_PUPDR_PUPD8; // GPIO c8 pull up
    gpioc->PUPDR |= 0b01 << GPIO_PUPDR_PUPD8_Pos;
    gpioc->PUPDR &= ~GPIO_PUPDR_PUPD6; // GPIO c6 pull up
    gpioc->PUPDR |= 0b01 << GPIO_PUPDR_PUPD6_Pos;
    gpioc->PUPDR &= ~GPIO_PUPDR_PUPD5; // GPIO c5 pull up
    gpioc->PUPDR |= 0b01 << GPIO_PUPDR_PUPD5_Pos;

    initialized = true;
    leds_clear(); // set all to off at the start

    RETURN_NO_ERROR();
}

ERROR_CODE leds_clear()
{
    if(initialized) {
        //pull up resistor leads to logic 1 being off
        gpioc->ODR |= (GPIO_ODR_OD8 | GPIO_ODR_OD6 | GPIO_ODR_OD5); // set all to off
    } else {
        //if not initialized ERROR
        THROW_ERROR(0x07);
    }
    RETURN_NO_ERROR();
}

ERROR_CODE led_set(leds_t led, bool set)
{
   if(initialized) {
       if(set) {
           switch(led) {
               //pull up resistor leads to logic 0 being ON
               case LED_RED : gpioc->ODR &= ~GPIO_ODR_OD8;
                   break;
               case LED_YELLOW : gpioc->ODR &= ~GPIO_ODR_OD8;
                   break;
               case LED_GREEN : gpioc->ODR &= ~GPIO_ODR_OD8;
                   break;
               default : THROW_ERROR(0x01);
           }
       }else {
           switch(led) {
               //pull up resistor leads to logic 1 being off
               case LED_RED : gpioc->ODR |= GPIO_ODR_OD8;
                   break;
               case LED_YELLOW : gpioc->ODR |= GPIO_ODR_OD8;
                   break;
               case LED_GREEN : gpioc->ODR |= GPIO_ODR_OD8;
                   break;
               default : THROW_ERROR(0x01);
           }
       }
   } else {
       //if not initialized ERROR
       THROW_ERROR(0x07);
   }
    RETURN_NO_ERROR();
}
