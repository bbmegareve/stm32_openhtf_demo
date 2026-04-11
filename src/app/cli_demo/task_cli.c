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

/* Simple circular buffer for received characters.
 */
#define CLI_RX_BUFFER_SIZE 256
static uint8_t rxBuffer[CLI_RX_BUFFER_SIZE];
static volatile uint16_t rxBufferHead = 0;
static volatile uint16_t rxBufferTail = 0;

static inline uint8_t CLI_RxBuffer_Empty(void)
{
    return (rxBufferHead == rxBufferTail);
}

static inline uint8_t CLI_RxBuffer_Full(void)
{
    return (((rxBufferHead + 1u) % CLI_RX_BUFFER_SIZE) == rxBufferTail);
}

static inline void CLI_RxBuffer_Put(uint8_t c)
{
    if (!CLI_RxBuffer_Full()) {
        rxBuffer[rxBufferHead] = c;
        rxBufferHead = (rxBufferHead + 1u) % CLI_RX_BUFFER_SIZE;
    }
}

static inline uint8_t CLI_RxBuffer_Get(void)
{
    uint8_t c = 0;

    if (!CLI_RxBuffer_Empty()) {
        c = rxBuffer[rxBufferTail];
        rxBufferTail = (rxBufferTail + 1u) % CLI_RX_BUFFER_SIZE;
    }

    return c;
}

/**
  * @brief  UART Receive Complete Callback
  * @param  huart: UART handle
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (huart == &hcom_uart[COM1]) {
        /* Push received character into the circular buffer */
        CLI_RxBuffer_Put(rxChar);

        /* Notify the CLI task that a character is available */
        vTaskNotifyGiveFromISR(cliHandle, &xHigherPriorityTaskWoken);

        /* Immediately restart reception for next character */
        HAL_UART_Receive_IT(&hcom_uart[COM1], &rxChar, 1);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
        /* Wait until the ISR notifies that a character is available. */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* Drain all available characters (notification may be coalesced). */
        while (!CLI_RxBuffer_Empty()) {
            localChar = CLI_RxBuffer_Get();
            CLI_ProcessInput((char)localChar);
        }
    }
}
