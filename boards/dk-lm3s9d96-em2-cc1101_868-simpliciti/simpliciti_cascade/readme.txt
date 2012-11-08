``Cascading End Devices'' Example

This application offers the functionality of the generic SimpliciTI
``Cascading End Devices'' example and simulates a network of alarm devices.
When an alarm is raised on any one device, the signal is cascaded through
the network and retransmitted by all the other devices receiving it.

The application can communicate with other SimpliciTI-enabled devices
running with compatible radios and running their own version of the
''Cascading End Devices'' example or with other Stellaris development
boards running this example.

To run this binary correctly, the Stellaris development board must be
equipped with an EM2 expansion board with a CC1101:868/915 EM module
installed in the ``MOD1'' position (the connectors nearest the oscillator
on the EM2).  Hardware platforms supporting SimpliciTI 1.1.1 with which
this application may communicate are the following:

<ul>
<li>SmartRF04EB + CC1110EM</li>
<li>EM430F6137RF900</li>
<li>FET430F6137RF900</li>
<li>CC1111EM USB Dongle</li>
<li>EXP430FG4618 + CC1101:868/915 + USB Debug Interface</li>
<li>EXP430FG4618 + CC1100:868/915 + USB Debug Interface</li>
<li>Stellaris Development Board + EM2 expansion board + CC1101:868/915</li>
</ul>

The main loop of this application wakes every 5 seconds or so and checks
its ``sensor'', in this case a button on the display labelled ``Sound
Alarm''.  If its own alarm has not been raised, it listens for alarm
messages from other devices.  If nothing is heard, the application toggles
LED1 then goes back to sleep again.  If, however, an alarm is signalled
by the local ``sensor'' or an alarm message from another device is
received, the application continually retransmits the alarm message and
toggles LED2.  In the Stellaris implementation of the application the
``LEDs'' are shown using onscreen widgets.

For additional information on running this example and an explanation of
the communication between talker and listener, see section 3.3 of the
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

This is part of revision 9453 of the DK-LM3S9D96-EM2-CC1101_868-SIMPLICITI Firmware Package.
