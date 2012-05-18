USB HID Keyboard Device

This example application turns the evaluation board into a USB keyboard
supporting the Human Interface Device class.  The color STN display shows a
virtual keyboard which can be navigated using the direction control button
on the board.  Pressing down on the button presses the highlighted key,
sending its usage code and, if necessary, a shift modifier, to the USB
host.  The board status LED is used to indicate the current Caps Lock state
and is updated in response to pressing the ``Caps'' key on the virtual
keyboard or any other keyboard attached to the same USB host system.

The device implemented by this application also supports USB remote wakeup
allowing it to request the host to reactivate a suspended bus.  If the bus
is suspended (as indicated on the application display), pressing the
Select key will request a remote wakeup assuming the host has not
specifically disabled such requests.

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

This is part of revision 8555 of the EK-LM3S3748 Firmware Package.
