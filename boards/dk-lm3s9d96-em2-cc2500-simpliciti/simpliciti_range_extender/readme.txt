Range Extender for ``Polling with Access Point'' example


This application provides range extender functionality in a SimpliciTI low
power RF network.  It may be run on a development board equipped with an EM
expansion board and SimpliciTI-compatible radio board to extend the
distance between end devices and the access point when running the
``Polling with Access Point'' example (simpliciti_polling_ap and
simpliciti_polling_dev).

The functionality provided here is equivalent to the ``Range Extender''
configuration included in the generic SimpliciTI ``Polling_with_AP''
example application.  The application merely receives all SimpliciTI
packets and retransmits them.  No user interaction is required.

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

For additional information on running this example and an explanation of
the communication between the two devices and access point, see section 3.2
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

This is part of revision 10636 of the DK-LM3S9D96-EM2-CC2500-SIMPLICITI Firmware Package.
