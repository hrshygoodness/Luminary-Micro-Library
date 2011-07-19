//****************************************************************************
//
// usb_dev_keyboard.c - Main routines for the keyboard portion of the
// application.
//
// Copyright (c) 2010-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
//
//****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidkeyb.h"
#include "drivers/touch.h"
#include "usb_structs.h"
#include "usb_dev_caudiohid.h"

//****************************************************************************
//
// Notes about the virtual keyboard definition
//
// The virtual keyboard is defined in terms of rows of keys.  Each row of
// keys may be either a normal alphanumeric row in which all keys are the
// same size and handled in exactly the say way, or a row of "special keys"
// which may have different widths and which have a handler function defined
// for each key.  In the definition used here, g_psKeyboard[] contains 6 rows
// and defines the keyboard at the top level.
//
// The keyboard can be in 1 of 4 states defined by the current shift and
// caps lock state.  For alphanumeric rows, the row definition (tAlphaKeys)
// contain strings representing the key cap characters for each of the keys
// in each of the four states.  Function DrawVirtualKeyboard uses these
// strings and the current state to display the correct key caps.
//
//****************************************************************************

extern const tUSBDHIDKeyboardDevice g_sKeyboardDevice;

//****************************************************************************
//
// Hardware resources related to the LED we use to show the CAPSLOCK state.
//
//****************************************************************************
#define CAPSLOCK_GPIO_BASE      GPIO_PORTF_BASE
#define CAPSLOCK_GPIO_PIN       GPIO_PIN_3
#define CAPSLOCK_ACTIVE         CAPSLOCK_GPIO_PIN
#define CAPSLOCK_INACTIVE       0

//****************************************************************************
//
// The system tick timer period.
//
//****************************************************************************
#define SYSTICKS_PER_SECOND 100
#define SYSTICK_PERIOD_MS (1000 / SYSTICKS_PER_SECOND)

//****************************************************************************
//
// A structure describing special keys which are not handled the same way as
// all the alphanumeric keys.
//
//****************************************************************************
typedef struct
{
    //
    // The label string for the key.
    //
    const char *pcLabel;

    //
    // The width of the displayed key in pixels.
    //
    short sWidth;

    //
    // The usage code (if any) associated with this key.
    //
    char cUsageCode;

    //
    // A function to be called when the user presses or releases this key.
    //
    unsigned long (*pfnPressHandler)(short sRow, short sCol, tBoolean bPress);

    //
    // A function to be called to redraw the special key. If NULL, the
    // default redraw handler is used.
    //
    void (*pfnRedrawHandler)(short sCol, short sRow, tBoolean bFocus,
                             tBoolean bPressed, tBoolean bBorder);
} tSpecialKey;

//****************************************************************************
//
// A list of the states that the keyboard can be in.
//
//****************************************************************************
typedef enum
{
    //
    // Neither shift nor caps lock is active.
    //
    KEY_STATE_NORMAL,

    //
    // Shift is active, caps lock is not.
    //
    KEY_STATE_SHIFT,

    //
    // Shift is not active, caps lock is active.
    //
    KEY_STATE_CAPS,

    //
    // Both shift and caps lock are active.
    //
    KEY_STATE_BOTH,

    //
    // State counter member.
    //
    NUM_KEY_STATES
} tKeyState;

tKeyState g_eVirtualKeyState = KEY_STATE_NORMAL;

//****************************************************************************
//
// A structure describing typical alphanumeric keys.
//
//****************************************************************************
typedef struct
{
    //
    // Strings containing the unshifted, shifted and caps representations of
    // each of the keys in the row.
    //
    const char *pcKey[NUM_KEY_STATES];
    const char *pcUsageCodes;
} tAlphaKeys;

//****************************************************************************
//
// A structure describing a single row of the virtual keyboard.
//
//****************************************************************************
typedef struct
{
    //
    // Does this row consist of alphanumeric keys or special keys?
    //
    tBoolean bSpecial;

    //
    // Pointer to data describing this row of keys.  If bSpecial is true,
    // this points to an array of tSpecialKey structures.  If bSpecial is
    // false, it points to a single tAlphaKeys structure.
    //
    void *pvKeys;

    //
    // The number of keys in the row.
    //
    short sNumKeys;

    //
    // The horizontal offset to apply when drawing the characters in this
    // row to the screen.  This allows us to offset the rows slightly as they
    // would look on a normal keyboard.
    //
    short sLeftOffset;
} tRow;

//****************************************************************************
//
// Labels defining the layout of the virtual keyboard on the display.
//
//****************************************************************************
#define NUM_KEYBOARD_ROWS       6
#define KEYBOARD_TOP            40
#define KEYBOARD_KEY_WIDTH      26
#define KEYBOARD_KEY_HEIGHT     24
#define KEYBOARD_COL_SPACING    2
#define KEYBOARD_ROW_SPACING    4

#define KEYBOARD_CELL_WIDTH     (KEYBOARD_KEY_WIDTH + KEYBOARD_COL_SPACING)
#define KEYBOARD_CELL_HEIGHT    (KEYBOARD_KEY_HEIGHT + KEYBOARD_ROW_SPACING)

//****************************************************************************
//
// Colors used to draw various parts of the virtual keyboard.
//
//****************************************************************************
#define FOCUS_COLOR             ClrRed
#define BACKGROUND_COLOR        ClrBlack
#define HIGHLIGHT_COLOR         ClrWhite
#define SHADOW_COLOR            ClrGray
#define KEY_COLOR               0x00E0E0E0
#define KEY_BRIGHT_COLOR        0x00E0E000
#define HIGHLIGHT_BRIGHT_COLOR  ClrYellow
#define SHADOW_BRIGHT_COLOR     0x00808000
#define KEY_TEXT_COLOR          ClrBlack

//****************************************************************************
//
// Keys on the top row of the virtual keyboard.  Strings are defined showing
// the keycaps in unshifted, shifted and caps states.
//
//****************************************************************************
#define NUM_ROW0_KEYS 10

const char g_pcRow0UsageCodes[NUM_ROW0_KEYS] =
{
    HID_KEYB_USAGE_1, HID_KEYB_USAGE_2, HID_KEYB_USAGE_3, HID_KEYB_USAGE_4,
    HID_KEYB_USAGE_5, HID_KEYB_USAGE_6, HID_KEYB_USAGE_7, HID_KEYB_USAGE_8,
    HID_KEYB_USAGE_9, HID_KEYB_USAGE_0
};

const tAlphaKeys g_sRow0 =
{
    {"1234567890",  // Normal
     "!@#$%^&*()",  // Shift
     "1234567890",  // Caps
     "!@#$%^&*()"}, // Shift + Caps
    g_pcRow0UsageCodes
};

//****************************************************************************
//
// Keys on the second row of the virtual keyboard.  Strings are defined
// showing the keycaps in unshifted, shifted and caps states.
//
//****************************************************************************
#define NUM_ROW1_KEYS 10

const char g_pcRow1UsageCodes[NUM_ROW1_KEYS] =
{
    HID_KEYB_USAGE_Q, HID_KEYB_USAGE_W, HID_KEYB_USAGE_E, HID_KEYB_USAGE_R,
    HID_KEYB_USAGE_T, HID_KEYB_USAGE_Y, HID_KEYB_USAGE_U, HID_KEYB_USAGE_I,
    HID_KEYB_USAGE_O, HID_KEYB_USAGE_P
};

