/**
  ******************************************************************************
  * @file    cli_demo.c
  * @brief   CLI demo implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "cli_demo.h"
#include "FreeRTOS_CLI.h"
#include "version.h"
#include "stm32c0xx_nucleo.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define CLI_INPUT_BUFFER_SIZE   128
#define CLI_OUTPUT_BUFFER_SIZE  512

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef hcom_uart[];

/* Private variables ---------------------------------------------------------*/
static char cInputBuffer[CLI_INPUT_BUFFER_SIZE];
static uint16_t usInputIndex = 0;

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Safe UART print (blocking, for use outside ISR)
  * @param  str: String to print
  * @retval None
  */
static void CLI_Print(const char *str)
{
    HAL_UART_Transmit(&hcom_uart[0], (uint8_t*)str, strlen(str), 1000);
}

/* Command handlers ----------------------------------------------------------*/

/**
  * @brief  Version command handler
  * @param  pcWriteBuffer: Output buffer
  * @param  xWriteBufferLen: Output buffer length
  * @param  pcCommandString: Command string
  * @retval pdFALSE (command is complete)
  */
static BaseType_t prvVersionCommand(char *pcWriteBuffer, 
                                    size_t xWriteBufferLen, 
                                    const char *pcCommandString)
{
    (void)pcCommandString;
    
    snprintf(pcWriteBuffer, xWriteBufferLen,
             "\r\n%s %s\r\n"
             "Build: %s %s\r\n\r\n",
             FW_NAME, FW_VERSION,
             BUILD_DATE, BUILD_TIME);
    
    return pdFALSE;
}

/* Command definitions -------------------------------------------------------*/
static const CLI_Command_Definition_t xVersionCommand =
{
    "version",
    "\r\nversion:\r\n  Display firmware version and build information\r\n\r\n",
    prvVersionCommand,
    0 /* No parameters */
};

/**
  * @brief  Initialize CLI module
  * @retval None
  */
void CLI_Init(void)
{
    /* Register commands */
    FreeRTOS_CLIRegisterCommand(&xVersionCommand);
    
    /* Initialize input buffer */
    memset(cInputBuffer, 0, CLI_INPUT_BUFFER_SIZE);
    usInputIndex = 0;
    
    CLI_Print("\r\n===========================================\r\n");
    CLI_Print("  ");
    CLI_Print(FW_NAME);
    CLI_Print(" ");
    CLI_Print(FW_VERSION);
    CLI_Print("\r\n  Build: ");
    CLI_Print(BUILD_DATE);
    CLI_Print(" ");
    CLI_Print(BUILD_TIME);
    CLI_Print("\r\n===========================================\r\n");
    CLI_Print("Type 'help' for list of commands\r\n");
    CLI_Print("> ");
}

/**
  * @brief  Process a single input character
  * @param  c: Input character
  * @retval None
  */
void CLI_ProcessInput(char c)
{
    static char cOutputBuffer[CLI_OUTPUT_BUFFER_SIZE];
    BaseType_t xMoreDataToFollow;
    
    /* Handle backspace */
    if (c == '\b' || c == 0x7F) {
        if (usInputIndex > 0) {
            usInputIndex--;
            cInputBuffer[usInputIndex] = '\0';
            CLI_Print("\b \b"); /* Erase character on terminal */
        }
        return;
    }
    
    /* Echo character */
    HAL_UART_Transmit(&hcom_uart[0], (uint8_t*)&c, 1, 100);
    
    /* Handle carriage return / newline */
    if (c == '\r' || c == '\n') {
        CLI_Print("\r\n");
        
        /* Process command if buffer is not empty */
        if (usInputIndex > 0) {
            /* Null terminate the input string */
            cInputBuffer[usInputIndex] = '\0';
            
            /* Process the command */
            do {
                xMoreDataToFollow = FreeRTOS_CLIProcessCommand(
                    cInputBuffer,
                    cOutputBuffer,
                    CLI_OUTPUT_BUFFER_SIZE
                );
                
                /* Output the response */
                CLI_Print(cOutputBuffer);
                
            } while (xMoreDataToFollow != pdFALSE);
        }
        
        /* Reset input buffer */
        usInputIndex = 0;
        memset(cInputBuffer, 0, CLI_INPUT_BUFFER_SIZE);
        
        /* Print prompt */
        CLI_Print("> ");
    } else if (c >= 32 && c <= 126) {
         /* Handle regular characters */
        if (usInputIndex < (CLI_INPUT_BUFFER_SIZE - 1)) {
            cInputBuffer[usInputIndex] = c;
            usInputIndex++;
        }
    }
}
