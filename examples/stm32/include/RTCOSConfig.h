/* 
 **************************************************************************************************
 *
 * @file    : RTCOSConfig.h
 * @author  : Bayrem GHARSELLAOUI
 * @date    : October 2021
 * @brief   : RTCOS user configuration example used to override os default configuration
 * 
 **************************************************************************************************
 */

#ifndef RTCOS_CONFIG_H
#define RTCOS_CONFIG_H

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/*-----------------------------------------------------------------------------------------------*/
#define RTCOS_ENABLE_MESSAGES
#define RTCOS_ENABLE_TIMERS

#define RTCOS_MAX_TASKS_COUNT                    2
#define RTCOS_MAX_FUTURE_EVENTS_COUNT            2
#define RTCOS_MAX_MESSAGES_COUNT                 2
#define RTCOS_MAX_TIMERS_COUNT                   2

/*-----------------------------------------------------------------------------------------------*/
/* Macros                                                                                        */
/*-----------------------------------------------------------------------------------------------*/
#define RTCOS_ENTER_CRITICAL_SECTION()           __disable_irq();
#define RTCOS_EXIT_CRITICAL_SECTION()            __enable_irq();

#endif /* RTCOS_CONFIG_H */
