#ifndef DRIVER_NETWORK_H
#define DRIVER_NETWORK_H


#include <stddef.h>
#include <stdbool.h>
#include "error.h"


typedef struct
{
    uint8_t preamble;
    uint8_t version ;
    uint8_t source;
    uint8_t destination;
    uint8_t length;
    uint8_t crc_flag;
} frame_header_t;

typedef struct
{
    uint8_t crc8_fcs;
} frame_trailer_t;

typedef struct
{
    frame_header_t header;
    char * message;
    frame_trailer_t trailer;
} frame_t;


uint8_t get_local_machine_address();
ERROR_CODE set_local_machine_address(uint8_t newAddress);

ERROR_CODE network_init();
ERROR_CODE network_tx(uint8_t dest, uint8_t * buffer, size_t size);
bool network_rx(uint8_t * messageBuf, uint8_t * sourceAddr, uint8_t * destinationAddr);
ERROR_CODE network_start_tx();

bool network_tx_queue_is_full();
bool network_tx_queue_is_empty();
unsigned int network_tx_queue_count();
static bool network_tx_queue_push(uint8_t * buffer, size_t size);
static bool network_tx_queue_pop();

bool network_rx_queue_is_full();
bool network_rx_queue_is_empty();
unsigned int network_rx_queue_count();
void network_rx_queue_reset();
bool network_rx_queue_push_bit(bool bit);
bool network_rx_queue_get_last_bit();
bool network_rx_queue_push();
bool network_rx_queue_pop();

static unsigned int network_encode_manchester(uint8_t * manchester, uint8_t * buffer, size_t size);
static unsigned int network_encode_frame_manchester( uint8_t * manchester, frame_t * frame);
static ERROR_CODE network_decode_manchester(uint8_t * buffer, uint8_t * manchester, size_t size);
static ERROR_CODE network_decode_manchester_frame( frame_t * frame, uint8_t * manchester);
static ERROR_CODE network_decode_manchester_frame_message_trailer( frame_t * frame, uint8_t * manchester);
static ERROR_CODE network_decode_manchester_header( frame_header_t * header, uint8_t * manchester);

static uint8_t crc8_calculate(uint8_t * buffer, unsigned int size, uint8_t initialValue);
static bool frame_crc_isValid(frame_t * frame);
static void frame_crc_apply(frame_t * frame);



#endif // DRIVER_NETWORK_H
