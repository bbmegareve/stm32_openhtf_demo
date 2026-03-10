/**
  ******************************************************************************
  * @file    task_cli.c
  * @brief   CLI task implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "app_freertos.h"
#include "cli_demo.h"
#include "stm32c0xx_nucleo.h"
#include <stdio.h>

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef hcom_uart[COMn];

/* Private variables ---------------------------------------------------------*/
static uint8_t rxChar;
static volatile uint8_t rxReady = 0;

/**
  * @brief  UART Receive Complete Callback
  * @param  huart: UART handle
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &hcom_uart[COM1]) {
        /* Set flag to indicate character received */
        rxReady = 1;
        /* Immediately restart reception for next character */
        HAL_UART_Receive_IT(&hcom_uart[COM1], &rxChar, 1);
    }
}

/**
  * @brief  Function implementing the cli thread.
  * @param  argument: Not used
  * @retval None
  */
void TaskCli(void *argument)
{
    uint8_t localChar;

    /* Small delay to ensure UART is fully initialized */
    osDelay(100);

    /* Initialize CLI */
    CLI_Init();
    
    /* Print a test message to verify UART TX is working */
    printf("\r\n*** UART Echo Test - Type something ***\r\n");
    
    /* Start receiving first character via interrupt */
    HAL_UART_Receive_IT(&hcom_uart[COM1], &rxChar, 1);
    
    /* Infinite loop */
    for(;;) {
        /* Check if a character was received */
        if (rxReady) {
            /* Copy character locally and clear flag immediately */
            taskENTER_CRITICAL();
            localChar = rxChar;
            rxReady = 0;
            taskEXIT_CRITICAL();
            
            /* Process the received character */
            CLI_ProcessInput((char)localChar);
        }
        
        /* Yield to other tasks */
        osDelay(10);
    }
}