const tAlphaKeys g_sRow1 =
{
    {"qwertyuiop",   // Normal
     "QWERTYUIOP",   // Shift
     "QWERTYUIOP",   // Caps
     "qwertyuiop"},  // Shift + Caps
    g_pcRow1UsageCodes
};

//****************************************************************************
//
// Keys on the third row of the virtual keyboard.  Strings are defined showing
// the keycaps in unshifted, shifted and caps states.
//
//****************************************************************************
#define NUM_ROW2_KEYS 10

const char g_pcRow2UsageCodes[NUM_ROW2_KEYS] =
{
    HID_KEYB_USAGE_A, HID_KEYB_USAGE_S, HID_KEYB_USAGE_D, HID_KEYB_USAGE_F,
    HID_KEYB_USAGE_G, HID_KEYB_USAGE_H, HID_KEYB_USAGE_J, HID_KEYB_USAGE_K,
    HID_KEYB_USAGE_L, HID_KEYB_USAGE_SEMICOLON
};

const tAlphaKeys g_sRow2 =
{
    {"asdfghjkl;",  // Normal
     "ASDFGHJKL:",  // Shift
     "ASDFGHJKL;",  // Caps
     "asdfghjkl;"}, // Shift + Caps
    g_pcRow2UsageCodes
};

//****************************************************************************
//
// Keys on the fourth row of the virtual keyboard.  Strings are defined
// showing the keycaps in unshifted, shifted and caps states.
//
//****************************************************************************
#define NUM_ROW3_KEYS 10

const char g_pcRow3UsageCodes[NUM_ROW3_KEYS] =
{
 HID_KEYB_USAGE_Z, HID_KEYB_USAGE_X, HID_KEYB_USAGE_C, HID_KEYB_USAGE_V,
 HID_KEYB_USAGE_B, HID_KEYB_USAGE_N, HID_KEYB_USAGE_M, HID_KEYB_USAGE_COMMA,
 HID_KEYB_USAGE_PERIOD, HID_KEYB_USAGE_FSLASH
};

const tAlphaKeys g_sRow3 =
{
    {"zxcvbnm,./",   // Normal
     "ZXCVBNM<>?",   // Shift
     "ZXCVBNM,./",   // Caps
     "zxcvbnm<>?"},  // Shift + Caps
    g_pcRow3UsageCodes
};

//****************************************************************************
//
// Prototypes for special key handlers
//
//****************************************************************************
unsigned long CapsLockHandler(short sCol, short sRow, tBoolean bPress);
unsigned long ShiftLockHandler(short sCol, short sRow, tBoolean bPress);
unsigned long CtrlHandler(short sCol, short sRow, tBoolean bPress);
unsigned long AltHandler(short sCol, short sRow, tBoolean bPress);
unsigned long GUIHandler(short sCol, short sRow, tBoolean bPress);
unsigned long DefaultSpecialHandler(short sCol, short sRow, tBoolean bPress);
void CapsLockRedrawHandler(short sCol, short sRow, tBoolean bFocus,
                           tBoolean bPressed, tBoolean bBorder);
void ShiftLockRedrawHandler(short sCol, short sRow, tBoolean bFocus,
                            tBoolean bPressed, tBoolean bBorder);
void CtrlRedrawHandler(short sCol, short sRow, tBoolean bFocus,
                       tBoolean bPressed, tBoolean bBorder);
void AltRedrawHandler(short sCol, short sRow, tBoolean bFocus,
                      tBoolean bPressed, tBoolean bBorder);
void GUIRedrawHandler(short sCol, short sRow, tBoolean bFocus,
                      tBoolean bPressed, tBoolean bBorder);

//****************************************************************************
//
// The bottom 2 rows of the virtual keyboard contains special keys which are
// handled differently from the basic, alphanumeric keys.
//
//****************************************************************************
const tSpecialKey g_psRow4[] =
{
    {"Cap", 38, HID_KEYB_USAGE_CAPSLOCK, CapsLockHandler,
     CapsLockRedrawHandler},
    {"Shift", 54 , 0, ShiftLockHandler, ShiftLockRedrawHandler},
    {" ", 80, HID_KEYB_USAGE_SPACE, DefaultSpecialHandler, 0},
    {"Ent", 54, HID_KEYB_USAGE_ENTER, DefaultSpecialHandler, 0},
    {"BS", 38, HID_KEYB_USAGE_BACKSPACE, DefaultSpecialHandler, 0}
};

#define NUM_ROW4_KEYS (sizeof(g_psRow4) / sizeof(tSpecialKey))

//****************************************************************************
//
// Keys on the fifth row of the virtual keyboard.  Strings are defined showing
// the keycaps in unshifted, shifted and caps states.  This row contains only
// cursor keys so the key caps are the same for each state.
//
//****************************************************************************
const tSpecialKey g_psRow5[] =
{
    {"Alt", 54, 0, AltHandler, AltRedrawHandler},
    {"Ctrl", 54, 0, CtrlHandler, CtrlRedrawHandler},
    {"GUI", 36, 0, GUIHandler, GUIRedrawHandler},
    {"<", 26, HID_KEYB_USAGE_LEFT_ARROW, DefaultSpecialHandler, 0},
    {">", 26, HID_KEYB_USAGE_RIGHT_ARROW, DefaultSpecialHandler, 0},
    {"^", 26, HID_KEYB_USAGE_UP_ARROW, DefaultSpecialHandler, 0},
    {"v", 26, HID_KEYB_USAGE_DOWN_ARROW, DefaultSpecialHandler, 0},
};

#define NUM_ROW5_KEYS (sizeof(g_psRow5) / sizeof(tSpecialKey))

//****************************************************************************
//
// Define the rows of the virtual keyboard.
//
//****************************************************************************
const tRow g_psKeyboard[NUM_KEYBOARD_ROWS] =
{
    {false, (void *)&g_sRow0, NUM_ROW0_KEYS, 20},
    {false, (void *)&g_sRow1, NUM_ROW1_KEYS, 20 + (KEYBOARD_CELL_WIDTH / 3)},
    {false, (void *)&g_sRow2, NUM_ROW2_KEYS, 20 +
     ((2 * KEYBOARD_CELL_WIDTH) / 3)},
    {false, (void *)&g_sRow3, NUM_ROW3_KEYS, 20},
    {true, (void *)g_psRow4, NUM_ROW4_KEYS, 20},
    {true, (void *)g_psRow5, NUM_ROW5_KEYS, 20 + (KEYBOARD_CELL_WIDTH / 4)}
};

//****************************************************************************
//
// The current active key in the virtual keyboard.
//
//****************************************************************************
short g_sFocusRow = 0;
short g_sFocusCol = 0;

//****************************************************************************
//
// The coordinates of the last touchscreen press.
//
//****************************************************************************
short g_sXPress = 0;
short g_sYPress = 0;

//****************************************************************************
//
// Flags used to indicate events requiring attention from the main loop.
//
//****************************************************************************
unsigned long g_ulCommand = 0;

//****************************************************************************
//
// Values ORed into g_ulCommand to indicate screen press and release events.
//
//****************************************************************************
#define COMMAND_PRESS       0x01
#define COMMAND_RELEASE     0x02

//****************************************************************************
//
// SysCtlDelay takes 3 clock cycles so calculate the number of loops
// per millisecond.
//
// = ((50000000 cycles/sec) / (1000 ms/sec)) / 3 cycles/loop
//
// = (50000000 / (1000 * 3)) loops
//
//****************************************************************************
#define SYSDELAY_1_MS (50000000 / (1000 * 3))

