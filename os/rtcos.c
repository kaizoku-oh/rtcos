#include "rtcos.h"

typedef struct
{
  _U08 u08Head;
  _U08 u08Tail;
  _U08 u08Count;
  void *tpvBuffer[RTCOS_MAX_MESSAGES_COUNT];
}rtcos_fifo_t;

typedef struct
{
  _U32 u32EventFlags;
  volatile _U32 u32Timeout;
  _U32 u32ReloadTimeout;
  _U08 u08TaskID;
  volatile _BOOL bInUse;
}rtcos_future_event_t;

typedef struct
{
  volatile _BOOL bInUse;
  rtcos_timer_type_t mPeriodType;
  volatile _U32	u32StartTickCount;
  _U32 u32TickDelay;
  pf_timer_cb_t pfTimerCb;
}rtcos_timer_t;

typedef struct
{
  volatile _U32 u32EventFlags;
  pf_task_handler_t pfTaskHandlerCb;
  _U32 u32TaskParam;
  rtcos_fifo_t stFifo;
}rtcos_task_t;

typedef struct
{
  _U08 u08TasksCount;
  _U08 u08CurrentTask;
  volatile _U32 u32SysTickCount;
  pf_idle_handler_t pfIdleHandler;
  volatile _U08 u08FutureEventsCount;
  rtcos_future_event_t tstFutureEvents[RTCOS_MAX_FUTURE_EVENTS_COUNT];
  rtcos_task_t tstTasks[RTCOS_MAX_TASKS_COUNT];
  rtcos_timer_t tstTimers[RTCOS_MAX_TIMERS_COUNT];
  _U08 u08TimersCount;
}rtcos_ctx_t;

static rtcos_ctx_t stRtcosCtx;

static void _rtcos_fifo_init(_U08 u08TaskID)
{
  stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Head = 0;
  stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Tail = 0;
  stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Count = 0;
}

static _BOOL _rtcos_fifo_empty(_U08 u08TaskID)
{
  return (stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Count > 0)?_FALSE:_TRUE;
}

static _BOOL _rtcos_fifo_full(_U08 u08TaskID)
{
  return (stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Count >= RTCOS_MAX_MESSAGES_COUNT)?_TRUE:_FALSE;
}

static _U08 _rtcos_fifo_count(_U08 u08TaskID)
{
  return stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Count;
}

static rtcos_status_t _rtcos_fifo_push(_U08 u08TaskID, void *pvMsg)
{
  rtcos_status_t eRetVal;

  if(_FALSE == _rtcos_fifo_full(u08TaskID))
  {
    stRtcosCtx
      .tstTasks[u08TaskID]
        .stFifo.tpvBuffer[stRtcosCtx.tstTasks[u08TaskID]
          .stFifo.u08Head++] = pvMsg;
    ++stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Count;
    if(stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Head >= RTCOS_MAX_MESSAGES_COUNT)
    {
      stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Head = 0;
    }
    eRetVal = RTCOS_ERR_NONE;
  }
  else
  {
    eRetVal = RTCOS_ERR_MSG_FULL;
  }
  return eRetVal;
}

static rtcos_status_t _rtcos_fifo_pop(_U08 u08TaskID, void **ppvMsg)
{
  rtcos_status_t eRetVal;

  if(_FALSE == _rtcos_fifo_empty(u08TaskID))
  {
    *ppvMsg = stRtcosCtx
                .tstTasks[u08TaskID]
                  .stFifo.tpvBuffer[stRtcosCtx.tstTasks[u08TaskID]
                    .stFifo.u08Tail++];
    --stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Count;
    if(stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Tail >= RTCOS_MAX_MESSAGES_COUNT)
    {
      stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Tail = 0;
    }
    eRetVal = RTCOS_ERR_NONE;
  }
  else
  {
    eRetVal = RTCOS_ERR_MSG_EMPTY;
  }
  return eRetVal;
}

static _BOOL _rtcos_find_ready_task(_U08 *u08NewCurrTaskID)
{
  _U08 u08Index;
  _BOOL bRetVal;

  bRetVal = _FALSE;
  for(u08Index = 0; u08Index < stRtcosCtx.u08TasksCount; ++u08Index)
  {
    if((0 != stRtcosCtx.tstTasks[u08Index].u32EventFlags) ||
       (_FALSE == _rtcos_fifo_empty(u08Index)))
    {
      *u08NewCurrTaskID = u08Index;
      bRetVal = _TRUE;
      break;
    }
  }
  return bRetVal;
}

