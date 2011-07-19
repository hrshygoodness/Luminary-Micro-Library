UART Echo running in external flash

This example application is equivalent to uart_echo but has been reworked
to run out of external flash attached via the Extended Peripheral Interface
(EPI).  It utilizes the UART to echo text.  The first UART (connected to
the FTDI virtual serial port on the evaluation board) will be configured in
115,200 baud, 8-n-1 mode.  All characters received on the UART are
transmitted back to the UART and this continues until "swupd" followed by a
carriage return is entered, at which point the application transfers
control back to the boot loader to initiate a firmware update.

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

Copyright (c) 2008-2011 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
