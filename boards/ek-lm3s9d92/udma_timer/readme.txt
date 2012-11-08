uDMA with Timer

This example application demonstrates the use of the timer to trigger
periodic DMA transfers.  A timer is configured for periodic operation.
The uDMA controller channel is configured to perform a transfer when
requested from the timer.  For the purposes of this demonstration, the
data that is transferred is the value of a separate free-running timer.
However in a real application the data transferred could be to/from memory
or a peripheral.

After a small number of transfers are performed, the captured timer values
are compared to make sure the expected duration elapsed between transfers.
The results are printed out.

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

This is part of revision 9453 of the EK-LM3S9D92 Firmware Package.
