//*****************************************************************************
//
// usb_dev_mouse.c - Main routines for the enumeration example.
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
// This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/usb.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidmouse.h"
#include "utils/uartstdio.h"
#include "drivers/buttons.h"
#include "drivers/formike128x128x16.h"
#include "usb_mouse_structs.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Mouse Device (usb_dev_mouse)</h1>
//!
//! This example application turns the evaluation board into a USB mouse
//! supporting the Human Interface Device class.  Presses on the navigation
//! control on the evaluation board are translated into mouse movement and
//! button press messages in HID reports sent to the USB host allowing the
//! evaluation board to control the mouse pointer on the host system.
//
//*****************************************************************************

//*****************************************************************************
//
// Debug-related definitions and declarations.
//
// Debug output is available via UART0 if DEBUG is defined during build.
//
//*****************************************************************************
#ifdef DEBUG
//*****************************************************************************
//
// Map all debug print calls to UARTprintf in debug builds.
//
//*****************************************************************************
#define DEBUG_PRINT UARTprintf

#else

//*****************************************************************************
//
// Compile out all debug print calls in release builds.
//
//*****************************************************************************
#define DEBUG_PRINT while(0) ((int (*)(char *, ...))0)
#endif

//*****************************************************************************
//
// The parameters to control the USB mux on the LM3S3748 board.
//
//*****************************************************************************
#define USB_MUX_GPIO_PERIPH     SYSCTL_PERIPH_GPIOH
#define USB_MUX_GPIO_BASE       GPIO_PORTH_BASE
#define USB_MUX_GPIO_PIN        GPIO_PIN_2
#define USB_MUX_SEL_DEVICE      USB_MUX_GPIO_PIN
#define USB_MUX_SEL_HOST        0

//*****************************************************************************
//
// The defines used with the g_ulCommands variable.
//
//*****************************************************************************
#define BUTTON_TICK_EVENT       0x80000000

//*****************************************************************************
//
// The incremental update for the mouse.
//
//*****************************************************************************
#define MOUSE_MOVE_INC          ((char)4)
#define MOUSE_MOVE_DEC          ((char)-4)

//*****************************************************************************
//
// The system tick timer rate.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// Holds command bits used to signal the main loop to perform various tasks.
//
//*****************************************************************************
volatile unsigned long g_ulCommands;

//*****************************************************************************
//
// A flag used to indicate whether or not we are currently connected to the USB
// host.
//
//*****************************************************************************
volatile tBoolean g_bConnected;

//*****************************************************************************
//
// Global system tick counter holds elapsed time since the application started
// expressed in 100ths of a second.
//
//*****************************************************************************
volatile unsigned long g_ulSysTickCount;

//*****************************************************************************
//
// The number of system ticks to wait for each USB packet to be sent before
// we assume the host has disconnected.  The value 50 equates to half a second.
//
//*****************************************************************************
#define MAX_SEND_DELAY          50

//*****************************************************************************
//
// Structure holding name and screen positions for each button state
// indicator.
//
//*****************************************************************************
typedef struct
{
    short sX;
    short sY;
    unsigned char ucButton;
    const char *pcName;
}
tButtonDisplay;

tButtonDisplay g_psButtonDisplay[] =
{
    {64, 45, UP_BUTTON, "Up"},
    {28, 65, LEFT_BUTTON, "Left"},
    {100, 65, RIGHT_BUTTON, "Right"},
    {64, 85, DOWN_BUTTON, "Down"},
    {64, 110, SELECT_BUTTON, "Select"},
};

//*****************************************************************************
//
// This enumeration holds the various states that the mouse can be in during
// normal operation.
//
//*****************************************************************************
volatile enum
{
    //
    // Unconfigured.
    //
    MOUSE_STATE_UNCONFIGURED,

    //
    // No keys to send and not waiting on data.
    //
    MOUSE_STATE_IDLE,

    //
    // Waiting on data to be sent out.
    //
    MOUSE_STATE_SENDING
}
g_eMouseState = MOUSE_STATE_UNCONFIGURED;

//*****************************************************************************
//
// Graphics context used to show text on the color STN display.
//
//*****************************************************************************
tContext g_sContext;

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


