Advanced Audio Distribution Profile

This application provides a Bluetooth A2DP streaming endpoint that is
capable of receiving audio data from Bluetooth-enabled A2DP sources and
playing the audio data out of the headset and lineout ports.  The
development kit must be equipped with an EM2 expansion board and a
CC2560/PAN1323 Bluetooth radio transceiver module for this application to
run correctly.  The CC2560/PAN1323 module must be installed in the ``mod1''
connector (the connector nearest the oscillator) on the EM2 expansion
board.

The A2DP demo application uses the Bluetooth A2DP Profile to manage the
audio streaming connection.  The application creates and advertises
support for an A2DP audio sink endpoint.  The endpoint can be discovered
by Bluetooth devices that are capable of supporting the A2DP audio source
role.  Devices supporting the audio source role can connect to the sink
endpoint, configure the playback parameters, and stream audio data to the
audio sink for playback.  The A2DP sink provides support for the SBC codec.

The A2DP demo supports the following three Bluetooth pairing modes:

- No pairing
- Legacy pairing
- Secure simple pairing

The application supports up to five persistent link keys.  The oldest link
keys are purged to make room for a newer link key once five have been
saved.

When the application is running, the LED will toggle periodically.
Assuming you have already loaded the example program into the development
board, follow these steps to run the example program:

- Attach the headphone output to any standard headphone or attach the line
output to an external amplifier.
- Turn on the development board.  The display should show ``Bluetooth A2DP
Demo Waiting for connection ...''.
- Using any Bluetooth device capable of A2DP source, search for the
development board.  Bluetooth inquiries will display the device friendly
name of ``A2DP Demo''.  If the source device requests a passkey, use
``0000''.
- After a successful connection, the development board should change the
display to ``Connected ... Paused''.
- Start the Audio on the A2DP source device and you should hear audio (via
the headphone or attached speaker).

During audio streaming, press the user button to adjust the headphone
port's audio volume.  The volume is set to 90% maximum volume at
initialization and decreases by 10% each time the user button is pressed.
The volume wraps to 100% once 0% is passed.

-------------------------------------------------------------------------------

Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 8555 of the DK-LM3S9B96-EM2-CC2560-BLUETOPIA Firmware Package.
