/******************************************************************************
    Filename: rtcOS.c
    Description: 
        This file contains the simple tasking system that allows
        events and messages to be sent to tasks.  When a task 
        is called for an event/message then it will run to completion
        and then return to the scheduler.

        The implementation has the task info in an array and the max is
        defined at compile time.  When control returns back to the 
        scheduler it looks through the event flags of each task
        starting with the highest priority.  The first task it
        finds with events or messages, it will call the task handler.
        Pretty basic.
    
    Written: Jon Norenberg, 2013 - 2016
    Copyright: My code is your code.  No warranties. 
******************************************************************************/

//******************************************************************************
// Includes
//******************************************************************************
#include "stdio.h"

#include "rtcos.h"

//******************************************************************************
// Constants
//******************************************************************************

//******************************************************************************
// Typedefs
//******************************************************************************
#if (defined MAX_MESSAGES_IN_SYSTEM) && (MAX_MESSAGES_IN_SYSTEM > 0)
// each task has a message fifo that tracks all the messages for a task
typedef struct _OSFIFO_t
{
    osIndex_t       mHead;
    osIndex_t       mTail;
    osIndex_t       mCount;
    osMsg_t     	mBuffer[MAX_MESSAGES_IN_SYSTEM];
} OSFIFO_t, *OSFIFOPTR_t;
#endif


// stores information about a event to send in the future
typedef struct _FutureEventInfo_t
{
    osEvents_t          mEventFlags;
    osTick_t            mTimeout;
    osTick_t            mReloadTimeout;
    osTaskID_t          mTaskID;
    bool                mInUse;
} FutureEventInfo_t;


// keeps track of information about each task in the system
typedef struct _TaskInfo_t
{
    osEvents_t              mEventFlags;
    pTaskEventHandler_t     mEventHandler;
    osTaskParam_t           mTaskParam;
#if (defined MAX_MESSAGES_IN_SYSTEM) && (MAX_MESSAGES_IN_SYSTEM > 0)
    OSFIFO_t                mFifo;   
#endif
} TaskInfo_t;
 
// information about the entire system in one structure
typedef struct _OSInfo_t
{
    // current number of tasks in the system
    osIndex_t               mTaskCount;    
    // current number of future events in the system
    osTaskID_t              mCurrTask;
    // tracks "time" in the system
    osTick_t                mSystemTickCount;
    // stores the client routine to be called when the system can sleep
    pSystemSleepHandler_t   mClientSystemSleepHandler;
    // current number of future events in the system
    osIndex_t               mFutureEventCount;
    // an array of all the future events to send
    FutureEventInfo_t       mFutureEventArray[MAX_FUTURE_EVENTS];
    // an array of information about each task
    TaskInfo_t              mTaskInfoArray[MAX_TASKS_IN_SYSTEM];
    
} OSInfo_t;

//******************************************************************************
// Globals
//******************************************************************************

// contains the status of the os, tasks, events, messages, ...
static OSInfo_t gOS;

//******************************************************************************
// Prototypes
//******************************************************************************
#if (defined MAX_MESSAGES_IN_SYSTEM) && (MAX_MESSAGES_IN_SYSTEM > 0)
static osStatus_t os_FIFOInit( osTaskID_t taskID );
static bool os_FIFOEmpty( osTaskID_t taskID );
static int os_GetFIFOCount( osTaskID_t taskID );

#endif


