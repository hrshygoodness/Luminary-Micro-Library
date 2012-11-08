USB OTG HID Mouse Example

This example application demonstrates the use of USB On-The-Go (OTG) to
offer both USB host and device operation.  When the EK board is connected
to a USB host, it acts as a BIOS-compatible USB mouse.  The user button
on the board (nearest the USB OTG connector) acts as mouse button 1 and
the mouse pointer may be moved by dragging your finger or a stylus across
the touchscreen in the desired direction.

If a USB mouse is connected to the USB OTG port, the board operates as a
USB host and draws dots on the display to track the mouse movement.  The
states of up to three mouse buttons are shown at the bottom right of the
display.

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

This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
