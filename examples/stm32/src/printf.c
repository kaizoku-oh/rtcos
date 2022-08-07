/* 
 **************************************************************************************************
 *
 * @file    : printf.c
 * @author  : Bayrem GHARSELLAOUI
 * @date    : October 2021
 * @brief   : Redirect printf to UART source file
 * 
 **************************************************************************************************
 */

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include <unistd.h>
#include <stdio.h>
#include "stm32f1xx_hal.h"

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
#define PRINTF_USART_INSTANCE                    USART1
#define PRINTF_USART_BAUDRATE                    115200
#define PRINTF_USART_PORT                        GPIOA
#define PRINTF_USART_TX_PIN                      GPIO_PIN_9
#define PRINTF_USART_RX_PIN                      GPIO_PIN_10

/*-----------------------------------------------------------------------------------------------*/
/* Macros                                                                                        */
/*-----------------------------------------------------------------------------------------------*/
#define PRINTF_GPIO_CLK_ENABLE()                 __HAL_RCC_GPIOA_CLK_ENABLE()
#define PRINTF_USART_CLK_ENABLE()                __HAL_RCC_USART1_CLK_ENABLE()

/*-----------------------------------------------------------------------------------------------*/
/* Private variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/
static UART_HandleTypeDef stUsartHandle;


/*-----------------------------------------------------------------------------------------------*/
/* Exported functions                                                                            */
/*-----------------------------------------------------------------------------------------------*/
/** ***********************************************************************************************
  * @brief      Configure UART
  * @return     Nothing
  ********************************************************************************************** */
void printf_init(void)
{
  stUsartHandle.Instance = PRINTF_USART_INSTANCE;
  stUsartHandle.Init.BaudRate = PRINTF_USART_BAUDRATE;
  stUsartHandle.Init.WordLength = UART_WORDLENGTH_8B;
  stUsartHandle.Init.StopBits = UART_STOPBITS_1;
  stUsartHandle.Init.Parity = UART_PARITY_NONE;
  stUsartHandle.Init.Mode = UART_MODE_TX_RX;
  stUsartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  stUsartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&stUsartHandle);
}

/** ***********************************************************************************************
  * @brief      Initialize UART GPIOs and interrupt, called from HAL_Init()
  * @param      pstHandle pointer to UART handle structure as defined in ::UART_HandleTypeDef
  * @return     Nothing
  ********************************************************************************************** */
void HAL_UART_MspInit(UART_HandleTypeDef *pstHandle)
{
  GPIO_InitTypeDef stGpioInit = {0};

  if(PRINTF_USART_INSTANCE == pstHandle->Instance)
  {
    PRINTF_USART_CLK_ENABLE();
    PRINTF_GPIO_CLK_ENABLE();
    stGpioInit.Pin = PRINTF_USART_TX_PIN;
    stGpioInit.Mode = GPIO_MODE_AF_PP;
    stGpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PRINTF_USART_PORT, &stGpioInit);
    stGpioInit.Pin = PRINTF_USART_RX_PIN;
    stGpioInit.Mode = GPIO_MODE_INPUT;
    stGpioInit.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(PRINTF_USART_PORT, &stGpioInit);
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
}

/** ***********************************************************************************************
  * @brief      Override the standard printf function to output on UART
  * @param      s32Fd File descriptor
  * @param      pcString string to print
  * @param      s32Length string length
  * @return     Length of the printed string
  ********************************************************************************************** */
int _write(int s32Fd, char *pcString, int s32Length)
{
  int s32RetVal;

  if((STDOUT_FILENO == s32Fd) || (STDERR_FILENO == s32Fd))
  {
    s32RetVal = (HAL_OK == HAL_UART_Transmit(&stUsartHandle,
                                             (uint8_t *)pcString,
                                             (s32Length * sizeof(uint8_t)),
                                             HAL_MAX_DELAY))
                ?(s32Length * sizeof(uint8_t))
                :-1;
  }
  else
  {
    s32RetVal = -1;
  }
  return s32RetVal;
}

/** ***********************************************************************************************
  * @brief      UART ISR function
  * @return     Nothing
  ********************************************************************************************** */
void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&stUsartHandle);
}