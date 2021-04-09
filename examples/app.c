#include <stdio.h>
#include <time.h>
#include "rtcos.h"

#define TASK_ID_PRIORITY_ONE (_U08)0
#define TASK_ID_PRIORITY_TWO (_U08)1
#define EVENT_PING (_U08)1
#define EVENT_PONG (_U08)2

void delay(_U32 u32Seconds);
_U32 _task_one_handler(_U32 u32EventFlags, _U08 u08MsgCount, _U32 u32Param);
_U32 _task_two_handler(_U32 u32EventFlags, _U08 u08MsgCount, _U32 u32Param);

int main(int s32Argc, char const *pu08Argv[])
{
  rtcos_init();
  rtcos_register_task_handler(_task_one_handler, TASK_ID_PRIORITY_ONE, 0);
  rtcos_register_task_handler(_task_two_handler, TASK_ID_PRIORITY_TWO, 0);
  rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, 0, _FALSE);
  rtcos_send_message(TASK_ID_PRIORITY_TWO, (void *)"Hello");
  rtcos_run();

  return 0;
}

_U32 _task_one_handler(_U32 u32EventFlags, _U08 u08MsgCount, _U32 u32Param)
{
  _U32 u32RetVal;

  u32RetVal = 0;
  if(u32EventFlags & EVENT_PING)
  {
    printf("Task one received PING event!\r\n");
    rtcos_send_event(TASK_ID_PRIORITY_TWO, EVENT_PONG, 0, _FALSE);
    u32RetVal = u32EventFlags & ~EVENT_PING;
  }
  delay(1000);
  return u32RetVal;
}

_U32 _task_two_handler(_U32 u32EventFlags, _U08 u08MsgCount, _U32 u32Param)
{
  _U32 u32RetVal;
  _CHAR *pcMessage;

  u32RetVal = 0;
  if(u32EventFlags & EVENT_PONG)
  {
    printf("Task two received PONG event!\r\n");
    rtcos_send_event(TASK_ID_PRIORITY_ONE, EVENT_PING, 0, _FALSE);
    u32RetVal = u32EventFlags & ~EVENT_PONG;
  }
  if(u08MsgCount)
  {
    if(RTCOS_ERR_NONE == rtcos_get_message((void **)&pcMessage))
    {
      printf("Task two received a message: %s\r\n", pcMessage);
    }
  }
  delay(1000);
  return u32RetVal;
}

void delay(_U32 u32Seconds)
{
  _U32 u32MilliSeconds;
  clock_t s32StartTime;
  
  u32MilliSeconds = 1000 * u32Seconds;
  s32StartTime = clock();
  while(clock() < s32StartTime + u32MilliSeconds);
}
