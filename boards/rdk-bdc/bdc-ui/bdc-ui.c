//*****************************************************************************
//
// bdc-ui.c - User interface to control a RDK-BDC.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 9453 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "shared/can_proto.h"
#include "about.h"
#include "bdc-ui.h"
#include "buttons.h"
#include "can_comm.h"
#include "config.h"
#include "current.h"
#include "dev_list.h"
#include "help.h"
#include "menu.h"
#include "position.h"
#include "rit128x96x4.h"
#include "speed.h"
#include "splash.h"
#include "update.h"
#include "vcomp.h"
#include "voltage.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Brushed DC User Interface (bdc-ui)</h1>
//!
//! This application provides a simple user interface for the Brushed DC Motor
//! Controller board, running on the EK-LM3S2965 board and communicating over
//! CAN.  In addition to running the motor, the motor status can be viewed, the
//! CAN network enumerated, and the motor controller's firmware can be updated.
//!
//! The direction buttons (left, right, up, and down) on the left side of the
//! EK-LM3S2965 are used to navigate through the user interface, and the select
//! button on the right side of the EK-LM3S2965 is used to select items.
//!
//! The user interface is divided into several panels; the top line of the
//! display always contains the name of the current panel.  By moving the
//! cursor to the top line and pressing select, a menu is displayed which will
//! allow a different panel to be displayed by pressing select again.
//!
//! Of the control modes available via this application, only voltage control
//! mode is usable with the motor and power supply provided with the RDK-BDC.
//! In order to use current control mode, a larger motor (with an attached
//! load) and power supply are required.  In order to use speed control mode, a
//! motor with an encoder is required.  In order to use position control mode,
//! a motor with an encoder or a potentiometer is required.
//!
//! The panels in the user interface will be individually discussed below.  At
//! startup, the Voltage Control Mode panel is displayed first.
//!
//!
//! \addtogroup example_list
//! <b>Voltage Control Mode</b>
//!
//! The voltage control mode panel allows the motor to be controlled by
//! directly selecting the output voltage.  The speed of the motor is directly
//! proportional to the voltage applied, and applying a ``negative'' voltage
//! (in other words, electronically reversing the power and ground connections)
//! will result in the motor spinning in the opposite direction.
//!
//! There are three parameters that can be adjusted on this panel: the ID,
//! voltage, and ramp rate.  The up and down buttons are used to select the
//! parameter to be modified, and the left and right buttons are used to adjust
//! the parameter's value.  The following parameters can be adjusted:
//!
//! - ID, which selects the motor controller to which commands are sent.  If
//!   the ID is changed while the motor is running, the motor will be stopped.
//!
//!   If the select button is pressed, a demonstration mode will be enabled or
//!   disabled.  In demonstration mode, the output voltage is automatically
//!   cycled through a sequence of values.
//!
//! - Voltage, which specifies the output voltage sent from the motor
//!   controller to the motor.  A positive voltage will result in voltage being
//!   applied to the white output terminal and ground being applied to the
//!   green output terminal, while a negative voltage will apply voltage to the
//!   green output terminal and ground to the white output terminal.
//!
//!   If the select button is pressed, changes to the output voltage will not
//!   be sent to the motor controller immediately (allowing the ramp to be
//!   used).  The text color of the voltage changes from white to black to
//!   indicate that a deferred update is active.  Pressing select again will
//!   send the final output voltage to the motor controller.
//!
//! - Ramp, which specifies the rate of change of the output voltage.  When set
//!   to ``none'', the output voltage will change immediately.  When set to a
//!   value, the output voltage is slowly changed from the current to to the
//!   target value at the specified rate.  This can be used to avoid browning
//!   out the power supply or to avoid over-torquing the motor on startup
//!   (for example preventing a loss of traction when a wheel is being driven).
//!
//! The bottom portion of the panel provides the current motor controller
//! status.
//!
//!
//! \addtogroup example_list
//! <b>Current Control Mode</b>
//!
//! The current control panel allows the motor to be controlled via closed-loop
//! current control.  The torque of the motor is directly proportional to the
//! current applied, and applying a ``negative'' current will result in the
//! motor spinning in the opposite direction.
//!
//! There are five parameters that can be adjusted on this panel: the ID,
//! current, and PID parameters.  The up and down buttons are used to select
//! the parameter to be modified, and the left and right buttons are used to
//! adjust the parameter's value.  The following parameters can be adjusted:
//!
//! - ID, which selects the motor controller to which commands are sent.  If
//!   the ID is changed while the motor is running, the motor will be stopped.
//!
//!   If the select button is pressed, a demonstration mode will be enabled or
//!   disabled.  In demonstration mode, the output current is automatically
//!   cycled through a sequence of values.
//!
//! - Current, which specifies the output current sent from the motor
//!   controller to the motor.  A positive current will result in voltage being
//!   applied to the white output terminal and ground being applied to the
//!   green output terminal, while a negative current will apply voltage to the
//!   green output terminal and ground to the white output terminal.
//!
//!   If the select button is pressed, changes to the output current will not
//!   be sent to the motor controller immediately (allowing an arbitrary step
//!   function to be applied).  The text color of the current changes from
//!   white to black to indicate that a deferred update is active.  Pressing
//!   select again will send the final output current to the motor controller.
//!
//! - P coefficient, which specifies the gain applied to the instantaneous
//!   motor current error.
//!
//! - I coefficient, which specifies the gain applied to the integral of the
//!   motor current error.
//!
//! - D coefficient, which specifies the gain applied to the derivitive of the
//!   motor current error.
//!
//! The bottom portion of the panel provides the current motor controller
//! status.
//!
//!
//! \addtogroup example_list
//! <b>Speed Control Mode</b>
//!
//! The speed control panel allows the motor to be controlled via closed-loop
//! speed control.  The voltage applied to the motor is varied in order to
//! achieve a desired output speed.  Applying a ``negative'' speed will result
//! in the motor spinning in the opposite direction.
//!
//! The speed control mode requires that an encoder be attached to the output
//! of the motor, either directly to the motor's output shaft or at some stage
//! within or after the gearbox that is optionally attached to the motor.
//! Examples of encoders that can be used are quadrature encoders and gear
//! tooth sensors.  The speed will be regulated based on the measurement point;
//! if the output speed of a gearbox is measured, then the motor will be
//! running faster or slower than the desired speed (based on the gear ratio of
//! the gearbox) in order to make the gearbox output match the set speed.
//!
//! There are five parameters that can be adjusted on this panel: the ID,
//! speed, and PID parameters.  The up and down buttons are used to select the
//! parameter to be modified, and the left and right buttons are used to adjust
//! the parameter's value.  The following parameters can be adjusted:
//!
//! - ID, which selects the motor contorller to which commands are sent.  If
//!   the ID is changed while the motor is running, the motor will be stopped.
//!
//!   If the select button is pressed, a demonstratino mode will be enabled or
//!   disabled.  In demonstratino mode, the output speed is automatically
//!   cycled through a sequence of values.
//!
//! - Speed, which specifies the speed that the motor should run.  A positive
//!   speed will result in voltage being applied to the white output terminal
//!   and groud being applied to the green output terminal, while a negative
//!   speed will apply voltage to the green output terminal and ground to the
//!   white output terminal.
//!
//!   If the select button is pressed, changes to the output speed will not be
//!   sent to the motor controller immediately (allowing an arbitrary step
//!   function to be applied).  The text color of the speed changes from white
//!   to black to indicate that a deferred update is active.  Pressing select
//!   again will send the final output speed to the motor controller.
//!
//! - P coefficient, which specifies the gain applied to the instantaneous
//!   motor speed error.
//!
//! - I coefficient, which specifies the gain applied to the integral of the
//!   motor speed error.
//!
//! - D coefficient, which specifies the gain applied to the derivitive of the
//!   motor speed error.
//!
//! The bottom portion of the panel provides the current motor controller
//! status.
//!
//!
//! \addtogroup example_list
//! <b>Position Control Mode</b>
//!
//! The position control panel allows the motor to be controlled via
//! closed-loop position control.  The voltage applied to the motor is varied
//! in order to move the shaft to a desired position.  The motor will spin in
//! either direction in order to achieve the requested position.
//!
//! There are six parameters that can be adjusted on this panel: the ID,
//! position, PID parameters, and position reference.  The up and down buttons
//! are used to select the parameter to be modified, and the left and right
//! buttons are used to adjust the parameter's value.  The following parameters
//! can be adjusted:
//!
//! - ID, which selects the motor controller to which commands are sent.  If
//!   the ID is changed while the motor is running, the motor will be stopped.
//!
//!   If the select button is pressed, a demonstration mode will be enabled or
//!   disabled.  In demonstratino mode, the motor position is automatically
//!   cycled through a sequence of values.
//!
//! - Position, which specifies the position to which the motor should turn.
//!
//!   If the select button is pressed, changes to the position will not be sent
//!   to the motor contorller immediately (allowing an arbitrary step funciton
//!   to be applied).  The text color of the position changes from white to
//!   black to indicate that a deferred update is active.  Pressing select
//!   again will send the final output position to the motor controller.
//!
//! - P coefficient, which specifies the gain applied to the instantaneous
//!   motor position error.
//!
//! - I coefficient, which specifies the gain applied to the integral of the
//!   motor position error.
//!
//! - D coefficient, which specifies the gain applied to the derivitive of the
//!   motor position error.
//!
//! - Position reference, which specifies how the motor position is measured.
//!   If this is set to ``encoder'', an encoder (such as a quadrature encoder)
//!   is used to measure the motor position (a gear tooth sensor can not be
//!   used with position control mode).  If this is set to ``potentiometer'', a
//!   potentiometer coupled to the motor output shaft (pre- or post-gearbox) is
//!   used to measure the motor position.
//!
//! The bottom portion of the panel provides the current motor controller
//! status.
//!
//!
//! \addtogroup example_list
//! <b>Configuration</b>
//!
//! This panel allows general parameters of the motor controller to be
//! configured.  There are ten parameters that can be adjusted on this panel:
//! the ID, number of encoder lines, number of potentiometer turns, brake or
//! coast, soft limit switch enable, forward soft limit switch position,
//! forward soft limit switch comparison, reverse soft limit switch position,
//! reverse soft limit switch comparison, and maximum output voltage.  The up
//! and down buttons are used to select the parameter to be modified, and the
//! left and right buttons are used to adjust the parameter's value.  The
//! following parameters can be adjusted:
//!
//! - ID, which selects the motor controller that is to be configured.
//!
//! - Encoder lines, which specifies the number of lines in the attached
//!   encoder.  When using a quadrature encoder, this number will match the
//!   clocks per revolution (CPR) specified by the encoder manufacturer.  When
//!   using a gear tooth sensor, this will be twice the number of teeth in the
//!   gear that is being measured.
//!
//! - Potentiometer turns, which specifies the number of full turn in the
//!   travel of the potentiometer.  Typical potentiometers used for rotational
//!   measurement have one, three, five, or ten turns in their travel.
//!
//! - Brake/coast, which specifies the action to be taken when the motor is
//!   stopped.  This can be set to ``jumper'', which uses the brake/coast
//!   jumper on the MDL-BDC to determine whether to brake or coast, ``brake''
//!   to apply dynamic braking, and ``coast'' to electrically disconnect the
//!   motor windings and allow it to coast to a stop under the affects of
//!   friction.
//!
//! - Soft limit, which specifies whether or not the soft limit switches are
//!   enabled.  When enabled, the soft limit switches use measured motor
//!   position to prevent the motor from running forward or backward.  For
//!   positioning applications, which require either an encoder or a
//!   potentiometer, this allows the use of limit switches (to prevent
//!   rotational extremes, thereby protecting the attached assembly) without
//!   the need to physically place and wire up real switches.
//!
//! - Forward limit, which specifies the motor position that corresponds to the
//!   position of the forward soft limit switch.
//!
//! - Forward compare, which specifies the comparison applied to the forward
//!   soft limit switch.  This will be ``lt'' if the motor position must be
//!   less than the position of the forward limit switch in order to run
//!   forward, or ``gt'' if it must be greater.  The ``lt'' setting will be
//!   used for setups where positive voltage applied to the motor results in
//!   the measured motor position increasing, and ``gt'' will be used for
//!   setups where positive voltage results in the measured motor position
//!   decreasing.
//!
//! - Reverse limit, which specifies the motor position that corresopnds to the
//!   position of the reverse soft limit switch.
//!
//! - Reverse compare, which specifies the comparison applied to the reverse
//!   soft limit switch.  This will be ``lt'' if the motor position must be
//!   less than the position of the reverse limit switch in order to run
//!   backward, or ``gt'' if it must be greater.  The ``gt'' setting will be
//!   used for setups where positive voltage applied to the motor results in
//!   the measured motor position increasing, and ``lt'' will be used for
//!   setups where positive voltage results in the measured motor position
//!   decreasing.
//!
//! - Maximum output voltage, which specifies the maximum voltage that can be
//!   safely applied to the attached motor.  All voltage commands are scaled
//!   such that a ``full scale'' voltage output matches this value.  This can
//!   be used to attach 7.2V motor (for example) to the MDL-BDC and avoid
//!   applying 12V to them.
//!
//!
//! \addtogroup example_list
//! <b>Device List</b>
//!
//! This panel lists the motor controllers that reside on the CAN network.  All
//! 63 possible device IDs are listed, with those that are not present shown in
//! a dark gray and those that are present in a bright white.  By moving the
//! cursor to a particular ID and pressing the select button, a device ID
//! assignment will be performed.  The motor controller(s) will wait for five
//! seconds after an assignment request for its button to be pressed,
//! indicating that it should accept the device ID assignment.  So, for
//! example, if there are three motor controllers on a network, the following
//! sequence can be used to give them each unique IDs:
//!
//! - Move the cursor to number 1 and press select.  The LED on all three motor
//!   controllers will blink green to indicate that assignment mode is active.
//!
//! - Press the button on one of the motor controllers.  It will blink its LED
//!   yellow one time to indicate that it's ID is one.
//!
//! - Move the cursor to number 2 and press select.
//!
//! - Press the button on the second motor controller.  It will blink its LED
//!   yellow two times to indicate that it's ID is two.
//!
//! - Move the cursor to number 3 and press select.
//!
//! - Press the button on the third motor controller.  It will blink its LED
//!   yellow three times to indicate that it's ID is three.
//!
//! Once complete, this panel will then show that there are devices at IDs 1,
//! 2, and 3.
//!
//!
//! \addtogroup example_list
//! <b>Firmware Update</b>
//!
//! This panel allows the firmware on the motor controller to be updated over
//! the CAN network.  A firmware image for the motor controller is stored in
//! the flash of the EK-LM3S2965 board and used to update the motor controller.
//! The local copy of the motor controller firmware can be updated using the
//! UART boot loader protocol to transfer the image from a PC.  When the UART
//! ``update'' begins, this panel will automatically be displayed.
//!
//! The ID of the motor controller to be updated can be changed on this panel.
//! By using the local firmware image, multiple motor controllers can be
//! updated (one at a time) using this panel, without the need to redownload
//! from a PC each time.
//!
//! When not updating, the firmware version of the currently selected motor
//! controller will be displayed.  If there is no motor controller on the CAN
//! network with the current ID, the firmware version will be displayed as
//! ``---''.
//!
//! By pressing the select button when the ``Start'' button is highlighted, the
//! update of the motor controller firmware will commence.
//!
//! When the firmware is being transferred (either from the PC using the UART
//! or to the motor controller using the CAN network), the ID, firmware
//! version, and ``Start'' buttons will all be grayed out.  A progress bar will
//! appear below them to indicate what is happening and the how far it is
//! through the process.
//!
//!
//! \addtogroup example_list
//! <b>Help</b>
//!
//! This panel displays a condensed version of this application help text.  Use
//! the up and down buttons to scroll through the text.
//!
//!
//! \addtogroup example_list
//! <b>About</b>
//!
//! This panel simply displays the startup splash screen.
//
//*****************************************************************************

