#include <stdbool.h>
#include <uio.h>
#include <memory.h>
#include "stm32f446xx.h"

#include "network.h"
#include "state.h"
#include "hb_timer.h"


#define MIN(a,b) (((a)<(b))?(a):(b))


#define HALF_BIT_PERIOD_US              (500)

#define LOCAL_MACHINE_ADDRESS           (0x23) //TODO changeme if needed
#define HEADER_PREAMBLE                 (0x55)
#define PROTOCOL_VERSION                (0x01)

#define MAX_MESSAGE_SIZE                (255)
#define MAX_FRAME_SIZE                  (MAX_MESSAGE_SIZE + sizeof(frame_header_t) + sizeof(frame_trailer_t))
#define MAX_FRAME_SIZE_MANCHESTER       (MAX_FRAME_SIZE * 2)

#define TX_QUEUE_SIZE                   (10)
#define RX_QUEUE_SIZE                   (10)

// #define NETWORK_TX_DBG
// #define MANCHESTER_DBG

/**
 * Initialization flag
 */
static bool network_is_init = false;


/**
 * Node structure for the network's circular queues
 */
typedef struct
{
    char buffer[MAX_FRAME_SIZE_MANCHESTER];
    size_t size;
} queue_node_t;


/**
 * Transmit queue variables
 *
 * NOTE:
 * Push index references the index of the location where the next element will be pushed
 * onto the queue
 *
 * NOTE:
 * Pop index references the index of the most recently popped element of the queue
 */
static queue_node_t tx_queue[TX_QUEUE_SIZE];
static int tx_queue_push_idx = 1;
static int tx_queue_pop_idx = 0;


/**
 * Receive queue variables
 *
 * NOTE:
 * Push index references the index of the location where the next element will be pushed
 * onto the queue
 *
 * NOTE:
 * Pop index references the index of the most recently popped element of the queue
 */
static queue_node_t rx_queue[RX_QUEUE_SIZE];
static int rx_queue_push_idx = 1;
static int rx_queue_pop_idx = 0;


/**
 * Initializes the network component
 *
 * @return Error code
 */
ERROR_CODE network_init()
{
    // throw an error if the network is already initialized
    if (network_is_init)
    {
        THROW_ERROR(ERROR_CODE_NETWORK_ALREADY_INITIALIZED);
    }

    ELEVATE_IF_ERROR(hb_timer_init( HALF_BIT_PERIOD_US));
  
    // Initialize PC11 as an Output
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; //SYSCFGEN
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;  //GPIOCEN

    // set GPIOC high on start
    GPIOC->ODR |= GPIO_ODR_OD11;

    GPIOC->MODER &= ~GPIO_MODER_MODER11;
    GPIOC->MODER |= 0b01 << GPIO_MODER_MODER11_Pos; //Set PC11 as output

    network_is_init = true;

    RETURN_NO_ERROR();
}

//handy function to print bytes for debugging
static void printBytesHex(char * name, uint8_t * bytes, size_t size)
{
    uprintf("%s:", name);
    for (int i = 0; i < size; i++)
    {
        if (i % 64 == 0)
        {
            uprintf("\n");
        }
        else if (i % 2 == 0)
        {
            uprintf(" ");
        }
        uprintf("%02X", bytes[i]);
    }
    uprintf("\n\n");
}


/**
 * Queues a frame in the transmit queue and attempts to begin transmission.
 *
 * @param   [in]    dest    The destination address of the message
 * @param   [in]    buffer  The buffer to transmit
 * @param   [in]    size    The size of the buffer in bytes
 *
 * @return  Error code
 */
