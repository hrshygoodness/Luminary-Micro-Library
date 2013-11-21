//*****************************************************************************
//
// qs_ek-lm3s1968.c - The quick start application for the LM3S1968 Evaluation
//                    Board.
//
// Copyright (c) 2006-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S1968 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/hibernate.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "drivers/class-d.h"
#include "drivers/rit128x96x4.h"
#include "game.h"
#include "globals.h"
#include "images.h"
#include "random.h"
#include "screen_saver.h"
#include "sounds.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>EK-LM3S1968 Quickstart Application (qs_ek-lm3s1968)</h1>
//!
//! A game in which a blob-like character tries to find its way out of a maze.
//! The character starts in the middle of the maze and must find the exit,
//! which will always be located at one of the four corners of the maze.  Once
//! the exit to the maze is located, the character is placed into the middle of
//! a new maze and must find the exit to that maze; this repeats endlessly.
//!
//! The game is started by pressing the select push button on the right side
//! of the board.  During game play, the select push button will fire a bullet
//! in the direction the character is currently facing, and the navigation push
//! buttons on the left side of the board will cause the character to walk in
//! the corresponding direction.
//!
//! Populating the maze are a hundred spinning stars that mindlessly attack the
//! character.  Contact with one of these stars results in the game ending, but
//! the stars go away when shot.
//!
//! Score is accumulated for shooting the stars and for finding the exit to the
//! maze.  The game lasts for only one character, and the score is displayed on
//! the virtual UART at 115,200, 8-N-1 during game play and will be displayed
//! on the screen at the end of the game.
//!
//! Since the OLED display on the evaluation board has burn-in characteristics
//! similar to a CRT, the application also contains a screen saver.  The screen
//! saver will only become active if two minutes have passed without the user
//! push button being pressed while waiting to start the game (that is, it will
//! never come on during game play).  Qix-style bouncing lines are drawn on the
//! display by the screen saver.
//!
//! After two minutes of running the screen saver, the processor will enter
//! hibernation mode, and the red LED will turn on.  Hibernation mode will be
//! exited by pressing the select push button.  The select push button will
//! then need to be pressed again to start the game.
//
//*****************************************************************************

//*****************************************************************************
//
// A set of flags used to track the state of the application.
//
//*****************************************************************************
unsigned long g_ulFlags;

//*****************************************************************************
//
// The speed of the processor clock, which is therefore the speed of the clock
// that is fed to the peripherals.
//
//*****************************************************************************
unsigned long g_ulSystemClock;

//*****************************************************************************
//
// Storage for a local frame buffer.
//
//*****************************************************************************
unsigned char g_pucFrame[6144];

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
// The number of clock ticks that have occurred.  This is used as an entropy
// source for the random number generator; the number of clock ticks at the
// time of a button press or release is an entropic event.
//
//*****************************************************************************
static unsigned long g_ulTickCount = 0;

//*****************************************************************************
//
// The number of clock ticks that have occurred since the last screen update
// was requested.  This is used to divide down the system clock tick to the
// desired screen update rate.
//
//*****************************************************************************
static unsigned char g_ucScreenUpdateCount = 0;

//*****************************************************************************
//
// The number of clock ticks that have occurred since the last application
// update was performed.  This is used to divide down the system clock tick to
// the desired application update rate.
//
//*****************************************************************************
static unsigned char g_ucAppUpdateCount = 0;

//*****************************************************************************
//
// The debounced state of the five push buttons.  The bit positions correspond
// to:
//
//     0 - Up
//     1 - Down
//     2 - Left
//     3 - Right
//     4 - Select
//
//*****************************************************************************
unsigned char g_ucSwitches = 0x1f;

//*****************************************************************************
//
// The vertical counter used to debounce the push buttons.  The bit positions
// are the same as g_ucSwitches.
//
//*****************************************************************************
static unsigned char g_ucSwitchClockA = 0;
static unsigned char g_ucSwitchClockB = 0;

