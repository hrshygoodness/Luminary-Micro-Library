Access Point for use with eZ430-Chronos-433

This application provides a SimpliciTI Low Power RF access point that is
capable of receiving and displaying data from an eZ430-Chronos-433
development tool running the default Sports Watch firmware.  The
development board must be equipped with an EM2 expansion board and a
CC1101EM 433Mhz radio transceiver and antenna for this application to run
correctly.  The CC1101EM module must be installed in the ``mod1'' connector
(the connector nearest the oscillator) on the EM2 expansion board.

The eZ430-Chronos-433 development tool uses the SimpliciTI protocol to
support three of the Sport Watch features - ``RF Tilt Control'', ``PPT
Control'' and ``RF Sync''. The simpliciti_chronos example application
detects which of these modes is in operation and configures the display
appropriately to show the information that the watch is transmitting.  If
no packet is! received from the watch within 5 seconds, the display reverts
to the initial opening screen and the application waits for new data.

To select each of the operating modes, cycle through the various options
offered by pressing the lower left ``#'' button on the sports watch until
either ``ACC'', ``PPt'' or ``SYNC'' is displayed. To activate data
transmission press the lower right, down arrow key.  When transmission is
active, a wireless icon will flash on the watch's display.  Pressing the
down arrow key once again will disable the transmitter.

<h3>RF Tilt Control Mode (ACC)</h3>

In RF Tilt Control mode, the watch sends packets containing information on
button presses and also the output from its integrated 3-axis
accelerometer.  The simpliciti_chronos application displays button presses
by highlighting graphics of the buttons on the display.  Accelerometer
data is plotted in an area of the screen with (x,y) data used to determine
the position of lines drawn and z data controlling the color.  Buttons are
provided on the display to set the origin point for the accelerometer
data (``Calibrate'') and clear the accelerometer display (``Clear'').

<h3>PowerPoint Control Mode (PPt)</h3>

In PowerPoint Control mode, only button press information is transmitted
by the watch.  When these packets are detected by the access point, it
displays an image of the watch face and highlights which buttons have been
pressed.

<h3>Sync Mode (SYNC)</h3>

Sync mode is used to set various watch parameters and allows an access
point to send commands to the watch and retrieve status from it.  Every
500mS or so, the watch sends a ``ready to receive'' packet indicating that
it is able to process a new command.  The access point application
responds to this with a status request causing the watch to send back a 19
byte status packet containing the current time, date, alarm time, altitude
and temperature.  This data is then formatted and displayed on the
development board LCD screen.  A button on the display allows the watch
display format to be toggled between metric and imperial.  When this button
is pressed, a command is sent to the watch to set the new format and this
display format change takes effect with the next incoming status message.

Note that the ``ready to receive'' packets sent by the watch are not
synchronized with the watch second display.  This introduces an
interference effect that results in the development board seconds display
on the LCD not update at regular 1 second intervals.

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

This is part of revision 9453 of the DK-LM3S9D96-EM2-CC1101_433-SIMPLICITI Firmware Package.
