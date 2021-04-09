/******************************************************************************
    Filename: board_arduino_uno.h
    Description: 
        This file contains defines for the OS for this board.
    
    Written: Jon Norenberg, 2013 - 2016
    Copyright: My code is your code.  No warranties. 
******************************************************************************/

#ifndef BOARD_ARDUINO_UNO_H
#define BOARD_ARDUINO_UNO_H

//******************************************************************************
// Some generic constants
//******************************************************************************

#ifndef TRUE
#define TRUE                            1
#endif

#ifndef FALSE
#define FALSE                           0
#endif

//******************************************************************************
// Set these to the appropriate sizes
//******************************************************************************
typedef signed char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef long int32;
typedef unsigned long uint32;

//******************************************************************************
// Change the size of these if needed
//******************************************************************************
typedef int8 osStatus_t;
typedef uint8 osTaskID_t;
typedef uint32 osEvents_t;
typedef uint32 osTick_t;
typedef uint32 osMsg_t;
typedef uint32 osTaskParam_t;
typedef uint8 osMsgCount_t;
typedef uint16 osIndex_t;

//******************************************************************************
// Define how this system handles critical sections
//******************************************************************************
#define ENTER_CRITICAL_SECTION
#define EXIT_CRITICAL_SECTION
#define CRITICAL_SECTION_VARIABLE

//******************************************************************************
// Set the max number of tasks in the system
//******************************************************************************
#define MAX_TASKS_IN_SYSTEM             5

//******************************************************************************
// Defines how many future events that can be scheduled.
// This is for the entire system, not per task
//******************************************************************************
#define MAX_FUTURE_EVENTS               4

//******************************************************************************
// Set the max number of messages that can be sent to each task
// This is for each task in the system
// If you do not want to use messages in your system, set this to 0
//******************************************************************************
#define MAX_MESSAGES_IN_SYSTEM          3


#endif