unsigned long
MouseHandler(void *pvCBData, unsigned long ulEvent,
             unsigned long ulMsgData, void *pvMsgData)
{
    switch(ulEvent)
    {
        //
        // The USB host has connected to and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            DEBUG_PRINT("Host connected.\n");
            g_eMouseState = MOUSE_STATE_IDLE;
            g_bConnected = true;
            break;
        }

        //
        // The USB host has disconnected from the device.
        //
        case USB_EVENT_DISCONNECTED:
        {
            DEBUG_PRINT("Host disconnected.\n");
            g_bConnected = false;
            g_eMouseState = MOUSE_STATE_UNCONFIGURED;
            break;
        }

        //
        // A report was sent to the host. We are not free to send another.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            DEBUG_PRINT("TX complete.\n");
            g_eMouseState = MOUSE_STATE_IDLE;
            break;
        }

    }
    return(0);
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
        if (g_eMouseState == MOUSE_STATE_IDLE)
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

//*****************************************************************************
//
// This function updates the color STN display to show button state.
//
// This function is called from ButtonHandler to update the display showing
// the state of each of the buttons.
//
// Returns None.
//
//*****************************************************************************
void
UpdateDisplay(unsigned char ucButtons)
{
    unsigned long ulLoop;

    for(ulLoop = 0;
        ulLoop < (sizeof(g_psButtonDisplay) / sizeof(tButtonDisplay));
        ulLoop++)
    {
        //
        // Set the appropriate foreground color depending upon whether the
        // button is pressed or not.
        //
        if(g_psButtonDisplay[ulLoop].ucButton & ucButtons)
        {
            //
            // Button is not pressed.
            //
            GrContextForegroundSet(&g_sContext, ClrWhite);
        }
        else
        {
            //
            // Button is pressed.
            //
            GrContextForegroundSet(&g_sContext, ClrRed);
        }

        //
        // Draw the button name in the appropriate color.
        //
        GrStringDrawCentered(&g_sContext, g_psButtonDisplay[ulLoop].pcName, -1,
                             g_psButtonDisplay[ulLoop].sX,
                             g_psButtonDisplay[ulLoop].sY, true);
    }
}

//*****************************************************************************
//
// This function handles updates due to the buttons.
//
// This function is called from the main loop each time the buttons need to
// be checked.  If it detects an update it will schedule an transfer to the
// host.
//
// Returns Returns \b true on success or \b false if an error is detected.
//
//*****************************************************************************
tBoolean
ButtonHandler(void)
{
    unsigned char ucButtons, ucChanged, ucRepeat;
    unsigned long ulRetcode;
    char cDeltaX, cDeltaY;
    tBoolean bSuccess;

    //
    // Determine the state of the pushbuttons.
    //
    ucButtons = ButtonsPoll(&ucChanged, &ucRepeat);

    //
    // Update the display to show which buttons are currently pressed.
    //
    UpdateDisplay(ucButtons);

    //
    // If we are connected to the host and the select button changed state or
    // we see a repeat message from any of the direction buttons, go ahead and
    // send a mouse state change to the host.
    //
    if((ucChanged & SELECT_BUTTON) || (ucRepeat & ~SELECT_BUTTON))
    {
        //
        // Assume no change in the position until we determine otherwise.
        //
        cDeltaX = 0;
        cDeltaY = 0;

        //
        // Up button is held down
        //
        if(BUTTON_REPEAT(UP_BUTTON, ucRepeat))
        {
            cDeltaY = MOUSE_MOVE_DEC;
        }

        //
        // Down button is held down.
        //
        if(BUTTON_REPEAT(DOWN_BUTTON, ucRepeat))
        {
            cDeltaY = MOUSE_MOVE_INC;
        }

        //
        // Left button is held down.
        //
        if(BUTTON_REPEAT(LEFT_BUTTON, ucRepeat))
        {
            cDeltaX = MOUSE_MOVE_DEC;
        }

        //
        // Right button is held down
        //
        if(BUTTON_REPEAT(RIGHT_BUTTON, ucRepeat))
        {
            cDeltaX = MOUSE_MOVE_INC;
        }

        //
        // Tell the HID driver to send this new report for us.  Note that a 0
        // in ucButtons indicates that the relevant button is pressed so we
        // set the MOUSE_REPORT_BUTTON_1 bit if SELECT_BUTTON is clear in
        // ucButtons.
        //
        DEBUG_PRINT("Sending (0x%02x, 0x%02x), button %s.\n", cDeltaX,
                    cDeltaY, ((ucButtons & SELECT_BUTTON) ? "released" :
                    "pressed"));

        g_eMouseState = MOUSE_STATE_SENDING;
        ulRetcode = USBDHIDMouseStateChange((void *)&g_sMouseDevice, cDeltaX,
                                            cDeltaY,
                                            ((ucButtons & SELECT_BUTTON) ? 0 :
                                            MOUSE_REPORT_BUTTON_1));

        //
        // Did we schedule the report for transmission?
        //
        if(ulRetcode == MOUSE_SUCCESS)
        {
            //
            // Wait for the host to acknowledge the transmission if all went
            // well.
            //
            bSuccess = WaitForSendIdle(MAX_SEND_DELAY);

            //
            // Did we time out waiting for the packet to be sent?
            //
            if (!bSuccess)
            {
                //
                // Yes - assume the host disconnected and go back to
                // waiting for a new connection.
                //
                DEBUG_PRINT("Send timed out!\n");
                g_bConnected = 0;
            }
        }
        else
        {
            //
            // An error was reported when trying to send the report. This may
            // be due to host disconnection but could also be due to a clash
            // between our attempt to send a report and the driver sending the
            // last report in response to an idle timer timeout so we don't
            // jump to the conclusion that we were disconnected in this case.
            //
            DEBUG_PRINT("Can't send report.\n");
            bSuccess = false;
        }
    }
    else
    {
        //
        // There was no change in the state of the buttons so we have nothing
        // to do.
        //
        bSuccess = true;
    }

    return(bSuccess);
}

