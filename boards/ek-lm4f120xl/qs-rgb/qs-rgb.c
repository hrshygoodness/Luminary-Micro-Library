//*****************************************************************************
//
// qs-rgb.c - Quickstart for the EK-LM4F120XL Stellaris LaunchPad
//
// Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
//
//*****************************************************************************

#include <math.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_hibernate.h"
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/hibernate.h"
#include "utils/uartstdio.h"
#include "utils/cmdline.h"
#include "drivers/rgb.h"
#include "drivers/buttons.h"
#include "rgb_commands.h"
#include "qs-rgb.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>EK-LM4F120XL Quickstart Application (qs-rgb)</h1>
//!
//! A demonstration of the Stellaris LaunchPad (EK-LM4F120XL) capabilities.
//!
//! Press and/or hold the left button traverse toward the red end of the
//! ROYGBIV color spectrum.  Press and/or hold the right button to traverse
//! toward the violet end of the ROYGBIV color spectrum.
//!
//! Leave idle for 5 seconds to see a automatically changing color display
//!
//! Press and hold both left and right buttons for 3 seconds to enter
//! hibernation.  During hibernation last color on screen will blink on the
//! LED for 0.5 seconds every 3 seconds.
//!
//! Command line UART protocol can also control the system.
//!
//! Command 'help' to generate list of commands and helpful information.
//! Command 'hib' will place the device into hibernation mode.
//! Command 'rand' will initiate the pseudo-random sequence.
//! Command 'intensity' followed by a number between 0.0 and 1.0 will scale
//! the brightness of the LED by that factor.
//! Command 'rgb' followed by a six character hex value will set the color. For
//! example 'rgb FF0000' will produce a red color.
//
//*****************************************************************************


//*****************************************************************************
//
// Entry counter to track how long to stay in certain staging states before 
// making transition into hibernate.
//
//*****************************************************************************
static volatile unsigned long ulHibModeEntryCount;

//*****************************************************************************
//
// Array of pre-defined colors for use when buttons cause manual color steps.
//
//*****************************************************************************
static float fManualColors[7] = {0.0f, .214f, .428f, .642f, .856f, 1.07f, 
                                 1.284f};

//*****************************************************************************
//
// Input buffer for the command line interpreter.
//
//*****************************************************************************
static char g_cInput[APP_INPUT_BUF_SIZE];

