#include "network.h"
#include "hb_timer.h"

#define MSG_QUEUE_SIZE 10
#define HALF_BIT_TIME_US 1000 //todo fixme

#define MAX_MESSAGE_SIZE 512 //todo fixme
#define MAX_MESSAGE_SIZE_MANCHESTER MAX_MESSAGE_SIZE * 2

typedef struct msg_struct
{
    char buffer[MAX_MESSAGE_SIZE];
    size_t size;
} msg_t;


static msg_t msgQueue[MSG_QUEUE_SIZE];

static int startIdx = 0;
static int endIdx = 0;


ERROR_CODE network_init()
{
    hb_timer_init(HALF_BIT_TIME_US);
    RETURN_NO_ERROR();
}

ERROR_CODE network_tx(char* msg, size_t size) 
{
    if (network_tx_isFull())
    {
        THROW_ERROR(ERROR_CODE_NETWORK_TX_FULL);
    }

    //todo packetize message & store


    endIdx = (endIdx + 1) % MSG_QUEUE_SIZE;
    RETURN_NO_ERROR();
}

bool network_tx_isFull()
{
    return (endIdx + 1) % MSG_QUEUE_SIZE == startIdx;
}

//Returns false if tx buffer is empty
bool network_startTx()
{
    if (endIdx == startIdx)
    {
        return false;
    } 
    else
    {
        //todo start tx
        return true;
    }
}

//todo implement IRQHandler for hb_timer
void TIMXXX_IRQHandler()
{
    static int byteIdx;
    static int bitIdx;

}
