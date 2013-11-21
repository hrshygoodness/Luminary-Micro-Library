//*****************************************************************************
//
// simpliciti_chronos.c -  Access point application for the use with the eZ430
//                         Chronos development kit.
//
// Copyright (c) 2010-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the DK-LM3S9D96-EM2-CC1101_433-SIMPLICITI Firmware Package.
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
#include "grlib/imgbutton.h"
#include "drivers/touch.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "widgets.h"
#include "images.h"
#include "simpliciti_chronos.h"

//
// SimpliciTI Headers
//
#include "simplicitilib.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Access Point for use with eZ430-Chronos-433 (simpliciti_chronos)</h1>
//!
//! This application provides a SimpliciTI Low Power RF access point that is
//! capable of receiving and displaying data from an eZ430-Chronos-433
//! development tool running the default Sports Watch firmware.  The
//! development board must be equipped with an EM2 expansion board and a
//! CC1101EM 433Mhz radio transceiver and antenna for this application to run
//! correctly.  The CC1101EM module must be installed in the ``mod1'' connector
//! (the connector nearest the oscillator) on the EM2 expansion board.
//!
//! The eZ430-Chronos-433 development tool uses the SimpliciTI protocol to
//! support three of the Sport Watch features - ``RF Tilt Control'', ``PPT
//! Control'' and ``RF Sync''. The simpliciti_chronos example application
//! detects which of these modes is in operation and configures the display
//! appropriately to show the information that the watch is transmitting.  If
//! no packet is! received from the watch within 5 seconds, the display reverts
//! to the initial opening screen and the application waits for new data.
//!
//! To select each of the operating modes, cycle through the various options
//! offered by pressing the lower left ``#'' button on the sports watch until
//! either ``ACC'', ``PPt'' or ``SYNC'' is displayed. To activate data
//! transmission press the lower right, down arrow key.  When transmission is
//! active, a wireless icon will flash on the watch's display.  Pressing the
//! down arrow key once again will disable the transmitter.
//!
//! <h3>RF Tilt Control Mode (ACC)</h3>
//!
//! In RF Tilt Control mode, the watch sends packets containing information on
//! button presses and also the output from its integrated 3-axis
//! accelerometer.  The simpliciti_chronos application displays button presses
//! by highlighting graphics of the buttons on the display.  Accelerometer
//! data is plotted in an area of the screen with (x,y) data used to determine
//! the position of lines drawn and z data controlling the color.  Buttons are
//! provided on the display to set the origin point for the accelerometer
//! data (``Calibrate'') and clear the accelerometer display (``Clear'').
//!
//! <h3>PowerPoint Control Mode (PPt)</h3>
//!
//! In PowerPoint Control mode, only button press information is transmitted
//! by the watch.  When these packets are detected by the access point, it
//! displays an image of the watch face and highlights which buttons have been
//! pressed.
//!
//! <h3>Sync Mode (SYNC)</h3>
//!
//! Sync mode is used to set various watch parameters and allows an access
//! point to send commands to the watch and retrieve status from it.  Every
//! 500mS or so, the watch sends a ``ready to receive'' packet indicating that
//! it is able to process a new command.  The access point application
//! responds to this with a status request causing the watch to send back a 19
//! byte status packet containing the current time, date, alarm time, altitude
//! and temperature.  This data is then formatted and displayed on the
//! development board LCD screen.  A button on the display allows the watch
//! display format to be toggled between metric and imperial.  When this button
//! is pressed, a command is sent to the watch to set the new format and this
//! display format change takes effect with the next incoming status message.
//!
//! Note that the ``ready to receive'' packets sent by the watch are not
//! synchronized with the watch second display.  This introduces an
//! interference effect that results in the development board seconds display
//! on the LCD not update at regular 1 second intervals.
//
//*****************************************************************************

#ifdef ISM_EU
const char g_pcFrequency[] = "868MHz";
#else
#ifdef ISM_US
const char g_pcFrequency[] = "915MHz";
#else
#ifdef ISM_LF
const char g_pcFrequency[] = "433MHz";
#else
#error Frequency band is not defined!
#endif
#endif
#endif

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
typedef struct
{
    linkID_t sLinkID;
    tBoolean bMetric;
    short sAccel[3];
    short sAccelOffset[3];
    unsigned short usYear;
    short sTemperature;
    short sAltitude;
    unsigned char ucMode;
    unsigned char ucButtons;
    unsigned char ucHours;
    unsigned char ucMinutes;
    unsigned char ucSeconds;
    unsigned char ucMonth;
    unsigned char ucDay;
    unsigned char ucAlarmHours;
    unsigned char ucAlarmMinutes;
}
tLinkState;

static tLinkState g_sLinkInfo[NUM_CONNECTIONS];
static uint8_t g_ucNumCurrentPeers = 0;
static uint8_t g_ucCurrentPeer = 0;

//*****************************************************************************
//
// Strings containing the names of the months.
//
//*****************************************************************************
#define NUM_MONTHS 12
const char *g_pcMonths[NUM_MONTHS] =
{
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
};

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
static void ProcessMessage(unsigned long ulIndex, uint8_t *, uint8_t);

//*****************************************************************************
//
// Prototypes for various display update functions.
//
//*****************************************************************************
static void UpdateButtonDisplay(unsigned long ulPanel,
                                unsigned char ucBtnState);
static void UpdateAccelDisplay(unsigned long ulPanel, unsigned long ulIndex);

//*****************************************************************************
//
// Work loop semaphores
//
//*****************************************************************************
static volatile uint8_t g_ucPeerFrameSem = 0;
static volatile uint8_t g_ucJoinSem = 0;

