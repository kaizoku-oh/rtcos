/* 
 **************************************************************************************************
 *
 * @file    : rtcos.c
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.3.0
 * @date    : April 2021
 * @brief   : RTCOS source file
 * 
 **************************************************************************************************
 */

/** @addtogroup rtcos
  * @{
  */
/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
/** @defgroup rtcos_private_includes Private includes
  * @{
  */
#include "rtcos.h"
#include "config.h"
/**
  * @}
  */

/*-----------------------------------------------------------------------------------------------*/
/* Private types                                                                                 */
/*-----------------------------------------------------------------------------------------------*/
/** @defgroup rtcos_private_types Private types
  * @{
  */
#ifdef RTCOS_ENABLE_MESSAGES
/** Fifo structure used for storing messages */
typedef struct
{
  _u08 u08Head;                                  /**< Fifo head position                         */
  _u08 u08Tail;                                  /**< Fifo tail position                         */
  _u08 u08Count;                                 /**< Fifo current count                         */
  void *tpvBuffer[RTCOS_MAX_MESSAGES_COUNT];     /**< Fifo buffer of pointers                    */
}rtcos_fifo_t;
#endif /* RTCOS_ENABLE_MESSAGES */

/** Future event structure representing information about each event */
typedef struct
{
  _u32 u32EventFlags;                            /**< 32 bits representing different events      */
  volatile _u32 u32EventDelay;                   /**< Delay to wait before handling the event    */
  _u32 u32ReloadDelay;                           /**< Delay to wait before reloading the event   */
  _u08 u08TaskID;                                /**< ID of the task associated with this event  */
  volatile _bool bInUse;                         /**< Indicates if the event is still used       */
}rtcos_future_event_t;

#ifdef RTCOS_ENABLE_TIMERS
/** Software os timer structure representing information about each timer */
typedef struct
{
  volatile _bool bInUse;                         /**< Indicates if the timer is still used       */
  rtcos_timer_type_t ePeriodType;                /**< Periodic or one shot timer                 */
  volatile _u32	u32StartTickCount;               /**< Start time of the timer                    */
  _u32 u32TickDelay;                             /**< Period of the timer                        */
  pf_os_timer_cb_t pfTimerCb;                    /**< Timer callback function                    */
  void *pvArg;                                   /**< Timer callback argument                    */
}rtcos_timer_t;
#endif /* RTCOS_ENABLE_TIMERS */

/** Task structure representing information about each task */
typedef struct
{
  volatile _u32 u32EventFlags;                   /**< Event flags associated to this task        */
  pf_os_task_handler_t pfTaskHandlerCb;          /**< Task handler function                      */
   void *pvArg;                                  /**< Task argument                              */
#ifdef RTCOS_ENABLE_MESSAGES
  rtcos_fifo_t stFifo;                           /**< Fifo associated to this task               */
#endif /* RTCOS_ENABLE_MESSAGES */
}rtcos_task_t;

/** Context structure representing the main context of the OS */
typedef struct
{
  _u08 u08TasksCount;                            /**< Number of the tasks present in the system  */
  _u08 u08CurrentTaskID;                         /**< Current task ID                            */
  volatile _u32 u32SysTicksCount;                /**< Current number of the system ticks         */
  pf_os_idle_handler_t pfIdleHandler;            /**< Handler function when the system is Idle   */
  volatile _u08 u08FutureEventsCount;            /**< Number of the events present in the system */
  rtcos_future_event_t tstFutureEvents[RTCOS_MAX_FUTURE_EVENTS_COUNT]; /**< Array of events      */
  rtcos_task_t tstTasks[RTCOS_MAX_TASKS_COUNT];  /**< Array of tasks                             */
#ifdef RTCOS_ENABLE_TIMERS
  rtcos_timer_t tstTimers[RTCOS_MAX_TIMERS_COUNT]; /**< Array of timers                          */
  _u08 u08TimersCount;                           /**< Number of the timers present in the system */
#endif /* RTCOS_ENABLE_TIMERS */
}rtcos_main_t;
/**
  * @}
  */

