Calibration for the Touch Screen

The raw sample interface of the touch screen driver is used to compute the
calibration matrix required to convert raw samples into screen X/Y
positions.  The produced calibration matrix can be inserted into the touch
screen driver to map the raw samples into screen coordinates.

The touch screen calibration is performed according to the algorithm
described by Carlos E. Videles in the June 2002 issue of Embedded Systems
Design.  It can be found online at
<a href="http://www.embedded.com/story/OEG20020529S0046">
http://www.embedded.com/story/OEG20020529S0046</a>.

This application supports remote software update over Ethernet using the
LM Flash Programmer application.  A firmware update is initiated using the
remote update request ``magic packet'' from LM Flash Programmer.

-------------------------------------------------------------------------------

Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
Software License Agreement

Texas Instruments (TI) is supplying this software for use solely and
exclusively on TI's microcontroller products. The software is owned by
TI and/or its suppliers, and is protected under applicable copyright
laws. You may not combine this software with "viral" open-source
software in order to form a larger program.

THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
DAMAGES, FOR ANY REASON WHATSOEVER.

This is part of revision 9453 of the RDK-IDM-SBC Firmware Package.
