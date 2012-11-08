//*****************************************************************************
//
// simpliciti_polling_dev.c - End Device application for the "Polling with
//                            Access Point" SimpliciTI LPRF example.
//
// Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the DK-LM3S9D96-EM2-CC1101_868-SIMPLICITI Firmware Package.
//
//*****************************************************************************

#include <stddef.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/flash.h"
#include "drivers/set_pinout.h"
#include "utils/ustdlib.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/container.h"
#include "grlib/pushbutton.h"
#include "drivers/touch.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"

//
// SimpliciTI Headers
//
#include "simplicitilib.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>End Device for ``Polling with Access Point'' example
//! (simpliciti_polling_dev)</h1>
//!
//! This application offers the end device functionality of the generic
//! SimpliciTI Polling_with_AP example.  The application can communicate with
//! other SimpliciTI-enabled devices with compatible radios running the
//! ``Polling_with_AP'' Sender or Receiver configuration.  To run this example,
//! a third SimpliciTI-enabled board must also be present running the access
//! point binary, simpliciti_polling_ap for Stellaris development board, or the
//! relevant Access Point configuration of the Polling_with_AP example if using
//! another hardware platform.
//!
//! The functionality provided here is equivalent to the ``Sender'' and
//! ''Receiver'' configurations included in the generic SimpliciTI ``Polling
//! with AP'' example application.
//!
//! To run this binary correctly, the development board must be equipped with
//! an EM2 expansion board with a CC1101:868/915 EM module installed in the
//! ``MOD1'' position (the connectors nearest the oscillator on the EM2).
//! Hardware platforms supporting SimpliciTI 1.1.1 with which this application
//! may communicate are the following:
//!
//! <ul>
//! <li>SmartRF04EB + CC1110EM</li>
//! <li>EM430F6137RF900</li>
//! <li>FET430F6137RF900</li>
//! <li>CC1111EM USB Dongle</li>
//! <li>EXP430FG4618 + CC1101:868/915 + USB Debug Interface</li>
//! <li>EXP430FG4618 + CC1100:868/915 + USB Debug Interface</li>
//! <li>Stellaris Development Board + EM2 expansion board + CC1101:868/915</li>
//! </ul>
//!
//! Power up the access point board and both its LEDs should light indicating
//! that it is active.  Next, power up the receiver board and select the
//! correct mode by pressing the ``Receiver'' button on the development board
//! display, pressing button 2 on dual button boards, or pressing the single
//! button for less than 3 seconds on boards with only one button.  At this
//! point, only LED1 on the receiver board should be lit.  Finally power up the
//! sender select the mode using the onscreen button, by pressing button 2 on
//! dual button boards or by pressing an holding the single button for more
//! than 3 seconds.  Both LEDs on the sender will blink until it successfully
//! links with the receiver.  After successful linking, the sender will
//! transmit a message to the receiver every 3 to 6 seconds.  This message will
//! be stored by the access point and passed to the receiver the next time it
//! polls the access point.  While the example is running, LEDs on both the
//! sender and receiver will blink.  No user interaction is required on the
//! access point.
//!
//! For additional information on running this example and an explanation of
//! the communication between the two devices and access point, see section 3.2
//! of the ``SimpliciTI Sample Application User's Guide'' which can be found
//! under C:/StellarisWare/SimpliciTI-1.1.1/Documents assuming that
//! StellarisWare is installed in its default directory.
//
//*****************************************************************************

//*****************************************************************************
//
// This application sets the SysTick to fire every 100mS.
//
//*****************************************************************************
#define TICKS_PER_SECOND 10

//*****************************************************************************
//
// The number of seconds we wait while trying to establish a link.
//
//*****************************************************************************
#define LINK_TIMEOUT_SECONDS 10

//*****************************************************************************
//
// A couple of macros used to introduce delays during monitoring.
//
//*****************************************************************************
#define SPIN_ABOUT_A_QUARTER_SECOND ApplicationDelay(250)
#define SPIN_ABOUT_A_SECOND         ApplicationDelay(1000)

//*****************************************************************************
//
// SimpliciTI packet transaction ID.
//
//*****************************************************************************
static uint8_t g_ucTid = 0;

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tPushButtonWidget g_sLinkBtn;
extern tPushButtonWidget g_sListenBtn;
extern tPushButtonWidget g_sLED1;
extern tPushButtonWidget g_sLED2;
extern tCanvasWidget g_sMainStatus;
extern tCanvasWidget g_sLinkStatus;
extern tContainerWidget g_sBtnContainer;
extern tContainerWidget g_sLEDContainer;

