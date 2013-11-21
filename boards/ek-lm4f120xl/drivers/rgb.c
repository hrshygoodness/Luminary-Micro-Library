//*****************************************************************************
//
// rgb.c - Evaluation board driver for RGB LED.
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

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
#include "driverlib/gpio.h"
#include "rgb.h"

//*****************************************************************************
//
//! \addtogroup rgb_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This is a custom driver that allows the easy manipulation of the RGB LED.
//
// The driver uses the general purpose timers to govern the brightness of the 
// LED through simple PWM output mode of the GP Timers.
//
// A global array contains the current relative color of each of the three
// LEDs. A global float variable controls intensity of the overall mixed color.
//
//*****************************************************************************
static unsigned long  g_ulColors[3];
static float g_fIntensity = 0.3f;

//*****************************************************************************
//
//! Initializes the Timer and GPIO functionality associated with the RGB LED
//!
//! \param ulEnable enables RGB immediately if set.
//!
//! This function must be called during application initialization to
//! configure the GPIO pins to which the LEDs are attached.  It enables
//! the port used by the LEDs and configures each color's Timer. It optionally
//! enables the RGB LED by configuring the GPIO pins and starting the timers.
//!
//! \return None.
//
//*****************************************************************************
void 
RGBInit(unsigned long ulEnable)
{
    //
    // Enable the GPIO Port and Timer for each LED
    //
    SysCtlPeripheralEnable(RED_GPIO_PERIPH);
    SysCtlPeripheralEnable(RED_TIMER_PERIPH);

    SysCtlPeripheralEnable(GREEN_GPIO_PERIPH);
    SysCtlPeripheralEnable(GREEN_TIMER_PERIPH);

    SysCtlPeripheralEnable(BLUE_GPIO_PERIPH);
    SysCtlPeripheralEnable(BLUE_TIMER_PERIPH);

    //
    // Configure each timer for output mode
    //
    HWREG(GREEN_TIMER_BASE + TIMER_O_CFG)   = 0x04;
    HWREG(GREEN_TIMER_BASE + TIMER_O_TAMR)  = 0x0A;
    HWREG(GREEN_TIMER_BASE + TIMER_O_TAILR) = 0xFFFF;

    HWREG(BLUE_TIMER_BASE + TIMER_O_CFG)   = 0x04;
    HWREG(BLUE_TIMER_BASE + TIMER_O_TBMR)  = 0x0A;
    HWREG(BLUE_TIMER_BASE + TIMER_O_TBILR) = 0xFFFF;

    HWREG(RED_TIMER_BASE + TIMER_O_CFG)   = 0x04;
    HWREG(RED_TIMER_BASE + TIMER_O_TBMR)  = 0x0A;
    HWREG(RED_TIMER_BASE + TIMER_O_TBILR) = 0xFFFF;
    
    //
    // Invert the output signals.
    //
    HWREG(RED_TIMER_BASE + TIMER_O_CTL)   |= 0x4000;
    HWREG(GREEN_TIMER_BASE + TIMER_O_CTL)   |= 0x40;
    HWREG(BLUE_TIMER_BASE + TIMER_O_CTL)   |= 0x4000;
    
    if(ulEnable)
    {
        RGBEnable();
    }
}

//*****************************************************************************
//
//! Enable the RGB LED with already configured timer settings
//!
//! This function or RGBDisable should be called during application 
//! initialization to configure the GPIO pins to which the LEDs are attached.  
//! This function enables the timers and configures the GPIO pins as timer
//! outputs.
//!
//! \return None.
//
//*****************************************************************************
void 
RGBEnable(void)
{
    //
    // Enable timers to begin counting
    //
    TimerEnable(RED_TIMER_BASE, TIMER_BOTH);
    TimerEnable(GREEN_TIMER_BASE, TIMER_BOTH);
    TimerEnable(BLUE_TIMER_BASE, TIMER_BOTH);
    
    //
    // Reconfigure each LED's GPIO pad for timer control
    //
    GPIOPinConfigure(GREEN_GPIO_PIN_CFG);
    GPIOPinTypeTimer(GREEN_GPIO_BASE, GREEN_GPIO_PIN);
    GPIOPadConfigSet(GREEN_GPIO_BASE, GREEN_GPIO_PIN, GPIO_STRENGTH_8MA_SC,
                     GPIO_PIN_TYPE_STD);

    GPIOPinConfigure(BLUE_GPIO_PIN_CFG);
    GPIOPinTypeTimer(BLUE_GPIO_BASE, BLUE_GPIO_PIN);
    GPIOPadConfigSet(BLUE_GPIO_BASE, BLUE_GPIO_PIN, GPIO_STRENGTH_8MA_SC,
                     GPIO_PIN_TYPE_STD);

    GPIOPinConfigure(RED_GPIO_PIN_CFG);
    GPIOPinTypeTimer(RED_GPIO_BASE, RED_GPIO_PIN);
    GPIOPadConfigSet(RED_GPIO_BASE, RED_GPIO_PIN, GPIO_STRENGTH_8MA_SC,
                     GPIO_PIN_TYPE_STD);
}

