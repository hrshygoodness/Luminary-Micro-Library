//*****************************************************************************
//
// simpliciti_polling_ap.c -  Access Point application for the "Polling with
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
// This is part of revision 9107 of the DK-LM3S9B96-EM2-CC2500-SIMPLICITI Firmware Package.
//
//*****************************************************************************

#include <stddef.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/debug.h"
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
//! <h1>Access Point for ``Polling with Access Point'' example
//! (simpliciti_polling_ap)</h1>
//!
//! This application offers the access point functionality of the generic
//! SimpliciTI Polling_with_AP example.  To run this example, two additional
//! SimpliciTI-enabled boards using compatible radios must also be present, one
//! running the sender application and the other running the receiver.  If
//! using the Stellaris development board, these functions are found in the
//! simpliciti_polling_dev example application.  On other hardware, these are
//! the Sender and Receiver configurations of the ``Polling_with_AP'' example
//! as supplied with SimpliciTI 1.1.1.
//!
//! The functionality provided here is equivalent to the ``Access Point''
//! configuration included in the generic SimpliciTI ``Polling with AP''
//! example application.
//!
//! To run this binary correctly, the development board must be equipped with
//! an EM2 expansion board with a CC2520EM module installed in the ``MOD1''
//! position (the connectors nearest the oscillator on the EM2).  Hardware
//! platforms supporting SimpliciTI 1.1.1 with which this application may
//! communicate are the following:
//!
//! <ul>
//! <li>eZ430 + RF2500</li>
//! <li>EXP430FG4618 + CC2500 + USB Debug Interface</li>
//! <li>SmartRF04EB + CC2510EM</li>
//! <li>CC2511EM USB Dongle</li>
//! <li>Stellaris Development Board + EM2 expansion board + CC2500EM</li>
//! </ul>
//!
//! To run this example, power up the access point board and both its LEDs
//! should light indicating that it is active.  Next, power up the receiver
//! board and press button 2 (or, on single button boards, press the button for
//! less than 3 seconds).  At this point, only LED1 on the receiver board
//! should be lit.  Finally power up the sender and press its button 1 (or, on
//! single button boards, press the button for more than 3 seconds).  Both LEDs
//! on the sender will blink until it successfully links with the receiver.
//! After successful linking, the sender will transmit a message to the
//! receiver every 3 to 6 seconds.  This message will be stored by the access
//! point and passed to the receiver the next time it polls the access point.
//! While the example is running, LEDs on both the sender and receiver will
//! blink.  No user interaction is required on the access point.
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
// A couple of macros used to introduce delays during monitoring.
//
//*****************************************************************************
#define SPIN_ABOUT_A_SECOND         ApplicationDelay(1000)

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tPushButtonWidget g_sLED1;
extern tPushButtonWidget g_sLED2;
extern tCanvasWidget g_sMainStatus;

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sMainStatus, &g_sBackground,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "SimpliciTI-polling-ap",
       0, 0);

//*****************************************************************************
//
// Canvas used to display the latest status.
//
//*****************************************************************************
#define MAX_STATUS_STRING_LEN 40
char g_pcStatus[MAX_STATUS_STRING_LEN];
Canvas(g_sMainStatus, WIDGET_ROOT, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 217, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, g_pcStatus,
       0, 0);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sBackground, &g_sHeading, 0, &g_sLED1,
       &g_sKitronix320x240x16_SSD2119, 0, 23, 320, (240 - 46),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The "LED"s used to indicate application status.  These are deliberately not
// linked to the widget tree yet.  We add these once the user choses which
// mode to run in.
//
//*****************************************************************************
CircularButton(g_sLED1, &g_sBackground, &g_sLED2, 0,
               &g_sKitronix320x240x16_SSD2119, 90, 120, 40,
               (PB_STYLE_OUTLINE | PB_STYLE_FILL | PB_STYLE_TEXT_OPAQUE |
               PB_STYLE_TEXT), ClrGreen, ClrGreen, ClrWhite, ClrWhite,
               g_pFontCmss22b, "LED1", 0, 0, 0, 0, 0);

CircularButton(g_sLED2, &g_sBackground, 0, 0,
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
UpdateStatus(const char *pcString, ...)
{
    va_list vaArgP;

    //
    // Start the varargs processing.
    //
    va_start(vaArgP, pcString);

    //
    // Call vsnprintf to format the text into the status string buffer.
    //
    uvsnprintf(g_pcStatus, MAX_STATUS_STRING_LEN, pcString, vaArgP);

    //
    // End the varargs processing.
    //
    va_end(vaArgP);

    //
    // Update the status string on the display.
    //
    WidgetPaint((tWidget *)&g_sMainStatus);
}

#ifndef USE_FIXED_DEVICE_ADDRESS
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
        UpdateStatus("Flash user registers are clear");
        UpdateStatus("Error - address not set!");
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
#else
//*****************************************************************************
//
// Set the SimpliciTI device address based on the fixed address from header
// file simpliciti_config.h.  Care must be taken when doing this since devices
// on the network are required to have unique addresses.
//
//*****************************************************************************
tBoolean
SetSimpliciTIAddress(void)
{
    static addr_t sAddr = THIS_DEVICE_ADDRESS;

    //
    // Tell the SimpliciTI stack which device address we want to use.
    //
    SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &sAddr);

    //
    // If we get here, all is well.
    //
    return(true);
}
#endif

//*****************************************************************************
//
// Main application entry function.
//
//*****************************************************************************
int
main(void)
{
    tBoolean bRetcode;
    smplStatus_t eRetcode;
    ioctlToken_t eToken;

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
    UpdateStatus("Initializing...");

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
    // Turn on both our LEDs
    //
    SetLED(1, true);
    SetLED(2, true);

    UpdateStatus("Waiting...");

    //
    // Initialize the SimpliciTI stack but don't set any receive callback.
    //
    while(1)
    {
        eRetcode = SMPL_Init((uint8_t (*)(linkID_t))0);

        if(eRetcode == SMPL_SUCCESS)
        {
            break;
        }

        ToggleLED(1);
        ToggleLED(2);
        SPIN_ABOUT_A_SECOND;
    }

    // This code example changes the Link token to be distributed to those who
    // Join. For the example here this should be done before anyone joins so
    // the Join context is defaulted to OFF for this scenario. See the
    // smpl_config.dat file. After the link token is set the Join context must
    // be enabled.
    //
    // NOTE that this is done after initialization. For APs the init sequence
    // consists of a step in which a link token is generated. The sequence here
    // overrides that setting. It can be used to distribute different link
    // tokens to different devices. The sequence here is a simple example of
    // how to use the IOCTL interface to set the Link token for subsequent
    // Joiners.
    //
    // You might want to be careful about following this particular example if
    // you are restoring from NV unless you are setting a fixed value as is
    // done here.  Unconditionally setting a random value will make it
    // essentially impossible for newly joining devices to link to devices that
    // joined before the AP was reset since they will have different link
    // tokens.
    //
    eToken.tokenType       = TT_LINK;
    eToken.token.linkToken = 0x78563412;

    SMPL_Ioctl(IOCTL_OBJ_TOKEN, IOCTL_ACT_SET, &eToken);

    //
    // Enable join context.
    //
    SMPL_Ioctl(IOCTL_OBJ_AP_JOIN, IOCTL_ACT_ON, 0);

    //
    // Tell the user what's up.
    //
    UpdateStatus("Access point active.");

    //
    // Do nothing after this - the SimpliciTI stack code handles all the
    // access point function required.
    //
    while(1)
    {
        //
        // Process the widget message queue.
        //
        WidgetMessageQueueProcess();
    }
}
