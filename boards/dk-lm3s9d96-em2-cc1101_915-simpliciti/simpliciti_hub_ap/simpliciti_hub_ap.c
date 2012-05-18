//*****************************************************************************
//
// simpliciti_hub_ap.c -  Access Point application for the "Access Point as
//                        Data Hub" SimpliciTI LPRF example.
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
// This is part of revision 8555 of the DK-LM3S9D96-EM2-CC1101_915-SIMPLICITI Firmware Package.
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
//! <h1>Access Point for ``Access Point as Data Hub'' example
//! (simpliciti_hub_ap)</h1>
//!
//! This application offers the access point functionality of the generic
//! SimpliciTI Ap_as_Data_Hub.  To run this example, two additional
//! SimpliciTI-enabled boards using compatible radios must also be present,
//! each running the EndDevice configuration of the application.  If using the
//! Stellaris development board this function is found in the
//! simpliciti_hub_dev example application.  On other hardware it is the
//! EndDevice configuration of the Ap_as_Data_Hub example as supplied with
//! SimpliciTI 1.1.1.
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
//! <li>Stellaris Development Board + EM2 expansion board + CC1101:868/915</li>
//! </ul>
//!
//! When the access point application is started, both ``LEDs'' on the display
//! are lit indicating that the access point is waiting for connections from
//! end devices.  The LEDs may start flashing, indicating that the frequency
//! agility feature has caused an automatic chanel change.  This flashing will
//! continue until a message is received from an end device.  When an end
//! device connects to the access point, pressing buttons on the end device
//! will send a message to the access point which will toggle one of its LEDs
//! depending upon the content of the message.
//!
//! The access point also offers the option to force a channel change.
//! Pressing the ``Change Channel'' button cycles to the next available radio
//! channel.  As in the automatic channel change case, this will cause the LEDs
//! to flash until a new message is received from the end device indicating
//! that it has also changes to the new channel.
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
// COMMENTS ON ASYNC LISTEN
//
// Summary:
//  This AP build includes implementation of an unknown number of end device
//  peers in addition to AP functionality.  In this scenario, all end devices
//  establish a link to the AP and only to the AP.  The AP acts as a data hub.
//  All end device peers are on the AP and not on other distinct ED platforms.
//
//  There is still a limit to the number of peers supported on the AP that is
//  defined by the macro NUM_CONNECTIONS.  The AP will support NUM_CONNECTIONS
//  or fewer peers but the exact number does not need to be known at build
//  time.
//
//  In this special but common scenario SimpliciTI restricts each end device
//  object to a single connection to the AP.  If multiple logical connections
//  are required these must be accommodated by supporting contexts in the
//  application payload itself.
//
// Solution overview:
//  When a new peer connection is required the AP main loop must be notified.
//  In essence the main loop polls a semaphore to know whether to begin
//  listening for a peer Link request from a new end device.  There are two
//  solutions: automatic notification and external notification.  The only
//  difference between the automatic notification solution and the external
//  notification solution is how the listen semaphore is set. In the external
//  notification solution the sempahore is set by the user when the AP is
//  stimulated for example by a button press or a commend over a serial link.
//  In the automatic scheme the notification is accomplished as a side effect
//  of a new end device joining.  This example illustrates the automatic
//  notification method.
//
//  The Rx callback must be implemented. When the callback is invoked with a
//  non-zero Link ID the handler could set a semaphore that alerts the main
//  work loop that an SMPL_Receive() can be executed successfully on that Link
//  ID.
//
//  If the callback conveys an argument (LinkID) of 0 then a new device has
//  joined the network and an SMPL_LinkListen() should be executed.
//
//  Whether the joining device supports ED objects is indirectly inferred on
//  the joining device from the setting of the NUM_CONNECTIONS macro.  The
//  value of this macro should be non-zero only if ED objects exist on the
//  device.  This macro is always non-zero for ED-only devices.  Range
//  Extenders may or may not support ED objects.  The macro should be be set
//  to 0 for REs that do not also support ED objects.  This prevents the Access
//  Point from reserving resources for a joining device that does not support
//  any end device objects and it prevents the AP from executing an
//  SMPL_LinkListen().  The access point will not ever see a Link frame if the
//  joining device does not support any connections.
//
//  Each joining device must execute an SMPL_Link() after receiving the join
//  reply from the access point.  The access point will be listening.
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
#define SPIN_ABOUT_A_QUARTER_SECOND ApplicationDelay(250)

