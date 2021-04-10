/* 
 **************************************************************************************************
 *
 * @file    : config.h
 * @author  : 
 * @version : 2.0
 * @date    : April 2021
 * @brief   : RTCOS configuration file
 * 
 **************************************************************************************************
 * 
 * @project  : {Generic}
 * @board    : {Generic}
 * @target   : {Generic}
 * @compiler : {Generic}
 * 
 **************************************************************************************************
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
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

#endif /* CONFIG_H */
