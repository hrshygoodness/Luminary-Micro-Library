//*****************************************************************************
//
// qs-keypad.c - Provides a virtual keypad on the screen to simulate an entry
//               door security system.
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
// This is part of revision 8555 of the RDK-IDM-L35 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "utils/ustdlib.h"
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"
#include "drivers/kitronix320x240x16_ssd2119.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "images.h"
#include "log.h"
#include "qs-keypad.h"
#include "random.h"
#include "relay.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Quickstart Security Keypad (qs-keypad)</h1>
//!
//! This application provides a security keypad to allow access to a door.
//! Upon entry of the correct access code, a call is made to a function which
//! could be used to set the state of a relay to activate an electric door
//! strike, unlocking the door.
//!
//! The screen is divided into three parts; the Texas Instruments banner across
//! the top, a hint across the bottom, and the main application area in the
//! middle (which is the only portion that should appear if this application
//! is used for a real door access system).  The hints provide an on-screen
//! guide to what the application is expecting at any given time.
//!
//! Upon startup, the screen is blank and the hint says to touch the screen.
//! Pressing the screen will bring up the keypad, which is randomized as an
//! added security measure (so that an observer can not ``steal'' the access
//! code by simply looking at the relative positions of the button presses).
//! The current access code is provided in the hint at the bottom of the screen
//! (which is clearly not secure).
//!
//! If an incorrect access code is entered (``#'' ends the code entry), then
//! the screen will go blank and wait for another access attempt.  If the
//! correct access code is entered, the ``relay'' will be toggled for a few
//! seconds (as indicated by the hint at the bottom stating that the door is
//! open) and the screen will go blank.  Once the door is closed again, the
//! screen can be touched again to repeat the process.
//!
//! The UART is used to output a log of events.  Each event in the log is time
//! stamped, with the arbitrary date of February 26, 2008 at 14:00 UT
//! (universal time) being the starting time when the application is run.  The
//! following events are logged:
//!
//! - The start of the application
//! - The access code being changed
//! - Access being granted (correct access code being entered)
//! - Access being denied (incorrect access code being entered)
//! - The door being relocked after access has been granted
//!
//! Input received by the application via the UART is interpreted as various
//! commands which can be used to query the current access code, set a new code,
//! show some statistics regarding the number of successful and unsuccessful
//! access attempts and initiate a firmware update.
//!
//! When the access code is changed, if a micro-SD card is present, the new
//! code will be stored in a file called ``key.txt'' in the root directory.
//! This file is read at startup to initialize the access code.  If a micro-SD
//! card is not present, or the ``key.txt'' file does not exist, the access
//! code defaults to 6918.
//!
//! If ``**'' is entered on the numeric keypad, the application provides
//! a demonstration of the Stellaris Graphics Library with various panels
//! showing the available widget type and graphics primitives.  Navigate
//! between the panels using buttons marked ``+'' and ``-'' at the bottom of
//! the screen and return to keypad mode by pressing the ``X'' buttons
//! which appear when you are on either the first or last demonstration
//! panel.
//!
//! UART0, which is connected to the 3 pin header on the underside of the
//! IDM-L35 RDK board (J3), is configured for 115,200 bits per second, and
//! 8-n-1 mode.  When the program is started a message will be printed to the
//! terminal.  Type ``help'' for command help.
//!
//! This application supports remote software update via the serial interface
//! using the LM Flash Programmer application.  A firmware update is initiated
//! either by entering ``*0'' on the numeric keypad or entering command
//! ``swupd'' on the serial terminal command line.
//!
//! UART0, which is connected to the 6 pin header on the underside of the
//! IDM-L35 RDK board (J8), is configured for 115200bps, and 8-n-1 mode.  The
//! USB-to-serial cable supplied with the IDM-L35 RDK may be used to connect
//! this TTL-level UART to the host PC to allow firmware update.
//
//*****************************************************************************

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
// The sound effect that is played when the keypad is turned on.
//
//*****************************************************************************
static const unsigned short g_pusKeypadOn[] =
{
    0, C6,
    20, E6,
    40, G6,
    60, C7,
    80, SILENCE
};

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
// The sound effect that is played when the correct code is entered.
//
//*****************************************************************************
static const unsigned short g_pusAccessGranted[] =
{
    0, G6,
    20, SILENCE,
    25, G6,
    45, SILENCE,
    50, G6,
    70, SILENCE,
    75, G6,
    95, SILENCE
};