//*****************************************************************************
//
// Forward reference to the button press handlers.
//
//*****************************************************************************
void OnLinkButtonPress(tWidget *pWidget);
void OnListenButtonPress(tWidget *pWidget);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sMainStatus, &g_sBackground,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "SimpliciTI-polling-dev",
       0, 0);

//*****************************************************************************
//
// Canvas used to display the latest status.
//
//*****************************************************************************
#define MAX_STATUS_STRING_LEN 40
char g_pcStatus[2][MAX_STATUS_STRING_LEN];
Canvas(g_sMainStatus, WIDGET_ROOT, &g_sLinkStatus, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 217, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, g_pcStatus[0],
       0, 0);

//*****************************************************************************
//
// A canvas showing the link status.
//
//*****************************************************************************
Canvas(g_sLinkStatus, WIDGET_ROOT, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 194, 320, 23, (CANVAS_STYLE_FILL |
       CANVAS_STYLE_TEXT), ClrBlack, ClrWhite, ClrWhite, g_pFontCm20,
       g_pcStatus[1], 0, 0);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sBackground, &g_sHeading, 0, &g_sBtnContainer,
       &g_sKitronix320x240x16_SSD2119, 0, 23, 320, (240 - 69),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// A container used to hold the buttons shown during mode selection.  This