/******************************************************************************
    @fn     osInit

    @brief  System init should before any other os call.

    @param  
            
    @return osStatus_t
******************************************************************************/
osStatus_t osInit( void )
{
	osIndex_t taskIdx, futureEventIdx;
    
    // clear out the task info
    for ( taskIdx = 0; taskIdx < MAX_TASKS_IN_SYSTEM; ++taskIdx )
    {
        // clear the task event processing routine, ie no task here
        gOS.mTaskInfoArray[taskIdx].mEventHandler = NULL;
        // clear any task events
        gOS.mTaskInfoArray[taskIdx].mEventFlags = 0;
#if (defined MAX_MESSAGES_IN_SYSTEM) && (MAX_MESSAGES_IN_SYSTEM > 0)
        os_FIFOInit( taskIdx );    
#endif
    }
    
    // clear out the future event info
    for ( futureEventIdx = 0; futureEventIdx < MAX_FUTURE_EVENTS; ++futureEventIdx )
    {
        gOS.mFutureEventArray[futureEventIdx].mInUse = FALSE;
        gOS.mFutureEventArray[futureEventIdx].mTaskID = 0;
        gOS.mFutureEventArray[futureEventIdx].mEventFlags = 0;
        gOS.mFutureEventArray[futureEventIdx].mTimeout = 0;
        gOS.mFutureEventArray[futureEventIdx].mReloadTimeout = 0;
    }

    // init a variety of global variables
    gOS.mCurrTask = 0;
    gOS.mSystemTickCount = 0;
    gOS.mTaskCount = 0;
    gOS.mFutureEventCount = 0;
    gOS.mClientSystemSleepHandler = NULL;
        
    return OS_ERR_NONE;
}

/******************************************************************************
    @fn     osRegisterTaskEventHandler

    @brief  System init should call this for each task.  An error
            is returned if the task slot is already occupied.

    @param  taskEventHandler - pointer to task event handler
            
    @return osStatus_t
******************************************************************************/
osStatus_t osRegisterTaskEventHandler( pTaskEventHandler_t taskEventHandler, osTaskID_t taskID,
                                     osTaskParam_t taskParam )
{
    osStatus_t retCode;
    
    if ( taskID >= MAX_TASKS_IN_SYSTEM )
    {
        return OS_ERR_OUT_OF_RANGE;    
    }
    
    if ( gOS.mTaskInfoArray[taskID].mEventHandler != NULL )
    {
        // already have a task assigned to this slot
        retCode = OS_ERR_IN_USE;
    }
    else
    {
        // still space for the task
        gOS.mTaskInfoArray[taskID].mEventHandler = taskEventHandler;
        gOS.mTaskInfoArray[taskID].mTaskParam = taskParam;
        ++gOS.mTaskCount;
        retCode = OS_ERR_NONE;
    }
    
    return retCode;
}

/******************************************************************************
    @fn     osRegisterSystemSleepHandler

    @brief  System init should call this to install a callback for
            when the system can go to sleep.

    @param  taskEventHandler - pointer to task event handler
            
    @return  osStatus_t
******************************************************************************/
osStatus_t osRegisterSystemSleepHandler( pSystemSleepHandler_t sleepHandler )
{
    gOS.mClientSystemSleepHandler = sleepHandler;    
    
    return OS_ERR_NONE;
}


/******************************************************************************
    @fn     os_FindReadyTask

    @brief  This function finds the highest priority task with some
            event or message.

    @param  
            
    return  TRUE, if it finds a ready task, FALSE otherwise  
******************************************************************************/
static bool os_FindReadyTask( osTaskID_t *newCurrTask )
{
    osIndex_t taskIdx;
    
    // find the first task that has events
    for ( taskIdx = 0; ( taskIdx < gOS.mTaskCount ); ++taskIdx )
    {
#if (defined MAX_MESSAGES_IN_SYSTEM) && (MAX_MESSAGES_IN_SYSTEM > 0)
        if (( gOS.mTaskInfoArray[ taskIdx ].mEventFlags != 0 ) ||
            ( FALSE == os_FIFOEmpty( taskIdx )))
#else                
        if ( gOS.mTaskInfoArray[ taskIdx ].mEventFlags != 0 )
#endif  
        {   // this task has either an event or message
            *newCurrTask = taskIdx;
            return TRUE;
        }
    }
    
    return FALSE;
}

