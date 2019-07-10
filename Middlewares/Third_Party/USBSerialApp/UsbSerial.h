/****************************************************************************
 *  Header file for handling inputs from Bt device
 *  2017.11.06 Created by JY Koh
*****************************************************************************/

#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#define LPVOID  uint8_t *
#define DWORD   unsigned int
#define WORD    unsigned short
#define BYTE    unsigned char
#define BOOL    int

#ifdef __cplusplus
extern "C" {
#endif

int	get_USBcommand(BYTE *buf, BYTE *len);
int	put_USBresponse(BYTE *buf, BYTE length);


extern	void debuglog_wr_frame(const char *str, unsigned char *buf, BYTE buflen);
#ifdef __cplusplus
}
#endif