//****************************************************************************
//
// This global indicates if there is currently a key pressed.
//
//****************************************************************************
tBoolean g_bKeyPressed;

//****************************************************************************
//
// Global system tick counter holds elapsed time since the application started
// expressed in 100ths of a second.
//
//****************************************************************************
volatile unsigned long g_ulSysTickCount;

//****************************************************************************
//
// The number of system ticks to wait for each USB packet to be sent before
// we assume the host has disconnected.  The value 50 equates to half a
// second.
//
//****************************************************************************
#define MAX_SEND_DELAY          50

//****************************************************************************
//
// This global holds the current state of the keyboard LEDs as sent by the
// host.
//
//****************************************************************************
volatile unsigned char g_ucLEDStates;

//****************************************************************************
//
// This global is set by the USB data handler if the host reports a change in
// the keyboard LED states.  The main loop uses it to update the virtual
// keyboard state.
//
//****************************************************************************
volatile tBoolean g_bLEDStateChanged;

//****************************************************************************
//
// This enumeration holds the various states that the keyboard can be in
// during normal operation.
//
//****************************************************************************
volatile enum
{
    //
    // Unconfigured.
    //
    STATE_UNCONFIGURED,

    //
    // No keys to send and not waiting on data.
    //
    STATE_IDLE,

    //
    // Waiting on data to be sent out.
    //
    STATE_SENDING
} g_eKeyboardState = STATE_UNCONFIGURED;

//****************************************************************************
//
// The current state of the modifier key flags which form the first byte of
// the report to the host.  This indicates the state of the shift, control,
// alt and GUI keys on the keyboard.
//
//****************************************************************************
static unsigned char g_ucModifiers = 0;

//****************************************************************************
//
// This function is called by the touchscreen driver whenever there is a
// change in press state or position.
//
//****************************************************************************
static long
KeyboardTouchHandler(unsigned long ulMessage, long lX, long lY)
{
    switch(ulMessage)
    {
        //
        // The touchscreen has been pressed.  Remember the coordinates and
        // set the flag indicating that the main loop should process some new
        // input.
        //
        case WIDGET_MSG_PTR_DOWN:
        {
            g_sXPress = (short)lX;
            g_sYPress = (short)lY;
            g_ulCommand |= COMMAND_PRESS;
            break;
        }

        //
        // The touchscreen is no longer being pressed.  Release any key which
        // was previously pressed.
        //
        case WIDGET_MSG_PTR_UP:
        {
            g_ulCommand |= COMMAND_RELEASE;
            break;
        }

        //
        // We have nothing to do on pointer move events.
        //
        case WIDGET_MSG_PTR_MOVE:
        {
            break;
        }
    }

    return(0);
}

//****************************************************************************
//
// Handles asynchronous events from the HID keyboard driver.
//
// \param pvCBData is the event callback pointer provided during
// USBDHIDKeyboardInit().  This is a pointer to our keyboard device structure
// (&g_sKeyboardDevice).
// \param ulEvent identifies the event we are being called back for.
// \param ulMsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the HID keyboard driver to inform the
// application of particular asynchronous events related to operation of the
// keyboard HID device.
//
// \return Returns 0 in all cases.
//
//****************************************************************************
unsigned long KeyboardHandler(void *pvCBData, unsigned long ulEvent,
                              unsigned long ulMsgData, void *pvMsgData)
{
    switch (ulEvent)
    {
        //
        // We receive this event every time the host acknowledges transmission
        // of a report. It is used here purely as a way of determining whether
        // the host is still talking to us or not.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // Enter the idle state since we finished sending something.
            //
            g_eKeyboardState = STATE_IDLE;
            break;
        }

        //
        // This event indicates that the host has sent us an Output or
        // Feature report and that the report is now in the buffer we provided
        // on the previous USBD_HID_EVENT_GET_REPORT_BUFFER callback.
        //
        case USBD_HID_KEYB_EVENT_SET_LEDS:
        {

            //
            // Remember the new LED state.
            //
            g_ucLEDStates = (unsigned char)(ulMsgData & 0xFF);

            //
            // Set a flag to tell the main loop that the LED state changed.
            //
            g_bLEDStateChanged = true;

            break;
        }

        //
        // We ignore all other events.
        //
        default:
        {
            break;
        }
    }
    return (0);
}

//***************************************************************************
//
// Wait for a period of time for the state to become idle.
//
// \param ulTimeoutTick is the number of system ticks to wait before
// declaring a timeout and returning \b false.
//
// This function polls the current keyboard state for ulTimeoutTicks system
// ticks waiting for it to become idle.  If the state becomes idle, the
// function returns true.  If it ulTimeoutTicks occur prior to the state
// becoming idle, false is returned to indicate a timeout.
//
// \return Returns \b true on success or \b false on timeout.
//
//***************************************************************************
tBoolean WaitForSendIdle(unsigned long ulTimeoutTicks)
{
    unsigned long ulStart;
    unsigned long ulNow;
    unsigned long ulElapsed;

    ulStart = g_ulSysTickCount;
    ulElapsed = 0;

    while (ulElapsed < ulTimeoutTicks)
    {
        //
        // Is the keyboard is idle, return immediately.
        //
        if (g_eKeyboardState == STATE_IDLE)
        {
            return (true);
        }

        //
        // Determine how much time has elapsed since we started waiting.  This
        // should be safe across a wrap of g_ulSysTickCount.  I suspect you
        // won't likely leave the app running for the 497.1 days it will take
        // for this to occur but you never know...
        //
        ulNow = g_ulSysTickCount;
        ulElapsed = (ulStart < ulNow) ? (ulNow - ulStart)
                        : (((unsigned long)0xFFFFFFFF - ulStart) + ulNow + 1);
    }

    //
    // If we get here, we timed out so return a bad return code to let the
    // caller know.
    //
    return (false);
}

//***************************************************************************
//
// Determine the X position on the screen for a given key in the virtual
// keyboard.
//
// \param sCol is the column number of the key whose position is being
//  queried.
// \param sRow is the row number of the key whose position is being queried.
//
// \return Returns the horizontal pixel coordinate of the left edge of the
// key. Note that this is 1 greater than you would expect since we allow
// space for the focus border round the character.
//
//***************************************************************************
short GetVirtualKeyX(short sCol, short sRow)
{
    short sX;
    short sCount;
    tSpecialKey *pKey;

    //
    // Is this a row of special keys?
    //
    if (g_psKeyboard[sRow].bSpecial)
    {
        //
        // Yes - we need to walk along the row of keys since the widths can
        // vary by key.
        //
        sX = g_psKeyboard[sRow].sLeftOffset;
        pKey = (tSpecialKey *)(g_psKeyboard[sRow].pvKeys);

        for (sCount = 0; sCount < sCol; sCount++)
        {
            sX += (pKey[sCount].sWidth + KEYBOARD_COL_SPACING);
        }

        //
        // Return the calculated X position for the key.
        //
        return (sX + 1);
    }
    else
    {
        //
        // This is a normal alphanumeric row so the keys are all the same
        // width.
        //
        return (g_psKeyboard[sRow].sLeftOffset +
                (sCol * KEYBOARD_CELL_WIDTH) + 1);
    }
}

