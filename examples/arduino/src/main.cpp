/* 
 **************************************************************************************************
 *
 * @file    : main.cpp
 * @author  : Bayrem GHARSELLAOUI
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
#include <string.h>

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
#define TASK_ID_PRIORITY_ONE                     (uint8_t)0
#define TASK_ID_PRIORITY_TWO                     (uint8_t)1
#define EVENT_PING                               (uint32_t)1
#define EVENT_PONG                               (uint32_t)2
#define EVENT_COMMON                             (uint32_t)3
#define SERIAL_BAUDRATE                          9600
#define HARDWARE_TIMER_PERIOD_IN_US              1000
#define SOFTWARE_TIMER_PERIOD_IN_MS              100

/*-----------------------------------------------------------------------------------------------*/
/* Private function prototypes                                                                   */
/*-----------------------------------------------------------------------------------------------*/
static void _timer_isr(void);
static uint32_t _task_one_handler(uint32_t u32EventFlags, uint8_t u08MsgCount, void const *pvArg);
static uint32_t _task_two_handler(uint32_t u32EventFlags, uint8_t u08MsgCount, void const *pvArg);
static void _on_os_timer_expired(void const *);

/*-----------------------------------------------------------------------------------------------*/
/* Private variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/
static uint8_t u08LedState;

/*-----------------------------------------------------------------------------------------------*/
/* Exported functions                                                                            */
/*-----------------------------------------------------------------------------------------------*/
/** ***********************************************************************************************
  * @brief      Runs once to setup hardware and os
  * @return     Returns nothing
  ********************************************************************************************** */
void setup()
{
  uint8_t u08OsTimerID;

  u08LedState = LOW;
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, u08LedState); 

  Serial.begin(SERIAL_BAUDRATE);
  Timer1.initialize(HARDWARE_TIMER_PERIOD_IN_US);
  Timer1.attachInterrupt(_timer_isr);

  rtcos_init();
  rtcos_register_task_handler(_task_one_handler, TASK_ID_PRIORITY_ONE, (void *)"TaskOne");
  rtcos_register_task_handler(_task_two_handler, TASK_ID_PRIORITY_TWO, (void *)"TaskTwo");
  rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, (uint32_t)0, false);
  u08OsTimerID = rtcos_create_timer(RTCOS_TIMER_PERIODIC, _on_os_timer_expired, (void *)"blink");
  rtcos_start_timer(u08OsTimerID, SOFTWARE_TIMER_PERIOD_IN_MS);
  rtcos_broadcast_event(EVENT_COMMON, 0, false);
  rtcos_broadcast_message((void *)"Hello");

  rtcos_run();
}

/** ***********************************************************************************************
  * @brief      Runs repeatedly, not used here
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
  if(0 == strcmp("blink", (char *)pvArg))
  {
    u08LedState = !u08LedState;
    digitalWrite(LED_BUILTIN, u08LedState);
  }
}

/** ***********************************************************************************************
  * @brief      Task handler function
  * @param      u32EventFlags Bit feild event
  * @param      u08MsgCount number of messages belonging to this task
  * @param      pvArg Task argument
  * @return     Return unhandled events
  ********************************************************************************************** */
static uint32_t _task_one_handler(uint32_t u32EventFlags, uint8_t u08MsgCount, void const *pvArg)
{
  uint32_t u32RetVal;
  char *pcMessage;

  u32RetVal = 0;
  Serial.print("Task one argument is: ");
  Serial.println((char *)pvArg);
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
    rtcos_send_event(TASK_ID_PRIORITY_TWO, EVENT_PONG, 1000, false);
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_PING;
  }
  else if(u32EventFlags & EVENT_COMMON)
  {
    Serial.println("Task one received a broadcasted event: EVENT_COMMON");
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_COMMON;
  }
  if(u08MsgCount)
  {
    if(RTCOS_ERR_NONE == rtcos_get_message((void **)&pcMessage))
    {
      Serial.print("Task one received a broadcasted message: ");
      Serial.println(pcMessage);
    }
  }
  return u32RetVal;
}

/** ***********************************************************************************************
  * @brief      Task handler function
  * @param      u32EventFlags Bit feild event
  * @param      u08MsgCount number of messages belonging to this task
  * @param      pvArg Task argument
  * @return     Return unhandled events
  ********************************************************************************************** */
static uint32_t _task_two_handler(uint32_t u32EventFlags, uint8_t u08MsgCount, void const *pvArg)
{
  uint32_t u32RetVal;
  char *pcMessage;

  u32RetVal = 0;
  Serial.print("Task two argument is: ");
  Serial.println((char *)pvArg);
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
    rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, 1000, false);
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_PONG;
  }
  else if(u32EventFlags & EVENT_COMMON)
  {
    Serial.println("Task two received a broadcasted event: EVENT_COMMON");
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_COMMON;
  }
  if(u08MsgCount)
  {
    if(RTCOS_ERR_NONE == rtcos_get_message((void **)&pcMessage))
    {
      Serial.print("Task two received a broadcasted message: ");
      Serial.println(pcMessage);
    }
  }
  return u32RetVal;
}