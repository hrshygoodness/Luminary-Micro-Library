Ethernet with PTP

This example application demonstrates the operation of the Stellaris
Ethernet controller using the lwIP TCP/IP Stack.  DHCP is used to obtain
an Ethernet address.  If DHCP times out without obtaining an address,
AutoIP will be used to obtain a link-local address.  The address that is
selected will be shown on the QVGA display and output to the UART.

A default set of pages will be served up by an internal file system and
the httpd server.

The IEEE 1588 (PTP) software has been enabled in this code to synchronize
the internal clock to a network master clock source.

UART0, connected to the FTDI virtual COM port and running at 115,200,
8-N-1, is used to display messages from this application.

For additional details on lwIP, refer to the lwIP web page at:
http://savannah.nongnu.org/projects/lwip/

For additional details on the PTPd software, refer to the PTPd web page at:
http://ptpd.sourceforge.net

-------------------------------------------------------------------------------

Copyright (c) 2009-2010 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
