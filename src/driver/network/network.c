#include <stdbool.h>
#include <uio.h>
#include <memory.h>
#include "stm32f446xx.h"

#include "network.h"
#include "state.h"
#include "hb_timer.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

#define MSG_QUEUE_SIZE                  (10)
#define HALF_BIT_PERIOD_US              (500)

#define MAX_MESSAGE_SIZE                (255)
#define MAX_FRAME_SIZE                  (MAX_MESSAGE_SIZE + sizeof(msg_header_t) + sizeof(msg_trailer_t))

#define MAX_MESSAGE_SIZE_MANCHESTER     (MAX_MESSAGE_SIZE * 2)

#define LOCAL_MACHINE_ADDRESS           (0x23) //TODO changeme if needed
#define HEADER_PREAMBLE                 (0x55)
#define PROTOCOL_VERSION                (0x01)

// #define NETWORK_TX_DBG
// #define MANCHESTER_DBG

/**
 * Initialization flag
 */
static bool network_is_init = false;

/**
 * Message node for the network's circular message queue
 */
typedef struct
{
    char buffer[MAX_MESSAGE_SIZE_MANCHESTER];
    size_t size;
} msg_queue_node_t;


/**
 * Network circular message queue.
 */
static msg_queue_node_t msg_queue[MSG_QUEUE_SIZE];

/**
 * References the index of the location where the next element will be pushed
 * onto the queue
 */
static int push_idx = 1;

/**
 * References the index of the most recently popped element of the queue
 */
static int pop_idx = 0;


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
 * Adds a buffer to a message queue and attempts to begin transmission.
 *
 * @param   [in]    buffer  The buffer to transmit
 * @param   [in]    size    The size of the buffer in bytes
 *
 * @return  Error code
 */
