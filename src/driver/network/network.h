#ifndef DRIVER_NETWORK_H
#define DRIVER_NETWORK_H

#include <stddef.h>
#include <stdbool.h>
#include "error.h"


ERROR_CODE network_init();

ERROR_CODE network_tx(void * buffer, size_t size);

bool network_msg_queue_is_full();
bool network_msg_queue_is_empty();

static ERROR_CODE network_start_tx();

static bool network_msg_queue_push(void * buffer, size_t size);

#endif // DRIVER_NETWORK_H
