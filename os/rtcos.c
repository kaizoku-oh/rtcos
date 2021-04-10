/* 
 **************************************************************************************************
 *
 * @file    : rtcos.c
 * @author  : 
 * @version : 2.0
 * @date    : April 2021
 * @brief   : RTCOS source file
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

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include "rtcos.h"
#include "config.h"

/*-----------------------------------------------------------------------------------------------*/
/* Private types                                                                                 */
/*-----------------------------------------------------------------------------------------------*/
/** Fifo structure used for storing messages */
typedef struct
{
  _u08 u08Head;                                  /**< Fifo head position                         */
  _u08 u08Tail;                                  /**< Fifo tail position                         */
  _u08 u08Count;                                 /**< Fifo current cout                          */
  void *tpvBuffer[RTCOS_MAX_MESSAGES_COUNT];     /**< Fifo buffer of pointers                    */
}rtcos_fifo_t;

/** Future event structure representing information about each event */
typedef struct
{
  _u32 u32EventFlags;                            /**< 32 bits representing different events      */
  volatile _u32 u32EventDelay;                   /**< Delay to wait before handling the event    */
  _u32 u32ReloadDelay;                           /**< Delay to wait before reloading the event   */
  _u08 u08TaskID;                                /**< ID of the task associated with this event  */
  volatile _bool bInUse;                         /**< Indicates if the event is still used       */
}rtcos_future_event_t;

/** Software os timer structure representing information about each timer */
typedef struct
{
  volatile _bool bInUse;                         /**< Indicates if the timer is still used       */
  rtcos_timer_type_t ePeriodType;                /**< Periodic or one shot timer                 */
  volatile _u32	u32StartTickCount;               /**< Start time of the timer                    */
  _u32 u32TickDelay;                             /**< Period of the timer                        */
  pf_timer_cb_t pfTimerCb;                       /**< Timer callback function                    */
}rtcos_timer_t;

/** Task structure representing information about each task */
typedef struct
{
  volatile _u32 u32EventFlags;                   /**< Event flags associated to this task        */
  pf_task_handler_t pfTaskHandlerCb;             /**< Task handler function                      */
  _u32 u32TaskParam;                             /**< Task parameter                             */
  rtcos_fifo_t stFifo;                           /**< Fifo associated to this task               */
}rtcos_task_t;

/** Context structure representing the main context of the OS */
typedef struct
{
  _u08 u08TasksCount;                            /**< Number of the tasks present in the system  */
  _u08 u08CurrentTaskID;                         /**< Current task ID                            */
  volatile _u32 u32SysTicksCount;                 /**< Current number of the system ticks        */
  pf_idle_handler_t pfIdleHandler;               /**< Handler function when the system is Idle   */
  volatile _u08 u08FutureEventsCount;            /**< Number of the events present in the system */
  rtcos_future_event_t tstFutureEvents[RTCOS_MAX_FUTURE_EVENTS_COUNT]; /**< Array of events      */
  rtcos_task_t tstTasks[RTCOS_MAX_TASKS_COUNT];  /**< Array of tasks                             */
  rtcos_timer_t tstTimers[RTCOS_MAX_TIMERS_COUNT]; /**< Array of timers                          */
  _u08 u08TimersCount;                           /**< Number of the timers present in the system */
}rtcos_ctx_t;

/*-----------------------------------------------------------------------------------------------*/
/* Private variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/
static rtcos_ctx_t stRtcosCtx;

/*-----------------------------------------------------------------------------------------------*/
/* Private functions                                                                             */
/*-----------------------------------------------------------------------------------------------*/
/** ***********************************************************************************************
  * @brief      Initialize the fifo that will hold a task's messages
  * @param      u08TaskID ID of the task using this fifo
  * @return     Nothing
  ********************************************************************************************** */
static void _rtcos_fifo_init(_u08 u08TaskID)
{
  stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Head = 0;
  stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Tail = 0;
  stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Count = 0;
}

/** ***********************************************************************************************
  * @brief      Check if the fifo of a certain task is empty
  * @param      u08TaskID ID of the task using this fifo
  * @return     _TRUE if empty, else _FALSE
  ********************************************************************************************** */
static _bool _rtcos_fifo_empty(_u08 u08TaskID)
{
  return (stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Count > 0)?_FALSE:_TRUE;
}

/** ***********************************************************************************************
  * @brief      Check if the fifo of a certain task is full
  * @param      u08TaskID ID of the task using this fifo
  * @return     _TRUE if full, else _FALSE
  ********************************************************************************************** */
