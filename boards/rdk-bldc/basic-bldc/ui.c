//*****************************************************************************
//
// ui.c - User interface module.
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-BLDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "adc_ctrl.h"
#include "hall_ctrl.h"
#include "main.h"
#include "pins.h"
#include "pwm_ctrl.h"
#include "ui.h"
#include "ui_onboard.h"

//*****************************************************************************
//
// \page ui_intro Introduction
//
// There are two user interfaces for the the Brushless DC motor application.
// One uses an push button for basic control of
// the motor and two LEDs for basic status feedback, and the other uses the
// Ethernet port to provide complete control of all aspects of the motor drive
// as well as monitoring of real-time performance data.
//
// The on-board user interface consists of a push button and
// two LEDs.  The push button cycles between run
// forward, stop, run backward, stop.
//
// The ``Run'' LED flashes the entire time the application is running.  The
// LED is off most of the time if the motor drive is stopped and on most of
// the time if it is running.  The ``Fault'' LED is normally off but flashes
// at a fast rate when a fault occurs.
//
// A periodic interrupt is used to poll the state of the push button and
// perform debouncing.
//
// The Ethernet user interface is entirely handled by the Ethernet user
// interface
// module.  The only thing provided here is the list of parameters and
// real-time data items, plus a set of helper functions that are required in
// order to properly set the values of some of the parameters.
//
// This user interface (and the accompanying Ethernet and on-board user
// interface modules) is more complicated and consumes more program space than
// would typically exist in a real motor drive application.  The added
// complexity allows a great deal of flexibility to configure and evaluate the
// motor drive, its capabilities, and adjust it for the target motor.
//
// The code for the user interface is contained in <tt>ui.c</tt>, with
// <tt>ui.h</tt> containing the definitions for the structures, defines,
// variables, and functions exported to the remainder of the application.
//
//*****************************************************************************

//*****************************************************************************
//
// \defgroup ui_api Definitions
// @{
//
//*****************************************************************************

//*****************************************************************************
//
// The rate at which the user interface interrupt occurs.
//
//*****************************************************************************
#define UI_INT_RATE             200
#define UI_TICK_MS              (1000/UI_INT_RATE)
#define UI_TICK_US              (1000000/UI_INT_RATE)
#define UI_TICK_NS              (1000000000/UI_INT_RATE)

//*****************************************************************************
//
// The rate at which the timer interrupt occurs.
//
//*****************************************************************************
#define TIMER1A_INT_RATE        100
#define TIMER1A_TICK_MS         (1000/TIMER1A_INT_RATE)
#define TIMER1A_TICK_US         (1000000/TIMER1A_INT_RATE)
#define TIMER1A_TICK_NS         (1000000000/TIMER1A_INT_RATE)

//*****************************************************************************
//
// Forward declarations for functions declared within this source file for use
// in the parameter and real-time data structures.
//
//*****************************************************************************
void UIButtonPress(void);
static void UIButtonHold(void);

//*****************************************************************************
//
// The blink rate of the two LEDs on the board; this is the number of user
// interface interrupts for an entire blink cycle.  The run LED is the first
// entry of the array and the fault LED is the second entry of the array.
//
//*****************************************************************************
static unsigned short g_pusBlinkRate[2] =
{
    0, 0
};

//*****************************************************************************
//
// The blink period of the two LEDs on the board; this is the number of user
// interface interrupts for which the LED will be turned on.  The run LED is
// the first entry of the array and the fault LED is the second entry of the
// array.
//
//*****************************************************************************
static unsigned short g_pusBlinkPeriod[2];

//*****************************************************************************
//
// The count of count of user interface interrupts that have occurred.  This
// is used to determine when to toggle the LEDs that are blinking.
//
//*****************************************************************************
static unsigned long g_ulBlinkCount = 0;

//*****************************************************************************
//
// This array contains the base address of the GPIO blocks for the two LEDs
// on the board.
//
//*****************************************************************************
static const unsigned long g_pulLEDBase[2] =
{
    PIN_LEDRUN_PORT,
    PIN_LEDFAULT_PORT
};

//*****************************************************************************
//
// This array contains the pin numbers of the two LEDs on the board.
//
//*****************************************************************************
static const unsigned char g_pucLEDPin[2] =
{
    PIN_LEDRUN_PIN,
    PIN_LEDFAULT_PIN
};

//*****************************************************************************
//
// A 32-bit unsigned value that represents the value of various GPIO signals
// on the board.  Bit 0 corresponds to CFG0; Bit 1 corresponds to CFG1; Bit
// 2 correpsonds to CFG2; Bit 8 corresponds to the Encoder A input; Bit 8
// corresponds to the Encode B input; Bit 10 corresponds to the Encoder
// Index input.
//
//*****************************************************************************
unsigned long g_ulGPIOData = 0;

