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
#include "stm32c0xx_hal.h"
#include "stm32c0xx_ll_adc.h" /* for LL_ADC_SetCommonPathInternalCh etc. */
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

/* ADC is configured by CubeMX in Core/Src/adc.c; handle declared there */
#include "adc.h" /* for extern hadc1 */

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

/* additional command prototypes */
static BaseType_t prvUIDCommand(char *pcWriteBuffer,
                                size_t xWriteBufferLen,
                                const char *pcCommandString);
static BaseType_t prvDevInfoCommand(char *pcWriteBuffer,
                                    size_t xWriteBufferLen,
                                    const char *pcCommandString);
static BaseType_t prvTempCommand(char *pcWriteBuffer,
                                 size_t xWriteBufferLen,
                                 const char *pcCommandString);

static const CLI_Command_Definition_t xUIDCommand =
{
    "uid",
    "\r\nuid:\r\n  Print unique device ID (96‑bit)\r\n\r\n",
    prvUIDCommand,
    0
};

static const CLI_Command_Definition_t xDevInfoCommand =
{
    "devinfo",
    "\r\ndevinfo:\r\n  Show device/revision codes and flash size\r\n\r\n",
    prvDevInfoCommand,
    0
};

static const CLI_Command_Definition_t xTempCommand =
{
    "temp",
    "\r\ntemp:\r\n  Read internal temperature sensor (°C)\r\n\r\n",
    prvTempCommand,
    0
};

/**
  * @brief  Initialize CLI module
  * @retval None
  */
void CLI_Init(void)
{
    /* Register commands */
    FreeRTOS_CLIRegisterCommand(&xVersionCommand);
    FreeRTOS_CLIRegisterCommand(&xUIDCommand);
    FreeRTOS_CLIRegisterCommand(&xDevInfoCommand);
    FreeRTOS_CLIRegisterCommand(&xTempCommand);
    
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

/*------------------------------------------------------------------*/
/* Helper/utility functions                                         */
/* ADC is initialised by MX_ADC1_Init() in main.c */

/* Command handlers ----------------------------------------------------------*/

static BaseType_t prvUIDCommand(char *pcWriteBuffer,
                                size_t xWriteBufferLen,
                                const char *pcCommandString)
{
    (void)pcCommandString;
    snprintf(pcWriteBuffer, xWriteBufferLen,
             "\r\nUID = %08lX-%08lX-%08lX\r\n\r\n",
             HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2());
    return pdFALSE;
}

static BaseType_t prvDevInfoCommand(char *pcWriteBuffer,
                                    size_t xWriteBufferLen,
                                    const char *pcCommandString)
{
    uint32_t id  = HAL_GetDEVID();
    uint32_t rev = HAL_GetREVID();
    uint16_t fsz = *(uint16_t*)FLASHSIZE_BASE;
    (void)pcCommandString;

    snprintf(pcWriteBuffer, xWriteBufferLen,
             "\r\nDevID=0x%03lX Rev=0x%03lX Flash=%uKB\r\n\r\n",
             id, rev, fsz);
    return pdFALSE;
}

/* calibration addresses for temp sensor (unused since ADC is disabled) */
#define TS_CAL1_TEMP   30u
#define TS_CAL2_TEMP   110u
#define TS_CAL1_ADDR   ((uint32_t)0x1FFF75A8U)
#define TS_CAL2_ADDR   ((uint32_t)0x1FFF75CAU)

static BaseType_t prvTempCommand(char *pcWriteBuffer,
                                 size_t xWriteBufferLen,
                                 const char *pcCommandString)
{
    uint32_t vref_raw = 0;
    uint32_t ts_raw = 0;
    (void)pcCommandString;

    /* Enable both VREFINT and temperature sensor internal paths */
    LL_ADC_SetCommonPathInternalCh(ADC1_COMMON,
        LL_ADC_PATH_INTERNAL_TEMPSENSOR | LL_ADC_PATH_INTERNAL_VREFINT);

    ADC_ChannelConfTypeDef sConfig = {0};

    /* Measure VREFINT first to compute actual Vdda (mV) */
    sConfig.Channel = ADC_CHANNEL_VREFINT;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "\r\nADC config error\r\n\r\n");
        return pdFALSE;
    }

    /* short settling time for internal path */
    HAL_Delay(10);

    /* take several samples and average to reduce noise */
    const int samples = 8;
    uint32_t acc = 0;
    for (int i = 0; i < samples; ++i) {
        if (HAL_ADC_Start(&hadc1) != HAL_OK) {
            snprintf(pcWriteBuffer, xWriteBufferLen,
                     "\r\nADC start error\r\n\r\n");
            return pdFALSE;
        }
        if (HAL_ADC_PollForConversion(&hadc1, 20) == HAL_OK) {
            acc += HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);
        HAL_Delay(1);
    }
    vref_raw = acc / samples;

    /* Compute Vdda in mV using factory VREFINT calibration */
    uint32_t vdda_mv = 0;
    if (vref_raw == 0) {
        /* protect against divide-by-zero */
        vdda_mv = VREFINT_CAL_VREF;
    } else {
        vdda_mv = __LL_ADC_CALC_VREFANALOG_VOLTAGE(vref_raw, LL_ADC_RESOLUTION_12B);
    }

    /* Clamp Vdda to a realistic range (2.0V - 4.5V). If out-of-range, fall back
       to calibration nominal value to avoid absurd temperatures. */
    if (vdda_mv < 2000U || vdda_mv > 4500U) {
        vdda_mv = VREFINT_CAL_VREF; /* 3000 mV nominal calibration */
    }

    /* Now measure the temperature sensor */
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "\r\nADC config error\r\n\r\n");
        return pdFALSE;
    }

    HAL_Delay(10);
    acc = 0;
    for (int i = 0; i < samples; ++i) {
        if (HAL_ADC_Start(&hadc1) != HAL_OK) {
            snprintf(pcWriteBuffer, xWriteBufferLen,
                     "\r\nADC start error\r\n\r\n");
            return pdFALSE;
        }
        if (HAL_ADC_PollForConversion(&hadc1, 20) == HAL_OK) {
            acc += HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);
        HAL_Delay(1);
    }
    ts_raw = acc / samples;

    /* Read factory calibration raw value (ADC code at 30°C, Vref+ = 3.0V) */
    uint32_t calraw = (uint32_t)(*TEMPSENSOR_CAL1_ADDR);

    /* Convert calibration raw (digital) into mV at calibration Vref (3000 mV) */
    uint32_t ts_cal_mv = (calraw * VREFINT_CAL_VREF) / 4095U;

    /* Use LL helper macro to compute temperature (units: °C) */
    const int32_t slope_uv_per_deg = 4300; /* typical Avg_Slope (uV/°C) */
    int32_t temp = __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS(
                    slope_uv_per_deg, /* uV/°C */
                    ts_cal_mv,         /* mV */
                    TEMPSENSOR_CAL1_TEMP,
                    vdda_mv,           /* mV */
                    ts_raw,
                    LL_ADC_RESOLUTION_12B);

    /* debug */
    printf("vref_raw=0x%X vdda=%lumV calraw=0x%X ts_raw=0x%X\r\n",
           (unsigned)vref_raw, (unsigned long)vdda_mv, (unsigned)calraw, (unsigned)ts_raw);

    snprintf(pcWriteBuffer, xWriteBufferLen,
             "\r\nTemperature: %ld C (raw=0x%lX)\r\n\r\n",
             (long)temp, (long)ts_raw);
    return pdFALSE;
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
