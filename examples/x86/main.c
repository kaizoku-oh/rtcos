/* 
 **************************************************************************************************
 *
 * @file    : main.c
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.3.0
 * @date    : April 2021
 * @brief   : x86 example program
 * 
 **************************************************************************************************
 */

/*-----------------------------------------------------------------------------------------------*/
/* Includes                                                                                      */
/*-----------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <time.h>
#include "rtcos.h"

/*-----------------------------------------------------------------------------------------------*/
/* Defines                                                                                       */
/*-----------------------------------------------------------------------------------------------*/
#define TASK_ID_PRIORITY_ONE                     (_u08)0
#define TASK_ID_PRIORITY_TWO                     (_u08)1
#define EVENT_PING                               (_u32)1
#define EVENT_PONG                               (_u32)2
#define EVENT_COMMON                             (_u32)3

/*-----------------------------------------------------------------------------------------------*/
/* Private function prototypes                                                                   */
/*-----------------------------------------------------------------------------------------------*/
static void delay(_u32 u32Seconds);
static _u32 _task_one_handler(_u32 u32EventFlags, _u08 u08MsgCount, void const *pvArg);
static _u32 _task_two_handler(_u32 u32EventFlags, _u08 u08MsgCount, void const *pvArg);

/** ***********************************************************************************************
  * @brief      Program entry point
  * @return     Return nothing
  ********************************************************************************************** */
int main(void)
{
  rtcos_init();

  rtcos_register_task_handler(_task_one_handler, TASK_ID_PRIORITY_ONE, (void *)"TaskOne");
  rtcos_register_task_handler(_task_two_handler, TASK_ID_PRIORITY_TWO, (void *)"TaskTwo");

  rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, (_u32)0, FALSE);
  rtcos_broadcast_message((void *)"Hello");
  rtcos_broadcast_event(EVENT_COMMON, 0, FALSE);

  rtcos_run();
  return 0;
}

/** ***********************************************************************************************
  * @brief      Task handler function
  * @param      u32EventFlags Bit feild event
  * @param      u08MsgCount number of messages belonging to this task
  * @param      pvArg Task argument
  * @return     Return unhandled events
  ********************************************************************************************** */
static _u32 _task_one_handler(_u32 u32EventFlags, _u08 u08MsgCount, void const *pvArg)
{
  _u32 u32RetVal;
  _char *pcMessage;

  u32RetVal = 0;
  printf("Task one argument is: %s\r\n", (_char *)pvArg);
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
    printf("Task one received PING event!\r\n");
    /* Send immediate pong event to task two */
    rtcos_send_event(TASK_ID_PRIORITY_TWO, EVENT_PONG, 0, FALSE);
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_PING;
  }
  else if(u32EventFlags & EVENT_COMMON)
  {
    printf("Task one received a broadcasted event: EVENT_COMMON\r\n");
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_COMMON;
  }
  if(u08MsgCount)
  {
    if(RTCOS_ERR_NONE == rtcos_get_message((void **)&pcMessage))
    {
      printf("Task one received a broadcasted message: %s\r\n", pcMessage);
    }
  }
  /* This delay is added only for testing purposes under x86 */
  delay(1000);
  return u32RetVal;
}

/** ***********************************************************************************************
  * @brief      Task handler function
  * @param      u32EventFlags Bit feild event
  * @param      u08MsgCount number of messages belonging to this task
  * @param      pvArg Task argument
  * @return     Return unhandled events
  ********************************************************************************************** */
static _u32 _task_two_handler(_u32 u32EventFlags, _u08 u08MsgCount, void const *pvArg)
{
  _u32 u32RetVal;
  _char *pcMessage;

  u32RetVal = 0;
  printf("Task two argument is: %s\r\n", (_char *)pvArg);
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
    printf("Task two received PONG event!\r\n");
    /* Send immediate ping event to task one */
    rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, 0, FALSE);
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_PONG;
  }
  else if(u32EventFlags & EVENT_COMMON)
  {
    printf("Task two received a broadcasted event: EVENT_COMMON\r\n");
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_COMMON;
  }
  if(u08MsgCount)
  {
    if(RTCOS_ERR_NONE == rtcos_get_message((void **)&pcMessage))
    {
      printf("Task two received a broadcasted message: %s\r\n", pcMessage);
    }
  }
  /* This delay is added only for testing purposes under x86 */
  delay(1000);
  return u32RetVal;
}

/** ***********************************************************************************************
  * @brief      Block for a certain time
  * @param      u32Seconds Number of seconds to wait
  * @return     Return unhandled events
  ********************************************************************************************** */
static void delay(_u32 u32Seconds)
{
  _u32 u32MilliSeconds;
  clock_t s32StartTime;
  
  u32MilliSeconds = 1000 * u32Seconds;
  s32StartTime = clock();
  while(clock() < s32StartTime + u32MilliSeconds);
}
