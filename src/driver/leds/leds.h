#ifndef LEDS_H
#define LEDS_H

#include <stdbool.h>

enum LEDS {LED_GREEN, LED_RED, LED_YELLOW};

void led_init();
void led_clear();
void led_set(enum LEDS led, bool set);

#endif