//*****************************************************************************
//
// The sound effect that is played when the wrong code is entered, or the
// kaypad times out.
//
//*****************************************************************************
static const unsigned short g_pusAccessDenied[] =
{
    0, C7,
    20, G6,
    40, E6,
    60, C6,
    80, SILENCE
};

//*****************************************************************************
//
// A buffer in RAM to hold the palette from the image of the stylized Texas
// Instruments name.  This is used to adjust the palette in the image in order
// to fade the image from black.
//
//*****************************************************************************
unsigned char g_pucPalette[16 * 3];

//*****************************************************************************
//
// A set of flags used by the application.  FLAG_SYSTICK_INT is set each time
// a systick interrupt occurs.  FLAG_HINT_SEC is toggled after each second
// passes; each pair of toggles results in the hint at the bottom changing.
//
//*****************************************************************************
unsigned long g_ulFlags;
#define FLAG_SYSTICK_INT        0
#define FLAG_HINT_SEC           1

//*****************************************************************************
//
// The filesystem structure for the SD card.
//
//*****************************************************************************
FATFS g_sFatFs;

//*****************************************************************************
//
// The file object for the current opened file, which will be key.txt for
// reading during startup, or for writing if the access code is changed.
//
//*****************************************************************************
FIL g_sFileObject;

//*****************************************************************************
//
// When this counter reaches zero, the keypad is removed from the screen and an
// access denied log entry is output.
//
//*****************************************************************************
unsigned long g_ulKeypadTimeout;

//*****************************************************************************
//
// When this counter reaches zero, the relay is disabled causing the door to
// relock.
//
//*****************************************************************************
unsigned long g_ulRelayTimeout;

//*****************************************************************************
//
// Counters which keep track of the number of successful and unsuccessful
// attempts to enter the door unlock code.
//
//*****************************************************************************
unsigned long g_ulAllowedCount = 0;
unsigned long g_ulDeniedCount = 0;

//*****************************************************************************
//
// The hint that indicates how to get started with the application.
//
//*****************************************************************************
const char g_pcHintStart[] = "     Hint: Touch the screen to start     ";

//*****************************************************************************
//
// The hint providing the current access code.  The code in this string is
// replaced with the actual access code if one is found on the SD card, or if
// the access code is changed via the Ethernet interface.
//
//*****************************************************************************
char g_pcHintCode[] = "         Hint: The code is 6918#         ";

//*****************************************************************************
//
// The hint that indicates that access has been granted and the door is open.
//
//*****************************************************************************
const char g_pcHintEnter[] = "         Hint: The door is open         ";

//*****************************************************************************
//
// The hint string which informs the user that a software update is pending.
//
//*****************************************************************************
char g_pcHintUpdate[] = "     Waiting for firmware update...     ";

//*****************************************************************************
//
// The hint string which displays the application title.
//
//*****************************************************************************
char g_pcHintTitle[] =  "      Door Security Keypad Example      ";

//*****************************************************************************
//
// This array holds all the hint strings in the order that they will be
// displayed.  The display cycles through these, changing every 2 seconds.
//
//*****************************************************************************
#define NUM_HINT_STRINGS 2
const char *g_pcHintStrings[NUM_HINT_STRINGS] =
{
    g_pcHintStart,
    g_pcHintTitle
};

//*****************************************************************************
//
// The index of the g_pcHintStrings entry which is changed depending upon the
// mode of the application.
//
//*****************************************************************************
#define INFO_HINT_INDEX 0

//*****************************************************************************
//
// The index of the currently displayed hint string as found in array
// g_pcHintStrings.
//
//*****************************************************************************
unsigned char g_ucHintIndex = 0;

//*****************************************************************************
//
// The current access code.
//
//*****************************************************************************
unsigned long g_ulAccessCode = 0x6918;

//*****************************************************************************
//
// The current code that has been entered via the keypad.
//
//*****************************************************************************
unsigned long g_ulCode;

//*****************************************************************************
//
// The number of seconds since January 1, 1970, 12:00am UT.  This is
// initialized to the arbitrary date of February 26, 2008 at 2:00pm UT.
//
//*****************************************************************************
unsigned long g_ulTime = 1204034400;

//*****************************************************************************
//
// The number of milliseconds that have passed since the last second update.
//
//*****************************************************************************
unsigned long g_ulTimeCount;