static void _rtcos_run_ready_task(_U08 u08NewCurrTaskID)
{
  _U32 u32UnhandledEvents;
  _U32 u32CurrentEvents;

  ENTER_CRITICAL_SECTION();
  stRtcosCtx.u08CurrentTask = u08NewCurrTaskID;
  u32CurrentEvents = stRtcosCtx.tstTasks[stRtcosCtx.u08CurrentTask].u32EventFlags;
  stRtcosCtx.tstTasks[stRtcosCtx.u08CurrentTask].u32EventFlags = 0;  
  EXIT_CRITICAL_SECTION();

  u32UnhandledEvents = (stRtcosCtx.tstTasks[stRtcosCtx.u08CurrentTask].pfTaskHandlerCb)
                       (u32CurrentEvents,
                        _rtcos_fifo_count(stRtcosCtx.u08CurrentTask),
                        stRtcosCtx.tstTasks[stRtcosCtx.u08CurrentTask].u32TaskParam);
  ENTER_CRITICAL_SECTION();
  stRtcosCtx.tstTasks[stRtcosCtx.u08CurrentTask].u32EventFlags |= u32UnhandledEvents;
  EXIT_CRITICAL_SECTION();
}

static rtcos_status_t _rtcos_find_future_event(_U08 u08TaskID,
                                               _U32 u32EventFlags,
                                               _U08 *pu08FoundEventIdx)
{
  _U08 u08Index;
  rtcos_status_t eRetVal;

  eRetVal = RTCOS_ERR_NOT_FOUND;
  for(u08Index = 0; u08Index < RTCOS_MAX_FUTURE_EVENTS_COUNT; ++u08Index)
  {
    if((_TRUE == stRtcosCtx.tstFutureEvents[u08Index].bInUse) &&
       (stRtcosCtx.tstFutureEvents[u08Index].u08TaskID == u08TaskID) && 
       (stRtcosCtx.tstFutureEvents[u08Index].u32EventFlags == u32EventFlags))
    {
      *pu08FoundEventIdx = u08Index;
      eRetVal = RTCOS_ERR_NONE;
      break;
    }
  }
  return eRetVal;
}

static rtcos_status_t _rtcos_delete_future_event(_U08 u08TaskID, _U32 u32EventFlags)
{
  rtcos_status_t eRetVal;
  _U08 u08FoundEventIdx;

  eRetVal = _rtcos_find_future_event(u08TaskID, u32EventFlags, &u08FoundEventIdx);
  if(RTCOS_ERR_NONE == eRetVal)
  {
    stRtcosCtx.tstFutureEvents[u08FoundEventIdx].bInUse = _FALSE;
    if(stRtcosCtx.u08FutureEventsCount > 0)
    {
      stRtcosCtx.u08FutureEventsCount--;
    }
  }
  return eRetVal;
}

static rtcos_status_t _rtcos_find_empty_future_event_index(_U08 *pu08FoundEventIdx)
{
  _U08 u08Index;
  rtcos_status_t eRetVal;

  eRetVal = RTCOS_ERR_NOT_FOUND;
  for(u08Index = 0; u08Index < RTCOS_MAX_FUTURE_EVENTS_COUNT; ++u08Index)
  {
    if(_FALSE == stRtcosCtx.tstFutureEvents[u08Index].bInUse)
    {
      *pu08FoundEventIdx = u08Index;
      eRetVal = RTCOS_ERR_NONE;
      break;
    }
  }
  return eRetVal;
}

static rtcos_status_t _rtcos_add_future_event(_U08 u08TaskID,
                                              _U32 u32EventFlags,
                                              _U32 u32EventDelay,
                                              _BOOL bPeriodicEvent)
{
  _U08 u08FoundEventIdx;
  rtcos_status_t eRetVal;

  ENTER_CRITICAL_SECTION();
  eRetVal = _rtcos_find_future_event(u08TaskID, u32EventFlags, &u08FoundEventIdx);
  if(RTCOS_ERR_NONE == eRetVal)
  {
    stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u32Timeout = u32EventDelay;
  }
  else
  {
    eRetVal = _rtcos_find_empty_future_event_index(&u08FoundEventIdx);
    if(RTCOS_ERR_NONE == eRetVal)
    {
      stRtcosCtx.tstFutureEvents[u08FoundEventIdx].bInUse = _TRUE;
      stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u32Timeout = u32EventDelay;
      stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u08TaskID = u08TaskID;
      stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u32EventFlags = u32EventFlags;
      ++stRtcosCtx.u08FutureEventsCount;
      if(_TRUE == bPeriodicEvent)
      {   
        stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u32ReloadTimeout = u32EventDelay;
      }
      else
      {
        stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u32ReloadTimeout = 0;
      }
    }
  }
  EXIT_CRITICAL_SECTION();
  return eRetVal;
}
 
