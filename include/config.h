/* 
 **************************************************************************************************
 *
 * @file    : config.h
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.3.0
 * @date    : April 2021
 * @brief   : RTCOS default configuration file
 * 
 **************************************************************************************************
 */

#ifndef CONFIG_H
#define CONFIG_H

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include "RTCOSConfig.h"

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
/** System default configuration if no user configuration is specified */

#ifndef RTCOS_MAX_TASKS_COUNT
#define RTCOS_MAX_TASKS_COUNT                    2
#endif /* RTCOS_MAX_TASKS_COUNT */

#ifndef RTCOS_MAX_FUTURE_EVENTS_COUNT
#define RTCOS_MAX_FUTURE_EVENTS_COUNT            2
#endif /* RTCOS_MAX_FUTURE_EVENTS_COUNT */

#ifndef RTCOS_MAX_MESSAGES_COUNT
#define RTCOS_MAX_MESSAGES_COUNT                 2
#endif /* RTCOS_MAX_MESSAGES_COUNT */

#ifndef RTCOS_MAX_TIMERS_COUNT
#define RTCOS_MAX_TIMERS_COUNT                   2
#endif /* RTCOS_MAX_TIMERS_COUNT */

/*-----------------------------------------------------------------------------------------------*/
/* Macros                                                                                        */
/*-----------------------------------------------------------------------------------------------*/
#ifndef RTCOS_ENTER_CRITICAL_SECTION
#define RTCOS_ENTER_CRITICAL_SECTION()
#endif /* RTCOS_ENTER_CRITICAL_SECTION */

#ifndef RTCOS_EXIT_CRITICAL_SECTION
#define RTCOS_EXIT_CRITICAL_SECTION()
#endif /* RTCOS_EXIT_CRITICAL_SECTION */

#endif /* CONFIG_H */
