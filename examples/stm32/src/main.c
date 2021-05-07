/* 
 **************************************************************************************************
 *
 * @file    : main.c
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.3.0
 * @date    : May 2021
 * @brief   : Nucleo F767ZI running RTCOS example
 * 
 **************************************************************************************************
 */

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "rtcos.h"

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
#define TASK_ID_PRIORITY_ONE                     (uint8_t)0
#define TASK_ID_PRIORITY_TWO                     (uint8_t)1
#define EVENT_PING                               (uint32_t)1
#define EVENT_PONG                               (uint32_t)2
#define EVENT_COMMON                             (uint32_t)3
#define SERIAL_BAUDRATE                          115200
#define HARDWARE_TIMER_PERIOD_IN_US              1000
#define SOFTWARE_TIMER_PERIOD_IN_MS              100

/*-----------------------------------------------------------------------------------------------*/
/* Private function prototypes                                                                   */
/*-----------------------------------------------------------------------------------------------*/
static void _system_clock_configure(void);
static void _gpio_init(void);
static void _uart_init(void);
static uint32_t _task_one_handler(uint32_t u32EventFlags, uint8_t u08MsgCount, void const *pvArg);
static uint32_t _task_two_handler(uint32_t u32EventFlags, uint8_t u08MsgCount, void const *pvArg);
static void _on_os_timer_expired(void const *pvArg);

/*-----------------------------------------------------------------------------------------------*/
/* Private variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/
static UART_HandleTypeDef stUartHandle;
static uint8_t u08OsTimerID;

/*-----------------------------------------------------------------------------------------------*/
/* Exported functions                                                                            */
/*-----------------------------------------------------------------------------------------------*/
/** ***********************************************************************************************
  * @brief      Application entry point
  * @return     Nothing
  ********************************************************************************************** */
int main(void)
{
  HAL_Init();
  _system_clock_configure();
  _gpio_init();
  _uart_init();

  rtcos_init();
  rtcos_register_task_handler(_task_one_handler, TASK_ID_PRIORITY_ONE, (void *)"TaskOne");
  rtcos_register_task_handler(_task_two_handler, TASK_ID_PRIORITY_TWO, (void *)"TaskTwo");
  rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, (uint32_t)0, FALSE);
  u08OsTimerID = rtcos_create_timer(RTCOS_TIMER_PERIODIC, _on_os_timer_expired, (void *)"blink");
  rtcos_start_timer(u08OsTimerID, SOFTWARE_TIMER_PERIOD_IN_MS);
  rtcos_broadcast_event(EVENT_COMMON, 0, FALSE);
  rtcos_broadcast_message((void *)"Hello");

  rtcos_run();
  while(1)
  {}
}

#if defined(__GNUC__)
/** ***********************************************************************************************
  * @brief      Override the standard printf function
  * @param      s32Fd File descriptor
  * @param      pcString string to print
  * @param      s32Length string length
  * @return     Length of the printed string
  ********************************************************************************************** */
int _write(int s32Fd, char *pcString, int s32Length)
{
  HAL_UART_Transmit(&stUartHandle, (uint8_t *)pcString, s32Length, HAL_MAX_DELAY);
  return s32Length;
}
#endif /* __GNUC__ */

/*-----------------------------------------------------------------------------------------------*/
/* Private functions                                                                             */
/*-----------------------------------------------------------------------------------------------*/
/** ***********************************************************************************************
  * @brief      Configure system clock at 216 MHz
  * @return     Nothing
  ********************************************************************************************** */
