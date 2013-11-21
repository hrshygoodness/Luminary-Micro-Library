//*****************************************************************************
//
// sensors.c - Driver for the bump sensors and wheel encoders.
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
#include "inc/hw_i2c.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/interrupt.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "sensors.h"

//*****************************************************************************
//
//! \addtogroup sensors_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Hardware resources associated with the IR sensors used to detect speed and
// position.  The hardware supports two sensors on each wheel to allow speed
// and direction to be calculated but this driver makes use of only one of
// these for simplicity.  It is assumed that the wheel drive direction is as
// commanded.
//
//*****************************************************************************
#define LEFT_RIGHT_IR_LED_PERIPH      SYSCTL_PERIPH_GPIOE
#define LEFT_RIGHT_IR_LED_PORT        GPIO_PORTE_BASE
#define LEFT_RIGHT_IR_LED_PIN         GPIO_PIN_6

#define LEFT_IR_SENSOR_PERIPH         SYSCTL_PERIPH_GPIOE
#define LEFT_IR_SENSOR_PORT           GPIO_PORTE_BASE
#define LEFT_IR_SENSOR_PIN            GPIO_PIN_3
#define LEFT_IR_SENSOR_INT            INT_GPIOE

#define RIGHT_IR_SENSOR_PERIPH        SYSCTL_PERIPH_GPIOE
#define RIGHT_IR_SENSOR_PORT          GPIO_PORTE_BASE
#define RIGHT_IR_SENSOR_PIN           GPIO_PIN_2
#define RIGHT_IR_SENSOR_INT           INT_GPIOE

//*****************************************************************************
//
// Pointer to the application function called for each click of the wheel
// position/speed sensor.
//
//*****************************************************************************
static void (*g_pfnWheelCallback)(tWheel);

//*****************************************************************************
//
// Holds the debounced state of the bump sensors.
//
//*****************************************************************************
static unsigned char g_ucDebouncedBumpers = GPIO_PIN_0 | GPIO_PIN_1;

//*****************************************************************************
//
//! Initializes the board's bump sensors.
//!
//! This function must be called before any attempt to read the board bump
//! sensors.  It configures the GPIO ports used by the sensors.
//!
//! \return None.
//
//*****************************************************************************
void
BumpSensorsInit (void)
{
    //
    // Enable the GPIO ports used for the bump sensors.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    //
    // Configure the sensor GPIOs as pulled-up inputs.
    //
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

//*****************************************************************************
//
//! Gets the status of a bump sensors on the board.
//!
//! \param eBumper identifies the sensor whose state is to be returned. Valid
//! values are \e BUMP_RIGHT and \e BUMP_LEFT.
//!
//! This function may be used to determine the current state of one or other of
//! the EVALBOT's front bump sensors.
//!
//! \return Returns \e true if the sensor is closed or \e false if it is open.
//!
//*****************************************************************************
tBoolean BumpSensorGetStatus (tBumper eBumper)
{
    tBoolean  status;

    //
    // Which sensor are we being asked to read?
    //
    switch (eBumper)
    {
        //
        // Return the state of the right sensor.
        //
        case BUMP_RIGHT:
        {
            status = ROM_GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_0) ?
                                     true : false;
            break;
        }

        //
        // Return the state of the left sensor.
        //
        case BUMP_LEFT:
        {
            status = ROM_GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_1) ?
                                     true : false;
            break;
        }

        //
        // This case should never be seen since tSide only contains two
        // possible values.  Some people have been known to cast integers
        // into enums, though, so....
        //
        default:
        {
            status = false;
            break;
        }
    }

    return (status);
}