//*****************************************************************************
//
// The Analog Input voltage, specified in millivolts.
//
//*****************************************************************************
static unsigned short g_usAnalogInputVoltage;

//*****************************************************************************
//
// An array of structures describing the on-board switches.
//
//*****************************************************************************
const tUIOnboardSwitch g_sUISwitches[] =
{
    //
    // The run/stop/mode button.  Pressing the button will cycle between
    // stopped and running, and holding the switch for five seconds will toggle
    // between sine wave and space vector modulation.
    //
    {
        PIN_SWITCH_PIN_BIT,
        UI_INT_RATE * 5,
        UIButtonPress,
        0,
        UIButtonHold
    }
};

//*****************************************************************************
//
// The number of switches in the g_sUISwitches array.  This value is
// automatically computed based on the number of entries in the array.
//
//*****************************************************************************
#define NUM_SWITCHES            (sizeof(g_sUISwitches) /   \
                                 sizeof(g_sUISwitches[0]))

//*****************************************************************************
//
// The number of switches on this target.  This value is used by the on-board
// user interface module.
//
//*****************************************************************************
const unsigned long g_ulUINumButtons = NUM_SWITCHES;

//*****************************************************************************
//
// This is the count of the number of samples during which the switches have
// been pressed; it is used to distinguish a switch press from a switch
// hold.  This array is used by the on-board user interface module.
//
//*****************************************************************************
unsigned long g_pulUIHoldCount[NUM_SWITCHES];

//*****************************************************************************
//
// This is the config switch value.
//
//*****************************************************************************
unsigned long g_ulConfigSwitch = 0;

//*****************************************************************************
//
// The running count of system clock ticks.
//
//*****************************************************************************
static unsigned long g_ulUITickCount = 0;

//*****************************************************************************
//
// Handles button presses.
//
// This function is called when a press of the on-board push button has been
// detected.  If the motor drive is running, it will be stopped.  If it is
// stopped, the direction will be reversed and the motor drive will be
// started.
//
// \return None.
//
//*****************************************************************************
void
UIButtonPress(void)
{
    //
    // See if the motor drive is running.
    //
    if(MainIsRunning())
    {
        //
        // Stop the motor drive.
        //
        MainStop();
    }
    else
    {
        //
        // Start the motor drive.
        //
        MainRun();
    }
}

//*****************************************************************************
//
// Handles button holds.
//
// This function is called when a hold of the on-board push button has been
// detected.  The modulation type of the motor will be toggled between sine
// wave and space vector modulation, but only if a three phase motor is in
// use.
//
// \return None.
//
//*****************************************************************************
static void
UIButtonHold(void)
{
    //
    // Toggle the modulation type.  UIModulationType() will take care of
    // forcing sine wave modulation for single phase motors.
    //
    //g_ucModulation ^= 1;
    //UIModulationType();
}

//*****************************************************************************
//
// Sets the blink rate for an LED.
//
// \param ulIdx is the number of the LED to configure.
// \param usRate is the rate to blink the LED.
// \param usPeriod is the amount of time to turn on the LED.
//
// This function sets the rate at which an LED should be blinked.  A blink
// period of zero means that the LED should be turned off, and a blink period
// equal to the blink rate means that the LED should be turned on.  Otherwise,
// the blink rate determines the number of user interface interrupts during
// the blink cycle of the LED, and the blink period is the number of those
// user interface interrupts during which the LED is turned on.
//
// \return None.
//
//*****************************************************************************
static void
UILEDBlink(unsigned long ulIdx, unsigned short usRate, unsigned short usPeriod)
{
    //
    // Clear the blink rate for this LED.
    //
    g_pusBlinkRate[ulIdx] = 0;

    //
    // A blink period of zero means that the LED should be turned off.
    //
    if(usPeriod == 0)
    {
        //
        // Turn off the LED.
        //
        GPIOPinWrite(g_pulLEDBase[ulIdx], g_pucLEDPin[ulIdx],
                     (ulIdx == 0) ? g_pucLEDPin[0] : 0);
    }

    //
    // A blink rate equal to the blink period means that the LED should be
    // turned on.
    //
    else if(usRate == usPeriod)
    {
        //
        // Turn on the LED.
        //
        GPIOPinWrite(g_pulLEDBase[ulIdx], g_pucLEDPin[ulIdx],
                     (ulIdx == 0) ? 0 : g_pucLEDPin[ulIdx]);
    }

    //
    // Otherwise, the LED should be blinked at the given rate.
    //
    else
    {
        //
        // Save the blink rate and period for this LED.
        //
        g_pusBlinkRate[ulIdx] = usRate;
        g_pusBlinkPeriod[ulIdx] = usPeriod;
    }
}

