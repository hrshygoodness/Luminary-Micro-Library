USB Stick Update Demo

An example to demonstrate the use of the flash-based USB stick update
program.  This example is meant to be loaded into flash memory from a USB
memory stick, using the USB stick update program (usb_stick_update),
running on the microcontroller.

After this program is built, the binary file (usb_stick_demo.bin), should
be renamed to the filename expected by usb_stick_update ("FIRMWARE.BIN" by
default) and copied to the root directory of a USB memory stick.  Then,
when the memory stick is plugged into the eval board that is running the
usb_stick_update program, this example program will be loaded into flash
and then run on the microcontroller.

This program simply displays a message on the screen and prompts the user
to press a "button" on the touch screen.  Once the button is pressed,
control is passed back to the usb_stick_update program which is still in
flash, and it will attempt to load another program from the memory stick.
This shows how a user application can force a new firmware update from the
memory stick.

This application also supports remote software update over Ethernet using
the LM Flash Programmer application.  A firmware update is initiated using
the remote update request ``magic packet'' from LM Flash Programmer.

If the flash is updated using the Ethernet method, then the usb_stick_update
program located at the beginning of flash will be erased.

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

This is part of revision 9453 of the RDK-IDM-SBC Firmware Package.