//*****************************************************************************
//
// The current mode of the application.  Mode 0 is when the door is locked and
// the display is blank, mode 1 is when the keypad is being displayed, and mode
// 2 is when the door is unlocked and the display is blank.  A transition is
// made from mode 0 to mode 1 when the screen is touched, from mode 1 to mode 0
// when the wrong access code is entered or the keypad timeout has expired,
// from mode 1 to mode 2 when the correct access code is entered, and from mode
// 2 to mode 0 after the relay timeout has expired.  Mode 3 is entered when the
// user presses "**" on the keypad. When the graphics demo exits, mode 1 is
// reinstated.
//
//*****************************************************************************
unsigned long g_ulMode = 0;

//*****************************************************************************
//
// A flag indicating whether or not the last button pressed was the "*".  This
// is used as an escape to perform a couple of tasks - forcing a firmware
// update and starting the graphics library demo.
//
//*****************************************************************************
tBoolean g_bLastWasStar = false;

//*****************************************************************************
//
// A flag indicating that a firmware update request has been received.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
// The number of buttons in the keypad.
//
//*****************************************************************************
#define NUM_BUTTONS 12

//*****************************************************************************
//
// The mapping of button indices to their location on the display.  This is the
// randomized placements of the buttons.
//
//*****************************************************************************
unsigned char g_pucButtonMap[NUM_BUTTONS];

//*****************************************************************************
//
// The labels that are displayed on the keys on the keypad.
//
//*****************************************************************************
const char * const g_ppcLabels[NUM_BUTTONS] =
{
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "*",
    "#"
};

//*****************************************************************************
//
// The indices of the non-numeric buttons on the keypad
//
//*****************************************************************************
#define BTN_INDEX_STAR  10
#define BTN_INDEX_POUND 11

//*****************************************************************************
//
// A set of push button widgets, one per key on the keypad.
//
//*****************************************************************************
tPushButtonWidget g_pPB[NUM_BUTTONS];

//*****************************************************************************
//
// Forward declarations for the globals required to statically instantiate the
// compile-time portion of the widget tree.
//
//*****************************************************************************
void OnClick(tWidget *pWidget);
extern tPushButtonWidget g_sBackground;
extern tCanvasWidget g_sBlackBackground;

//*****************************************************************************
//
// A canvas widget across the bottom of the screen for use in providing hints
// to make the application easier to use.
//
//*****************************************************************************
Canvas(g_sHelp, &g_sBackground, 0, 0, &g_sKitronix320x240x16_SSD2119, 0, 220,
       320, 20, CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack,
       ClrBlack, ClrOrange, g_pFontCmss18i, g_pcHintStart, 0, 0);

