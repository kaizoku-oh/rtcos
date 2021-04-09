/******************************************************************************
    Filename: boards.h
    Description: 
        This file contains include mechanism for all the different board files.
		These files define the parameters for the os on a particular board.
    
    Written: Jon Norenberg, 2013 - 2016
    Copyright: My code is your code.  No warranties. 
******************************************************************************/

#ifndef PORT_H
#define PORT_H

//******************************************************************************
// Include the appropriate board file depending on the build
//******************************************************************************

#if BOARD_ARDUINO_UNO
// #error "warning Copy the entire arduino_uno directory to your Arduino projects directory"
#include "board_arduino_uno.h"
#else
#error "warning No board file has been specified for this build"

#endif


#endif
