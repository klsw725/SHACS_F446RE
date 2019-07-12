/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart4;
UART_HandleTypeDef huart5;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart1_rx;

//#define	SHACS_BRD
#define	NUCLEO_446RE_LED_TEST
#define	WIFI_ON

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE
{
	HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
	return ch;
}

osThreadId defaultTaskHandle;
osThreadId Task1Handle;
osThreadId Task2Handle;
osThreadId Task3Handle;
osThreadId Task5Handle;
osThreadId Task6Handle;
osThreadId Task7Handle;
osThreadId Task8Handle;
  
osTimerId ZWhostAppTimerHandle;
osSemaphoreId ZwCommandQueueHandle;
osSemaphoreId ZwTransmitQueueHandle;
osSemaphoreId BtCommandQueueHandle;
osSemaphoreId UsbCommandQueueHandle;
osSemaphoreId WifiCommandQueueHandle;
osSemaphoreId BtResponseQueueHandle;
osSemaphoreId UsbResponseQueueHandle;
osSemaphoreId WifiResponseQueueHandle;

//#define		_USE_SEMAPHOE_HANDLING_UART
#ifdef	_USE_SEMAPHOE_HANDLING_UART
osSemaphoreId U1commandQueueHandle;
osSemaphoreId U3commandQueueHandle;
osSemaphoreId U4commandQueueHandle;
osSemaphoreId U5commandQueueHandle;
#endif

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_UART4_Init(void);
static void MX_UART5_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USART6_UART_Init(void);
//void StartDefaultTask(void const * argument);
void CallbackFuntion(void const * argument);
void LedTask1(void const * argument);

extern	void APIProcTask(void const * argument);
extern 	void DispatchProcTask(void const * argument);
extern 	void BtSerialApp(void const * argument);
extern 	void UsbSerialApp(void const * argument);
#ifdef	WIFI_ON
extern 	void WifiSerialApp(void const * argument);
#endif
extern 	void ZWApp_main(void const * argument);


#define	UART1_RX_BUF_SIZE	64

uint8_t	Uart1_Rx_buffer[UART1_RX_BUF_SIZE];
uint8_t	Widx1 = 0;
uint8_t	Ridx1 = 0;
/* DMA Timeout event structure
 * Note: prevCNDTR initial value must be set to maximum size of DMA buffer!
*/
DMA_Event_t dma_uart_rx = {0,0,DMA_BUF_SIZE};

uint8_t dma_rx_buf[DMA_BUF_SIZE];       /* Circular buffer for DMA */
uint8_t data[DMA_BUF_SIZE] = {'\0'};    /* Data buffer that contains newly received data */


#define	UART3_RX_BUF_SIZE	32

uint8_t	Uart3_Rx_buffer[UART3_RX_BUF_SIZE];
uint8_t	Widx3 = 0;
uint8_t	Ridx3 = 0;

#define	UART4_RX_BUF_SIZE	32

uint8_t	Uart4_Rx_buffer[UART4_RX_BUF_SIZE];
uint8_t	Widx4 = 0;
uint8_t	Ridx4 = 0;

#define	UART5_RX_BUF_SIZE	32

uint8_t	Uart5_Rx_buffer[UART5_RX_BUF_SIZE];
uint8_t	Widx5 = 0;
uint8_t	Ridx5 = 0;

#define	UART6_RX_BUF_SIZE	32

uint8_t	Uart6_Rx_buffer[UART6_RX_BUF_SIZE];
uint8_t	Widx6 = 0;
uint8_t	Ridx6 = 0;

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
#ifdef WIFI_ON
  MX_UART4_Init();
#endif
  MX_UART5_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_USART6_UART_Init();
  
  // LED
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);


  // OTG Disable ????
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
  
  /* USER CODE BEGIN 2 */
  __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);
#ifdef	WIFI_ON
  __HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE);
