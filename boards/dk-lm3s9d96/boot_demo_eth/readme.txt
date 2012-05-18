Ethernet Boot Loader Demo

An example to demonstrate the use of remote update signaling with the
flash-based Ethernet boot loader.  This application configures the Ethernet
controller and acquires an IP address which is displayed on the screen
along with the board's MAC address.  It then listens for a ``magic packet''
telling it that a firmware upgrade request is being made and, when this
packet is received, transfers control into the boot loader to perform the
upgrade.

Although there are three flavors of flash-based boot loader provided with
this software release (boot_serial, boot_eth and boot_usb), this example is
specific to the Ethernet boot loader since the magic packet used to trigger
entry into the boot loader from the application is only sent via Ethernet.

The boot_demo1 and boot_demo2 applications do not make use of the Ethernet
magic packet and can be used along with this application to easily
demonstrate that the boot loader is actually updating the on-chip flash.

Note that the LM3S9D96 and other Firestorm-class Stellaris devices also
support serial and ethernet boot loaders in ROM.  To make use of this
function, link your application to run at address 0x0000 in flash and enter
the bootloader using either the ROM_UpdateEthernet or ROM_UpdateSerial
functions (defined in rom.h).  This mechanism is used in the
utils/swupdate.c module when built specifically targeting a suitable
Firestorm-class device.

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

This is part of revision 8555 of the DK-LM3S9D96 Firmware Package.
