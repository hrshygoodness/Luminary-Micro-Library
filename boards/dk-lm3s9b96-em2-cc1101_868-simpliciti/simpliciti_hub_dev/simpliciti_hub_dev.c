//*****************************************************************************
//
// simpliciti_hub_dev.c - End Device application for the "Access Point as Data
//                        Hub" SimpliciTI LPRF example.
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
// This is part of revision 9107 of the DK-LM3S9B96-EM2-CC1101_868-SIMPLICITI Firmware Package.
//
//*****************************************************************************

#include <stddef.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
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
//! <h1>End Device for ``Access Point as Data Hub'' example
//! (simpliciti_hub_dev)</h1>
//!
//! This application offers the end device functionality of the generic
//! SimpliciTI ``Access Point as Data Hub'' example.  Pressing buttons on the
//! display will toggle the corresponding LEDs on the access point board to
//! which this end device is linked.
//!
//! The application can communicate with another SimpliciTI-enabled device
//! equipped with a compatible radio and running its own version of the
//! access point from the ''Access Point as Data Hub'' example or with other
//! development boards running the simpliciti_hub_ap example.
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
//! Start the board running the access point example first then start the
//! end devices.  The LEDs on the end device will flash once to indicate that
//! they have joined the network.  After this point, pressing one of the
//! buttons on the display will send a message to the access point causing it
//! to toggle either LED1 or LED2 depending upon which button was pressed.
//!
//! For additional information on running this example and an explanation of
//! the communication between the two devices and access point, see section 3.4
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
// A couple of macros used to introduce delays during monitoring.
//
//*****************************************************************************
#define SPIN_ABOUT_A_QUARTER_SECOND ApplicationDelay(250)
#define SPIN_ABOUT_A_SECOND         ApplicationDelay(1000)

//*****************************************************************************
//
// The number of times we try a transmit and miss an acknowledge before doing
// a channel scan.
//
//*****************************************************************************
#define MISSES_IN_A_ROW  2

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tPushButtonWidget g_sButton1;
extern tPushButtonWidget g_sButton2;
extern tPushButtonWidget g_sLED1;
extern tPushButtonWidget g_sLED2;
extern tCanvasWidget g_sMainStatus;
extern tCanvasWidget g_sLinkStatus;

