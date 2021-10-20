#ifndef DRIVER_NETWORK_H
#define DRIVER_NETWORK_H

#include <stddef.h>
#include <stdbool.h>
#include "error.h"


ERROR_CODE network_init();

ERROR_CODE network_tx(uint8_t * buffer, size_t size);

ERROR_CODE network_start_tx();

bool network_msg_queue_is_full();
bool network_msg_queue_is_empty();
unsigned int network_msg_queue_count();

static void
network_encode_manchester(uint8_t * manchester, uint8_t * buffer, size_t size);

static bool network_msg_queue_push(uint8_t * buffer, size_t size);
static bool network_msg_queue_pop();

#endif // DRIVER_NETWORK_H
