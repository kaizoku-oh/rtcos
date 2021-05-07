/* 
 **************************************************************************************************
 *
 * @file    : stm32f7xx_hal_msp.c
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.3.0
 * @date    : May 2021
 * @brief   : MSP Initialization and de-Initialization
 * 
 **************************************************************************************************
 */

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include "main.h"

/*-----------------------------------------------------------------------------------------------*/
/* Exported functions                                                                            */
/*-----------------------------------------------------------------------------------------------*/
/** ***********************************************************************************************
  * @brief      Initializes the Global MSP
  * @return     Nothing
  ********************************************************************************************** */
void HAL_MspInit(void)
{
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();
}

/** ***********************************************************************************************
  * @brief      Initialize UART
  * @param      pstHandle pointer on UART handle
  * @return     Nothing
  ********************************************************************************************** */
void HAL_UART_MspInit(UART_HandleTypeDef *pstHandle)
{
  GPIO_InitTypeDef stGpioInit = {0};

  if(pstHandle->Instance == USART3)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /* USART3 GPIO Configuration
         PD8 ------> USART3_TX
         PD9 ------> USART3_RX
    */
    stGpioInit.Pin = STLINK_UART_TX_PIN | STLINK_UART_RX_PIN;
    stGpioInit.Mode = GPIO_MODE_AF_PP;
    stGpioInit.Pull = GPIO_NOPULL;
    stGpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    stGpioInit.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &stGpioInit);
    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  }
}

/** ***********************************************************************************************
  * @brief      De-initialize UART
  * @param      pstHandle pointer on UART handle
  * @return     Nothing
  ********************************************************************************************** */
void HAL_UART_MspDeInit(UART_HandleTypeDef *pstHandle)
{
  if(pstHandle->Instance == USART3)
  {
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();
    /* USART3 GPIO Configuration
         PD8 ------> USART3_TX
         PD9 ------> USART3_RX
    */
    HAL_GPIO_DeInit(GPIOD, STLINK_UART_TX_PIN | STLINK_UART_RX_PIN);
    /* USART3 interrupt DeInit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  }
}
