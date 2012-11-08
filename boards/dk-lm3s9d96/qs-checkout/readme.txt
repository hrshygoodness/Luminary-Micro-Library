Quickstart Checkout Application

This widget-based application exercises many of the peripherals found on
the development kit board.  It offers the following features:

- USB mouse support.  The application will show the state of up to
  three mouse buttons and a cursor position when a USB mouse is connected
  to the board.  If the board is connected by USB to a host, it will operate
  as a USB mouse.  In this mode, the host mouse pointer may be moved by
  dragging a finger across the touchscreen and the user button on the
  development board will act as mouse button 1.
- TFTP server.  This server allows the image stored in the 1MB serial flash
  device to be written and read from a remote Ethernet-connected system.
  The image in the serial flash is copied to SDRAM on startup and used as
  the source for the external web server file system.  Suitable images
  can be created using the makefsfile utility with the -b and -h switches.
  If an Flash/SRAM/LCD daughter board is detected, the external file system
  is served directly from the flash on this board assuming an image is found
  there.
  To upload a binary image to the serial flash, use the TFTP command line
  <code>tftp -i \<board IP\> PUT \<binary file\> eeprom</code>.
  To read the current image out of serial flash, use command line
  <code>tftp -i \<board IP\> GET eeprom \<binary file\></code>.
  To read or write the file system image on the Flash/SRAM/LCD daughter
  board (if present), use the same commands but replace ``eeprom'' with
  ``exflash''.
  When shipped, the serial flash on the board contains file ramfs_data.bin
  which contains a web photo gallery.
  The TFTP server also allows access to files on an installed SDCard.  To
  access the SDCard file system, add "sdcard/" before the filename to GET
  or PUT.  This support does not allow creation of new directories but files
  may be read or written anywhere in the existing directory structure of the
  SDCard.
- Web server.  The lwIP TCP/IP stack is used to implement a web server
  which can serve files from an internal file system, a FAT file system
  on an installed microSD card or USB flash drive, or a file system image
  stored in serial flash and copied to SDRAM during initialization.  These
  file systems may be accessed from a web browser using URLs
  <code>http://\<board IP\></code>,
  <code>http://\<board IP\>/sdcard/\<filename\></code>,
  <code>http://\<board IP\>/usb/\<filename\></code> and
  <code>http://\<board ID\>/ram/\<filename\></code> respectively where
  \<board IP\> is the dotted decimal IP address assigned to the board and
  \<filename\> indicates the particular file being requested.  Note that
  the web server does not open default filenames (such as index.htm) in
  any directory other than the root so the full path and filename must be
  specified in all other cases.
- Touch screen.  The current touch coordinates are displayed whenever the
  screen is pressed.
- LED control.  A GUI widget allows control of the user LED on the board.
- Serial command line.  A simple command line is implemented via UART0.
  Connect a terminal emulator set to 115200/8/N/1 and enter "help" for
  information on supported commands.
- JPEG image viewer.  The QVGA display is used to display JPEG images
  from the ``images'' directory in the web server's exteral file system
  image.  The user can scroll the image on the display using the
  touchscreen.
- Audio player.  Uncompressed WAV files stored on the microSD card or USB
  flash drive may be played back via the headphone jack on the I2S daughter
  board.  Available wave audio files are shown in a listbox on the left
  side of the display.  Click the desired file then press "Play" to play it
  back.  Volume control is provided via a touchscreen slider shown on the
  right side of the display.

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
