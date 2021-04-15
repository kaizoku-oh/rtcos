/* 
 **************************************************************************************************
 *
 * @file    : main.cpp
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.2.0
 * @date    : April 2021
 * @brief   : Arduino example sketch
 * 
 **************************************************************************************************
 */

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include <Arduino.h>
#include <TimerOne.h>
#include <rtcos.h>

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
#define TASK_ID_PRIORITY_ONE                     (_u08)0
#define TASK_ID_PRIORITY_TWO                     (_u08)1
#define EVENT_PING                               (_u32)1
#define EVENT_PONG                               (_u32)2

/*-----------------------------------------------------------------------------------------------*/
/* Private function prototypes                                                                   */
/*-----------------------------------------------------------------------------------------------*/
static void _timer_isr(void);
static _u32 _task_one_handler(_u32 u32EventFlags, _u08 u08MsgCount, _u32 u32Param);
static _u32 _task_two_handler(_u32 u32EventFlags, _u08 u08MsgCount, _u32 u32Param);
static void _on_os_timer_expired(void const *);

/*-----------------------------------------------------------------------------------------------*/
/* Private variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/
static _u08 u08LedState = LOW;

/** ***********************************************************************************************
  * @brief      Runs once to setup hardware and os
  * @return     Returns nothing
  ********************************************************************************************** */
void setup()
{
  _u08 u08OsTimerID;

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(_timer_isr);

  rtcos_init();
  rtcos_register_task_handler(_task_one_handler, TASK_ID_PRIORITY_ONE, (_u32)0);
  rtcos_register_task_handler(_task_two_handler, TASK_ID_PRIORITY_TWO, (_u32)0);
  rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, (_u32)0, FALSE);
  rtcos_send_message(TASK_ID_PRIORITY_TWO, (void *)"Hello");
  u08OsTimerID = rtcos_create_timer(RTCOS_TIMER_PERIODIC, _on_os_timer_expired, 0);
  rtcos_start_timer(u08OsTimerID, 100);

  rtcos_run();
}

/** ***********************************************************************************************
  * @brief      Runs repeadetly, not used here
  * @return     Returns nothing
  ********************************************************************************************** */
void loop()
{}

/** ***********************************************************************************************
  * @brief      Arduino hardware timer ISR, used here to increment the os tick
  * @return     Returns nothing
  ********************************************************************************************** */
static void _timer_isr(void)
{
  rtcos_update_tick();
}

/** ***********************************************************************************************
  * @brief      OS software timer callback
  * @param      pvArg Additional argument passed to the timer callback
  * @return     Returns nothing
  ********************************************************************************************** */
static void _on_os_timer_expired(void const *pvArg)
{
  u08LedState = !u08LedState;
  digitalWrite(LED_BUILTIN, u08LedState); 
}

/** ***********************************************************************************************
  * @brief      Task handler function
  * @param      u32EventFlags Bit feild event
  * @param      u08MsgCount number of messages belonging to this task
  * @param      u32Param Task parameter
  * @return     Return unhandled events
  ********************************************************************************************** */
static _u32 _task_one_handler(_u32 u32EventFlags, _u08 u08MsgCount, _u32 u32Param)
{
  _u32 u32RetVal;

  u32RetVal = 0;
  /* To allow executing higher priority tasks we just handle one event then return */
  if(u32EventFlags & EVENT_PING)
  {
    /*
     * Get events that have NOT been handled to return them to scheduler.
     * To make the system more responsive the task should only handle
     * the highest priority data and then return to the OS.
     * It should not try to handle all its outstanding data
     * otherwise a higher priority task might be kept from running.
     */
    Serial.println("Task one received PING event!");
    /* Send a future pong event to task two */
    rtcos_send_event(TASK_ID_PRIORITY_TWO, EVENT_PONG, 1000, FALSE);
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_PING;
  }
  return u32RetVal;
}

/** ***********************************************************************************************
  * @brief      Task handler function
  * @param      u32EventFlags Bit feild event
  * @param      u08MsgCount number of messages belonging to this task
  * @param      u32Param Task parameter
  * @return     Return unhandled events
  ********************************************************************************************** */
static _u32 _task_two_handler(_u32 u32EventFlags, _u08 u08MsgCount, _u32 u32Param)
{
  _u32 u32RetVal;
  _char *pcMessage;

  u32RetVal = 0;
  /* To allow executing higher priority tasks we just handle one event then return */
  if(u32EventFlags & EVENT_PONG)
  {
    /*
     * Get events that have NOT been handled to return them to scheduler.
     * To make the system more responsive the task should only handle
     * the highest priority data and then return to the OS.
     * It should not try to handle all its outstanding data
     * otherwise a higher priority task might be kept from running.
     */
    Serial.println("Task two received PONG event!");
    /* Send a future ping event to task one */
    rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, 1000, FALSE);
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_PONG;
  }
  if(u08MsgCount)
  {
    if(RTCOS_ERR_NONE == rtcos_get_message((void **)&pcMessage))
    {
      Serial.print("Task two received a message: ");
      Serial.println(pcMessage);
    }
  }
  return u32RetVal;
}