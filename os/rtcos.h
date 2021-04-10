/* 
 **************************************************************************************************
 *
 * @file    : rtcos.h
 * @author  : 
 * @version : 2.0
 * @date    : April 2021
 * @brief   : RTCOS header file
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

#ifndef RTCOS_H
#define RTCOS_H

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include "port.h"

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
#define _NIL                                     0x00000000uL
#define _TRUE                                    (_bool)(1 == 1)
#define _FALSE                                   (_bool)(0 == 1)

/*-----------------------------------------------------------------------------------------------*/
/* Types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/
/** Generic types */
typedef char                                     _char;
typedef unsigned char                            _bool;
typedef unsigned char                            _u08;
typedef signed char                              _s08;
typedef unsigned short                           _u16;
typedef signed short                             _s16;
typedef unsigned long                            _u32;
typedef signed long                              _s32;
typedef unsigned long long                       _u64;
typedef signed long long                         _s64;
typedef void (*pFunction)(void);
/** RTCOS related types */
typedef void (*pf_idle_handler_t)(void);
typedef void (*pf_timer_cb_t)(void const *);
typedef _u32 (*pf_task_handler_t)(_u32, _u08, _u32);
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
typedef enum
{
  RTCOS_TIMER_PERIODIC       = 0,
  RTCOS_TIMER_ONE_SHOT,
}rtcos_timer_type_t;

/*-----------------------------------------------------------------------------------------------*/
/* Functions                                                                                     */
/*-----------------------------------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

void rtcos_init(void);
void rtcos_run(void);
void rtcos_delay(_u32);
void rtcos_update_tick(void);
void rtcos_set_tick_count(_u32);
_u32 rtcos_get_tick_count(void);
_u08 rtcos_create_timer(rtcos_timer_type_t, pf_timer_cb_t, void *);
_bool rtcos_timer_expired(_u08);
rtcos_status_t rtcos_start_timer(_u08, _u32);
rtcos_status_t rtcos_stop_timer(_u08);
rtcos_status_t rtcos_register_task_handler(pf_task_handler_t, _u08, _u32);
rtcos_status_t rtcos_register_idle_handler(pf_idle_handler_t);
rtcos_status_t rtcos_send_event(_u08, _u32, _u32, _bool);
rtcos_status_t rtcos_clear_event(_u08, _u32);
rtcos_status_t rtcos_send_message(_u08, void *);
rtcos_status_t rtcos_get_message(void **);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* RTCOS_H */