static void _system_clock_configure(void)
{
  RCC_OscInitTypeDef stRccOscInit = {0};
  RCC_ClkInitTypeDef stRccClkInit = {0};
  RCC_PeriphCLKInitTypeDef stPeriphClkInit = {0};

  /* Configure LSE Drive Capability */
  HAL_PWR_EnableBkUpAccess();
  /* Configure the main internal regulator output voltage */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /* Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  stRccOscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  stRccOscInit.HSEState = RCC_HSE_ON;
  stRccOscInit.PLL.PLLState = RCC_PLL_ON;
  stRccOscInit.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  stRccOscInit.PLL.PLLM = 4;
  stRccOscInit.PLL.PLLN = 216;
  stRccOscInit.PLL.PLLP = RCC_PLLP_DIV2;
  stRccOscInit.PLL.PLLQ = 2;
  HAL_RCC_OscConfig(&stRccOscInit);
  /* Activate the Over-Drive mode */
  HAL_PWREx_EnableOverDrive();
  /* Initializes the CPU, AHB and APB buses clocks */
  stRccClkInit.ClockType = RCC_CLOCKTYPE_HCLK   |
                           RCC_CLOCKTYPE_SYSCLK |
                           RCC_CLOCKTYPE_PCLK1  |
                           RCC_CLOCKTYPE_PCLK2;
  stRccClkInit.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  stRccClkInit.AHBCLKDivider = RCC_SYSCLK_DIV1;
  stRccClkInit.APB1CLKDivider = RCC_HCLK_DIV4;
  stRccClkInit.APB2CLKDivider = RCC_HCLK_DIV2;

  HAL_RCC_ClockConfig(&stRccClkInit, FLASH_LATENCY_7);
  stPeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
  stPeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  HAL_RCCEx_PeriphCLKConfig(&stPeriphClkInit);
}

/** ***********************************************************************************************
  * @brief      Configure UART
  * @return     Nothing
  ********************************************************************************************** */
static void _uart_init(void)
{
  stUartHandle.Instance = USART3;
  stUartHandle.Init.BaudRate = SERIAL_BAUDRATE;
  stUartHandle.Init.WordLength = UART_WORDLENGTH_8B;
  stUartHandle.Init.StopBits = UART_STOPBITS_1;
  stUartHandle.Init.Parity = UART_PARITY_NONE;
  stUartHandle.Init.Mode = UART_MODE_TX_RX;
  stUartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  stUartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
  stUartHandle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  stUartHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  HAL_UART_Init(&stUartHandle);
}

/** ***********************************************************************************************
  * @brief      Configure GPIOs
  * @return     Nothing
  ********************************************************************************************** */
static void _gpio_init(void)
{
  GPIO_InitTypeDef stGpioInit = {0};

  /* Enable GPIO Ports Clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  /* Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_GREEN_PIN | LED_RED_PIN | LED_BLUE_PIN, GPIO_PIN_RESET);
  /* Configure GPIO pin : BUTTON_PIN */
  stGpioInit.Pin = BUTTON_PIN;
  stGpioInit.Mode = GPIO_MODE_IT_RISING;
  stGpioInit.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BUTTON_GPIO_PORT, &stGpioInit);
  /* Configure GPIO pins : LED_GREEN_PIN LED_RED_PIN LED_BLUE_PIN */
  stGpioInit.Pin = LED_GREEN_PIN|LED_RED_PIN|LED_BLUE_PIN;
  stGpioInit.Mode = GPIO_MODE_OUTPUT_PP;
  stGpioInit.Pull = GPIO_NOPULL;
  stGpioInit.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &stGpioInit);
  /* EXTI interrupt init */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/** ***********************************************************************************************
  * @brief      OS software timer callback
  * @param      pvArg Additional argument passed to the timer callback
  * @return     Returns nothing
  ********************************************************************************************** */
static void _on_os_timer_expired(void const *pvArg)
{
  if(0 == strcmp("blink", (_char *)pvArg))
  {
    HAL_GPIO_TogglePin(LED_BLUE_GPIO_PORT, LED_BLUE_PIN);
  }
}

/** ***********************************************************************************************
  * @brief      Task handler function
  * @param      u32EventFlags Bit feild event
  * @param      u08MsgCount number of messages belonging to this task
  * @param      pvArg Task argument
  * @return     Return unhandled events
  ********************************************************************************************** */
