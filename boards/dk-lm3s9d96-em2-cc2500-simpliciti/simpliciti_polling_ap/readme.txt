Access Point for ``Polling with Access Point'' example


This application offers the access point functionality of the generic
SimpliciTI Polling_with_AP example.  To run this example, two additional
SimpliciTI-enabled boards using compatible radios must also be present, one
running the sender application and the other running the receiver.  If
using the Stellaris development board, these functions are found in the
simpliciti_polling_dev example application.  On other hardware, these are
the Sender and Receiver configurations of the ``Polling_with_AP'' example
as supplied with SimpliciTI 1.1.1.

The functionality provided here is equivalent to the ``Access Point''
configuration included in the generic SimpliciTI ``Polling with AP''
example application.

To run this binary correctly, the development board must be equipped with
an EM2 expansion board with a CC2520EM module installed in the ``MOD1''
position (the connectors nearest the oscillator on the EM2).  Hardware
platforms supporting SimpliciTI 1.1.1 with which this application may
communicate are the following:

<ul>
<li>eZ430 + RF2500</li>
<li>EXP430FG4618 + CC2500 + USB Debug Interface</li>
<li>SmartRF04EB + CC2510EM</li>
<li>CC2511EM USB Dongle</li>
<li>Stellaris Development Board + EM2 expansion board + CC2500EM</li>
</ul>

To run this example, power up the access point board and both its LEDs
should light indicating that it is active.  Next, power up the receiver
board and press button 2 (or, on single button boards, press the button for
less than 3 seconds).  At this point, only LED1 on the receiver board
should be lit.  Finally power up the sender and press its button 1 (or, on
single button boards, press the button for more than 3 seconds).  Both LEDs
on the sender will blink until it successfully links with the receiver.
After successful linking, the sender will transmit a message to the
receiver every 3 to 6 seconds.  This message will be stored by the access
point and passed to the receiver the next time it polls the access point.
While the example is running, LEDs on both the sender and receiver will
blink.  No user interaction is required on the access point.

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

This is part of revision 9453 of the DK-LM3S9D96-EM2-CC2500-SIMPLICITI Firmware Package.