//*****************************************************************************
//
//! Debounces the EVALBOT sensor switches when called periodically.
//!
//! If bump sensor debouncing is used, this function should be called
//! periodically, for example every 10 ms.  It will check the bump sensor
//! state and save a debounced state that can be read by the application at
//! any time.
//!
//! \return None.
//
//*****************************************************************************
void
BumpSensorDebouncer(void)
{
    static unsigned char ucBumperClockA = 0;
    static unsigned char ucBumperClockB = 0;
    unsigned char ucData;
    unsigned char ucDelta;

    //
    // Read the current state of the hardware bump sensors
    //
    ucData = ROM_GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Determine bumpers that are in a different state from the debounced state
    //
    ucDelta = ucData ^ g_ucDebouncedBumpers;

    //
    // Increment the debounce counter by one
    //
    ucBumperClockA ^= ucBumperClockB;
    ucBumperClockB = ~ucBumperClockB;

    //
    // Reset the debounce counter for any bumper that is unchanged
    //
    ucBumperClockA &= ucDelta;
    ucBumperClockB &= ucDelta;

    //
    // Determine the new debounced bumper state based on the debounce
    // counter.
    //
    g_ucDebouncedBumpers &= ucBumperClockA | ucBumperClockB;
    g_ucDebouncedBumpers |= (~(ucBumperClockA | ucBumperClockB)) & ucData;
}

//*****************************************************************************
//
//! Gets the debounced state of a bump sensors on the board.
//!
//! \param eBumper identifies the sensor whose state is to be returned. Valid
//! values are \e BUMP_RIGHT and \e BUMP_LEFT.
//!
//! This function may be used to determine the debounced state of one or other
//! of the EVALBOT's front bump sensors.  If this function is used, then the
//! application must periodically call the function BumpSensorDebouncer().
//!
//! \return Returns \e true if the sensor is closed or \e false if it is open.
//!
//*****************************************************************************
tBoolean BumpSensorGetDebounced(tBumper eBumper)
{
    tBoolean  status;

    //
    // Which sensor are we being asked to read?
    //
    switch (eBumper)
    {
        //
        // Return the state of the right sensor.
        //
        case BUMP_RIGHT:
        {
            status = g_ucDebouncedBumpers & GPIO_PIN_0 ? true : false;
            break;
        }

        //
        // Return the state of the left sensor.
        //
        case BUMP_LEFT:
        {
            status = g_ucDebouncedBumpers & GPIO_PIN_1 ? true : false;
            break;
        }

        //
        // This case should never be seen since tSide only contains two
        // possible values.  Some people have been known to cast integers
        // into enums, though, so....
        //
        default:
        {
            status = false;
            break;
        }
    }

    return (status);
}

