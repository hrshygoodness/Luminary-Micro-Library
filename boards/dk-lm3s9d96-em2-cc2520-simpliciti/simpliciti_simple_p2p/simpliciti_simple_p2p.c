//*****************************************************************************
//
// simpliciti_simple_p2p.c - Simple Peer-to-Peer SimpliciTI LPRF example.
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
// This is part of revision 8555 of the DK-LM3S9D96-EM2-CC2520-SIMPLICITI Firmware Package.
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
//! <h1>``Simple Peer-to-Peer'' example (simpliciti_simple_p2p)</h1>
//!
//! This application offers the functionality of the generic SimpliciTI
//! Simple_Peer_to_Peer example.  Whereas the original example builds two
//! independent executables, this version implements both the talker (LinkTo)
//! and listener (LinkListen) functionality, the choice being made via two
//! buttons shown on the display.  The application can communicate with another
//! SimpliciTI-enabled device with a compatible radio running one of the
//! Simple_Peer_to_Peer example binaries or another copy of itself running on a
//! second development board.
//!
//! To run this binary correctly, the development board must be equipped with
//! an EM2 expansion board with a CC2520EM module installed in the ``MOD1''
//! position (the connectors nearest the oscillator on the EM2).  Hardware
//! platforms supporting SimpliciTI 1.1.1 with which this application may
//! communicate are the following:
//!
//! <ul>
//! <li>CC2430DB</li>
//! <li>SmartRF04EB + CC2430EM</li>
//! <li>SmartRF04EB + CC2431EM</li>
//! <li>SmartRF05EB + CC2530EM</li>
//! <li>SmartRF05EB + MSP430F2618 + CC2520
//! <li>Stellaris Development Board + EM2 expansion board + CC2520EM</li>
//! </ul>
//!
//! On starting the application, you are presented with two choices on the
//! LCD display.  If your companion board is running the ``LinkTo''
//! configuration of the application, press the ``LinkListen'' button.  If the
//! companion board is running ``LinkListen'', press the ``LinkTo'' button.
//! Once one of these buttons is pressed, the application attempts to start
//! communication with its peer, either listening for an incoming link
//! request or sending a request.  After a link is established the talker
//! board (running in ``LinkTo'' mode) sends packets to the listener which
//! echos them back after toggling an LED.  In the Stellaris implementation
//! of the application the ``LEDs'' are shown using onscreen widgets.
//!
//! For additional information on this example and an explanation of the
//! communication between talker and listener, see section 3.1 of the
//! ``SimpliciTI Sample Application User's Guide'' which can be found under
//! C:/StellarisWare/SimpliciTI-1.1.1/Documents assuming you installed
//! StellarisWare in its default directory.
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
// When running in talker (LinkTo) mode, this application varies the delay
// between each packet between 1 and 4 seconds. For debug purposes, it is
// sometimes handier to have a fixed delay so define the following label if
// you want a packet sent every 2 seconds all the time instead of using a
// variable delay.
//
//*****************************************************************************
#define USE_2_SECOND_DELAY

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
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "SimpliciTI-simple-p2p",
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
                   g_pFontCmss22b, "LinkTo", 0, 0, 0, 0, OnLinkButtonPress);

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
                   g_pFontCmss22b, "LinkListen", 0, 0, 0, 0,
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
// The system tick count at which we send the next packet when in LinkTo mode.
//
//*****************************************************************************
static unsigned long g_ulNextPacketTick;

//*****************************************************************************
//
// Globals related to the SimpliciTI connection.
//
//*****************************************************************************
static uint8_t  g_ucRxTid=0;
static linkID_t sLinkID = 0;
static volatile unsigned long g_ulRxCount, g_ulTxCount;

//*****************************************************************************
//
// Flags used to send commands to the main loop.
//
//*****************************************************************************
static volatile unsigned long g_ulCommandFlags;

#define COMMAND_MODE_SET    0
#define COMMAND_LED1_TOGGLE 1
#define COMMAND_LED2_TOGGLE 2
#define COMMAND_SEND_REPLY  3

