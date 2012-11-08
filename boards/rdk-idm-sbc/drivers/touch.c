//*****************************************************************************
//
// touch.c - Touch screen driver for the RDK-IDM-SBC board.
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
// This is part of revision 9453 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup touch_api
//! @{
//
//*****************************************************************************

#include "inc/hw_adc.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/touch.h"

//*****************************************************************************
//
// This driver operates in four different screen orientations.  They are:
//
// * Portrait - The screen is taller than it is wide, and the flex connector is
//              on the left of the display.  This is selected by defining
//              PORTRAIT.
//
// * Landscape - The screen is wider than it is tall, and the flex connector is
//               on the bottom of the display.  This is selected by defining
//               LANDSCAPE.
//
// * Portrait flip - The screen is taller than it is wide, and the flex
//                   connector is on the right of the display.  This is
//                   selected by defining PORTRAIT_FLIP.
//
// * Landscape flip - The screen is wider than it is tall, and the flex
//                    connector is on the top of the display.  This is
//                    selected by defining LANDSCAPE_FLIP.
//
// These can also be imagined in terms of screen rotation; if portrait mode is
// 0 degrees of screen rotation, landscape is 90 degrees of counter-clockwise
// rotation, portrait flip is 180 degrees of rotation, and landscape flip is
// 270 degress of counter-clockwise rotation.
//
// If no screen orientation is selected, landscape mode will be used.
//
//*****************************************************************************
#if ! defined(PORTRAIT) && ! defined(PORTRAIT_FLIP) && \
    ! defined(LANDSCAPE) && ! defined(LANDSCAPE_FLIP)
#define LANDSCAPE
#endif

//*****************************************************************************
//
// The GPIO pins to which the touch screen is connected.
//
//*****************************************************************************
#define TS_P_PERIPH             SYSCTL_PERIPH_GPIOE
#define TS_P_BASE               GPIO_PORTE_BASE
#define TS_XP_PIN               GPIO_PIN_6
#define TS_YP_PIN               GPIO_PIN_7
#define TS_N_PERIPH             SYSCTL_PERIPH_GPIOE
#define TS_N_BASE               GPIO_PORTE_BASE
#define TS_XN_PIN               GPIO_PIN_2
#define TS_YN_PIN               GPIO_PIN_3

//*****************************************************************************
//
// The ADC channels connected to each of the touch screen contacts.
//
//*****************************************************************************
#define ADC_CTL_CH_XP ADC_CTL_CH1
#define ADC_CTL_CH_YP ADC_CTL_CH0

//*****************************************************************************
//
// The coefficients used to convert from the ADC touch screen readings to the
// screen pixel positions.
//
//*****************************************************************************
#ifdef PORTRAIT
#define M0                      -288
#define M1                      73728
#define M2                      -19735920
#define M3                      74656
#define M4                      -448
#define M5                      -14142432
#define M6                      162882
#endif
#ifdef LANDSCAPE
#define M0                      85888
#define M1                      -384
#define M2                      -18005888
#define M3                      264
#define M4                      -76008
#define M5                      66659640
#define M6                      193184
#endif
#ifdef PORTRAIT_FLIP
#define M0                      -576
#define M1                      -74112
#define M2                      606010272
#define M3                      -77376
#define M4                      960
#define M5                      68476608
#define M6                      169716
#endif
#ifdef LANDSCAPE_FLIP
#define M0                      -75008
#define M1                      1152
#define M2                      66131936
#define M3                      552
#define M4                      73320
#define M5                      -20207040
#define M6                      162767
#endif

//*****************************************************************************
//
// The current state of the touch screen driver's state machine.  This is used
// to cycle the touch screen interface through the powering sequence required
// to read the two axes of the surface.
//
//*****************************************************************************
static unsigned long g_ulTSState;
#define TS_STATE_INIT           0
#define TS_STATE_READ_X         1
#define TS_STATE_READ_Y         2
#define TS_STATE_SKIP_X         3
#define TS_STATE_SKIP_Y         4

