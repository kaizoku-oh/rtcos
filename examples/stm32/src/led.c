/* 
 **************************************************************************************************
 *
 * @file    : led.c
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.3.0
 * @brief   : STM32 bluepill LED BSP source file
 * 
 **************************************************************************************************
 */

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
#define LED_PIN                                  GPIO_PIN_12
#define LED_GPIO_PORT                            GPIOB

/*-----------------------------------------------------------------------------------------------*/
/* Macros                                                                                        */
/*-----------------------------------------------------------------------------------------------*/
#define LED_GPIO_CLK_ENABLE()                    __HAL_RCC_GPIOB_CLK_ENABLE()

/*-----------------------------------------------------------------------------------------------*/
/* Exported functions                                                                            */
/*-----------------------------------------------------------------------------------------------*/
/** ***********************************************************************************************
  * @brief      Configure LED GPIO
  * @return     Nothing
  ********************************************************************************************** */
void led_init(void)
{
  GPIO_InitTypeDef stGpioInit;

  LED_GPIO_CLK_ENABLE();
  stGpioInit.Pin = LED_PIN;
  stGpioInit.Mode = GPIO_MODE_OUTPUT_PP;
  stGpioInit.Pull = GPIO_PULLUP;
  stGpioInit.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(LED_GPIO_PORT, &stGpioInit);
}

/** ***********************************************************************************************
  * @brief      Turn on LED
  * @return     Nothing
  ********************************************************************************************** */
void led_on(void)
{
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET);
}

/** ***********************************************************************************************
  * @brief      Turn off LED
  * @return     Nothing
  ********************************************************************************************** */
void led_off(void)
{
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_RESET);
}

/** ***********************************************************************************************
  * @brief      Toggle LED
  * @return     Nothing
  ********************************************************************************************** */
void led_toggle(void)
{
  HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
}