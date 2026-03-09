/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : FreeRTOS applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
uint32_t MyBufferTask01[ 128 ];
osStaticThreadDef_t MycontrolBlocTask01;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_mem = &MyBufferTask01[0],
  .stack_size = sizeof(MyBufferTask01),
  .cb_mem = &MycontrolBlocTask01,
  .cb_size = sizeof(MycontrolBlocTask01),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for cli */
osThreadId_t cliHandle;
uint32_t bufferTaskCli[ 128 ];
osStaticThreadDef_t controlBlocTaskCli;
const osThreadAttr_t cli_attributes = {
  .name = "cli",
  .stack_mem = &bufferTaskCli[0],
  .stack_size = sizeof(bufferTaskCli),
  .cb_mem = &controlBlocTaskCli,
  .cb_size = sizeof(controlBlocTaskCli),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for can */
osThreadId_t canHandle;
uint32_t bufferTaskCan[ 128 ];
osStaticThreadDef_t controlBlocCan;
const osThreadAttr_t can_attributes = {
  .name = "can",
  .stack_mem = &bufferTaskCan[0],
  .stack_size = sizeof(bufferTaskCan),
  .cb_mem = &controlBlocCan,
  .cb_size = sizeof(controlBlocCan),
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of cli */
  cliHandle = osThreadNew(TaskCli, NULL, &cli_attributes);

  /* creation of can */
  canHandle = osThreadNew(TaskCan, NULL, &can_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_StartDefaultTask */
/**
* @brief Function implementing the defaultTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDefaultTask */
__weak void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN defaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);
  }
  /* USER CODE END defaultTask */
}

/* USER CODE BEGIN Header_TaskCli */
/**
* @brief Function implementing the cli thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_TaskCli */
__weak void TaskCli(void *argument)
{
  /* USER CODE BEGIN cli */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END cli */
}

/* USER CODE BEGIN Header_TaskCan */
/**
* @brief Function implementing the can thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_TaskCan */
__weak void TaskCan(void *argument)
{
  /* USER CODE BEGIN can */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END can */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

