//*****************************************************************************
//
// bldc_ctrl.c - A simple GUI to control a BLDC RDK board.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/container.h"
#include "grlib/pushbutton.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/ustdlib.h"
#include "drivers/formike240x320x16_ili9320.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "images.h"
#include "motor.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>BLDC RDK Control (bldc_ctrl)</h1>
//!
//! This application provides a simple GUI for controlling a BLDC RDK board.
//! The motor can be started and stopped, the target speed can be adjusted, and
//! the current speed can be monitored.
//!
//! The target speed up and down buttons utilize the auto-repeat capability of
//! the push button widget.  For example, pressing the up button will increase
//! the target speed by 100 rpm.  Holding it for more than 0.5 seconds will
//! commence the auto-repeat, at which point the target speed will increase by
//! 100 rpm every 1/10th of a second.  The same behavior occurs on the down
//! button.
//!
//! Upon startup, the application will attempt to contact a DHCP server to get
//! an IP address.  If a DHCP server can not be contacted, it will instead use
//! the IP address 169.254.19.70 without performing any ARP checks to see if it
//! is already in use.  Once the IP address is determined, it will initiate a
//! connection to a BLDC RDK board at IP address 169.254.89.71.  While
//! attempting to contact the DHCP server and the BLDC RDK board, the target
//! speed will display as a set of bouncing dots.
//!
//! The push buttons will not operate until a connection to a BLDC RDK board
//! has been established.
//!
//! This application supports remote software update over Ethernet using the
//! LM Flash Programmer application.  A firmware update is initiated using the
//! remote update request ``magic packet'' from LM Flash Programmer.  This
//! feature is available in versions of LM Flash Programmer with build numbers
//! greater than 560.
//
//*****************************************************************************

//*****************************************************************************
//
// The sound effect that is played when a key is pressed.
//
//*****************************************************************************
static const unsigned short g_pusKeyClick[] =
{
    0, G5,
    25, SILENCE
};

//*****************************************************************************
//
// The count of SysTick interrupts since the last time we attempted to connect
// to the motor.
//
//*****************************************************************************
static volatile unsigned long g_ulCount = 0;

//*****************************************************************************
//
// The count of SysTick interrupts since the last time we toggled between
// display of the IP address and the MAC address.
//
//*****************************************************************************
static volatile unsigned long g_ulIPDisplayCount = 0;

//*****************************************************************************
//
// A flag used to signal to the main loop that a remote ethernet firmware
// update request has been received.
//
//*****************************************************************************
static volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
// The previous motor speed; the motor speed display is only updated when the
// motor speed does not match this value.  When attempting to connect to the
// BLDC RDK board, this controls the state machine that displays the bouncing
// dots.
//
//*****************************************************************************
static unsigned long g_ulPrevious;

//*****************************************************************************
//
// This buffer contains the ASCII representation of the target motor speed,
// which is used by a canvas widget to display the target speed on the screen.
//
//*****************************************************************************
static char g_pcTargetBuffer[16] = "  3000 rpm  ";

//*****************************************************************************
//
// This buffer contains the ASCII representation of the current motor speed,
// which is used by a canvas widget to display the current speed on the screen.
//
//*****************************************************************************
static char g_pcSpeedBuffer[16] = "       .       ";

//*****************************************************************************
//
// This buffer contains the ASCII representation of the module's IP or MAC
// address.
//
//*****************************************************************************
static char g_pcEthernetAddr[32] = "";

//*****************************************************************************
//
// This defines the number of milliseconds that the IP or MAC address remains
// displayed before the string toggles.
//
//*****************************************************************************
#define IP_ADDR_DISPLAY_TIME_MS 2000

//*****************************************************************************
//
// Forward declarations for functions called by the widgets used in the user
// interface.
//
//*****************************************************************************
void OnRun(tWidget *pWidget);
void OnStop(tWidget *pWidget);
void OnUp(tWidget *pWidget);
void OnDown(tWidget *pWidget);
void OnPaint(tWidget *pWidget, tContext *pContext);