//*****************************************************************************
//
// Handles the SysTick timeout interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    unsigned long ulData, ulDelta;

    //
    // Increment the tick count.
    //
    g_ulTickCount++;

    //
    // Indicate that a timer interrupt has occurred.
    //
    HWREGBITW(&g_ulFlags, FLAG_CLOCK_TICK) = 1;

    //
    // Increment the screen update count.
    //
    g_ucScreenUpdateCount++;

    //
    // See if 1/30th of a second has passed since the last screen update.
    //
    if(g_ucScreenUpdateCount == (CLOCK_RATE / 30))
    {
        //
        // Restart the screen update count.
        //
        g_ucScreenUpdateCount = 0;

        //
        // Request a screen update.
        //
        HWREGBITW(&g_ulFlags, FLAG_UPDATE) = 1;
    }

    //
    // Increment the application update count.
    //
    g_ucAppUpdateCount++;

    //
    // See if 1/100th of a second has passed since the last application update.
    //
    if(g_ucAppUpdateCount != (CLOCK_RATE / 100))
    {
        //
        // Return without doing any further processing.
        //
        return;
    }

    //
    // Restart the application update count.
    //
    g_ucAppUpdateCount = 0;

    //
    // Read the state of the push buttons.
    //
    ulData = ((GPIOPinRead(GPIO_PORTG_BASE, (GPIO_PIN_3 | GPIO_PIN_4 |
                                            GPIO_PIN_5 | GPIO_PIN_6)) >> 3) |
              (GPIOPinRead(GPIO_PORTG_BASE, GPIO_PIN_7) >> 3));

    //
    // Determine the switches that are at a different state than the debounced
    // state.
    //
    ulDelta = ulData ^ g_ucSwitches;

    //
    // Increment the clocks by one.
    //
    g_ucSwitchClockA ^= g_ucSwitchClockB;
    g_ucSwitchClockB = ~g_ucSwitchClockB;

    //
    // Reset the clocks corresponding to switches that have not changed state.
    //
    g_ucSwitchClockA &= ulDelta;
    g_ucSwitchClockB &= ulDelta;

    //
    // Get the new debounced switch state.
    //
    g_ucSwitches &= g_ucSwitchClockA | g_ucSwitchClockB;
    g_ucSwitches |= (~(g_ucSwitchClockA | g_ucSwitchClockB)) & ulData;

    //
    // Determine the switches that just changed debounced state.
    //
    ulDelta ^= (g_ucSwitchClockA | g_ucSwitchClockB);

    //
    // See if any switches just changed debounced state.
    //
    if(ulDelta)
    {
        //
        // Add the current tick count to the entropy pool.
        //
        RandomAddEntropy(g_ulTickCount);
    }

    //
    // See if the select button was just pressed.
    //
    if((ulDelta & 0x10) && !(g_ucSwitches & 0x10))
    {
        //
        // Set a flag to indicate that the select button was just pressed.
        //
        HWREGBITW(&g_ulFlags, FLAG_BUTTON_PRESS) = 1;
    }
}

//*****************************************************************************
//
// Delay for a multiple of the system tick clock rate.
//
//*****************************************************************************
static void
Delay(unsigned long ulCount)
{
    //
    // Loop while there are more clock ticks to wait for.
    //
    while(ulCount--)
    {
        //
        // Wait until a SysTick interrupt has occurred.
        //
        while(!HWREGBITW(&g_ulFlags, FLAG_CLOCK_TICK))
        {
        }

        //
        // Clear the SysTick interrupt flag.
        //
        HWREGBITW(&g_ulFlags, FLAG_CLOCK_TICK) = 0;
    }
}

