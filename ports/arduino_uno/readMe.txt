
The arduino IDE 1.6.5 is very picky for location of the project and directories, 
so for this example I moved all the files
into this directory.  I actually built and ran it from the
Arduino directory pointed to in the IDE preferences.  After
it was working I moved it here since the IDE wants the library
directory as a peer of the projects directory.

The TimerOne arduino library is used to get a callback from the 
hardware every 100 ms.  This is tied into the OS tick handler
to update the timing.

I also moved the os files to this directory and change the .c to .cpp
so that the C++ compiler would use the bool type correctly.

This is a simple use of the RTC OS that just creates 5 tasks
and sends events back and forth to turn on and off the led.

With the current configuration:
#define MAX_TASKS_IN_SYSTEM             5
#define MAX_FUTURE_EVENTS               4
#define MAX_MESSAGES_IN_SYSTEM          3


Sketch uses 5,476 bytes (16%) of program storage space. Maximum is 32,256 bytes.
Global variables use 609 bytes (29%) of dynamic memory, leaving 1,439 bytes for 
local variables. Maximum is 2,048 bytes.
