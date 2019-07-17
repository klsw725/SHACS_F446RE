/****************************************************************************
 *  Task for handling inputs from BT device
 *  2017.11.06 Created by JY Koh
*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#include "WifiSerial.h"

#define LPVOID  uint8_t *
#define DWORD   unsigned int
#define WORD    unsigned short
#define BYTE    unsigned char
#define BOOL    int

extern	UART_HandleTypeDef huart4;
extern	osSemaphoreId WifiCommandQueueHandle;
extern	osSemaphoreId WifiResponseQueueHandle;


extern void get_NodeInfomation(BYTE *get_nodeList, BYTE *get_nodeType, BYTE *NumofNode);

#define SOF 0xAB  /* Start Of Frame */
#define CMD 0xAD  /*  Cmd Frame  */
#define ACK 0xA5  /*  Ack Frame  */


#define	SOF_TIMEOUT		600
#define	BODY_TIMEOUT	100

#define WIFI_QUEUE_MAX 		3
#define	WIFI_BUF_SIZE			32
#define	ZW_MAX_NODES		16

/* BtCommand queue */
struct WifiCommandElement
{
  BYTE  buf[WIFI_BUF_SIZE];      /* xxxx       */
  BYTE  length;         /* Frame type */
} WifiCommandQueue[WIFI_QUEUE_MAX];

/* BtResponse queue */
struct WifiResponseElement
{
  BYTE  buf[WIFI_BUF_SIZE];      /* xxxx       */
  BYTE  length;         /* Frame type */
} WifiResponseQueue[WIFI_QUEUE_MAX];

static BYTE WifiQueueIndex = 0;
static BYTE WifiResponseQueueIndex = 0;

int	get_WIFIcommand(BYTE *buf, BYTE *len)
{
	if (WifiQueueIndex > 0)
	{
		osSemaphoreWait(WifiCommandQueueHandle, 0);

		WifiQueueIndex--;
        *len = WifiCommandQueue[0].length;
        memcpy(buf,WifiCommandQueue[0].buf,WifiCommandQueue[0].length);

        /* POP the message from the queue (fifo) */
		memcpy((BYTE *)&WifiCommandQueue[0], (BYTE *)&WifiCommandQueue[1],
                    WifiQueueIndex * sizeof(struct WifiCommandElement));
		osSemaphoreRelease(WifiCommandQueueHandle);

		return	1;
	}
	else
	{
		osDelay(20);
		return	0;
	}
}

int	put_WIFIcommand(BYTE *buf, BYTE length)
{
	int	ret;
	osSemaphoreWait(WifiCommandQueueHandle,0);
	if (WifiQueueIndex < WIFI_QUEUE_MAX)
	{
		memcpy(WifiCommandQueue[WifiQueueIndex].buf,buf,length);
		WifiCommandQueue[WifiQueueIndex].length = length;
		WifiQueueIndex++;
		ret = 1;
	}
	else
		ret	= 0;

	osSemaphoreRelease(WifiCommandQueueHandle);
	return	(ret);
}

int	put_WIFIresponse(BYTE *buf, BYTE length)
{
	int	ret;

	osSemaphoreWait(WifiResponseQueueHandle,0);
	if (WifiResponseQueueIndex < WIFI_QUEUE_MAX)
	{
		memcpy(WifiResponseQueue[WifiResponseQueueIndex].buf,buf,length);
		WifiResponseQueue[WifiResponseQueueIndex].length = length;
		WifiResponseQueueIndex++;
		ret = 1;
	}
	else
		ret	= 0;

	osSemaphoreRelease(WifiResponseQueueHandle);
	return	(ret);
}

/*
*	WIfiSerialApp	Task
*/
void WifiSerialApp(void const *argument)
{
    int i=0;
    BYTE ch, length;
    BYTE buffer[WIFI_BUF_SIZE];
//    BYTE NodeList[ZW_MAX_NODES];
//    BYTE NodeType[ZW_MAX_NODES];

    printf("\r\nWIfiSerialApp Start\r\n");


//    while(1){osDelay(1000);}

	HAL_UART_Receive_IT(&huart4,&ch,1);

    while(1)
    {
    	// check TX data to BT
        if (WifiResponseQueueIndex > 0)
        {
        	osSemaphoreWait(WifiResponseQueueHandle, 0);

            WifiResponseQueueIndex--;

            length = WifiResponseQueue[0].length;
            memset(buffer,0,sizeof(buffer));
            memcpy(buffer,WifiResponseQueue[0].buf,WifiResponseQueue[0].length);

            /* POP the message from the queue (fifo) */
            memcpy((BYTE *)&WifiResponseQueue[0], (BYTE *)&WifiResponseQueue[1],
                    WifiResponseQueueIndex * sizeof(struct WifiResponseElement));
        	osSemaphoreRelease(WifiResponseQueueHandle);
            //// Sent Data to Bt
        	HAL_UART_Transmit(&huart4, (uint8_t *)buffer, length, 0x100);
        	debuglog_wr_frame("[WIFI]Tx -> ", buffer, length);
        }

    	// Wait for SOF (StartOfFrame)
    	if (Uart4_GetChar(&ch, SOF_TIMEOUT) == 0)
    	{
    		continue;
    	}
    	if ( ch != SOF )
    	{
    		printf("[WIFI]Received - Garbage %02X\r\n", ch);
    		continue;
        }
    	else
    	{
    		printf("[WIFI]Received - SOF[%02X]\r\n",ch);
        	// Wait for CMD/ACK (Frame)
        	if (Uart4_GetChar(&ch, BODY_TIMEOUT) == 0)
        	{
        		printf("[WIFI]Received - Timeout of CMD/ACK\r\n");
        		continue;
        	}
            if (( ch != CMD ) && ( ch != ACK ))
            {
            	printf("[WIFI]Received - Garbage %02X while waiting CMD/ACK\r\n", ch);
            	continue;
            }
    		printf("[WIFI]Received - CMDorACK[%02X]\r\n",ch);
            // Receive Body of Command/Ack
            buffer[0] = ch;
            i = 1;
            while(1)
            {
            	if (Uart4_GetChar(&buffer[i], BODY_TIMEOUT) == 0)
            	{
            		printf("[WIFI]Received - Timeout while waiting Body\r\n");
            		break;
            	}
            	printf("[WIFI]Received - %02X\r\n", buffer[i]);
            	i++;
            }
        	printf("[WIFI]Total Recv Length - %d\r\n", i);

        	if (buffer[0] == CMD)
			{
				printf("[WIFI] CMD=%02x\r\n", buffer[0]);
				printf("put_WIFIcommand[%d]\r\n", i);
				put_WIFIcommand(buffer, i);
			}
			else if (buffer[0] == ACK)
			{
				printf("[WIFI] ACK=%02x\r\n", buffer[0]);
			}
    	}
    }
}


