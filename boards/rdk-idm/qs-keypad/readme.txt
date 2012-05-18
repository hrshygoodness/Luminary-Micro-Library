Quickstart Security Keypad

This application provides a security keypad to allow access to a door.  The
relay output is momentarily toggled upon entry of the access code to
activate an electric door strike, unlocking the door.

The screen is divided into three parts; the Texas Instruments banner across
the top, a hint across the bottom, and the main application area in the
middle (which is the only portion that should appear if this application
is used for a real door access system).  The hints provide an on-screen
guide to what the application is expecting at any given time.

Upon startup, the screen is blank and the hint says to touch the screen.
Pressing the screen will bring up the keypad, which is randomized as an
added security measure (so that an observer can not ``steal'' the access
code by simply looking at the relative positions of the button presses).
The current access code is provide in the hint at the bottom of the screen
(which is clearly not secure).

If an incorrect access code is entered (``#'' ends the code entry), then
the screen will go blank and wait for another access attempt.  If the
correct access code is entered, the relay will be toggled for a few seconds
(as indicated by the hint at the bottom stating that the door is open) and
the screen will go blank.  Once the door is closed again, the screen can
be touched again to repeat the process.

The UART is used to output a log of events.  Each event in the log is time
stamped, with the arbitrary date of February 26, 2008 at 14:00 UT
(universal time) being the starting time when the application is run.  The
following events are logged:

- The start of the application
- The access code being changed
- Access being granted (correct access code being entered)
- Access being denied (incorrect access code being entered)
- The door being relocked after access has been granted

A simple web server is provided to allow the access code to be changed.
The Ethernet interface will attempt to contact a DHCP server, and if it is
unable to acquire a DHCP address it will instead use the IP address
169.254.19.70 without performing any ARP checks to see if it is already in
use.  The web page shows the current access code and provides a form for
updating the access code.

If a micro-SD card is present, the access code will be stored in a file
called ``key.txt'' in the root directory.  This file is written whenever
the access code is changed, and is read at startup to initialize the access
code.  If a micro-SD card is not present, or the ``key.txt'' file does not
exist, the access code defaults to 6918.

If ``**'' is entered on the numeric keypad, the application provides
a demonstration of the Stellaris Graphics Library with various panels
showing the available widget types and graphics primitives.  Navigate
between the panels using buttons marked ``+'' and ``-'' at the bottom of
the screen and return to keypad mode by pressing the ``X'' buttons
which appear when you are on either the first or last demonstration
panel.

This application supports remote software update over Ethernet using the
LM Flash Programmer application.  A firmware update is initiated via the
remote update request ``magic packet'' from LM Flash Programmer.  If using

Note that remote firmware update signalling is only supported in versions
of LM Flash Programmer with build numbers greater than 560.  If using an
earlier version of LM Flash Programmer which does not send the ``magic
packet'' signalling an update request, an update may be initiated by
entering ``*0'' on the application's numeric keypad.

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