//*****************************************************************************
//
// The number of bytes in the various data packets from the Chronos watch.
//
//*****************************************************************************
#define ACC_PACKET_SIZE     4
#define R2R_PACKET_SIZE     2
#define STATUS_PACKET_SIZE 19

//*****************************************************************************
//
// Command sent to the Chronos watch in Sync mode requesting it to send its
// status information.
//
//*****************************************************************************
#define SYNC_AP_CMD_GET_STATUS 2
#define SYNC_AP_CMD_SET_WATCH  3

//*****************************************************************************
//
// Values in the bottom nibble of the first byte of accelerometer packets.
// These tell us whether the packet contains accelerometer data in addition to
// the button states.
//
//*****************************************************************************
#define SIMPLICITI_EVENT_MASK   0x0F
#define SIMPLICITI_MOUSE_EVENTS 0x01
#define SIMPLICITI_KEY_EVENTS   0x02

//*****************************************************************************
//
// Values in the top nibble of the first byte of accelerometer packets.
// These tell us whenever the state of one of the watch buttons changes.
//
//*****************************************************************************
#define PACKET_BTN_MASK     0x30
#define PACKET_BTN_SHIFT    4

//*****************************************************************************
//
// Bits representing each of the 3 buttons whose states are passed to us in
// accelerometer packets.  These are equivalent to
//
//     (1 << ((acc_packet[0] & PACKET_BTN_MASK) >> PACKET_BTN_SHIFT))
//
// since the button is encoded as an index in the packet.
//
//*****************************************************************************
#define BUTTON_BIT_STAR     0x02
#define BUTTON_BIT_NUM      0x04
#define BUTTON_BIT_UP       0x08
#define BUTTON_BIT(x) (1 << (((x) & PACKET_BTN_MASK) >> PACKET_BTN_SHIFT))

//*****************************************************************************
//
// Global holding the index of the panel that is currently displayed.
//
//*****************************************************************************
unsigned long g_ulCurrentPanel = PANEL_WAITING;

//*****************************************************************************
//
// A global system tick counter.
//
//*****************************************************************************
static volatile unsigned long g_ulSysTickCount;

//*****************************************************************************
//
// The time at which all button press indicators on the display must be
// cleared.
//
//*****************************************************************************
static unsigned long g_ulClearBtnTimer;

//*****************************************************************************
//
// The time at which we switch back to "Waiting" mode.  This is set each time
// we receive a packet from a Chronos device.
//
//*****************************************************************************
#define PACKET_TIMEOUT_SECONDS 5
static unsigned long g_ulPacketResetTimer;

//*****************************************************************************
//
// Variables used to control the accelerometer scribble drawing.
//
//*****************************************************************************
static tBoolean g_bClearAccelCanvas = true;
static short g_sXPosAccel;
static short g_sYPosAccel;

//*****************************************************************************
//
// A flag used to record that we must switch the eZ430-Chronos display format.
//
//*****************************************************************************
static tBoolean g_bSwitchFormat;

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
// Update the status string on the display.
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
        UpdateStatus("No MAC Address!");
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
// Set the display to show a particular panel of widgets.  If the requested
// panel is already displayed, this function does nothing.
//
//*****************************************************************************
void
ChangeDisplayPanel(unsigned long ulNewPanel)
{
    //
    // Is the new panel different from the one that is currently displayed?
    //
    if(ulNewPanel != g_ulCurrentPanel)
    {
        //
        // Remove the widgets from the current panel.
        //
        WidgetRemove((tWidget *)&g_psPanels[g_ulCurrentPanel]);

        //
        // Add the new panel's widgets into the active tree.
        //
        WidgetAdd((tWidget *)&g_sHeading, (tWidget *)&g_psPanels[ulNewPanel]);

        //
        // Repaint the screen to show the new widgets.
        //
        WidgetPaint((tWidget *)&g_psPanels[ulNewPanel]);

        //
        // Update our record of the current display panel.
        //
        g_ulCurrentPanel = ulNewPanel;

        //
        // Perform some panel-specific initialization here.
        //
        if(g_ulCurrentPanel == PANEL_ACC)
        {
            //
            // Clear the acceleration scribble control.
            //
            g_bClearAccelCanvas = true;
        }
    }
}

//*****************************************************************************
//
// Cause a repaint of the provided widget and its children if the current panel
// matches the panel whose index is passed.  This function may be called to
// update the display at any time regardless of which panel is displayed but
// will only actually do the update if the parameters indicate that the update
// is for the currently displayed panel.
//
//*****************************************************************************
void
UpdateDisplay(tWidget *psWidget, unsigned long ulPanel)
{
    //
    // Is this panel currently on the display?
    //
    if(ulPanel == g_ulCurrentPanel)
    {
        //
        // Yes - actually perform the repaint.
        //
        WidgetPaint(psWidget);
    }
}

//*****************************************************************************
//
// Button handler for the "Change" button.  This cycles between the various
// eZ430-Chronos devices currently attached to the access point.
//
//*****************************************************************************
void
OnChangeButtonPress(tWidget *pWidget)
{
    //
    // If nothing is connected, this button shouldn't be displayed so can't
    // be pressed.  Just in case, though...
    //
    if(g_ucNumCurrentPeers)
    {
        //
        // Cycle to the next connected device.
        //
        g_ucCurrentPeer++;

        //
        // If we reach the end of our peer list, cycle back to the start.
        //
        if(g_ucCurrentPeer > g_ucNumCurrentPeers)
        {
            g_ucCurrentPeer = 1;
        }

        //
        // Tell the user which of the devices we are monitoring.
        //
        UpdateStatus("Showing device %d of %d connected.",
                     g_ucCurrentPeer, g_ucNumCurrentPeers);

        //
        // Make sure we clear the accelerometer display next time it is shown.
        //
        g_bClearAccelCanvas = 1;

        //
        // Reset the packet timeout to ensure that the screen is cleared in
        // half a second if no data is received from the new device.
        //
        g_ulPacketResetTimer = g_ulSysTickCount + (TICKS_PER_SECOND / 2);
    }
}


