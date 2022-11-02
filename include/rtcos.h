/* 
 **************************************************************************************************
 *
 * @file    : rtcos.h
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.3.4
 * @date    : April 2021
 * @brief   : RTCOS header file
 * 
 **************************************************************************************************
 */
#ifndef RTCOS_H
#define RTCOS_H

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "config.h"


/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
#ifndef NULL
#define NULL                                     0x00000000uL
#endif /* NULL */

/*-----------------------------------------------------------------------------------------------*/
/* Types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/
/** A hook callback function to execute when the OS is doing nothing */
typedef void (*pf_os_idle_handler_t)(void);
#ifdef RTCOS_ENABLE_TIMERS
/** A callback function to execute when an OS timer expires */
typedef void (*pf_os_timer_cb_t)(void const *);
#endif /* RTCOS_ENABLE_TIMERS */
/** A task handler function to execute when the task receives an event or a message */
typedef uint32_t (*pf_os_task_handler_t)(uint32_t, uint8_t, void const *);

typedef enum
{
  RTCOS_ERR_NONE             = 0,
  RTCOS_ERR_OUT_OF_RESOURCES = -1,
  RTCOS_ERR_IN_USE           = -2,
  RTCOS_ERR_OUT_OF_RANGE     = -3,
  RTCOS_ERR_NOT_FOUND        = -4,
  RTCOS_ERR_TOO_MANY_EVENTS  = -5,
  RTCOS_ERR_NO_EVENT         = -6,
  RTCOS_ERR_INVALID_TASK     = -7,
  RTCOS_ERR_MSG_FULL         = -8,
  RTCOS_ERR_MSG_EMPTY        = -9,
  RTCOS_ERR_ARG              = -10,
}rtcos_status_t;

#ifdef RTCOS_ENABLE_TIMERS
typedef enum
{
  RTCOS_TIMER_PERIODIC       = 0,
  RTCOS_TIMER_ONE_SHOT,
}rtcos_timer_type_t;
#endif /* RTCOS_ENABLE_TIMERS */

/*-----------------------------------------------------------------------------------------------*/
/* Functions                                                                                     */
/*-----------------------------------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

void rtcos_init(void);
void rtcos_run(void);
void rtcos_delay(uint32_t);
void rtcos_update_tick(void);
void rtcos_set_tick_count(uint32_t);
uint32_t rtcos_get_tick_count(void);
#ifdef RTCOS_ENABLE_TIMERS
int8_t rtcos_create_timer(rtcos_timer_type_t, pf_os_timer_cb_t, void *);
bool rtcos_timer_expired(uint8_t);
rtcos_status_t rtcos_start_timer(uint8_t, uint32_t);
rtcos_status_t rtcos_stop_timer(uint8_t);
#endif /* RTCOS_ENABLE_TIMERS */
rtcos_status_t rtcos_register_task_handler(pf_os_task_handler_t, uint8_t,  void *);
rtcos_status_t rtcos_register_idle_handler(pf_os_idle_handler_t);
rtcos_status_t rtcos_send_event(uint8_t, uint32_t, uint32_t, bool);
rtcos_status_t rtcos_broadcast_event(uint32_t, uint32_t, bool);
rtcos_status_t rtcos_clear_event(uint8_t, uint32_t);
#ifdef RTCOS_ENABLE_MESSAGES
rtcos_status_t rtcos_send_message(uint8_t, void *);
rtcos_status_t rtcos_broadcast_message(void *);
rtcos_status_t rtcos_get_message(void **);
#endif /* RTCOS_ENABLE_MESSAGES */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* RTCOS_H */