//***************************************************************************
//
// Find a key on one row closest the a key on another row.
//
// \param sFromCol
// \param sFromRow
// \param sToRow
//
// This function is called during processing of the up and down keys while
// navigating the virtual keyboard.  It finds the key in row sToRow that sits
// closest to key index sFromCol in row sFromRow.
//
// \return Returns the index (column number) of the closest key in row sToRow.
//
//***************************************************************************
short VirtualKeyboardFindClosestKey(short sFromCol, short sFromRow,
                                    short sToRow)
{
    short sIndex;
    short sX;

    //
    // If moving between 2 alphanumeric rows, just move to the same key
    // index in the new row (taking care to pass back a valid key index).
    //
    if (!g_psKeyboard[sFromRow].bSpecial && !g_psKeyboard[sToRow].bSpecial)
    {
        sIndex = sFromCol;
        if (sIndex > g_psKeyboard[sToRow].sNumKeys)
        {
            sIndex = g_psKeyboard[sToRow].sNumKeys - 1;
        }

        return (sIndex);
    }

    //
    // Determine the x position of the key we are moving from.
    //
    sX = GetVirtualKeyX(sFromCol, sFromRow);

    //
    // Check for cases where the supplied x coordinate is at or to the left of
    // any key in this row.  In this case, we always pass back index 0.
    //
    if (sX <= g_psKeyboard[sToRow].sLeftOffset)
    {
        return (0);
    }

    //
    // The x coordinate is not to the left of any key so we need to determine
    // which particular key it relates to.  The position is associated with a
    // key if it falls within the width of the key and the following space.
    //
    if (g_psKeyboard[sToRow].bSpecial)
    {
        //
        // This is a special key so the keys on this row can all have
        // different widths.  We walk through them looking for a hit.
        //
        for (sIndex = 1; sIndex < g_psKeyboard[sToRow].sNumKeys; sIndex++)
        {
            //
            // If the passed coordinate is less than the leftmost position of
            // this key, we've overshot.  Drop out since we've found our
            // answer.
            //
            if (sX < GetVirtualKeyX(sIndex, sToRow))
            {
                break;
            }
        }

        //
        // Return the index of the key one before the one we last looked at
        // since this is the key which contains the supplied x coordinate.
        // Since we end the loop above on the last key this handles cases
        // where the x coordinate passed is further right than any key on the
        // row.
        //
        return (sIndex - 1);
    }
    else
    {
        //
        // This is an alphanumeric row so we determine the index based on
        // the fixed character cell width.
        //
        sIndex = (sX - g_psKeyboard[sToRow].sLeftOffset) / KEYBOARD_CELL_WIDTH;

        //
        // If we calculated an index higher than the number of keys on the
        // row, return the largest index supported.
        //
        if (sIndex >= g_psKeyboard[sToRow].sNumKeys)
        {
            sIndex = g_psKeyboard[sToRow].sNumKeys - 1;
        }
    }

    //
    // Return the column index we calculated.
    //
    return (sIndex);
}

//***************************************************************************
//
// Draw a single key of the virtual keyboard.
//
// \param sCol contains the column number for the key to be drawn.
// \param sRow contains the row  number for the key to be drawn.
// \param bFocus is \b true if the red focus border is to be drawn around this
//  key or \b false if the border is to be erased.
// \param bPressed is \b true of the key is to be drawn in the pressed state
//  or \b false if it is to be drawn in the released state.
// \param bBorder is \b true if the whole key is to be redrawn or \b false
//  if only the key cap text is to be redrawn.
// \param bBright is \b true if the key is to be drawn in the bright (yellow)
//  color or \b false if drawn in the normal (grey) color.
//
// This function draws a single key, varying the look depending upon whether
// the key is pressed or released and whether it has the input focus or not.
// If the bBorder parameter is false, only the key label is refreshed.  If
// true, the whole key is redrawn.
//
// This is the lowest level function used to refresh the display of both
// alphanumeric and special keys.
//
// \return None.
//
//***************************************************************************
void DrawKey(short sCol, short sRow, tBoolean bFocus, tBoolean bPressed,
             tBoolean bBorder, tBoolean bBright)
{
    tRectangle sRectOutline;
    tRectangle sFocusBorder;
    tSpecialKey *psSpecial;
    tAlphaKeys *psAlpha;
    short sX;
    short sY;
    short sWidth;
    char pcBuffer[2];
    char *pcLabel;
    unsigned long ulHighlight;
    unsigned long ulShadow;

    //
    // Determine the position, width and text label for this key.
    //
    sX = GetVirtualKeyX(sCol, sRow);
    sY = KEYBOARD_TOP + (sRow * KEYBOARD_CELL_HEIGHT);
    if (g_psKeyboard[sRow].bSpecial)
    {
        psSpecial = (tSpecialKey *)g_psKeyboard[sRow].pvKeys;
        sWidth = psSpecial[sCol].sWidth;
        pcLabel = (char *)psSpecial[sCol].pcLabel;
    }
    else
    {
        sWidth = KEYBOARD_KEY_WIDTH;

        psAlpha = (tAlphaKeys *)g_psKeyboard[sRow].pvKeys;
        pcBuffer[1] = (char)0;
        pcBuffer[0] = (psAlpha->pcKey[g_eVirtualKeyState])[sCol];
        pcLabel = pcBuffer;
    }

    //
    // Determine the bounding rectangle for the key.  This rectangle is the
    // area containing the key background color and label text.  It excludes
    // the 1 line border.
    //
    sRectOutline.sXMin = sX + 1;
    sRectOutline.sYMin = sY + 1;
    sRectOutline.sXMax = (sX + sWidth) - 2;
    sRectOutline.sYMax = (sY + KEYBOARD_KEY_HEIGHT) - 2;

    //
    // If the key has focus, we will draw a 1 pixel red line around it
    // outside the actual key cell.  Set up the rectangle for this here.
    //
    sFocusBorder.sXMin = sX - 1;
    sFocusBorder.sYMin = sY - 1;
    sFocusBorder.sXMax = sX + sWidth;
    sFocusBorder.sYMax = sY + KEYBOARD_KEY_HEIGHT;

    //
    // Pick the relevant highlight and shadow colors depending upon the button
    // state.
    //
    if (!bBright)
    {
        //
        // The key is not bright so just pick the normal (grey)
        //
        ulHighlight = bPressed ? SHADOW_COLOR : HIGHLIGHT_COLOR;
        ulShadow = bPressed ? HIGHLIGHT_COLOR : SHADOW_COLOR;
    }
    else
    {
        ulHighlight = bPressed ? SHADOW_BRIGHT_COLOR : HIGHLIGHT_BRIGHT_COLOR;
        ulShadow = bPressed ? HIGHLIGHT_BRIGHT_COLOR : SHADOW_BRIGHT_COLOR;
    }

    //
    // Are we drawing the whole key or merely updating the label?
    //
    if (bBorder)
    {
        //
        // Draw the focus border in the relevant color.
        //
        GrContextForegroundSet(&g_sContext, bFocus ? FOCUS_COLOR
                        : BACKGROUND_COLOR);
        GrRectDraw(&g_sContext, &sFocusBorder);

        //
        // Draw the key border.
        //
        GrContextForegroundSet(&g_sContext, ulHighlight);
        GrLineDrawH(&g_sContext, sX, sX + sWidth - 1, sY);
        GrLineDrawV(&g_sContext, sX, sY, sY + KEYBOARD_KEY_HEIGHT - 1);
        GrContextForegroundSet(&g_sContext, ulShadow);
        GrLineDrawH(&g_sContext, sX + 1, sX + sWidth - 1, sY
                        + KEYBOARD_KEY_HEIGHT - 1);
        GrLineDrawV(&g_sContext, sX + sWidth - 1, sY + 1, sY
                        + KEYBOARD_KEY_HEIGHT - 1);
    }

    //
    // Fill the button with the main button color
    //
    GrContextForegroundSet(&g_sContext,
                           bBright ? KEY_BRIGHT_COLOR : KEY_COLOR);
    GrRectFill(&g_sContext, &sRectOutline);

    //
    // Update the key label.  We center the text in the key, moving it one
    // pixel down and to the right if the key is in the pressed state.
    //
    GrContextForegroundSet(&g_sContext, KEY_TEXT_COLOR);
    GrContextBackgroundSet(&g_sContext,
                           bBright ? KEY_BRIGHT_COLOR : KEY_COLOR);
    GrContextClipRegionSet(&g_sContext, &sRectOutline);
    GrStringDrawCentered(&g_sContext, pcLabel, -1, (bPressed ? 1 : 0)
                    + ((sRectOutline.sXMax + sRectOutline.sXMin) / 2),
                          (bPressed ? 1 : 0) + ((sRectOutline.sYMax
                                         + sRectOutline.sYMin) / 2), true);

    //
    // Revert to the previous clipping region.
    //
    sRectOutline.sXMin = 0;
    sRectOutline.sYMin = 0;
    sRectOutline.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRectOutline.sYMax = GrContextDpyHeightGet(&g_sContext) - 1;
    GrContextClipRegionSet(&g_sContext, &sRectOutline);

    //
    // Revert to the usual background and foreground colors.
    //
    GrContextBackgroundSet(&g_sContext, BACKGROUND_COLOR);
    GrContextForegroundSet(&g_sContext, ClrWhite);
}