// makes it a bit easier to remove the buttons (and anything else we want to
// show only during mode selection).
//
//*****************************************************************************
Container(g_sBtnContainer, &g_sBackground, 0, &g_sLinkBtn,
          &g_sKitronix320x240x16_SSD2119, 0, 23, 320, (240 - 69),
          0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The button used to start "LinkTo" (talker) example.
//
//*****************************************************************************
RectangularButton(g_sLinkBtn, &g_sBtnContainer, &g_sListenBtn, 0,
                  &g_sKitronix320x240x16_SSD2119, 20, 80, 130, 80,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrDarkRed, ClrRed, ClrWhite, ClrWhite,
                   g_pFontCmss22b, "Sender", 0, 0, 0, 0, OnLinkButtonPress);

//*****************************************************************************
//
// The button used to start "LinkTo" (talker) example.
//
//*****************************************************************************
RectangularButton(g_sListenBtn, &g_sBtnContainer, 0, 0,
                  &g_sKitronix320x240x16_SSD2119, 170, 80, 130, 80,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrDarkRed, ClrRed, ClrWhite, ClrWhite,
                   g_pFontCmss22b, "Receiver", 0, 0, 0, 0,
                   OnListenButtonPress);

//*****************************************************************************
//
// The container for the "LED"s used to indicate application status.  This is
// deliberately not linked to the widget tree yet.  We add this subtree
// once the user choses which mode to run in.
//
//*****************************************************************************
Container(g_sLEDContainer, &g_sBackground, 0, &g_sLED1,
          &g_sKitronix320x240x16_SSD2119, 0, 23, 320, (240 - 69),
          0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The "LED"s used to indicate application status.  These are deliberately not
// linked to the widget tree yet.  We add these once the user choses which
// mode to run in.
//
//*****************************************************************************
CircularButton(g_sLED1, &g_sLEDContainer, &g_sLED2, 0,
               &g_sKitronix320x240x16_SSD2119, 90, 120, 40,
               (PB_STYLE_OUTLINE | PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE |
               PB_STYLE_TEXT), ClrGreen, ClrGreen, ClrWhite, ClrWhite,
               g_pFontCmss22b, "LED1", 0, 0, 0, 0, 0);

CircularButton(g_sLED2, &g_sLEDContainer, 0, 0,
               &g_sKitronix320x240x16_SSD2119, 230, 120, 40,
               (PB_STYLE_OUTLINE | PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE |
               PB_STYLE_TEXT), ClrRed, ClrRed, ClrWhite, ClrWhite,
               g_pFontCmss22b, "LED2", 0, 0, 0, 0, 0);

//*****************************************************************************
//
// A global system tick counter.
//
//*****************************************************************************
static volatile unsigned long g_ulSysTickCount = 0;

//*****************************************************************************
//
// Variable storing the current operating mode (talker or listener).
//
//*****************************************************************************
#define MODE_UNDEFINED 0
#define MODE_SENDER    1
#define MODE_RECEIVER  2
static volatile unsigned long g_ulMode = MODE_UNDEFINED;

//*****************************************************************************
//
// The states of the 2 "LEDs" on the display.
//
//*****************************************************************************
static tBoolean g_bLEDStates[2];

//*****************************************************************************
//
// The colors of each LED in the OFF and ON states.
//
//*****************************************************************************
#define DARK_GREEN      0x00002000
#define DARK_RED        0x00200000
#define BRIGHT_GREEN    0x0000FF00
#define BRIGHT_RED      0x00FF0000

static unsigned long g_ulLEDColors[2][2] =
{
  {DARK_GREEN, BRIGHT_GREEN},
  {DARK_RED, BRIGHT_RED}
};

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
// This is the handler for this SysTick interrupt.  All it does is increment
// a tick counter.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Update our tick counter.
    //
    g_ulSysTickCount++;
}

//*****************************************************************************
//
// A simple delay function which will wait for a particular number of
// milliseconds before returning.  During this time, the application message
// queue is serviced.  The delay granularity here is the system tick period.
//
//*****************************************************************************
void
ApplicationDelay(unsigned long ulDelaymS)
{
    unsigned long ulTarget;

    //
    // What will the system tick counter be when we are finished this delay?
    //
    ulTarget = g_ulSysTickCount + ((ulDelaymS * TICKS_PER_SECOND) / 1000);

    //
    // Hang around waiting until this time.  This doesn't take into account the
    // system tick counter wrapping but, since this takes about 13 and a half
    // years, it's probably not too much of a problem.
    //
    while(g_ulSysTickCount < ulTarget)
    {
        //
        // Process the message queue in case there are any new messages to
        // handle.
        //
        WidgetMessageQueueProcess();
    }
}

//*****************************************************************************
//
// Draw one of the LED widgets in a particular state.
//
//*****************************************************************************
void
UpdateLEDWidget(unsigned long ulLED, tBoolean bOn)
{
    tPushButtonWidget *pButton;

    //
    // Which widget are we dealing with?
    //
    pButton = (ulLED == 1) ? &g_sLED1 : &g_sLED2;

    //
    // Turn the LED on or off by setting the background fill color
    // appropriately.
    //
    PushButtonFillColorSet(pButton, g_ulLEDColors[ulLED - 1][bOn]);
    PushButtonFillColorPressedSet(pButton, g_ulLEDColors[ulLED - 1][bOn]);

    //
    // Ensure that the LED is repainted.  This will occur on the next call to
    // WidgetMessageQueueProcess().
    //
    WidgetPaint((tWidget *)pButton);
}

//*****************************************************************************
//
// Toggle the state of one of the LEDs on the display.
//
//*****************************************************************************
void
ToggleLED(unsigned long ulLED)
{
    //
    // We only support LEDs 1 and 2)
    //
    ASSERT((ulLED == 1) || (ulLED == 2));

    //
    // Toggle our virtual LED state.
    //
    g_bLEDStates[ulLED - 1] = g_bLEDStates[ulLED - 1] ? false : true;

    //
    // Set the state of the LED on the display.
    //
    UpdateLEDWidget(ulLED, g_bLEDStates[ulLED - 1]);
}

//*****************************************************************************
//
// Set or clear one of the LEDs.
//
//*****************************************************************************
void
SetLED(unsigned long ulLED, tBoolean bState)
{
    //
    // We only support LEDs 1 and 2)
    //
    ASSERT((ulLED == 1) || (ulLED == 2));

    //
    // Toggle our virtual LED state.
    //
    g_bLEDStates[ulLED - 1] = bState;

    //
    // Set the state of the LED on the display.
    //
    UpdateLEDWidget(ulLED, bState);
}

//*****************************************************************************
//
// Map a SimpliciTI API return value into a human-readable string.
//
//*****************************************************************************
char *
MapSMPLStatus(smplStatus_t eVal)
{
    switch(eVal)
    {
        case SMPL_SUCCESS:          return("SUCCESS");
        case SMPL_TIMEOUT:          return("TIMEOUT");
        case SMPL_BAD_PARAM:        return("BAD_PARAM");
        case SMPL_NO_FRAME:         return("NO_FRAME");
        case SMPL_NO_LINK:          return("NO_LINK");
        case SMPL_NO_JOIN:          return("NO_JOIN");
        case SMPL_NO_CHANNEL:       return("NO_CHANNEL");
        case SMPL_NO_PEER_UNLINK:   return("NO_PEER_UNLINK");
        case SMPL_NO_PAYLOAD:       return("NO_PAYLOAD");
        case SMPL_NOMEM:            return("NOMEM");
        case SMPL_NO_AP_ADDRESS:    return("NO_AP_ADDRESS");
        case SMPL_NO_ACK:           return("NO_ACK");
        case SMPL_TX_CCA_FAIL:      return("TX_CCA_FAIL");
        default:                    return ("Unknown");
    }
}

//*****************************************************************************
//
// Update one or other of the the status strings on the display.
//
//*****************************************************************************
void
UpdateStatus(tBoolean bMainStatus, const char *pcString, ...)
{
    va_list vaArgP;

    //
    // Start the varargs processing.
    //
    va_start(vaArgP, pcString);

    //
    // Call vsnprintf to format the text into the status string buffer.
    //
    uvsnprintf(g_pcStatus[bMainStatus ? 0 : 1], MAX_STATUS_STRING_LEN,
               pcString, vaArgP);

    //
    // End the varargs processing.
    //
    va_end(vaArgP);

    //
    // Update the status string on the display.
    //
    WidgetPaint(bMainStatus ? (tWidget *)&g_sMainStatus :
                (tWidget *)&g_sLinkStatus);
}

//*****************************************************************************
//
// Handler for the "LinkTo" button.
//
//*****************************************************************************
void
OnLinkButtonPress(tWidget *pWidget)
{
    //
    // Tell the user we're switching into "LinkTo" mode.
    //
    UpdateStatus(true, "Running as sender (LinkTo)");

    //
    // Set the mode we will be running in.
    //
    g_ulMode = MODE_SENDER;
}

//*****************************************************************************
//
// Handler for the "LinkListen" button.
//
//*****************************************************************************
void
OnListenButtonPress(tWidget *pWidget)
{
    //
    // Tell the user we're switching into "LinkFrom" mode.
    //
    UpdateStatus(true, "Running as receiver (LinkListen)");

    //
    // Set the mode we will be running in.
    //
    g_ulMode = MODE_RECEIVER;
}

//*****************************************************************************
//
// This function listens for a link request from another SimpliciTI device.
//
//*****************************************************************************
tBoolean
LinkFrom(void)
{
    linkID_t linkID1;
    uint8_t  pucMsg[MAX_APP_PAYLOAD], ucLen, ucLtid;
    unsigned long ulCount;
    smplStatus_t eRetcode;

    //
    // Tell the user what we're doing.
    //
    UpdateStatus(false, "Listening for link...");

    //
    // Keep the compiler happy.
    //
    eRetcode = SMPL_TIMEOUT;

    //
    // Turn on LED 1 to indicate that we are listening.
    //
    SetLED(1, true);

    //
    // Listen for link for 10 seconds or so.  This logic may fail if you
    // happen to have sat around for about 13.6 years between starting the
    // example and pressing the mode selection button.  I suspect I will be
    // forgiven for this.
    //
    ulCount = g_ulSysTickCount + (LINK_TIMEOUT_SECONDS * TICKS_PER_SECOND);
    while (ulCount > g_ulSysTickCount)
    {
        //
        // Process our message queue to keep the widget library happy.
        //
        WidgetMessageQueueProcess();

        //
        // Listen for a link.  This call takes quite some time to return.
        //
        eRetcode = SMPL_LinkListen(&linkID1);

        //
        // Was the link successful?
        //
        if (SMPL_SUCCESS == eRetcode)
        {
            //
            // Yes - drop out of the loop.
            //
            break;
        }
    }

    //
    // Did we link successfully?
    //
    if(eRetcode != SMPL_SUCCESS)
    {
        //
        // No - Tell the user what happened and return an error.
        //
        UpdateStatus(false, "Failed to link!");
        return(false);
    }

    //
    // Turn off LED 1 to indicate that our listen succeeded.
    //
    UpdateStatus(false, "Link succeeded.");
    SetLED(1, false);

    //
    // Clear our message counter.
    //
    ulCount = 0;

    //
    // Enter an infinite loop polling for messages.
    //
    while (1)
    {
        //
        // Turn the radio off and pretend to sleep for a second or so.
        //
        SMPL_Ioctl(IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SLEEP, 0);
        SPIN_ABOUT_A_SECOND;  /* emulate MCU sleeping */

        //
        // Turn the radio back on again.
        //
        SMPL_Ioctl(IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_AWAKE, 0);

        //
        // Were any messages "received"?
        //
        // The receive call results in polling the Access Point.  The success
        // case occurs when a payload is actually returned.  When there is no
        // frame waiting for the device a frame with no payload is returned by
        // the Access Point.  Note that this loop will retrieve any and all
        // frames that are waiting for this device on the specified link ID.
        // This call will also return frames that were received directly.  It
        // is possible to get frames that were repeated either from the initial
        // transmission from the peer or via a Range Extender.  This is why we
        // implement the TID check.
        //
        do
        {
            //
            // Receive whatever the AP has for us.
            //
            eRetcode = SMPL_Receive(linkID1, pucMsg, &ucLen);

            //
            // Did we get a real frame?
            //
            if((eRetcode == SMPL_SUCCESS) && ucLen)
            {
                //
                // Tell the user what's going on.
                //
                UpdateStatus(false, "Received msg %d", ++ulCount);

                //
                // Process our message queue to keep the widget library happy.
                //
                WidgetMessageQueueProcess();

                //
                // Check the application sequence number to detect late or missing
                // frames.
                //
                ucLtid = *(pucMsg+1);
                if (ucLtid)
                {
                    //
                    // If the current TID is non-zero and the last one we saw was
                    // less than this one assume we've received the 'next' one.
                    //
                    if (g_ucTid < ucLtid)
                    {
                        //
                        // 'Next' frame.  We may have missed some but we don't
                        // care.
                        //
                        if ((*pucMsg == 1) || (*pucMsg == 2))
                        {
                            //
                            // We're good. Toggle the requested LED.
                            //
                            ToggleLED(*pucMsg);
                        }

                        //
                        // Remember the last TID.
                        //
                        g_ucTid = ucLtid;
                    }

                    //
                    // If current TID is non-zero and less than or equal to the last
                    // one we saw assume we received a duplicate.  Just ignore it.
                    //
                }
                else
                {
                    //
                    // Current TID is zero so the count wrapped or we just started.
                    // Let's just accept it and start over.
                    //
                    if ((*pucMsg == 1) || (*pucMsg == 2))
                    {
                        //
                        // We're good. Toggle the requested LED.
                        //
                        ToggleLED(*pucMsg);
                    }

                    //
                    // Remember the last TID.
                    //
                    g_ucTid = ucLtid;
                }
            }
        } while ((eRetcode == SMPL_SUCCESS) & ucLen);
    }
}

//*****************************************************************************
//
// This function attempts to link to another SimpliciTI device by sending a
// link request.
//
//*****************************************************************************
tBoolean
LinkTo(void)
{
    linkID_t linkID1;
    uint8_t  pucMsg[2], ucWrap;
    unsigned long ulCount;
    smplStatus_t eRetcode;

    //
    // Our message counter hasn't wrapped.
    //
    ucWrap = 0;

    //
    // Turn on LED 2
    //
    SetLED(2, true);

    //
    // Tell the user what we're doing.
    //
    UpdateStatus(false, "Attempting to link...");

    //
    // Try to link for about 10 seconds.
    //
    for(ulCount = 0; ulCount < LINK_TIMEOUT_SECONDS; ulCount++)
    {
        //
        // Try to link.
        //
        eRetcode = SMPL_Link(&linkID1);

        //
        // Did we succeed?
        //
        if(eRetcode == SMPL_SUCCESS)
        {
            //
            // Yes - drop out of the loop.
            //
            break;
        }

        //
        // We didn't link so toggle the LEDs, wait a bit then try again.
        //
        ToggleLED(1);
        ToggleLED(2);
        SPIN_ABOUT_A_SECOND;
    }

    //
    // Did the link succeed?
    //
    if(eRetcode != SMPL_SUCCESS)
    {
        //
        // No - return an error code.
        //
        UpdateStatus(false, "Failed to link!");
        return(false);
    }
    else
    {
        UpdateStatus(false, "Link succeeded.");
    }

    //
    // Turn off the second LED now that we have linked.
    //
    SetLED(2, false);

#ifdef FREQUENCY_AGILITY
    //
    // The radio comes up with Rx off. If Frequency Agility is enabled we need
    // it to be on so we don't miss a channel change broadcast.  We need to
    // hear this since this application has no ack.  The broadcast from the AP
    // is, therefore, the only way to find out about a channel change.
    //
    SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_RXON, 0);
#endif

    //
    // Set up the initial message and message counter.
    //
    pucMsg[0] = 2;
    pucMsg[1] = ++g_ucTid;
    ulCount = 0;

    //
    // We've linked successfully so drop into an infinite loop during which
    // we send messages to the receiver every 5 seconds or so.
    //
    while (1)
    {
        //
        // Send a message every 5 seconds. This could be emulating a sleep.
        //
#ifndef FREQUENCY_AGILITY
        //
        // If Frequency Agility is disabled we don't need to listen for the
        // broadcast channel change command.
        //
        SMPL_Ioctl(IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SLEEP, 0);
#endif

        //
        // Kill about 5 seconds.
        //
        SPIN_ABOUT_A_SECOND;
        SPIN_ABOUT_A_SECOND;
        SPIN_ABOUT_A_SECOND;
        SPIN_ABOUT_A_SECOND;
        SPIN_ABOUT_A_SECOND;

#ifndef FREQUENCY_AGILITY
        //
        // See comment above...If Frequency Agility is not enabled we never
        // listen so it is OK if we just awaken leaving Rx off.
        //
        SMPL_Ioctl(IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_AWAKE, 0);
#endif

        //
        // Send a message.
        //
        eRetcode = SMPL_Send(linkID1, pucMsg, sizeof(pucMsg));
        if (SMPL_SUCCESS == eRetcode)
        {
            //
            // Toggle LED 1 to indicate that we sent something.
            //
            ToggleLED(1);

            //
            // Set up the next message.  Every 8th message toggle LED 1 instead
            // of LED 2 on the receiver.
            //
            pucMsg[0] = (++ucWrap & 0x7) ? 2 : 1;
            pucMsg[1] = ++g_ucTid;
        }

        //
        // Tell the user what we did.
        //
        UpdateStatus(false, "Sent msg %d (%s).", ++ulCount,
                     MapSMPLStatus(eRetcode));
    }
}