static _bool _rtcos_fifo_full(_u08 u08TaskID)
{
  return (stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Count >= RTCOS_MAX_MESSAGES_COUNT)?_TRUE:_FALSE;
}

/** ***********************************************************************************************
  * @brief      Get the number of items in the fifo
  * @param      u08TaskID ID of the task using this fifo
  * @return     Number of items in the fifo
  ********************************************************************************************** */
static _u08 _rtcos_fifo_count(_u08 u08TaskID)
{
  return stRtcosCtx.tstTasks[u08TaskID].stFifo.u08Count;
}

/** ***********************************************************************************************
  * @brief      Put a message on the fifo of a certain task
  * @param      u08TaskID ID of the task using this fifo
  * @param      pvMsg Pointer to the message
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
static rtcos_status_t _rtcos_fifo_push(_u08 u08TaskID, void *pvMsg)
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

/** ***********************************************************************************************
  * @brief      Retrieve a message of a certain task
  * @param      u08TaskID ID of the task using this fifo
  * @param      ppvMsg Pointer to a pointer to the message
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
static rtcos_status_t _rtcos_fifo_pop(_u08 u08TaskID, void **ppvMsg)
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

/** ***********************************************************************************************
  * @brief      Find the highest priority task with an event or a message
  * @param      pu08NewCurrTaskID This will hold the ID of the found ready task
  * @return     _TRUE if a task is found, else _FALSE
  ********************************************************************************************** */
static _bool _rtcos_find_ready_task(_u08 *pu08NewCurrTaskID)
{
  _u08 u08Index;
  _bool bRetVal;

  bRetVal = _FALSE;
  for(u08Index = 0; u08Index < stRtcosCtx.u08TasksCount; ++u08Index)
  {
    if((0 != stRtcosCtx.tstTasks[u08Index].u32EventFlags) ||
       (_FALSE == _rtcos_fifo_empty(u08Index)))
    {
      *pu08NewCurrTaskID = u08Index;
      bRetVal = _TRUE;
      break;
    }
  }
  return bRetVal;
}

/** ***********************************************************************************************
  * @brief      Run the task that has the highest priority and is ready
  * @param      u08NewCurrTaskID ID of the task to run
  * @return     Nothing
  ********************************************************************************************** */
static void _rtcos_run_ready_task(_u08 u08NewCurrTaskID)
{
  _u32 u32UnhandledEvents;
  _u32 u32CurrentEvents;

  ENTER_CRITICAL_SECTION();
  stRtcosCtx.u08CurrentTaskID = u08NewCurrTaskID;
  u32CurrentEvents = stRtcosCtx.tstTasks[stRtcosCtx.u08CurrentTaskID].u32EventFlags;
  stRtcosCtx.tstTasks[stRtcosCtx.u08CurrentTaskID].u32EventFlags = 0;  
  EXIT_CRITICAL_SECTION();
  u32UnhandledEvents = (stRtcosCtx.tstTasks[stRtcosCtx.u08CurrentTaskID].pfTaskHandlerCb)
                       (u32CurrentEvents,
                        _rtcos_fifo_count(stRtcosCtx.u08CurrentTaskID),
                        stRtcosCtx.tstTasks[stRtcosCtx.u08CurrentTaskID].u32TaskParam);
  ENTER_CRITICAL_SECTION();
  stRtcosCtx.tstTasks[stRtcosCtx.u08CurrentTaskID].u32EventFlags |= u32UnhandledEvents;
  EXIT_CRITICAL_SECTION();
}

