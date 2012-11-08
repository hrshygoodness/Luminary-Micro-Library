Quickstart Oscilloscope

A two channel oscilloscope implemented using the Stellaris
microcontroller's analog-to-digital converter (ADC).  The oscilloscope
supports sample rates of up to 1M sample per second and will show the
captured waveforms on the color STN display.  On-screen menus provide user
control over timebase, channel voltage scale and position, trigger type,
trigger level and trigger position.  Other features include the ability
to save captured data as comma-separated-value files suitable for use with
spreadsheet applications or bitmap images on either an installed microSD
card or a USB flash drive.  The board may also be connected to a WindowsXP
or Vista host system and controlled remotely using a Windows application.

Oscilloscope User Interface

All oscilloscope controls and settings are accessed using the navigation
control on the board.  This control offers up, down, left, right and select
functions in a single unit.  Rocking the control in the desired direction
sends ``up'', ``down'', ``left'' or ``right'' messages to the application
and pressing on the center sends a ``select'' message.

Controls and settings are arranged into groups by function such as display
settings, trigger settings, file operations and setup choices.  These
groups are accessed by pressing ``select'' to display the main menu.  With
the menu displayed, use ``up'' and ``down'' to select between the available
groups.  When the desired group is highlighted, press ``select'' once again
to dismiss the menu.

Controls from the currently selected group are shown in the bottom portion
of the application display.  Use ``up'' and ``down'' to cycle through the
controls in the group and ``left'' and ``right'' to change the value of, or
select the action associated with the control which is currently
displayed.

The control groups and the individual controls offered by each are
outlined below:

  Group    Control       Setting
  -----    -------       -------

  Display
           Channel 2     ON or OFF.
           Timebase      Select values from 2uS to 50mS per division.
           Ch1 Scale     Select values from 100mV to 10V per division.
           Ch2 Scale     Select values from 100mV to 10V per division.
           Ch1 Offset    Press and hold "left" or "right" to move the
                         waveform up or down in 100mV increments.
           Ch2 Offset    Press and hold "left" or "right" to move the
                         waveform up or down in 100mV increments.
  Trigger
           Trigger       The trigger type - Always, Rising, Falling or
                         Level.
           Trig Channel  1 or 2 to select the channel used for triggering.
           Trig Level    Press and hold "left" or "right" to change the
                         trigger level in 100mV increments.
           Trig Pos      Press and hold "left" or "right" to move the
                         trigger position on the display.
           Mode          Running or Stopped.
           One Shot      If the current mode is "Stopped", pressing
                         "left" or "right" initiates capture and display
                         of a single waveform.
  Setup
           Captions      Select ON to show the timebase and scale
                         captions or OFF to remove them from the display.
           Voltages      Select ON to show the measured voltages for each
                         channel or OFF to remove them from the display.
           Grid          Select ON to show the graticule lines or OFF to
                         remove them from the display.
           Ground        Select ON to show dotted lines corresponding
                         to the ground levels for each channel or OFF
                         to remove them from the display.
           Trig Level    Select ON to show a solid horizontal line
                         corresponding to the trigger level for the
                         trigger channel or OFF to remove this line from
                         the display.
           Trig Pos      Select ON to show a solid vertical line at the
                         trigger position or OFF to remove this line from
                         the display.
           Clicks        Select ON to enable sounds on button presses or
                         OFF to disable them.
           USB Mode      Select Host to operate in USB host mode and
                         allow use of flash memory sticks, or Device to
                         operate as a USB device and allow connection to
                         a host PC system.
  File
           CSV on SD     Save the current waveform data as a text file
                         on an installed microSD card.
           CSV on USB    Save the current waveform data as a text file
                         on an installed USB flash stick (if in USB host
                         mode - see the Setup group above).
           BMP on SD     Save the current waveform display as a bitmap
                         on an installed microSD card.
           BMP on USB    Save the current waveform display as a bitmap
                         on an installed USB flash stick (if in USB host
                         mode - see the Setup group above).
  Help
           Help          Pressing "left" or "right" will show or hide the
                         screen showing oscilloscope connection help.
           Channel 1     Pressing "left" or "right" will cause the scale
                         and position for the channel 1 waveform to be set
                         such that the waveform is visible on the display.
           Channel 2     Pressing "left" or "right" will cause the scale
                         and position for the channel 2 waveform to be set
                         such that the waveform is visible on the display.

Oscilloscope Connections

The 8 pins immediately above the color STN display panel offer connections
for both channels of the oscilloscope and also two test signals that can be
used to provide input in the absence of any other suitable signals.  Each
channel input must lie in the range -16.5V to +16.5V relative to the board
ground allowing differences of up to 33V to be measured.

The connections are as follow where pin 1 is the leftmost pin, nearest the
microSD card socket:

 1    Test 1      A test signal connected to one side of the speaker on
                  the board.
 2    Channel 1+  This is the positive connection for channel 1 of the
                  oscilloscope.
 3    Channel 1-  This is the negative connection for channel 1 of the
                  oscilloscope.
 4    Ground      This is connected to board ground.
 5    Test 2      A test signal connected to the board Status LED which
                  is driven from PWM0.  This signal is configured to
                  provide a 1KHz square wave.
 6    Channel 2+  This is the positive connection for channel 2 of the
                  oscilloscope.
 7    Channel 2-  This is the negative connection for channel 2 of the
                  oscilloscope.
 8    Ground      This is connected to board ground.

Triggering and Sample Rate Notes

The oscilloscope can sample at a maximum combined rate of 1M samples
per second.  When both channels are enabled, therefore, the maximum
sample rate on each channel is 500K samples per second.  For maximum
resolution at the lowest timebases (maximum samples rates), disable
channel 2 if it is not required.  These sample rates give usable waveform
capture for signals up to around 100KHz.

Trigger detection is performed in software during ADC interrupt handling.
At the highest sampling rates, this interrupt service routine consumes
almost all the available CPU cycles when searching for trigger conditions.
At these sample rates, if a trigger level is set which does not correspond
to a voltage that is ever seen in the trigger channel signal, the user
interface response can become sluggish.  To combat this, the oscilloscope
will abort any pending waveform capture operation if a key is pressed
before the capture cycle as completed.  This prevents the user interface
from being locked out and allows the trigger level or type to be changed
to values more appropriate for the signal being measured.

File Operations

Comma-separated-value or bitmap files representing the last waveform
captured may be saved to either a microSD card or a USB flash drive.
In each case, the files are written to the root directory of the microSD
card or flash drive with file names of the form ``scopeXXX.csv'' or
``scopeXXX.bmp'' where ``XXX'' represents the lowest, three digit, decimal
number which offers a file name which does not already exist on the
device.

Companion Application

A companion application, LMScope, which runs on WindowsXP and Vista PCs
and the required device driver installer are available on the software CD
and via download from the TI Stellaris web site at
http://focus.ti.com/mcu/docs/mcuorphan.tsp?contentId=87903.  This
application offers full control of the oscilloscope from the PC and allows
waveform display and save to local hard disk.

Note that the USB device drivers for the oscilloscope device must be
installed prior to running the LMScope application.  If the drivers are not
installed when LMScope is run, the application will report that various
required DLLs including lmusbdll.dll and winusb.dll are missing.  To install
the drivers, make sure that the USB mode is set to ``Device'' in the Setup
menu then connect the ek-lm3s3748 board to a PC via the ``USB Device''
connector.  This will cause Windows to prompt for device driver
installation.  The required driver can be found on the kit installation
CD or in the ``SW-USB-windrivers-xxxx'' package downloadable from the
software update web site.

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

This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