//*****************************************************************************
//
// Variable storing the current operating mode (talker or listener).
//
//*****************************************************************************
#define MODE_UNDEFINED 0
#define MODE_TALKER    1
#define MODE_LISTENER  2
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
    UpdateStatus(true, "Running as talker (LinkTo)");

    //
    // Set the mode we will be running in.
    //
    g_ulMode = MODE_TALKER;

    //
    // Tell the main loop to go ahead and start communication.
    //
    HWREGBITW(&g_ulCommandFlags, COMMAND_MODE_SET) = 1;
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
    UpdateStatus(true, "Running as listener (LinkListen)");

    //
    // Set the mode we will be running in.
    //
    g_ulMode = MODE_LISTENER;

    //
    // Tell the main loop to go ahead and start communication.
    //
    HWREGBITW(&g_ulCommandFlags, COMMAND_MODE_SET) = 1;
}

//*****************************************************************************
//
// This function listens for a link request from another SimpliciTI device.
//
//*****************************************************************************
tBoolean
LinkFrom(void)
{
    smplStatus_t eRetcode;
    unsigned long ulCount;

    //
    // Tell SimpliciTI to try to link to an access point.
    //
    for(ulCount = 1; ulCount <= 10; ulCount++)
    {
        //
        // Update the displayed count.  Note that we must process the widget
        // message queue here to ensure that the change makes it onto the
        // display.
        //
        UpdateStatus(false, "Listening %d (%s)", ulCount,
                     (ulCount > 1) ? MapSMPLStatus(eRetcode) : "Waiting");
        WidgetMessageQueueProcess();

        //
        // Try to link to the access point.
        //
        eRetcode = SMPL_LinkListen(&sLinkID);
        if(eRetcode == SMPL_SUCCESS)
        {
            break;
        }
    }

    //
    // Did we manage to link to the access point?
    //
    if(eRetcode == SMPL_SUCCESS)
    {
        UpdateStatus(false, "Listen successful.");

        //
        // Turn on RX. Default is off.
        //
        SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_RXON, 0);

        //
        // Tell the main loop that we established communication successfully.
        //
        return(true);
    }
    else
    {
        UpdateStatus(false, "No link request received.");

        //
        // Tell the main loop that we failed to establish communication.
        //
        return(false);
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
    smplStatus_t eRetcode;
    unsigned long ulCount;

    //
    // Turn both "LEDs" on.
    //
    SetLED(1, true);
    SetLED(2, true);

    //
    // Tell SimpliciTI to try to link to an access point.
    //
    for(ulCount = 1; ulCount <= 10; ulCount++)
    {
        //
        // Update the displayed count.  Note that we must process the widget
        // message queue here to ensure that the change makes it onto the
        // display.
        //
        UpdateStatus(false, "Link request %d (%s)", ulCount,
                     (ulCount > 1) ? MapSMPLStatus(eRetcode) : "Waiting");
        WidgetMessageQueueProcess();

        //
        // Try to link to the access point.
        //
        eRetcode = SMPL_Link(&sLinkID);
        if(eRetcode == SMPL_SUCCESS)
        {
            break;
        }

        //
        // Wait a bit before trying again.
        //
        NWK_DELAY(1000);

        //
        // Toggle both the LEDs
        //
        ToggleLED(1);
        ToggleLED(2);
    }

    //
    // Did we manage to link to the access point?
    //
    if(eRetcode == SMPL_SUCCESS)
    {
        //
        // Tell the user how we got on.
        //
        UpdateStatus(false, "Link successful.");
        SetLED(2, false);

        //
        // Turn on RX. Default is off.
        //
        SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_RXON, 0);

        //
        // Set the time at which we have to send the next packet to our peer to
        // one second in the future.
        //
        g_ulNextPacketTick = g_ulSysTickCount + TICKS_PER_SECOND;

        //
        // Tell the main loop that we established communication successfully.
        //
        return(true);
    }
    else
    {
        UpdateStatus(false, "Failed to link.");

        //
        // Tell the main loop that we failed to establish communication.
        //
        return(false);
    }
}

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
// Handle frames received from the radio.
//
//*****************************************************************************
static uint8_t
RxCallback(linkID_t port)
{
    uint8_t pcMsg[2], ucLen, ucTid;
    smplStatus_t eRetcode;

    //
    // Is the callback for the link ID we want to handle?
    //
    if (port == sLinkID)
    {
        //
        // Yes - go and get the frame. We know this call will succeed.
        //
        eRetcode = SMPL_Receive(sLinkID, pcMsg, &ucLen);

        if ((eRetcode == SMPL_SUCCESS) && ucLen)
        {
            //
            // Update our receive counter.
            //
            g_ulRxCount++;

            //
            // Check the application sequence number to detect late or missing
            // frames.
            //
            ucTid = *(pcMsg+1);
            if (ucTid)
            {
                if (ucTid > g_ucRxTid)
                {
                    //
                    // Things are fine so toggle the requested LED.  Note that
                    // we send a message to the main loop to do this for us
                    // since redrawing the widget on the screen is a
                    // time-consuming operation that we don't want to do in
                    // interrupt context.
                    //
                    HWREGBITW(&g_ulCommandFlags, ((*pcMsg == 1) ?
                              COMMAND_LED1_TOGGLE : COMMAND_LED2_TOGGLE)) = 1;
                    g_ucRxTid = ucTid;
                }
            }
            else
            {
                //
                // The sequence number has wrapped.
                //
                if (g_ucRxTid)
                {
                    //
                    // Send a message to the main loop asking it to toggle the
                    // "LED" on the display.
                    //
                    HWREGBITW(&g_ulCommandFlags, ((*pcMsg == 1) ?
                              COMMAND_LED1_TOGGLE : COMMAND_LED2_TOGGLE)) = 1;
                    g_ucRxTid = ucTid;
                }
            }

            //
            // If we're operating as the listener, we need to reply with another
            // packet so tell the main loop to do this for us.
            //
            if(g_ulMode == MODE_LISTENER)
            {
                HWREGBITW(&g_ulCommandFlags, COMMAND_SEND_REPLY) = 1;
            }

            //
            // We've finished processing this frame.
            //
            return(1);
        }
    }

    //
    // Tell SimpliciTI to keep frame for later handling.
    //
    return(0);
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
    ROM_FlashUserGet(&ulUser0, &ulUser1);

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
    tBoolean bSuccess, bRetcode;
    unsigned char pucMsg[2];
    unsigned char ucTid;
    unsigned char ucDelay;
    unsigned long ulLastRxCount, ulLastTxCount;
    smplStatus_t eRetcode;

    //
    // Set the system clock to run at 50MHz from the PLL
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
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
    // Initialize the status string.
    //
    UpdateStatus(true, "Please choose the operating mode.");

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
        // The board does not have a MAC address configured so we can't set
        // the SimpliciTI device address (which we derive from the MAC address).
        //
        while(1);
    }

    //
    // Initialize the SimpliciTI stack and supply our receive callback
    // function pointer.
    //
    SMPL_Init(RxCallback);

    //
    // Initialize our message ID, initial inter-message delay and packet
    // counters.
    //
    ucTid = 0;
    ucDelay = 0;
    ulLastRxCount = 0;
    ulLastTxCount = 0;

    //
    // Fall into the command line processing loop.
    //
    while (1)
    {
        //
        // Process any messages from or for the widgets.
        //
        WidgetMessageQueueProcess();

        //
        // Check to see if we've been told to do anything.
        //
        if(g_ulCommandFlags)
        {
            //
            // Has the mode been set?  If so, set up the display to show the
            // "LEDs" and then start communication.
            //
            if(HWREGBITW(&g_ulCommandFlags, COMMAND_MODE_SET))
            {
                //
                // Clear the bit now that we have seen it.
                //
                HWREGBITW(&g_ulCommandFlags, COMMAND_MODE_SET) = 0;

                //
                // Remove the buttons and replace them with the LEDs then
                // repaint the display.
                //
                WidgetRemove((tWidget *)&g_sBtnContainer);
                WidgetAdd((tWidget *)&g_sBackground,
                          (tWidget *)&g_sLEDContainer);
                WidgetPaint((tWidget *)&g_sBackground);

                //
                // Now call the function that initiates communication in
                // the desired mode.  Note that these functions will not return
                // until communication is established or an error occurs.
                //
                if(g_ulMode == MODE_TALKER)
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
                    WidgetAdd((tWidget *)&g_sBackground,
                              (tWidget *)&g_sBtnContainer);
                    WidgetPaint((tWidget *)&g_sBackground);

                    //
                    // Tell the user what happened.
                    //
                    UpdateStatus(false, "Error establishing communication!");
                    UpdateStatus(true, "Please choose the operating mode.");

                    //
                    // Remember that we don't have an operating mode chosen.
                    //
                    g_ulMode = MODE_UNDEFINED;
                }
            }

            //
            // Have we been asked to toggle the first "LED"?
            //
            if(HWREGBITW(&g_ulCommandFlags, COMMAND_LED1_TOGGLE))
            {
                //
                // Clear the bit now that we have seen it.
                //
                HWREGBITW(&g_ulCommandFlags, COMMAND_LED1_TOGGLE) = 0;

                //
                // Toggle the LED.
                //
                ToggleLED(1);
            }

            //
            // Have we been asked to toggle the second "LED"?
            //
            if(HWREGBITW(&g_ulCommandFlags, COMMAND_LED2_TOGGLE))
            {
                //
                // Clear the bit now that we have seen it.
                //
                HWREGBITW(&g_ulCommandFlags, COMMAND_LED2_TOGGLE) = 0;

                //
                // Toggle the LED.
                //
                ToggleLED(2);
            }

            //
            // Have we been asked to send a packet back to our peer?  This
            // command is only ever sent to the main loop when we are running
            // in listener mode (LinkListen).
            //
            if(HWREGBITW(&g_ulCommandFlags, COMMAND_SEND_REPLY))
            {
                //
                // Clear the bit now that we have seen it.
                //
                HWREGBITW(&g_ulCommandFlags, COMMAND_SEND_REPLY) = 0;

                //
                // Create the message.  The first byte tells the receiver to
                // toggle LED1 and the second is a sequence counter.
                //
                pucMsg[0] = 1;
                pucMsg[1] = ++ucTid;
                eRetcode = SMPL_Send(sLinkID, pucMsg, 2);

                //
                // Update our transmit counter if we transmitted the packet
                // successfully.
                //
                if(eRetcode == SMPL_SUCCESS)
                {
                    g_ulTxCount++;
                }
                else
                {
                    UpdateStatus(false, "TX error %s (%d)", MapSMPLStatus(eRetcode),
                                 eRetcode);
                }
            }
        }

        //
        // If we are the talker (LinkTo mode), check to see if it's time to
        // send another packet to our peer.
        //
        if((g_ulMode == MODE_TALKER) &&
           (g_ulSysTickCount >= g_ulNextPacketTick))
        {
            //
            // Create the message.  The first byte tells the receiver to
            // toggle LED1 and the second is a sequence counter.
            //
            pucMsg[0] = 1;
            pucMsg[1] = ++ucTid;
            eRetcode = SMPL_Send(sLinkID, pucMsg, 2);

            //
            // Update our transmit counter if we transmitted the packet
            // correctly.
            //
            if(eRetcode == SMPL_SUCCESS)
            {
                g_ulTxCount++;
            }
            else
            {
                UpdateStatus(false, "TX error %s (%d)", MapSMPLStatus(eRetcode),
                             eRetcode);
            }

            //
            // Set the delay before the next message.
            //
#ifndef USE_2_SECOND_DELAY
            //
            // Set the delay before the next message.  We increase this from 1
            // second to 4 seconds then cycle back to 1.
            //
            ucDelay = (ucDelay == 4) ? 1 : (ucDelay + 1);
#else
            //
            // Wait 2 seconds before sending the next message.
            //
            ucDelay = 2;
#endif

            //
            // Calculate the system tick count when our delay has completed.
            // This algorithm will generate a spurious packet every 13.7 years
            // since I don't handle the rollover case in the comparison above
            // but I'm pretty sure you will forgive me for this oversight.
            //
            g_ulNextPacketTick = g_ulSysTickCount +
                                 (TICKS_PER_SECOND * ucDelay);
        }

        //
        // If either the transmit or receive packet count changed, update
        // the status on the display.
        //
        if((g_ulRxCount != ulLastRxCount) || (g_ulTxCount != ulLastTxCount))
        {
            ulLastTxCount = g_ulTxCount;
            ulLastRxCount = g_ulRxCount;
            UpdateStatus(false, "Received %d pkts, sent %d (%d)",
                         ulLastRxCount, ulLastTxCount);
        }
    }
}