//*****************************************************************************
//
//! Disable the RGB LED by configuring the GPIO's as inputs.
//!
//! This function or RGBEnable should be called during application 
//! initialization to configure the GPIO pins to which the LEDs are attached.  
//! This function disables the timers and configures the GPIO pins as inputs 
//! for minimum current draw.
//!
//! \return None.
//
//*****************************************************************************
void
RGBDisable(void)
{
    //
    // Configure the GPIO pads as general purpose inputs.
    //
    GPIOPinTypeGPIOInput(RED_GPIO_BASE, RED_GPIO_PIN);
    GPIOPinTypeGPIOInput(GREEN_GPIO_BASE, GREEN_GPIO_PIN);
    GPIOPinTypeGPIOInput(BLUE_GPIO_BASE, BLUE_GPIO_PIN);

    //
    // Stop the timer counting.
    //
    TimerDisable(RED_TIMER_BASE, TIMER_BOTH);
    TimerDisable(GREEN_TIMER_BASE, TIMER_BOTH);
    TimerDisable(BLUE_TIMER_BASE, TIMER_BOTH);
}

//*****************************************************************************
//
//! Set the output color and intensity.
//!
//! \param pulRGBColor points to a three element array representing the
//! relative intensity of each color.  Red is element 0, Green is element 1,
//! Blue is element 2. 0x0000 is off.  0xFFFF is fully on.
//!
//! \param fIntensity is used to scale the intensity of all three colors by
//! the same amount.  fIntensity should be between 0.0 and 1.0.  This scale
//! factor is applied to all three colors.
//!
//! This function should be called by the application to set the color and
//! intensity of the RGB LED.
//!
//! \return None.
//
//*****************************************************************************
void
RGBSet(volatile unsigned long * pulRGBColor,  float fIntensity)
{
    RGBColorSet(pulRGBColor);
    RGBIntensitySet(fIntensity);
}

//*****************************************************************************
//
//! Set the output color.
//!
//! \param pulRGBColor points to a three element array representing the
//! relative intensity of each color.  Red is element 0, Green is element 1,
//! Blue is element 2. 0x0000 is off.  0xFFFF is fully on.
//!
//! This function should be called by the application to set the color
//! of the RGB LED.
//!
//! \return None.
//
//*****************************************************************************
void 
RGBColorSet(volatile unsigned long * pulRGBColor)
{
    unsigned long ulColor[3];
    unsigned long ulIndex;

    for(ulIndex=0; ulIndex < 3; ulIndex++)
    {
        g_ulColors[ulIndex] = pulRGBColor[ulIndex];
        ulColor[ulIndex] = (unsigned long) (((float) pulRGBColor[ulIndex]) *
                            g_fIntensity + 0.5f);

        if(ulColor[ulIndex] > 0xFFFF)
        {
            ulColor[ulIndex] = 0xFFFF;
        }
    }

    TimerMatchSet(RED_TIMER_BASE, RED_TIMER, ulColor[RED]);
    TimerMatchSet(GREEN_TIMER_BASE, GREEN_TIMER, ulColor[GREEN]);
    TimerMatchSet(BLUE_TIMER_BASE, BLUE_TIMER, ulColor[BLUE]);
}

//*****************************************************************************
//
//! Set the current output intensity.
//!
//! \param fIntensity is used to scale the intensity of all three colors by
//! the same amount.  fIntensity should be between 0.0 and 1.0.  This scale
//! factor is applied individually to all three colors.
//!
//! This function should be called by the application to set the intensity
//! of the RGB LED.
//!
//! \return None.
//
//*****************************************************************************
void 
RGBIntensitySet(float fIntensity)
{
    g_fIntensity = fIntensity;
    RGBColorSet(g_ulColors);
}

//*****************************************************************************
//
//! Get the output color.
//!
//! \param pulRGBColor points to a three element array representing the
//! relative intensity of each color.  Red is element 0, Green is element 1,
//! Blue is element 2. 0x0000 is off.  0xFFFF is fully on. Caller must allocate
//! and pass a pointer to a three element array of unsigned longs.
//!
//! This function should be called by the application to get the current color
//! of the RGB LED.
//!
//! \return None.
//
//*****************************************************************************
void 
RGBColorGet(unsigned long * pulRGBColor)
{
    unsigned long ulIndex;

    for(ulIndex=0; ulIndex < 3; ulIndex++)
    {
        pulRGBColor[ulIndex] = g_ulColors[ulIndex];
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