//***************************************************************************
//
// Call the appropriate handler to draw a single key on the virtual
// keyboard. This top level function handles both alphanumeric and special
// keys.
//
// \param sCol contains the column number for the key to be drawn.
// \param sRow contains the row  number for the key to be drawn.
// \param bFocus is \b true if the red focus border is to be drawn around this
//  key or \b false if the border is to be erased.
// \param bPressed is \b true of the key is to be drawn in the pressed state
//  or \b false if it is to be drawn in the released state.
// \param bBorder is \b true if the whole key is to be redrawn or \b false
//  if only the key cap text is to be redrawn.
//
// This function draws a single key on the keyboard, varying the look
// depending upon whether the key is pressed or released and whether it has
// the input focus or not.  If the bBorder parameter is \b false, only the key
// label is refreshed.  If \b true, the whole key is redrawn.
//
// If the specific key is a special key with a redraw handler set, the
// handler function is called to update the display.  If not, the basic
// DrawKey() function is used.
//
// \return None.
//
//***************************************************************************
void DrawVirtualKey(short sCol, short sRow, tBoolean bFocus,
                    tBoolean bPressed, tBoolean bBorder)
{
    tSpecialKey *psSpecial;

    //
    // Get a pointer to the array of special keys for this row (even though
    // we are not yet sure if this is a special row).
    //
    psSpecial = (tSpecialKey *)g_psKeyboard[sRow].pvKeys;

    //
    // Is this a special row and, if so, does the current key have a redraw
    // handler installed?
    //
    if (g_psKeyboard[sRow].bSpecial && psSpecial[sCol].pfnRedrawHandler)
    {
        //
        // Yes - call the special handler for this key.
        //
        psSpecial[sCol].pfnRedrawHandler(sCol, sRow, bFocus, bPressed,
                                         bBorder);
    }
    else
    {
        //
        // The key has no redraw handler so just treat it as a normal
        // key.
        //
        DrawKey(sCol, sRow, bFocus, bPressed, bBorder, false);
    }
}

//***************************************************************************
//
// Draw or update the virtual keyboard on the display.
//
// \param bBorder is \b true if the whole virtual keyboard is to be drawn or
// \b false if only the key caps have to be updated.
//
// Draw the virtual keyboard on the display.  The bBorder parameter controls
// whether the whole keyboard is drawn (true) or whether only the key labels
// are replaced (false).
//
// \return None.
//
//***************************************************************************
void DrawVirtualKeyboard(tBoolean bBorder)
{
    short sCol;
    short sRow;

    //
    // Select the font we use for the keycaps.
    //
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);

    //
    // Loop through each row, drawing each to the display
    //
    for (sRow = 0; sRow < NUM_KEYBOARD_ROWS; sRow++)
    {
        //
        // Loop through each key on this row of the keyboard.
        //
        for (sCol = 0; sCol < g_psKeyboard[sRow].sNumKeys; sCol++)
        {
            //
            // Draw a single key.
            //
            DrawVirtualKey(sCol, sRow, false, false, bBorder);
        }
    }
}

//****************************************************************************
//
// This function is called by the main loop if it receives a signal from the
// USB data handler telling it that the host has changed the state of the
// keyboard LEDs.  We update the state and display accordingly.
//
//****************************************************************************
void KeyboardLEDsChanged(void)
{
    tBoolean bCapsOn;

    //
    // Clear the flag indicating a state change occurred.
    //
    g_bLEDStateChanged = false;

    //
    // Is CAPSLOCK on or off?
    //
    bCapsOn = (g_ucLEDStates & HID_KEYB_CAPS_LOCK) ? true : false;

    //
    // Update the state to ensure that the communicated CAPSLOCK state is
    // incorporated.
    //
    switch (g_eVirtualKeyState)
    {
        //
        // Are we in an unshifted state?
        //
        case KEY_STATE_NORMAL:
        case KEY_STATE_CAPS:
        {
            if (bCapsOn)
            {
                g_eVirtualKeyState = KEY_STATE_CAPS;
            }
            else
            {
                g_eVirtualKeyState = KEY_STATE_NORMAL;
            }
            break;
        }

            //
            // Are we in a shifted state?
            //
        case KEY_STATE_SHIFT:
        case KEY_STATE_BOTH:
        {
            if (bCapsOn)
            {
                g_eVirtualKeyState = KEY_STATE_BOTH;
            }
            else
            {
                g_eVirtualKeyState = KEY_STATE_SHIFT;
            }
            break;
        }

        default:
        {
            //
            // Do nothing.  This default case merely prevents a compiler
            // warning related to the NUM_KEY_STATES enum member not having
            // a handler.
            //
            break;
        }
    }

    //
    // Redraw the virtual keyboard keycaps with the appropriate characters.
    //
    DrawVirtualKeyboard(false);

    //
    // Set the CAPSLOCK LED appropriately.
    //
    GPIOPinWrite(CAPSLOCK_GPIO_BASE, CAPSLOCK_GPIO_PIN,
    bCapsOn ? CAPSLOCK_ACTIVE : CAPSLOCK_INACTIVE);
}

