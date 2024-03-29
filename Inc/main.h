/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx_hal.h"
#include <stdio.h>
/* Includes ------------------------------------------------------------------*/

/* Configuration **************************************************************/
#define DMA_BUF_SIZE        32      /* DMA circular buffer size in bytes */
#define DMA_TIMEOUT_MS      10      /* DMA Timeout duration in msec */
/******************************************************************************/

/* Type definitions ----------------------------------------------------------*/
typedef struct
{
    volatile unsigned char  flag;     /* Timeout event flag */
    unsigned short timer;             /* Timeout duration in msec */
    unsigned short prevCNDTR;         /* Holds previous value of DMA_CNDTR */
} DMA_Event_t;

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private defines -----------------------------------------------------------*/

#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void Error_Handler(void);

#ifdef __cplusplus
extern "C" {
#endif

int Uart1_GetChar(unsigned char *, int);
int Uart3_GetChar(unsigned char *, int);
int Uart4_GetChar(unsigned char *, int);
int Uart5_GetChar(unsigned char *, int);
int Uart6_GetChar(unsigned char *, int);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
