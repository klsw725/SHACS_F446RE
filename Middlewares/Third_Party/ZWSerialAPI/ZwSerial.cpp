/****************************************************************************
 *
 * Z-Wave, the wireless language.
 *
 * Copyright (c) 2001-2011
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Serial interface functions.
 *
 * Last Changed By:  $Author$: 
 * Revision:         $Rev$: 
 * Last Changed:     $Date$: 
 * History
   -  2017.11.06 Modified by JY Koh for SHACS project
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "ZwSerial.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static char lastError=0;

CSerial::CSerial()
{

}

CSerial::~CSerial()
{

}

/****************************************************************************
 *
 * Function:  serGetLastError
 *
 *   Get the error code for the last serial.c function that failed.
 *
 * Input: Nothing
 *
 * Return: The error code
 *
 ***************************************************************************/
char CSerial::serGetLastError()
{
    return lastError;
}

/****************************************************************************
 *
 * Function:  serOpen
 *
 *   Opens a serial port in a given mode.
 *
 * Input: port, string describing with port to open (ie. COM1, COM2..)
 *        mode, the bitrate plus the number of databits,stopbits and
 *        the parity. Should be given as one of the serial_xxxx_yyy
 *        constants (see serial.h)
 *
 * Return: A win32 HANDLE to the file corresponding to the serial port.
 *
 ***************************************************************************/
HANDLE CSerial::serOpen(char *port, int mode)
{

}

/****************************************************************************
 *
 * Function:  serClose
 *
 *   Free a serial port after use.
 *
 * Input: h, the win32 handle returned by serOpen().
 *
 * Return: Nothing
 *
 ***************************************************************************/
void CSerial::serClose(UART_HandleTypeDef *h)
{

}

/****************************************************************************
 *
 * Function:  serRead
 *
 *   Reads a number of bytes from a serial port. Will wait until 
 *   all request bytes are available.
 *
 * Input: h, the win32 handle returned by serOpen
 *        buf, pointer to where the data should be stored
 *        len, the number of bytes to receive
 *
 * Return: The number of bytes actually read.
 *
 ***************************************************************************/
unsigned int CSerial::serRead(UART_HandleTypeDef *uart, LPVOID buf, DWORD len)
{
  DWORD numread;
  int	i;

  numread = HAL_UART_Receive(uart, buf, len, SER_TIMEOUT);

  if (numread == HAL_OK)
	  numread = len;
  else
	  numread = 0;

  return numread;
}

/****************************************************************************
 *
 * Function:  serGetChar
 *
 *   Read a single byte from a serial port.
 *
 * Input: h, the win32 handle returned by serOpen()
 *        bStop, pointer to boolean indicating if the thread is stopping
 *
 * Return: The read byte.
 *
 ***************************************************************************/
unsigned int CSerial::serGetChar(UART_HandleTypeDef *uart, unsigned char *c, BOOL *bStop)
{
    int res;


#if	0	//
    res = HAL_UART_Receive(uart, c, 1, SER_TIMEOUT);
    if (res == HAL_OK)
    	printf("DR<--[%02X]\r\n", *c);

    if (res == HAL_OK)
    	res = 1;
    else
    	res = 0;
#else
    res =  Uart1_GetChar(c, SER_TIMEOUT);

#endif
    return res;
}

/****************************************************************************
 *
 * Function:  serWrite
 *
 *   Writes a number of bytes to a serial port. Will wait until 
 *   all bytes are sent.
 *
 * Input: h, the win32 handle returned by serOpen
 *        buf, pointer to where the data are be stored
 *        len, the number of bytes to send
 *
 * Return: The number of bytes actually send.
 *
 ***************************************************************************/
unsigned int CSerial::serWrite(UART_HandleTypeDef *uart, LPVOID buf, DWORD len)
{
    DWORD num;
    int   i;

    num = HAL_UART_Transmit(uart, buf, len, 0xFFFF);

    return num;
} 

/****************************************************************************
 *
 * Function:  serPutChar
 *
 *   Write a single byte to a serial port.
 *
 * Input: h, the win32 handle returned by serOpen()
 *        c, the char to write/send
 *
 * Return: The number of bytes actually send.
 *
 ***************************************************************************/
unsigned int CSerial::serPutChar(UART_HandleTypeDef *uart, unsigned char c)
{
    return HAL_UART_Transmit(uart, &c, 1, 0xFFFF);
}