#endif
  __HAL_UART_ENABLE_IT(&huart5, UART_IT_RXNE);
  __HAL_UART_ENABLE_IT(&huart6, UART_IT_RXNE);
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */
  
  //  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 1);

  /* Create the semaphores(s) */
  /* definition and creation of commandQueue */
  osSemaphoreDef(ZwCommandQueue);
  ZwCommandQueueHandle = osSemaphoreCreate(osSemaphore(ZwCommandQueue), 1);

  /* definition and creation of transmitQueue */
  osSemaphoreDef(ZwTransmitQueue);
  ZwTransmitQueueHandle = osSemaphoreCreate(osSemaphore(ZwTransmitQueue), 1);

  /* definition and creation of BtcommandQueue */
  osSemaphoreDef(BtCommandQueue);
  BtCommandQueueHandle = osSemaphoreCreate(osSemaphore(BtCommandQueue), 1);

  /* definition and creation of UsbcommandQueue */
  osSemaphoreDef(UsbCommandQueue);
  UsbCommandQueueHandle = osSemaphoreCreate(osSemaphore(UsbCommandQueue), 1);

  /* definition and creation of WificommandQueue */
  osSemaphoreDef(WifiCommandQueue);
  WifiCommandQueueHandle = osSemaphoreCreate(osSemaphore(WifiCommandQueue), 1);

  /* definition and creation of BtresponseQueue */
  osSemaphoreDef(BtResponseQueue);
  BtResponseQueueHandle = osSemaphoreCreate(osSemaphore(BtResponseQueue), 1);

  /* definition and creation of UsbcommandQueue */
  osSemaphoreDef(UsbResponseQueue);
  UsbResponseQueueHandle = osSemaphoreCreate(osSemaphore(UsbResponseQueue), 1);

  /* definition and creation of WificommandQueue */
  osSemaphoreDef(WifiResponseQueue);
  WifiResponseQueueHandle = osSemaphoreCreate(osSemaphore(WifiResponseQueue), 1);

#ifdef	_USE_SEMAPHOE_HANDLING_UART
  /* definition and creation of U1commandQueue */
  osSemaphoreDef(U1commandQueue);
  U1commandQueueHandle = osSemaphoreCreate(osSemaphore(U1commandQueue), 1);

  /* definition and creation of U3commandQueue */
  osSemaphoreDef(U3commandQueue);
  U3commandQueueHandle = osSemaphoreCreate(osSemaphore(U3commandQueue), 1);

  /* definition and creation of U4commandQueue */
  osSemaphoreDef(U4commandQueue);
  U4commandQueueHandle = osSemaphoreCreate(osSemaphore(U4commandQueue), 1);

  /* definition and creation of U5commandQueue */
  osSemaphoreDef(U5commandQueue);
  U5commandQueueHandle = osSemaphoreCreate(osSemaphore(U5commandQueue), 1);
  
  /* definition and creation of U5commandQueue */
  osSemaphoreDef(U6commandQueue);
  U6commandQueueHandle = osSemaphoreCreate(osSemaphore(U6commandQueue), 1);
#endif
  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of ZWhostAppTimer */
  osTimerDef(ZWhostAppTimer, CallbackFuntion);
  ZWhostAppTimerHandle = osTimerCreate(osTimer(ZWhostAppTimer), osTimerPeriodic, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
//  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
//  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
    osThreadDef(Task1, DispatchProcTask, osPriorityAboveNormal, 0, 1024);
    Task1Handle = osThreadCreate(osThread(Task1), NULL);

    osThreadDef(Task2, LedTask1, osPriorityLow, 0, 128);
    Task2Handle = osThreadCreate(osThread(Task2), NULL);

    osThreadDef(Task3, APIProcTask, osPriorityHigh, 0, 1024);
    Task3Handle = osThreadCreate(osThread(Task3), NULL);

    osThreadDef(Task5, BtSerialApp, osPriorityBelowNormal, 0, 512);
    Task5Handle = osThreadCreate(osThread(Task5), NULL);

    osThreadDef(Task6, UsbSerialApp, osPriorityBelowNormal, 0, 512);
    Task6Handle = osThreadCreate(osThread(Task6), NULL);

  #ifdef	WIFI_ON
    osThreadDef(Task8, WifiSerialApp, osPriorityBelowNormal, 0, 512);
    Task8Handle = osThreadCreate(osThread(Task8), NULL);
  #endif

    osThreadDef(Task7, ZWApp_main, osPriorityNormal, 0, 1024);
    Task7Handle = osThreadCreate(osThread(Task7), NULL);

  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
   if(HAL_UART_Receive_DMA(&huart1, (uint8_t*)dma_rx_buf, DMA_BUF_SIZE) != HAL_OK)
  {
      Error_Handler();
  }

  /* Disable Half Transfer Interrupt */
  __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

#ifdef SHACS_BRD
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode 
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

#else
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode 
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}
#endif

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

 #ifdef	NUCLEO_446RE_LED_TEST
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
#endif
  
  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */
/** DMA Rx Complete AND DMA Rx Timeout function
 * Timeout event: generated after UART IDLE IT + DMA Timeout value
 * Scenarios:
 *  - Timeout event when previous event was DMA Rx Complete --> new data is from buffer beginning till (MAX-currentCNDTR)
 *  - Timeout event when previous event was Timeout event   --> buffer contains old data, new data is in the "middle": from (MAX-previousCNDTR) till (MAX-currentCNDTR)
 *  - DMA Rx Complete event when previous event was DMA Rx Complete --> entire buffer holds new data
 *  - DMA Rx Complete event when previous event was Timeout event   --> buffer entirely filled but contains old data, new data is from (MAX-previousCNDTR) till MAX
 * Remarks:
 *  - If there is no following data after DMA Rx Complete, the generated IDLE Timeout has to be ignored!
 *  - When buffer overflow occurs, the following has to be performed in order not to lose data:
 *      (1): DMA Rx Complete event occurs, process first part of new data till buffer MAX.
 *      (2): In this case, the currentCNDTR is already decreased because of overflow.
 *              However, previousCNDTR has to be set to MAX in order to signal for upcoming Timeout event that new data has to be processed from buffer beginning.
 *      (3): When many overflows occur, simply process DMA Rx Complete events (process entire DMA buffer) until Timeout event occurs.
 *      (4): When there is no more overflow, Timeout event occurs, process last part of data from buffer beginning till currentCNDTR.
*/

