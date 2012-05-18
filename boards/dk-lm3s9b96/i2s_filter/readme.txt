I2S Record and Playback

This example application demonstrates recording audio from the codec's ADC,
transferring the audio over the I2S receive interface to the
microcontroller, and then sending the audio back to the codec via the I2S
trasmit interface.  A line level audio source should be fed into the LINE IN
audio jack which will be recorded with the codec's ADC and then played back
through both the HEADPHONE and LINE OUT jacks.  This application requires
some modifications to the default jumpers on the board in order for the
audio record path to function correctly.  The PD4/LD4 jumper should be
removed and placed on the PD4/RXSD jumper to receive I2S data.  The PD4/LD4
is located in the row of jumpers near the LCD connector while the PD4/RXSD
jumper is in the row near the audio jacks.

\note Moving this jumper will cause the LCD to not function for other
applications so the jumper should be moved back to run other applications.


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

This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