//*****************************************************************************
//
// A set of flags that indicate global status for the application.
//
//*****************************************************************************
volatile unsigned long g_ulFlags;

//*****************************************************************************
//
// The count of timer ticks, used for controlling demo mode.
//
//*****************************************************************************
volatile unsigned long g_ulTickCount = 0;

//*****************************************************************************
//
// A widget that causes the entire display to be cleared.
//
//*****************************************************************************
static Canvas(g_sBackground, 0, 0, 0, &g_sRIT128x96x4Display, 0, 0, 128, 96,
              CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// A mapping from buttons returned by ButtonsTick() to flags in g_ulFlags.
//
//*****************************************************************************
static unsigned long g_ppulButtonMap[][2] =
{
    { BUTTON_UP, FLAG_UP_PRESSED },
    { BUTTON_DOWN, FLAG_DOWN_PRESSED },
    { BUTTON_LEFT, FLAG_LEFT_PRESSED },
    { BUTTON_LEFT << 8, FLAG_LEFT_ACCEL1 },
    { BUTTON_LEFT << 16, FLAG_LEFT_ACCEL2 },
    { BUTTON_LEFT << 24, FLAG_LEFT_ACCEL3 },
    { BUTTON_RIGHT, FLAG_RIGHT_PRESSED },
    { BUTTON_RIGHT << 8, FLAG_RIGHT_ACCEL1 },
    { BUTTON_RIGHT << 16, FLAG_RIGHT_ACCEL2 },
    { BUTTON_RIGHT << 24, FLAG_RIGHT_ACCEL3 },
    { BUTTON_SELECT, FLAG_SELECT_PRESSED }
};

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFileName, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// This function is called by SysTick once every millisecond.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    unsigned long ulButtons, ulIdx;

    //
    // Set the flag that indicates that a timer tick has occurred.
    //
    HWREGBITW(&g_ulFlags, FLAG_TICK) = 1;

    //
    // Increment the count of ticks.
    //
    g_ulTickCount++;

    //
    // Call the CAN periodic tick function.
    //
    CANTick();

    //
    // Call the push button periodic tick function.
    //
    ulButtons = ButtonsTick();

    //
    // Set the appropriate button press flags if the corresponding button was
    // pressed.
    //
    for(ulIdx = 0;
        ulIdx < (sizeof(g_ppulButtonMap) / sizeof(g_ppulButtonMap[0]));
        ulIdx++)
    {
        if(ulButtons & g_ppulButtonMap[ulIdx][0])
        {
            HWREGBITW(&g_ulFlags, g_ppulButtonMap[ulIdx][1]) = 1;
        }
    }
}