void DMA_Uart1(UART_HandleTypeDef *huart)
{
    uint16_t i, pos, start, length;
    uint16_t currCNDTR = __HAL_DMA_GET_COUNTER(huart->hdmarx);

	if (huart->Instance == USART1)
	{

		/* Ignore IDLE Timeout when the received characters exactly filled up the DMA buffer and DMA Rx Complete IT is generated, but there is no new character during timeout */
		if(dma_uart_rx.flag && currCNDTR == DMA_BUF_SIZE)
		{
			dma_uart_rx.flag = 0;
			return;
		}

		/* Determine start position in DMA buffer based on previous CNDTR value */
		start = (dma_uart_rx.prevCNDTR < DMA_BUF_SIZE) ? (DMA_BUF_SIZE - dma_uart_rx.prevCNDTR) : 0;

		if(dma_uart_rx.flag)    /* Timeout event */
		{
			/* Determine new data length based on previous DMA_CNDTR value:
			 *  If previous CNDTR is less than DMA buffer size: there is old data in DMA buffer (from previous timeout) that has to be ignored.
			 *  If CNDTR == DMA buffer size: entire buffer content is new and has to be processed.
			 */
			length = (dma_uart_rx.prevCNDTR < DMA_BUF_SIZE) ? (dma_uart_rx.prevCNDTR - currCNDTR) : (DMA_BUF_SIZE - currCNDTR);
			dma_uart_rx.prevCNDTR = currCNDTR;
			dma_uart_rx.flag = 0;
		}
		else                /* DMA Rx Complete event */
		{
			length = DMA_BUF_SIZE - start;
			dma_uart_rx.prevCNDTR = DMA_BUF_SIZE;
		}

		/* Copy and Process new data */
		for(i=0,pos=start; i<length; ++i,++pos)
		{
			Uart1_Rx_buffer[Widx1] = dma_rx_buf[pos];
			Widx1++;
			Widx1 = Widx1 % UART1_RX_BUF_SIZE;
		}
	}
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	char	ch;
	char	sema = 0;
    uint16_t i, pos, start, length;
    uint16_t currCNDTR;

	if (huart->Instance == USART1)
	{
	    currCNDTR = __HAL_DMA_GET_COUNTER(huart->hdmarx);
		/* Ignore IDLE Timeout when the received characters exactly filled up the DMA buffer and DMA Rx Complete IT is generated, but there is no new character during timeout */
		if(dma_uart_rx.flag && currCNDTR == DMA_BUF_SIZE)
		{
			dma_uart_rx.flag = 0;
			return;
		}

		/* Determine start position in DMA buffer based on previous CNDTR value */
		start = (dma_uart_rx.prevCNDTR < DMA_BUF_SIZE) ? (DMA_BUF_SIZE - dma_uart_rx.prevCNDTR) : 0;

		if(dma_uart_rx.flag)    /* Timeout event */
		{
			/* Determine new data length based on previous DMA_CNDTR value:
			 *  If previous CNDTR is less than DMA buffer size: there is old data in DMA buffer (from previous timeout) that has to be ignored.
			 *  If CNDTR == DMA buffer size: entire buffer content is new and has to be processed.
			 */
			length = (dma_uart_rx.prevCNDTR < DMA_BUF_SIZE) ? (dma_uart_rx.prevCNDTR - currCNDTR) : (DMA_BUF_SIZE - currCNDTR);
			dma_uart_rx.prevCNDTR = currCNDTR;
			dma_uart_rx.flag = 0;
		}
		else                /* DMA Rx Complete event */
		{
			length = DMA_BUF_SIZE - start;
			dma_uart_rx.prevCNDTR = DMA_BUF_SIZE;
		}

		/* Copy and Process new data */
		for(i=0,pos=start; i<length; ++i,++pos)
		{
			Uart1_Rx_buffer[Widx1] = dma_rx_buf[pos];
			Widx1++;
			Widx1 = Widx1 % UART1_RX_BUF_SIZE;
		}
		HAL_UART_Receive_DMA(&huart1, (uint8_t*)dma_rx_buf, DMA_BUF_SIZE);
	}
	else if (huart->Instance == USART3)
	{
		while (HAL_UART_Receive_IT(&huart3,(uint8_t *)&ch,1) == HAL_OK)
		{
			Uart3_Rx_buffer[Widx3] = ch;
#ifdef	_USE_SEMAPHOE_HANDLING_UART
			if (sema == 0 )
			{
				osSemaphoreWait(U3commandQueueHandle, 0);
				sema = 1;
			}
#endif
			Widx3++;
			Widx3 = Widx3 % UART3_RX_BUF_SIZE;
//			printf("I3[%02x]", ch);
		}
#ifdef	_USE_SEMAPHOE_HANDLING_UART
		if (sema != 0)
			osSemaphoreRelease(U3commandQueueHandle);
#endif
//		printf("\r\n");
	}
	else if (huart->Instance == UART4)
	{
		while (HAL_UART_Receive_IT(&huart4,(uint8_t *)&ch,1) == HAL_OK)
		{
			Uart4_Rx_buffer[Widx4] = ch;
#ifdef	_USE_SEMAPHOE_HANDLING_UART
			if (sema == 0 )
			{
				osSemaphoreWait(U4commandQueueHandle, 0);
				sema = 1;
			}
#endif
			Widx4++;
			Widx4 = Widx4 % UART4_RX_BUF_SIZE;
//			printf("I4[%02x]", ch);
		}
#ifdef	_USE_SEMAPHOE_HANDLING_UART
		if (sema != 0)
			osSemaphoreRelease(U4commandQueueHandle);
#endif
//		printf("\r\n");
	}
	else if (huart->Instance == UART5)
	{
		while (HAL_UART_Receive_IT(&huart5,&ch,1) == HAL_OK)
		{
			Uart5_Rx_buffer[Widx5] = ch;
#ifdef	_USE_SEMAPHOE_HANDLING_UART
			if (sema == 0 )
			{
				osSemaphoreWait(U5commandQueueHandle, 0);
				sema = 1;
			}
#endif
			Widx5++;
			Widx5 = Widx5 % UART5_RX_BUF_SIZE;
//			printf("I5[%02x]", ch);
		}
#ifdef	_USE_SEMAPHOE_HANDLING_UART
		if (sema != 0)
			osSemaphoreRelease(U5commandQueueHandle);
#endif
//		printf("\r\n");
	}
        else if (huart->Instance == USART6)
	{
		while (HAL_UART_Receive_IT(&huart6,&ch,1) == HAL_OK)
		{
			Uart6_Rx_buffer[Widx6] = ch;
#ifdef	_USE_SEMAPHOE_HANDLING_UART
			if (sema == 0 )
			{
				osSemaphoreWait(U6commandQueueHandle, 0);
				sema = 1;
			}
#endif
			Widx6++;
			Widx6 = Widx6 % UART6_RX_BUF_SIZE;
//			printf("I5[%02x]", ch);
		}
#ifdef	_USE_SEMAPHOE_HANDLING_UART
		if (sema != 0)
			osSemaphoreRelease(U6commandQueueHandle);
#endif
//		printf("\r\n");
	}
}


