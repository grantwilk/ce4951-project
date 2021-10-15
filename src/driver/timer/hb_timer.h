
#ifndef DRIVER_HB_TIMER_H
#define DRIVER_HB_TIMER_H

#include <stdint.h>
#include "error.h"


ERROR_CODE hb_timer_init(uint16_t us);

ERROR_CODE hb_timer_reset_and_start();

ERROR_CODE hb_timer_stop();

ERROR_CODE hb_timer_set_timeout(uint16_t us);

#endif //DRIVER_HB_TIMER_H