//*****************************************************************************
//
// Sets the blink rate for the run LED.
//
// \param usRate is the rate to blink the run LED.
// \param usPeriod is the amount of time to turn on the run LED.
//
// This function sets the rate at which the run LED should be blinked.  A
// blink period of zero means that the LED should be turned off, and a blink
// period equal to the blink rate means that the LED should be turned on.
// Otherwise, the blink rate determines the number of user interface
// interrupts during the blink cycle of the run LED, and the blink period
// is the number of those user interface interrupts during which the LED is
// turned on.
//
// \return None.
//
//*****************************************************************************
void
UIRunLEDBlink(unsigned short usRate, unsigned short usPeriod)
{
    //
    // The run LED is the first LED.
    //
    UILEDBlink(0, usRate, usPeriod);
}

//*****************************************************************************
//
// Sets the blink rate for the fault LED.
//
// \param usRate is the rate to blink the fault LED.
// \param usPeriod is the amount of time to turn on the fault LED.
//
// This function sets the rate at which the fault LED should be blinked.  A
// blink period of zero means that the LED should be turned off, and a blink
// period equal to the blink rate means that the LED should be turned on.
// Otherwise, the blink rate determines the number of user interface
// interrupts during the blink cycle of the fault LED, and the blink period
// is the number of those user interface interrupts during which the LED is
// turned on.
//
// \return None.
//
//*****************************************************************************
void
UIFaultLEDBlink(unsigned short usRate, unsigned short usPeriod)
{
    //
    // The fault LED is the second LED.
    //
    UILEDBlink(1, usRate, usPeriod);
}

//*****************************************************************************
//
// This function returns the current number of system ticks.
//
// \return The number of system timer ticks.
//
//*****************************************************************************
unsigned long
UIGetTicks(void)
{
    unsigned long ulTime1;
    unsigned long ulTime2;
    unsigned long ulTicks;

    //
    // We read the SysTick value twice, sandwiching taking the snapshot of
    // the tick count value. If the second SysTick read gives us a higher
    // number than the first read, we know that it wrapped somewhere between
    // the two reads so our tick count value is suspect.  If this occurs,
    // we go round again. Note that it is not sufficient merely to read the
    // values with interrupts disabled since the SysTick counter keeps
    // counting regardless of whether or not the wrap interrupt has been
    // serviced.
    //
    do
    {
        ulTime1 = TimerValueGet(TIMER1_BASE, TIMER_A);
        ulTicks = g_ulUITickCount;
        ulTime2 = TimerValueGet(TIMER1_BASE, TIMER_A);
    }
    while(ulTime2 > ulTime1);

    //
    // Calculate the number of ticks
    //
    ulTime1 = ulTicks + (SYSTEM_CLOCK / TIMER1A_INT_RATE) - ulTime2;

    //
    // Return the value.
    //
    return(ulTime1);
}

//*****************************************************************************
//
// Handles the Timer1A interrupt.
//
// This function is called when Timer1A asserts its interrupt.  It is
// responsible for keeping track of system time.  This should be the highest
// priority interrupt.
//
// \return None.
//
//*****************************************************************************
void
Timer1AIntHandler(void)
{
    //
    // Clear the Timer interrupt.
    //
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    //
    // Increment the running count of timer ticks, based on the Timer1A Tick
    // interrupt rate.
    //
    g_ulUITickCount += (SYSTEM_CLOCK / TIMER1A_INT_RATE);
}