/*-----------------------------------------------------------------------------------------------*/
/* Private variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/
/** @defgroup rtcos_private_variables Private variables
  * @{
  */
static rtcos_main_t RTCOSi_stMain;
/**
  * @}
  */

/*-----------------------------------------------------------------------------------------------*/
/* Private functions                                                                             */
/*-----------------------------------------------------------------------------------------------*/
/** @defgroup rtcos_private_functions Private functions
  * @{
  */
#ifdef RTCOS_ENABLE_MESSAGES
/** ***********************************************************************************************
  * @brief      Initialize the fifo that will hold a task's messages
  * @param      u08TaskID ID of the task using this fifo
  * @return     Nothing
  ********************************************************************************************** */
static void _rtcos_fifo_init(_u08 u08TaskID)
{
  RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Head = 0;
  RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Tail = 0;
  RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Count = 0;
}

/** ***********************************************************************************************
  * @brief      Check if the fifo of a certain task is empty
  * @param      u08TaskID ID of the task using this fifo
  * @return     TRUE if empty, else FALSE
  ********************************************************************************************** */
static _bool _rtcos_fifo_empty(_u08 u08TaskID)
{
  return (RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Count > 0)?FALSE:TRUE;
}

/** ***********************************************************************************************
  * @brief      Check if the fifo of a certain task is full
  * @param      u08TaskID ID of the task using this fifo
  * @return     TRUE if full, else FALSE
  ********************************************************************************************** */
static _bool _rtcos_fifo_full(_u08 u08TaskID)
{
  return (RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Count >= RTCOS_MAX_MESSAGES_COUNT)?TRUE:FALSE;
}

/** ***********************************************************************************************
  * @brief      Get the number of items in the fifo
  * @param      u08TaskID ID of the task using this fifo
  * @return     Number of items in the fifo
  ********************************************************************************************** */
