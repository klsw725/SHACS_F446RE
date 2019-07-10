/****************************************************************************
 *  Task for handling inputs from USB device
 *  2017.11.06 Created by JY Koh
*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#include "UsbSerial.h"

extern	UART_HandleTypeDef huart5;
extern	osSemaphoreId UsbCommandQueueHandle;
extern	osSemaphoreId UsbResponseQueueHandle;

extern void get_NodeInfomation(BYTE *get_nodeList, BYTE *get_nodeType, BYTE *NumofNode);

#define SOF 0xAB  /* Start Of Frame */
#define CMD 0xAD  /*  Cmd Frame  */
#define ACK 0xA5  /*  Ack Frame  */

#define	SOF_TIMEOUT		600
#define	BODY_TIMEOUT	100

#define USB_QUEUE_MAX 		3
#define	USB_BUF_SIZE		32
#define	ZW_MAX_NODES		16

/* UsbCommand queue */
struct UsbCommandElement
{
  BYTE  buf[USB_BUF_SIZE];      /* xxxx       */
  BYTE  length;         /* Frame type */
} UsbCommandQueue[USB_QUEUE_MAX];

/* BtResponse queue */
struct UsbResponseElement
{
  BYTE  buf[USB_BUF_SIZE];      /* xxxx       */
  BYTE  length;         /* Frame type */
} UsbResponseQueue[USB_QUEUE_MAX];

static BYTE UsbQueueIndex = 0;
static BYTE UsbResponseQueueIndex = 0;

int	get_USBcommand(BYTE *buf, BYTE *len)
{
	if (UsbQueueIndex > 0)
	{
		osSemaphoreWait(UsbCommandQueueHandle, 0);

		UsbQueueIndex--;
        *len = UsbCommandQueue[0].length;
        memcpy(buf,UsbCommandQueue[0].buf,UsbCommandQueue[0].length);

        /* POP the message from the queue (fifo) */
		memcpy((BYTE *)&UsbCommandQueue[0], (BYTE *)&UsbCommandQueue[1],
                    UsbQueueIndex * sizeof(struct UsbCommandElement));
		osSemaphoreRelease(UsbCommandQueueHandle);

		return	1;
	}
	else
	{
		osDelay(20);
		return	0;
	}
}

int	put_USBcommand(BYTE *buf, BYTE length)
{
	int	ret;

	osSemaphoreWait(UsbCommandQueueHandle,0);
	if (UsbQueueIndex < USB_QUEUE_MAX)
	{
		memcpy(UsbCommandQueue[UsbQueueIndex].buf,buf,length);
		UsbCommandQueue[UsbQueueIndex].length = length;
		UsbQueueIndex++;
		ret = 1;
	}
	else
		ret	= 0;

	osSemaphoreRelease(UsbCommandQueueHandle);

	return	(ret);
}


int	put_USBresponse(BYTE *buf, BYTE length)
{
	int	ret;
	osSemaphoreWait(UsbResponseQueueHandle,0);
	if (UsbResponseQueueIndex < USB_QUEUE_MAX)
	{
		memcpy(UsbResponseQueue[UsbResponseQueueIndex].buf,buf,length);
		UsbResponseQueue[UsbResponseQueueIndex].length = length;
		UsbResponseQueueIndex++;
		ret = 1;
	}
	else
		ret	= 0;

	osSemaphoreRelease(UsbResponseQueueHandle);
	return	(ret);
}

/*
*	UsbSerialApp	Task
*/
void UsbSerialApp(void const *argument)
{
    int i=0;
    BYTE ch, length;
    BYTE buffer[USB_BUF_SIZE+1];
//    BYTE NodeList[ZW_MAX_NODES];
//    BYTE NodeType[ZW_MAX_NODES];

    osDelay(10);
    printf("\r\nUsbSerialApp Start\r\n");

#if	0	//Controlled by upperdevice(BT or USB command )
    osDelay(500);

    printf("\r\nUsb OTG Enable\r\n");

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 1);
    printf("\r\nUsb OTG Start\r\n");
#endif


	HAL_UART_Receive_IT(&huart5,&ch,1);

    while(1)
    {
    	// check TX data to USB
        if (UsbResponseQueueIndex > 0)
        {
        	osSemaphoreWait(UsbResponseQueueHandle, 0);

            UsbResponseQueueIndex--;

            length = UsbResponseQueue[0].length;
            memset(buffer,0,sizeof(buffer));
#if		1	// Add len Byte
            memcpy(buffer,UsbResponseQueue[0].buf,2);	//Header
            buffer[2] = UsbResponseQueue[0].length;
            memcpy(&buffer[3],&UsbResponseQueue[0].buf[2],UsbResponseQueue[0].length-2);
            length +=1;
#else
            memcpy(buffer,UsbResponseQueue[0].buf,UsbResponseQueue[0].length);
#endif
            /* POP the message from the queue (fifo) */
            memcpy((BYTE *)&UsbResponseQueue[0], (BYTE *)&UsbResponseQueue[1],
                    UsbResponseQueueIndex * sizeof(struct UsbResponseElement));
        	osSemaphoreRelease(UsbResponseQueueHandle);
            //// Sent Data to Usb

        	HAL_UART_Transmit(&huart5, (uint8_t *)buffer, length, 0x100);
        	debuglog_wr_frame("[USB]Tx -> ", buffer, length);
        }
    	// Wait for SOF (StartOfFrame)
    	if (Uart5_GetChar(&ch, SOF_TIMEOUT) == 0)
    	{
    		continue;
    	}
    	if ( ch != SOF )
    	{
    		printf("[USB]Received - Garbage %02X\r\n", ch);
    		continue;
        }
    	else
    	{
    		printf("[USB]Received - SOF[%02X]\r\n",ch);
        	// Wait for CMD/ACK (Frame)
        	if (Uart5_GetChar(&ch, BODY_TIMEOUT) == 0)
        	{
        		printf("[USB]Received - Timeout of CMD/ACK\r\n");
        		continue;
        	}
            if (( ch != CMD ) && ( ch != ACK ))
            {
            	printf("[USB]Received - Garbage %02X while waiting CMD/ACK\r\n", ch);
            	continue;
            }
    		printf("[USB]Received - CMDorACK[%02X]\r\n",ch);
            // Receive Body of Command/Ack
            buffer[0] = ch;
            i = 1;
            while(1)
            {
            	if (Uart5_GetChar(&buffer[i], BODY_TIMEOUT) == 0)
            	{
            		printf("[USB]Received - Timeout while waiting Body\r\n");
            		break;
            	}
            	printf("[USB]Received - %02X\r\n", buffer[i]);
            	i++;
            }
        	printf("[USB]Total Recv Length - %d\r\n", i);

        	if (buffer[0] == CMD)
			{
				printf("[USB] CMD=%02x\r\n", buffer[0]);
				printf("put_USBcommand[%d]\r\n", i);
				put_USBcommand(buffer, i);
//				printf("get_NodeInfomation[%d]\r\n",ch2);
			}
			else if (buffer[0] == ACK)
			{
				printf("[USB] ACK=%02x\r\n", buffer[0]);
			}
    	}
    }
}