//*****************************************************************************
//
// Displays a logo for a specified amount of time.
//
//*****************************************************************************
static void
DisplayLogo(const unsigned char *pucLogo, unsigned long ulWidth,
            unsigned long ulHeight, unsigned long ulDelay)
{
    unsigned char *pucDest, ucHigh, ucLow;
    unsigned long ulLoop1, ulLoop2;
    const unsigned char *pucSrc;
    long lIdx;

    //
    // Loop through thirty two intensity levels to fade the logo in from black.
    //
    for(lIdx = 1; lIdx <= 32; lIdx++)
    {
        //
        // Clear the local frame buffer.
        //
        for(ulLoop1 = 0; ulLoop1 < sizeof(g_pucFrame); ulLoop1 += 4)
        {
            *(unsigned long *)(g_pucFrame + ulLoop1) = 0;
        }

        //
        // Get a pointer to the beginning of the logo.
        //
        pucSrc = pucLogo;

        //
        // Get a point to the upper left corner of the frame buffer where the
        // logo will be placed.
        //
        pucDest = (g_pucFrame + (((96 - ulHeight) / 2) * 64) +
                   ((128 - ulWidth) / 4));

        //
        // Copy the logo into the frame buffer, scaling the intensity.  Loop
        // over the rows.
        //
        for(ulLoop1 = 0; ulLoop1 < ulHeight; ulLoop1++)
        {
            //
            // Loop over the columns.
            //
            for(ulLoop2 = 0; ulLoop2 < (ulWidth / 2); ulLoop2++)
            {
                //
                // Get the two nibbles of the next byte from the source.
                //
                ucHigh = pucSrc[ulLoop2] >> 4;
                ucLow = pucSrc[ulLoop2] & 15;

                //
                // Scale the intensity of the two nibbles.
                //
                ucHigh = ((unsigned long)ucHigh * lIdx) / 32;
                ucLow = ((unsigned long)ucLow * lIdx) / 32;

                //
                // Write the two nibbles to the frame buffer.
                //
                pucDest[ulLoop2] = (ucHigh << 4) | ucLow;
            }

            //
            // Increment to the next row of the source and destination.
            //
            pucSrc += (ulWidth / 2);
            pucDest += 64;
        }

        //
        // Wait until an update has been requested.
        //
        while(HWREGBITW(&g_ulFlags, FLAG_UPDATE) == 0)
        {
        }

        //
        // Clear the update request flag.
        //
        HWREGBITW(&g_ulFlags, FLAG_UPDATE) = 0;

        //
        // Display the local frame buffer on the display.
        //
        RIT128x96x4ImageDraw(g_pucFrame, 0, 0, 128, 96);
    }

    //
    // Delay for the specified time while the logo is displayed.
    //
    Delay(ulDelay);

    //
    // Loop through the thirty two intensity levels to face the logo back to
    // black.
    //
    for(lIdx = 31; lIdx >= 0; lIdx--)
    {
        //
        // Clear the local frame buffer.
        //
        for(ulLoop1 = 0; ulLoop1 < sizeof(g_pucFrame); ulLoop1 += 4)
        {
            *(unsigned long *)(g_pucFrame + ulLoop1) = 0;
        }

        //
        // Get a pointer to the beginning of the logo.
        //
        pucSrc = pucLogo;

        //
        // Get a point to the upper left corner of the frame buffer where the
        // logo will be placed.
        //
        pucDest = (g_pucFrame + (((96 - ulHeight) / 2) * 64) +
                   ((128 - ulWidth) / 4));

        //
        // Copy the logo into the frame buffer, scaling the intensity.  Loop
        // over the rows.
        //
        for(ulLoop1 = 0; ulLoop1 < ulHeight; ulLoop1++)
        {
            //
            // Loop over the columns.
            //
            for(ulLoop2 = 0; ulLoop2 < (ulWidth / 2); ulLoop2++)
            {
                //
                // Get the two nibbles of the next byte from the source.
                //
                ucHigh = pucSrc[ulLoop2] >> 4;
                ucLow = pucSrc[ulLoop2] & 15;

                //
                // Scale the intensity of the two nibbles.
                //
                ucHigh = ((unsigned long)ucHigh * lIdx) / 32;
                ucLow = ((unsigned long)ucLow * lIdx) / 32;

                //
                // Write the two nibbles to the frame buffer.
                //
                pucDest[ulLoop2] = (ucHigh << 4) | ucLow;
            }

            //
            // Increment to the next row of the source and destination.
            //
            pucSrc += (ulWidth / 2);
            pucDest += 64;
        }

        //
        // Wait until an update has been requested.
        //
        while(HWREGBITW(&g_ulFlags, FLAG_UPDATE) == 0)
        {
        }

        //
        // Clear the update request flag.
        //
        HWREGBITW(&g_ulFlags, FLAG_UPDATE) = 0;

        //
        // Display the local frame buffer on the display.
        //
        RIT128x96x4ImageDraw(g_pucFrame, 0, 0, 128, 96);
    }
}