/** ***********************************************************************************************
  * @brief      Search for a used event that has the requested task ID and event flag
  * @param      u08TaskID ID of the task using this fifo
  * @param      u32EventFlags Bit feild event
  * @param      pu08FoundEventIdx This will hold the index the event if found
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
static rtcos_status_t _rtcos_find_future_event(_u08 u08TaskID,
                                               _u32 u32EventFlags,
                                               _u08 *pu08FoundEventIdx)
{
  _u08 u08Index;
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

/** ***********************************************************************************************
  * @brief      Search for a used event that has the requested task ID and event flag and delete it
  * @param      u08TaskID ID of the task using this fifo
  * @param      u32EventFlags Bit feild event
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
static rtcos_status_t _rtcos_delete_future_event(_u08 u08TaskID, _u32 u32EventFlags)
{
  rtcos_status_t eRetVal;
  _u08 u08FoundEventIdx;

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

/** ***********************************************************************************************
  * @brief      Search for an unused event
  * @param      pu08FoundEventIdx ID of the the found unused event
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
static rtcos_status_t _rtcos_find_empty_future_event_index(_u08 *pu08FoundEventIdx)
{
  _u08 u08Index;
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

/** ***********************************************************************************************
  * @brief      Schedule a future event if there is space in the future event
  *             array. First look if there is already a future event
  *             in the array, if not find an empty spot.
  * @param      u08TaskID ID of the task using this event
  * @param      u32EventFlags Bit feild event
  * @param      u32EventDelay How long to wait before sending event, if 0 send immediately
  * @param      bPeriodicEvent Indicates whether to send this event periodically or not
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
static rtcos_status_t _rtcos_add_future_event(_u08 u08TaskID,
                                              _u32 u32EventFlags,
                                              _u32 u32EventDelay,
                                              _bool bPeriodicEvent)
{
  _u08 u08FoundEventIdx;
  rtcos_status_t eRetVal;

  ENTER_CRITICAL_SECTION();
  eRetVal = _rtcos_find_future_event(u08TaskID, u32EventFlags, &u08FoundEventIdx);
  if(RTCOS_ERR_NONE == eRetVal)
  {
    stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u32EventDelay = u32EventDelay;
  }
  else
  {
    eRetVal = _rtcos_find_empty_future_event_index(&u08FoundEventIdx);
    if(RTCOS_ERR_NONE == eRetVal)
    {
      stRtcosCtx.tstFutureEvents[u08FoundEventIdx].bInUse = _TRUE;
      stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u32EventDelay = u32EventDelay;
      stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u08TaskID = u08TaskID;
      stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u32EventFlags = u32EventFlags;
      ++stRtcosCtx.u08FutureEventsCount;
      if(_TRUE == bPeriodicEvent)
      {   
        stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u32ReloadDelay = u32EventDelay;
      }
      else
      {
        stRtcosCtx.tstFutureEvents[u08FoundEventIdx].u32ReloadDelay = 0;
      }
    }
  }
  EXIT_CRITICAL_SECTION();
  return eRetVal;
}

/** ***********************************************************************************************
  * @brief      Find if there are any set events
  * @param      u32EventFlags Bit feild event
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
static rtcos_status_t _rtcos_count_events(_u32 u32EventFlags)
{
  return (u32EventFlags)?RTCOS_ERR_NONE:RTCOS_ERR_NO_EVENT;
}

/** ***********************************************************************************************
  * @brief      Check the inputs to the event routines
  * @param      u08TaskID ID of the task using this event
  * @param      u32EventFlags Bit feild event
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
static rtcos_status_t _rtcos_check_event_input(_u08 u08TaskID, _u32 u32EventFlags)
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

/*-----------------------------------------------------------------------------------------------*/
/* Exported functions                                                                            */
/*-----------------------------------------------------------------------------------------------*/
/** ***********************************************************************************************
  * @brief      Initialize rtcos main context, this should be called beffore any other os calls
  * @param      None
  * @return     Nothing
  ********************************************************************************************** */
void rtcos_init(void)
{
  _u08 u08Index;

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
    stRtcosCtx.tstFutureEvents[u08Index].u32EventDelay = 0;
    stRtcosCtx.tstFutureEvents[u08Index].u32ReloadDelay = 0;
  }
  for(u08Index = 0; u08Index < RTCOS_MAX_TASKS_COUNT; ++u08Index)
  {
    stRtcosCtx.tstTimers[u08Index].u32StartTickCount = 0;
    stRtcosCtx.tstTimers[u08Index].u32TickDelay = 0;
    stRtcosCtx.tstTimers[u08Index].bInUse = _FALSE;
    stRtcosCtx.tstTimers[u08Index].pfTimerCb = _NIL;
  }
  stRtcosCtx.u08CurrentTaskID = 0;
  stRtcosCtx.u32SysTicksCount = 0;
  stRtcosCtx.u08TasksCount = 0;
  stRtcosCtx.u08FutureEventsCount = 0;
  stRtcosCtx.pfIdleHandler = _NIL;
  stRtcosCtx.u08TimersCount = 0;
}

