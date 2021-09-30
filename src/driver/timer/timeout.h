#ifndef DRIVER_TIMEOUT_H
#define DRIVER_TIMEOUT_H

#include "error.h"

ERROR_CODE timeout_init(long us);

ERROR_CODE timeout_set_timeout(long us);

ERROR_CODE timeout_reset();

#endif // DRIVER_TIMEOUT_H
