USB Generic Bulk Device

This example provides a generic USB device offering simple bulk data
transfer to and from the host.  The device uses a vendor-specific class ID
and supports a single bulk IN endpoint and a single bulk OUT endpoint.
Data received from the host is assumed to be ASCII text and it is
echoed back with the case of all alphabetic characters swapped.

A Windows INF file for the device is provided on the installation CD.  This
INF contains information required to install the WinUSB subsystem on
WindowsXP and Vista PCs.  WinUSB is a Windows subsystem allowing user mode
applications to access the USB device without the need for a
vendor-specific kernel mode driver.  The device driver may also be
downloaded from http://www.luminarymicro.com/products/software_updates.html
as part of the ``Stellaris embedded USB drivers'' package
(SW-USB-windrivers).

A sample Windows command-line application, usb_bulk_example, illustrating
how to connect to and communicate with the bulk device is also provided.
The application binary is installed as part of the ``Windows-side examples
for USB kits'' package (SW-USB-win) on the installation CD or via download
from http://www.luminarymicro.com/products/software_updates.html .  Project
files are included to allow the examples to be built using Microsoft
VisualStudio.  Source code for this application can be found in directory
StellarisWare/tools/usb_bulk_example.

-------------------------------------------------------------------------------

Copyright (c) 2008-2010 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
