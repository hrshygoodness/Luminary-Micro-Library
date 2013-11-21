BLDC RDK Control

This application provides a simple GUI for controlling a BLDC RDK board.
The motor can be started and stopped, the target speed can be adjusted, and
the current speed can be monitored.

The target speed up and down buttons utilize the auto-repeat capability of
the push button widget.  For example, pressing the up button will increase
the target speed by 100 rpm.  Holding it for more than 0.5 seconds will
commence the auto-repeat, at which point the target speed will increase by
100 rpm every 1/10th of a second.  The same behavior occurs on the down
button.

Upon startup, the application will attempt to contact a DHCP server to get
an IP address.  If a DHCP server can not be contacted, it will instead use
the IP address 169.254.19.70 without performing any ARP checks to see if it
is already in use.  Once the IP address is determined, it will initiate a
connection to a BLDC RDK board at IP address 169.254.89.71.  While
attempting to contact the DHCP server and the BLDC RDK board, the target
speed will display as a set of bouncing dots.

The push buttons will not operate until a connection to a BLDC RDK board
has been established.

This application supports remote software update over Ethernet using the
LM Flash Programmer application.  A firmware update is initiated using the
remote update request ``magic packet'' from LM Flash Programmer.  This
feature is available in versions of LM Flash Programmer with build numbers
greater than 560.

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

This is part of revision 10636 of the RDK-IDM Firmware Package.