//*****************************************************************************
//
// A canvas widget above the help text to provide a dividing line between the
// hint text and main display.
//
//*****************************************************************************
Canvas(g_sLine2, &g_sBackground, &g_sHelp, 0, &g_sKitronix320x240x16_SSD2119, 0,
       219, 320, 1, CANVAS_STYLE_FILL, ClrSilver, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// A canvas widget below the logo at the top to provide a dividing line between
// the logo and the main display.
//
//*****************************************************************************
Canvas(g_sLine1, &g_sBackground, &g_sLine2, 0, &g_sKitronix320x240x16_SSD2119,
       0, 20, 320, 1, CANVAS_STYLE_FILL, ClrSilver, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// A canvas widget across the top of the screen to display the Texas
// Instruments logo.
//
//*****************************************************************************
Canvas(g_sLogo, &g_sBackground, &g_sLine1, 0, &g_sKitronix320x240x16_SSD2119, 0,
       4, 320, 13, CANVAS_STYLE_IMG, 0, 0, 0, 0, 0,
       g_pucTIName, 0);

//*****************************************************************************
//
// A background push button that encompasses the entire display, to catch any
// press on the display and to ensure that the screen is cleared when the
// whole widget tree is redrawn.
//
//*****************************************************************************
RectangularButton(g_sBackground, &g_sBlackBackground, 0, &g_sLogo,
                  &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 240, 0, 0, 0, 0,
                  0, 0, 0, 0, 0, 0, 0, OnClick);

//*****************************************************************************
//
// A canvas widget that covers the entire display. This acts to fill the screen
// with black when we switch to and from the demo. We can't just make the
// background black since this would cause the screen to be cleared every time
// you pressed on an area that didn't contain another widget which responded
// to pointer clicks.
//
//*****************************************************************************
Canvas(g_sBlackBackground, 0, 0, &g_sBackground, &g_sKitronix320x240x16_SSD2119,
       0, 0, 320, 240, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// This function is called whenever the screen is pressed.
//
//*****************************************************************************
void
OnClick(tWidget *pWidget)
{
    unsigned long ulIdx, ulRand, ulTemp;

    //
    // Add the current time to the random number entropy pool.
    //
    RandomAddEntropy((g_ulTime * 1000) + g_ulTimeCount);

    //
    // See if the display is in locked mode.
    //
    if(g_ulMode == MODE_LOCKED)
    {
        //
        // Produce a new random seed from the entropy pool.
        //
        RandomSeed();

        //
        // Reset the button mapping to an identity map.
        //
        for(ulIdx = 0; ulIdx < NUM_BUTTONS; ulIdx++)
        {
            g_pucButtonMap[ulIdx] = ulIdx;
        }

        //
        // Loop through all but the last button in the button map.
        //
        for(ulIdx = 0; ulIdx < (NUM_BUTTONS - 1); ulIdx++)
        {
            //
            // Select a random number between the next button and the end of
            // the button map.
            //
            ulRand = (((RandomNumber() >> 16) * (11 - ulIdx)) >> 16) + 1;

            //
            // Swap this button with the selected button, randomizing the
            // keypad.
            //
            ulTemp = g_pucButtonMap[ulIdx];
            g_pucButtonMap[ulIdx] = g_pucButtonMap[ulIdx + ulRand];
            g_pucButtonMap[ulIdx + ulRand] = ulTemp;
        }

        //
        // Loop through the twelve buttons.
        //
        for(ulIdx = 0; ulIdx < NUM_BUTTONS; ulIdx++)
        {
            //
            // Set the button text based on the randomized button map.
            //
            PushButtonTextSet(g_pPB + ulIdx,
                              g_ppcLabels[g_pucButtonMap[ulIdx]]);

            //
            // Turn on the image and text for this button, and turn off the
            // fill.
            //
            PushButtonImageOn(g_pPB + ulIdx);
            PushButtonTextOn(g_pPB + ulIdx);
            PushButtonFillOff(g_pPB + ulIdx);
        }

        //
        // Set the application hint to the access code hint.
        //
        g_pcHintStrings[INFO_HINT_INDEX] = g_pcHintCode;

        //
        // If the application information hint is currently being displayed,
        // change the string immediately.
        //
        if(g_ucHintIndex == INFO_HINT_INDEX)
        {
            CanvasTextSet(&g_sHelp, g_pcHintStrings[INFO_HINT_INDEX]);
        }

        //
        // Set the keypad timeout to the maximum value.
        //
        g_ulKeypadTimeout = KEYPAD_TIMEOUT;

        //
        // Change to keypad mode.
        //
        g_ulMode = MODE_KEYPAD;

        //
        // Clear the flag telling us whether or not the "*" key was the last
        // one pressed.
        //
        g_bLastWasStar = false;

        //
        // Reset the entered code to zero.
        //
        g_ulCode = 0;

        //
        // Redraw the entire display.
        //
        WidgetPaint((tWidget *)&g_sBlackBackground);

        //
        // Play the sound effect associated with enabling the keypad.
        //
        SoundPlay(g_pusKeypadOn, sizeof(g_pusKeypadOn) / 2);
    }

    //
    // Otherwise, see if the display is in keypad mode.
    //
    else if(g_ulMode == MODE_KEYPAD)
    {
        //
        // A key was pressed, so reset the keypad timeout to the maximum value.
        //
        g_ulKeypadTimeout = KEYPAD_TIMEOUT;

        //
        // Loop through the twelve push buttons.
        //
        for(ulIdx = 0; ulIdx < NUM_BUTTONS; ulIdx++)
        {
            //
            // See if the widget being pressed is this push button.
            //
            if(pWidget == (tWidget *)(g_pPB + ulIdx))
            {
                break;
            }
        }

        //
        // See if a push button was found.
        //
        if(ulIdx != NUM_BUTTONS)
        {
            //
            // If star was pressed last, check to see if the new key
            // corresponds to any of the special function escapes.
            //
            if(g_bLastWasStar)
            {
                switch(g_pucButtonMap[ulIdx])
                {
                    //
                    // Someone pressed "**".  This is used to switch to the
                    // graphics driver demo mode.
                    //
                    case BTN_INDEX_STAR:
                    {
                        //
                        // Play the sound effect associated with a key press.
                        //
                        SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);

                        //
                        // Fix up the widget tree to show the graphics demo.
                        //
                        GraphicsDemoShow();

                        break;
                    }

                    //
                    // Someone pressed "*0".  This is used to initiate a
                    // firmware update via the bootloader.
                    //
                    case 0:
                    {
                        //
                        // Play the sound effect associated with a key press.
                        //
                        SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);

                        //
                        // Signal the main loop to start the update
                        //
                        g_bFirmwareUpdate = true;
                    }

                    default:
                        break;
                }

                //
                // We've handled the special functions (if required) so clear
                // the flag that we use to remember if star was previously
                // pressed.
                //
                g_bLastWasStar = false;

                //
                // We are done handling this key.
                //
                return;
            }

            //
            // See if the '#' button was pressed.
            //
            if(g_pucButtonMap[ulIdx] == BTN_INDEX_POUND)
            {
                //
                // Set the keypad timeout to 1 so that the display will revert
                // on the next tick.
                //
                g_ulKeypadTimeout = 1;

                //
                // See if the access code matches.
                //
                if((g_ulCode & 0x0000ffff) == g_ulAccessCode)
                {
                    //
                    // Change to unlocked mode.
                    //
                    g_ulMode = MODE_UNLOCKED;

                    //
                    // Set the application hint to the entry hint.
                    //
                    g_pcHintStrings[INFO_HINT_INDEX] = g_pcHintEnter;

                    //
                    // If the application information hint is currently being
                    // displayed change the string immediately.
                    //
                    if(g_ucHintIndex == INFO_HINT_INDEX)
                    {
                        CanvasTextSet(&g_sHelp,
                                      g_pcHintStrings[INFO_HINT_INDEX]);
                    }

                    //
                    // Set the relay timeout to the maximum value.
                    //
                    g_ulRelayTimeout = RELAY_TIMEOUT;

                    //
                    // Update the count of the number of successful unlocking
                    // attempts.
                    //
                    g_ulAllowedCount++;

                    //
                    // Open the "relay" to unlock the door.
                    //
                    RelayEnable();

                    //
                    // Play the sound effect associated with unlocking the
                    // door.
                    //
                    SoundPlay(g_pusAccessGranted,
                              sizeof(g_pusAccessGranted) / 2);

                    //
                    // Write an event to the log indicating that access has
                    // been granted.
                    //
                    LogWrite("Access granted");
                }
            }

            //
            // See if one of the digit buttons have been pressed.
            //
            else if(g_pucButtonMap[ulIdx] != BTN_INDEX_STAR)
            {
                //
                // Shift up the entered code and append the new digit.
                //
                g_ulCode <<= 4;
                g_ulCode |= g_pucButtonMap[ulIdx];

                //
                // Play the sound effect associated with a key press.
                //
                SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);
            }
            else
            {
                //
                // Otherwise, the '*' button has been pressed.  Remember this
                // for later.
                //
                g_bLastWasStar = true;

                //
                // Shift up the entered code.
                //
                g_ulCode <<= 4;

                //
                // Play the sound effect associated with a key press.
                //
                SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);
            }
        }
    }
}