//*****************************************************************************
//
// Forward reference to the button press handlers.
//
//*****************************************************************************
void OnButton1Press(tWidget *pWidget);
void OnButton2Press(tWidget *pWidget);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sMainStatus, &g_sBackground,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "SimpliciTI-hub-dev",
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
Canvas(g_sBackground, &g_sHeading, 0, &g_sButton1,
       &g_sKitronix320x240x16_SSD2119, 0, 23, 320, (240 - 69),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The button used to toggle AP LED 1.
//
//*****************************************************************************
RectangularButton(g_sButton1, &g_sBackground, &g_sButton2, 0,
                  &g_sKitronix320x240x16_SSD2119, 174, 56, 140, 60,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrBlue, ClrLightBlue, ClrWhite, ClrWhite,
                   g_pFontCmss22b, "Button 1", 0, 0, 0, 0,
                   OnButton1Press);

//*****************************************************************************
//
// The button used to toggle AP LED 1.
//
//*****************************************************************************
RectangularButton(g_sButton2, &g_sBackground, &g_sLED1, 0,
                  &g_sKitronix320x240x16_SSD2119, 174, 124, 140, 60,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrBlue, ClrLightBlue, ClrWhite, ClrWhite,
                   g_pFontCmss22b, "Button 2", 0, 0, 0, 0,
                   OnButton2Press);

//*****************************************************************************
//
// The "LED"s used to indicate application status.  These are deliberately not
// linked to the widget tree yet.  We add these once the user choses which
// mode to run in.
//
//*****************************************************************************
CircularButton(g_sLED1, &g_sBackground, &g_sLED2, 0,
               &g_sKitronix320x240x16_SSD2119, 40, 120, 34,
               (PB_STYLE_OUTLINE | PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE |
               PB_STYLE_TEXT), ClrGreen, ClrGreen, ClrWhite, ClrWhite,
               g_pFontCmss22b, "LED1", 0, 0, 0, 0, 0);

CircularButton(g_sLED2, &g_sBackground, 0, 0,
               &g_sKitronix320x240x16_SSD2119, 124, 120, 34,
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
// The states of the 2 "LEDs" on the display.
//
//*****************************************************************************
static tBoolean g_bLEDStates[2];

//*****************************************************************************
//
// Flag used to indicate which button has been pressed.
//
//*****************************************************************************
static unsigned long g_ulButtonPressed;

static uint8_t  g_ucTid = 0;
static linkID_t g_sLinkID = 0;

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

    //
    // Process the messages in the widget message queue.
    //
    WidgetMessageQueueProcess();
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
// Handler for the "Button 1" button.
//
//*****************************************************************************
void
OnButton1Press(tWidget *pWidget)
{
    //
    // Set a flag that will be checked in the LinkTo main loop to indicate that
    // this button has been pressed.
    //
    g_ulButtonPressed = 1;
}

//*****************************************************************************
//
// Handler for the "Button 2" button.
//
//*****************************************************************************
void
OnButton2Press(tWidget *pWidget)
{
    //
    // Set a flag that will be checked in the LinkTo main loop to indicate that
    // this button has been pressed.
    //
    g_ulButtonPressed = 2;
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
// Link to the access point and continue processing local button requests
// forever.  This function is called following initialization in main() and
// does not return.
//
//*****************************************************************************
static void
LinkTo(void)
{
    uint8_t     pucMsg[2];
    uint8_t     ucMisses, ucDone;
    uint8_t     ucNoAck;
    smplStatus_t eRetcode;
    unsigned long ulButton;

    UpdateStatus(false, "Linking to Access Point");

    //
    // Keep trying to link.  Flash our "LEDs" while these attempts continue.
    //
    while (SMPL_SUCCESS != SMPL_Link(&g_sLinkID))
    {
        ToggleLED(1);
        ToggleLED(2);
        SPIN_ABOUT_A_SECOND;
    }

    //
    // Turn off both LEDs now that we are linked.
    //
    SetLED(1, false);
    SetLED(2, false);

    //
    // Tell the user all is well.
    //
    UpdateStatus(false, "Link successful");

    //
    // Put the radio to sleep until a button is pressed.
    //
    SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SLEEP, 0);

    //
    // Start of the main button processing loop.
    //
    while (1)
    {

        //
        // Grab the button press flag and clear it.  We do this since the flag
        // could be changed during the loop whenever function
        // WidgetMessageQueueProcess() is called and we don't want to miss any
        // button presses.
        //
        ulButton = g_ulButtonPressed;

        //
        // Has either button been pressed?
        //
        if (ulButton)
        {
            //
            // Clear the main button press flag.
            //
            g_ulButtonPressed = 0;

            //
            // Wake the radio up since we are about to need it.
            //
            SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_AWAKE, 0);

            /* Set TID and designate which LED to toggle */
            pucMsg[1] = ++g_ucTid;
            pucMsg[0] = (ulButton == 1) ? 1 : 2;
            ucDone = 0;
            while (!ucDone)
            {

                //
                // Remember that we have yet to receive an ack from the AP.
                //
                ucNoAck = 0;

                //
                // Try sending the message MISSES_IN_A_ROW times looking for an
                // ack after each transmission.
                //
                for (ucMisses = 0; ucMisses < MISSES_IN_A_ROW; ucMisses++)
                {
                    //
                    // Send the message and request acknowledgement.
                    //
                    eRetcode=SMPL_SendOpt(g_sLinkID, pucMsg, sizeof(pucMsg),
                                          SMPL_TXOPTION_ACKREQ);

                    //
                    // Did we get the ack?
                    //
                    if (eRetcode == SMPL_SUCCESS)
                    {
                        //
                        // Yes - Message acked.  We're done.  Toggle LED 1 to
                        // indicate ack received. */
                        ToggleLED(1);
                        break;
                    }

                    //
                    // Did we send the message but fail to get an ack back?
                    //
                    if (SMPL_NO_ACK == eRetcode)
                    {
                        //
                        // Count ack failures.  Could also fail because of CCA
                        // and we don't want to scan in this case.
                        //
                        ucNoAck++;
                    }
                }

                //
                // Did we fail to get an ack after every transmission?
                //
                if (MISSES_IN_A_ROW == ucNoAck)
                {
                    //
                    // Tell the user what happened.
                    //
                    UpdateStatus(false, "Channel changed?");

                    //
                    // Message not acked.  Toggle LED 2.
                    //
                    ToggleLED(2);

#ifdef FREQUENCY_AGILITY
                    //
                    // At this point, we assume we're on the wrong channel so
                    // look for the correct channel by using Ping to initiate a
                    // scan when it gets no reply.  With a successful ping try
                    // sending the message again.  Otherwise, for any error we
                    // get we will wait until the next button press to try
                    // again.
                    //
                    if (SMPL_SUCCESS != SMPL_Ping(g_sLinkID))
                    {
                        ucDone = 1;
                    }
#else
                    ucDone = 1;
#endif  /* FREQUENCY_AGILITY */
                }
                else
                {
                    //
                    // We got the ack so drop out of the transmit loop.
                    //
                    ucDone = 1;

                    UpdateStatus(false, "Toggled AP LED %d", ulButton);
                }
            }

            //
            // Now that we are finished with it, put the radio back to sleep.
            //
            SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SLEEP, 0);
        }

        //
        // Process the widget message queue.
        //
        WidgetMessageQueueProcess();
    }
}


//*****************************************************************************
//
// Main application entry function.
//
//*****************************************************************************
int
main(void)
{
    tBoolean bRetcode;

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
    UpdateStatus(true, "Joining network...");

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

    //
    // Did we have a problem with the address?
    //
    if(!bRetcode)
    {
        //
        // Yes - make sure the display is updated then hang the app.
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
    // Turn both "LEDs" off.
    //
    SetLED(1, false);
    SetLED(2, false);

    //
    // Keep trying to join (a side effect of successful initialization) until
    // successful.  Toggle LEDS to indicate that joining has not occurred.
    //
    while(SMPL_SUCCESS != SMPL_Init(0))
    {
      ToggleLED(1);
      ToggleLED(2);
      SPIN_ABOUT_A_SECOND;
    }

    //
    // We have joined the network so turn on both "LEDs" to indicate this.
    //
    SetLED(1, true);
    SetLED(2, true);
    UpdateStatus(true, "Joined network");

    //
    // Link to the access point which is now listening for us and continue
    // processing.  This function does not return.
    //
    LinkTo();
}
