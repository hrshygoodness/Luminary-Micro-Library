//*****************************************************************************
//
// sound_demo.c - Example to play sounds using EVALBOT audio system
//
// Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ethernet.h"
#include "driverlib/ethernet.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "driverlib/udma.h"
#include "drivers/display96x16x1.h"
#include "drivers/io.h"
#include "drivers/sensors.h"
#include "drivers/sound.h"
#include "drivers/wav.h"
#include "sounds.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Sound Demo (sound_demo)</h1>
//!
//! This example application demonstrates the use of the EVALBOT audio system
//! by cycling through a set of audio clips and playing them using the buttons
//! and bump sensors.
//!
//! When the application starts, a message displays for several seconds.  Once
//! the message is finished, the display shows the name of the first audio
//! clip.  You can cycle through the available audio clips by pressing the left
//! and right bump sensors.  The name of each clip selected is shown on the
//! display.
//!
//! A clip can be played by pressing the front-most, right user button.  While
//! the clip is playing, it can be stopped by pressing the rear-most, right
//! user button.
//
//*****************************************************************************

//*****************************************************************************
//
// Defines the possible states for the motor state machine.
//
//*****************************************************************************
typedef enum
{
    STATE_STOPPED,
    STATE_PLAYING,
} tSoundState;

typedef struct
{
    const unsigned char *pucWav;
    const char * pcName;
} tWaveClip;

static const tWaveClip sWaveClips[] =
{
    { g_ucSoundWav1, "Intro" },
    { g_ucSoundWav2, "Boing" },
    { g_ucSoundWav3, "Crash" },
    { g_ucSoundWav4, "Horn" },
    { g_ucSoundWav5, "Note" },
};
#define NUM_WAVES (sizeof(sWaveClips) / sizeof(tWaveClip))

//*****************************************************************************
//
// Counter for the 10 ms system clock ticks.  Used for tracking time.
//
//*****************************************************************************
static volatile unsigned long g_ulTickCount;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
    for(;;)
    {
    }
}
#endif

//*****************************************************************************
//
// The DMA control structure table.  This is required by the sound driver.
//
//*****************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable sDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(sDMAControlTable, 1024)
tDMAControlTable sDMAControlTable[64];
#else
tDMAControlTable sDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The interrupt handler for the SysTick timer.  This handler will increment a
// tick counter to keep track of time, toggle the LEDs, and call the button
// and bump sensor debouncers.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Increment the tick counter.
    //
    g_ulTickCount++;

    //
    // Every 20 ticks (200 ms), toggle the LEDs
    //
    if((g_ulTickCount % 20) == 0)
    {
        LED_Toggle(BOTH_LEDS);
    }

    //
    // Call the user button and bump sensor debouncing functions, which must
    // be called periodically.
    //
    PushButtonDebouncer();
    BumpSensorDebouncer();
}