//*****************************************************************************
//
// Handles the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    unsigned long ulIdx;

    //
    // Set the flag that indicates a SysTick interrupt has occurred.
    //
    HWREGBITW(&g_ulFlags, 0) = 1;

    //
    // Call the FatFs tick timer.
    //
    disk_timerproc();

    //
    // We only update the hint string and mode if the graphics demo is not
    // currently running.
    //
    if(g_ulMode != MODE_DEMO)
    {
        //
        // Increment the count of SysTick interrupts.
        //
        g_ulTimeCount += 1000 / TICKS_PER_SECOND;
        if(g_ulTimeCount >= 1000)
        {
            //
            // Increment the count of seconds and reset the millisecond counter.
            //
            g_ulTime++;
            g_ulTimeCount -= 1000;

            //
            // Toggle the hint second flag.
            //
            HWREGBITW(&g_ulFlags, FLAG_HINT_SEC) ^= 1;

            //
            // See if the hint second flag is clear.
            //
            if(!HWREGBITW(&g_ulFlags, FLAG_HINT_SEC))
            {
                //
                // Two seconds have passed, so cycle to the next hint string,
                // taking care to wrap if we are already displaying the last
                // string in the list.
                //
                g_ucHintIndex++;
                if(g_ucHintIndex == NUM_HINT_STRINGS)
                {
                    g_ucHintIndex = 0;
                }

                //
                // Set the new string in the relevant widget
                //
                CanvasTextSet(&g_sHelp, g_pcHintStrings[g_ucHintIndex]);

                //
                // Redraw the hint widget.
                //
                WidgetPaint((tWidget *)&g_sHelp);
            }
        }

        //
        // See if the keypad timeout is active.
        //
        if(g_ulKeypadTimeout != 0)
        {
            //
            // Decrement the keypad timeout.
            //
            g_ulKeypadTimeout--;

            //
            // See if the keypad timeout has reached zero.
            //
            if(g_ulKeypadTimeout == 0)
            {
                //
                // Loop over the buttons on the keypad.
                //
                for(ulIdx = 0; ulIdx < NUM_BUTTONS; ulIdx++)
                {
                    //
                    // Turn off the image and text for this button, and turn on
                    // the fill.  This will erase the button.
                    //
                    PushButtonImageOff(g_pPB + ulIdx);
                    PushButtonTextOff(g_pPB + ulIdx);
                    PushButtonFillOn(g_pPB + ulIdx);
                }

                //
                // See if the application is in unlocked mode.
                //
                if(g_ulMode != MODE_UNLOCKED)
                {
                    //
                    // Set the application hint to the startup hint.
                    //
                    g_pcHintStrings[INFO_HINT_INDEX] = g_pcHintStart;

                    //
                    // If the application information hint is currently being
                    // displayed, change the string immediately.
                    //
                    if(g_ucHintIndex == INFO_HINT_INDEX)
                    {
                        CanvasTextSet(&g_sHelp, g_pcHintStrings[INFO_HINT_INDEX]);
                    }

                    //
                    // Change to locked mode.
                    //
                    g_ulMode = MODE_LOCKED;

                    //
                    // Play the sound effect associated with access denial.
                    //
                    SoundPlay(g_pusAccessDenied, sizeof(g_pusAccessDenied) / 2);

                    //
                    // Update the count of the number of unsuccessful attempts.
                    //
                    g_ulDeniedCount++;

                    //
                    // Write an event to the log indicating that access has been
                    // denied.
                    //
                    LogWrite("Access denied");
                }

                //
                // Redraw the display.
                //
                WidgetPaint((tWidget *)&g_sBlackBackground);
            }
        }

        //
        // See if the relay timeout is active.
        //
        if(g_ulRelayTimeout != 0)
        {
            //
            // Derement the relay timeout.
            //
            g_ulRelayTimeout--;

            //
            // See if the relay timeout has reached zero.
            //
            if(g_ulRelayTimeout == 0)
            {
                //
                // Disable the "relay".
                //
                RelayDisable();

                //
                // Set the application hint to the startup hint.
                //
                g_pcHintStrings[INFO_HINT_INDEX] = g_pcHintStart;

                //
                // If the application information hint is currently being
                // displayed, change the string immediately.
                //
                if(g_ucHintIndex == INFO_HINT_INDEX)
                {
                    CanvasTextSet(&g_sHelp, g_pcHintStrings[INFO_HINT_INDEX]);
                    WidgetPaint((tWidget *)&g_sHelp);
                }

                //
                // Change to locked mode.
                //
                g_ulMode = MODE_LOCKED;

                //
                // Write an event to the log indicating that the door has been
                // locked.
                //
                LogWrite("Door locked");
            }
        }
    }
}

