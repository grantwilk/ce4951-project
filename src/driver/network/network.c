#include <stdint.h>
#include <stdbool.h>
#include "stm32f446xx.h"

#include "network.h"
#include "state.h"
#include "timeout.h"

#define EXTI_15_10_NVIC 8

static volatile EXTI_TypeDef * const exti = (volatile EXTI_TypeDef *) EXTI_BASE;
static volatile GPIO_TypeDef * const gpioc = (volatile GPIO_TypeDef *) GPIOC_BASE;

ERROR_CODE network_init()
{
	static volatile RCC_TypeDef * const rcc = (volatile RCC_TypeDef *) RCC_BASE;
	static volatile NVIC_Type * const nvic = (volatile NVIC_Type *) NVIC_BASE;
	static volatile SYSCFG_TypeDef * const syscfg = (volatile SYSCFG_TypeDef *) SYSCFG_BASE;

	rcc->APB2ENR |= RCC_APB2ENR_SYSCFGEN; //SYSCFGEN
	rcc->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; //GPIOCEN

	gpioc->MODER &= ~GPIO_MODER_MODER12; //set PC12 as input

	gpioc->PUPDR &= ~GPIO_PUPDR_PUPD12;
	gpioc->PUPDR |= 0b01<<GPIO_PUPDR_PUPD12_Pos; //set pullup resistor

	syscfg->EXTICR[3]  &= ~SYSCFG_EXTICR4_EXTI12;
	syscfg->EXTICR[3]  |= SYSCFG_EXTICR4_EXTI12 & SYSCFG_EXTICR4_EXTI12_PC;  //EXTI line 12 set to bank C

	nvic->ISER[1] |= 1<<EXTI_15_10_NVIC; //Set enable in NVIC, NVIC POS 40

	exti->IMR |= EXTI_IMR_IM12; //Unmask in EXTI
	exti->FTSR |= EXTI_FTSR_TR12; //Set falling trigger
	exti->RTSR |= EXTI_RTSR_TR12; //set rising trigger

	RETURN_NO_ERROR();
}

void EXTI15_10_IRQHandler()
{
	__asm__("CPSID i"); //disable interrupts
	if (exti->PR & EXTI_PR_PR12) 
	{
		bool isHigh = gpioc->IDR >> GPIO_IDR_ID12_Pos;
		timeout_reset();

		exti->PR = EXTI_PR_PR12; //clear pending interrupt
		
		if (!isHigh) {
			state_set(BUSY);
		}

	}
	__asm__("CPSIE i"); //enable interrupts
}
