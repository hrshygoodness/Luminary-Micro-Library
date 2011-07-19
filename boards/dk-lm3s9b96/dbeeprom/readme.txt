Daughter Board EEPROM Read/Write Example

This application may be used to read and write the ID structures written
into the 128 byte EEPROMs found on the optional Flash/SRAM and FPGA
daughter boards for the development board.  A command line interface is
provided via UART 0 and commands allow the existing ID EEPROM content to be
read and one of the standard structures identifying the available daughter
boards to be written to the device.

The ID EEPROM is read in function PinoutSet() and used to configure the
EPI interface appropriately for the attached daughter board.  If the EEPROM
content is incorrect, this auto-configuration will not be possible and
example applications will typically show merely a blank display when run.

-------------------------------------------------------------------------------

Copyright (c) 2010-2011 Texas Instruments Incorporated.  All rights reserved.
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