//***************************************************************************
//
// Special key handler for the Caps virtual key.
//
// \param sCol is the column number of the key which has been pressed.
// \param sRow is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the CapsLock key on the virtual keyboard has input focus.
//
// \returns Returns \b KEYB_SUCCESS on success or a non-zero value to
// indicate failure.
//
//***************************************************************************
unsigned long
CapsLockHandler(short sCol, short sRow, tBoolean bPress)
{
    unsigned long ulRetcode;

    //
    // Note that we don't set the state or redraw the keyboard here since the
    // host is expected to send us an update telling is that the CAPSLOCK
    // state changed.  We trigger the keyboard redrawing and LED setting off
    // this message instead.  In this function, we only redraw the CAPSLOCK
    // key itself to provide user feedback.
    //
    DrawKey(sCol, sRow, bPress ? true : false, bPress, true,
            (g_ucLEDStates & HID_KEYB_CAPS_LOCK) ? true : false);

    //
    // Send the CAPSLOCK key code back to the host.
    //
    g_eKeyboardState = STATE_SENDING;
    ulRetcode = USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                              g_ucModifiers,
                                              HID_KEYB_USAGE_CAPSLOCK,
                                              bPress);

    return (ulRetcode);
}

//***************************************************************************
//
// Special key handler for the Ctrl virtual key.
//
// \param sCol is the column number of the key which has been pressed.
// \param sRow is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the Ctrl key on the virtual keyboard has input focus.
//
// \returns Returns \b KEYB_SUCCESS on success or a non-zero value to
// indicate failure.
//
//***************************************************************************
unsigned long
CtrlHandler(short sCol, short sRow, tBoolean bPress)
{
    unsigned long ulRetcode;

    //
    // Ignore key release messages.
    //
    if(bPress)
    {
        //
        // Toggle the modifier bit for the left control key.
        //
        g_ucModifiers ^= HID_KEYB_LEFT_CTRL;

        //
        // Update the host with the new modifier state.  Sending usage code
        // HID_KEYB_USAGE_RESERVED indicates no key press so this changes only
        // the modifiers.
        //
        g_eKeyboardState = STATE_SENDING;
        ulRetcode = USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                                  g_ucModifiers,
                                                  HID_KEYB_USAGE_RESERVED,
                                                  true);
    }
    else
    {
        //
        // We are ignoring key release but tell the caller that all is well.
        //
        ulRetcode = KEYB_SUCCESS;
    }

    //
    // Redraw the key in the appropriate state.
    //
    DrawKey(sCol, sRow, bPress ? true : false, bPress, true,
            (g_ucModifiers & HID_KEYB_LEFT_CTRL) ? true : false);

    return(ulRetcode);
}

//***************************************************************************
//
// Special key handler for the Alt virtual key.
//
// \param sCol is the column number of the key which has been pressed.
// \param sRow is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the Alt key on the virtual keyboard has input focus.
//
// \returns Returns \b KEYB_SUCCESS on success or a non-zero value to
// indicate failure.
//
//***************************************************************************
unsigned long
AltHandler(short sCol, short sRow, tBoolean bPress)
{
    unsigned long ulRetcode;

    //
    // Ignore key release messages.
    //
    if(bPress)
    {
        //
        // Toggle the modifier bit for the left ALT key.
        //
        g_ucModifiers ^= HID_KEYB_LEFT_ALT;

        //
        // Update the host with the new modifier state.  Sending usage code
        // HID_KEYB_USAGE_RESERVED indicates no key press so this changes only
        // the modifiers.
        //
        g_eKeyboardState = STATE_SENDING;
        ulRetcode = USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                                  g_ucModifiers,
                                                  HID_KEYB_USAGE_RESERVED,
                                                  true);
    }
    else
    {
        //
        // We are ignoring key release but tell the caller that all is well.
        //
        ulRetcode = KEYB_SUCCESS;
    }

    //
    // Redraw the key in the appropriate state.
    //
    DrawKey(sCol, sRow, bPress ? true : false, bPress, true,
            (g_ucModifiers & HID_KEYB_LEFT_ALT) ? true : false);

    return(ulRetcode);
}

//***************************************************************************
//
// Special key handler for the GUI virtual key.
//
// \param sCol is the column number of the key which has been pressed.
// \param sRow is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the GUI key on the virtual keyboard has input focus.
//
// \returns Returns \b KEYB_SUCCESS on success or a non-zero value to
// indicate failure.
//
//***************************************************************************
unsigned long
GUIHandler(short sCol, short sRow, tBoolean bPress)
{
    unsigned long ulRetcode;

    //
    // Ignore key release messages.
    //
    if(bPress)
    {
        //
        // Toggle the modifier bit for the left GUI key.
        //
        g_ucModifiers ^= HID_KEYB_LEFT_GUI;

        //
        // Update the host with the new modifier state.  Sending usage code
        // HID_KEYB_USAGE_RESERVED indicates no key press so this changes only
        // the modifiers.
        //
        g_eKeyboardState = STATE_SENDING;
        ulRetcode = USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                                  g_ucModifiers,
                                                  HID_KEYB_USAGE_RESERVED,
                                                  true);
    }
    else
    {
        //
        // We are ignoring key release but tell the caller that all is well.
        //
        ulRetcode = KEYB_SUCCESS;
    }

    //
    // Redraw the key in the appropriate state.
    //
    DrawKey(sCol, sRow, bPress ? true : false, bPress, true,
            (g_ucModifiers & HID_KEYB_LEFT_GUI) ? true : false);

    return(ulRetcode);
}

//***************************************************************************
//
// Special key handler for the Shift virtual key.
//
// \param sCol is the column number of the key which has been pressed.
// \param sRow is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the ShiftLock key on the virtual keyboard has input focus.
//
// \returns Returns \b true on success or \b false on failure.
//
//***************************************************************************
unsigned long
ShiftLockHandler(short sCol, short sRow, tBoolean bPress)
{
    //
    // We ignore key release for the shift lock.
    //
    if(bPress)
    {
        //
        // Set the new state by toggling the shift component.
        //
        switch(g_eVirtualKeyState)
        {
            case KEY_STATE_NORMAL:
            {
                g_eVirtualKeyState = KEY_STATE_SHIFT;
                g_ucModifiers |= HID_KEYB_LEFT_SHIFT;
                break;
            }

            case KEY_STATE_SHIFT:
            {
                g_eVirtualKeyState = KEY_STATE_NORMAL;
                g_ucModifiers &= ~HID_KEYB_LEFT_SHIFT;
                break;
            }

            case KEY_STATE_CAPS:
            {
                g_eVirtualKeyState = KEY_STATE_BOTH;
                g_ucModifiers |= HID_KEYB_LEFT_SHIFT;
                break;
            }

            case KEY_STATE_BOTH:
            {
                g_eVirtualKeyState = KEY_STATE_CAPS;
                g_ucModifiers &= ~HID_KEYB_LEFT_SHIFT;
                break;
            }

            default:
            {
                //
                // Do nothing.  This default case merely prevents a compiler
                // warning related to the NUM_KEY_STATES enum member not
                // having a handler.
                //
                break;
            }
        }

        //
        // Redraw the keycaps to show the shifted characters.
        //
        DrawVirtualKeyboard(false);
    }

    //
    // Redraw the SHIFT key in the appropriate state.
    //
    DrawKey(sCol, sRow, bPress ? true : false, bPress, true,
            (g_ucModifiers & HID_KEYB_LEFT_SHIFT) ? true : false);

    return(KEYB_SUCCESS);
}

//***************************************************************************
//
// Redraw the caps lock key. This is a thin layer over the usual DrawKey
// function which merely sets the key into bright or normal mode depending
// upon the current caps lock state.
//
//***************************************************************************
void
CapsLockRedrawHandler(short sCol, short sRow, tBoolean bFocus,
                tBoolean bPressed, tBoolean bBorder)
{
    //
    // Draw the key in either normal color if the CAPS lock is not active
    // or in the bright color if it is.
    //
    DrawKey(sCol, sRow, bFocus, bPressed, bBorder,
            ((g_eVirtualKeyState == KEY_STATE_BOTH) ||
             (g_eVirtualKeyState == KEY_STATE_CAPS)) ? true : false);
}