//*****************************************************************************
//
// A red push button with the text "Stop" on it.  Pressing this button will
// command the motor to stop.
//
//*****************************************************************************
RectangularButton(g_sStop, WIDGET_ROOT, 0, 0, &g_sFormike240x320x16_ILI9320,
                  135, 256, 90, 50, PB_STYLE_TEXT | PB_STYLE_IMG, ClrBlack,
                  ClrBlack, ClrSilver, ClrSilver, g_pFontCmss24b, "Stop",
                  g_pucRed90x50, g_pucRed90x50Press, 0, 0, OnStop);

//*****************************************************************************
//
// A green push button with the text "Run" on it.  Pressing this button will
// command the motor to run.
//
//*****************************************************************************
RectangularButton(g_sRun, WIDGET_ROOT, &g_sStop, 0,
                  &g_sFormike240x320x16_ILI9320, 135, 196, 90, 50,
                  PB_STYLE_TEXT | PB_STYLE_IMG, ClrBlack, ClrBlack, ClrSilver,
                  ClrSilver, g_pFontCmss24b, "Run", g_pucGreen90x50,
                  g_pucGreen90x50Press, 0, 0, OnRun);

//*****************************************************************************
//
// A canvas widget with the Texas Instruments logo on it.
//
//*****************************************************************************
Canvas(g_sLogo, WIDGET_ROOT, &g_sRun, 0, &g_sFormike240x320x16_ILI9320, 25,
       212, 80, 75, CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucTILogo, 0);

//*****************************************************************************
//
// A canvas widget that displays the current motor speed.  When a connection is
// being made to the BLDC RDK, this will have a set of bouncing dots.
//
//*****************************************************************************
Canvas(g_sSpeed, WIDGET_ROOT, &g_sLogo, 0, &g_sFormike240x320x16_ILI9320, 20,
       155, 200, 25, CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack,
       ClrBlack, ClrSilver, g_pFontCmss24, g_pcSpeedBuffer, 0, 0);

//*****************************************************************************
//
// A container widget that surrounds the current motor speed display.
//
//*****************************************************************************
Container(g_sSpeedBox, WIDGET_ROOT, &g_sSpeed, 0,
          &g_sFormike240x320x16_ILI9320, 5, 138, 230, 47,
          CTR_STYLE_OUTLINE | CTR_STYLE_TEXT | CTR_STYLE_TEXT_CENTER, ClrBlack,
          ClrSilver, ClrSilver, g_pFontCmss18, "Current Speed");

//*****************************************************************************
//
// A blue push button with the text "+" on it.  Pressing this button will
// increase the target motor speed.
//
//*****************************************************************************
RectangularButton(g_sUp, WIDGET_ROOT, &g_sSpeedBox, 0,
                  &g_sFormike240x320x16_ILI9320, 175, 72, 50, 50,
                  PB_STYLE_TEXT | PB_STYLE_IMG | PB_STYLE_AUTO_REPEAT,
                  ClrBlack, ClrBlack, ClrSilver, ClrSilver, g_pFontCmss24b,
                  "+", g_pucBlue50x50, g_pucBlue50x50Press, 125, 25, OnUp);

//*****************************************************************************
//
// A blue push button with the text "-" on it.  Pressing this button will
// decrease the target motor speed.
//
//*****************************************************************************
RectangularButton(g_sDown, WIDGET_ROOT, &g_sUp, 0,
                  &g_sFormike240x320x16_ILI9320, 15, 72, 50, 50,
                  PB_STYLE_TEXT | PB_STYLE_IMG | PB_STYLE_AUTO_REPEAT,
                  ClrBlack, ClrBlack, ClrSilver, ClrSilver, g_pFontCmss24b,
                  "-", g_pucBlue50x50, g_pucBlue50x50Press, 125, 25, OnDown);

//*****************************************************************************
//
// A canvas widget that displays the target motor speed.
//
//*****************************************************************************
Canvas(g_sTarget, WIDGET_ROOT, &g_sDown, 0, &g_sFormike240x320x16_ILI9320, 70,
       85, 100, 25, CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack,
       ClrBlack, ClrSilver, g_pFontCmss24, g_pcTargetBuffer, 0, 0);