//*****************************************************************************
//
// Displays the Texas Instruments logo on the screen, fading it in and then
// sliding it to the top of the screen.
//
//*****************************************************************************
static void
DisplayLogo(void)
{
    long lIdx, lColor;
    tContext sContext;
    tRectangle sRect;
    unsigned short usWidth, usHeight;
    short sX, sY;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Get the size of the image.
    //
    usHeight = GrImageHeightGet(g_pucTIName);
    usWidth = GrImageWidthGet(g_pucTIName);

    //
    // Determine the X and Y coordinates that allow us to place the image
    // at the center of the screen.
    //
    sX = (GrContextDpyWidthGet(&sContext) - usWidth) / 2;
    sY = (GrContextDpyHeightGet(&sContext) - usHeight) / 2;

    //
    // Copy the original palette
    //
    for(lIdx = 0; lIdx < (GrImageColorsGet(g_pucTIName) * 3); lIdx++)
    {
        g_pucPalette[lIdx] = g_pucTIName[lIdx + 6];
    }

    //
    // Fade the Texas Instruments logo from black.
    //
    for(lIdx = 0; lIdx <= 256; lIdx += 4)
    {
        //
        // Adjust the colormap of the image, fading each color as appropriate.
        //
        for(lColor = 0; lColor < (GrImageColorsGet(g_pucTIName) * 3);
            lColor++)
        {
            g_pucTIName[lColor + 6] = (g_pucPalette[lColor] * lIdx) / 256;
        }

        //
        // Wait until the next SysTick interrupt.
        //
        while(HWREGBITW(&g_ulFlags, 0) == 0)
        {
        }

        //
        // Clear the SysTick interrupt flag.
        //
        HWREGBITW(&g_ulFlags, 0) = 0;

        //
        // Draw the image on the screen.
        //
        GrImageDraw(&sContext, g_pucTIName, sX, sY);
    }

    //
    // Delay for three seconds.
    //
    for(lIdx = 0; lIdx < (3 * TICKS_PER_SECOND); lIdx++)
    {
        //
        // Wait until the next SysTick interrupt.
        //
        while(HWREGBITW(&g_ulFlags, 0) == 0)
        {
        }

        //
        // Clear the SysTick interrupt flag.
        //
        HWREGBITW(&g_ulFlags, 0) = 0;
    }

    //
    // Set the foreground color in the drawing context to black.
    //
    GrContextForegroundSet(&sContext, ClrBlack);

    //
    // Slide the logo to the top of the screen, stopping at the location of the
    // logo widget.
    //
    for(lIdx = sY; lIdx >= 4; lIdx -= 2)
    {
        //
        // Wait until the next SysTick interrupt.
        //
        while(HWREGBITW(&g_ulFlags, 0) == 0)
        {
        }

        //
        // Clear the SysTick interrupt flag.
        //
        HWREGBITW(&g_ulFlags, 0) = 0;

        //
        // Draw the image on the screen.
        //
        GrImageDraw(&sContext, g_pucTIName, sX, lIdx);

        //
        // Fill the rows immediately below the image to erase the residual
        // image from the previous image location.
        //
        sRect.sXMin = sX;
        sRect.sYMin = lIdx + usHeight;
        sRect.sXMax = sX + usWidth;
        sRect.sYMax = sRect.sYMin + 1;
        GrRectFill(&sContext, &sRect);
    }
}