//*****************************************************************************
//
// Set the SimpliciTI device address as the least significant 4 digits of the
// device Ethernet MAC address.  This ensures that the address is unique across
// Stellaris devices.  If the MAC address has not been set, we return false to
// indicate failure.
//
//*****************************************************************************
tBoolean
SetSimpliciTIAddress(void)
{
    unsigned long ulUser0, ulUser1;
    addr_t sAddr;

    //
    // Make sure we are using 4 byte addressing.
    //
    ASSERT(NET_ADDR_SIZE == 4);

    //
    // Get the MAC address from the non-volatile user registers.
    //
    FlashUserGet(&ulUser0, &ulUser1);

    //
    // Has the MAC address been programmed?
    //
    if((ulUser0 == 0xffffffff) || (ulUser1 == 0xffffffff))
    {
        //
        // No - we don't have an address so return a failure.
        //
        UpdateStatus(false, "Flash user registers are clear");
        UpdateStatus(true, "Error - address not set!");
        return(false);
    }
    else
    {
        //
        // The MAC address is stored with 3 bytes in each of the 2 flash user
        // registers.  Extract the least significant 4 MAC bytes for use as the
        // SimpliciTI device address.
        //
        sAddr.addr[0] = ((ulUser1 >> 16) & 0xff);
        sAddr.addr[1] = ((ulUser1 >>  8) & 0xff);
        sAddr.addr[2] = ((ulUser1 >>  0) & 0xff);
        sAddr.addr[3] = ((ulUser0 >> 16) & 0xff);

        //
        // SimpliciTI requires that the first byte of the device address is
        // never either 0x00 or 0xFF so we check for these cases and invert the
        // first bit if either is detected.  This does result in the
        // possibility of two devices having the same address but, for example
        // purposes, is likely to be fine.
        //
        if((sAddr.addr[0] == 0x00) || (sAddr.addr[0] == 0xFF))
        {
            sAddr.addr[0] ^= 0x80;
        }

        //
        // Tell the SimpliciTI stack which device address we want to use.
        //
        SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &sAddr);
    }

    //
    // If we get here, all is well.
    //
    return(true);
}