//*****************************************************************************
//
// Application state structure.  Gets stored to hibernate memory for 
// preservation across hibernate events.  
//
//*****************************************************************************
volatile sAppState_t g_sAppState;

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
// Handler to manage the button press events and state machine transitions
// that result from those button events.
//
// This function is called by the SysTickIntHandler if a button event is 
// detected. Function will determine which button was pressed and tweak various
// elements of the global state structure accordingly.
//
//*****************************************************************************
void 
AppButtonHandler(void)
{
    static unsigned long ulTickCounter;
    
    ulTickCounter++;

    //
    // Switch statement to adjust the color wheel position based on buttons
    //
    switch(g_sAppState.ulButtons & ALL_BUTTONS)
    {
    
    case LEFT_BUTTON:

        //
        // Check if the button has been held long enough to peform another
        // color wheel increment.
        //
        if((ulTickCounter % APP_BUTTON_POLL_DIVIDER) == 0)
        {
            //
            // Perform the increment and index wrap around.
            //
            g_sAppState.ulManualIndex++;
            if(g_sAppState.ulManualIndex >= APP_NUM_MANUAL_COLORS)
            {
                g_sAppState.ulManualIndex = 0;
            }
            g_sAppState.fColorWheelPos = APP_PI *
                                      fManualColors[g_sAppState.ulManualIndex];
        }

        //
        // Reset some state counts and system mode so that we know the user 
        // is present and actively engaging with the application.
        //
        ulHibModeEntryCount = 0;
        g_sAppState.ulModeTimer = 0;
        g_sAppState.ulMode = APP_MODE_NORMAL;
        break;

    case RIGHT_BUTTON:

        //
        // Check if the button has been held long enough to perform another 
        // color wheel decrement.
        //
        if((ulTickCounter % APP_BUTTON_POLL_DIVIDER) == 0)
        {
            //
            // Perform the decrement and index wrap around.
            //
            if(g_sAppState.ulManualIndex == 0)
            {
                //
                // set to one greater than the last color so that we decrement
                // back into range with next instruction.
                //
                g_sAppState.ulManualIndex = APP_NUM_MANUAL_COLORS;
            }
            g_sAppState.ulManualIndex--;
            g_sAppState.fColorWheelPos = APP_PI *
                                      fManualColors[g_sAppState.ulManualIndex];
        }
        //
        // Reset some state counts and system mode so that we know the user 
        // is present and actively engaging with the application.
        //
        ulHibModeEntryCount = 0;
        g_sAppState.ulModeTimer = 0;
        g_sAppState.ulMode = APP_MODE_NORMAL;
        break;

    case ALL_BUTTONS:

        //
        // Both buttons for longer than debounce time will cause hibernation
        //
        if(ulHibModeEntryCount < APP_HIB_BUTTON_DEBOUNCE)
        {
            ulHibModeEntryCount++;
            g_sAppState.ulMode = APP_MODE_NORMAL;
        }
        else
        {
            g_sAppState.ulMode = APP_MODE_HIB;
        }
        g_sAppState.ulModeTimer = 0;
        break;

    default:
        if(g_sAppState.ulMode == APP_MODE_HIB_FLASH)
        {
            //
            // Waking from hibernate RTC just do a quick flash then back to 
            // hibernation.
            //
            if(ulHibModeEntryCount < APP_HIB_FLASH_DURATION)
            {
                ulHibModeEntryCount++;
            }
            else
            {
                g_sAppState.ulMode = APP_MODE_HIB;
            }
        }    
        else
        {
            //
            // Normal or remote mode and no user action will cause transition
            // to automatic scrolling mode.
            //
            ulHibModeEntryCount = 0;
            if(g_sAppState.ulModeTimer < APP_AUTO_MODE_TIMEOUT)
            {
                g_sAppState.ulModeTimer++;
            }
            else
            {
                g_sAppState.ulMode = APP_MODE_AUTO;
            }

            //
            // reset the tick counter when no buttons are pressed
            // this makes the first button reaction speed quicker
            //
            ulTickCounter = APP_BUTTON_POLL_DIVIDER - 1;
        }
        break;
    }
}

//*****************************************************************************
//
// Uses the fColorWheelPos variable to update the color mix shown on the RGB
//
// ulForceUpdate when set forces a color update even if a color change
// has not been detected.  Used primarily at startup to init the color after
// a hibernate.
//
// This function is called by the SysTickIntHandler to update the colors on
// the RGB LED whenever a button or timeout event has changed the color wheel
// position.  Color is determined by a series of sine functions and conditions
//
//*****************************************************************************
void 
AppRainbow(unsigned long ulForceUpdate)
{
    static float fPrevPos;
    float fCurPos;
    float fTemp;
    volatile unsigned long * pulColors;
    
    pulColors = g_sAppState.ulColors;
    fCurPos = g_sAppState.fColorWheelPos;


    if((fCurPos != fPrevPos) || ulForceUpdate)
    {
        //
        // Preserve the new color wheel position 
        //
        fPrevPos = fCurPos;
        
        //
        // Adjust the BLUE value based on the control state
        //
        fTemp = 65535.0f * sinf(fCurPos);
        if(fTemp < 0)
        {
            pulColors[GREEN] = 0;
        }
        else
        {
            pulColors[GREEN] = (unsigned long) fTemp;
        }


        //
        // Adjust the RED value based on the control state
        //
        fTemp = 65535.0f * sinf(fCurPos - APP_PI / 2.0f);
        if(fTemp < 0)
        {
            pulColors[BLUE] = 0;
        }
        else
        {
            pulColors[BLUE] = (unsigned long) fTemp;
        }


        //
        // Adjust the GREEN value based on the control state
        //
        if(fCurPos < APP_PI)
        {
            fTemp = 65535.0f * sinf(fCurPos + APP_PI * 0.5f);
        }
        else
        {
            fTemp = 65535.0f * sinf(fCurPos + APP_PI);
        }
        if(fTemp < 0)
        {
            pulColors[RED] = 0;
        }
        else
        {
            pulColors[RED] = (unsigned long) fTemp;
        }
        
        //
        // Update the actual LED state
        //
        RGBColorSet(pulColors);
    }

}

