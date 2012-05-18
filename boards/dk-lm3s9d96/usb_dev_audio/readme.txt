USB Audio Device

This example application makes the evaluation board a USB audio device
that supports a single 16 bit stereo audio stream at 48 kHz sample rate.
The application can also receive volume control and mute changes and apply
them to the sound driver.  These changes will only affect the headphone
output and not the line output because the audio DAC used on this board
only allows volume changes to the headphones.

The USB audio device example will work on any operating system that
supports USB audio class devices natively.  There should be no addition
operating system specific drivers required to use the example.   The
application's main task is to pass buffers to the  the USB library's audio
device class, receive them back with audio data and pass the buffers on to
the sound driver for this board.

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

This is part of revision 8555 of the DK-LM3S9D96 Firmware Package.
