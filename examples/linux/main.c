/* 
 **************************************************************************************************
 *
 * @file    : main.c
 * @author  : Bayrem GHARSELLAOUI
 * @version : 1.1.0
 * @date    : April 2021
 * @brief   : Linux example program
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

/*-----------------------------------------------------------------------------------------------*/
/* Private function prototypes                                                                   */
/*-----------------------------------------------------------------------------------------------*/
static void delay(_u32 u32Seconds);
static _u32 _task_one_handler(_u32 u32EventFlags, _u08 u08MsgCount, _u32 u32Param);
static _u32 _task_two_handler(_u32 u32EventFlags, _u08 u08MsgCount, _u32 u32Param);

/** ***********************************************************************************************
  * @brief      Program entry point
  * @return     Return nothing
  ********************************************************************************************** */
void main(void)
{
  rtcos_init();

  rtcos_register_task_handler(_task_one_handler, TASK_ID_PRIORITY_ONE, (_u32)0);
  rtcos_register_task_handler(_task_two_handler, TASK_ID_PRIORITY_TWO, (_u32)0);

  rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, (_u32)0, FALSE);
  rtcos_send_message(TASK_ID_PRIORITY_TWO, (void *)"Hello");

  rtcos_run();
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
    printf("Task one received PING event!\r\n");
    /* Send immediate pong event to task two */
    rtcos_send_event(TASK_ID_PRIORITY_TWO, EVENT_PONG, 0, FALSE);
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_PING;
  }
  /* This delay is added only for testing purposes under x86 */
  delay(1000);
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
    printf("Task two received PONG event!\r\n");
    /* Send immediate ping event to task one */
    rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, 0, FALSE);
    /* Return the events that have NOT been handled */
    u32RetVal = u32EventFlags & ~EVENT_PONG;
  }
  if(u08MsgCount)
  {
    if(RTCOS_ERR_NONE == rtcos_get_message((void **)&pcMessage))
    {
      printf("Task two received a message: %s\r\n", pcMessage);
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