//*****************************************************************************
//
// A container widget that surrounds the target motor speed and the two push
// buttons that adjust the target motor speed.
//
//*****************************************************************************
Container(g_sTargetBox, WIDGET_ROOT, &g_sTarget, 0,
          &g_sFormike240x320x16_ILI9320, 5, 58, 230, 72,
          CTR_STYLE_OUTLINE | CTR_STYLE_TEXT | CTR_STYLE_TEXT_CENTER, ClrBlack,
          ClrSilver, ClrSilver, g_pFontCmss18, "Target Speed");

//*****************************************************************************
//
// A canvas widget with the application logo on it.
//
//*****************************************************************************
Canvas(g_sBanner, WIDGET_ROOT, &g_sTargetBox, 0, &g_sFormike240x320x16_ILI9320,
       0, 0, 240, 50, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnPaint);

//*****************************************************************************
//
// A canvas widget used to display the IP and MAC addresses of the module.
//
//*****************************************************************************
Canvas(g_sIPAddress, WIDGET_ROOT, &g_sBanner, 0,
       &g_sFormike240x320x16_ILI9320, 5, 310, 230, 10,
       CANVAS_STYLE_TEXT_OPAQUE | CANVAS_STYLE_TEXT,
       ClrBlack, ClrBlack, ClrSilver, g_pFontFixed6x8, g_pcEthernetAddr, 0,
       0);

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// This function is called when the "Run" push button is pressed.
//
//*****************************************************************************
void
OnRun(tWidget *pWidget)
{
    //
    // Return without doing anything if there is not a connection to the BLDC
    // RDK board.
    //
    if(g_ulMotorState != MOTOR_STATE_CONNECTED)
    {
        return;
    }

    //
    // Tell the motor to start running.
    //
    MotorRun();

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);
}

//*****************************************************************************
//
// This function is called when the "Stop" push button is pressed.
//
//*****************************************************************************
void
OnStop(tWidget *pWidget)
{
    //
    // Return without doing anything if there is not a connection to the BLDC
    // RDK board.
    //
    if(g_ulMotorState != MOTOR_STATE_CONNECTED)
    {
        return;
    }

    //
    // Tell the motor to stop running.
    //
    MotorStop();

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);
}

//*****************************************************************************
//
// This function is called when the "+" push button is pressed.
//
//*****************************************************************************
void
OnUp(tWidget *pWidget)
{
    //
    // Return without doing anything if there is not a connection to the BLDC
    // RDK board, or if the target speed is already at is maximum value.
    //
    if((g_ulMotorState != MOTOR_STATE_CONNECTED) || (g_ulTargetSpeed == 10000))
    {
        return;
    }

    //
    // Increase the target speed by 100.
    //
    g_ulTargetSpeed += 100;

    //
    // Tell the motor the new target speed.
    //
    MotorSpeedSet(g_ulTargetSpeed);

    //
    // Convert the target speed to an ASCII string for display on the screen.
    //
    usprintf(g_pcTargetBuffer, "  %d rpm  ", g_ulTargetSpeed);

    //
    // Request a re-paint of the canvas widget that displays the target speed.
    //
    WidgetPaint((tWidget *)&g_sTarget);
    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);
}

//*****************************************************************************
//
// This function is called when the "-" push button is pressed.
//
//*****************************************************************************
void
OnDown(tWidget *pWidget)
{
    //
    // Return without doing anything if there is not a connection to the BLDC
    // RDK board, or if the target speed is already at is minimum value.
    //
    if((g_ulMotorState != MOTOR_STATE_CONNECTED) || (g_ulTargetSpeed == 0))
    {
        return;
    }

    //
    // Decrease the target speed by 100.
    //
    g_ulTargetSpeed -= 100;

    //
    // Tell the motor the new target speed.
    //
    MotorSpeedSet(g_ulTargetSpeed);

    //
    // Convert the target speed to an ASCII string for display on the screen.
    //
    usprintf(g_pcTargetBuffer, "  %d rpm  ", g_ulTargetSpeed);

    //
    // Request a re-paint of the canvas widget that displays the target speed.
    //
    WidgetPaint((tWidget *)&g_sTarget);

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);
}