/******************************************************************************
    @fn     os_RunReadyTask

    @brief  This function runs the task that has the highest priority
            and is ready.

    @param  
            
    return  
******************************************************************************/
static void os_RunReadyTask( osTaskID_t newCurrTask )
{
    osEvents_t unhandledEvents, currentEvents;
    CRITICAL_SECTION_VARIABLE;

    ENTER_CRITICAL_SECTION;
    gOS.mCurrTask = newCurrTask;
    currentEvents = gOS.mTaskInfoArray[ gOS.mCurrTask ].mEventFlags;
    // clear all events so that if new ones come in via an ISR they can be saved
    gOS.mTaskInfoArray[ gOS.mCurrTask ].mEventFlags = 0;  
    EXIT_CRITICAL_SECTION;

    // calling the task with events for it
    // returns the task events that were NOT handled
#if (defined MAX_MESSAGES_IN_SYSTEM) && (MAX_MESSAGES_IN_SYSTEM > 0)
    unhandledEvents = ( gOS.mTaskInfoArray[gOS.mCurrTask].mEventHandler )( currentEvents,
                         os_GetFIFOCount( gOS.mCurrTask ) , gOS.mTaskInfoArray[gOS.mCurrTask].mTaskParam );
#else
    unhandledEvents = ( gOS.mTaskInfoArray[gOS.mCurrTask].mEventHandler )( currentEvents,
                         0 , gOS.mTaskInfoArray[gOS.mCurrTask].mTaskParam );
#endif

    ENTER_CRITICAL_SECTION;
    // some new events might have come in during the task call
    // so add them to the events that were NOT handled
    gOS.mTaskInfoArray[ gOS.mCurrTask ].mEventFlags |= unhandledEvents;  
    EXIT_CRITICAL_SECTION;
}

/******************************************************************************
    @fn     osRun

    @brief  This function finds the highest priority task with some event.
            When it finds a task with events, calls the task with the
            events.  If no events or future events are in the sytem
            then the client's sleep routine can be called.

    @param  
            
    return  
******************************************************************************/
void osRun( void )
{
    bool foundReadyTask;
    osTaskID_t newCurrTask;
    CRITICAL_SECTION_VARIABLE;

    // infinite loop for this application
    // spin here and call the task's that have events
    while ( 1 )
    {   
        ENTER_CRITICAL_SECTION;
        foundReadyTask = os_FindReadyTask( &newCurrTask );
        EXIT_CRITICAL_SECTION;
        
        if ( TRUE == foundReadyTask )
        {
            os_RunReadyTask( newCurrTask );
        }
        else if (( NULL != gOS.mClientSystemSleepHandler ) && 
                 ( 0 == gOS.mFutureEventCount ))
        {   // no events were found in the system, we could go to sleep here
            ( gOS.mClientSystemSleepHandler )();      
        }
    }
}

/******************************************************************************
    @fn     osUpdateTick

    @brief  This routine should be called every time a tick occurs in the
            system.  A tick is system dependent and is the measuring
            point for delay of sending events.

    @param  
            
    @return   
******************************************************************************/
void osUpdateTick( void )
{
	osIndex_t idx;
    CRITICAL_SECTION_VARIABLE;
    
    ENTER_CRITICAL_SECTION;
    ++gOS.mSystemTickCount;
    
    for( idx = 0; idx < MAX_FUTURE_EVENTS; ++idx )
    {   // look through the array and every in use slot
        // needs to decrement the delay and see if it it time
        // to send the event by moving the event to the current
        // event array
        if ( gOS.mFutureEventArray[idx].mInUse == TRUE )
        {
            --gOS.mFutureEventArray[idx].mTimeout; 
            if ( gOS.mFutureEventArray[idx].mTimeout == 0 )
            {
                // transfer the future event to the now event since the delay is 0
                gOS.mTaskInfoArray[gOS.mFutureEventArray[idx].mTaskID].mEventFlags |= 
                    gOS.mFutureEventArray[idx].mEventFlags;
                if ( gOS.mFutureEventArray[idx].mReloadTimeout == 0 )
                {   // disable the slot, so that it can be reused
                    gOS.mFutureEventArray[idx].mInUse = FALSE;
                }
                else // reload tick count is non zero so reload the time
                {   // event will be sent again in x ticks
                    gOS.mFutureEventArray[idx].mTimeout = gOS.mFutureEventArray[idx].mReloadTimeout;    
                }
            }
        }
    }
    
    EXIT_CRITICAL_SECTION;
}

