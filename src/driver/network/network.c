#include <stdbool.h>
#include <uio.h>
#include <memory.h>
#include "stm32f446xx.h"

#include "network.h"
#include "state.h"
#include "hb_timer.h"

#define MSG_QUEUE_SIZE                  (10)
#define HALF_BIT_PERIOD_US              (500)

#define MAX_MESSAGE_SIZE                (256)
#define MAX_MESSAGE_SIZE_MANCHESTER     (MAX_MESSAGE_SIZE * 2)

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
 * Network circular message queue. One is added to the message queue size
 * because there is one unused node when the queue is full.
 *
 * So, 10 + 1 total nodes - 1 unusable node  = 10 usable nodes.
 */
static msg_queue_node_t msg_queue[MSG_QUEUE_SIZE + 1];

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
    // Initialize PC11 as an Output
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; //SYSCFGEN
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;  //GPIOCEN

    GPIOC->MODER &= ~GPIO_MODER_MODER11;
    GPIOC->MODER |= GPIO_MODER_MODER11_0; //Set PC11 as output

    // throw an error if the network is already initialized
    if (network_is_init)
    {
        THROW_ERROR(ERROR_CODE_NETWORK_ALREADY_INITIALIZED);
    }

    ELEVATE_IF_ERROR(hb_timer_init( HALF_BIT_PERIOD_US));

    network_is_init = true;

    RETURN_NO_ERROR();
}


/**
 * Adds a buffer to a message queue and attempts to begin transmission.
 *
 * @param   [in]    buffer  The buffer to transmit
 * @param   [in]    size    The size of the buffer in bytes
 *
 * @return  Error code
 */
ERROR_CODE network_tx(void * buffer, size_t size)
{
    // throw an error if the network is not initialized
    if (!network_is_init)
    {
        THROW_ERROR(ERROR_CODE_NETWORK_NOT_INITIALIZED);
    }

    int queued_bytes = 0;

    // if size > MAX_MESSAGE_SIZE, the buffer must be broken into chunks
    while ((size - queued_bytes) > MAX_MESSAGE_SIZE)
    {
        if (!network_msg_queue_push( buffer + queued_bytes, MAX_MESSAGE_SIZE ))
        {
            THROW_ERROR(ERROR_CODE_NETWORK_MSG_QUEUE_FULL);
        }
        queued_bytes += MAX_MESSAGE_SIZE;
    }

    // after that, we can queue up the remaining bytes
    if (!network_msg_queue_push( buffer + queued_bytes, size - queued_bytes ))
    {
        THROW_ERROR(ERROR_CODE_NETWORK_MSG_QUEUE_FULL);
    }

    // attempt to start a transmission
    ELEVATE_IF_ERROR(network_start_tx());

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
    return ( push_idx + 1) % MSG_QUEUE_SIZE == pop_idx;
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
 * Pushes an element into this network's circular queue
 *
 * @param   [in]    buffer  The buffer to enqueue
 * @param   [in]    size    The size of the buffer
 *
 * @return  False if the message queue is full, true otherwise
 */
static bool network_msg_queue_push(void * buffer, size_t size)
{
    // return false if the queue is full
    if (network_msg_queue_is_full())
    {
        return false;
    }

    // it is important that we make a copy of the buffer in the queue,
    // otherwise we risk modifying the data before it can be transmitted
    memcpy( msg_queue[push_idx].buffer, buffer, size);
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
    if (!network_msg_queue_is_empty())
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

    // Get the message index of the circular buffer
    int msg_idx;
    msg_idx = (pop_idx + 1) % MSG_QUEUE_SIZE;

    // Get the current state
    STATE_TYPE state = state_get();

    if(state != COLLISION)
    {
        // Reset the timer
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
            network_msg_queue_pop();
            // Output a 1 to PC11 - IDLE State
            GPIOC->ODR |= GPIO_ODR_OD11;

        } else
        {
            // Get the next bit from the message buffer
            bool bit = msg_queue[msg_idx].buffer[byteIdx] >> (bitIdx) & 0b01;

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
