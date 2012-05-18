Scribble Pad

The scribble pad provides a drawing area on the screen.  Touching the
screen will draw onto the drawing area using a selection of fundamental
colors (in other words, the seven colors produced by the three color
channels being either fully on or fully off).  Each time the screen is
touched to start a new drawing, the drawing area is erased and the next
color is selected.  This behavior can be modified using various commands
entered via a terminal emulator connected to the IDM-L35 UART.

UART0, which is connected to the 3 pin header on the underside of the
IDM-L35 RDK board (J3), is configured for 115,200 bits per second, and
8-n-1 mode.  When the program is started a message will be printed to the
terminal.  Type ``help'' for command help.

This application supports remote software update over serial using the
LM Flash Programmer application.  Firmware updates can be initiated by
entering the "swupd" command on the serial terminal.  The LMFlash serial
data rate must be set to 115200bps and the "Program Address Offset" to
0x800.

UART0, which is connected to the 6 pin header on the underside of the
IDM-L35 RDK board (J8), is configured for 115200bps, and 8-n-1 mode.  The
USB-to-serial cable supplied with the IDM-L35 RDK may be used to connect
this TTL-level UART to the host PC to allow firmware update.

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

This is part of revision 8555 of the RDK-IDM-L35 Firmware Package.