/******************************************************************************
    @fn     os_FindFutureEvent

    @brief  Search through future event array and the task + event combo.

    @param  taskID - task to send the event to
            eventFlag - bit flag event
            foundSlot - pointer to place to put the index in the array
            
    @return osStatus_t  
******************************************************************************/
static osStatus_t os_FindFutureEvent( osTaskID_t taskID, osEvents_t eventFlag, osIndex_t *foundSlot )
{
	osIndex_t idx;

    for( idx = 0; idx < MAX_FUTURE_EVENTS; ++idx )
    {   // look through the array to find the task + event combo
        if (( gOS.mFutureEventArray[idx].mInUse == TRUE ) &&
            ( gOS.mFutureEventArray[idx].mTaskID == taskID ) && 
            ( gOS.mFutureEventArray[idx].mEventFlags == eventFlag ))
        {
            *foundSlot = idx;
            return  OS_ERR_NONE;   
        }
    }
    
    return OS_ERR_NOT_FOUND;
}

/******************************************************************************
    @fn     os_DeleteFutureEvent

    @brief  Find a certain future event and delete the future event
            which is the same as deleting it..

    @param  taskID - task to clear the event
            eventFlag - bit flag event
            
    @return osStatus_t  
******************************************************************************/
static osStatus_t os_DeleteFutureEvent( osTaskID_t taskID, osEvents_t eventFlag )
{
    osStatus_t retCode;
    osIndex_t foundSlot;

    retCode = os_FindFutureEvent( taskID, eventFlag, &foundSlot );
    if ( retCode == OS_ERR_NONE )
    {   // found the future event
        // disable/delete by setting the slot inUse to FALSE
        gOS.mFutureEventArray[foundSlot].mInUse = FALSE;
        --gOS.mFutureEventCount;
        return OS_ERR_NONE;
    }

    return OS_ERR_NOT_FOUND;
}    

/******************************************************************************
    @fn     os_FindEmptyFutureSlot

    @brief  Search through the future event array to find an unused slot.

    @param  foundSlot - pointer to place to put the index of the free slot
            
    @return osStatus_t  
******************************************************************************/
static osStatus_t os_FindEmptyFutureSlot( osIndex_t *foundSlot )
{
    osIndex_t idx;

    for( idx = 0; idx < MAX_FUTURE_EVENTS; ++idx )
    {   // look through the array to find an unused slot
        if ( gOS.mFutureEventArray[idx].mInUse == FALSE )
        {
            *foundSlot = idx;
            return  OS_ERR_NONE;   
        }
    }
    
    return OS_ERR_NOT_FOUND;
}

/******************************************************************************
    @fn     os_AddFutureEvent

    @brief  Schedule a future event if there is space in the future event
            array.  First look if there is already a future event
            in the array, if not find an empty spot.

    @param  taskID - task to send the event to
            eventFlag - bit flag event
            delayTime - how long to wait before sending event, if 0
                        send immediately
            
    @return osStatus_t  
******************************************************************************/
static osStatus_t os_AddFutureEvent( osTaskID_t taskID, osEvents_t eventFlag, osTick_t delayTime, bool reloadDelay )
{
    osIndex_t futureSlot;
    osStatus_t retCode;
    CRITICAL_SECTION_VARIABLE;
    
    ENTER_CRITICAL_SECTION;
    retCode = os_FindFutureEvent( taskID, eventFlag, &futureSlot );
    if ( retCode == OS_ERR_NONE )
    {   // found the task + event combo in the array
        // just reset the delay to the new value
        gOS.mFutureEventArray[ futureSlot ].mTimeout = delayTime;    
    }
    else    // task + event combo was not in the future event array
    {
        retCode = os_FindEmptyFutureSlot( &futureSlot ); 
        if ( retCode == OS_ERR_NONE )
        {   // found a free slot in the array
            gOS.mFutureEventArray[ futureSlot ].mInUse = TRUE;    
            gOS.mFutureEventArray[ futureSlot ].mTimeout = delayTime;    
            gOS.mFutureEventArray[ futureSlot ].mTaskID = taskID;    
            gOS.mFutureEventArray[ futureSlot ].mEventFlags = eventFlag; 
            ++gOS.mFutureEventCount;
            if ( reloadDelay == TRUE )
            {   
                gOS.mFutureEventArray[ futureSlot ].mReloadTimeout = delayTime; 
            }
            else
            {
                gOS.mFutureEventArray[ futureSlot ].mReloadTimeout = 0; 
            }
        }
    }

    EXIT_CRITICAL_SECTION;

    return retCode;
}

