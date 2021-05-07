/* 
 **************************************************************************************************
 *
 * @file    : main.h
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.3.0
 * @date    : May 2021
 * @brief   : Nucleo F767ZI running RTCOS example
 * 
 **************************************************************************************************
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
#define BUTTON_PIN                               GPIO_PIN_13
#define BUTTON_GPIO_PORT                         GPIOC
#define BUTTON_EXTI_IRQN                         EXTI15_10_IRQn

#define LED_GREEN_PIN                            GPIO_PIN_0
#define LED_GREEN_GPIO_PORT                      GPIOB

#define LED_BLUE_PIN                             GPIO_PIN_7
#define LED_BLUE_GPIO_PORT                       GPIOB

#define LED_RED_PIN                              GPIO_PIN_14
#define LED_RED_GPIO_PORT                        GPIOB

#define STLINK_UART_TX_PIN                       GPIO_PIN_8
#define STLINK_UART_TX_GPIO_PORT                 GPIOD
#define STLINK_UART_RX_PIN                       GPIO_PIN_9
#define STLINK_UART_RX_GPIO_PORT                 GPIOD

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