//*****************************************************************************
//
// The most recent raw ADC reading for the X position on the screen.  This
// value is not affected by the selected screen orientation.
//
//*****************************************************************************
volatile short g_sTouchX;

//*****************************************************************************
//
// The most recent raw ADC reading for the Y position on the screen.  This
// value is not affected by the selected screen orientation.
//
//*****************************************************************************
volatile short g_sTouchY;

//*****************************************************************************
//
// A pointer to the function to receive messages from the touch screen driver
// when events occur on the touch screen (debounced presses, movement while
// pressed, and debounced releases).
//
//*****************************************************************************
static long (*g_pfnTSHandler)(unsigned long ulMessage, long lX, long lY);

//*****************************************************************************
//
// The current state of the touch screen debouncer.  When zero, the pen is up.
// When three, the pen is down.  When one or two, the pen is transitioning from
// one state to the other.
//
//*****************************************************************************
static unsigned char g_cState = 0;

//*****************************************************************************
//
// The queue of debounced pen positions.  This is used to slightly delay the
// returned pen positions, so that the pen positions that occur while the pen
// is being raised are not send to the application.
//
//*****************************************************************************
static short g_psSamples[8];

//*****************************************************************************
//
// The count of pen positions in g_psSamples.  When negative, the buffer is
// being pre-filled as a result of a detected pen down event.
//
//*****************************************************************************
static signed char g_cIndex = 0;

//*****************************************************************************
//
//! Debounces presses of the touch screen.
//!
//! This function is called when a new X/Y sample pair has been captured in
//! order to perform debouncing of the touch screen.
//!
//! \return None.
//
//*****************************************************************************
static void
TouchScreenDebouncer(void)
{
    long lX, lY, lTemp;

    //
    // Convert the ADC readings into pixel values on the screen.
    //
    lX = g_sTouchX;
    lY = g_sTouchY;
    lTemp = ((lX * M0) + (lY * M1) + M2) / M6;
    lY = ((lX * M3) + (lY * M4) + M5) / M6;
    lX = lTemp;

    //
    // See if the touch screen is being touched.
    //
    if((g_sTouchX < TOUCH_MIN) || (g_sTouchY < TOUCH_MIN))
    {
        //
        // See if the pen is not up right now.
        //
        if(g_cState != 0x00)
        {
            //
            // Decrement the state count.
            //
            g_cState--;

            //
            // See if the pen has been detected as up three times in a row.
            //
            if(g_cState == 0x80)
            {
                //
                // Indicate that the pen is up.
                //
                g_cState = 0x00;

                //
                // See if there is a touch screen event handler.
                //
                if(g_pfnTSHandler)
                {
                    //
                    // Send the pen up message to the touch screen event
                    // handler.
                    //
                    g_pfnTSHandler(WIDGET_MSG_PTR_UP, g_psSamples[g_cIndex],
                                   g_psSamples[g_cIndex + 1]);
                }
            }
        }
    }
    else
    {
        //
        // See if the pen is not down right now.
        //
        if(g_cState != 0x83)
        {
            //
            // Increment the state count.
            //
            g_cState++;

            //
            // See if the pen has been detected as down three times in a row.
            //
            if(g_cState == 0x03)
            {
                //
                // Indicate that the pen is up.
                //
                g_cState = 0x83;

                //
                // Set the index to -8, so that the next 3 samples are stored
                // into the sample buffer before sending anything back to the
                // touch screen event handler.
                //
                g_cIndex = -8;

                //
                // Store this sample into the sample buffer.
                //
                g_psSamples[0] = lX;
                g_psSamples[1] = lY;
            }
        }
        else
        {
            //
            // See if the sample buffer pre-fill has completed.
            //
            if(g_cIndex == -2)
            {
                //
                // See if there is a touch screen event handler.
                //
                if(g_pfnTSHandler)
                {
                    //
                    // Send the pen down message to the touch screen event
                    // handler.
                    //
                    g_pfnTSHandler(WIDGET_MSG_PTR_DOWN, g_psSamples[0],
                                   g_psSamples[1]);
                }

                //
                // Store this sample into the sample buffer.
                //
                g_psSamples[0] = lX;
                g_psSamples[1] = lY;

                //
                // Set the index to the next sample to send.
                //
                g_cIndex = 2;
            }

            //
            // Otherwise, see if the sample buffer pre-fill is in progress.
            //
            else if(g_cIndex < 0)
            {
                //
                // Store this sample into the sample buffer.
                //
                g_psSamples[g_cIndex + 10] = lX;
                g_psSamples[g_cIndex + 11] = lY;

                //
                // Increment the index.
                //
                g_cIndex += 2;
            }

            //
            // Otherwise, the sample buffer is full.
            //
            else
            {
                //
                // See if there is a touch screen event handler.
                //
                if(g_pfnTSHandler)
                {
                    //
                    // Send the pen move message to the touch screen event
                    // handler.
                    //
                    g_pfnTSHandler(WIDGET_MSG_PTR_MOVE, g_psSamples[g_cIndex],
                                   g_psSamples[g_cIndex + 1]);
                }

                //
                // Store this sample into the sample buffer.
                //
                g_psSamples[g_cIndex] = lX;
                g_psSamples[g_cIndex + 1] = lY;

                //
                // Increment the index.
                //
                g_cIndex = (g_cIndex + 2) & 7;
            }
        }
    }
}

