USB composite HID Mouse and CDC serial Device

This example application turns the evaluation board into a composite USB
mouse supporting the Human Interface Device class and a CDC serial device
The mouse pointer will move in a square pattern for the duration of the
time it is plugged in.  The serial port is used as a command prompt to
change the behavior of the board.  By default the mouse will simply
enumerate and not move.  The serial port can then be opened and a command
can be issued to start the mouse moving or stop it again.

The commands supported by the UART are the following:

? or help or h - Will display the help message.

led <on|off|toggle|activity>
* on       - Turns on the LED.
* off      - Turns off the LED
* toggle   - Toggle the LED
* activity - Toggle the LED due to serial activity.

mouse <on|off>
* on  - Starts the mouse moving in a square pattern.
* off - Stops the mouse moving.

Assuming you installed StellarisWare in the default directory, a driver
information (INF) file for use with Windows XP, Windows Vista and
Windows7 can be found in C:/StellarisWare/windows_drivers. For Windows
2000, the required INF file is in C:/StellarisWare/windows_drivers/win2K.

-------------------------------------------------------------------------------

Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 8555 of the EK-LM3S9D92 Firmware Package.