//*****************************************************************************
//
// Updates the access code in the application.
//
//*****************************************************************************
static void
UpdateAccessCode(unsigned long ulCode)
{
    //
    // Set the new access code.
    //
    g_ulAccessCode = ulCode & 0xffff;

    //
    // Place the access code into the access code hint.
    //
    g_pcHintCode[27] = '0' + ((ulCode >> 12) & 0xf);
    g_pcHintCode[28] = '0' + ((ulCode >> 8) & 0xf);
    g_pcHintCode[29] = '0' + ((ulCode >> 4) & 0xf);
    g_pcHintCode[30] = '0' + (ulCode & 0xf);

    //
    // If the display is showing the keypad), redraw the hint.
    //
    if(g_ulMode == MODE_KEYPAD)
    {
        WidgetPaint((tWidget*)&g_sHelp);
    }
}

//*****************************************************************************
//
// Reads the stored access code from the SD card (if present).
//
//*****************************************************************************
void
ReadAccessCode(void)
{
    unsigned char pucBuffer[8];
    unsigned short usCount;
    FIL sFile;

    //
    // Mount the SD card filesystem.
    //
    f_mount(0, &g_sFatFs);

    //
    // Attempt to open the file containing the access code.
    //
    if(f_open(&sFile, "/key.txt", FA_READ) == FR_OK)
    {
        //
        // Attempt to read the data from the file.
        //
        if(f_read(&sFile, pucBuffer, sizeof(pucBuffer) - 1, &usCount) == FR_OK)
        {
            //
            // Make sure that the access code is valid.
            //
            if((pucBuffer[0] >= '0') && (pucBuffer[0] <= '9') &&
               (pucBuffer[1] >= '0') && (pucBuffer[1] <= '9') &&
               (pucBuffer[2] >= '0') && (pucBuffer[2] <= '9') &&
               (pucBuffer[2] >= '0') && (pucBuffer[3] <= '9'))
            {
                //
                // Extract the access code from the file data.
                //
                usCount = (((pucBuffer[0] - '0') << 12) |
                           ((pucBuffer[1] - '0') << 8) |
                           ((pucBuffer[2] - '0') << 4) |
                           (pucBuffer[3] - '0'));

                //
                // Update the access code.
                //
                UpdateAccessCode(usCount);
            }
        }

        //
        // Close the file.
        //
        f_close(&sFile);
    }
}

