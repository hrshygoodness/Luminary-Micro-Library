USB HID Mouse Host

This example application demonstrates how to support a USB mouse
attached to the evaluation kit board.  The display will show if a mouse
is currently connected and the current state of the buttons on the
on the bottom status area of the screen.  The main drawing area will show
a mouse cursor that can be moved around in the main area of the screen.
If the left mouse button is held while moving the mouse, the cursor will
draw on the screen.  A side effect of the application not being able to
read the current state of the screen is that the cursor will erase
anything it moves over while the left mouse button is not pressed.

This application supports remote software update over Ethernet using the
LM Flash Programmer application.  A firmware update is initiated using the
remote update request ``magic packet'' from LM Flash Programmer.

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

This is part of revision 10636 of the RDK-IDM-SBC Firmware Package.