/* Error callback */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    Error_Handler();
}

int	Uart1_GetChar(unsigned char *ch, int timeout)
{
	int	rd;

	while (Widx1 == Ridx1)
	{
		if (--timeout > 0)
			osDelay(1);
		else
			return	0;
	}

	*ch = Uart1_Rx_buffer[Ridx1++];
	Ridx1 = Ridx1 % UART1_RX_BUF_SIZE;
	return	1;
}

int	Uart3_GetChar(unsigned char *ch, int timeout)
{
	int	rd;

	while (Widx3 == Ridx3)
	{
		if (--timeout > 0)
			osDelay(1);
		else
			return	0;
	}

	*ch = Uart3_Rx_buffer[Ridx3++];
	Ridx3 = Ridx3 % UART3_RX_BUF_SIZE;
	return	1;
}

int	Uart4_GetChar(unsigned char *ch, int timeout)
{
	int	rd;

	while (Widx4 == Ridx4)
	{
		if (--timeout > 0)
			osDelay(1);
		else
			return	0;
	}

	*ch = Uart4_Rx_buffer[Ridx4++];
	Ridx4 = Ridx4 % UART4_RX_BUF_SIZE;
	return	1;
}

int	Uart5_GetChar(unsigned char *ch, int timeout)
{
	int	rd;

	while (Widx5 == Ridx5)
	{
		if (--timeout > 0)
			osDelay(1);
		else
			return	0;
	}

	*ch = Uart5_Rx_buffer[Ridx5++];
	Ridx5 = Ridx5 % UART5_RX_BUF_SIZE;
	return	1;
}

int	Uart6_GetChar(unsigned char *ch, int timeout)
{
	int	rd;

	while (Widx6 == Ridx6)
	{
		if (--timeout > 0)
			osDelay(1);
		else
			return	0;
	}

	*ch = Uart5_Rx_buffer[Ridx5++];
	Ridx6 = Ridx6 % UART6_RX_BUF_SIZE;
	return	1;
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
//void StartDefaultTask(void const * argument)
//{
//
//  /* USER CODE BEGIN 5 */
//  /* Infinite loop */
//  for(;;)
//  {
//    osDelay(1);
//  }
//  /* USER CODE END 5 */ 
//}

/* CallbackFuntion function */
void CallbackFuntion(void const * argument)
{
  /* USER CODE BEGIN CallbackFuntion */
  //printf("Tmr:--> Out\r\n");
  /* USER CODE END CallbackFuntion */
}

/* LedTask function */
void LedTask1(void const * argument)
{

  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {

	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6);

#ifdef	  NUCLEO_446RE_LED_TEST
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
#endif
    osDelay(1000);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