/** ***********************************************************************************************
  * @brief      Register a task handler if there is space
  * @param      pfTaskHandler task handler function
  * @param      u08TaskID ID of this task
  * @param      u32TaskParam Task parameter
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_register_task_handler(pf_task_handler_t pfTaskHandler,
                                           _u08 u08TaskID,
                                           _u32 u32TaskParam)
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

/** ***********************************************************************************************
  * @brief      Register an idle handler to be called when the system is idle
  * @param      pfIdleHandler idle handler function
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
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

/** ***********************************************************************************************
  * @brief      Send a message to a task
  * @param      u08TaskID ID of the task which will receive the message
  * @param      pMsg Pointer on the message to send
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_send_message(_u08 u08TaskID, void *pMsg)
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

/** ***********************************************************************************************
  * @brief      Retrieve a message from inside a task handler
  * @param      u08TaskID ID of the task which will receive the message
  * @param      ppMsg Pointer on a pointer to retrieved message
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_get_message(void **ppMsg)
{
  rtcos_status_t eRetVal;

  if(stRtcosCtx.u08CurrentTaskID < stRtcosCtx.u08TasksCount)
  {
    ENTER_CRITICAL_SECTION();
    eRetVal = _rtcos_fifo_pop(stRtcosCtx.u08CurrentTaskID, ppMsg);
    EXIT_CRITICAL_SECTION();
  }
  else
  {
    eRetVal = RTCOS_ERR_INVALID_TASK;
  }
  return eRetVal;
}

/** ***********************************************************************************************
  * @brief      Create an os software timer
  * @param      ePeriodType timer type as defined in ::rtcos_timer_type_t
  * @param      pfTimerCb Timer callback function
  * @param      pvArg Additional argument passed to the timer callback
  * @return     ID of the created timer or error
  ********************************************************************************************** */
_u08 rtcos_create_timer(rtcos_timer_type_t ePeriodType, pf_timer_cb_t pfTimerCb, void *pvArg)
{
  _u08 eRetVal;

  ENTER_CRITICAL_SECTION();
  if(stRtcosCtx.u08TimersCount >= RTCOS_MAX_TIMERS_COUNT)
  {
    eRetVal = RTCOS_ERR_OUT_OF_RESOURCES;
  }
  else
  {
    stRtcosCtx.tstTimers[stRtcosCtx.u08TimersCount].ePeriodType = ePeriodType;
    stRtcosCtx.tstTimers[stRtcosCtx.u08TimersCount].pfTimerCb = pfTimerCb;
    eRetVal = stRtcosCtx.u08TimersCount++;
  }
  EXIT_CRITICAL_SECTION();
  return eRetVal;
}

/** ***********************************************************************************************
  * @brief      Start os software timer
  * @param      u08TimerID ID of the timer to start
  * @param      u32PeriodInTicks Timer period in ticks
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_start_timer(_u08 u08TimerID, _u32 u32PeriodInTicks)
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
    stRtcosCtx.tstTimers[u08TimerID].u32StartTickCount = stRtcosCtx.u32SysTicksCount;
    stRtcosCtx.tstTimers[u08TimerID].bInUse = _TRUE;
    eRetVal = RTCOS_ERR_NONE;
  }
  EXIT_CRITICAL_SECTION();
  return eRetVal;
}

/** ***********************************************************************************************
  * @brief      Start os software timer
  * @param      u08TimerID ID of the timer to stop
  * @param      u32PeriodInTicks Timer period in ticks
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_stop_timer(_u08 u08TimerID)
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

/** ***********************************************************************************************
  * @brief      Check if the os software timer has expired
  * @param      u08TimerID ID of the timer to check
  * @return     _TRUE if timer has expired, else _FALSE
  ********************************************************************************************** */
_bool rtcos_timer_expired(_u08 u08TimerID)
{
  _u32 u32CurrentTicksCount;
  _bool bExpired;

  bExpired = _FALSE;
  u32CurrentTicksCount = stRtcosCtx.u32SysTicksCount;
  ENTER_CRITICAL_SECTION();
  if((stRtcosCtx.tstTimers[u08TimerID].bInUse) && (u08TimerID < RTCOS_MAX_TIMERS_COUNT))
  {
    if((u32CurrentTicksCount - stRtcosCtx.tstTimers[u08TimerID].u32StartTickCount) >
       (stRtcosCtx.tstTimers[u08TimerID].u32TickDelay))
    {
      bExpired = _TRUE;
    }
  }
  EXIT_CRITICAL_SECTION();
  return bExpired;
}

