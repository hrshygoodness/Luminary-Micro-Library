``Simple Peer-to-Peer'' example

This application offers the functionality of the generic SimpliciTI
Simple_Peer_to_Peer example.  Whereas the original example builds two
independent executables, this version implements both the talker (LinkTo)
and listener (LinkListen) functionality, the choice being made via two
buttons shown on the display.  The application can communicate with another
SimpliciTI-enabled device with a compatible radio running one of the
Simple_Peer_to_Peer example binaries or another copy of itself running on a
second development board.

To run this binary correctly, the development board must be equipped with
an EM2 expansion board with a CC1101:433 EM module installed in the
``MOD1'' position (the connectors nearest the oscillator on the EM2).
Hardware platforms supporting SimpliciTI 1.1.1 with which this application
may communicate are the following:

<ul>
<li>SmartRF04EB + CC1110EM</li>
<li>EM430F6137RF900</li>
<li>FET430F6137RF900</li>
<li>CC1111EM USB Dongle</li>
<li>EXP430FG4618 + CC1101:433 + USB Debug Interface</li>
<li>EXP430FG4618 + CC1100:433 + USB Debug Interface</li>
<li>Stellaris Development Board + EM2 expansion board + CC1101:433</li>
</ul>

On starting the application, you are presented with two choices on the
LCD display.  If your companion board is running the ``LinkTo''
configuration of the application, press the ``LinkListen'' button.  If the
companion board is running ``LinkListen'', press the ``LinkTo'' button.
Once one of these buttons is pressed, the application attempts to start
communication with its peer, either listening for an incoming link
request or sending a request.  After a link is established the talker
board (running in ``LinkTo'' mode) sends packets to the listener which
echos them back after toggling an LED.  In the Stellaris implementation
of the application the ``LEDs'' are shown using onscreen widgets.

For additional information on this example and an explanation of the
communication between talker and listener, see section 3.1 of the
``SimpliciTI Sample Application User's Guide'' which can be found under
C:/StellarisWare/SimpliciTI-1.1.1/Documents assuming you installed
StellarisWare in its default directory.

-------------------------------------------------------------------------------

Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 9107 of the DK-LM3S9B96-EM2-CC1101_433-SIMPLICITI Firmware Package.