//*****************************************************************************
//
//! Initializes the infrared wheel sensors.
//!
//! \param pfnCallback is a pointer to the function which will be called on
//! each pulse from the wheel sensors.  It may be null to disable callbacks.
//!
//! This function must be called to initialize the infrared sensors used to
//! calculate actual EVALBOT speed.  If a non-NULL function pointer is provided
//! in the \e pfnCallback parameters, this function will be called on each
//! pulse from a wheel sensor when they are enabled.  Note that this callback
//! is made in interrupt context.
//!
//! \return None.
//
//*****************************************************************************
void
WheelSensorsInit(void (*pfnCallback)(tWheel eWheel))
{
    //
    // Remember the application's callback function pointer.
    //
    g_pfnWheelCallback = pfnCallback;

    //
    // Enable the GPIO ports used for the wheel encoders.
    //
    ROM_SysCtlPeripheralEnable(LEFT_RIGHT_IR_LED_PERIPH);
    ROM_SysCtlPeripheralEnable(LEFT_IR_SENSOR_PERIPH);
    ROM_SysCtlPeripheralEnable(RIGHT_IR_SENSOR_PERIPH);

    //
    // Configure the sensor inputs.
    //
    ROM_GPIOPinTypeGPIOInput(LEFT_IR_SENSOR_PORT, LEFT_IR_SENSOR_PIN);
    ROM_GPIOPinTypeGPIOInput(RIGHT_IR_SENSOR_PORT, RIGHT_IR_SENSOR_PIN);

    //
    // Configure the LED outputs.  Initially turn the LEDs off by setting the
    // pins high.
    //
    ROM_GPIOPinTypeGPIOOutput(LEFT_RIGHT_IR_LED_PORT, LEFT_RIGHT_IR_LED_PIN);
    ROM_GPIOPadConfigSet(LEFT_RIGHT_IR_LED_PORT, LEFT_RIGHT_IR_LED_PIN,
                         GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
    ROM_GPIOPinWrite(LEFT_RIGHT_IR_LED_PORT, LEFT_RIGHT_IR_LED_PIN,
                     LEFT_RIGHT_IR_LED_PIN);

    //
    // Disable all of the pin interrupts
    //
    ROM_GPIOPinIntDisable(LEFT_IR_SENSOR_PORT, LEFT_IR_SENSOR_PIN);
    ROM_GPIOPinIntDisable(RIGHT_IR_SENSOR_PORT, RIGHT_IR_SENSOR_PIN);

    ROM_GPIOIntTypeSet(LEFT_IR_SENSOR_PORT, LEFT_IR_SENSOR_PIN,
                       GPIO_RISING_EDGE);
    ROM_GPIOIntTypeSet(RIGHT_IR_SENSOR_PORT, RIGHT_IR_SENSOR_PIN,
                       GPIO_RISING_EDGE);

    //
    // Enable the GPIO port interrupts for the inputs.  The interrupts for the
    // individual pins still need to be enabled by WheelSensorIntEnable().
    //
    ROM_IntEnable(LEFT_IR_SENSOR_INT);
    ROM_IntEnable(RIGHT_IR_SENSOR_INT);
}

//*****************************************************************************
//!
//! Enables the LEDs for both EVALBOT wheel sensors.
//!
//! This function enables the LEDs used by both EVALBOT wheel sensors.
//! When the sensors are enabled, notification of sensor pulses will be
//! made to the \e pfnCallback function passed to WheelSensorsInit() for that
//! wheel assuming sensor interrupts for a given wheel have also been enabled
//! by a previous call to WheelSensorIntEnable().  The sensors may be
//! disabled by calling WheelSensorDisable().
//!
//! \return None.
//
//*****************************************************************************
void
WheelSensorEnable(void)
{
    //
    // Turn on the LEDs by setting the pin high.
    //
    ROM_GPIOPinWrite(LEFT_RIGHT_IR_LED_PORT, LEFT_RIGHT_IR_LED_PIN,
                     LEFT_RIGHT_IR_LED_PIN);
}

//*****************************************************************************
//!
//! Disables the LEDs for both EVALBOT wheel sensors.
//!
//! This function disables the LEDs used by both EVALBOT wheel sensors.
//! When the sensors are disabled, no notification of sensor pulses will be
//! made to the \e pfnCallback function passed to WheelSensorsInit() for that
//! wheel.  The sensors may be reenabled by calling WheelSensorEnable().
//!
//! \return None.
//
//*****************************************************************************
void
WheelSensorDisable(void)
{
    //
    // Turn off the LEDs by setting the pin low.
    //
    ROM_GPIOPinWrite(LEFT_RIGHT_IR_LED_PORT, LEFT_RIGHT_IR_LED_PIN, 0);
}

//*****************************************************************************
//!
//! Enables the interrupts from an infrared wheel sensor.
//!
//! \param eWheel defines which wheel sensor interrupt to enable.  Valid values
//! are \e WHEEL_LEFT and \e WHEEL_RIGHT.
//!
//! This function enables the wheel sensor interrupt from one EVALBOT wheel.
//! When a sensor interrupt is enabled, callbacks will be made to the \e
//! pfnCallback function passed to WheelSensorsInit() for that wheel.
//! Interrupts may be disabled by calling WheelSensorIntDisable().
//!
//! \return None.
//
//*****************************************************************************
void
WheelSensorIntEnable(tWheel eWheel)
{
    //
    // Check for a valid parameter value.
    //
    ASSERT((eWheel == WHEEL_LEFT) || (eWheel == WHEEL_RIGHT));

    //
    // Enable the interrupt for the specified wheel.
    //
    if(eWheel == WHEEL_LEFT)
    {
        ROM_GPIOPinIntClear(LEFT_IR_SENSOR_PORT, LEFT_IR_SENSOR_PIN);
        ROM_GPIOPinIntEnable(LEFT_IR_SENSOR_PORT, LEFT_IR_SENSOR_PIN);
    }
    else
    {
        ROM_GPIOPinIntClear(RIGHT_IR_SENSOR_PORT, RIGHT_IR_SENSOR_PIN);
        ROM_GPIOPinIntEnable(RIGHT_IR_SENSOR_PORT, RIGHT_IR_SENSOR_PIN);
    }
}

//*****************************************************************************
//!
//! Disables the interrupts from an infrared wheel sensor.
//!
//! \param eWheel defines which wheel sensor interrupt to disable.  Valid values
//! are \e WHEEL_LEFT and \e WHEEL_RIGHT.
//!
//! This function disables the wheel sensor interrupt from one EVALBOT wheel.
//! When a sensor interrupt is disabled, no further callbacks will be made to
//! the \e pfnCallback function passed to WheelSensorsInit() for that wheel.
//! Interrupts may be reenabled by calling WheelSensorIntEnable().
//!
//! \return None.
//
//*****************************************************************************
void
WheelSensorIntDisable(tWheel eWheel)
{
    //
    // Check for a valid parameter value.
    //
    ASSERT((eWheel == WHEEL_LEFT) || (eWheel == WHEEL_RIGHT));

    //
    // Disable the interrupt for the specified wheel.
    //
    if(eWheel == WHEEL_LEFT)
    {
        ROM_GPIOPinIntDisable(LEFT_IR_SENSOR_PORT, LEFT_IR_SENSOR_PIN);
    }
    else
    {
        ROM_GPIOPinIntDisable(RIGHT_IR_SENSOR_PORT, RIGHT_IR_SENSOR_PIN);
    }
}

//*****************************************************************************
//
//! Handles interrupts from each of the IR sensors used to determine speed.
//!
//! This interrupt function is called by the processor due to an interrupt
//! on the rising edge signals from each of the wheel sensors and is used to
//! call a callback function to the client application informing it that the
//! wheel has moved.  The application-supplied callback function will typically
//! be used to calculate wheel rotation speed.
//!
//! Applications using the motor driver must hook this function to the interrupt
//! vectors for each GPIO port containing a wheel sensor pin.  For the existing
//! EVALBOT hardware, this is GPIO port E.
//!
//! \return None.
//!
//! \note This function is called by the interrupt system and should not be
//! called directly from application code.
//
//*****************************************************************************
void
WheelSensorIntHandler(void)
{
    unsigned long ulStatus, ulLoop;

    //
    // Was this interrupt from the left wheel sensor?
    //
    ulStatus = ROM_GPIOPinIntStatus(LEFT_IR_SENSOR_PORT, true);
    if (ulStatus & LEFT_IR_SENSOR_PIN)
    {
        //
        // Clear the interrupt.
        //
        ROM_GPIOPinIntClear(LEFT_IR_SENSOR_PORT,
                            LEFT_IR_SENSOR_PIN);

        //
        // Add a short polling loop to reject noise.  If the sensor input goes
        // low inside this loop, we assume we've read a noise spike and ignore
        // it.
        //
        for (ulLoop = 0; ulLoop < 100; ulLoop++)
        {
            if (!ROM_GPIOPinRead(LEFT_IR_SENSOR_PORT, LEFT_IR_SENSOR_PIN))
            {
                return;
            }
        }

        //
        // Tell the app that we got a click from the left wheel sensor.
        //
        if(g_pfnWheelCallback)
        {
            g_pfnWheelCallback(WHEEL_LEFT);
        }
    }

    //
    // Was this from the right side sensor?
    //
    ulStatus = ROM_GPIOPinIntStatus(RIGHT_IR_SENSOR_PORT, true);
    if (ulStatus & RIGHT_IR_SENSOR_PIN)
    {
        //
        // Clear the interrupt.
        //
        ROM_GPIOPinIntClear(RIGHT_IR_SENSOR_PORT,
                            RIGHT_IR_SENSOR_PIN);

        //
        // Add a short polling loop to reject noise.  If the sensor input goes
        // low inside this loop, we assume we've read a noise spike and ignore
        // it.
        //
        for (ulLoop = 0; ulLoop < 100; ulLoop++)
        {
            if (!ROM_GPIOPinRead(RIGHT_IR_SENSOR_PORT, RIGHT_IR_SENSOR_PIN))
            {
                return;
            }
        }

        //
        // Tell the app that we got a click from the right wheel sensor.
        //
        if(g_pfnWheelCallback)
        {
            g_pfnWheelCallback(WHEEL_RIGHT);
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
