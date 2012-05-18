CAN FIFO mode example

This application uses the CAN controller in FIFO mode to communicate with
the CAN device board.  The CAN device board must have the can_device_fifo
example program loaded and running prior to starting this application.
This program expects the CAN device board to echo back the data that it
receives and this application will then compare the data received with the
transmitted data and look for differences.  This application will then
modify the data and continue transmitting and receiving data over the CAN
bus indefinitely.

\note This application must be started after the can_device_fifo example
has started on the CAN device board.

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

This is part of revision 8555 of the EK-LM3S2965 Firmware Package.