/******************************************************************************
    @fn     os_CountEvents

    @brief  Count the number of event flags set.  There should only be one.
            Return an error if no events are set and if they are
            more than 1 set.

    @param  eventFlag - bit flag event
            
    @return osStatus_t  
******************************************************************************/
static osStatus_t os_CountEvents( osEvents_t eventFlag )
{
    int idx;
    int bitCount = 0;

    // walk through the event bit flags and count them
    for ( idx = 0; ( idx < sizeof( osEvents_t )) && ( eventFlag ); ++idx )
    {
        if ( eventFlag & ( 1 << idx ))
        {   // bit is set
            ++bitCount;
        }
    }

    if ( bitCount == 0 )
    {   // must have at least one event flag set
        return OS_ERR_NO_EVENT;
    }
    
    if ( bitCount == 1 )
    {   // ahh, just the right number
        return OS_ERR_NONE;
    }
    
    // more than 1 event flag was set
    return OS_ERR_TOO_MANY_EVENTS;   
}

/******************************************************************************
    @fn     os_CheckEventInput

    @brief  Check the inputs to the event routines.

    @param  taskID - task to send the event to
            eventFlag - bit flag event
            
    @return osStatus_t  
******************************************************************************/
static osStatus_t os_CheckEventInput( osTaskID_t taskID, osEvents_t eventFlag )
{
    osStatus_t retCode;
    
    retCode = os_CountEvents( eventFlag );
    if ( retCode != OS_ERR_NONE )
    {   // more than one event flag is set
        return retCode;
    }

    if ( taskID >= gOS.mTaskCount )
    {   // bad task number
        return OS_ERR_INVALID_TASK;
    }
    
    return OS_ERR_NONE;
}

/******************************************************************************
    @fn     osSendEvent

    @brief  Set an event for a certain task.

    @param  taskID - task to send the event to
            eventFlag - bit flag event
            delayTime - how long to wait before sending event, if 0
                        send immediately
            
    @return osStatus_t  
******************************************************************************/
osStatus_t osSendEvent( osTaskID_t taskID, osEvents_t eventFlag, osTick_t tickCountDelay, bool reloadDelay )
{
    osStatus_t retCode;
    CRITICAL_SECTION_VARIABLE;

    retCode = os_CheckEventInput( taskID, eventFlag );
    if ( retCode != OS_ERR_NONE )
    {   // problems with the input parameters
        return retCode;
    }

    if ( tickCountDelay == 0 )
    {   // send event immediately since there is no delay ticks
        ENTER_CRITICAL_SECTION;
        gOS.mTaskInfoArray[taskID].mEventFlags |= eventFlag;  
        EXIT_CRITICAL_SECTION;
        retCode = OS_ERR_NONE;
    }
    else    // delayed event
    {   // must add it to the future event system
        retCode = os_AddFutureEvent( taskID, eventFlag, tickCountDelay, reloadDelay );    
    }
    
    return retCode;
}

/******************************************************************************
    @fn     osClearEvent

    @brief  Clear an event for a certain task, the event might be in
            the current event flags or in a future event.  Must
            clear them in both places.

    @param  taskID - task to clear the event
            eventFlag - bit flag event
            
    @return osStatus_t  
******************************************************************************/
osStatus_t osClearEvent( osTaskID_t taskID, osEvents_t eventFlag )
{
    osStatus_t retCode;
    CRITICAL_SECTION_VARIABLE;
    
    retCode = os_CheckEventInput( taskID, eventFlag );
    if ( retCode != OS_ERR_NONE )
    {   // problems with the input parameters
        return retCode;
    }

    ENTER_CRITICAL_SECTION;
    // clear the current event flags
    gOS.mTaskInfoArray[taskID].mEventFlags &= ~( eventFlag );  
    // clear the future event flags
    os_DeleteFutureEvent( taskID, eventFlag ); 
    EXIT_CRITICAL_SECTION;
    
    return OS_ERR_NONE;
}