//*****************************************************************************
//
// Main application entry function.
//
//*****************************************************************************
int
main(void)
{
    tBoolean bSuccess, bRetcode, bInitialized;

    //
    // Set the system clock to run at 50MHz from the PLL
    //
    MAP_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // NB: We don't call PinoutSet() in this testcase since the EM header
    // expansion board doesn't currently have an I2C ID EEPROM.  If we did
    // call PinoutSet() this would configure all the EPI pins for SDRAM and
    // we don't want to do this.
    //
    g_eDaughterType = DAUGHTER_NONE;

    //
    // Enable peripherals required to drive the LCD.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);

    //
    // Configure SysTick for a 10Hz interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sHeading);

    //
    // Paint the widget tree to make sure they all appear on the display.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Initialize the SimpliciTI BSP.
    //
    BSP_Init();

    //
    // Set the SimpliciTI device address using the current Ethernet MAC address
    // to ensure something like uniqueness.
    //
    bRetcode = SetSimpliciTIAddress();
    if(!bRetcode)
    {
        //
        // The Ethernet MAC address can't have been set so hang here since we
        // don't have an address to use for SimpliciTI.
        //
        WidgetMessageQueueProcess();
        while(1)
        {
            //
            // MAC address is not set so hang the app.
            //
        }
    }

    //
    // First time through, we need to initialize the SimpliciTI stack.
    //
    bInitialized = false;

    //
    // The main loop starts here now that we have joined the network.
    //
    while(1)
    {
        //
        // Tell the user what to do.
        //
        UpdateStatus(true, "Please choose the operating mode.");

        //
        // Now wait until the user selects whether we should run as the sender
        // or the receiver.
        //
        while(g_ulMode == MODE_UNDEFINED)
        {
            //
            // Just spin, processing UI messages and waiting for someone to
            // press one of the mode buttons.
            //
            WidgetMessageQueueProcess();
        }

        //
        // At this point, the mode is set so remove the buttons from the
        // display and replace them with the LEDs.
        //
        WidgetRemove((tWidget *)&g_sBtnContainer);
        WidgetAdd((tWidget *)&g_sBackground,
                  (tWidget *)&g_sLEDContainer);
        WidgetPaint((tWidget *)&g_sBackground);

        //
        // Tell the user what we're doing now.
        //
        UpdateStatus(false, "Joining network...");

        if(!bInitialized)
        {
            //
            // Initialize the SimpliciTI stack  We keep trying to initialize until
            // we get a success return code.  This indicates that we have also
            // successfully joined the network.
            //
            while(SMPL_SUCCESS != SMPL_Init((uint8_t (*)(linkID_t))0))
            {
                ToggleLED(1);
                ToggleLED(2);
                SPIN_ABOUT_A_SECOND;
            }

            //
            // Now that we are initialized, remember not to call this again.
            //
            bInitialized = true;
        }

        //
        // Once we have joined, turn both LEDs on and tell the user what we want
        // them to do.
        //
        SetLED(1, true);
        SetLED(2, true);

        //
        // Now call the function that initiates communication in
        // the desired mode.  Note that these functions will not return
        // until communication is established or an error occurs.
        //
        if(g_ulMode == MODE_SENDER)
        {
            bSuccess = LinkTo();
        }
        else
        {
            bSuccess = LinkFrom();
        }

        //
        // If we were unsuccessfull, go back to the mode selection
        // display.
        //
        if(!bSuccess)
        {
            //
            // Remove the LEDs and show the buttons again.
            //
            WidgetRemove((tWidget *)&g_sLEDContainer);
            WidgetAdd((tWidget *)&g_sBackground, (tWidget *)&g_sBtnContainer);
            WidgetPaint((tWidget *)&g_sBackground);

            //
            // Tell the user what happened.
            //
            UpdateStatus(false, "Error establishing communication!");

            //
            // Remember that we don't have an operating mode chosen.
            //
            g_ulMode = MODE_UNDEFINED;
        }
    }
}
