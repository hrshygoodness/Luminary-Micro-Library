uDMA with Timer Edge Capture

This example application demonstrates the use of the timer edge capture
mode to trigger DMA transfers whenever there is an edge capture event.
A timer is configured for edge capture mode using CCP1 on pin PD7.  The
uDMA controller is configured to transfer the captured edge value to a
buffer until a certain number of edges have been captured.  A PWM output
is configured to generate a square wave on GPIO port bit PD0 to use as
an input source for the CCP1 pin.  In order to run the example, the pins
PD0 and PD7 must be jumpered together.

After a certain number of edges have been captured, the DMA transfer will
be complete.  The example program will print out the captured values.

UART0, connected to the FTDI virtual COM port and running at 115,200,
8-N-1, is used to display messages from this application.

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

This is part of revision 8555 of the EK-LM3S9B92 Firmware Package.