ERROR_CODE network_tx(uint8_t dest, uint8_t * buffer, size_t size)
{
    // throw an error if the network is not initialized
    if (!network_is_init)
    {
        THROW_ERROR(ERROR_CODE_NETWORK_NOT_INITIALIZED);
    }

    frame_t frame = {
        .header = {
            .preamble = HEADER_PREAMBLE,
            .version = PROTOCOL_VERSION,
            .source = LOCAL_MACHINE_ADDRESS,
            .destination = dest,
            .length = 0x0,
            .crc_flag = 0x01
        },
        .message = (char *) buffer,
        .trailer = {
            .crc8_fcs = 0x00
        }
    };

    unsigned int queued_bytes = 0;
    unsigned char manchester[MAX_FRAME_SIZE_MANCHESTER];

    // break buffer into chunks and queue
    while (size - queued_bytes)
    {
        //TODO set trailer with big brain CRC algorithm
        frame.trailer.crc8_fcs = 0x00;
        frame.header.length = MIN(MAX_MESSAGE_SIZE, size - queued_bytes);

        frame.message = (char *) buffer + queued_bytes;

        // encode in manchester
        unsigned int manchester_size = network_encode_frame_manchester(manchester, &frame);

        #ifdef NETWORK_TX_DBG
            printBytesHex("ORIGINAL HEADER", (uint8_t *) &frame.header, sizeof(msg_header_t));
            printBytesHex("ORIGINAL MESSAGE", frame.message, frame.header.length);
            printBytesHex("ORIGINAL TRAILER", (uint8_t *) &frame.trailer, sizeof(msg_trailer_t));
            printBytesHex("ENTIRE MANCHESTER FRAME ENCODED", manchester, manchesterSize);
            msg_frame_t retFrame;
            char buf[256];
            retFrame.message = buf;
            network_decode_manchester_frame(&retFrame, manchester);
            printBytesHex("DECODED HEADER", (uint8_t *) &retFrame.header, sizeof(msg_header_t));
            printBytesHex("DECODED MESSAGE", frame.message, retFrame.header.length);
            printBytesHex("DECODED TRAILER", (uint8_t *) &retFrame.trailer, sizeof(msg_trailer_t));
        #endif

        if (!network_tx_queue_push( manchester, manchester_size ))
        {
            THROW_ERROR(ERROR_CODE_NETWORK_MSG_QUEUE_FULL);
        }
        queued_bytes += frame.header.length;
    }

    // attempt to start a transmission
    ELEVATE_IF_ERROR(network_start_tx());

    RETURN_NO_ERROR();
}


/**
 * Signals the network component to begin transmitting messages from its
 * internal message queue
 *
 * @return  Error code
 */
ERROR_CODE network_start_tx()
{
    // throw an error if the network is not initialized
    if (!network_is_init)
    {
        THROW_ERROR(ERROR_CODE_NETWORK_NOT_INITIALIZED);
    }

    if ( !network_tx_queue_is_empty() && ( state_get() == IDLE))
    {
        ELEVATE_IF_ERROR(hb_timer_reset_and_start());
    }

    RETURN_NO_ERROR();
}


/**
 * Determines whether the network's transmit queue is full
 *
 * @return  True if the queue is full, false otherwise.
 */
bool network_tx_queue_is_full()
{
    return tx_queue_pop_idx == tx_queue_push_idx;
}


/**
 * Determines whether the network's transmit queue is full
 *
 * @return  True if the queue is full, false otherwise.
 */
bool network_tx_queue_is_empty()
{
    return ( tx_queue_pop_idx + 1) % TX_QUEUE_SIZE == tx_queue_push_idx;
}


/**
 * Gets the number of messages in the queue
 *
 * @return  The number of messages in the message queue
 */
unsigned int network_tx_queue_count()
{
    if ( tx_queue_push_idx > tx_queue_pop_idx)
    {
        return tx_queue_push_idx - tx_queue_pop_idx - 1;
    }
    else
    {
        return ( tx_queue_push_idx + TX_QUEUE_SIZE) - tx_queue_pop_idx - 1;
    }
}

/**
 * Pushes an element into this network's transmit queue
 *
 * @param   [in]    buffer  The buffer to enqueue
 * @param   [in]    size    The size of the buffer
 *
 * @return  False if the transmit queue is full, true otherwise
 */
static bool network_tx_queue_push(uint8_t * buffer, size_t size)
{
    // return false if the queue is full
    if ( network_tx_queue_is_full())
    {
        return false;
    }

    // it is important that we make a copy of the buffer in the queue,
    // otherwise we risk modifying the data before it can be transmitted
    memcpy( tx_queue[tx_queue_push_idx].buffer, buffer, size);
    tx_queue[tx_queue_push_idx].size = size;

    tx_queue_push_idx = ( tx_queue_push_idx + 1) % TX_QUEUE_SIZE;

    return true;
}


/**
 * Pops an element from this network's transmit queue
 *
 * @return True if an element is successfully popped, false otherwise
 */
static bool network_tx_queue_pop()
{
    // return false if the queue is empty (cannot pop element)
    if ( network_tx_queue_is_empty())
    {
        return false;
    }

    tx_queue_pop_idx = ( tx_queue_pop_idx + 1) % TX_QUEUE_SIZE;
    return true;
}


/**
 * Determines whether the network's receive queue is full
 *
 * @return  True if the queue is full, false otherwise.
 */
bool network_rx_queue_is_full()
{
    return rx_queue_pop_idx == rx_queue_push_idx;
}


/**
 * Determines whether the network's receive queue is full
 *
 * @return  True if the queue is full, false otherwise.
 */
bool network_rx_queue_is_empty()
{
    return ( rx_queue_pop_idx + 1) % TX_QUEUE_SIZE == rx_queue_push_idx;
}


/**
 * Gets the number of messages in the receive queue
 *
 * @return  The number of messages in the message queue
 */
