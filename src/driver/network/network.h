#ifndef DRIVER_NETWORK_H
#define DRIVER_NETWORK_H

#include <stddef.h>
#include <stdbool.h>
#include "error.h"


ERROR_CODE network_init();

ERROR_CODE network_tx(char* msg, size_t size);

bool network_tx_isFull();

bool network_startTx();

#endif // DRIVER_NETWORK_H
