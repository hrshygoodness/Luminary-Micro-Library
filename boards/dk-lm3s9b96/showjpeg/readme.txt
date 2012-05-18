JPEG Image Decompression

This example application decompresses a JPEG image which is linked into
the application and shows it on the 320x240 display.  External RAM is used
for image storage and decompression workspace.  The image may be scrolled
in the display window by dragging a finger across the touchscreen.

JPEG decompression and display are handled using a custom graphics library
widget, the source for which can be found in drivers/jpgwidget.c.

The JPEG library used by this application is release 6b of the Independent
JPEG Group's reference decoder.  For more information, see the README and
various text file in the /third_party/jpeg directory or visit
<a href="http://www.ijg.org/">http://www.ijg.org/</a>.

-------------------------------------------------------------------------------

Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