unsigned int network_rx_queue_count()
{
    if ( rx_queue_push_idx > rx_queue_pop_idx)
    {
        return rx_queue_push_idx - rx_queue_pop_idx - 1;
    }
    else
    {
        return ( rx_queue_push_idx + RX_QUEUE_SIZE) - rx_queue_pop_idx - 1;
    }
}


/**
 * Pushes a singular bit into the "under-construction" element in the
 * receive queue
 *
 * @param   [in]    bit     The bit to push
 *
 * @return  True if the bit is added successfully, false otherwise
 */
bool network_rx_queue_push_bit(bool bit)
{
    // TODO
}

/**
 * Fetches the value of the bit that was most recently pushed into the
 * "under-construction" element of the receive queue
 *
 * @return  The value of the most recently pushed bit
 */
bool network_rx_queue_get_last_bit()
{
    // TODO
}

/**
 * Formally pushes an "under-construction" element into the receive queue
 *
 * @return  True if the element pushes successfully, false otherwise
 */
bool network_rx_queue_push()
{
    // TODO
}


/**
 * Pushes an element from the receive queue
 *
 * @return  True if the element pushes successfully, false otherwise
 */
bool network_rx_queue_pop()
{
    // TODO
}

/**
 * Decodes a manchester encoded message into a frame
 *
 * @param   [out]   frame       Frame to decode into (must have message buffer pre-allocated)
 * @param   [in]    manchester  The input buffer of a manchester encoded frame
 * @return  error code
 */
static ERROR_CODE network_decode_manchester_frame( frame_t * frame, uint8_t * manchester)
{
    ERROR_HANDLE_NON_FATAL(network_decode_manchester_header(&frame->header, manchester));
    return network_decode_manchester_frame_message_trailer(
        frame,
        manchester + sizeof(frame_header_t) * 2
    );
    RETURN_NO_ERROR();  
}

/**
 * Decodes a manchester encoded frame header
 *
 * @param   [out]   header      Frame header to decode into
 * @param   [in]    manchester  Pointer to the start of a Manchester encoded frame header
 * @return  error code
 */
static ERROR_CODE network_decode_manchester_header( frame_header_t * header, uint8_t * manchester)
{
    return network_decode_manchester((uint8_t *) header, manchester, sizeof(frame_header_t));
}

/**
 * Decodes a manchester encoded message and trailer into a frame with an existing header
 *
 * @param   [out]   frame       Frame to decode into (must have message buffer pre-allocated and header already decoded)
 * @param   [in]    manchester  Pointer to the start of the message in a Manchester encoded frame
 * @return  error code
 */
static ERROR_CODE network_decode_manchester_frame_message_trailer( frame_t * frame, uint8_t * manchester)
{
    ERROR_HANDLE_NON_FATAL(network_decode_manchester(
        (uint8_t *)frame->message, 
        manchester, 
        frame->header.length)
    );
    ERROR_HANDLE_NON_FATAL(network_decode_manchester(
        (uint8_t *)&frame->trailer, 
        manchester + frame->header.length * 2, 
        sizeof(frame_trailer_t))
    );
    RETURN_NO_ERROR();
}

/**
 * Encodes a frame into a manchester encoded buffer
 * 
 * @param   [in]    manchester  Buffer to encode frame into. Must be at least the max size of a manchester encoded frame
 * @param   [out]   frame       Frame to encode

 * @return  number of bytes filled into the manchester buffer
 */
static unsigned int network_encode_frame_manchester( uint8_t * manchester, frame_t * frame)
{
    unsigned int size = 0;
    size += network_encode_manchester(manchester,  (uint8_t *) &(frame->header), sizeof(frame_header_t));
    size += network_encode_manchester(manchester + size, frame->message, frame->header.length);
    size += network_encode_manchester(manchester + size, (uint8_t *) &(frame->trailer), sizeof(frame_trailer_t));
    return size;
}


/**
 * Encodes a buffer into Manchester encoding
 *
 * @param   [out] buffer      The out buffer to decode from Manchester
 * @param   [in]  manchester  The input Manchester encoded buffer
 * @param   [in]  size        The number of DECODED BYTES expected from 
 *                              the manchester buffer (1/2 x size of manchester input buffer)
 */
