EVALBOT Controlled by an eZ430-Chronos Watch

This application allows the EVALBOT to be driven under radio control from
a 915 MHz eZ430-Chronos sport watch.  To run it, you need both the sport
watch (running its default firmware) and a "CC1101 Evaluation Module
868-915", part number CC1101EMK868-915.  Both of these can be ordered from
estore.ti.com or other Texas Instruments part distributors.

To run the demonstration:

<ol>
<li>Place the EVALBOT within a flat, enclosed space then press the
"On/Reset" button. You should see scrolling text appear on its OLED display.
If you wait 5 seconds at this step and without doing anything else, the
robot will start driving, making random turns or turning away from anything
it bumps.
</li>
<li>Hold the Chronos watch level and repeatedly press the bottom left
button on the Chronos watch until you see "ACC" displayed, then press the
watch's bottom right button to enable the radio.</li>
<li>After a few seconds, the EVALBOT links with the watch and stops
driving.  The display also indicates that it is connected to a Chronos.  If
this does not occur within 5 seconds, turn the Chronos radio off by
pressing the bottom right button again and then resetting the EVALBOT using
the "On/Reset" button.</li>
<li>Once the watch and EVALBOT are connected, drive the robot by tilting the
watch. Tilting the watch forward and backward controls the direction -
tilting forward moves the robot forward, and tilting backward moves it in
reverse.  The speed is controlled by the amount of tilt.  When reversing,
the robot beeps.</li>
<li>To turn the robot, when it is moving in forward or reverse, tilt the
watch left or right to initiate a turn.</li>
</ol>

While controlling the EVALBOT, the watch buttons perform the following
actions:

<dl>
<dt>Top Left</dt>     <dd>Stop the EVALBOT.</dd>
<dt>Bottom Left</dt>  <dd>Restart the EVALBOT.</dd>
<dt>Top Right</dt>    <dd>Sound the EVALBOT horn.</dd>
<dt>Bottom Right</dt> <dd>Turn Chronos radio on or off.</dd>
</dl>

When controlling the EVALBOT, maximum speed is achieved with the watch face
oriented vertically (90 degrees from the normal, "flat" viewing position).
Since the accelerometer calibration varies slightly from watch to watch, the
example contains a calibration mode.  Press "Switch 1" on EVALBOT to enter
calibration then move the watch through the full range of movement you want
to use to control the robot. Once you have finished the movement, press
"Switch 2" to resum normal operation with the control inputs scaled
appropriately.

The application can be easily recompiled to operate using either the 433 MHz
or 868 Mhz version of the eZ430-Chronos.  To operate with a 433 MHz watch,
a CC1101EMK433 is required in place of the CC1101EMK868-915 since the
antenna design is different between these two frequency bands.

To compile for different frequencies, modify the
<tt>simpliciti-config.h</tt> file to ensure that labels are defined as
follows and then rebuild the application.

For 915 MHz operation (as used in the USA):

#define ISM_US
#undef ISM_LF
#undef ISM_EU

For 868 MHz operation (as used in Europe):

#undef ISM_US
#undef ISM_LF
#define ISM_EU

For 433 MHz operation (as used in Japan):

#undef ISM_US
#define ISM_LF
#undef ISM_EU

-------------------------------------------------------------------------------

Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 10636 of the Stellaris Firmware Development Package.