//*****************************************************************************
//
// This is the interrupt handler for the SysTick interrupt.  It is called
// periodically and updates a global tick counter then sets a flag to tell the
// main loop to check the button state.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    g_ulSysTickCount++;
    g_ulCommands |= BUTTON_TICK_EVENT;
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Enable the USB mux GPIO.
    //
    SysCtlPeripheralEnable(USB_MUX_GPIO_PERIPH);

    //
    // The LM3S3748 board uses a USB mux that must be switched to use the
    // host connector and not the device connecter.
    //
    GPIOPinTypeGPIOOutput(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN);
    GPIOPinWrite(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN, USB_MUX_SEL_DEVICE);

#ifdef DEBUG
    //
    // Configure the relevant pins such that UART0 owns them.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Open UART0 for debug output.
    //
    UARTStdioInit(0);
#endif

    //
    // Initialize the pushbuttons.
    //
    ButtonsInit();
    ButtonsSetAutoRepeat((LEFT_BUTTON | RIGHT_BUTTON | UP_BUTTON |
                          DOWN_BUTTON), 0, 2);

    //
    // Set the system tick to fire 100 times per second.
    //
    SysTickPeriodSet(SysCtlClockGet() / SYSTICKS_PER_SECOND);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Initialize the display driver.
    //
    Formike128x128x16Init();

    //
    // Turn on the backlight.
    //
    Formike128x128x16BacklightOn();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sFormike128x128x16);

    //
    // Fill the top 15 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = 14;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_pFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "usb_dev_mouse", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 7, 0);

    //
    // Pass the USB library our device information, initialize the USB
    // controller and connect the device to the bus.
    //
    USBDHIDMouseInit(0, (tUSBDHIDMouseDevice *)&g_sMouseDevice);

    //
    // Drop into the main loop.
    //
    while(1)
    {
        //
        // Fill all but the top 15 rows of the screen with black to erase the
        // previous status.
        //
        sRect.sXMin = 0;
        sRect.sYMin = 15;
        sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
        sRect.sYMax = GrContextDpyHeightGet(&g_sContext) - 1;
        GrContextForegroundSet(&g_sContext, ClrBlack);
        GrRectFill(&g_sContext, &sRect);

        //
        // Tell the user what we are doing.
        //
        GrContextForegroundSet(&g_sContext, ClrWhite);
        GrStringDrawCentered(&g_sContext, "Waiting for host...", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 24, true);
        //
        // Wait for USB configuration to complete.
        //
        while(!g_bConnected)
        {
        }

        //
        // Update the status.
        //
        GrStringDrawCentered(&g_sContext, " Host connected... ", -1,
                             GrContextDpyWidthGet(&g_sContext) / 2, 24, true);

        //
        // Now keep processing the mouse as long as the host is connected.
        //
        while(g_bConnected)
        {
            //
            // If it is time to check the button state then do so.
            //
            if(g_ulCommands & BUTTON_TICK_EVENT)
            {
                g_ulCommands &= ~BUTTON_TICK_EVENT;
                ButtonHandler();
            }
        }

        //
        // If we drop out of the previous loop, the host has disconnected so
        // go back and wait for a new connection.
        //
    }
}
