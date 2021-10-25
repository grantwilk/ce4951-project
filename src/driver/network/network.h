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



typedef struct
{
    uint8_t preamble;
    uint8_t version ;
    uint8_t source;
    uint8_t destination;
    uint8_t length;
    uint8_t crc_flag;
} msg_header_t;

typedef struct
{
    uint8_t crc8_fcs;
} msg_trailer_t;

/**
 *
 */
typedef struct
{
    msg_header_t header;
    char * message;
    msg_trailer_t trailer;
} msg_frame_t;


static unsigned int network_encode_manchester(uint8_t * manchester, uint8_t * buffer, size_t size);
static unsigned int network_encode_frame_manchester(uint8_t * manchester, msg_frame_t * frame);
static ERROR_CODE network_decode_manchester(uint8_t * buffer, uint8_t * manchester, size_t size);
static ERROR_CODE network_decode_manchester_frame(msg_frame_t * frame, uint8_t * manchester);
static ERROR_CODE network_decode_manchester_frame_message_trailer(msg_frame_t * frame, uint8_t * manchester);
static ERROR_CODE network_decode_manchester_header(msg_header_t * header, uint8_t * manchester);

static bool network_msg_queue_push(uint8_t * buffer, size_t size);
static bool network_msg_queue_pop();




#endif // DRIVER_NETWORK_H
