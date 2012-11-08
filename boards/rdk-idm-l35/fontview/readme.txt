Font Viewer

This example displays the contents of a Stellaris graphics library font
on the DK board's LCD touchscreen.  By default, the application shows a
test font containing ASCII, the Japanese Hiragana and Katakana alphabets,
and a group of Korean Hangul characters.  If an SDCard is installed and
the root directory contains a file named <tt>font.bin</tt>, this file is
opened and used as the display font instead.  In this case, the graphics
library font wrapper feature is used to access the font from the file
system rather than from internal memory.

When the ``Update'' button is pressed, the application transfers control to
the boot loader to allow a new application image to be downloaded.  The
LMFlash serial data rate must be set to 115200bps and the "Program Address
Offset" to 0x800.

UART0, which is connected to the 6 pin header on the underside of the
IDM-L35 RDK board (J8), is configured for 115200bps, and 8-n-1 mode.  The
USB-to-serial cable supplied with the IDM-L35 RDK may be used to connect
this TTL-level UART to the host PC to allow firmware update.

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

This is part of revision 9453 of the RDK-IDM-L35 Firmware Package.