//*****************************************************************************
//
//! Handles the ADC interrupt for the touch screen.
//!
//! This function is called when the ADC sequence that samples the touch screen
//! has completed its acquisition.  The touch screen state machine is advanced
//! and the acquired ADC sample is processed appropriately.
//!
//! It is the responsibility of the application using the touch screen driver
//! to ensure that this function is installed in the interrupt vector table for
//! the ADC3 interrupt.
//!
//! \return None.
//
//*****************************************************************************
void
TouchScreenIntHandler(void)
{
    //
    // Clear the ADC sample sequence interrupt.
    //
    HWREG(ADC0_BASE + ADC_O_ISC) = 1 << 3;

    //
    // Determine what to do based on the current state of the state machine.
    //
    switch(g_ulTSState)
    {
        //
        // The new sample is an X axis sample that should be discarded.
        //
        case TS_STATE_SKIP_X:
        {
            //
            // Read and throw away the ADC sample.
            //
            HWREG(ADC0_BASE + ADC_O_SSFIFO3);

            //
            // Set the analog mode select for the YP pin.
            //
            HWREG(TS_P_BASE + GPIO_O_AMSEL) =
                HWREG(TS_P_BASE + GPIO_O_AMSEL) | TS_YP_PIN;

            //
            // Configure the Y axis touch layer pins as inputs.
            //
            HWREG(TS_P_BASE + GPIO_O_DIR) =
                HWREG(TS_P_BASE + GPIO_O_DIR) & ~TS_YP_PIN;
            HWREG(TS_N_BASE + GPIO_O_DIR) =
                HWREG(TS_N_BASE + GPIO_O_DIR) & ~TS_YN_PIN;

            //
            // The next sample will be a valid X axis sample.
            //
            g_ulTSState = TS_STATE_READ_X;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The new sample is an X axis sample that should be processed.
        //
        case TS_STATE_READ_X:
        {
            //
            // Read the raw ADC sample.
            //
            g_sTouchX = HWREG(ADC0_BASE + ADC_O_SSFIFO3);

            //
            // Clear the analog mode select for the YP pin.
            //
            HWREG(TS_P_BASE + GPIO_O_AMSEL) =
                HWREG(TS_P_BASE + GPIO_O_AMSEL) & ~TS_YP_PIN;

            //
            // Configure the X and Y axis touch layers as outputs.
            //
            HWREG(TS_P_BASE + GPIO_O_DIR) =
                HWREG(TS_P_BASE + GPIO_O_DIR) | TS_XP_PIN | TS_YP_PIN;
            HWREG(TS_N_BASE + GPIO_O_DIR) =
                HWREG(TS_N_BASE + GPIO_O_DIR) | TS_XN_PIN | TS_YN_PIN;

            //
            // Drive the positive side of the Y axis touch layer with VDD and
            // the negative side with GND.  Also, drive both sides of the X
            // axis layer with GND to discharge any residual voltage (so that
            // a no-touch condition can be properly detected).
            //
            HWREG(TS_N_BASE + GPIO_O_DATA + ((TS_XN_PIN | TS_YN_PIN) << 2)) =
                0;
            HWREG(TS_P_BASE + GPIO_O_DATA + ((TS_XP_PIN | TS_YP_PIN) << 2)) =
                TS_YP_PIN;

            //
            // Configure the sample sequence to capture the X axis value.
            //
            HWREG(ADC0_BASE + ADC_O_SSMUX3) = ADC_CTL_CH_XP;

            //
            // The next sample will be an invalid Y axis sample.
            //
            g_ulTSState = TS_STATE_SKIP_Y;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The new sample is a Y axis sample that should be discarded.
        //
        case TS_STATE_SKIP_Y:
        {
            //
            // Read and throw away the ADC sample.
            //
            HWREG(ADC0_BASE + ADC_O_SSFIFO3);

            //
            // Set the analog mode select for the XP pin.
            //
            HWREG(TS_P_BASE + GPIO_O_AMSEL) =
                HWREG(TS_P_BASE + GPIO_O_AMSEL) | TS_XP_PIN;

            //
            // Configure the X axis touch layer pins as inputs.
            //
            HWREG(TS_P_BASE + GPIO_O_DIR) =
                HWREG(TS_P_BASE + GPIO_O_DIR) & ~TS_XP_PIN;
            HWREG(TS_N_BASE + GPIO_O_DIR) =
                HWREG(TS_N_BASE + GPIO_O_DIR) & ~TS_XN_PIN;

            //
            // The next sample will be a valid Y axis sample.
            //
            g_ulTSState = TS_STATE_READ_Y;

            //
            // This state has been handled.
            //
            break;
        }

        //
        // The new sample is a Y axis sample that should be processed.
        //
        case TS_STATE_READ_Y:
        {
            //
            // Read the raw ADC sample.
            //
            g_sTouchY = HWREG(ADC0_BASE + ADC_O_SSFIFO3);

            //
            // The next configuration is the same as the initial configuration.
            // Therefore, fall through into the initialization state to avoid
            // duplicating the code.
            //
        }

        //
        // The state machine is in its initial state
        //
        case TS_STATE_INIT:
        {
            //
            // Clear the analog mode select for the XP pin.
            //
            HWREG(TS_P_BASE + GPIO_O_AMSEL) =
                HWREG(TS_P_BASE + GPIO_O_AMSEL) & ~TS_XP_PIN;

            //
            // Configure the X and Y axis touch layers as outputs.
            //
            HWREG(TS_P_BASE + GPIO_O_DIR) =
                HWREG(TS_P_BASE + GPIO_O_DIR) | TS_XP_PIN | TS_YP_PIN;
            HWREG(TS_N_BASE + GPIO_O_DIR) =
                HWREG(TS_N_BASE + GPIO_O_DIR) | TS_XN_PIN | TS_YN_PIN;

            //
            // Drive one side of the X axis touch layer with VDD and the other
            // with GND.  Also, drive both sides of the Y axis layer with GND
            // to discharge any residual voltage (so that a no-touch condition
            // can be properly detected).
            //
            HWREG(TS_P_BASE + GPIO_O_DATA + ((TS_XP_PIN | TS_YP_PIN) << 2)) =
                TS_XP_PIN;
            HWREG(TS_N_BASE + GPIO_O_DATA + ((TS_XN_PIN | TS_YN_PIN) << 2)) =
                0;

            //
            // Configure the sample sequence to capture the Y axis value.
            //
            HWREG(ADC0_BASE + ADC_O_SSMUX3) = ADC_CTL_CH_YP;

            //
            // If this is the valid Y sample state, then there is a new X/Y
            // sample pair.  In that case, run the touch screen debouncer.
            //
            if(g_ulTSState == TS_STATE_READ_Y)
            {
                TouchScreenDebouncer();
            }

            //
            // The next sample will be an invalid X axis sample.
            //
            g_ulTSState = TS_STATE_SKIP_X;

            //
            // This state has been handled.
            //
            break;
        }
    }
}

//*****************************************************************************
//
//! Initializes the touch screen driver.
//!
//! This function initializes the touch screen driver, beginning the process of
//! reading from the touch screen.  This driver uses the following hardware
//! resources:
//!
//! - ADC sample sequence 3
//! - Timer 1 subtimer A
//!
//! \return None.
//
//*****************************************************************************
void
TouchScreenInit(void)
{
    //
    // Set the initial state of the touch screen driver's state machine.
    //
    g_ulTSState = TS_STATE_INIT;

    //
    // There is no touch screen handler initially.
    //
    g_pfnTSHandler = 0;

    //
    // Enable the peripherals used by the touch screen interface.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    ROM_SysCtlPeripheralEnable(TS_P_PERIPH);
    ROM_SysCtlPeripheralEnable(TS_N_PERIPH);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    //
    // Configure the ADC sample sequence used to read the touch screen reading.
    //
    ROM_ADCHardwareOversampleConfigure(ADC0_BASE, 4);
    ROM_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 0);
    ROM_ADCSequenceStepConfigure(ADC0_BASE, 3, 0,
                                 ADC_CTL_CH_YP | ADC_CTL_END | ADC_CTL_IE);
    ROM_ADCSequenceEnable(ADC0_BASE, 3);

    //
    // Enable the ADC sample sequence interrupt.
    //
    ROM_ADCIntEnable(ADC0_BASE, 3);
    ROM_IntEnable(INT_ADC0SS3);

    //
    // Configure the GPIOs used to drive the touch screen layers.
    //
    ROM_GPIOPinTypeGPIOOutput(TS_P_BASE, TS_XP_PIN | TS_YP_PIN);
    ROM_GPIOPinTypeGPIOOutput(TS_N_BASE, TS_XN_PIN | TS_YN_PIN);
    ROM_GPIOPinWrite(TS_P_BASE, TS_XP_PIN | TS_YP_PIN, 0x00);
    ROM_GPIOPinWrite(TS_N_BASE, TS_XN_PIN | TS_YN_PIN, 0x00);

    //
    // See if the ADC trigger timer has been configured, and configure it only
    // if it has not been configured yet.
    //
    if((HWREG(TIMER1_BASE + TIMER_O_CTL) & TIMER_CTL_TAEN) == 0)
    {
        //
        // Configure the timer to trigger the sampling of the touch screen
        // every millisecond.
        //
        ROM_TimerConfigure(TIMER1_BASE, (TIMER_CFG_SPLIT_PAIR |
                           TIMER_CFG_A_PERIODIC | TIMER_CFG_B_PERIODIC));
        ROM_TimerLoadSet(TIMER1_BASE, TIMER_A, (SysCtlClockGet() / 1000) - 1);
        ROM_TimerControlTrigger(TIMER1_BASE, TIMER_A, true);

        //
        // Enable the timer.  At this point, the touch screen state machine
        // will sample and run once per millisecond.
        //
        ROM_TimerEnable(TIMER1_BASE, TIMER_A);
    }
}

//*****************************************************************************
//
//! Sets the callback function for touch screen events.
//!
//! \param pfnCallback is a pointer to the function to be called when touch
//! screen events occur.
//!
//! This function sets the address of the function to be called when touch
//! screen events occur.  The events that are recognized are the screen being
//! touched (``pen down''), the touch position moving while the screen is
//! touched (``pen move''), and the screen no longer being touched (``pen
//! up'').
//!
//! \return None.
//
//*****************************************************************************
void
TouchScreenCallbackSet(long (*pfnCallback)(unsigned long ulMessage, long lX,
                                           long lY))
{
    //
    // Save the pointer to the callback function.
    //
    g_pfnTSHandler = pfnCallback;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
