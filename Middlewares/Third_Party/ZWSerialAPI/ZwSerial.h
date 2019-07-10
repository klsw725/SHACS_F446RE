
/*****************************************************************************
* History
   -  2017.11.06 Modified by JY Koh for SHACS project
 ****************************************************************************/

#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"


#define SER_TIMEOUT      1000 /*Timeout value in milliseconds*/
#define HANDLE  int
#define LPVOID  uint8_t *
#define DWORD   unsigned int
#define WORD    unsigned short
#define BOOL    int

class CSerial  
{
public:
    static unsigned int serPutChar(UART_HandleTypeDef *h, unsigned char c);
    static unsigned int serWrite(UART_HandleTypeDef *h, LPVOID buf, DWORD len);
    static unsigned int serGetChar(UART_HandleTypeDef *h, unsigned char *c, BOOL *bStop);
    static unsigned int serRead(UART_HandleTypeDef *h, LPVOID buf, DWORD len);
    static void serClose(UART_HandleTypeDef *h);
    static HANDLE serOpen( char *port, int mode);
    CSerial();
    virtual ~CSerial();
    /* Mode definitions */
    enum { serial_115200_8n1=0, serial_57600_8n1, serial_19200_8n1, serial_9600_8n1, serial_max };

private:
    char serGetLastError();
    /* Error definitions */
    #define SERIAL_ERROR_CREATING_DCB 1
    #define SERIAL_ERROR_SETTING_BAUDRATE 2 
    #define SERIAL_ERROR_OPENING_PORT 3
};
