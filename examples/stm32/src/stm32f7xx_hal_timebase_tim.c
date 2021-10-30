/* 
 **************************************************************************************************
 *
 * @file    : stm32f7xx_hal_timebase_tim.c
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.3.0
 * @date    : May 2021
 * @brief   : HAL timebase
 * 
 **************************************************************************************************
 */

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_tim.h"

/*-----------------------------------------------------------------------------------------------*/
/* Private variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/
TIM_HandleTypeDef stTimerHandle;

/*-----------------------------------------------------------------------------------------------*/
/* Exported functions                                                                            */
/*-----------------------------------------------------------------------------------------------*/
/** ***********************************************************************************************
  * @brief      Configure TIM2 as a time base source
  *             The time source is configured  to have 1ms time base with a dedicated
  *             Tick interrupt priority.
  * @param      u32TickPriority Tick interrupt priority
  * @return     HAL status as defined in ::HAL_StatusTypeDef
  ********************************************************************************************** */
HAL_StatusTypeDef HAL_InitTick(uint32_t u32TickPriority)
{
  RCC_ClkInitTypeDef stClkConfig;
  uint32_t u32TimerClock = 0;
  uint32_t u32PrescalerValue = 0;
  uint32_t u32FlashLatency;

  /* Configure the TIM2 IRQ priority */
  HAL_NVIC_SetPriority(TIM2_IRQn, u32TickPriority ,0);
  /* Enable the TIM2 global Interrupt */
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
  /* Enable TIM2 clock */
  __HAL_RCC_TIM2_CLK_ENABLE();
  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&stClkConfig, &u32FlashLatency);
  /* Compute TIM2 clock */
  u32TimerClock = 2*HAL_RCC_GetPCLK1Freq();
  /* Compute the prescaler value to have TIM2 counter clock equal to 1MHz */
  u32PrescalerValue = (uint32_t)((u32TimerClock / 1000000U) - 1U);
  /* Initialize TIM2 */
  stTimerHandle.Instance = TIM2;
  /* Initialize TIMx peripheral as follow:
       Period = [(TIM2CLK/1000) - 1]. to have a (1/1000) s time base.
       Prescaler = (u32TimerClock/1000000 - 1) to have a 1MHz counter clock.
       ClockDivision = 0
       Counter direction = Up
  */
  stTimerHandle.Init.Period = (1000000U / 1000U) - 1U;
  stTimerHandle.Init.Prescaler = u32PrescalerValue;
  stTimerHandle.Init.ClockDivision = 0;
  stTimerHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
  if(HAL_TIM_Base_Init(&stTimerHandle) == HAL_OK)
  {
    /* Start the TIM time Base generation in interrupt mode */
    return HAL_TIM_Base_Start_IT(&stTimerHandle);
  }
  /* Return function status */
  return HAL_ERROR;
}

/** ***********************************************************************************************
  * @brief      Suspend Tick increment
  * @return     Nothing
  ********************************************************************************************** */
void HAL_SuspendTick(void)
{
  /* Disable TIM2 update Interrupt */
  __HAL_TIM_DISABLE_IT(&stTimerHandle, TIM_IT_UPDATE);
}

/** ***********************************************************************************************
  * @brief      Resume Tick increment
  * @return     Nothing
  ********************************************************************************************** */
void HAL_ResumeTick(void)
{
  /* Enable TIM2 Update interrupt */
  __HAL_TIM_ENABLE_IT(&stTimerHandle, TIM_IT_UPDATE);
}

/** ***********************************************************************************************
  * @brief      Period elapsed callback
  * @param      pstHandle pionter on timer handle
  * @return     Nothing
  ********************************************************************************************** */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *pstHandle)
{
  if(pstHandle->Instance == TIM2)
  {
    HAL_IncTick();
  }
}

/** ***********************************************************************************************
  * @brief      Handle TIM2 global interrupt
  * @return     Nothing
  ********************************************************************************************** */
void TIM2_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&stTimerHandle);
}