#if 1
/******************************************************************************
    @fn     osSetTickCount

    @brief  Set the current tick count that is kept by the system.
			FOR TESTING ONLY, TO CHECK OVERFLOW

    @param  newCount - value for the tick count
            
    @return   
******************************************************************************/
void osSetTickCount( osTick_t newCount )
{
	gOS.mSystemTickCount = newCount;
}
#endif

/******************************************************************************
    @fn     osGetTickCount

    @brief  Get the current tick count that is kept by the system.

    @param  
            
    @return osTick_t, current tick count since boot  
******************************************************************************/
osTick_t osGetTickCount( void )
{
    osTick_t currTickCount;
    
    CRITICAL_SECTION_VARIABLE;
    
    ENTER_CRITICAL_SECTION;
    
    currTickCount =  gOS.mSystemTickCount;
    
    EXIT_CRITICAL_SECTION;
        
    return currTickCount;
}


/******************************************************************************
    @fn     osTimerCreate

    @brief  Create a tick count that is in the future.  
			As long as the interval between two related events is not 
			larger than the range of the uint32 there is no problem at all.
			If you  keep the values as uInt32 and subtract the earlier event 
			time from the current event time with the subtract function 
			you will get the correct interval anyhow even if there has 
			been a counter turn over. This is a feature of proper integer 
			arithmetic implementation as specified by IEEE and LabVIEW implements 
			that too. Just try it out by subtracting 4294967295 from 10 
			both set as unsigned int32 and you will see the result to be 
			11 which is the difference between the two numbers when looked 
			at as unsigned int32.

    @param  expireTickCount - ticks before timer will expire
            newTimer - a os timer that track the expire time
            
    @return osStatus_t  
******************************************************************************/
osStatus_t osTimerCreate( osTick_t expireTickCount, osTimer_t *newTimer )
{
    CRITICAL_SECTION_VARIABLE;
    
    ENTER_CRITICAL_SECTION;

    if ( newTimer != NULL )
    {
        newTimer->mTickDelay = expireTickCount;
        newTimer->mStartTickCount = gOS.mSystemTickCount;
    }
    
    EXIT_CRITICAL_SECTION;
        
    return OS_ERR_NONE;
}

/******************************************************************************
    @fn     osTimerExpired

    @brief  Client calls this to check if the timer has expired.

    @param  newTimer - contains the tick count of the expiration
            
    @return osStatus_t  
******************************************************************************/
bool osTimerExpired( osTimer_t *newTimer )
{
    bool expired = FALSE;
    CRITICAL_SECTION_VARIABLE;
   
    ENTER_CRITICAL_SECTION;
 
    if (( gOS.mSystemTickCount - newTimer->mStartTickCount ) > newTimer->mTickDelay )
    {
        expired = TRUE;
    }
    
    EXIT_CRITICAL_SECTION;

    return expired;
}

#if (defined MAX_MESSAGES_IN_SYSTEM) && (MAX_MESSAGES_IN_SYSTEM > 0)

/******************************************************************************
    @fn     os_FIFOInit

    @brief  Init the fifo that will hold a task's messages.

    @param  

    @return osStatus_t  
******************************************************************************/
static osStatus_t os_FIFOInit( osTaskID_t taskID )
{
    
    gOS.mTaskInfoArray[ taskID ].mFifo.mHead = 0;    
    gOS.mTaskInfoArray[ taskID ].mFifo.mTail = 0;
    gOS.mTaskInfoArray[ taskID ].mFifo.mCount = 0;
    
    return OS_ERR_NONE;    
}

/******************************************************************************
    @fn     os_FIFOEmpty

    @brief  If count is 0 then the fifo is empty.

    @param  taskID - task id

    @return TRUE, if empty  
******************************************************************************/
static bool os_FIFOEmpty( osTaskID_t taskID )
{
    return ( gOS.mTaskInfoArray[ taskID ].mFifo.mCount ? FALSE : TRUE );
}

/******************************************************************************
    @fn     os_FIFOFull

    @brief  If count is equal to the total capacity.

    @param  taskID - task id

    @return TRUE, if full  
******************************************************************************/
static bool os_FIFOFull( osTaskID_t taskID )
{
    return ( gOS.mTaskInfoArray[ taskID ].mFifo.mCount >= MAX_MESSAGES_IN_SYSTEM );
}