static rtcos_status_t _rtcos_count_events(_U32 u32EventFlags)
{
  return (u32EventFlags)?RTCOS_ERR_NONE:RTCOS_ERR_NO_EVENT;
}

static rtcos_status_t _rtcos_check_event_input(_U08 u08TaskID, _U32 u32EventFlags)
{
  rtcos_status_t eRetVal;

  eRetVal = _rtcos_count_events(u32EventFlags);
  if(RTCOS_ERR_NONE == eRetVal)
  {
    if(u08TaskID >= stRtcosCtx.u08TasksCount)
    {
      eRetVal = RTCOS_ERR_INVALID_TASK;
    }
  }
  return eRetVal;
}

void rtcos_init(void)
{
  _U08 u08Index;

  for(u08Index = 0; u08Index < RTCOS_MAX_TASKS_COUNT; ++u08Index)
  {
    stRtcosCtx.tstTasks[u08Index].pfTaskHandlerCb = _NIL;
    stRtcosCtx.tstTasks[u08Index].u32EventFlags = 0;
    _rtcos_fifo_init(u08Index);
  }
  for(u08Index = 0; u08Index < RTCOS_MAX_FUTURE_EVENTS_COUNT; ++u08Index)
  {
    stRtcosCtx.tstFutureEvents[u08Index].bInUse = _FALSE;
    stRtcosCtx.tstFutureEvents[u08Index].u08TaskID = 0;
    stRtcosCtx.tstFutureEvents[u08Index].u32EventFlags = 0;
    stRtcosCtx.tstFutureEvents[u08Index].u32Timeout = 0;
    stRtcosCtx.tstFutureEvents[u08Index].u32ReloadTimeout = 0;
  }
  for(u08Index = 0; u08Index < RTCOS_MAX_TASKS_COUNT; ++u08Index)
  {
    stRtcosCtx.tstTimers[u08Index].u32StartTickCount = 0;
    stRtcosCtx.tstTimers[u08Index].u32TickDelay = 0;
    stRtcosCtx.tstTimers[u08Index].bInUse = _FALSE;
    stRtcosCtx.tstTimers[u08Index].pfTimerCb = _NIL;
  }
  stRtcosCtx.u08CurrentTask = 0;
  stRtcosCtx.u32SysTickCount = 0;
  stRtcosCtx.u08TasksCount = 0;
  stRtcosCtx.u08FutureEventsCount = 0;
  stRtcosCtx.pfIdleHandler = _NIL;
  stRtcosCtx.u08TimersCount = 0;
}

rtcos_status_t rtcos_register_task_handler(pf_task_handler_t pfTaskHandler,
                                           _U08 u08TaskID,
                                           _U32 u32TaskParam)
{
  rtcos_status_t eRetVal;

  if(u08TaskID < RTCOS_MAX_TASKS_COUNT)
  {
    if(stRtcosCtx.tstTasks[u08TaskID].pfTaskHandlerCb != _NIL)
    {
      eRetVal = RTCOS_ERR_IN_USE;
    }
    else
    {
      stRtcosCtx.tstTasks[u08TaskID].pfTaskHandlerCb = pfTaskHandler;
      stRtcosCtx.tstTasks[u08TaskID].u32TaskParam = u32TaskParam;
      ++stRtcosCtx.u08TasksCount;
      eRetVal = RTCOS_ERR_NONE;
    }
  }
  else
  {
    eRetVal = RTCOS_ERR_OUT_OF_RANGE;
  }
  return eRetVal;
}

rtcos_status_t rtcos_register_idle_handler(pf_idle_handler_t pfIdleHandler)
{
  rtcos_status_t eRetVal;

  if(pfIdleHandler)
  {
    stRtcosCtx.pfIdleHandler = pfIdleHandler; 
    eRetVal = RTCOS_ERR_NONE;
  }
  else
  {
    eRetVal = RTCOS_ERR_ARG;
  }
  return eRetVal;
}