//*****************************************************************************
//
// The main code for the application.  It sets up the peripherals, displays the
// splash screens, and then manages the interaction between the game and the
// screen saver.
//
//*****************************************************************************
int
main(void)
{
    unsigned char bSkipIntro;

    //
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    // a workaround to allow the PLL to operate reliably.
    //
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }

    //
    // Set the clocking to run at 50MHz from the PLL.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);
    SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    //
    // Get the system clock speed.
    //
    g_ulSystemClock = SysCtlClockGet();

    //
    // Enable the peripherals used by the application.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure the GPIOs used to read the state of the on-board push buttons.
    //
    GPIOPinTypeGPIOInput(GPIO_PORTG_BASE,
                         GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 |
                         GPIO_PIN_7);
    GPIOPadConfigSet(GPIO_PORTG_BASE,
                     GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 |
                     GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    //
    // Configure the LED, speaker, and UART GPIOs as required.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinTypePWM(GPIO_PORTH_BASE, GPIO_PIN_1);
    GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_2);
    GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_2, 0);

    //
    // Check to see if this is a wake from hibernation.  If so, then skip past
    // the introductory audio and splash screen.
    //
    if(HibernateIsActive() && (HibernateIntStatus(0) & HIBERNATE_INT_PIN_WAKE))
    {
        bSkipIntro = 1;
    }
    else
    {
        bSkipIntro = 0;
    }

    //
    // Initialize the Hibernation module, which is used for storing the high
    // score.
    //
    HibernateEnableExpClk(SysCtlClockGet());
    HibernateClockSelect(HIBERNATE_CLOCK_SEL_DIV128);

    //
    // Configure the first UART for 115,200, 8-N-1 operation.
    //
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                         UART_CONFIG_PAR_NONE));
    UARTEnable(UART0_BASE);

    //
    // Send a welcome message to the UART.
    //
    UARTCharPut(UART0_BASE, 'W');
    UARTCharPut(UART0_BASE, 'e');
    UARTCharPut(UART0_BASE, 'l');
    UARTCharPut(UART0_BASE, 'c');
    UARTCharPut(UART0_BASE, 'o');
    UARTCharPut(UART0_BASE, 'm');
    UARTCharPut(UART0_BASE, 'e');
    UARTCharPut(UART0_BASE, '\r');
    UARTCharPut(UART0_BASE, '\n');

    //
    // Initialize the OSRAM OLED display.
    //
    RIT128x96x4Init(3500000);

    //
    // Initialize the Class-D audio driver.
    //
    ClassDInit(g_ulSystemClock);

    //
    // Wait until the Class-D audio driver has completed its startup.
    //
    while(ClassDBusy())
    {
    }

    //
    // Configure SysTick to periodically interrupt.
    //
    SysTickPeriodSet(g_ulSystemClock / CLOCK_RATE);
    SysTickIntEnable();
    SysTickEnable();

    //
    // If this is a wake from hibernation, then skip the splash screen and
    // audio.
    //
    if(!bSkipIntro)
    {
        //
        // Delay for a bit to allow the initial display flash to subside.
        //
        Delay(CLOCK_RATE / 4);

        //
        // Play the intro music.
        //
        ClassDPlayADPCM(g_pucIntro, sizeof(g_pucIntro));

        //
        // Display the Texas Instruments logo for five seconds (or twelve
        // seconds if using gcc).
        //
#if defined(gcc)
        DisplayLogo(g_pucTILogo, 120, 42, 12 * CLOCK_RATE);
#else
        DisplayLogo(g_pucTILogo, 120, 42, 5 * CLOCK_RATE);
#endif

        //
        // Display the Code Composer Studio logo for five seconds.
        //
#if defined(ccs)
        DisplayLogo(g_pucCodeComposer, 128, 34, 5 * CLOCK_RATE);
#endif

        //
        // Display the Keil/ARM logo for five seconds.
        //
#if defined(rvmdk) || defined(__ARMCC_VERSION)
        DisplayLogo(g_pucKeilLogo, 128, 40, 5 * CLOCK_RATE);
#endif

        //
        // Display the IAR logo for five seconds.
        //
#if defined(ewarm)
        DisplayLogo(g_pucIarLogo, 102, 61, 5 * CLOCK_RATE);
#endif

        //
        // Display the CodeSourcery logo for five seconds.
        //
#if defined(sourcerygxx)
        DisplayLogo(g_pucCodeSourceryLogo, 128, 34, 5 * CLOCK_RATE);
#endif

        //
        // Display the CodeRed logo for five seconds.
        //
#if defined(codered)
        DisplayLogo(g_pucCodeRedLogo, 128, 32, 5 * CLOCK_RATE);
#endif
    }

    //
    // Set bit for select switch, to make sure that the button press to wake
    // does not start the game.  This will require the button to be released,
    // then pressed in order to start the game.
    //
    g_ucSwitches &= ~0x10;

    //
    // Throw away any button presses that may have occurred while the splash
    // screens were being displayed.
    //
    HWREGBITW(&g_ulFlags, FLAG_BUTTON_PRESS) = 0;

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Display the main screen.
        //
        if(MainScreen())
        {
            //
            // The button was pressed, so start the game.
            //
            PlayGame();
        }
        else
        {
            //
            // The button was not pressed during the timeout period, so start
            // the screen saver.
            //
            ScreenSaver();
        }
    }
}
