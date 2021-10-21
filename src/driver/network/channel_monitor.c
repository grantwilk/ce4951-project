#include <stdint.h>
#include <stdbool.h>
#include "stm32f446xx.h"

#include "channel_monitor.h"
#include "state.h"
#include "timeout.h"

#define EXTI_15_10_NVIC 8

ERROR_CODE channel_monitor_init()
{

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; //SYSCFGEN
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;  //GPIOCEN

    GPIOC->MODER &= ~GPIO_MODER_MODER12; //set PC12 as input

    GPIOC->PUPDR &= ~GPIO_PUPDR_PUPD12;
    GPIOC->PUPDR |= 0b01 << GPIO_PUPDR_PUPD12_Pos; //set pullup resistor

    SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI12;
    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI12 & SYSCFG_EXTICR4_EXTI12_PC; //EXTI line 12 set to bank C

    NVIC->ISER[1] |= 1 << EXTI_15_10_NVIC; //Set enable in NVIC, NVIC POS 40

    EXTI->IMR |= EXTI_IMR_IM12;   //Unmask in EXTI
    EXTI->FTSR |= EXTI_FTSR_TR12; //Set falling trigger
    EXTI->RTSR |= EXTI_RTSR_TR12; //set rising trigger

    RETURN_NO_ERROR();
}

//Handles channel monitor input interrupts
void EXTI15_10_IRQHandler()
{
    if (EXTI->PR & EXTI_PR_PR12)
    {
        bool isHigh = GPIOC->IDR & GPIO_IDR_ID12;

        timeout_reset();

        if (!timeout_is_running())
        {
            timeout_start();
        }

        EXTI->PR = EXTI_PR_PR12; //clear pending interrupt

        if (!isHigh)
        {
            state_set(BUSY);
        }
    }
}
