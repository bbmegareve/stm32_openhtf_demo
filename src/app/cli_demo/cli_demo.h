/**
  ******************************************************************************
  * @file    cli_demo.h
  * @brief   CLI demo module header
  ******************************************************************************
  */

#ifndef __CLI_DEMO_H
#define __CLI_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"

/* Exported functions --------------------------------------------------------*/
void CLI_Init(void);
void CLI_ProcessInput(char c);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_DEMO_H */