//*****************************************************************************
//
// This function is called when the application logo widget is painted.
//
//*****************************************************************************
void
OnPaint(tWidget *pWidget, tContext *pContext)
{
    //
    // Display the application title at the top of the screen.
    //
    GrContextFontSet(pContext, g_pFontCmss30);
    GrContextForegroundSet(pContext, ClrSilver);
    GrStringDrawCentered(pContext, "BLDC Motor RDK", -1, 120, 15, 0);

    //
    // Draw a seperating line.
    //
    GrLineDraw(pContext, 0, 31, 239, 31);

    //
    // Draw the TI stylized name below the separating line.
    //
    GrImageDraw(pContext, g_pucTIName, 0, 36);
}

//*****************************************************************************
//
// This function is called when the SysTick interrupt occurs.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Call the lwIP timer function, indicating that another millisecond has
    // passed.
    //
    lwIPTimer(1);

    //
    // Increment the SysTick interrupt count.
    //
    g_ulCount++;

    //
    // Increment the display timer for IP and MAC address switching.
    //
    g_ulIPDisplayCount++;
}

//*****************************************************************************
//
// This function is called by the software update module whenever a remote
// host requests to update the firmware on this board.  We set a flag that
// will cause the bootloader to be entered the next time the user enters a
// command on the console.
//
//*****************************************************************************
void SoftwareUpdateRequestCallback(void)
{
    g_bFirmwareUpdate = true;
}