//*****************************************************************************
//
// Changes the access code, saving it to a file on the SD card (if present).
//
//*****************************************************************************
void
SetAccessCode(unsigned long ulCode)
{
    unsigned short usCount;
    FIL sFile;

    //
    // Update the access code.
    //
    UpdateAccessCode(ulCode);

    //
    // Attempt to create the file to hold the access code.
    //
    if(f_open(&sFile, "/key.txt", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
    {
        //
        // Attempt to write the access code to the file.
        //
        if(f_write(&sFile, g_pcHintCode + 27, 4, &usCount) == FR_OK)
        {
            //
            // Synchronize the file with the disk, flushing any cached data.
            //
            f_sync(&sFile);
        }

        //
        // Close the file.
        //
        f_close(&sFile);
    }

    //
    // Write an event to the log indicating that the access code has changed.
    //
    LogWrite("Access code changed");
}

//*****************************************************************************
//
// Passes control to the bootloader and initiates a remote software update
// over the serial connection.
//
// This function passes control to the bootloader and initiates an update of
// the main application firmware image via UART0.
//
// \return Never returns.
//
//*****************************************************************************
void
JumpToBootLoader(void)
{
    //
    // Disable all processor interrupts.  Instead of disabling them
    // one at a time (and possibly missing an interrupt if new sources
    // are added), a direct write to NVIC is done to disable all
    // peripheral interrupts.
    //
    IntMasterDisable();
    SysTickIntDisable();
    HWREG(NVIC_DIS0) = 0xffffffff;
    HWREG(NVIC_DIS1) = 0xffffffff;

    //
    // Return control to the boot loader.  This is a call to the SVC
    // handler in the boot loader.
    //
    (*((void (*)(void))(*(unsigned long *)0x2c)))();
}

//*****************************************************************************
//
// A simple security keypad application.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulIdx;

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
    // Enable SysTick to provide a periodic interrupt.  This is used as a time
    // base for timed events.
    //
    SysTickPeriodSet(SysCtlClockGet() / TICKS_PER_SECOND);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Turn on the backlight.
    //
    Kitronix320x240x16_SSD2119BacklightOn(255);

    //
    // Display the Texas Instruments logo splash screen.
    //
    DisplayLogo();

    //
    // Initialize the logging interface.
    //
    LogInit();

    //
    // Initialize the "relay".  There is no physical relay on the IDM-L28
    // board, but the application toggles a GPIO pin that could be used to
    // control a relay.
    //
    RelayInit();

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
    // Read the initial access code from the SD card (if present).
    //
    ReadAccessCode();

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBlackBackground);

    //
    // Loop through the 12 push buttons on the keypad.
    //
    for(ulIdx = 0; ulIdx < NUM_BUTTONS; ulIdx++)
    {
        //
        // Initialize this push button.
        //
        RectangularButtonInit(g_pPB + ulIdx, &g_sKitronix320x240x16_SSD2119,
                              (80 * (ulIdx % 4)) + 5, (66 * (ulIdx / 4)) + 25,
                              70, 60);

        //
        // Set the properties of this push button.
        //
        PushButtonFillColorSet(g_pPB + ulIdx, ClrBlack);
        PushButtonTextColorSet(g_pPB + ulIdx, ClrSilver);
        PushButtonFontSet(g_pPB + ulIdx, g_pFontCmss48);
        PushButtonImageSet(g_pPB + ulIdx, g_pucBlue70x60);
        PushButtonImagePressedSet(g_pPB + ulIdx, g_pucBlue70x60Press);
        PushButtonCallbackSet(g_pPB + ulIdx, OnClick);
        PushButtonFillOn(g_pPB + ulIdx);

        //
        // Add this push button to the widget tree.
        //
        WidgetAdd((tWidget *)&g_sBackground, (tWidget *)(g_pPB + ulIdx));
    }

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Loop until the user asks for a firmware update.
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();

        //
        // Process any commands received via the serial port.
        //
        LogProcessCommands();
    }

    //
    // Turn off the system tick (to prevent the hint text
    // from changing).
    //
    SysTickIntDisable();

    //
    // Change the hint text message to indicate that a
    // firmware update is being requested.
    //
    CanvasTextSet(&g_sHelp, g_pcHintUpdate);
    WidgetPaint((tWidget *)&g_sHelp);

    //
    // Process outstanding widget messages.  This ensures that the previous
    // WidgetPaint order completes before we jump into the boot loader.
    //
    WidgetMessageQueueProcess();

    //
    // Now transfer control to the boot loader.
    //
    JumpToBootLoader();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