static _u08 _rtcos_fifo_count(_u08 u08TaskID)
{
  return RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Count;
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

  if(FALSE == _rtcos_fifo_full(u08TaskID))
  {
    RTCOSi_stMain
      .tstTasks[u08TaskID]
        .stFifo.tpvBuffer[RTCOSi_stMain.tstTasks[u08TaskID]
          .stFifo.u08Head++] = pvMsg;
    ++RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Count;
    if(RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Head >= RTCOS_MAX_MESSAGES_COUNT)
    {
      RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Head = 0;
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

  if(FALSE == _rtcos_fifo_empty(u08TaskID))
  {
    *ppvMsg = RTCOSi_stMain
                .tstTasks[u08TaskID]
                  .stFifo.tpvBuffer[RTCOSi_stMain.tstTasks[u08TaskID]
                    .stFifo.u08Tail++];
    --RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Count;
    if(RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Tail >= RTCOS_MAX_MESSAGES_COUNT)
    {
      RTCOSi_stMain.tstTasks[u08TaskID].stFifo.u08Tail = 0;
    }
    eRetVal = RTCOS_ERR_NONE;
  }
  else
  {
    eRetVal = RTCOS_ERR_MSG_EMPTY;
  }
  return eRetVal;
}
#endif /* RTCOS_ENABLE_MESSAGES */

/** ***********************************************************************************************
  * @brief      Find the highest priority task with an event or a message
  * @param      pu08NewCurrTaskID This will hold the ID of the found ready task
  * @return     TRUE if a task is found, else FALSE
  ********************************************************************************************** */
static _bool _rtcos_find_ready_task(_u08 *pu08NewCurrTaskID)
{
  _u08 u08Index;
  _bool bRetVal;

  bRetVal = FALSE;
  for(u08Index = 0; u08Index < RTCOSi_stMain.u08TasksCount; ++u08Index)
  {
    if((0 != RTCOSi_stMain.tstTasks[u08Index].u32EventFlags)
#ifdef RTCOS_ENABLE_MESSAGES
       || (FALSE == _rtcos_fifo_empty(u08Index))
#endif /* RTCOS_ENABLE_MESSAGES */
      )
    {
      *pu08NewCurrTaskID = u08Index;
      bRetVal = TRUE;
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

  RTCOS_ENTER_CRITICAL_SECTION();
  RTCOSi_stMain.u08CurrentTaskID = u08NewCurrTaskID;
  u32CurrentEvents = RTCOSi_stMain.tstTasks[RTCOSi_stMain.u08CurrentTaskID].u32EventFlags;
  RTCOSi_stMain.tstTasks[RTCOSi_stMain.u08CurrentTaskID].u32EventFlags = 0;  
  RTCOS_EXIT_CRITICAL_SECTION();
  u32UnhandledEvents = (RTCOSi_stMain.tstTasks[RTCOSi_stMain.u08CurrentTaskID].pfTaskHandlerCb)
                       (u32CurrentEvents,
#ifdef RTCOS_ENABLE_MESSAGES
                        _rtcos_fifo_count(RTCOSi_stMain.u08CurrentTaskID),
#else
                        0,
#endif /* RTCOS_ENABLE_MESSAGES */
                        RTCOSi_stMain.tstTasks[RTCOSi_stMain.u08CurrentTaskID].pvArg);
  RTCOS_ENTER_CRITICAL_SECTION();
  RTCOSi_stMain.tstTasks[RTCOSi_stMain.u08CurrentTaskID].u32EventFlags |= u32UnhandledEvents;
  RTCOS_EXIT_CRITICAL_SECTION();
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
    if((TRUE == RTCOSi_stMain.tstFutureEvents[u08Index].bInUse) &&
       (RTCOSi_stMain.tstFutureEvents[u08Index].u08TaskID == u08TaskID) && 
       (RTCOSi_stMain.tstFutureEvents[u08Index].u32EventFlags == u32EventFlags))
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
    RTCOSi_stMain.tstFutureEvents[u08FoundEventIdx].bInUse = FALSE;
    if(RTCOSi_stMain.u08FutureEventsCount > 0)
    {
      RTCOSi_stMain.u08FutureEventsCount--;
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
    if(FALSE == RTCOSi_stMain.tstFutureEvents[u08Index].bInUse)
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

  RTCOS_ENTER_CRITICAL_SECTION();
  eRetVal = _rtcos_find_future_event(u08TaskID, u32EventFlags, &u08FoundEventIdx);
  if(RTCOS_ERR_NONE == eRetVal)
  {
    RTCOSi_stMain.tstFutureEvents[u08FoundEventIdx].u32EventDelay = u32EventDelay;
  }
  else
  {
    eRetVal = _rtcos_find_empty_future_event_index(&u08FoundEventIdx);
    if(RTCOS_ERR_NONE == eRetVal)
    {
      RTCOSi_stMain.tstFutureEvents[u08FoundEventIdx].bInUse = TRUE;
      RTCOSi_stMain.tstFutureEvents[u08FoundEventIdx].u32EventDelay = u32EventDelay;
      RTCOSi_stMain.tstFutureEvents[u08FoundEventIdx].u08TaskID = u08TaskID;
      RTCOSi_stMain.tstFutureEvents[u08FoundEventIdx].u32EventFlags = u32EventFlags;
      ++RTCOSi_stMain.u08FutureEventsCount;
      if(TRUE == bPeriodicEvent)
      {   
        RTCOSi_stMain.tstFutureEvents[u08FoundEventIdx].u32ReloadDelay = u32EventDelay;
      }
      else
      {
        RTCOSi_stMain.tstFutureEvents[u08FoundEventIdx].u32ReloadDelay = 0;
      }
    }
  }
  RTCOS_EXIT_CRITICAL_SECTION();
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
    if(u08TaskID >= RTCOSi_stMain.u08TasksCount)
    {
      eRetVal = RTCOS_ERR_INVALID_TASK;
    }
  }
  return eRetVal;
}
/**
  * @}
  */

/*-----------------------------------------------------------------------------------------------*/
/* Exported functions                                                                            */
/*-----------------------------------------------------------------------------------------------*/
/** @defgroup rtcos_exported_functions Exported functions
  * @{
  */
/** ***********************************************************************************************
  * @brief      Initialize rtcos main context, this should be called beffore any other os calls
  * @return     Nothing
  ********************************************************************************************** */
void rtcos_init(void)
{
  _u08 u08Index;

  for(u08Index = 0; u08Index < RTCOS_MAX_TASKS_COUNT; ++u08Index)
  {
    RTCOSi_stMain.tstTasks[u08Index].pfTaskHandlerCb = NIL;
    RTCOSi_stMain.tstTasks[u08Index].u32EventFlags = 0;
#ifdef RTCOS_ENABLE_MESSAGES
    _rtcos_fifo_init(u08Index);
#endif /* RTCOS_ENABLE_MESSAGES */
  }
  for(u08Index = 0; u08Index < RTCOS_MAX_FUTURE_EVENTS_COUNT; ++u08Index)
  {
    RTCOSi_stMain.tstFutureEvents[u08Index].bInUse = FALSE;
    RTCOSi_stMain.tstFutureEvents[u08Index].u08TaskID = 0;
    RTCOSi_stMain.tstFutureEvents[u08Index].u32EventFlags = 0;
    RTCOSi_stMain.tstFutureEvents[u08Index].u32EventDelay = 0;
    RTCOSi_stMain.tstFutureEvents[u08Index].u32ReloadDelay = 0;
  }
#ifdef RTCOS_ENABLE_TIMERS
  for(u08Index = 0; u08Index < RTCOS_MAX_TASKS_COUNT; ++u08Index)
  {
    RTCOSi_stMain.tstTimers[u08Index].u32StartTickCount = 0;
    RTCOSi_stMain.tstTimers[u08Index].u32TickDelay = 0;
    RTCOSi_stMain.tstTimers[u08Index].bInUse = FALSE;
    RTCOSi_stMain.tstTimers[u08Index].pfTimerCb = NIL;
    RTCOSi_stMain.tstTimers[u08Index].pvArg = NIL;
  }
  RTCOSi_stMain.u08TimersCount = 0;
#endif /* RTCOS_ENABLE_TIMERS */
  RTCOSi_stMain.u08CurrentTaskID = 0;
  RTCOSi_stMain.u32SysTicksCount = 0;
  RTCOSi_stMain.u08FutureEventsCount = 0;
  RTCOSi_stMain.pfIdleHandler = NIL;
}

/** ***********************************************************************************************
  * @brief      Register a task handler if there is space
  * @param      pfTaskHandler task handler function
  * @param      u08TaskID ID of this task
  * @param      pvArg Task argument
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_register_task_handler(pf_os_task_handler_t pfTaskHandler,
                                           _u08 u08TaskID,
                                           void *pvArg)
{
  rtcos_status_t eRetVal;

  if(u08TaskID < RTCOS_MAX_TASKS_COUNT)
  {
    if(RTCOSi_stMain.tstTasks[u08TaskID].pfTaskHandlerCb != NIL)
    {
      eRetVal = RTCOS_ERR_IN_USE;
    }
    else
    {
      RTCOSi_stMain.tstTasks[u08TaskID].pfTaskHandlerCb = pfTaskHandler;
      RTCOSi_stMain.tstTasks[u08TaskID].pvArg = pvArg;
      ++RTCOSi_stMain.u08TasksCount;
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
rtcos_status_t rtcos_register_idle_handler(pf_os_idle_handler_t pfIdleHandler)
{
  rtcos_status_t eRetVal;

  if(pfIdleHandler)
  {
    RTCOSi_stMain.pfIdleHandler = pfIdleHandler; 
    eRetVal = RTCOS_ERR_NONE;
  }
  else
  {
    eRetVal = RTCOS_ERR_ARG;
  }
  return eRetVal;
}

#ifdef RTCOS_ENABLE_MESSAGES
/** ***********************************************************************************************
  * @brief      Send a message to a task
  * @param      u08TaskID ID of the task which will receive the message
  * @param      pvMsg Pointer on the message to send
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_send_message(_u08 u08TaskID, void *pvMsg)
{
  rtcos_status_t eRetVal;

  if(pvMsg)
  {
    if(u08TaskID < RTCOSi_stMain.u08TasksCount)
    {
      RTCOS_ENTER_CRITICAL_SECTION();
      eRetVal = _rtcos_fifo_push(u08TaskID, pvMsg);
      RTCOS_EXIT_CRITICAL_SECTION();
    }
    else
    {
      eRetVal = RTCOS_ERR_INVALID_TASK;
    }
  }
  else
  {
    eRetVal = RTCOS_ERR_ARG;
  }
  return eRetVal;
}

/** ***********************************************************************************************
  * @brief      Send a message to all tasks
  * @param      pvMsg Pointer on the message to send
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_broadcast_message(void *pvMsg)
{
  _u08 u08Index;
  rtcos_status_t eRetVal;

  eRetVal = RTCOS_ERR_NONE;
  if(pvMsg)
  {
    for(u08Index = 0; u08Index < RTCOSi_stMain.u08TasksCount; ++u08Index)
    {
      eRetVal |= rtcos_send_message(u08Index, pvMsg);
    }
  }
  else
  {
    eRetVal = RTCOS_ERR_ARG;
  }
  return eRetVal;
}

/** ***********************************************************************************************
  * @brief      Retrieve a message from inside a task handler
  * @param      ppvMsg Pointer on a pointer to retrieved message
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_get_message(void **ppvMsg)
{
  rtcos_status_t eRetVal;

  if(RTCOSi_stMain.u08CurrentTaskID < RTCOSi_stMain.u08TasksCount)
  {
    RTCOS_ENTER_CRITICAL_SECTION();
    eRetVal = _rtcos_fifo_pop(RTCOSi_stMain.u08CurrentTaskID, ppvMsg);
    RTCOS_EXIT_CRITICAL_SECTION();
  }
  else
  {
    eRetVal = RTCOS_ERR_INVALID_TASK;
  }
  return eRetVal;
}
#endif /* RTCOS_ENABLE_MESSAGES */

#ifdef RTCOS_ENABLE_TIMERS
/** ***********************************************************************************************
  * @brief      Create an os software timer
  * @param      ePeriodType timer type as defined in ::rtcos_timer_type_t
  * @param      pfTimerCb Timer callback function
  * @param      pvArg Additional argument passed to the timer callback
  * @return     ID of the created timer or error
  ********************************************************************************************** */
_s08 rtcos_create_timer(rtcos_timer_type_t ePeriodType, pf_os_timer_cb_t pfTimerCb, void *pvArg)
{
  _s08 s08RetVal;

  RTCOS_ENTER_CRITICAL_SECTION();
  if(RTCOSi_stMain.u08TimersCount >= RTCOS_MAX_TIMERS_COUNT)
  {
    s08RetVal = RTCOS_ERR_OUT_OF_RESOURCES;
  }
  else
  {
    RTCOSi_stMain.tstTimers[RTCOSi_stMain.u08TimersCount].ePeriodType = ePeriodType;
    RTCOSi_stMain.tstTimers[RTCOSi_stMain.u08TimersCount].pfTimerCb = pfTimerCb;
    RTCOSi_stMain.tstTimers[RTCOSi_stMain.u08TimersCount].pvArg = pvArg;
    s08RetVal = RTCOSi_stMain.u08TimersCount++;
  }
  RTCOS_EXIT_CRITICAL_SECTION();
  return s08RetVal;
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

  RTCOS_ENTER_CRITICAL_SECTION();
  if(u08TimerID >= RTCOS_MAX_TIMERS_COUNT)
  {
    eRetVal = RTCOS_ERR_OUT_OF_RESOURCES;
  }
  else
  {
    RTCOSi_stMain.tstTimers[u08TimerID].u32TickDelay = u32PeriodInTicks;
    RTCOSi_stMain.tstTimers[u08TimerID].u32StartTickCount = RTCOSi_stMain.u32SysTicksCount;
    RTCOSi_stMain.tstTimers[u08TimerID].bInUse = TRUE;
    eRetVal = RTCOS_ERR_NONE;
  }
  RTCOS_EXIT_CRITICAL_SECTION();
  return eRetVal;
}

/** ***********************************************************************************************
  * @brief      Start os software timer
  * @param      u08TimerID ID of the timer to stop
  * @return     Status as defined in ::rtcos_status_t
  ********************************************************************************************** */
rtcos_status_t rtcos_stop_timer(_u08 u08TimerID)
{
  rtcos_status_t eRetVal;

  RTCOS_ENTER_CRITICAL_SECTION();
  if(u08TimerID >= RTCOS_MAX_TIMERS_COUNT)
  {
    eRetVal = RTCOS_ERR_OUT_OF_RESOURCES;
  }
  else
  {
    RTCOSi_stMain.tstTimers[u08TimerID].bInUse = FALSE;
    eRetVal = RTCOS_ERR_NONE;
  }
  RTCOS_EXIT_CRITICAL_SECTION();
  return eRetVal;
}

/** ***********************************************************************************************
  * @brief      Check if the os software timer has expired
  * @param      u08TimerID ID of the timer to check
  * @return     TRUE if timer has expired, else FALSE
  ********************************************************************************************** */
_bool rtcos_timer_expired(_u08 u08TimerID)
{
  _u32 u32CurrentTicksCount;
  _bool bExpired;

  bExpired = FALSE;
  u32CurrentTicksCount = RTCOSi_stMain.u32SysTicksCount;
  RTCOS_ENTER_CRITICAL_SECTION();
  if((RTCOSi_stMain.tstTimers[u08TimerID].bInUse) && (u08TimerID < RTCOS_MAX_TIMERS_COUNT))
  {
    if((u32CurrentTicksCount - RTCOSi_stMain.tstTimers[u08TimerID].u32StartTickCount) >
       (RTCOSi_stMain.tstTimers[u08TimerID].u32TickDelay))
    {
      bExpired = TRUE;
    }
  }
  RTCOS_EXIT_CRITICAL_SECTION();
  return bExpired;
}
#endif /* RTCOS_ENABLE_TIMERS */

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
      RTCOS_ENTER_CRITICAL_SECTION();
      RTCOSi_stMain.tstTasks[u08TaskID].u32EventFlags |= u32EventFlags;
      RTCOS_EXIT_CRITICAL_SECTION();
    }
    else
    {
      eRetVal = _rtcos_add_future_event(u08TaskID, u32EventFlags, u32EventDelay, bPeriodicEvent);
    }
  }
  return eRetVal;
}

/** ***********************************************************************************************
  * @brief      Set an event to all tasks
  * @param      u32EventFlags Bit feild event
  * @param      u32EventDelay How long to wait before sending event, if 0 send immediately
  * @param      bPeriodicEvent Indicates whether to send this event periodically or not
  * @return     RTCOS_ERR_NONE if everything is ok
  ********************************************************************************************** */
rtcos_status_t rtcos_broadcast_event(_u32 u32EventFlags, _u32 u32EventDelay, _bool bPeriodicEvent)
{
  _u08 u08Index;
  rtcos_status_t eRetVal;

  eRetVal = RTCOS_ERR_NONE;
  for(u08Index = 0; u08Index < RTCOSi_stMain.u08TasksCount; ++u08Index)
  {
    eRetVal |= rtcos_send_event(u08Index, u32EventFlags, u32EventDelay, bPeriodicEvent);
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
    RTCOS_ENTER_CRITICAL_SECTION();
    RTCOSi_stMain.tstTasks[u08TaskID].u32EventFlags &= ~(u32EventFlags);
    _rtcos_delete_future_event(u08TaskID, u32EventFlags); 
    RTCOS_EXIT_CRITICAL_SECTION();
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
  RTCOSi_stMain.u32SysTicksCount = u32TickCount;
}

/** ***********************************************************************************************
  * @brief      Get the current tick count that is kept by the system
  * @return     Current number of ticks in the system
  ********************************************************************************************** */
_u32 rtcos_get_tick_count(void)
{
  _u32 u32CurrTickCount;

  RTCOS_ENTER_CRITICAL_SECTION();
  u32CurrTickCount = RTCOSi_stMain.u32SysTicksCount;
  RTCOS_EXIT_CRITICAL_SECTION();
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
  * @return     Nothing
  ********************************************************************************************** */
void rtcos_run(void)
{
  _bool bFoundReadyTask;
  _u08 u08NewCurrTaskID;

  while(1)
  {
    RTCOS_ENTER_CRITICAL_SECTION();
    bFoundReadyTask = _rtcos_find_ready_task(&u08NewCurrTaskID);
    RTCOS_EXIT_CRITICAL_SECTION();
    if(TRUE == bFoundReadyTask)
    {
      _rtcos_run_ready_task(u08NewCurrTaskID);
    }
    else if((NIL != RTCOSi_stMain.pfIdleHandler) &&
            (0 == RTCOSi_stMain.u08FutureEventsCount))
    {
      (RTCOSi_stMain.pfIdleHandler)();
    }
  }
}

/** ***********************************************************************************************
  * @brief      This function should be called every time a tick occurs in the system.
  *             A tick is system dependent and is the measuring point
  *             for the delay of sending events.
  * @return     Nothing
  ********************************************************************************************** */
void rtcos_update_tick(void)
{
  _u08 u08Index;
  
  RTCOS_ENTER_CRITICAL_SECTION();
  ++RTCOSi_stMain.u32SysTicksCount;
  for(u08Index = 0; u08Index < RTCOS_MAX_FUTURE_EVENTS_COUNT; ++u08Index)
  {
    if(TRUE == RTCOSi_stMain.tstFutureEvents[u08Index].bInUse)
    {
      --RTCOSi_stMain.tstFutureEvents[u08Index].u32EventDelay; 
      if(0 == RTCOSi_stMain.tstFutureEvents[u08Index].u32EventDelay)
      {
        if(RTCOSi_stMain.u08FutureEventsCount > 0)
        {
          RTCOSi_stMain.u08FutureEventsCount--;
        }
        RTCOSi_stMain
          .tstTasks[RTCOSi_stMain.tstFutureEvents[u08Index].u08TaskID]
            .u32EventFlags |= RTCOSi_stMain.tstFutureEvents[u08Index].u32EventFlags;
        if(0 == RTCOSi_stMain.tstFutureEvents[u08Index].u32ReloadDelay)
        {
          RTCOSi_stMain.tstFutureEvents[u08Index].bInUse = FALSE;
        }
        else
        {
          RTCOSi_stMain
            .tstFutureEvents[u08Index]
              .u32EventDelay = RTCOSi_stMain.tstFutureEvents[u08Index].u32ReloadDelay;
        }
      }
    }
  }
#ifdef RTCOS_ENABLE_TIMERS
  if(RTCOSi_stMain.u08TimersCount > 0)
  {
    for(u08Index = 0; u08Index < RTCOSi_stMain.u08TimersCount; u08Index++)
    {
      if(rtcos_timer_expired(u08Index))
      {
        if(RTCOSi_stMain.tstTimers[u08Index].pfTimerCb)
        {
          RTCOSi_stMain.tstTimers[u08Index].pfTimerCb(RTCOSi_stMain.tstTimers[u08Index].pvArg);
        }
        if(RTCOS_TIMER_ONE_SHOT == RTCOSi_stMain.tstTimers[u08Index].ePeriodType)
        {
          RTCOSi_stMain.tstTimers[u08Index].bInUse = FALSE;
        }
        RTCOSi_stMain.tstTimers[u08Index].u32StartTickCount = RTCOSi_stMain.u32SysTicksCount;
      }
    }
  }
#endif /* RTCOS_ENABLE_TIMERS */
  RTCOS_EXIT_CRITICAL_SECTION();
}
/**
  * @}
  */

/**
  * @}
  */