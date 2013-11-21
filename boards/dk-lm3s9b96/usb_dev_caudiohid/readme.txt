USB Audio Device

This example application turns the evaluation board into a composite USB
device supporting the Human Interface Device keyboard class and a USB audio
device that supports playback of a single 16 bit stereo audio stream at
48 kHz sample rate.

The audio device supports only plackback and will respond to volume control
and mute changes and apply them to the sound driver.  These volume control
changes will only affect the headphone output and not the line output
because the audio DAC used on this board only allows volume changes to the
headphones.  The USB audio device example will work on any operating system
that supports USB audio class devices natively.  There should be no
addition operating system specific drivers required to use the example. The
application's main task is to pass buffers to the USB library's audio
device class, receive them back with audio data and pass the buffers on to
the sound driver for this board.

This keyboard device supports the Human Interface Device class and the
color LCD display shows a virtual keyboard and taps on the touchscreen will
send appropriate key usage codes back to the USB host. Modifier keys
(Shift, Ctrl and Alt) are ``sticky'' and tapping them toggles their state.
The board status LED is used to indicate the current Caps Lock state and is
updated in response to pressing the ``Caps'' key on the virtual keyboard or
any other keyboard attached to the same USB host system.  The keyboard
device implemented by this application also supports USB remote wakeup
allowing it to request the host to reactivate a suspended bus.  If the bus
is suspended (as indicated on the application display), touching the
display will request a remote wakeup assuming the host has not
specifically disabled such requests.

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

This is part of revision 9107 of the DK-LM3S9B96 Firmware Package.