//*****************************************************************************
//
// Reserve space for the maximum possible number of peer Link IDs.
//
//*****************************************************************************
static linkID_t g_sLID[NUM_CONNECTIONS] = {0};
static uint8_t  g_ucNumCurrentPeers = 0;

//*****************************************************************************
//
// SimpliciTI receive callback handler
//
//*****************************************************************************
static uint8_t ReceiveCallback(linkID_t);

//*****************************************************************************
//
// Received message handler function
//
//*****************************************************************************
static void ProcessMessage(linkID_t, uint8_t *, uint8_t);

//*****************************************************************************
//
// Frequency Agility helper functions
//
//*****************************************************************************
static void    CheckChangeChannel(void);
static void    ChangeChannel(void);

//*****************************************************************************
//
// Work loop semaphores
//
//*****************************************************************************
static volatile uint8_t g_ucPeerFrameSem = 0;
static volatile uint8_t g_ucJoinSem = 0;

#ifdef FREQUENCY_AGILITY
//*****************************************************************************
//
// Variables and definitions related to supporting frequency agility.
//
//*****************************************************************************
#define INTERFERENCE_THRESHOLD_DBM (-70)
#define SSIZE    25
#define IN_A_ROW  3
static int8_t  g_cSample[SSIZE];
static uint8_t g_ucChannel = 0;

#endif  // FREQUENCY_AGILITY

//*****************************************************************************
//
// Flag used to indicate that we need to blink the LEDs during channel changes.
//
//*****************************************************************************
static volatile unsigned long g_ulBlinky = 0;

//*****************************************************************************
//
// Flag used to indicate that a channel change has been requested.
//
//*****************************************************************************
static volatile tBoolean g_bChangeChannel = false;

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tPushButtonWidget g_sChannelBtn;
extern tPushButtonWidget g_sLED1;
extern tPushButtonWidget g_sLED2;
extern tCanvasWidget g_sMainStatus;
extern tCanvasWidget g_sDeviceStatus;

//*****************************************************************************
//
// Forward reference to the button press handler.
//
//*****************************************************************************
void OnChannelButtonPress(tWidget *pWidget);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sMainStatus, &g_sBackground,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "SimpliciTI-hub-ap",
       0, 0);

//*****************************************************************************
//
// Canvas used to display the latest status.
//
//*****************************************************************************
#define MAX_STATUS_STRING_LEN 40
char g_pcStatus[2][MAX_STATUS_STRING_LEN];
Canvas(g_sMainStatus, WIDGET_ROOT, &g_sDeviceStatus, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 217, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, g_pcStatus[0],
       0, 0);

//*****************************************************************************
//
// A canvas showing the device status.
//
//*****************************************************************************
Canvas(g_sDeviceStatus, WIDGET_ROOT, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 194, 320, 23, (CANVAS_STYLE_FILL |
       CANVAS_STYLE_TEXT), ClrBlack, ClrWhite, ClrWhite, g_pFontCm20,
       g_pcStatus[1], 0, 0);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sBackground, &g_sHeading, 0, &g_sChannelBtn,
       &g_sKitronix320x240x16_SSD2119, 0, 23, 320, (240 - 69),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The button used to signal an alarm.