//*****************************************************************************
//
// Button handler for the "Format" button.  This causes the eZ430-Chronos to be
// toggled between imperial and metric display modes.
//
//*****************************************************************************
void
OnFormatButtonPress(tWidget *pWidget)
{
    //
    // Set the flag which will tell our packet handler that it must set the
    // eZ430-Chronos display mode next time it is polled for a command.
    //
    g_bSwitchFormat = true;
}

//*****************************************************************************
//
// Button handler for the "Calibrate" button.  This uses the current readings
// from the accelerometer to set the zero point for future measurement.
//
//*****************************************************************************
void
OnCalibrateButtonPress(tWidget *pWidget)
{
    unsigned long ulLoop;

    //
    // Copy the current accelerometer readings into the offset fields to use
    // as the origin when reading future values.
    //
    for(ulLoop = 0; ulLoop < 3; ulLoop++)
    {
        g_sLinkInfo[g_ucCurrentPeer - 1].sAccelOffset[ulLoop] =
            g_sLinkInfo[g_ucCurrentPeer - 1].sAccel[ulLoop];
    }

    //
    // Update the display and clear the current scribble.
    //
    g_bClearAccelCanvas = true;
    UpdateAccelDisplay(g_sLinkInfo[g_ucCurrentPeer - 1].ucMode,
                       g_ucCurrentPeer - 1);
}

//*****************************************************************************
//
// Button handler for the "Clear" button.  This clears the area of the screen
// which is drawn on by moving the eZ430-Chronos watch while in accelerometer
// mode.
//
//*****************************************************************************
void
OnClearButtonPress(tWidget *pWidget)
{
    //
    // Tell the acceleration scribble canvas to clear itself.
    //
    g_bClearAccelCanvas = true;
    WidgetPaint((tWidget *)&g_sDrawingCanvas);
}

//*****************************************************************************
//
// Calculate the coordinates of a point within the rectangle provided which
// represents the latest acceleration reading from the eZ430-Chronos whose
// data we are currently displaying.
//
// The raw acceleration data is in the range [-128, 127].
//
//*****************************************************************************
static void
CalculateAccelPoint(tRectangle *pRect, long *plNewX, long *plNewY)
{
    tRectangle sRect;
    long lRawX, lRawY, lXCenter, lYCenter;

    //
    // Get the raw X and Y acceleration values adjusted for any offset that
    // has been set.  We reverse these since this offers a more realistic
    // scribbling experience on the display.
    //
    lRawX = (long)(g_sLinkInfo[g_ucCurrentPeer - 1].sAccel[1] -
                   g_sLinkInfo[g_ucCurrentPeer - 1].sAccelOffset[1]);
    lRawY = (long)(g_sLinkInfo[g_ucCurrentPeer - 1].sAccel[0] -
                   g_sLinkInfo[g_ucCurrentPeer - 1].sAccelOffset[0]);

    //
    // Get the rectangle corresponding to the drawing canvas and adjust it
    // to represent the interior drawing area.
    //
    sRect = *pRect;
    sRect.sXMax--;
    sRect.sYMax--;
    sRect.sXMin++;
    sRect.sYMin++;

    //
    // Determine the coordinates of the center of the drawing area.
    //
    lXCenter = (long)((pRect->sXMax + pRect->sXMin) / 2);
    lYCenter = (long)((pRect->sYMax + pRect->sYMin) / 2);

    //
    // Translate the values such that their origin is at the center of the
    // drawing area.  Although the accelerometer range is [-128, 127] and
    // our drawing area is likely to be smaller than this, we don't scale the
    // values since normal tilting of the watch doesn't generate particularly
    // large swings in the measured acceleration.  Instead we use the unscaled
    // value and clip to the edges of the drawing area.
    //
    lRawX += lXCenter;
    lRawY += lYCenter;

    //
    // Clip them to the display area.
    //
    lRawX = (lRawX < sRect.sXMin) ? sRect.sXMin : lRawX;
    lRawX = (lRawX >= sRect.sXMax) ? sRect.sXMax : lRawX;
    lRawY = (lRawY < sRect.sYMin) ? sRect.sYMin : lRawY;
    lRawY = (lRawY >= sRect.sYMax) ? sRect.sYMax : lRawY;

    //
    // Translate the X and Y values such that they are centered about the
    // midpoint of the drawing area.
    //
    *plNewX = lRawX;
    *plNewY = lRawY;
}

