End Device for ``Access Point as Data Hub'' example


This application offers the end device functionality of the generic
SimpliciTI ``Access Point as Data Hub'' example.  Pressing buttons on the
display will toggle the corresponding LEDs on the access point board to
which this end device is linked.

The application can communicate with another SimpliciTI-enabled device
equipped with a compatible radio and running its own version of the
access point from the ''Access Point as Data Hub'' example or with other
development boards running the simpliciti_hub_ap example.

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

Start the board running the access point example first then start the
end devices.  The LEDs on the end device will flash once to indicate that
they have joined the network.  After this point, pressing one of the
buttons on the display will send a message to the access point causing it
to toggle either LED1 or LED2 depending upon which button was pressed.

For additional information on running this example and an explanation of
the communication between the two devices and access point, see section 3.4
of the ``SimpliciTI Sample Application User's Guide'' which can be found
under C:/StellarisWare/SimpliciTI-1.1.1/Documents assuming that
StellarisWare is installed in its default directory.

-------------------------------------------------------------------------------

Copyright (c) 2010-2013 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 10636 of the DK-LM3S9D96-EM2-CC1101_433-SIMPLICITI Firmware Package.
