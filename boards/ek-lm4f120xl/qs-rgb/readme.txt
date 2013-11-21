EK-LM4F120XL Quickstart Application

A demonstration of the Stellaris LaunchPad (EK-LM4F120XL) capabilities.

Press and/or hold the left button traverse toward the red end of the
ROYGBIV color spectrum.  Press and/or hold the right button to traverse
toward the violet end of the ROYGBIV color spectrum.

Leave idle for 5 seconds to see a automatically changing color display

Press and hold both left and right buttons for 3 seconds to enter
hibernation.  During hibernation last color on screen will blink on the
LED for 0.5 seconds every 3 seconds.

Command line UART protocol can also control the system.

Command 'help' to generate list of commands and helpful information.
Command 'hib' will place the device into hibernation mode.
Command 'rand' will initiate the pseudo-random sequence.
Command 'intensity' followed by a number between 0.0 and 1.0 will scale
the brightness of the LED by that factor.
Command 'rgb' followed by a six character hex value will set the color. For
example 'rgb FF0000' will produce a red color.

-------------------------------------------------------------------------------

Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
