Access Point for ``Access Point as Data Hub'' example


This application offers the access point functionality of the generic
SimpliciTI Ap_as_Data_Hub.  To run this example, two additional
SimpliciTI-enabled boards using compatible radios must also be present,
each running the EndDevice configuration of the application.  If using the
Stellaris development board this function is found in the
simpliciti_hub_dev example application.  On other hardware it is the
EndDevice configuration of the Ap_as_Data_Hub example as supplied with
SimpliciTI 1.1.1.

To run this binary correctly, the development board must be equipped with
an EM2 expansion board with a CC2520EM module installed in the ``MOD1''
position (the connectors nearest the oscillator on the EM2).  Hardware
platforms supporting SimpliciTI 1.1.1 with which this application may
communicate are the following:

<ul>
<li>CC2430DB</li>
<li>SmartRF04EB + CC2430EM</li>
<li>SmartRF04EB + CC2431EM</li>
<li>SmartRF05EB + CC2530EM</li>
<li>SmartRF05EB + MSP430F2618 + CC2520
<li>Stellaris Development Board + EM2 expansion board + CC2520EM</li>
</ul>

When the access point application is started, both ``LEDs'' on the display
are lit indicating that the access point is waiting for connections from
end devices.  The LEDs may start flashing, indicating that the frequency
agility feature has caused an automatic chanel change.  This flashing will
continue until a message is received from an end device.  When an end
device connects to the access point, pressing buttons on the end device
will send a message to the access point which will toggle one of its LEDs
depending upon the content of the message.

The access point also offers the option to force a channel change.
Pressing the ``Change Channel'' button cycles to the next available radio
channel.  As in the automatic channel change case, this will cause the LEDs
to flash until a new message is received from the end device indicating
that it has also changes to the new channel.

For additional information on running this example and an explanation of
the communication between the two devices and access point, see section 3.4
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

This is part of revision 8555 of the DK-LM3S9D96-EM2-CC2520-SIMPLICITI Firmware Package.
