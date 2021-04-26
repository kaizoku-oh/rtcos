/* 
 **************************************************************************************************
 *
 * @file    : RTCOSConfig.h
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.2.1
 * @date    : April 2021
 * @brief   : RTCOS user configuration example used to overrite os default configuration
 * 
 **************************************************************************************************
 */

#ifndef RTCOS_CONFIG_H
#define RTCOS_CONFIG_H

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include <Arduino.h>

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
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
#define RTCOS_ENTER_CRITICAL_SECTION()           noInterrupts();
#define RTCOS_EXIT_CRITICAL_SECTION()            interrupts();

#endif /* RTCOS_CONFIG_H */