//*****************************************************************************
//
// Called by the NVIC as a result of SysTick Timer rollover interrupt flag
//
// Checks buttons and calls AppButtonHandler to manage button events.
// Tracks time and auto mode color stepping.  Calls AppRainbow to implement
// RGB color changes.
//
//*****************************************************************************
void 
SysTickIntHandler(void)
{
    
    static float x;

    g_sAppState.ulButtons = ButtonsPoll(0,0);
    AppButtonHandler();

    //
    // Auto increment the color wheel if in the AUTO mode. AUTO mode is when
    // device is active but user interaction has timed out.
    // 
    if(g_sAppState.ulMode == APP_MODE_AUTO)
    {
        g_sAppState.fColorWheelPos += APP_AUTO_COLOR_STEP;
    }

    //
    // Provide wrap around of the control variable from 0 to 1.5 times PI
    //
    if(g_sAppState.fColorWheelPos > (APP_PI * 1.5f))
    {
        g_sAppState.fColorWheelPos = 0.0f;
    }
    if(x < 0.0f)
    {
        g_sAppState.fColorWheelPos = APP_PI * 1.5f;
    }

    //
    //    Set the RGB Color based on current control variable value.
    //
    AppRainbow(0);


}

//*****************************************************************************
//
// Uses the fColorWheelPos variable to update the color mix shown on the RGB
//
// This function is called when system has decided it is time to enter
// Hibernate.  This will prepare the hibernate peripheral, save the system
// state and then enter hibernate mode.
//
//*****************************************************************************
void 
AppHibernateEnter(void)
{
    //
    // Alert UART command line users that we are going to hibernate
    //
    UARTprintf("Entering Hibernate...\n");
    
    //
    // Prepare Hibernation Module
    //
    HibernateGPIORetentionEnable();
    HibernateRTCSet(0);
    HibernateRTCEnable();
    HibernateRTCMatch0Set(5);
    HibernateWakeSet(HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RTC);

    //
    // Store state information to battery backed memory
    // since sizeof returns number of bytes we convert to words and force
    // a rounding up to next whole word.
    //
    HibernateDataSet((unsigned long*)&g_sAppState, sizeof(sAppState_t)/4+1);

    //
    // Disable the LED for 100 milliseconds to let user know we are
    // ready for hibernate and will hibernate on relase of buttons
    //
    RGBDisable();
    SysCtlDelay(SysCtlClockGet()/3/10);
    RGBEnable();

    //
    // Wait for wake button to be released prior to going into hibernate
    // 
    while(g_sAppState.ulButtons & RIGHT_BUTTON)
    {
        //
        //Delay for about 300 clock ticks to allow time for interrupts to 
        //sense that button is released
        //
        SysCtlDelay(100);
    }
    
    //
    // Disable the LED for power savings and go to hibernate mode
    //
    RGBDisable();
    HibernateRequest();


}