rtcos_status_t rtcos_send_message(_U08 u08TaskID, void *pMsg)
{
  rtcos_status_t eRetVal;

  if(u08TaskID < stRtcosCtx.u08TasksCount)
  {
    ENTER_CRITICAL_SECTION();
    eRetVal = _rtcos_fifo_push(u08TaskID, pMsg);
    EXIT_CRITICAL_SECTION();
  }
  else
  {
    eRetVal = RTCOS_ERR_INVALID_TASK;
  }
  return eRetVal;
}

rtcos_status_t rtcos_get_message(void * *pMsg)
{
  rtcos_status_t eRetVal;

  if(stRtcosCtx.u08CurrentTask < stRtcosCtx.u08TasksCount)
  {
    ENTER_CRITICAL_SECTION();
    eRetVal = _rtcos_fifo_pop(stRtcosCtx.u08CurrentTask, pMsg);
    EXIT_CRITICAL_SECTION();
  }
  else
  {
    eRetVal = RTCOS_ERR_INVALID_TASK;
  }
  return eRetVal;
}

_U08 rtcos_create_timer(rtcos_timer_type_t periodType, pf_timer_cb_t pfTimerCb, void *pvArg)
{
  _U08 eRetVal;

  ENTER_CRITICAL_SECTION();
  if(stRtcosCtx.u08TimersCount >= RTCOS_MAX_TIMERS_COUNT)
  {
    eRetVal = RTCOS_ERR_OUT_OF_RESOURCES;
  }
  else
  {
    stRtcosCtx.tstTimers[stRtcosCtx.u08TimersCount].mPeriodType = periodType;
    stRtcosCtx.tstTimers[stRtcosCtx.u08TimersCount].pfTimerCb = pfTimerCb;
    eRetVal = stRtcosCtx.u08TimersCount++;
  }
  EXIT_CRITICAL_SECTION();
  return eRetVal;
}

rtcos_status_t rtcos_start_timer(_U08 u08TimerID, _U32 u32PeriodInTicks)
{
  rtcos_status_t eRetVal;

  ENTER_CRITICAL_SECTION();
  if(u08TimerID >= RTCOS_MAX_TIMERS_COUNT)
  {
    eRetVal = RTCOS_ERR_OUT_OF_RESOURCES;
  }
  else
  {
    stRtcosCtx.tstTimers[u08TimerID].u32TickDelay = u32PeriodInTicks;
    stRtcosCtx.tstTimers[u08TimerID].u32StartTickCount = stRtcosCtx.u32SysTickCount;
    stRtcosCtx.tstTimers[u08TimerID].bInUse = _TRUE;
    eRetVal = RTCOS_ERR_NONE;
  }
  EXIT_CRITICAL_SECTION();
  return eRetVal;
}

rtcos_status_t rtcos_stop_timer(_U08 u08TimerID)
{
  rtcos_status_t eRetVal;

  ENTER_CRITICAL_SECTION();
  if(u08TimerID >= RTCOS_MAX_TIMERS_COUNT)
  {
    eRetVal = RTCOS_ERR_OUT_OF_RESOURCES;
  }
  else
  {
    stRtcosCtx.tstTimers[u08TimerID].bInUse = _FALSE;
    eRetVal = RTCOS_ERR_NONE;
  }
  EXIT_CRITICAL_SECTION();
  return eRetVal;
}

_BOOL rtcos_timer_expired(_U08 u08TimerID)
{
  _U32 u32CurrentTickCount;
  _BOOL bExpired;

  bExpired = _FALSE;
  u32CurrentTickCount = stRtcosCtx.u32SysTickCount;
  ENTER_CRITICAL_SECTION();
  if((stRtcosCtx.tstTimers[u08TimerID].bInUse) && (u08TimerID < RTCOS_MAX_TIMERS_COUNT))
  {
    if((u32CurrentTickCount - stRtcosCtx.tstTimers[u08TimerID].u32StartTickCount) >
       (stRtcosCtx.tstTimers[u08TimerID].u32TickDelay))
    {
      bExpired = _TRUE;
    }
  }
  EXIT_CRITICAL_SECTION();
  return bExpired;
}

rtcos_status_t rtcos_send_event(_U08 u08TaskID,
                                _U32 u32EventFlags,
                                _U32 u32EventDelay,
                                _BOOL bPeriodicEvent)
{
  rtcos_status_t eRetVal;

  eRetVal = _rtcos_check_event_input(u08TaskID, u32EventFlags);
  if(RTCOS_ERR_NONE == eRetVal)
  {
    if(0 == u32EventDelay)
    {
      ENTER_CRITICAL_SECTION();
      stRtcosCtx.tstTasks[u08TaskID].u32EventFlags |= u32EventFlags;
      EXIT_CRITICAL_SECTION();
    }
    else
    {
      eRetVal = _rtcos_add_future_event(u08TaskID, u32EventFlags, u32EventDelay, bPeriodicEvent);
    }
  }
  return eRetVal;
}

