#ifndef DRIVER_LEDS_H
#define DRIVER_LEDS_H

#include <stdbool.h>
#include "error.h"

typedef enum leds { LED_GREEN, LED_RED, LED_YELLOW } leds_t;

ERROR_CODE leds_init();
ERROR_CODE leds_clear();
ERROR_CODE leds_set(leds_t led, bool set);

#endif // DRIVER_LEDS_H
