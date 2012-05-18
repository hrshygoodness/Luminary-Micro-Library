Channel Sniffer for use with ``Access Point as a Data Hub'' example


This application provides channel sniffer functionality in a SimpliciTI
low power RF network.  It may be run on a development board equipped with
an EM expansion board and SimpliciTI-compatible radio board to determine
which channel the network is currently on.  Use this application alongside
the ``Access Point as a Data Hub'' example (simpliciti_hub_ap/dev) which
supports frequency agility.

The functionality provided here is equivalent to the ``Channel Sniffer''
configuration included in the generic SimpliciTI ``AP_as_Data_Hub'' example
application.

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

At least one additional board is required to run this application and two
additional boards are needed to show all features of the ``Access Point as
a Data Hub'' example.  If running the sniffer application with only one
other board, make sure that that board is running the access point binary,
simpliciti_hub_ap from StellarisWare or the relevant access point
configuration of the generic SimpliciTI ``Ap_as_Data_Hub'' example if
running the access point on a different hardware platform.

Start the board running the sniffer application and both LEDs on the screen
will flash until the board joins the access point's network.  After the
network is joined the LEDs will indicate the current SimpliciTI channel
number in binary.  The channel number is also displayed in text at the
bottom of the screen.  To change the network channel, press button 1 on
the board running the access point software.

For additional information on running this example and an explanation of
the communication between the two devices and access point, see section
3.4.2 of the ``SimpliciTI Sample Application User's Guide'' which can be
found under C:/StellarisWare/SimpliciTI-1.1.1/Documents assuming
that StellarisWare is installed in its default directory.

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

This is part of revision 8555 of the DK-LM3S9B96-EM2-CC2500-SIMPLICITI Firmware Package.
