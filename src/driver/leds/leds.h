#ifndef DRIVER_LEDS_H
#define DRIVER_LEDS_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f446xx.h"
#include "error.h"

typedef enum leds { LED_GREEN, LED_RED, LED_YELLOW } leds_t;
static volatile GPIO_TypeDef *const gpioc = (volatile GPIO_TypeDef *)GPIOC_BASE;
static volatile RCC_TypeDef *const rcc = (volatile RCC_TypeDef *)RCC_BASE;
bool initialized = false;

ERROR_CODE leds_init();
ERROR_CODE leds_clear();
ERROR_CODE leds_set(leds_t led, bool set);

#endif// DRIVER_LEDS_H
