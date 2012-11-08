External flash execution demonstration

This example application illustrates execution out of external flash
attached via the Extended Peripheral Interface (EPI).  It utilizes the UART
to display a simple message before immediately transfering control back to
the boot loader in preparation for the download of a new application image.
The first UART (connected to the FTDI virtual serial port on the
development kit board) will be configured in 115200 baud, 8-n-1 mode.

This application is configured specifically for execution from external
flash and relies upon the external flash version of the Ethernet boot
loader, boot_eth_ext, being present in internal flash.  It will not run
with any other boot loader version.  The boot_eth_ext boot loader configures
the system clock and EPI to allow access to the external flash address
space then relocates the application exception vectors into internal SRAM
before branching to the main application in daughter-board flash.

Note that execution from external flash should be avoided if at all
possible due to significantly lower performance than achievable from
internal flash. Using an 8-bit wide interface to flash as found on the
Flash/SRAM/LCD daughter board and remembering that an external memory
access via EPI takes 8 or 9 system clock cycles, a program running from
off-chip memory will typically run at approximately 5% of the speed of the
same program in internal flash.

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

This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
