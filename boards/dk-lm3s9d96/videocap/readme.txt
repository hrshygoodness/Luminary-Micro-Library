Video Capture

This example application makes use of the optional FGPA daughter board to
capture and display motion video on the LCD display. VGA resolution
(640x480) video is captured from the daughter board camera and shown
either scaled to the QVGA (320x240) resolution of the display or full size,
in which case 25% of the image is visible and the user may scroll over the
full image by dragging a finger on the video area of the touchscreen.

The main screen of the application offers the following controls:
<dl>
<dt>Scale/Zoom</dt>
<dd>This button toggles the video display between scaled and zoomed modes.
In scaled mode, the 640x480 VGA video captured from the camera is
downscaled by a factor of two in each dimension making it fit on the
320x240 QVGA display.  In zoomed mode, the video image is shown without
scaling and is clipped before being placed onto the display.  The user can
drag a finger or stylus over the touchscreen to scroll the area of the video
which is visible.</dd>
<dt>Freeze/Unfreeze</dt>
<dd>Use this button to freeze and unfreeze the video on the display.  When
the video is frozen, a copy of the image may be saved to SDCard as a
Windows bitmap file by pressing the ``Save'' button.</dd>
<dt>Controls/Save</dt>
<dd>When motion video is being displayed, this button displays ``Controls''
and allows you to adjust picture brightness, saturation and contrast by
means of three slider controls which are shown when the button is pressed.
Once you are finished with image adjustments, pressing the ``Main'' button
will return you to the main controls screen.  When video is frozen, this
button shows ``Save'' and pressing it will save the currently displayed
video image onto a microSD card if one is installed.</dd>
<dt>Hide</dt>
<dd>This button hides all user interface elements to offer a clearer view
of the video.  To show the buttons again, press the small, red ``Show''
button displayed in the bottom right corner of the screen.</dd>
</dl>

Note that jumper ``PB4/POT'' on the main development kit board must be
removed when using the FPGA/Camera/LCD daughter board since the EPI signal
available on this pin is required for correct operation of the board.

-------------------------------------------------------------------------------

Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