//
//*****************************************************************************
RectangularButton(g_sChannelBtn, &g_sBackground, &g_sLED1, 0,
                  &g_sKitronix320x240x16_SSD2119, 174, 90, 140, 60,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrBlack, ClrRed, ClrWhite, ClrWhite,
                   g_pFontCmss18b, "Change Channel", 0, 0, 0, 0,
                   OnChannelButtonPress);

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
// Handler for the "Change Channel" button.
//
//*****************************************************************************
void
OnChannelButtonPress(tWidget *pWidget)
{
    //
    // Set a flag indicating that the main loop should change channel at the
    // next opportunity.
    //
    g_bChangeChannel = true;
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
                (tWidget *)&g_sDeviceStatus);
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
        UpdateStatus(true, "Flash user registers are clear");
        UpdateStatus(false, "Error - address not set!");
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
    bspIState_t intState;
    uint8_t ucLastChannel;

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
    UpdateStatus(true, "Initializing...");

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

    UpdateStatus(true, "Waiting for a device...");

    //
    // Initialize the SimpliciTI stack and register our receive callback.
    //
    SMPL_Init(ReceiveCallback);

    //
    // Tell the user what's up.
    //
    UpdateStatus(true, "Access point active.");

    //
    // Do nothing after this - the SimpliciTI stack code handles all the
    // access point function required.
    //
    while(1)
    {
        //
        // Wait for the Join semaphore to be set by the receipt of a Join
        // frame from a device that supports an end device.
        //
        // An external method could be used as well. A button press could be
        // connected to an ISR and the ISR could set a semaphore that is
        // checked by a function call here, or a command shell running in
        // support of a serial connection could set a semaphore that is
        // checked by a function call.
        //
        if (g_ucJoinSem && (g_ucNumCurrentPeers < NUM_CONNECTIONS))
        {
            //
            // Listen for a new incoming connection.
            //
            while (1)
            {
                if (SMPL_SUCCESS == SMPL_LinkListen(&g_sLID[g_ucNumCurrentPeers]))
                {
                    //
                    // The connection attempt succeeded so break out of the
                    // loop.
                    //
                    break;
                }

                //
                // Process our widget message queue.
                //
                WidgetMessageQueueProcess();

                //
                // A "real" application would implement its fail-to-link
                // policy here.  We go back and listen again.
                //
            }

            //
            // Increment our peer counter.
            //
            g_ucNumCurrentPeers++;

            //
            // Decrement the join semaphore.
            //
            BSP_ENTER_CRITICAL_SECTION(intState);
            g_ucJoinSem--;
            BSP_EXIT_CRITICAL_SECTION(intState);

            //
            // Tell the user how many devices we are now connected to.
            //
            UpdateStatus(false, "%d devices connected.", g_ucNumCurrentPeers);
        }

        //
        // Have we received a frame on one of the ED connections? We don't use
        // a critical section here since it doesn't really matter much if we
        // miss a poll.
        //
        if (g_ucPeerFrameSem)
        {
            uint8_t     pucMsg[MAX_APP_PAYLOAD], ucLen, ucLoop;

            /* process all frames waiting */
            for (ucLoop = 0; ucLoop < g_ucNumCurrentPeers; ucLoop++)
            {
                //
                // Receive the message.
                //
                if (SMPL_SUCCESS == SMPL_Receive(g_sLID[ucLoop], pucMsg,
                                                 &ucLen))
                {
                    //
                    // ...and pass it to the function that processes it.
                    //
                    ProcessMessage(g_sLID[ucLoop], pucMsg, ucLen);

                    //
                    // Decrement our frame semaphore.
                    //
                    BSP_ENTER_CRITICAL_SECTION(intState);
                    g_ucPeerFrameSem--;
                    BSP_EXIT_CRITICAL_SECTION(intState);
                }
            }
        }

        //
        // Have we been asked to change channel?
        //
        ucLastChannel = g_ucChannel;
        if (g_bChangeChannel)
        {
            //
            // Yes - go ahead and change to the next radio channel.
            //
            g_bChangeChannel = false;
            ChangeChannel();
        }
        else
        {
            //
            // No - check to see if we need to automatically change channel
            // due to interference on the current one.
            //
            CheckChangeChannel();
        }

        //
        // If the channel changed, update the display.
        //
        if(g_ucChannel != ucLastChannel)
        {
            UpdateStatus(false, "Changed to channel %d.", g_ucChannel);
        }

        //
        // If required, blink the "LEDs" to indicate we are waiting for a
        // message following a channel change.
        //
        BSP_ENTER_CRITICAL_SECTION(intState);
        if (g_ulBlinky)
        {
            if (++g_ulBlinky >= 0xF)
            {
                g_ulBlinky = 1;
                ToggleLED(1);
                ToggleLED(2);
            }
        }
        BSP_EXIT_CRITICAL_SECTION(intState);

        //
        // Process our widget message queue.
        //
        WidgetMessageQueueProcess();
    }
}