//*****************************************************************************
//
// Main function performs init and manages system.
//
// Called automatically after the system and compiler pre-init sequences.
// Performs system init calls, restores state from hibernate if needed and 
// then manages the application context duties of the system.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulStatus;
    unsigned long ulResetCause;
    long lCommandStatus;
    
    //
    // Enable stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPUEnable();
    ROM_FPUStackingEnable();

    //
    // Set the system clock to run at 40Mhz off PLL with external crystal as 
    // reference.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Enable the hibernate module
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);

    //
    // Enable and Initialize the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);

    UARTprintf("Welcome to the Stellaris LM4F120 LaunchPad!\n");
    UARTprintf("Type 'help' for a list of commands\n");
    UARTprintf("> ");

    //
    // Determine why system reset occurred and respond accordingly.
    // 
    ulResetCause = SysCtlResetCauseGet();
    SysCtlResetCauseClear(ulResetCause);
    if(ulResetCause == SYSCTL_CAUSE_POR)
    {
        if(HibernateIsActive())
        {
            //
            // Read the status bits to see what caused the wake.
            //
            ulStatus = HibernateIntStatus(0);
            HibernateIntClear(ulStatus);

            //
            // Wake was due to the push button.
            //
            if(ulStatus & HIBERNATE_INT_PIN_WAKE)
            {
                UARTprintf("Hibernate Wake Pin Wake Event\n");
                UARTprintf("> ");

                //
                // Recover the application state variables from battery backed
                // hibernate memory.  Set ulMode to normal.
                //
                HibernateDataGet((unsigned long*) &g_sAppState,
                                 sizeof(sAppState_t) / 4 + 1);
                g_sAppState.ulMode = APP_MODE_NORMAL;
            }

            //
            // Wake was due to RTC match
            //
            else if(ulStatus & HIBERNATE_INT_RTC_MATCH_0)
            {
                UARTprintf("Hibernate RTC Wake Event\n");
                UARTprintf("> ");
                //
                // Recover the application state variables from battery backed
                // hibernate memory. Set ulMode to briefly flash the RGB.
                //
                HibernateDataGet((unsigned long*) &g_sAppState,
                                sizeof(sAppState_t) / 4 + 1);
                g_sAppState.ulMode = APP_MODE_HIB_FLASH;
            }
        }

        else
        {
            //
            // Reset was do to a cold first time power up.
            // 
            UARTprintf("Power on reset. Hibernate not active.\n");
            UARTprintf("> ");

            g_sAppState.ulMode = APP_MODE_NORMAL;
            g_sAppState.fColorWheelPos = 0;
            g_sAppState.fIntensity = APP_INTENSITY_DEFAULT;
            g_sAppState.ulButtons = 0;
        }
    }
    else
    {
        // 
        // External Pin reset or other reset event occured.
        // 
        UARTprintf("External or other reset\n");
        UARTprintf("> ");
        
        //
        // Treat this as a cold power up reset without restore from hibernate.
        //
        g_sAppState.ulMode = APP_MODE_NORMAL;
        g_sAppState.fColorWheelPos = APP_PI;
        g_sAppState.fIntensity = APP_INTENSITY_DEFAULT;
        g_sAppState.ulButtons = 0;
        
    //
        // colors get a default initialization later when we call AppRainbow.
        //
    }
    
    //
    // Initialize clocking for the Hibernate module
    //
    HibernateEnableExpClk(SysCtlClockGet());

    //
    // Initialize the RGB LED. AppRainbow typically only called from interrupt
    // context. Safe to call here to force initial color update because 
    // interrupts are not yet enabled.
    //
    RGBInit(0);
    RGBIntensitySet(g_sAppState.fIntensity);
    AppRainbow(1);
    RGBEnable();
    
    //
    // Initialize the buttons
    //
    ButtonsInit();

    //
    // Initialize the SysTick interrupt to process colors and buttons.
    //
    SysTickPeriodSet(SysCtlClockGet() / APP_SYSTICKS_PER_SEC);
    SysTickEnable();
    SysTickIntEnable();
    IntMasterEnable();

    //
    // spin forever and wait for carriage returns or state changes.
    //
    while(1)
    {

        UARTprintf("\n>");

        
        //
        // Peek to see if a full command is ready for processing
        //
        while(UARTPeek('\r') == -1)
        {
            //
            // millisecond delay.  A SysCtlSleep() here would also be OK.
            //
            SysCtlDelay(SysCtlClockGet() / (1000 / 3));
            
            //
            // Check for change of mode and enter hibernate if requested.
            // all other mode changes handled in interrupt context.
            //
            if(g_sAppState.ulMode == APP_MODE_HIB)
            {
                AppHibernateEnter();
            }
        }
        
        //
        // a '\r' was detected get the line of text from the user.
        //
        UARTgets(g_cInput,sizeof(g_cInput));

        //
        // Pass the line from the user to the command processor.
        // It will be parsed and valid commands executed.
        //
        lCommandStatus = CmdLineProcess(g_cInput);

        //
        // Handle the case of bad command.
        //
        if(lCommandStatus == CMDLINE_BAD_CMD)
        {
            UARTprintf("Bad command!\n");
        }

        //
        // Handle the case of too many arguments.
        //
        else if(lCommandStatus == CMDLINE_TOO_MANY_ARGS)
        {
            UARTprintf("Too many arguments for command processor!\n");
        }
    }
}
