/******************************************************************************
    Filename: rtcOS.h
    Description: 
        This file contains the interface to the RtcOS system.
        Run To Completion OS allows each "task" to run until completed.
        RtcOS allows the creation of tasks which can send
        and receive events from other tasks or interrupt service 
        routines.  A task runs till completion, which means it
        handles the event sent to it.  Tasks should only handled
        one event and then return to the system.  If that task
        is still the highest priority, then it will be called
        again with the other outstanding events.  Again, the task
        should handle one event and then return.  This allows
        a higher priority task to run if it has events.  A task
        can prioritize its own events depending on the order
        they are processed in their event handler routine.

        The event system is very simple in that you can send one
        event at a time.  Events can be sent immediately to the receiving 
        task or they can be delayed by some amount of ticks.  A tick
        duration varies from system to system.  It depends on
        how often the osUpdateTick() is called.  If it is called
        every millisecond then a tick is one millisecond.  If it is 
        called every 10 milliseconds then a tick is 10 milliseconds.

        When an event has a delay associated with it, then the
        caller can also specify if they want the event to be
        sent continually or just once.  This is determined by
        reloadDelay paramter in osSendEvent().  This parameter
        is not valid for events sent immediately.

        To stop an event from being sent you can clear the event, which
        will clear the immediate or future event.  So if you have
        a continually sending event, you can stop it with the
        osClearEvent function.

        When no events are in the system then in theory the processor
        could stop or go to sleep until an interrupt arrives.  If you
        want the system to sleep then you must register a sleep routine
        with the system that will be called if there are no events
        in the system.
    
    Written: Jon Norenberg, 2013 - 2016
    Copyright: My code is your code.  No warranties. 
******************************************************************************/

#ifndef RTC_OS_H
#define RTC_OS_H

//******************************************************************************
// Includes
//******************************************************************************
#include "board_arduino_uno.h"

//******************************************************************************
// OS return codes
//******************************************************************************
#define OS_ERR_NONE                     0
#define OS_ERR_OUT_OF_RESOURCES         -1
#define OS_ERR_IN_USE                   -2
#define OS_ERR_OUT_OF_RANGE             -3
#define OS_ERR_NOT_FOUND                -4
#define OS_ERR_TOO_MANY_EVENTS          -5
#define OS_ERR_NO_EVENT                 -6
#define OS_ERR_INVALID_TASK             -7
#define OS_ERR_MSG_FULL                 -8
#define OS_ERR_MSG_EMPTY                -9

//******************************************************************************
// Typedefs
//******************************************************************************

typedef struct _osTimer_t
{
	uint32			mStartTickCount;
	uint32			mTickDelay;
} osTimer_t, *osTimerPtr_t;


typedef osEvents_t (*pTaskEventHandler_t)( osEvents_t eventFlags, osMsgCount_t msgCount, osTaskParam_t taskParam );
typedef void (*pSystemSleepHandler_t)( void );

//******************************************************************************
// Prototypes
//******************************************************************************

// call this routine after the basic hardware has been initialized
osStatus_t osInit( void );

// create tasks using this routine
osStatus_t osRegisterTaskEventHandler( pTaskEventHandler_t taskEventHandler, osTaskID_t taskID,
                                     osTaskParam_t taskParam );

// register a callback that will be invoked when the system has no events or messages
osStatus_t osRegisterSystemSleepHandler( pSystemSleepHandler_t sleepHandler );

// main() should call this as the last function, it will never return
void osRun( void );

// this routine must be called every tick, could be 1 millisecond, whatever the system wants
// the rate at which you call this routine determines send event delay time 
void osUpdateTick( void );

// events
// set tickCountDelay to 0 to send event immediately
// set reloadDelay to TRUE in order to send the event again in tickCountDelay,
// this event will continually be sent until it is cleared.
// if tickCountDelay is 0 then reloadDelay is ignored
osStatus_t osSendEvent( osTaskID_t taskID, osEvents_t eventFlag, osTick_t tickCountDelay, bool reloadDelay );
osStatus_t osClearEvent( osTaskID_t taskID, osEvents_t eventFlag );

// time
// create a timer to check if x amount of time has passed since
// the timer was created.  Call osTimerExpired to find out
// if the time has expired.
void osSetTickCount( osTick_t newCount );
osTick_t osGetTickCount( void );
osStatus_t osTimerCreate( osTick_t expireTickCount, osTimer_t *newTimer );
// return true if expired
bool osTimerExpired( osTimer_t *newTimer );


#if (defined MAX_MESSAGES_IN_SYSTEM) && (MAX_MESSAGES_IN_SYSTEM > 0)
// messages
// send a pointer to a message to a task
osStatus_t osSendMessage( osTaskID_t taskID, osMsg_t pMsg );
osStatus_t osGetMessage( osMsg_t *pMsg );
#endif

#endif
