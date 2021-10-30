/* 
 **************************************************************************************************
 *
 * @file    : button.c
 * @author  : Bayrem GHARSELLAOUI
 * @date    : October 2021
 * @brief   : STM32 bluepill button BSP source file
 * 
 **************************************************************************************************
 */

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include <stdbool.h>
#include "stm32f1xx_hal.h"
#include "button.h"

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
#define BUTTON_PIN                               GPIO_PIN_13
#define BUTTON_GPIO_PORT                         GPIOC

/*-----------------------------------------------------------------------------------------------*/
/* Macros                                                                                        */
/*-----------------------------------------------------------------------------------------------*/
#define BUTTON_GPIO_CLK_ENABLE()                 __HAL_RCC_GPIOC_CLK_ENABLE()

/*-----------------------------------------------------------------------------------------------*/
/* Private variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/
static button_callback_t pfButtonCallback;

/*-----------------------------------------------------------------------------------------------*/
/* Exported functions                                                                            */
/*-----------------------------------------------------------------------------------------------*/
/** ***********************************************************************************************
  * @brief      Configure button GPIO and interrupt
  * @return     Nothing
  ********************************************************************************************** */
void button_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  BUTTON_GPIO_CLK_ENABLE();
  GPIO_InitStruct.Pin = BUTTON_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(BUTTON_GPIO_PORT, &GPIO_InitStruct);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/** ***********************************************************************************************
  * @brief      Register button callack function
  * @param      pfCallback callback function as defined in ::button_callback_t
  * @return     Nothing
  ********************************************************************************************** */
void button_register_callback(button_callback_t pfCallback)
{
  if(pfCallback)
  {
    pfButtonCallback = pfCallback;
  }
}

/** ***********************************************************************************************
  * @brief      Get button state
  * @return     true if button is pressed else false
  ********************************************************************************************** */
bool button_is_pressed(void)
{
  bool result;

  result = (GPIO_PIN_SET == HAL_GPIO_ReadPin(BUTTON_GPIO_PORT, BUTTON_PIN))?true:false;
  return result;
}

/** ***********************************************************************************************
  * @brief      Button GPIO callback
  * @param      u16GpioPin button GPIO pin
  * @return     Nothing
  ********************************************************************************************** */
void HAL_GPIO_EXTI_Callback(uint16_t u16GpioPin)
{
  if(BUTTON_PIN == u16GpioPin)
  {
    if(pfButtonCallback)
    {
      pfButtonCallback();
    }
  }
}

/** ***********************************************************************************************
  * @brief      External interrupt ISR function
  * @return     Nothing
  ********************************************************************************************** */
void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(BUTTON_PIN);
}