//***************************************************************************
//
// Redraw the Shift lock key. This is a thin layer over the usual DrawKey
// function which merely sets the key into bright or normal mode depending
// upon the current shift state.
//
//***************************************************************************
void
ShiftLockRedrawHandler(short sCol, short sRow, tBoolean bFocus,
                tBoolean bPressed, tBoolean bBorder)
{
    //
    // Draw the key in either normal color if the shift lock is not active
    // or in the bright color if it is.
    //
    DrawKey(sCol, sRow, bFocus, bPressed, bBorder,
            (g_ucModifiers & HID_KEYB_LEFT_SHIFT) ? true : false);
}

//***************************************************************************
//
// Redraw the Ctrl sticky key. This is a thin layer over the usual DrawKey
// function which merely sets the key into bright or normal mode depending
// upon the current key state.
//
//***************************************************************************
void
CtrlRedrawHandler(short sCol, short sRow, tBoolean bFocus,
                  tBoolean bPressed, tBoolean bBorder)
{
    //
    // Draw the key in either normal color if CTRL is not active
    // or in the bright color if it is.
    //
    DrawKey(sCol, sRow, bFocus, bPressed, bBorder,
            (g_ucModifiers & HID_KEYB_LEFT_CTRL) ? true : false);
}

//***************************************************************************
//
// Redraw the Alt sticky key. This is a thin layer over the usual DrawKey
// function which merely sets the key into bright or normal mode depending
// upon the current key state.
//
//***************************************************************************
void
AltRedrawHandler(short sCol, short sRow, tBoolean bFocus,
                 tBoolean bPressed, tBoolean bBorder)
{
    //
    // Draw the key in either normal color if CTRL is not active
    // or in the bright color if it is.
    //
    DrawKey(sCol, sRow, bFocus, bPressed, bBorder,
            (g_ucModifiers & HID_KEYB_LEFT_ALT) ? true : false);
}

//***************************************************************************
//
// Redraw the GUI sticky key. This is a thin layer over the usual DrawKey
// function which merely sets the key into bright or normal mode depending
// upon the current key state.
//
//***************************************************************************
void
GUIRedrawHandler(short sCol, short sRow, tBoolean bFocus,
                  tBoolean bPressed, tBoolean bBorder)
{
    //
    // Draw the key in either normal color if CTRL is not active
    // or in the bright color if it is.
    //
    DrawKey(sCol, sRow, bFocus, bPressed, bBorder,
           (g_ucModifiers & HID_KEYB_LEFT_GUI) ? true : false);
}

//***************************************************************************
//
// Special key handler for the space, enter, backspace and cursor control
// virtual keys.
//
// \param sCol is the column number of the key which has been pressed.
// \param sRow is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the user presses the "Select" button
// when the space, backspace, enter or cursor control keys on the virtual
// keyboard have input focus.  These keys are like any other alpha key in that
// they merely send a single usage code back to the host.  We need a special
// handler for them, however, since they are on the bottom row of the virtual
// keyboard and this row contains other special keys.
//
// \returns Returns \b true on success or \b false on failure.
//
//***************************************************************************
unsigned long
DefaultSpecialHandler(short sCol, short sRow, tBoolean bPress)
{
    tSpecialKey *psKey;
    unsigned long ulRetcode;

    //
    // Get a pointer to the array of keys for this row.
    //
    psKey = (tSpecialKey *)g_psKeyboard[sRow].pvKeys;

    //
    // Send the usage code for this key back to the USB host.
    //
    g_eKeyboardState = STATE_SENDING;
    ulRetcode = USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                g_ucModifiers, psKey[sCol].cUsageCode,
                                bPress);

    //
    // Redraw the key in the appropriate state.
    //
    DrawKey(sCol, sRow, bPress ? true : false, bPress, true, false);

    return(ulRetcode);
}

//****************************************************************************
//
// Processes a single key press on the virtual keyboard.
//
// \param sCol is the column number of the key which has been pressed.
// \param sRow is the row number of the key which has been pressed.
// \param bPress is \b true if the key has been pressed or \b false if it has
// been released.
//
// This function is called whenever the "Select" button is pressed or
// released. Depending upon the specific key, this will either call a special
// key handler function or send a report back to the USB host indicating the
// change of state.
//
// \return Returns \b true on success or \b false on failure.
//
//****************************************************************************
tBoolean
VirtualKeyboardKeyPress(short sCol, short sRow, tBoolean bPress)
{
    tSpecialKey *psKey;
    tAlphaKeys *psAlphaKeys;
    unsigned long ulRetcode;
    tBoolean bSuccess;

    //
    // Are we dealing with a special key?
    //
    if(g_psKeyboard[sRow].bSpecial)
    {
        //
        // Yes - call the handler for this special key.
        //
        psKey = (tSpecialKey *)g_psKeyboard[sRow].pvKeys;
        ulRetcode = psKey[sCol].pfnPressHandler(sCol, sRow, bPress);
    }
    else
    {
        //
        // Normal key - add or remove this key from the list of keys currently
        // pressed and pass the latest report back to the host.
        //
        psAlphaKeys = (tAlphaKeys *)g_psKeyboard[sRow].pvKeys;
        g_eKeyboardState = STATE_SENDING;
        ulRetcode = USBDHIDKeyboardKeyStateChange(
                                             (void *)&g_sKeyboardDevice,
                                             g_ucModifiers,
                                             psAlphaKeys->pcUsageCodes[sCol],
                                             bPress);

        //
        // Redraw the key in the appropriate state.
        //
        DrawKey(sCol, sRow, bPress ? true : false, bPress, true, false);
    }

    //
    // Did we schedule the report for transmission?
    //
    if(ulRetcode == KEYB_SUCCESS)
    {
        //
        // Wait for the host to acknowledge the transmission if all went well.
        //
        bSuccess = WaitForSendIdle(MAX_SEND_DELAY);

        //
        // Did we time out waiting for the packet to be sent?
        //
        if(!bSuccess)
        {
            //
            // Yes - assume the host disconnected and go back to waiting for
            // a new connection.
            //
            HWREGBITW(&g_ulFlags, FLAG_CONNECTED) = 0;
        }
    }
    else
    {
        //
        // An error was reported when trying to send the character.
        //
        bSuccess = false;
    }
    return(bSuccess);
}

