Serial To Ethernet Module

The Serial to Ethernet Converter provides a means of accessing the UART on
a Stellaris device via a network connection.  The UART can be connected to
the UART on a non-networked device, providing the ability to access the
device via a network.  This can be useful to overcome the cable length
limitations of a UART connection (in fact, the cable can become thousands
of miles long) and to provide networking capability to existing devices
without modifying the device's operation.

The converter can be configured to use a static IP configuration or to use
DHCP to obtain its IP configuration.  Since the converter is providing a
telnet server, the effective use of DHCP requires a reservation in the DHCP
server so that the converter gets the same IP address each time it is
connected to the network.

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

This is part of revision 10636 of the RDK-S2E Firmware Package.