/** ***********************************************************************************************
  * @brief      Set an event for a certain task
  * @param      u08TaskID ID of the task which will receive the event
  * @param      u32EventFlags Bit feild event
  * @param      u32EventDelay How long to wait before sending event, if 0 send immediately
  * @param      bPeriodicEvent Indicates whether to send this event periodically or not
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_send_event(_u08 u08TaskID,
                                _u32 u32EventFlags,
                                _u32 u32EventDelay,
                                _bool bPeriodicEvent)
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

/** ***********************************************************************************************
  * @brief      Clear an event for a certain task, the event might be in
  *             the current event flags or in a future event.
  *             This will clear them in both places.
  * @param      u08TaskID ID of the task using the event
  * @param      u32EventFlags Bit feild event
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_clear_event(_u08 u08TaskID, _u32 u32EventFlags)
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

/** ***********************************************************************************************
  * @brief      Set the current tick count that is kept by the system.
  *             This can be used for testing purposes to check for an overflow.
  * @param      u32TickCount Number of ticks to set
  * @return     Nothing
  ********************************************************************************************** */
void rtcos_set_tick_count(_u32 u32TickCount)
{
  stRtcosCtx.u32SysTicksCount = u32TickCount;
}

/** ***********************************************************************************************
  * @brief      Get the current tick count that is kept by the system
  * @param      None
  * @return     Current number of ticks in the system
  ********************************************************************************************** */
_u32 rtcos_get_tick_count(void)
{
  _u32 u32CurrTickCount;

  ENTER_CRITICAL_SECTION();
  u32CurrTickCount = stRtcosCtx.u32SysTicksCount;
  EXIT_CRITICAL_SECTION();
  return u32CurrTickCount;
}

/** ***********************************************************************************************
  * @brief      Blocking delay
  * @param      u32DelayTicksCount Number of ticks to wait before unblocking
  * @return     Nothing
  ********************************************************************************************** */
void rtcos_delay(_u32 u32DelayTicksCount)
{
  _u32 u32Tick;

  u32Tick = rtcos_get_tick_count();
  while((rtcos_get_tick_count() - u32Tick) != u32DelayTicksCount);
}

/** ***********************************************************************************************
  * @brief      Find the highest priority task with some event.
  *             If found, call the task with the events.
  *             If no events or future events are in the system then the idle handler is called.
  * @param      None
  * @return     Nothing
  ********************************************************************************************** */
void rtcos_run(void)
{
  _bool bFoundReadyTask;
  _u08 u08NewCurrTaskID;

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

/** ***********************************************************************************************
  * @brief      This function should be called every time a tick occurs in the system.
  *             A tick is system dependent and is the measuring point
  *             for the delay of sending events.
  * @param      None
  * @return     Nothing
  ********************************************************************************************** */
void rtcos_update_tick(void)
{
  _u08 u08Index;
  
  ENTER_CRITICAL_SECTION();
  ++stRtcosCtx.u32SysTicksCount;
  for(u08Index = 0; u08Index < RTCOS_MAX_FUTURE_EVENTS_COUNT; ++u08Index)
  {
    if(_TRUE == stRtcosCtx.tstFutureEvents[u08Index].bInUse)
    {
      --stRtcosCtx.tstFutureEvents[u08Index].u32EventDelay; 
      if(0 == stRtcosCtx.tstFutureEvents[u08Index].u32EventDelay)
      {
        if(stRtcosCtx.u08FutureEventsCount > 0)
        {
          stRtcosCtx.u08FutureEventsCount--;
        }
        stRtcosCtx
          .tstTasks[stRtcosCtx.tstFutureEvents[u08Index].u08TaskID]
            .u32EventFlags |= stRtcosCtx.tstFutureEvents[u08Index].u32EventFlags;
        if(0 == stRtcosCtx.tstFutureEvents[u08Index].u32ReloadDelay)
        {
          stRtcosCtx.tstFutureEvents[u08Index].bInUse = _FALSE;
        }
        else
        {
          stRtcosCtx
            .tstFutureEvents[u08Index]
              .u32EventDelay = stRtcosCtx.tstFutureEvents[u08Index].u32ReloadDelay;
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
        if(RTCOS_TIMER_ONE_SHOT == stRtcosCtx.tstTimers[u08Index].ePeriodType)
        {
          stRtcosCtx.tstTimers[u08Index].bInUse = _FALSE;
        }
        stRtcosCtx.tstTimers[u08Index].u32StartTickCount = stRtcosCtx.u32SysTicksCount;
      }
    }
  }
  EXIT_CRITICAL_SECTION();
}
