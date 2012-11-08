Graphics Library Demonstration

This application provides a demonstration of the capabilities of the
Stellaris Graphics Library.  A series of panels show different features of
the library.  For each panel, the bottom provides a forward and back button
(when appropriate), along with a brief description of the contents of the
panel.

The first panel provides some introductory text and basic instructions for
operation of the application.

The second panel shows the available drawing primitives: lines, circles,
rectangles, strings, and images.

The third panel shows the canvas widget, which provides a general drawing
surface within the widget heirarchy.  A text, image, and application-drawn
canvas are displayed.

The fourth panel shows the check box widget, which provides a means of
toggling the state of an item.  Four check boxes are provided, with each
having a red ``LED'' to the right.  The state of the LED tracks the state
of the check box via an application callback.

The fifth panel shows the container widget, which provides a grouping
construct typically used for radio buttons.  Containers with a title, a
centered title, and no title are displayed.

The sixth panel shows the push button widget.  Two columns of push buttons
are provided; the appearance of each column is the same but the left column
does not utilize auto-repeat while the right column does.  Each push button
has a red ``LED'' to its left, which is toggled via an application callback
each time the push button is pressed.

The seventh panel shows the radio button widget.  Two groups of radio
buttons are displayed, the first using text and the second using images for
the selection value.  Each radio button has a red ``LED'' to its right,
which tracks the selection state of the radio buttons via an application
callback.  Only one radio button from each group can be selected at a time,
though the radio buttons in each group operate independently.

The eighth panel shows the slider widget.  Six sliders constructed using
the various supported style options are shown.  The slider value callback
is used to update two widgets to reflect the values reported by sliders.
A canvas widget near the top right of the display tracks the value of the
red and green image-based slider to its left and the text of the grey slider
on the left side of the panel is update to show its own value.  The
slider second from the right is configured as an indicator which tracks the
state of the upper slider and ignores user input.

The final panel provides instructions and information necessary to update
the board firmware via serial using the LM Flash Programmer application.
When the ``Update'' button is pressed, the application transfers control to
the boot loader to allow a new application image to be downloaded.  The
LMFlash serial data rate must be set to 115200bps and the "Program Address
Offset" to 0x800.

UART0, which is connected to the 6 pin header on the underside of the
IDM-L35 RDK board (J8), is configured for 115200bps, and 8-n-1 mode.  The
USB-to-serial cable supplied with the IDM-L35 RDK may be used to connect
this TTL-level UART to the host PC to allow firmware update.

-------------------------------------------------------------------------------

Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 9453 of the Stellaris Graphics Library.