//****************************************************************************
//
// Map a screen coordinate to the column and row of a virtual key.
//
// \param sX is the screen X coordinate that is to be mapped.
// \param sY is the screen Y coordinate that is to be mapped.
// \param pusCol is a pointer to the variable which will be written with the
// column number of the virtual key at screen position (sX, sY).
// \param pusRow is a pointer to the variable which will be written with the
// row number of the virtual key at screen position (sX, sY).
//
// \return Returns \b true if a virtual key exists at the position provided or
// \b false otherwise.  If \b false is returned, pointers \e pusCol and \e
// pusRow will not be written.
//
//****************************************************************************
static tBoolean
FindVirtualKey(short sX, short sY, short *psCol, short *psRow)
{
    unsigned long ulRow, ulCol, ulNumKeys;
    short sKeyX, sKeyWidth;
    tSpecialKey *pKey;

    //
    // Initialize the column value.
    //
    ulCol = 0;

    //
    // Determine which row the coordinates occur in.
    //
    for(ulRow = 0; ulRow < NUM_KEYBOARD_ROWS; ulRow++)
    {
        if((sY > (KEYBOARD_TOP + (ulRow * KEYBOARD_CELL_HEIGHT))) &&
           (sY < (KEYBOARD_TOP + (ulRow * KEYBOARD_CELL_HEIGHT) +
            KEYBOARD_KEY_HEIGHT)))
        {
            //
            // If this is a standard alphanumeric row, we can determine the
            // mapping arithmetically since all the keys are the same width.
            if(!g_psKeyboard[ulRow].bSpecial)
            {
                //
                // First check to make sure that the press is not to the left
                // of the first key in the row.
                //
                if(sX < g_psKeyboard[ulRow].sLeftOffset)
                {
                    return(false);
                }

                //
                // This includes presses that occur in the space between
                // keys but, given that the touchscreen is not hugely accurate
                // and that fingers or styli will likely cover more than a
                // couple of pixels, this is probably perfectly fine.
                //
                ulCol = ((sX - g_psKeyboard[ulRow].sLeftOffset) /
                         KEYBOARD_CELL_WIDTH);

                //
                // If we calculated an out of range column, this means no key
                // exists under the press position so return false to indicate
                // this.
                //
                if(ulCol >= g_psKeyboard[ulRow].sNumKeys)
                {
                    return(false);
                }
            }
            else
            {
                //
                // The touch is somewhere within this row of keys.  How many
                // keys are in this row?
                //
                ulNumKeys = g_psKeyboard[ulRow].sNumKeys;
                sKeyX = g_psKeyboard[ulRow].sLeftOffset;

                //
                // Walk through the keys in this row.
                //
                for(ulCol = 0; ulCol < ulNumKeys; ulCol++)
                {
                    sKeyX = GetVirtualKeyX(ulCol, ulRow);
                    pKey = (tSpecialKey *)(g_psKeyboard[ulRow].pvKeys);
                    sKeyWidth = pKey[ulCol].sWidth + KEYBOARD_COL_SPACING;

                    if((sX >= sKeyX) && (sX < (sKeyX + sKeyWidth)))
                    {
                        //
                        // We found a matching key so drop out of the loop.
                        //
                        break;
                    }
                }

                //
                // If we get here and ulCol has reached ulNumKeys, we didn't
                // find a key under the press position.
                //
                if(ulCol == ulNumKeys)
                {
                    return(false);
                }
            }
            break;
        }
    }

    //
    // If we end up here and the row number is equal to the number of rows in
    // the keyboard, the press was not in any keyboard row so return false.
    //
    if(ulRow == NUM_KEYBOARD_ROWS)
    {
        return(false);
    }

    //
    // At this point, we found a key beneath the press so we return the
    // information to the caller.
    //
    *psCol = (short)ulCol;
    *psRow = (short)ulRow;
    return(true);
}

//****************************************************************************
//
// Initialize the keyboard interface so that it is ready to start running
// the KeyboardMain() function.
//
//****************************************************************************
void
KeyboardInit(void)
{
    tRectangle sRect;

    //
    // Start with the assumption that no keys are pressed.
    //
    g_bKeyPressed = false;

    //
    // Configure GPIO pin which controls the CAPSLOCK LED and turn it off
    // initially.  Note that PinoutSet() already enabled the GPIO peripheral
    // containing this pin
    //
    GPIOPinTypeGPIOOutput(CAPSLOCK_GPIO_BASE, CAPSLOCK_GPIO_PIN);
    GPIOPinWrite(CAPSLOCK_GPIO_BASE, CAPSLOCK_GPIO_PIN, CAPSLOCK_INACTIVE);

    //
    // Enable the peripherals used by this example.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Set the system tick to fire 100 times per second.
    //
    SysTickPeriodSet(SysCtlClockGet() / SYSTICKS_PER_SECOND);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Initialize the touchscreen driver.
    //
    TouchScreenInit();

    //
    // Set the touchscreen event handler.
    //
    TouchScreenCallbackSet(KeyboardTouchHandler);

    //
    // Fill all but the top 24 rows of the screen with black to erase the
    // keyboard.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 24;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = GrContextDpyHeightGet(&g_sContext) - 1;
    GrContextForegroundSet(&g_sContext, ClrBlack);
    GrRectFill(&g_sContext, &sRect);

    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);

    //
    // Enter the idle state.
    //
    g_eKeyboardState = STATE_IDLE;

    //
    // Draw the keyboard on the display.
    //
    DrawVirtualKeyboard(true);
}

//****************************************************************************
//
// This is the main loop that runs the application.
//
//****************************************************************************
void
KeyboardMain(void)
{
    unsigned long ulProcessing;
    tBoolean bRetcode;

    //
    // If not connected or currently suspended then return.
    //
    if((HWREGBITW(&g_ulFlags, FLAG_CONNECTED) == 0) ||
       (HWREGBITW(&g_ulFlags, FLAG_SUSPENDED)))
    {
        return;
    }

    //
    // Do we have any touchscreen input to process?
    //
    if(g_ulCommand)
    {
        //
        // Take a snapshot of the commands we were sent then clear
        // the global command flags.
        //
        ulProcessing = g_ulCommand;
        g_ulCommand = 0;

        //
        // Process the command unless we got simultaneous press and
        // release commands in which case we ignore them.
        //
        if(!((ulProcessing & (COMMAND_PRESS | COMMAND_RELEASE)) ==
             (COMMAND_PRESS | COMMAND_RELEASE)))
        {
            //
            // Was the touchscreen pressed?
            //
            if(ulProcessing & COMMAND_PRESS)
            {
                //
                // Map the touchscreen press to an actual key in the
                // virtual keyboard.
                //
                bRetcode = FindVirtualKey(g_sXPress, g_sYPress,
                                         &g_sFocusCol, &g_sFocusRow);
                if(!bRetcode)
                {
                    //
                    // The press was outside any key on the virtual
                    // keyboard so just go back and wait for something
                    // else to happen.
                    //
                    return;
                }

                //
                // A key is pressed.
                //
                g_bKeyPressed = true;
            }

            //
            // Pass information on the press or release to the host,
            // making sure we only send a message if we really saw a
            // change of state.
            //
            if(g_bKeyPressed)
            {
                bRetcode = VirtualKeyboardKeyPress(g_sFocusCol, g_sFocusRow,
                                   ((ulProcessing == COMMAND_PRESS) ?
                                   true : false));
            }
            else
            {
                bRetcode = true;
            }

            //
            // Remember that no key is currently pressed.
            //
            if(ulProcessing & COMMAND_RELEASE)
            {
                g_bKeyPressed = false;
            }

            //
            // If the key press generated an error, this likely
            // indicates that the host has disconnected so drop out of
            // the loop and go back to looking for a new connection.
            //
            if(!bRetcode)
            {
                return;
            }
        }
    }

    //
    // Update the state if the host set the LEDs since we last looked.
    //
    if(g_bLEDStateChanged)
    {
        KeyboardLEDsChanged();
    }
}

//****************************************************************************
//
// This is the interrupt handler for the SysTick interrupt.  It is used to
// update our local tick count which, in turn, is used to check for transmit
// timeouts.
//
//****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update the system tick counter.
    //
    g_ulSysTickCount++;
}