//*****************************************************************************
//
// Handles the SysTick interrupt.
//
// This function is called when SysTick asserts its interrupt.  It is
// responsible for handling the on-board user interface elements (push button
// and potentiometer) if enabled, and the processor usage computation.
//
// \return None.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    unsigned long ulIdx, ulCount;

    //
    // Run the Hall module tick handler.
    //
    HallTickHandler();

    //
    // Run the ADC module tick handler.
    //
    ADCTickHandler();

    //
    // Convert the ADC Analog Input reading to milli-volts.  Each volt at the
    // ADC input corresponds to ~1.714 volts at the Analog Input.
    //
    ulCount = ADCReadAnalog();
    g_usAnalogInputVoltage = (((g_usAnalogInputVoltage * 3) +
        (((ulCount * 3000 * 240) / 140) / 1024)) / 4);

    //
    // Read the on-board switch and pass its current value to the switch
    // debouncer, only if the onboard user interface is enabled.
    //
    UIOnboardSwitchDebouncer(GPIOPinRead(PIN_SWITCH_PORT, PIN_SWITCH_PIN));

    //
    // Increment the blink counter.
    //
    g_ulBlinkCount++;

    //
    // Loop through the two LEDs.
    //
    for(ulIdx = 0; ulIdx < 2; ulIdx++)
    {
        //
        // See if this LED is enabled for blinking.
        //
        if(g_pusBlinkRate[ulIdx] != 0)
        {
            //
            // Get the count in terms of the clock for this LED.
            //
            ulCount = g_ulBlinkCount % g_pusBlinkRate[ulIdx];

            //
            // The LED should be turned on when the count is zero.
            //
            if(ulCount == 0)
            {
                GPIOPinWrite(g_pulLEDBase[ulIdx], g_pucLEDPin[ulIdx],
                             (ulIdx == 0) ? 0 : g_pucLEDPin[ulIdx]);
            }

            //
            // The LED should be turned off when the count equals the period.
            //
            if(ulCount == g_pusBlinkPeriod[ulIdx])
            {
                GPIOPinWrite(g_pulLEDBase[ulIdx], g_pucLEDPin[ulIdx],
                             (ulIdx == 0) ? g_pucLEDPin[0] : 0);
            }
        }
    }
}

//*****************************************************************************
//
// Initializes the user interface.
//
// This function initializes the user interface modules (on-board and serial),
// preparing them to operate and control the motor drive.
//
// \return None.
//
//*****************************************************************************
void
UIInit(void)
{
    volatile int iLoop;

    //
    //
    // Make the push button pin be a GPIO input.
    //
    GPIOPinTypeGPIOInput(PIN_SWITCH_PORT, PIN_SWITCH_PIN);
    GPIOPadConfigSet(PIN_SWITCH_PORT, PIN_SWITCH_PIN, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);

    //
    // Make the LEDs be GPIO outputs and turn them off.
    //
    GPIOPinTypeGPIOOutput(PIN_LEDRUN_PORT, PIN_LEDRUN_PIN);
    GPIOPinTypeGPIOOutput(PIN_LEDFAULT_PORT, PIN_LEDFAULT_PIN);
    GPIOPinWrite(PIN_LEDRUN_PORT, PIN_LEDRUN_PIN, 0);
    GPIOPinWrite(PIN_LEDFAULT_PORT, PIN_LEDFAULT_PIN, 0);

    //
    // Configure and read the configuration switches and store the values
    // for future reference.
    // 
    GPIOPinTypeGPIOInput(PIN_CFG0_PORT,
                         (PIN_CFG0_PIN | PIN_CFG1_PIN | PIN_CFG2_PIN));
    GPIOPadConfigSet(PIN_CFG0_PORT,
                    (PIN_CFG0_PIN | PIN_CFG1_PIN | PIN_CFG2_PIN),
                     GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);
    for(iLoop = 0; iLoop < 10000; iLoop++)
    {
    }
    g_ulConfigSwitch = ((GPIOPinRead(PIN_CFG0_PORT,
                        (PIN_CFG1_PIN | PIN_CFG0_PIN)) >> 2) & 0x03);

    //
    // Initialize the on-board user interface.
    //
    UIOnboardInit(GPIOPinRead(PIN_SWITCH_PORT, PIN_SWITCH_PIN), 0);

    //
    // Configure SysTick to provide a periodic user interface interrupt.
    //
    SysTickPeriodSet(SYSTEM_CLOCK / UI_INT_RATE);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Configure and enable a timer to provide a periodic interrupt.
    //
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, (SYSTEM_CLOCK / TIMER1A_INT_RATE));
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER1A);
    TimerEnable(TIMER1_BASE, TIMER_A);

    //
    // Configure the Hall sensor support routines.
    //
    HallConfigure();

    //
    // Configure the ADC support routines.
    //
    ADCConfigure();

    //
    // Configure the PWM generators.
    //
    MainSetPWMFrequency();
    PWMSetDeadBand();
    PWMSetMinPulseWidth();

    //
    // Update the Speed/Power PI controller.
    //
    MainUpdateFAdjI(UI_PARAM_SPEED_I);
    MainUpdatePAdjI(UI_PARAM_SPEED_I);

    //
    // Set the main speed/power target.
    //
    MainSetSpeed();
    MainSetPower();

    //
    // Clear any fault conditions that might exist.
    // 
    MainClearFaults();
}

//*****************************************************************************
//
// Close the Doxygen group.
// @}
//
//*****************************************************************************
