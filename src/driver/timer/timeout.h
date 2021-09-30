#ifndef TIMEOUT_H
#define TIMEOUT_H

#include "error.h"

ERROR_CODE timeout_init(long us);

ERROR_CODE timeout_set_timeout(long us);

ERROR_CODE timeout_reset();

#endif