//*****************************************************************************
//
// The main application.  It configures the board and then enters a loop
// to process the button and sensor presses and run the motor.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulPHYMR0;
    tBoolean bTopButtonWasPressed = false;
    tBoolean bBottomButtonWasPressed = false;
    tBoolean bLeftBumperWasPressed = false;
    tBoolean bRightBumperWasPressed = false;
    tSoundState sSoundState = STATE_STOPPED;
    tWaveHeader sWaveHeader;
    unsigned long ulWaveIndex = 0;
    char cTimeString[16];

    //
    // Set the system clock to run at 50MHz
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Since the Ethernet is not used, power down the PHY to save battery.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
    ulPHYMR0 = ROM_EthernetPHYRead(ETH_BASE, PHY_MR0);
    ROM_EthernetPHYWrite(ETH_BASE, PHY_MR0, ulPHYMR0 | PHY_MR0_PWRDN);

    //
    // Initialize the board display
    //
    Display96x16x1Init(true);
    Display96x16x1DisplayOn();

    //
    // Print a simple message to the display.
    // Wait a few seconds so the message remains on the display.
    //
    Display96x16x1StringDraw("SOUND", 29, 0);
    Display96x16x1StringDraw("DEMO", 31, 1);
    SysCtlDelay(ROM_SysCtlClockGet());
    Display96x16x1Clear();

    //
    // Initialize the LED driver, then turn one LED on
    //
    LEDsInit();
    LED_On(LED_1);

    //
    // Initialize the buttons driver
    //
    PushButtonsInit();

    //
    // Initialize the bump sensor driver
    //
    BumpSensorsInit();

    //
    // Initialize the sound driver
    //
    SoundInit();

    //
    // Show the name of the first audio clip on the display
    //
    Display96x16x1StringDrawCentered(sWaveClips[ulWaveIndex].pcName, 0, true);
    Display96x16x1StringDrawCentered("0:00/0:00", 1, true);

    //
    // Set up and enable the SysTick timer to use as a time reference.
    // It will be set up for a 10 ms tick.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 100);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enter loop to run forever, processing button presses to make the
    // motors run, pause, and stop.
    //
    for(;;)
    {
        tBoolean bTopButtonIsPressed;
        tBoolean bBottomButtonIsPressed;
        tBoolean bLeftBumperIsPressed;
        tBoolean bRightBumperIsPressed;

        bTopButtonIsPressed = !PushButtonGetDebounced(BUTTON_1);
        bBottomButtonIsPressed = !PushButtonGetDebounced(BUTTON_2);
        bLeftBumperIsPressed = !BumpSensorGetDebounced(BUMP_LEFT);
        bRightBumperIsPressed = !BumpSensorGetDebounced(BUMP_RIGHT);

        switch(sSoundState)
        {
            case STATE_STOPPED:
            {
                //
                // If the top button was pressed, then start playing the sound
                //
                if(bTopButtonIsPressed & !bTopButtonWasPressed)
                {
                    if(WaveOpen((unsigned long *)
                                 sWaveClips[ulWaveIndex].pucWav,
                                 &sWaveHeader) == WAVE_OK)
                    {
                        sSoundState = STATE_PLAYING;
                        WavePlayStart(&sWaveHeader);
                    }
                    else
                    {
                        Display96x16x1StringDrawCentered("ERROR", 1, true);
                    }
                }

                //
                // Left bumper was pressed
                //
                else if(bLeftBumperIsPressed & !bLeftBumperWasPressed)
                {
                    //
                    // Decrement the wave clip index
                    //
                    ulWaveIndex = ulWaveIndex ?
                                  (ulWaveIndex - 1) : (NUM_WAVES - 1);

                    //
                    // Show the new name on the first line of the display
                    //
                    Display96x16x1StringDrawCentered(
                                            sWaveClips[ulWaveIndex].pcName,
                                            0, true);
                    Display96x16x1StringDrawCentered("0:00/0:00", 1, true);
                }

                //
                // Right bumper was pressed
                //
                else if(bRightBumperIsPressed & !bRightBumperWasPressed)
                {
                    //
                    // Increment the wave clip index
                    //
                    if(++ulWaveIndex >= NUM_WAVES)
                    {
                        ulWaveIndex = 0;
                    }

                    //
                    // Show the new name on the first line of the display
                    //
                    Display96x16x1StringDrawCentered(
                                            sWaveClips[ulWaveIndex].pcName,
                                            0, true);
                    Display96x16x1StringDrawCentered("0:00/0:00", 1, true);
                }
                break;
            }

            case STATE_PLAYING:
            {
                //
                // The bottom button is pressed
                //
                if(bBottomButtonIsPressed & !bBottomButtonWasPressed)
                {
                    //
                    // Stop playing the audio and return to the stopped state
                    //
                    WaveStop();
                    sSoundState = STATE_STOPPED;
                }

                //
                // Otherwise no button is pressed
                //
                else
                {
                    //
                    // Continue the wave playing
                    //
                    if(WavePlayContinue(&sWaveHeader))
                    {
                        //
                        // Getting here means the sound is complete
                        //
                        sSoundState = STATE_STOPPED;
                    }
                    WaveGetTime(&sWaveHeader, cTimeString, sizeof(cTimeString));
                    Display96x16x1StringDrawCentered(cTimeString, 1, false);
                }
                break;

            }
        } // end switch

//        SysCtlDelay(ROM_SysCtlClockGet() / 33);

        //
        // Remember the buttons states for the next pass through
        ///
        bTopButtonWasPressed = bTopButtonIsPressed;
        bBottomButtonWasPressed = bBottomButtonIsPressed;
        bLeftBumperWasPressed = bLeftBumperIsPressed;
        bRightBumperWasPressed = bRightBumperIsPressed;

    } // end for(;;)
}

