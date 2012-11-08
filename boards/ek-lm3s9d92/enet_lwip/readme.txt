Ethernet with lwIP

This example application demonstrates the operation of the Stellaris
Ethernet controller using the lwIP TCP/IP Stack configured to operate as
an HTTP (web) server.  DHCP is used to obtain an Ethernet address.  If DHCP
times out without obtaining an address, AutoIP will be used to obtain a
link-local address.  The address that is selected will be shown on the
UART.

Source files for the internal file system image can be found in the ``fs''
directory.  If any of these files are changed, the file system image
(lmi-fsdata.h) should be rebuilt by running the following command from the
enet_lwip directory:

./../../tools/bin/makefsfile -i fs -o lmi-fsdata.h -r -h -q

UART0, connected to the FTDI virtual COM port and running at 115,200,
8-N-1, is used to display messages from this application.

For additional details on lwIP, refer to the lwIP web page at:
http://savannah.nongnu.org/projects/lwip/

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

This is part of revision 9453 of the EK-LM3S9D92 Firmware Package.