static _u32 _task_one_handler(_u32 u32EventFlags, _u08 u08MsgCount, void const *pvArg)
{
  _u32 u32RetVal;
  _char *pcMessage;

  u32RetVal = 0;
  printf("Task one argument is: %s\r\n", (_char *)pvArg);
  /* To allow executing higher priority tasks we just handle one event then return */
  if(u32EventFlags & EVENT_PING)
  {
    /*
     * Get events that have NOT been handled to return them to scheduler.
     * To make the system more responsive the task should only handle
     * the highest priority data and then return to the OS.
     * It should not try to handle all its outstanding data
     * otherwise a higher priority task might be kept from running.
     */
    printf("Task one received PING event!\r\n");
    /* Send a future pong event to task two */
    rtcos_send_event(TASK_ID_PRIORITY_TWO, EVENT_PONG, 1000, FALSE);
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_PING;
  }
  else if(u32EventFlags & EVENT_COMMON)
  {
    printf("Task one received a boadcasted event: EVENT_COMMON\r\n");
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_COMMON;
  }
  if(u08MsgCount)
  {
    if(RTCOS_ERR_NONE == rtcos_get_message((void **)&pcMessage))
    {
      printf("Task one received a boadcasted message: %s\r\n", pcMessage);
    }
  }
  return u32RetVal;
}

/** ***********************************************************************************************
  * @brief      Task handler function
  * @param      u32EventFlags Bit feild event
  * @param      u08MsgCount number of messages belonging to this task
  * @param      pvArg Task argument
  * @return     Return unhandled events
  ********************************************************************************************** */
static _u32 _task_two_handler(_u32 u32EventFlags, _u08 u08MsgCount, void const *pvArg)
{
  _u32 u32RetVal;
  _char *pcMessage;

  u32RetVal = 0;
  printf("Task two argument is: %s\r\n", (_char *)pvArg);
  /* To allow executing higher priority tasks we just handle one event then return */
  if(u32EventFlags & EVENT_PONG)
  {
    /*
     * Get events that have NOT been handled to return them to scheduler.
     * To make the system more responsive the task should only handle
     * the highest priority data and then return to the OS.
     * It should not try to handle all its outstanding data
     * otherwise a higher priority task might be kept from running.
     */
    printf("Task one received PONG event!\r\n");
    /* Send a future ping event to task one */
    rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, 1000, FALSE);
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_PONG;
  }
  else if(u32EventFlags & EVENT_COMMON)
  {
    printf("Task two received a boadcasted event: EVENT_COMMON\r\n");
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_COMMON;
  }
  if(u08MsgCount)
  {
    if(RTCOS_ERR_NONE == rtcos_get_message((void **)&pcMessage))
    {
      printf("Task two received a boadcasted message: %s\r\n", pcMessage);
    }
  }
  return u32RetVal;
}

/** ***********************************************************************************************
  * @brief      GPIO external interrupt callback
  * @param      u16Pin Interrupt source
  * @return     Nothing
  ********************************************************************************************** */
void HAL_GPIO_EXTI_Callback(uint16_t u16Pin)
{
  if(BUTTON_PIN == u16Pin)
  {
  }
}

/** ***********************************************************************************************
  * @brief      UART interrupt handler
  * @return     Nothing
  ********************************************************************************************** */
void USART3_IRQHandler(void)
{
  HAL_UART_IRQHandler(&stUartHandle);
}

/** ***********************************************************************************************
  * @brief      GPIO interrupt handler
  * @return     Nothing
  ********************************************************************************************** */
void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

/** ***********************************************************************************************
  * @brief      Systick timer interrupt handler
  * @return     Nothing
  ********************************************************************************************** */
void SysTick_Handler(void)
{
  rtcos_update_tick();
}

/** ***********************************************************************************************
  * @brief      System hardfault interrupt handler
  * @return     Nothing
  ********************************************************************************************** */
void HardFault_Handler(void)
{
  while(1);
}