//*****************************************************************************
//
// This function displays the splash screen.
//
//*****************************************************************************
static void
DisplaySplash(void)
{
    unsigned long ulIdx;
    tContext sContext;
    tRectangle sRect;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sContext, &g_sRIT128x96x4Display);

    //
    // Clear the screen.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = 127;
    sRect.sYMax = 95;
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &sRect);

    //
    // Draw the splash screen image on the screen.
    //
    GrImageDraw(&sContext, g_pucSplashImage, 0, 10);

    //
    // Draw some text below the splash screen image.
    //
    GrContextForegroundSet(&sContext, ClrWhite);
    GrContextFontSet(&sContext, g_pFontFixed6x8);
    GrStringDrawCentered(&sContext, "Brushed DC Motor", -1, 63, 73, 0);
    GrStringDrawCentered(&sContext, "Reference Design Kit", -1, 63, 81, 0);

    //
    // Flush the drawing operations to the screen.
    //
    GrFlush(&sContext);

    //
    // Delay for 5 seconds while the splash screen is displayed.
    //
    for(ulIdx = 0; ulIdx < 5000; ulIdx++)
    {
        HWREGBITW(&g_ulFlags, FLAG_TICK) = 0;
        while(HWREGBITW(&g_ulFlags, FLAG_TICK) == 0)
        {
        }
    }

    //
    // Ignore any buttons that were pressed while the splash screen was
    // displayed.
    //
    HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) = 0;
    HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) = 0;
    HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) = 0;
    HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) = 0;
    HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) = 0;
}

