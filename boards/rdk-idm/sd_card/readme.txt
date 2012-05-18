SD card using FAT file system

This example application demonstrates reading a file system from
an SD card.  It makes use of FatFs, a FAT file system driver.  It
provides a simple widget-based console on the display and also a UART-based
command line for viewing and navigating the file system on the SD card.

For additional details about FatFs, see the following site:
http://elm-chan.org/fsw/ff/00index_e.html

UART1, which is connected to the 3 pin header on the underside of the
IDM RDK board (J2), is configured for 115,200 bits per second, and
8-n-1 mode.  When the program is started a message will be printed to the
terminal.  Type ``help'' for command help.

To connect the IDM RDK board's UART to a 9 pin PC serial port, use a
standard male to female DB9 serial cable and connect TXD (J2 pin 1, nearest
the SD card socket) to pin 2 of the male serial cable connector, RXD (J2
pin 2, the center pin) to pin 3 of the serial connector and GND (J2 pin 3)
to pin 5 of the serial connector.

This application supports remote software update over Ethernet using the
LM Flash Programmer application.  A firmware update is initiated using the
remote update request ``magic packet'' from LM Flash Programmer.  This
feature is available in versions of LM Flash Programmer with build numbers
greater than 560.

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

This is part of revision 8555 of the RDK-IDM Firmware Package.
