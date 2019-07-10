/****************************************************************************
 *  Task for handling inputs from BT device
 *  2017.11.06 Created by JY Koh
*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#include "BtSerial.h"

#define LPVOID  uint8_t *
#define DWORD   unsigned int
#define WORD    unsigned short
#define BYTE    unsigned char
#define BOOL    int

extern	UART_HandleTypeDef huart3;
extern	osSemaphoreId BtCommandQueueHandle;
extern	osSemaphoreId BtResponseQueueHandle;


extern void get_NodeInfomation(BYTE *get_nodeList, BYTE *get_nodeType, BYTE *NumofNode);

#define SOF 0xAB  /* Start Of Frame */
#define CMD 0xAD  /*  Cmd Frame  */
#define ACK 0xA5  /*  Ack Frame  */


#define	SOF_TIMEOUT		600
#define	BODY_TIMEOUT	100

#define BT_QUEUE_MAX 		3
#define	BT_BUF_SIZE			32
#define	ZW_MAX_NODES		16

/* BtCommand queue */
struct BtCommandElement
{
  BYTE  buf[BT_BUF_SIZE];      /* xxxx       */
  BYTE  length;         /* Frame type */
} BtCommandQueue[BT_QUEUE_MAX];

/* BtResponse queue */
struct BtResponseElement
{
  BYTE  buf[BT_BUF_SIZE];      /* xxxx       */
  BYTE  length;         /* Frame type */
} BtResponseQueue[BT_QUEUE_MAX];

static BYTE BtQueueIndex = 0;
static BYTE BtResponseQueueIndex = 0;

int	get_BTcommand(BYTE *buf, BYTE *len)
{
	if (BtQueueIndex > 0)
	{
		osSemaphoreWait(BtCommandQueueHandle, 0);

		BtQueueIndex--;
        *len = BtCommandQueue[0].length;
        memcpy(buf,BtCommandQueue[0].buf,BtCommandQueue[0].length);

        /* POP the message from the queue (fifo) */
		memcpy((BYTE *)&BtCommandQueue[0], (BYTE *)&BtCommandQueue[1],
                    BtQueueIndex * sizeof(struct BtCommandElement));
		osSemaphoreRelease(BtCommandQueueHandle);

		return	1;
	}
	else
	{
		osDelay(20);
		return	0;
	}
}

int	put_BTcommand(BYTE *buf, BYTE length)
{
	int	ret;
	osSemaphoreWait(BtCommandQueueHandle,0);
	if (BtQueueIndex < BT_QUEUE_MAX)
	{
		memcpy(BtCommandQueue[BtQueueIndex].buf,buf,length);
		BtCommandQueue[BtQueueIndex].length = length;
		BtQueueIndex++;
		ret = 1;
	}
	else
		ret	= 0;

	osSemaphoreRelease(BtCommandQueueHandle);
	return	(ret);
}

int	put_BTresponse(BYTE *buf, BYTE length)
{
	int	ret;

	osSemaphoreWait(BtResponseQueueHandle,0);
	if (BtResponseQueueIndex < BT_QUEUE_MAX)
	{
		memcpy(BtResponseQueue[BtResponseQueueIndex].buf,buf,length);
		BtResponseQueue[BtResponseQueueIndex].length = length;
		BtResponseQueueIndex++;
		ret = 1;
	}
	else
		ret	= 0;

	osSemaphoreRelease(BtResponseQueueHandle);
	return	(ret);
}

/*
*	BtSerialApp	Task
*/
void BtSerialApp(void const *argument)
{
    int i=0;
    BYTE ch, length;
    BYTE buffer[BT_BUF_SIZE];
//    BYTE NodeList[ZW_MAX_NODES];
//    BYTE NodeType[ZW_MAX_NODES];

    printf("\r\nBtSerialApp Start\r\n");

	HAL_UART_Receive_IT(&huart3,&ch,1);

    while(1)
    {
    	// check TX data to BT
        if (BtResponseQueueIndex > 0)
        {
        	osSemaphoreWait(BtResponseQueueHandle, 0);

            BtResponseQueueIndex--;

            length = BtResponseQueue[0].length;
            memset(buffer,0,sizeof(buffer));
            memcpy(buffer,BtResponseQueue[0].buf,BtResponseQueue[0].length);

            /* POP the message from the queue (fifo) */
            memcpy((BYTE *)&BtResponseQueue[0], (BYTE *)&BtResponseQueue[1],
                    BtResponseQueueIndex * sizeof(struct BtResponseElement));
        	osSemaphoreRelease(BtResponseQueueHandle);
            //// Sent Data to Bt
        	HAL_UART_Transmit(&huart3, (uint8_t *)buffer, length, 0x100);
        	debuglog_wr_frame("[BT]Tx -> ", buffer, length);
        }

    	// Wait for SOF (StartOfFrame)
    	if (Uart3_GetChar(&ch, SOF_TIMEOUT) == 0)
    	{
    		continue;
    	}
    	if ( ch != SOF )
    	{
    		printf("[BT]Received - Garbage %02X\r\n", ch);
    		continue;
        }
    	else
    	{
    		printf("[BT]Received - SOF[%02X]\r\n",ch);
        	// Wait for CMD/ACK (Frame)
        	if (Uart3_GetChar(&ch, BODY_TIMEOUT) == 0)
        	{
        		printf("[BT]Received - Timeout of CMD/ACK\r\n");
        		continue;
        	}
            if (( ch != CMD ) && ( ch != ACK ))
            {
            	printf("[BT]Received - Garbage %02X while waiting CMD/ACK\r\n", ch);
            	continue;
            }
    		printf("[BT]Received - CMDorACK[%02X]\r\n",ch);
            // Receive Body of Command/Ack
            buffer[0] = ch;
            i = 1;
            while(1)
            {
            	if (Uart3_GetChar(&buffer[i], BODY_TIMEOUT) == 0)
            	{
            		printf("[BT]Received - Timeout while waiting Body\r\n");
            		break;
            	}
            	printf("[BT]Received - %02X\r\n", buffer[i]);
            	i++;
            }
        	printf("[BT]Total Recv Length - %d\r\n", i);

        	if (buffer[0] == CMD)
			{
				printf("[BT] CMD=%02x\r\n", buffer[0]);
				printf("put_BTcommand[%d]\r\n", i);
				put_BTcommand(buffer, i);
			}
			else if (buffer[0] == ACK)
			{
				printf("[BT] ACK=%02x\r\n", buffer[0]);
			}
    	}
    }
}


