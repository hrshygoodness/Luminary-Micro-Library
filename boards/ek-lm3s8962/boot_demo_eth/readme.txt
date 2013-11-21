Ethernet Boot Loader Demo

An example to demonstrate the use of remote update signaling with the
flash-based Ethernet boot loader.  This application configures the Ethernet
controller and acquires an IP address which is displayed on the screen
along with the board's MAC address.  It then listens for a ``magic packet''
telling it that a firmware upgrade request is being made and, when this
packet is received, transfers control into the boot loader to perform the
upgrade.

The boot_demo1 and boot_demo2 applications do not make use of the Ethernet
magic packet and can be used along with this application to easily
demonstrate that the boot loader is actually updating the on-chip flash.

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

This is part of revision 10636 of the EK-LM3S8962 Firmware Package.
