Quick Start Game

This game consists of a Z-machine interpreter running a Z-code version of
the classic Colossal Cave Adventure game originally created by William
Crowther.  The Ethernet interface provides a telnet server and the USB
interface provides a CDC serial port.  Either interface can be used to play
the game, though not at the same time.

The LED on the evaluation board will be turned on when the game is being
played; further connections will be refused since only one instance of the
game can be played at a time.  The push button on the evaluation board will
restart the game from the beginning; this is equivalent to typing
``restart'' followed by ``yes'' in the game itself.

The virtual COM port provided by the ICDI board (which is connected to
UART0 on the evaluation board) provides a simple status display.  The most
important piece of information provided is the IP address of the Ethernet
interface, which is selected using AutoIP (which uses DHCP if it is present
and a random link-local address otherwise).

The game is played by typing simple English sentences in order to direct
the actions of the protagonist, with abbreviations being allowed.  For
example, ``go west'', ``west'', and ``w'' all perform the same action.

Three display modes are available; ``verbose'' (which displays the full
description every time a location is visited), ``brief'' (which displays
the full description the first time a location is visited and only the name
every other time), and ``superbrief'' (which only displays the name).  The
default display mode is ``brief'', and ``look'' can be used to get the full
description at any time (regardless of the display mode).

For a history of the Colossal Cave Adventure game, its creation of the
``interactive fiction'' gaming genre, and game hints, an Internet search
will turn up numerous web sites.  A good starting place is
http://en.wikipedia.org/wiki/Colossal_Cave_Adventure .

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

This is part of revision 8555 of the EK-LM3S9D92 Firmware Package.