rtcos_status_t rtcos_clear_event(_U08 u08TaskID, _U32 u32EventFlags)
{
  rtcos_status_t eRetVal;

  eRetVal = _rtcos_check_event_input(u08TaskID, u32EventFlags);
  if(RTCOS_ERR_NONE == eRetVal)
  {
    ENTER_CRITICAL_SECTION();
    stRtcosCtx.tstTasks[u08TaskID].u32EventFlags &= ~(u32EventFlags);
    _rtcos_delete_future_event(u08TaskID, u32EventFlags); 
    EXIT_CRITICAL_SECTION();
  }
  return eRetVal;
}

void rtcos_set_tick_count(_U32 u32TickCount)
{
  stRtcosCtx.u32SysTickCount = u32TickCount;
}

_U32 rtcos_get_tick_count(void)
{
  _U32 u32CurrTickCount;

  ENTER_CRITICAL_SECTION();
  u32CurrTickCount = stRtcosCtx.u32SysTickCount;
  EXIT_CRITICAL_SECTION();
  return u32CurrTickCount;
}

void rtcos_delay(_U32 u32DelayTicksCount)
{
  _U32 u32Tick;

  u32Tick = rtcos_get_tick_count();
  while((rtcos_get_tick_count() - u32Tick) != u32DelayTicksCount);
}

void rtcos_run(void)
{
  _BOOL bFoundReadyTask;
  _U08 u08NewCurrTaskID;

  while(1)
  {
    ENTER_CRITICAL_SECTION();
    bFoundReadyTask = _rtcos_find_ready_task(&u08NewCurrTaskID);
    EXIT_CRITICAL_SECTION();
    if(_TRUE == bFoundReadyTask)
    {
      _rtcos_run_ready_task(u08NewCurrTaskID);
    }
    else if((_NIL != stRtcosCtx.pfIdleHandler) &&
            (0 == stRtcosCtx.u08FutureEventsCount))
    {
      (stRtcosCtx.pfIdleHandler)();
    }
  }
}

void rtcos_update_tick(void)
{
  _U08 u08Index;
  
  ENTER_CRITICAL_SECTION();
  ++stRtcosCtx.u32SysTickCount;
  for(u08Index = 0; u08Index < RTCOS_MAX_FUTURE_EVENTS_COUNT; ++u08Index)
  {
    if(_TRUE == stRtcosCtx.tstFutureEvents[u08Index].bInUse)
    {
      --stRtcosCtx.tstFutureEvents[u08Index].u32Timeout; 
      if(0 == stRtcosCtx.tstFutureEvents[u08Index].u32Timeout)
      {
        if(stRtcosCtx.u08FutureEventsCount > 0)
        {
          stRtcosCtx.u08FutureEventsCount--;
        }
        stRtcosCtx
          .tstTasks[stRtcosCtx.tstFutureEvents[u08Index].u08TaskID]
            .u32EventFlags |= stRtcosCtx.tstFutureEvents[u08Index].u32EventFlags;
        if(0 == stRtcosCtx.tstFutureEvents[u08Index].u32ReloadTimeout)
        {
          stRtcosCtx.tstFutureEvents[u08Index].bInUse = _FALSE;
        }
        else
        {
          stRtcosCtx.tstFutureEvents[u08Index].u32Timeout =
          stRtcosCtx.tstFutureEvents[u08Index].u32ReloadTimeout;
        }
      }
    }
  }
  if(stRtcosCtx.u08TimersCount > 0)
  {
    for(u08Index = 0; u08Index < stRtcosCtx.u08TimersCount; u08Index++)
    {
      if(rtcos_timer_expired(u08Index))
      {
        if(stRtcosCtx.tstTimers[u08Index].pfTimerCb)
        {
          stRtcosCtx.tstTimers[u08Index].pfTimerCb(0);
        }
        if(RTCOS_TIMER_ONE_SHOT == stRtcosCtx.tstTimers[u08Index].mPeriodType)
        {
          stRtcosCtx.tstTimers[u08Index].bInUse = _FALSE;
        }
        stRtcosCtx.tstTimers[u08Index].u32StartTickCount = stRtcosCtx.u32SysTickCount;
      }
    }
  }
  EXIT_CRITICAL_SECTION();
}