//*****************************************************************************
//
// This function causes the display to be redrawn.
//
//*****************************************************************************
void
DisplayFlush(void)
{
    //
    // Send a paint message to the entire widget tree.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Process any widget messages in the message queue.
    //
    WidgetMessageQueueProcess();

    //
    // Flush the drawing operations to the screen.
    //
    DpyFlush(&g_sRIT128x96x4Display);
}

//*****************************************************************************
//
// The main loop for the user interface.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulPanel;

    //
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    // a workaround to allow the PLL to operate reliably.
    //
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }

    //
    // Set the clocking to run at 50MHz from the PLL.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Set the priority of the interrupts.
    //
    IntPrioritySet(INT_CAN0, 0x00);
    IntPrioritySet(FAULT_SYSTICK, 0x20);

    //
    // Configure SysTick to generate an interrupt every millisecond.
    //
    SysTickPeriodSet(SysCtlClockGet() / 1000);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Initialize the push button driver.
    //
    ButtonsInit();

    //
    // Initialize the CAN communication channel.
    //
    CANCommInit();

    //
    // Initialize the UART used to perform a "firmware update".
    //
    UpdateUARTInit();

    //
    // Initialize the display.
    //
    RIT128x96x4Init(3500000);

    //
    // Add the screen-clearing widget to the widget tree.  As the first widget
    // in the tree, this will always be drawn first, resulting in a blank
    // screen before anything else is drawn.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBackground);

    //
    // Display the splash screen.
    //
    DisplaySplash();

    //
    // Set the CAN device ID to one.
    //
    CANSetID(1);

    //
    // The "Voltage Control Mode" panel should be displayed first.
    //
    ulPanel = PANEL_VOLTAGE;

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Determine which panel to display.
        //
        switch(ulPanel)
        {
            //
            // The "Voltage Control Mode" panel should be displayed.
            //
            case PANEL_VOLTAGE:
            {
                ulPanel = DisplayVoltage();
                break;
            }

            //
            // The "VComp Control Mode" panel should be displayed.
            //
            case PANEL_VCOMP:
            {
                ulPanel = DisplayVComp();
                break;
            }

            //
            // The "Current Control Mode" panel should be displayed.
            //
            case PANEL_CURRENT:
            {
                ulPanel = DisplayCurrent();
                break;
            }

            //
            // The "Speed Control Mode" panel should be displayed.
            //
            case PANEL_SPEED:
            {
                ulPanel = DisplaySpeed();
                break;
            }

            //
            // The "Position Control Mode" panel should be displayed.
            //
            case PANEL_POSITION:
            {
                ulPanel = DisplayPosition();
                break;
            }

            //
            // The "Configuration" panel should be displayed.
            //
            case PANEL_CONFIGURATION:
            {
                ulPanel = DisplayConfig();
                break;
            }

            //
            // The "Device List" panel should be displayed.
            //
            case PANEL_DEV_LIST:
            {
                ulPanel = DisplayDevList();
                break;
            }

            //
            // The "Firmware Update" panel should be displayed.
            //
            case PANEL_UPDATE:
            {
                ulPanel = DisplayUpdate();
                break;
            }

            //
            // The "Help" panel should be displayed.
            //
            case PANEL_HELP:
            {
                ulPanel = DisplayHelp();
                break;
            }

            //
            // The "About" panel should be displayed.
            //
            case PANEL_ABOUT:
            {
                ulPanel = DisplayAbout();
                break;
            }
        }
    }
}