//*****************************************************************************
//
// Paint handlers for the canvas widget we use to display accelerometer values.
// This control merely draws lines between points corresponding to each raw
// (x,y) accelerometer reading, changing the color according to the Z value.
//
// Note that this is merely an indication of the acceleration reading and does
// not perform any rigorous mathematics to convert acceleration to position.
// Using the raw values allows us to generate a reasonably good "scribble"
// effect by tilting the watch left or right, back or forward.
//
//*****************************************************************************
void
OnPaintAccelCanvas(tWidget *pWidget, tContext *pContext)
{
    tRectangle sRect;
    tCanvasWidget *pCanvas;
    unsigned long ulColor;
    long lNewX, lNewY;

    //
    // Get a pointer to the canvas structure.
    //
    pCanvas = (tCanvasWidget *)pWidget;

    //
    // Have we been asked to initialize the canvas?  If so, draw the border,
    // clear the main drawing area and reset the drawing position and color.
    //
    if(g_bClearAccelCanvas || !g_ucCurrentPeer)
    {
        //
        // Remember that we've cleared the control.
        //
        g_bClearAccelCanvas = false;

        //
        // Determine the bounding rectangle of the canvas.
        //
        sRect = pWidget->sPosition;

        //
        // Outline the area in the required color.
        //
        GrContextForegroundSet(pContext, pCanvas->ulOutlineColor);
        GrRectDraw(pContext, &sRect);

        //
        // Adjust the rectangle to represent only the inner drawing area.
        //
        sRect.sXMin++;
        sRect.sXMax--;
        sRect.sYMin++;
        sRect.sYMax--;

        //
        // Clear the drawing surface.
        //
        GrContextForegroundSet(pContext, pCanvas->ulFillColor);
        GrRectFill(pContext, &sRect);

        //
        // Reset our drawing position.
        //
        g_sXPosAccel = (sRect.sXMax + sRect.sXMin) / 2;
        g_sYPosAccel = (sRect.sYMax + sRect.sYMin) / 2;
    }
    else
    {
        //
        // We've not been told to initialize the whole canvas so merely draw
        // a line between the last point we plotted and the latest acceleration
        // value read.  First pick the color to use. We use cyan to indicate
        // negative Z acceleration and yellow to indicate positive with the
        // brightness indicating the magnitude.
        //

        //
        // Get the magnitude of the current Z acceleration value.
        //
        ulColor = (g_sLinkInfo[g_ucCurrentPeer - 1].sAccel[2] > 0) ?
                        g_sLinkInfo[g_ucCurrentPeer - 1].sAccel[2] :
                       -g_sLinkInfo[g_ucCurrentPeer - 1].sAccel[2];

        //
        // Scale it into the range [0-512].  This gives us a more dramatic
        // color shift with typical watch movements.
        //
        ulColor *= 4;

        //
        // Saturate at 255.
        //
        if(ulColor > 255)
        {
            ulColor = 255;
        }

        //
        // Now shift this value to create a shade of red or blue.
        //
        ulColor = ulColor << ((g_sLinkInfo[g_ucCurrentPeer - 1].sAccel[2] > 0) ?
                        16 : 0);

        //
        // Add blue to shift this to an orange or yellow value.  This prevents
        // the drawing from being black when no Z acceleration is registered.
        //
        ulColor |= 0xFF00;

        //
        // Get the X and Y coordinates representing the new accelerometer
        // reading
        //
        CalculateAccelPoint(&pWidget->sPosition, &lNewX, &lNewY);

        //
        // Now draw a line using this color from the last point we plotted to
        // the new one.
        //
        GrContextForegroundSet(pContext, ulColor);
        GrLineDraw(pContext, (long)g_sXPosAccel, (long)g_sYPosAccel,
                   lNewX, lNewY);

        //
        // Remember the new drawing position.
        //
        g_sXPosAccel = (short)lNewX;
        g_sYPosAccel = (short)lNewY;
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
    unsigned long ulLoop;
    unsigned char ucPower;
    bspIState_t intState;
    smplStatus_t eRetcode;
    tWidget *pStatus;

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
    // Clear our data value strings.
    //
    for(ulLoop = 0; ulLoop < 3; ulLoop++)
    {
        usnprintf(g_pcAccStrings[ulLoop], MAX_DATA_STRING_LEN, "-");
    }

    //
    // Set our instance data such that it is properly updated first time
    // through.
    //
    for(ulLoop = 0; ulLoop < NUM_CONNECTIONS; ulLoop++)
    {
        memset(&g_sLinkInfo[ulLoop], 0xFF, sizeof(tLinkState));
        g_sLinkInfo[ulLoop].sAccelOffset[0] = 0;
        g_sLinkInfo[ulLoop].sAccelOffset[1] = 0;
        g_sLinkInfo[ulLoop].sAccelOffset[2] = 0;
    }

    //
    // Construct the string telling the user we're waiting for data.
    //
    usnprintf(g_pcWaiting, MAX_WAITING_STRING_LEN,
              "Waiting for data from %s eZ430-Chronos...",
              g_pcFrequency);

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
    // Initialize the status string. This will remain displayed if the
    // SimpliciTI initialization hangs. If all is well, it will be erased
    // in a blink of the eye and no-one will see it.
    //
    UpdateStatus("Wrong radio! CC1101EM required.");

    //
    // Paint the widget tree to make sure they all appear on the display. We
    // do this here to ensure that the display shows something even if the
    // wrong radio EM is installed. If this happens, the SimpliciTI code will
    // hang in a while(1) loop so, without anything on the screen, it's not
    // very user-friendly.
    //
    WidgetPaint(WIDGET_ROOT);
    WidgetMessageQueueProcess();

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
        // Give the user a hint as to what could be going wrong.
        //
        UpdateStatus("Device address error! MAC set?");

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

    UpdateStatus("Initializing SimpliciTI...");

    //
    // Initialize the SimpliciTI stack and register our receive callback.
    //
    SMPL_Init(ReceiveCallback);

    //
    // Set output power to +1.1dBm (868MHz) / +1.3dBm (915MHz)
    //
    ucPower = IOCTL_LEVEL_2;
    SMPL_Ioctl(IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SETPWR, &ucPower);

    //
    // Tell the user what's up.
    //
    UpdateStatus("Access Point Active");

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
                eRetcode = SMPL_LinkListen(
                                    &g_sLinkInfo[g_ucNumCurrentPeers].sLinkID);
                if (SMPL_SUCCESS == eRetcode)
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
            // If this is the first device to connect, set it as the device
            // we are currently monitoring.
            //
            if(!g_ucNumCurrentPeers)
            {
                g_ucCurrentPeer = 1;
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
            // Get a pointer to our status string widget.
            //
            pStatus = (tWidget *)&g_sMainStatus;

            //
            // If more than 1 device is connected, show the "Change" button.
            //
            if(g_ucNumCurrentPeers > 1)
            {
                //
                // Resize the status control and add the change button to its
                // right.
                //
                pStatus->sPosition.sXMax = pStatus->sPosition.sXMin +
                                           (STATUS_PART_WIDTH - 1);
                WidgetAdd((tWidget *)&g_sMainStatus,
                          (tWidget *)&g_sChangeButton);
            }
            else
            {
                //
                // Remove the change button and revert the status widget to
                // the full width of the display/
                //
                WidgetRemove((tWidget *)&g_sChangeButton);
                pStatus->sPosition.sXMax = pStatus->sPosition.sXMin +
                                           (STATUS_FULL_WIDTH - 1);
            }

            //
            // Tell the user how many devices we are now connected to.
            //
            if(g_ucNumCurrentPeers > 1)
            {
                UpdateStatus("Showing device %d of %d connected.",
                             g_ucCurrentPeer, g_ucNumCurrentPeers);
            }
            else
            {
                UpdateStatus("%d device%s connected.", g_ucNumCurrentPeers,
                             (g_ucNumCurrentPeers == 1) ? "" : "s");
            }
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
                if (SMPL_SUCCESS == SMPL_Receive(g_sLinkInfo[ucLoop].sLinkID,
                                                 pucMsg, &ucLen))
                {
                    //
                    // ...and pass it to the function that processes it if it
                    // is from the device we are currently monitoring.  We
                    // discard packets from the other devices.  Note that the
                    // g_ucCurrentPeer variable holds a 1-based index so we
                    // need to adjust it to correlate with 0-based ucLoop.
                    //
                    if(ucLoop == (g_ucCurrentPeer - 1))
                    {
                        ProcessMessage(ucLoop, pucMsg, ucLen);
                    }

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
        // Check to see if we need to clear the button states on the display.
        //
        if(g_ulClearBtnTimer && (g_ulSysTickCount >= g_ulClearBtnTimer))
        {
            //
            // Clear all the button state widgets on both the Acc and Ppt
            // mode displays.
            //
            UpdateButtonDisplay(PANEL_ACC, 0);
            UpdateButtonDisplay(PANEL_PPT, 0);

            //
            // Clear the button timer.
            //
            g_ulClearBtnTimer = 0;
        }

        //
        // Check to see if it has been PACKET_TIMEOUT_SECONDS since we last
        // received a radio packet and, if so, revert to "Waiting" mode.
        //
        if((g_ulSysTickCount >= g_ulPacketResetTimer) && g_ulPacketResetTimer)
        {
            //
            // Turn the packet timer off for now.
            //
            g_ulPacketResetTimer = 0;

            //
            // Switch back to "Waiting" mode.
            //
            ChangeDisplayPanel(PANEL_WAITING);
        }

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
        // Yes - Set the semaphore indicating we need to receive the frame.
        // This will be done in the main loop.
        //
        g_ucPeerFrameSem++;
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
// Set the state of the button indicators on the display.  There are two sets
// of widgets that may need updating here depending upon the display mode so
// this function takes this into account.
//
//*****************************************************************************
static void
UpdateButtonDisplay(unsigned long ulPanel, unsigned char ucBtnState)
{
    tCanvasWidget *psUp, *psStar, *psNum;

    //
    // Determine which widgets we will be updating.  There are two sets
    // depending upon the mode.
    //
    psStar = (ulPanel == PANEL_ACC) ? &g_sBtnAccStar : &g_sBtnPptStar;
    psNum = (ulPanel == PANEL_ACC) ? &g_sBtnAccNum : &g_sBtnPptNum;
    psUp = (ulPanel == PANEL_ACC) ? &g_sBtnAccUp : &g_sBtnPptUp;

    //
    // Set the background color of each widget according to whether the
    // relevant watch button is pressed or released.
    //
    CanvasImageSet(psUp, (ucBtnState & BUTTON_BIT_UP) ?
                   g_pucRedCarat30x30Image : g_pucGreyCarat30x30Image);
    CanvasImageSet(psNum, (ucBtnState & BUTTON_BIT_NUM) ?
                   g_pucRedNum30x30Image : g_pucGreyNum30x30Image);
    CanvasImageSet(psStar, (ucBtnState & BUTTON_BIT_STAR) ?
                   g_pucRedStar30x30Image : g_pucGreyStar30x30Image);

    //
    // Repaint the widgets.
    //
    UpdateDisplay((tWidget *)psUp, ulPanel);
    UpdateDisplay((tWidget *)psNum, ulPanel);
    UpdateDisplay((tWidget *)psStar, ulPanel);
}

//*****************************************************************************
//
// Update the display widgets showing the current accelerometer readings.
//
//*****************************************************************************
static void
UpdateAccelDisplay(unsigned long ulPanel, unsigned long ulIndex)
{
    unsigned long ulLoop;

    for(ulLoop = 0; ulLoop < 3; ulLoop++)
    {
        //
        // Write the value as a decimal number into the relevant buffer.  We
        // apply the offset here assuming it has been set by the user.
        //
        usnprintf(g_pcAccStrings[ulLoop], MAX_DATA_STRING_LEN, "%d",
                  (g_sLinkInfo[ulIndex].sAccel[ulLoop] -
                   g_sLinkInfo[ulIndex].sAccelOffset[ulLoop]));
    }

    //
    // Tell the widget library to repaint all the indicator fields if the
    // display is currently showing them.  This will also update the canvas
    // used to show the accelerometer "scribble" since it is also a child of
    // g_sIndicators.
    //
    UpdateDisplay((tWidget *)&g_sIndicators, ulPanel);
}

//*****************************************************************************
//
// The handler called to process packets containing button and, optionally,
// accelerometer data.
//
//*****************************************************************************
static void
ProcessAccPacket(unsigned long ulIndex, uint8_t *pucMsg)
{
    unsigned long ulLoop, ulPanel;

    //
    // Which mode is the watch in?  We tell this from the first byte of
    // the packet.
    //
    g_sLinkInfo[ulIndex].ucMode = pucMsg[0] & SIMPLICITI_EVENT_MASK;

    //
    // Make sure the mode is valid.  Ignore the packet if not.
    //
    if((g_sLinkInfo[ulIndex].ucMode != SIMPLICITI_MOUSE_EVENTS) &&
       (g_sLinkInfo[ulIndex].ucMode != SIMPLICITI_KEY_EVENTS))
    {
        return;
    }

    //
    // Make sure the display is in the correct mode.
    //
    ulPanel = ((g_sLinkInfo[ulIndex].ucMode == SIMPLICITI_MOUSE_EVENTS) ?
                    PANEL_ACC : PANEL_PPT);
    ChangeDisplayPanel(ulPanel);

    //
    // Has any button been pressed?
    //
    if((pucMsg[0] & PACKET_BTN_MASK))
    {
        //
        // Yes, a button has been pressed.
        //
        g_sLinkInfo[ulIndex].ucButtons = BUTTON_BIT(pucMsg[0]);

        //
        // Update the button state on the display.
        //
        UpdateButtonDisplay(ulPanel, g_sLinkInfo[ulIndex].ucButtons);

        //
        // Set a 500mS timer that will be used to clear the button indicators.
        // The protocol used by the watch gives us no notification when a
        // button is released.
        //
        g_ulClearBtnTimer = g_ulSysTickCount + (TICKS_PER_SECOND / 2);
    }

    //
    // If this packet contains accelerometer data, update it.
    //
    if(g_sLinkInfo[ulIndex].ucMode == SIMPLICITI_MOUSE_EVENTS)
    {
        //
        // Update our copy of the acceleration values.
        //
        for(ulLoop = 0; ulLoop < 3; ulLoop++)
        {
            //
            // Update the acceleration stored.  We use a simple filter to
            // smooth the accelerometer readings.
            //
#ifndef USE_UNFILTERED_ACCEL_VALUES
            g_sLinkInfo[ulIndex].sAccel[ulLoop] =
                ((g_sLinkInfo[ulIndex].sAccel[ulLoop] * 3) / 4) +
                (((short)((signed char)pucMsg[ulLoop + 1])) / 4);
//                (short)(*(signed char *)(pucMsg + (ulLoop + 1)) / 4);
#else
            g_sLinkInfo[ulIndex].sAccel[ulLoop] =
                ((short)((signed char)pucMsg[ulLoop + 1]));
#endif
        }

        //
        // Update the display using the new acceleration values.
        //
        UpdateAccelDisplay(PANEL_ACC, ulIndex);
    }
}

//*****************************************************************************
//
// The handler called to process packets containing status information.  Note
// that this function can generate enough widget paint messages that the
// message queue overflows (it's only 16 entries deep) so we must process
// the queue partway through the function to ensure that this does not happen.
//
//*****************************************************************************
static void
ProcessStatusPacket(unsigned long ulIndex, uint8_t *pucMsg)
{
    tBoolean bMetric;
    unsigned char ucHours, ucMinutes, ucSeconds, ucMonth, ucDay;
    unsigned char ucAlarmMinutes, ucAlarmHours;
    unsigned short usYear;
    short sTemperature, sAltitude;
    long lTemperature, lAltitude;

    //
    // Parse the received packet into our local structure
    //
    bMetric = (pucMsg[1] & 0x80) ? true : false;
    ucHours = pucMsg[1] & 0x3F;
    ucMinutes = pucMsg[2];
    ucSeconds = pucMsg[3];
    usYear = (pucMsg[4] << 8) | pucMsg[5];
    ucMonth = pucMsg[6];
    ucDay = pucMsg[7];
    ucAlarmHours = pucMsg[8];
    ucAlarmMinutes = pucMsg[9];
    sTemperature = (pucMsg[10] << 8) | pucMsg[11];
    sAltitude = (pucMsg[12] << 8) | pucMsg[13];

    //
    // Now check to see what has changed and update fields on the display as
    // required.  First we consider the fields relating to the time of day.
    //
    if(ucSeconds != g_sLinkInfo[ulIndex].ucSeconds)
    {
        usnprintf(g_pcSeconds, 3, "%02d", ucSeconds);
        UpdateDisplay((tWidget *)&g_sSeconds, g_sLinkInfo[ulIndex].ucMode);
    }
    if(ucMinutes != g_sLinkInfo[ulIndex].ucMinutes)
    {
        usnprintf(g_pcMinutes, 3, "%02d", ucMinutes);
        UpdateDisplay((tWidget *)&g_sMinutes, g_sLinkInfo[ulIndex].ucMode);
    }
    if((ucHours != g_sLinkInfo[ulIndex].ucHours) ||
       (bMetric != g_sLinkInfo[ulIndex].bMetric))
    {
        //
        // Are we displaying in 24 hour format?
        //
        if(bMetric)
        {
            //
            // Yes - display the hour number as it is and clear the am/pm
            // indicator.
            //
            usnprintf(g_pcHours, 3, "%2d", ucHours);
            usnprintf(g_pcAmPm, 3, "");
        }
        else
        {
            //
            // No - we are in 12 hour format so set the hour and am/pm
            // indicator appropriately.  The rather nasty nested tertiary
            // operator here says 'If the hour is 0, display "12", else if the
            // hour is over 12, display "hour - 12", else display "hour"'
            //
            usnprintf(g_pcHours, 3, "%2d", ((ucHours == 0) ? 12 :
                                           ((ucHours > 12) ? (ucHours - 12) :
                                            ucHours)));
            //
            // Set the am/pm indicator.
            //
            usnprintf(g_pcAmPm, 3, "%s", (ucHours < 12) ? "am" : "pm");
        }

        UpdateDisplay((tWidget *)&g_sHours, g_sLinkInfo[ulIndex].ucMode);
    }

    //
    // Now see if the date has changed and, if so, update the display.
    //
    if((ucDay != g_sLinkInfo[ulIndex].ucDay) ||
       (ucMonth != g_sLinkInfo[ulIndex].ucMonth))
    {
        usnprintf(g_pcDate, MAX_DATE_LEN, "%d %s", ucDay,
                  ((ucMonth && (ucMonth <= NUM_MONTHS)) ?
                   g_pcMonths[ucMonth - 1] : "ERROR!"));
        UpdateDisplay((tWidget *)&g_sDate, g_sLinkInfo[ulIndex].ucMode);
    }

    //
    // Did the year change?
    //
    if(usYear != g_sLinkInfo[ulIndex].usYear)
    {
        usnprintf(g_pcYear, 6, "%d", usYear);
        UpdateDisplay((tWidget *)&g_sYear, g_sLinkInfo[ulIndex].ucMode);
    }

    //
    // Clear out any widget messages in the queue so that we don't overflow.
    //
    WidgetMessageQueueProcess();

    //
    // Now consider the alarm time.
    //
    if((ucAlarmHours != g_sLinkInfo[ulIndex].ucAlarmHours) ||
       (ucAlarmMinutes != g_sLinkInfo[ulIndex].ucAlarmMinutes) ||
       (bMetric != g_sLinkInfo[ulIndex].bMetric))
    {
        //
        // Are we displaying in 24 hour format?
        //
        if(bMetric)
        {
            //
            // Yes - display the hour number as it is and clear the am/pm
            // indicator.
            //
            usnprintf(g_pcAlarmTime, 10, "%2d:%02d", ucAlarmHours,
                      ucAlarmMinutes);
        }
        else
        {
            //
            // No - we are in 12 hour format so set the hour and am/pm
            // indicator appropriately.  The rather nasty nested tertiary
            // operator here says 'If the hour is 0, display "12", else if the
            // hour is over 12, display "hour - 12", else display "hour"'
            //
            usnprintf(g_pcAlarmTime, 10, "%2d:%02d:%s",
                      ((ucAlarmHours == 0) ? 12 :
                      ((ucAlarmHours > 12) ? (ucAlarmHours - 12) :
                      ucAlarmHours)), ucAlarmMinutes,
                      (ucAlarmHours < 12) ? "am" : "pm");
        }

        UpdateDisplay((tWidget *)&g_sAlarmTime, g_sLinkInfo[ulIndex].ucMode);
    }

    //
    // Update the altitude display.
    //
    if((sAltitude != g_sLinkInfo[ulIndex].sAltitude) ||
       (bMetric != g_sLinkInfo[ulIndex].bMetric))
    {
        //
        // Are we to display metric or imperial values?
        //
        if(bMetric)
        {
            //
            // No conversion necessary - the watch always sends us metres.
            //
            lAltitude = (long)sAltitude;
        }
        else
        {
            //
            // Convert from metres to feet.  One metre is 3.25 feet.
            //
            lAltitude = ((long)sAltitude * 325) / 100;
        }

        usnprintf(g_pcAltitude, 8, "%d%s", lAltitude,
                  bMetric ? "m" : "ft");
        UpdateDisplay((tWidget *)&g_sAltitudeValue,
                      g_sLinkInfo[ulIndex].ucMode);
    }

    //
    // Update the temperature display.
    //
    if((sTemperature != g_sLinkInfo[ulIndex].sTemperature) ||
        (bMetric != g_sLinkInfo[ulIndex].bMetric))
    {
        //
        // Are we to display metric or imperial values?
        //
        if(bMetric)
        {
            //
            // No conversion necessary - the value the watch sends us is scaled
            // as tenths of a Celcius degree.
            //
            lTemperature = (long)sTemperature;
        }
        else
        {
            //
            // Convert from Celcius to Farenheit, remembering that we receive
            // data as tenths of a Celcius degree.
            //
            lTemperature = (((long)sTemperature * 9) / 5) + 320;
        }
        usnprintf(g_pcTemperature, 8, "%3d.%1d%s", lTemperature / 10,
                  lTemperature - ((lTemperature / 10) * 10),
                  bMetric ? "C" : "F");
        UpdateDisplay((tWidget *)&g_sTemperatureValue,
                      g_sLinkInfo[ulIndex].ucMode);
    }

    //
    // Update the text on the format switch button if necessary.
    //
    if(bMetric != g_sLinkInfo[ulIndex].bMetric)
    {
        //
        // Set the text to reflect the mode that will be used when the button
        // is pressed rather than the current mode.
        //
        ImageButtonTextSet(&g_sFormatBtn, bMetric ? "Imperial" : "Metric");
        UpdateDisplay((tWidget *)&g_sFormatBtn, g_sLinkInfo[ulIndex].ucMode);
    }

    //
    // Update our global status now that we've finished checking for
    // changes.
    //
    g_sLinkInfo[ulIndex].bMetric = bMetric;
    g_sLinkInfo[ulIndex].ucHours = ucHours;
    g_sLinkInfo[ulIndex].ucMinutes = ucMinutes;
    g_sLinkInfo[ulIndex].ucSeconds = ucSeconds;
    g_sLinkInfo[ulIndex].usYear = usYear;
    g_sLinkInfo[ulIndex].sTemperature = sTemperature;
    g_sLinkInfo[ulIndex].sAltitude = sAltitude;
    g_sLinkInfo[ulIndex].ucMonth = ucMonth;
    g_sLinkInfo[ulIndex].ucDay = ucDay;
    g_sLinkInfo[ulIndex].ucAlarmHours = ucAlarmHours;
    g_sLinkInfo[ulIndex].ucAlarmMinutes = ucAlarmMinutes;
}

//*****************************************************************************
//
// The handler called to process "ready to receive" packets.
//
//*****************************************************************************
static void
ProcessR2RPacket(unsigned long ulIndex, uint8_t *pucMsg)
{
    unsigned char ucPacket[STATUS_PACKET_SIZE];

    //
    // Set the new mode
    //
    g_sLinkInfo[ulIndex].ucMode = PANEL_SYNC;

    //
    // Ensure that the display is showing the correct panel since we know we
    // must be in Sync mode for this packet to be received.
    //
    ChangeDisplayPanel(PANEL_SYNC);

    //
    // In response to an R2R packet, we send a status request unless the flag
    // is set telling us to switch format in which case we sent a command to
    // set the watch parameters.
    //
    if(g_bSwitchFormat)
    {
        //
        // We have been asked to switch display format so build a packet
        // setting the watch parameters based on the last status we received
        // but with the format toggled.
        //
        ucPacket[0] = SYNC_AP_CMD_SET_WATCH;
        ucPacket[1] = (g_sLinkInfo[ulIndex].bMetric ? 0x00 : 0x80) |
                      (g_sLinkInfo[ulIndex].ucHours & 0x7F);
        ucPacket[2] = g_sLinkInfo[ulIndex].ucMinutes;
        ucPacket[3] = g_sLinkInfo[ulIndex].ucSeconds;
        ucPacket[4] = g_sLinkInfo[ulIndex].usYear >> 8;
        ucPacket[5] = g_sLinkInfo[ulIndex].usYear & 0x00FF;
        ucPacket[6] = g_sLinkInfo[ulIndex].ucMonth;
        ucPacket[7] = g_sLinkInfo[ulIndex].ucDay;
        ucPacket[8] = g_sLinkInfo[ulIndex].ucAlarmHours;
        ucPacket[9] = g_sLinkInfo[ulIndex].ucAlarmMinutes;
        ucPacket[10] = (g_sLinkInfo[ulIndex].sTemperature >> 8) & 0xFF;
        ucPacket[11] = g_sLinkInfo[ulIndex].sTemperature & 0x00FF;
        ucPacket[12] = (g_sLinkInfo[ulIndex].sAltitude >> 8) & 0xFF;
        ucPacket[13] = g_sLinkInfo[ulIndex].sAltitude & 0x00FF;

        //
        // Remember that we set the format already.
        //
        g_bSwitchFormat = false;

        //
        // Send the command to the watch.
        //
        SMPL_Send(g_sLinkInfo[ulIndex].sLinkID, ucPacket, STATUS_PACKET_SIZE);
    }
    else
    {
        //
        // We've not been asked to switch format so just request the current
        // status.
        //
        ucPacket[0] = SYNC_AP_CMD_GET_STATUS;
        ucPacket[1] = 0;
        SMPL_Send(g_sLinkInfo[ulIndex].sLinkID, ucPacket, 2);
    }
}

//*****************************************************************************
//
// The main loop calls this function to process received SimpliciTI messages.
// In this case, there are three different packet types (differentiated by
// length) that we are interested in.  This function merely sorts the packet
// into one of the three types and calls a handler for that packet type.
//
//*****************************************************************************
static void
ProcessMessage(unsigned long ulIndex, uint8_t *pucMsg, uint8_t ucLen)
{
    //
    // Determine which type of packet we received and call the appropriate
    // handler.
    //
    switch(ucLen)
    {
        //
        // 4 byte packets sent when the watch is in Acc or Ppt mode.
        //
        case ACC_PACKET_SIZE:
        {
            ProcessAccPacket(ulIndex, pucMsg);
            break;
        }

        //
        // 19 byte packets sent in response to a status query.
        //
        case STATUS_PACKET_SIZE:
        {
            ProcessStatusPacket(ulIndex, pucMsg);
            break;
        }

        //
        // 2 byte packets sent when the watch is in Sync mode and is ready
        // to receive a command.
        //
        case R2R_PACKET_SIZE:
        {
            ProcessR2RPacket(ulIndex, pucMsg);
            break;
        }

        //
        // Ignore all other packets since we don't know what to do with
        // them
        //
        default:
            break;
    }

    //
    // Since we received something, update the timer we use to determine when
    // communication has stopped and switch back to "Waiting" mode.
    //
    g_ulPacketResetTimer = g_ulSysTickCount +
                           (TICKS_PER_SECOND * PACKET_TIMEOUT_SECONDS);
}
