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

This application supports remote software update over serial using the
LM Flash Programmer application.  The application transfers control to the
boot loader whenever it completes to allow a new image to be downloaded if
required.  The LMFlash serial data rate must be set to 115200bps and the
"Program Address Offset" to 0x800.

UART0, which is connected to the 6 pin header on the underside of the
IDM-L35 RDK board (J8), is configured for 115200bps, and 8-n-1 mode.  The
USB-to-serial cable supplied with the IDM-L35 RDK may be used to connect
this TTL-level UART to the host PC to allow firmware update.

-------------------------------------------------------------------------------

Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 10636 of the RDK-IDM-L35 Firmware Package.
