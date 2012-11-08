USB Boot Loader Demo 1

An example to demonstrate the use of the flash-based USB boot loader.  At
startup, the application displays a message then branches to the USB boot
loader to await the start of an update.  The boot loader presents a
Device Firmware Upgrade interface to the host allowing new applications
to be downloaded to flash via USB.

The usb_boot_demo2 application can be used along with this application to
easily demonstrate that the boot loader is actually updating the on-chip
flash.

The application dfuwrap, found in the boards directory, can be used to
prepare binary images for download to a particular position in device
flash.  This application adds a Stellairs-specific prefix and a DFU standard
suffix to the binary. A sample Windows command line application, dfuprog,
is also provided which allows either binary images or DFU-wrapped files to
be downloaded to the board or uploaded from it.

The usb_boot_demo1 and usb_boot_demo2 applications are essentially
identical to boot_demo1 and boot_demo2 with the exception that they are
linked to run at address 0x1800 rather than 0x0.  This is due to the fact
that the USB boot loader is not currently included in the Stellaris ROM
and therefore has to be stored in the bottom few KB of flash with the
main application stored above it.

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

This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