/******************************************************************************
    @fn     os_GetFIFOCount

    @brief  Get the number of items in the FIFO.

    @param  taskID - task id

    @return count  
******************************************************************************/
static int os_GetFIFOCount( osTaskID_t taskID )
{
    return ( gOS.mTaskInfoArray[ taskID ].mFifo.mCount );
}

/******************************************************************************
    @fn     os_FIFOPush

    @brief  Put a message on the fifo for a certain task.

    @param  taskID - task that receives the message
            msg - put this on the fifo

    @return osStatus_t  
******************************************************************************/
static osStatus_t os_FIFOPush( osTaskID_t taskID, osMsg_t msg )
{
    osStatus_t retCode;

    if ( os_FIFOFull( taskID ) == FALSE )
    {   // there is space in the fifo
        gOS.mTaskInfoArray[ taskID ].mFifo.mBuffer[ gOS.mTaskInfoArray[ taskID ].mFifo.mHead++ ] = msg;
        ++gOS.mTaskInfoArray[ taskID ].mFifo.mCount;
        if ( gOS.mTaskInfoArray[ taskID ].mFifo.mHead >= MAX_MESSAGES_IN_SYSTEM )
        {   // must wrap the head
            gOS.mTaskInfoArray[ taskID ].mFifo.mHead = 0;
        }
        retCode = OS_ERR_NONE;
    }
    else
    {
        retCode = OS_ERR_MSG_FULL; 
    }   

    return retCode;    
}

/******************************************************************************
    @fn     os_FIFOPop

    @brief  Retrieve a message for a certain task.

    @param  taskID - task ID that should have a message waiting
            pMsg - location to put the retrieve message

    @return osStatus_t  
******************************************************************************/
static osStatus_t os_FIFOPop( osTaskID_t taskID, osMsg_t *pMsg )
{
    osStatus_t retCode;
    
    if ( os_FIFOEmpty( taskID ) == FALSE )
    {   // there is something in the fifo to pop
        *pMsg = gOS.mTaskInfoArray[ taskID ].mFifo.mBuffer[ gOS.mTaskInfoArray[ taskID ].mFifo.mTail++ ];
        --gOS.mTaskInfoArray[ taskID ].mFifo.mCount;
        if ( gOS.mTaskInfoArray[ taskID ].mFifo.mTail >= MAX_MESSAGES_IN_SYSTEM )
        {   // must wrap the tail
            gOS.mTaskInfoArray[ taskID ].mFifo.mTail = 0;
        }
        retCode = OS_ERR_NONE;
    }
    else
    {
        retCode = OS_ERR_MSG_EMPTY; 
    }   
    return retCode;    
}

/******************************************************************************
    @fn     osSendMessage

    @brief  Send a message to a task.

    @param  taskID - destination task ID
            pMsg - pointer to message to send to the task

    @return osStatus_t  
******************************************************************************/
osStatus_t osSendMessage( osTaskID_t taskID, osMsg_t pMsg )
{
    osStatus_t retCode;
    CRITICAL_SECTION_VARIABLE;
    
    if ( taskID >= gOS.mTaskCount )
    {   // bad task number
        return OS_ERR_INVALID_TASK;
    }

    ENTER_CRITICAL_SECTION;
    retCode = os_FIFOPush( taskID, pMsg );
    EXIT_CRITICAL_SECTION;

    return retCode;
}

/******************************************************************************
    @fn     osGetMessage

    @brief  Retrieve a message for this task.

    @param  taskID - destination task ID
            pMsg - pointer to message to send to the task

    @return osStatus_t  
******************************************************************************/
osStatus_t osGetMessage( osMsg_t *pMsg )
{
    osStatus_t retCode;
    CRITICAL_SECTION_VARIABLE;
    
    if ( gOS.mCurrTask >= gOS.mTaskCount )
    {   // bad task number
        return OS_ERR_INVALID_TASK;
    }
    
    ENTER_CRITICAL_SECTION;
    retCode = os_FIFOPop( gOS.mCurrTask, pMsg );
    EXIT_CRITICAL_SECTION;

    return retCode;
}
#endif
