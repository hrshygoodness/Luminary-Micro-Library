//*****************************************************************************
//
// io.c - I/O routines for the enet_io example application.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "utils/ustdlib.h"
#include "io.h"

//*****************************************************************************
//
// Global Variables for the Frequency and Duty cycle
//
//*****************************************************************************
unsigned long g_ulFrequency;
unsigned long g_ulDutyCycle;

//*****************************************************************************
//
// Initialize the IO used in this demo
// 1. STATUS LED on Port F pin 0
// 2. PWM on Port D Pin 1 (PWM1)
//
//*****************************************************************************
void
io_init(void)
{
    unsigned long ulPWMClock;

    //
    // Enable GPIO bank F to allow control of the LED.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Configure Port F0 for as an output for the status LED.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,GPIO_PIN_0);

    //
    // Initialize LED to OFF (0)
    //
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);

    //
    // Enable Port G1 for PWM output.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    GPIOPinTypePWM(GPIO_PORTG_BASE,GPIO_PIN_1);

    //
    // Enable the PWM generator.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    //
    // Configure the PWM generator for count down mode with immediate updates
    // to the parameters.
    //
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0,
                    PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    //
    // Divide the PWM clock by 4.
    //
    SysCtlPWMClockSet(SYSCTL_PWMDIV_4);

    //
    // Get the PWM clock.
    //
    ulPWMClock = SysCtlClockGet()/4;

    //
    // Intialize the PWM frequency and duty cycle.
    //
    g_ulFrequency = 440;
    g_ulDutyCycle = 50;

    //
    // Set the period of PWM1.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ulPWMClock / g_ulFrequency);

    //
    // Set the pulse width of PWM1.
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1,
                     ((ulPWMClock * g_ulDutyCycle)/100) / g_ulFrequency);

    //
    // Start the timers in generator 0.
    //
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);

}

//*****************************************************************************
//
// Set the status LED on or off.
//
//*****************************************************************************
void
io_set_led(tBoolean bOn)
{
    //
    // Turn the LED on or off as requested.
    //
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, bOn ? GPIO_PIN_0 : 0);
}

//*****************************************************************************
//
// Turn PWM on/off
//
//*****************************************************************************
void
io_set_pwm(tBoolean bOn)
{
    //
    // Enable or disable the PWM1 output.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, bOn);
}

//*****************************************************************************
//
// Set PWM Frequency
//
//*****************************************************************************
void
io_pwm_freq(unsigned long ulFreq)
{
    unsigned long ulPWMClock;

    //
    // Get the PWM clock
    //
    ulPWMClock = SysCtlClockGet()/4;

    //
    // Set the global frequency
    //
    g_ulFrequency = ulFreq;

    //
    // Set the period.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ulPWMClock / g_ulFrequency);

    //
    // Set the pulse width of PWM1
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1,
                     ((ulPWMClock * g_ulDutyCycle)/100) / g_ulFrequency);

}

//*****************************************************************************
//
// Set PWM Duty Cycle
//
//*****************************************************************************
void
io_pwm_dutycycle(unsigned long ulDutyCycle)
{
    unsigned long ulPWMClock;

    //
    // Get the PWM clock
    //
    ulPWMClock = SysCtlClockGet()/4;

    //
    // Set the global duty cycle
    //
    g_ulDutyCycle = ulDutyCycle;

    //
    // Set the period.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ulPWMClock / g_ulFrequency);

    //
    // Set the pulse width of PWM1
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1,
                     ((ulPWMClock * g_ulDutyCycle)/100) / g_ulFrequency);

}

//*****************************************************************************
//
// Return LED state
//
//*****************************************************************************
void
io_get_ledstate(char * pcBuf, int iBufLen)
{
    //
    // Get the state of the LED
    //
    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0))
    {
        usnprintf(pcBuf, iBufLen, "ON");
    }
    else
    {
        usnprintf(pcBuf, iBufLen, "OFF");
    }

}

//*****************************************************************************
//
// Return LED state as an integer, 1 on, 0 off.
//
//*****************************************************************************
int
io_is_led_on(void)
{
    //
    // Get the state of the LED
    //
    if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0))
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
// Return PWM state
//
//*****************************************************************************
void
io_get_pwmstate(char * pcBuf, int iBufLen)
{
    //
    // Get the state of the PWM1
    //
    if(HWREG(PWM0_BASE + PWM_O_ENABLE) & PWM_OUT_1_BIT)
    {
        usnprintf(pcBuf, iBufLen, "ON");
    }
    else
    {
        usnprintf(pcBuf, iBufLen, "OFF");
    }

}

//*****************************************************************************
//
// Return PWM state as an integer, 1 on, 0 off.
//
//*****************************************************************************
int
io_is_pwm_on(void)
{
    //
    // Get the state of the PWM1
    //
    if(HWREG(PWM0_BASE + PWM_O_ENABLE) & PWM_OUT_1_BIT)
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
// Return PWM frequency
//
//*****************************************************************************
unsigned long
io_get_pwmfreq(void)
{
    //
    // Return PWM frequency
    //
    return g_ulFrequency;

}

//*****************************************************************************
//
// Return PWM duty cycle
//
//*****************************************************************************
unsigned long
io_get_pwmdutycycle(void)
{
    //
    // Return PWM duty cycle
    //
    return g_ulDutyCycle;

}
