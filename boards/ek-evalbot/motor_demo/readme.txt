Motor Demo

This example application demonstrates the use of the motors.  The buttons
and bump sensors are used to start, stop, and reverse the motors.  It uses
the system tick timer (SysTick) as a time reference for button debouncing
and blinking LEDs.

When this example is running, the display shows a message identifying the
example.  The two user buttons on the right are used to control
the motors.  The top button controls the left motor and the bottom button
controls the right motor.  When a motor control button is pressed, the
motor runs in the forward direction.  When the button is pressed a second
time, the motor pauses.  Pressing the button a third time causes the motor
to run in reverse.  This cycle can be repeated by continuing to press the
button.

When a motor is running, either forward or reverse, pressing the bump
sensor pauses the motor.  When the bump sensor is released, the motor
resumes running in the same direction.

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