//*****************************************************************************
//
// SimpliciTI receive callback.  This function runs in interrupt context.
// Reading the frame should be done in the application main loop or thread not
// in the ISR.
//
//*****************************************************************************
static uint8_t
ReceiveCallback(linkID_t sLinkID)
{
    //
    // Have we been sent a frame on an active link?
    //
    if (sLinkID)
    {
        //
        // Yes - set the semaphore indicating we need to receive the frame.
        // This will be done in the main loop.
        //
        g_ucPeerFrameSem++;
        g_ulBlinky = 0;
    }
    else
    {
        //
        // No - a new device has joined the network but has not linked to us.
        // Set the semaphore indicating that we should listen for an incoming
        // link request.
        //
        g_ucJoinSem++;
    }

    //
    // Leave the frame to be read by the main loop of the application.
    //
    return 0;
}

//*****************************************************************************
//
// The main loop calls this function to process received SimpliciTI messages.
// In this case, we are using a trivial protocol which merely passes a LED
// number and transaction ID.  We toggle the LED that is indicated in the
// message.
//
//*****************************************************************************
static void
ProcessMessage(linkID_t sLID, uint8_t *pucMsg, uint8_t ucLen)
{
    //
    // Were we passed a real message (non-zero length)?
    //
    if (ucLen)
    {
        //
        // Yes - the first byte of the message indicates an LED that we should
        // toggle so go ahead and do this.
        //
        ToggleLED(*pucMsg);
    }
    return;
}

//*****************************************************************************
//
// Change to a different SimpliciTI channel number (frequency).
//
//*****************************************************************************
static void ChangeChannel(void)
{
#ifdef FREQUENCY_AGILITY
    freqEntry_t sFreq;

    //
    // Cycle to the next channel, wrapping from the top of the table back
    // to the bottom if necessary.
    //
    if (++g_ucChannel >= NWK_FREQ_TBL_SIZE)
    {
        g_ucChannel = 0;
    }

    //
    // Set the radio frequency.
    //
    sFreq.logicalChan = g_ucChannel;
    SMPL_Ioctl(IOCTL_OBJ_FREQ, IOCTL_ACT_SET, &sFreq);

    //
    // Turn both of the "LEDs" off and set the flag we use to tell the main
    // loop that it should blink the LEDs until a new message is received from
    // an end device.
    //
    SetLED(1, false);
    SetLED(2, false);
    g_ulBlinky = 1;
#endif
    return;
}

//*****************************************************************************
//
// This function implements the auto channel change policy.
//
//*****************************************************************************
static void
CheckChangeChannel(void)
{
#ifdef FREQUENCY_AGILITY
    int8_t ucdBm, ucInARow = 0;
    unsigned long ulLoop;

    //
    // Clear out our signal quality array.
    //
    memset(g_cSample, 0x0, SSIZE);

    //
    // Poll the signal quality several times.
    //
    for (ulLoop = 0; ulLoop < SSIZE; ulLoop++)
    {
        //
        // Exit early if we need to service an app frame.
        //
        if (g_ucPeerFrameSem || g_ucJoinSem)
        {
            return;
        }

        //
        // Get the signal quality from the radio.
        //
        NWK_DELAY(1);
        SMPL_Ioctl(IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_RSSI, (void *)&ucdBm);
        g_cSample[ulLoop] = ucdBm;

        //
        // Is the noise level above our threshold?
        //
        if (ucdBm > INTERFERENCE_THRESHOLD_DBM)
        {
            //
            // Yes - increment the counter we use to determine how long the
            // signal quality has been unacceptable and check to see if we've
            // reached the point where we perform an automatic channel change.
            //
            if (++ucInARow == IN_A_ROW)
            {
                //
                // Signal quality has been too bad for too long so try the next
                // channel.
                //
                ChangeChannel();
                break;
            }
        }
        else
        {
            //
            // Signal quality is fine so clear the counter we use to determine
            // how long the quality has been bad.
            //
            ucInARow = 0;
        }
    }
#endif
    return;
}