ERROR_CODE network_tx(uint8_t * buffer, size_t size)
{
    // throw an error if the network is not initialized
    if (!network_is_init)
    {
        THROW_ERROR(ERROR_CODE_NETWORK_NOT_INITIALIZED);
    }

    // TODO: This needs to be fixed to accurately calculate the number of
    //       messages required for transmission
    // unsigned int availableMsgs = MSG_QUEUE_SIZE - network_msg_queue_count();
    // if ((size / MAX_MESSAGE_SIZE) <= availableMsgs)
    // {
    //     THROW_ERROR(ERROR_CODE_NETWORK_MSG_QUEUE_FULL);
    // }

    unsigned int queued_bytes = 0;
    unsigned char manchester[MAX_MESSAGE_SIZE_MANCHESTER];


    msg_frame_t frame = {{HEADER_PREAMBLE, PROTOCOL_VERSION, LOCAL_MACHINE_ADDRESS, 0x0, 0x0, 0x01}, NULL, {0x00}};

    //TODO set dest IP
    frame.header.destination = 0x00;

    // break buffer into chunks and queue
    while (size - queued_bytes)
    {
        frame.message = buffer + queued_bytes;

        //TODO set trailer with big brain CRC algorithm
        frame.trailer.crc8_fcs = 0x00;

        frame.header.length = MIN(MAX_MESSAGE_SIZE, size - queued_bytes);

        unsigned int manchesterSize = network_encode_frame_manchester(manchester, &frame);


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


        if (!network_msg_queue_push(manchester, manchesterSize))
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

    if (!network_msg_queue_is_empty() && (state_get() == IDLE))
    {
        ELEVATE_IF_ERROR(hb_timer_reset_and_start());
    }

    RETURN_NO_ERROR();
}


/**
 * Determines whether the network's message queue is full
 *
 * @return  True if the queue is full, false otherwise.
 */
bool network_msg_queue_is_full()
{
    return pop_idx == push_idx;
}


/**
 * Determines whether the network's message queue is full
 *
 * @return  True if the queue is full, false otherwise.
 */
bool network_msg_queue_is_empty()
{
    return (pop_idx + 1) % MSG_QUEUE_SIZE == push_idx;
}


/**
 * Gets the number of messages in the 
 *  queue
 *
 * @return  The number of messages in the message queue
 */
unsigned int network_msg_queue_count()
{
    if (push_idx > pop_idx)
    {
        return push_idx - pop_idx - 1;
    }
    else
    {
        return (push_idx + MSG_QUEUE_SIZE) - pop_idx - 1;
    }

}

/**
 * Decodes a manchester encoded message into a frame
 *
 * @param   [out]   frame       Frame to decode into (must have message buffer pre-allocated)
 * @param   [in]    manchester  The input buffer of a manchester encoded frame
 * @return  error code
 */
static ERROR_CODE network_decode_manchester_frame(msg_frame_t * frame, uint8_t * manchester)
{
    ERROR_HANDLE_NON_FATAL(network_decode_manchester_header(&frame->header, manchester));
    return network_decode_manchester_frame_message_trailer(
        frame,
        manchester + sizeof(msg_header_t) * 2
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
static ERROR_CODE network_decode_manchester_header(msg_header_t * header, uint8_t * manchester)
{
    return network_decode_manchester((uint8_t *) header, manchester, sizeof(msg_header_t));
}

/**
 * Decodes a manchester encoded message and trailer into a frame with an existing header
 *
 * @param   [out]   frame       Frame to decode into (must have message buffer pre-allocated and header already decoded)
 * @param   [in]    manchester  Pointer to the start of the message in a Manchester encoded frame
 * @return  error code
 */
static ERROR_CODE network_decode_manchester_frame_message_trailer(msg_frame_t * frame, uint8_t * manchester)
{
    ERROR_HANDLE_NON_FATAL(network_decode_manchester(
        (uint8_t *)frame->message, 
        manchester, 
        frame->header.length)
    );
    ERROR_HANDLE_NON_FATAL(network_decode_manchester(
        (uint8_t *)&frame->trailer, 
        manchester + frame->header.length * 2, 
        sizeof(msg_trailer_t))
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
static unsigned int network_encode_frame_manchester(uint8_t * manchester, msg_frame_t * frame)
{
    unsigned int size = 0;
    size += network_encode_manchester(manchester,  (uint8_t *) &(frame->header), sizeof(msg_header_t));
    size += network_encode_manchester(manchester + size, frame->message, frame->header.length);
    size += network_encode_manchester(manchester + size, (uint8_t *) &(frame->trailer), sizeof(msg_trailer_t));
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
 * Pushes an element into this network's circular queue
 *
 * @param   [in]    buffer  The buffer to enqueue
 * @param   [in]    size    The size of the buffer
 *
 * @return  False if the message queue is full, true otherwise
 */
static bool network_msg_queue_push(uint8_t * buffer, size_t size)
{
    // return false if the queue is full
    if (network_msg_queue_is_full())
    {
        return false;
    }

    // it is important that we make a copy of the buffer in the queue,
    // otherwise we risk modifying the data before it can be transmitted
    memcpy(msg_queue[push_idx].buffer, buffer, size);
    msg_queue[push_idx].size = size;

    push_idx = ( push_idx + 1) % MSG_QUEUE_SIZE;

    return true;
}


/**
 * Pops an element from this network's circular queue
 *
 * @return True if an element is successfully popped, false otherwise
 */
static bool network_msg_queue_pop()
{
    // return false if the queue is empty (cannot pop element)
    if (network_msg_queue_is_empty())
    {
        return false;
    }

    pop_idx = (pop_idx + 1) % MSG_QUEUE_SIZE;
    return true;
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
        int msg_idx = (pop_idx + 1) % MSG_QUEUE_SIZE;

        // Get the current state
        STATE_TYPE state = state_get();

        if(state != COLLISION)
        {
            hb_timer_reset_and_start();

            if(byteIdx == msg_queue[msg_idx].size)
            {
                // The transmission of the message is complete

                // Stop the timer
                hb_timer_stop();
                // Set the byteIdx and bitIdx to default
                byteIdx = 0;
                bitIdx = 0;
                // Should always return True because when we call this method there is always a message present
                bool status = network_msg_queue_pop();
                if (!status)
                {
                    ERROR_HANDLE_NON_FATAL(ERROR_CODE_NETWORK_MSG_POP_FAILURE)
                }

                // Output a 1 to PC11 - IDLE State
                GPIOC->ODR |= GPIO_ODR_OD11;

            } else
            {
                // Get the next bit from the message buffer
                bool bit = msg_queue[msg_idx].buffer[byteIdx] >> (7 - bitIdx) & 0b01;

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
