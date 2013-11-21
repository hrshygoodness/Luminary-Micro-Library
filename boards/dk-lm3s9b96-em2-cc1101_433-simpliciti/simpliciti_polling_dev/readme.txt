End Device for ``Polling with Access Point'' example


This application offers the end device functionality of the generic
SimpliciTI Polling_with_AP example.  The application can communicate with
other SimpliciTI-enabled devices with compatible radios running the
``Polling_with_AP'' Sender or Receiver configuration.  To run this example,
a third SimpliciTI-enabled board must also be present running the access
point binary, simpliciti_polling_ap for Stellaris development board, or the
relevant Access Point configuration of the Polling_with_AP example if using
another hardware platform.

The functionality provided here is equivalent to the ``Sender'' and
''Receiver'' configurations included in the generic SimpliciTI ``Polling
with AP'' example application.

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

Power up the access point board and both its LEDs should light indicating
that it is active.  Next, power up the receiver board and select the
correct mode by pressing the ``Receiver'' button on the development board
display, pressing button 2 on dual button boards, or pressing the single
button for less than 3 seconds on boards with only one button.  At this
point, only LED1 on the receiver board should be lit.  Finally power up the
sender select the mode using the onscreen button, by pressing button 2 on
dual button boards or by pressing an holding the single button for more
than 3 seconds.  Both LEDs on the sender will blink until it successfully
links with the receiver.  After successful linking, the sender will
transmit a message to the receiver every 3 to 6 seconds.  This message will
be stored by the access point and passed to the receiver the next time it
polls the access point.  While the example is running, LEDs on both the
sender and receiver will blink.  No user interaction is required on the
access point.

For additional information on running this example and an explanation of
the communication between the two devices and access point, see section 3.2
of the ``SimpliciTI Sample Application User's Guide'' which can be found
under C:/StellarisWare/SimpliciTI-1.1.1/Documents assuming that
StellarisWare is installed in its default directory.

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
