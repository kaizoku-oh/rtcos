#ifndef RTCOS_H
#define RTCOS_H

#include "port.h"

#define _NIL                                     0x00000000uL
#define _TRUE                                    (_BOOL)(1 == 1)
#define _FALSE                                   (_BOOL)(0 == 1)

typedef char                                     _CHAR;
typedef unsigned char                            _BOOL;
typedef unsigned char                            _U08;
typedef signed char                              _S08;
typedef unsigned short                           _U16;
typedef signed short                             _S16;
typedef unsigned long                            _U32;
typedef signed long                              _S32;
typedef unsigned long long                       _U64;
typedef signed long long                         _S64;
typedef void (*pFunction)(void);

typedef void (*pf_idle_handler_t)(void);
typedef void (*pf_timer_cb_t)(void const *);
typedef _U32 (*pf_task_handler_t)(_U32, _U08, _U32);

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

void rtcos_init(void);
void rtcos_run(void);
void rtcos_delay(_U32);
void rtcos_update_tick(void);
void rtcos_set_tick_count(_U32);
_U32 rtcos_get_tick_count(void);
_U08 rtcos_create_timer(rtcos_timer_type_t, pf_timer_cb_t, void *);
_BOOL rtcos_timer_expired(_U08);
rtcos_status_t rtcos_start_timer(_U08, _U32);
rtcos_status_t rtcos_stop_timer(_U08);
rtcos_status_t rtcos_register_task_handler(pf_task_handler_t, _U08, _U32);
rtcos_status_t rtcos_register_idle_handler(pf_idle_handler_t);
rtcos_status_t rtcos_send_event(_U08, _U32, _U32, _BOOL);
rtcos_status_t rtcos_clear_event(_U08, _U32);
rtcos_status_t rtcos_send_message(_U08, void *);
rtcos_status_t rtcos_get_message(void **);

#endif /* RTCOS_H */