static ERROR_CODE network_decode_manchester(uint8_t * buffer, uint8_t * manchester, size_t size)
{
    #ifdef MANCHESTER_DBG
    printBytesHex("MANCHESTER", manchester, size*2);
    #endif

    // zero the buffer
    memset(buffer, 0, size);


    // decoding loop
    for (unsigned int byteIdx = 0; byteIdx < size; byteIdx++)
    {
        uint8_t * bytePtr = &(buffer[byteIdx]);

        for (unsigned int bitIdx = 0; bitIdx <8; bitIdx++)
        {
            unsigned int manchesterBitIdx = (bitIdx * 2) % 8;

            unsigned int manchesterValue = (manchester[bitIdx > 3 ? byteIdx * 2 +1: byteIdx * 2] 
                                                >> (6 - manchesterBitIdx)) & 0b11;
            bool bitValue;  
            if (manchesterValue == 0b01){
                bitValue = true;
            }
            else if (manchesterValue == 0b10)
            {
                bitValue = false;
            }
            else {
                THROW_ERROR(ERROR_CODE_INVALID_MANCHESTER_RECEIVED); //todo error code, invalid manchester
            }
            *bytePtr |= bitValue << (7 - bitIdx);
        }
    }

    #ifdef MANCHESTER_DBG
    printBytesHex("BUFFER", buffer, size);
    #endif
    RETURN_NO_ERROR();
}



/**
 * Encodes a buffer into Manchester encoding
 *
 * @param   [out]   manchester  The output Manchester encoded buffer (will be
 *                              twice the size of the input buffer)
 * @param   [in]    buffer      The input buffer to encode in Manchester
 * @param   [in]    size        The size of the input buffer
 */
static unsigned int
network_encode_manchester(uint8_t * manchester, uint8_t * buffer, size_t size)
{
    #ifdef MANCHESTER_DBG
    printBytesHex("BUFFER", buffer, size);
    #endif

    // zero the manchester buffer
    memset(manchester, 0, size * 2);

    // encoding loop
    for (unsigned int byteIdx = 0; byteIdx < size; byteIdx++)
    {
        uint8_t inputByteValue = buffer[byteIdx];
        uint8_t * manchesterBytePtr = &(manchester[byteIdx * 2]);

        for (unsigned int bitIdx = 0; bitIdx < 8; bitIdx++)
        {
            bool inputBitValue = (inputByteValue >> (7 - bitIdx)) & 0x01;

            unsigned int manchesterBitIdx = (bitIdx * 2) % 8;
            unsigned int manchesterBitsValue = inputBitValue ? 0b01 : 0b10;

            manchesterBytePtr[bitIdx > 3 ? 1 : 0] |= manchesterBitsValue << (6 - manchesterBitIdx);
        }
    }

    #ifdef MANCHESTER_DBG
    printBytesHex("MANCHESTER", manchester, size*2);
    #endif

    return size * 2;
}


/**
 * IRQ Handler for hb_timer
 */
void TIM4_IRQHandler()

{
    static int byteIdx = 0; // A value 0 - 511
    static int bitIdx = 0; // A value 0 - 7

    if ( TIM4->SR & TIM_SR_UIF )
    {
        // clear update interrupt
        TIM4->SR &= ~( TIM_SR_UIF );

        // Get the message index of the circular buffer
        int msg_idx = ( tx_queue_pop_idx + 1) % TX_QUEUE_SIZE;

        // Get the current state
        STATE_TYPE state = state_get();

        if(state != COLLISION)
        {
            hb_timer_reset_and_start();

            if( byteIdx == tx_queue[msg_idx].size)
            {
                // The transmission of the message is complete

                // Stop the timer
                hb_timer_stop();
                // Set the byteIdx and bitIdx to default
                byteIdx = 0;
                bitIdx = 0;
                // Should always return True because when we call this method there is always a message present
                bool status = network_tx_queue_pop();
                if (!status)
                {
                    ERROR_HANDLE_NON_FATAL(ERROR_CODE_NETWORK_MSG_POP_FAILURE)
                }

                // Output a 1 to PC11 - IDLE State
                GPIOC->ODR |= GPIO_ODR_OD11;

            } else
            {
                // Get the next bit from the message buffer
                bool bit = tx_queue[msg_idx].buffer[byteIdx] >> ( 7 - bitIdx) & 0b01;

                if(bit == 1)
                {
                    // Output a 1 to PC11
                    GPIOC->ODR |= GPIO_ODR_OD11;
                }else {
                    // Output a 0 to PC11
                    GPIOC->ODR &= ~GPIO_ODR_OD11;
                }

                // If bit index is less than 7 increment
                if(bitIdx < 7)
                {
                    // Increment the bit index
                    bitIdx++;
                }else
                {
                    // Increment the byte index
                    byteIdx++;
                    // Set the bit index back to 0
                    bitIdx = 0;
                }
            }

        } else
        {
            // Stop the timer we are in COLLISION State
            hb_timer_stop();
            // Reset Transmission of the data
            byteIdx = 0;
            bitIdx = 0;
            // Output a 1 to PC11
            GPIOC->ODR |= GPIO_ODR_OD11;
        }
    }
}