//*****************************************************************************
//
// This program provides a simple GUI providing basic control of a BLDC RDK
// board.
//
//*****************************************************************************
int
main(void)
{
    tBoolean bIPDisplayed;
    unsigned long ulIPAddr;

    //
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    // a workaround to allow the PLL to operate reliably.
    //
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }

    //
    // Set the clocking to run from the PLL.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Initialize the display driver.
    //
    Formike240x320x16_ILI9320Init();

    //
    // Turn on the backlight.
    //
    Formike240x320x16_ILI9320BacklightOn();

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sIPAddress);

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Initialize the interface to the BLDC RDK board.
    //
    MotorInit();

    //
    // Initialize the sound driver.
    //
    SoundInit();

    //
    // Initialize the touch screen driver and have it route its messages to the
    // widget tree.
    //
    TouchScreenInit();
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Enable SysTick to provide a periodic interrupt.  This is used as a time
    // base for timed events.
    //
    SysTickPeriodSet(SysCtlClockGet() / 1000);
    SysTickEnable();
    SysTickIntEnable();

    //
    // Initialize remotely triggered firmware updates.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Remember that we start displaying the IP address.
    //
    bIPDisplayed = true;

    //
    // Loop forever (or, at least, until someone requests a firmware update).
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();

        //
        // See if 100 ms has passed.
        //
        if(g_ulCount > 100)
        {
            //
            // See if the motor board connection is presently disconnected.
            //
            if(g_ulMotorState == MOTOR_STATE_DISCON)
            {
                //
                // Initiate a connection to the BLDC RDK board.
                //
                MotorConnect();

                //
                // Set the previous value to 0, with the MSB set to indicate
                // that the value is for controlling the animated dots.
                //
                g_ulPrevious = 0x80000000;
            }

            //
            // See if the motor board connection is attempting to connect.
            //
            else if(g_ulMotorState == MOTOR_STATE_CONNECTING)
            {
                //
                // Set the current speed buffer with a number of dots that
                // correspond to the current state of the dot animation.
                //
                if(g_ulPrevious == 0x80000000)
                {
                    usprintf(g_pcSpeedBuffer, "       .       ");
                }
                else if((g_ulPrevious == 0x80000001) ||
                        (g_ulPrevious == 0x80000007))
                {
                    usprintf(g_pcSpeedBuffer, "      ...      ");
                }
                else if((g_ulPrevious == 0x80000002) ||
                        (g_ulPrevious == 0x80000006))
                {
                    usprintf(g_pcSpeedBuffer, "     .....     ");
                }
                else if((g_ulPrevious == 0x80000003) ||
                        (g_ulPrevious == 0x80000005))
                {
                    usprintf(g_pcSpeedBuffer, "    .......    ");
                }
                else if(g_ulPrevious == 0x80000004)
                {
                    usprintf(g_pcSpeedBuffer, "   .........   ");
                }

                //
                // Request a re-paint of the canvas widget that displays the
                // target speed.
                //
                WidgetPaint((tWidget *)&g_sSpeed);

                //
                // Increment the state of the dot animation.
                //
                g_ulPrevious = (g_ulPrevious + 1) & 0x80000007;
            }

            //
            // See if the motor board connection is established.
            //
            else if(g_ulMotorState == MOTOR_STATE_CONNECTED)
            {
                //
                // See if the current speed matches the previous speed.
                //
                if(g_ulMotorSpeed != g_ulPrevious)
                {
                    //
                    // Save the current speed as the new previous speed.
                    //
                    g_ulPrevious = g_ulMotorSpeed;

                    //
                    // Convert the current speed to an ASCII string for display
                    // on the screen.
                    //
                    usprintf(g_pcSpeedBuffer, "  %d rpm  ", g_ulPrevious);

                    //
                    // Request a re-paint of the canvas widget that displays
                    // the current speed.
                    //
                    WidgetPaint((tWidget *)&g_sSpeed);
                }
            }

            //
            // Reset the SysTick count to zero.
            //
            g_ulCount = 0;
        }

        //
        // See if we need to toggle between IP address and MAC address
        // display.
        //
        if(g_ulIPDisplayCount >= IP_ADDR_DISPLAY_TIME_MS)
        {
            //
            // Toggle the display indidcator.
            //
            bIPDisplayed = !bIPDisplayed;


            //
            // Format the appropriate string.
            //
            if(bIPDisplayed)
            {
                //
                // We are to display the IP address.  Get current address from
                // the TCP/IP stack.
                //
                ulIPAddr = lwIPLocalIPAddrGet();

                //
                // Format the string that will display it.
                //
                if(ulIPAddr)
                {
                    //
                    // We have an IP address so format it into the string.
                    //
                    usprintf(g_pcEthernetAddr, "     IP: %d.%d.%d.%d     ",
                             ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                             (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
                }
                else
                {
                    //
                    // We have not been assigned an IP address yet.
                    //
                    usprintf(g_pcEthernetAddr, "     IP: Not Assigned     ");
                }
            }
            else
            {
                //
                // We are to display the MAC address.  Format this into the
                // target string buffer.
                //
                usprintf(g_pcEthernetAddr,
                         "MAC: %02x-%02x-%02x-%02x-%02x-%02x",
                         g_pucMACAddr[0], g_pucMACAddr[1], g_pucMACAddr[2],
                         g_pucMACAddr[3], g_pucMACAddr[4], g_pucMACAddr[5]);
            }

            //
            // Reset our counter.
            //
            g_ulIPDisplayCount = 0;

            //
            // Repaint the widget to show the new string.
            //
            WidgetPaint((tWidget *)&g_sIPAddress);
        }
    }

    //
    // If we drop through to here, the application has received a remote
    // firmware update request.  First, we tell the motor to stop if it is
    // currently connected.
    //
    if(g_ulMotorState == MOTOR_STATE_CONNECTED)
    {
        MotorStop();
    }

    //
    // Tell the user what's going on.
    //
    usprintf(g_pcSpeedBuffer, "  Updating...  ");
    WidgetPaint((tWidget *)&g_sSpeed);

    //
    // Process the paint message we just posted.
    //
    WidgetMessageQueueProcess();

    //
    // Now pass control to the bootloader to handle the firmware update.  This
    // function call does not return.
    //
    SoftwareUpdateBegin();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
