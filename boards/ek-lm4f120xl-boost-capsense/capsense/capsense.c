//*****************************************************************************
//
// capsense.c - Capacitive touch example.
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "drivers/buttons.h"

#include "drivers/CTS_structure.h"
#include "drivers/CTS_Layer.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Capacitive Touch Example (capsense)</h1>
//!
//! An example that works with the 430BOOST-SENSE1 capactive sense 
//! BoosterPack, originally designed for the MSP430 LaunchPad.  
//!
//! The Stellaris LM4F120H5QR does not have the capacitive sense hardware
//! assisted peripheral features of some MSP430 chips. Therefore it is
//! required that the user install surface mount resistors on the pads
//! provided on the bottom of the capacitive sense BoosterPack. Resistor values
//! of 30 ohms are recommended. Calibration may be required for
//! resistor values other than 30 ohms.
//!
//
//*****************************************************************************


//*****************************************************************************
//
// Macro definitions
//
//*****************************************************************************
#define TESTMODE
#define NUM_OSCILLATIONS                    100

#define WAKE_UP_UART_CODE                   0xBE
#define WAKE_UP_UART_CODE2                  0xEF
#define SLEEP_MODE_UART_CODE                0xDE
#define SLEEP_MODE_UART_CODE2               0xAD
#define MIDDLE_BUTTON_CODE                  0x80
#define INVALID_CONVERTED_POSITION          0xFD
#define GESTURE_START                       0xFC
#define GESTURE_STOP                        0xFB
#define COUNTER_CLOCKWISE                   1
#define CLOCKWISE                           2
#define GESTURE_POSITION_OFFSET             0x20
#define WHEEL_POSITION_OFFSET               0x30

#define WHEEL_TOUCH_DELAY                   12
#define MAX_IDLE_TIME                       200
#define PROXIMITY_THRESHOLD                 60


//*****************************************************************************
//
// Global variables
//
//*****************************************************************************
unsigned long g_ulUpBaseline = 0;
unsigned long g_ulDownBaseline = 0;
unsigned long g_ulLeftBaseline = 0;
unsigned long g_ulRightBaseline = 0;
unsigned long g_ulCenterBaseline = 0;
unsigned long g_ulWheelPosition = ILLEGAL_SLIDER_WHEEL_POSITION;
unsigned long ulPreviousWheelPosition = ILLEGAL_SLIDER_WHEEL_POSITION;


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
// Delay for count milliseconds
//
//*****************************************************************************
void
DelayMs(unsigned long ulCount)
{
    SysCtlDelay((SysCtlClockGet()/3000)*ulCount);
}

//*****************************************************************************
//
// Abstraction for LED output.
//
//*****************************************************************************
void
LEDOutput(unsigned long ulWheelPosition)
{
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0);
    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_5);
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_6);
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_7);

    switch(ulWheelPosition)
    {
        case 0:
            break;
        case 1:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
            GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_5);
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0);
            break;
        case 2:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
            GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_5);
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0);
            //P1-5OFF
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, 0);
            break;
        case 3:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
            //P1-5OFF
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, 0);
            break;
        case 4:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
            //P1-5OFF
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, 0);
            //P1-6OFF
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0);
            break;
        case 5:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
            //P1-6OFF
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0);
            break;
        case 6:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
            //P1-6OFF
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0);
            //P1-7OFF
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_7);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0);
            break;
        case 7:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
            //P1-7OFF
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_7);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0);
            break;
        case 8:
            break;
        case 9:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0);
            //P1-7ON
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_7);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_PIN_7);
            break;
        case 10:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0);
            //P1-7ON
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_7);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_PIN_7);
            //P1-6ON
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, GPIO_PIN_6);
            break;
        case 11:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0);
            //P1-6ON
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, GPIO_PIN_6);
            break;
        case 12:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0);
            //P1-6ON
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, GPIO_PIN_6);
            //P1-5ON
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_PIN_4);
            break;
        case 13:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0);
            //P1-5ON
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_PIN_4);
            break;
        case 14:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0);
            //P1-5ON
            GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_PIN_4);
            //P1-4ON
            GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_5);
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_PIN_5);
            break;
        case 15:
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0);
            //P1-4ON
            GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_5);
            GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_PIN_5);
            break;
        default:
            break;
    }
}
//*****************************************************************************
//
// Get Gesture.
//
//*****************************************************************************
unsigned char GetGesture(unsigned char ulWheelPosition)
{
    unsigned char gesture = INVALID_CONVERTED_POSITION;
    unsigned char ucDirection, ccw_check, cw_check;

    if(ulPreviousWheelPosition != ILLEGAL_SLIDER_WHEEL_POSITION)
    {
        if(ulPreviousWheelPosition > ulWheelPosition)
        {
            ccw_check = ulPreviousWheelPosition - ulWheelPosition;
            if(ccw_check < 8)
            {
                gesture = ccw_check;
                ucDirection = COUNTER_CLOCKWISE;
            }
            else
            {
                cw_check = 16 - ulPreviousWheelPosition + ulWheelPosition;
                if(cw_check < 8)
                {
                    gesture = cw_check;
                    ucDirection = CLOCKWISE;
                }
            }
        }
        else
        {
            cw_check = ulWheelPosition - ulPreviousWheelPosition;
            if(cw_check < 8)
            {
                gesture = cw_check;
                ucDirection = CLOCKWISE;
            }
            else
            {
                ccw_check = 16 + ulPreviousWheelPosition - ulWheelPosition;
                if(ccw_check < 8)
                {
                    gesture = ccw_check;
                    ucDirection = COUNTER_CLOCKWISE;
                }
            }
        }
    }
    if(gesture == INVALID_CONVERTED_POSITION)
        return gesture;
    if(ucDirection == COUNTER_CLOCKWISE)
        return(gesture + 16);
    else
        return gesture;
}
//*****************************************************************************
//
// Main cap-touch example.
//
//*****************************************************************************
int
main(void)
{
    unsigned char ucCenterButtonTouched;
    unsigned long ulWheelTouchCounter;
    unsigned char ucConvertedWheelPosition;
    unsigned char ucGestureDetected;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPUEnable();
    ROM_FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the PLL at 80 MHz
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    //
    // Initialize a few GPIO outputs for the LEDs
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_5);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_6);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_7);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_5);

    //
    // Turn on the Center LED
    //
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, GPIO_PIN_5);

    //
    // Initialize the UART.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInitExpClk(0,9600);

    //
    // First, I'm configuring the pins as outputs to take care of all of the
    // non-recurring setup.
    //
    ROM_SysCtlGPIOAHBEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinTypeGPIOOutput(GPIO_PORTA_AHB_BASE, GPIO_PIN_2);
    GPIOPinTypeGPIOOutput(GPIO_PORTA_AHB_BASE, GPIO_PIN_3);
    GPIOPinTypeGPIOOutput(GPIO_PORTA_AHB_BASE, GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTA_AHB_BASE, GPIO_PIN_6);
    GPIOPinTypeGPIOOutput(GPIO_PORTA_AHB_BASE, GPIO_PIN_7);

    //
    // Start up Systick to measure time
    //
    SysTickPeriodSet(0x00FFFFFF);
    SysTickEnable();

    //
    // Set the baseline capacitance measurements for our wheel and center
    // button.
    //
    TI_CAPT_Init_Baseline(&g_sSensorWheel);
    TI_CAPT_Update_Baseline(&g_sSensorWheel,2);
    TI_CAPT_Init_Baseline(&g_sMiddleButton);
    TI_CAPT_Update_Baseline(&g_sMiddleButton,2);

    //
    // Send the Sleep code
    //
    UARTprintf("\0xDE\0xAD");

    //
    // Perform the LED startup sequence, and tell the GUI we're alive.
    //
    UARTprintf("\0xBE\0xEF");

    //
    // Assume that the center button starts off "untouched", the wheel has no
    // position, and no gestures are in progress.
    //
    ucCenterButtonTouched = 0;
    ucConvertedWheelPosition = INVALID_CONVERTED_POSITION;
    ucGestureDetected = 0;

    //
    // Set the wheel counter for the default delay period.
    //
    ulWheelTouchCounter = WHEEL_TOUCH_DELAY - 1;
    
    //
    // Go into active mode.
    //
    while(1)
    {
        g_ulWheelPosition = ILLEGAL_SLIDER_WHEEL_POSITION;
        g_ulWheelPosition = TI_CAPT_Wheel(&g_sSensorWheel);

        if(g_ulWheelPosition != ILLEGAL_SLIDER_WHEEL_POSITION)
        {
            ucCenterButtonTouched = 0;

            if(g_ulWheelPosition < 0x08)
            {
                g_ulWheelPosition += 0x40 - 0x08;
            }
            else
            {
                g_ulWheelPosition -= 0x08;
            }

            g_ulWheelPosition = g_ulWheelPosition >> 2;

            ucConvertedWheelPosition = GetGesture(g_ulWheelPosition);

            //
            // Add hysteresis to reduce toggling between g_sSensorWheel
            // positions if no gesture has been TRULY detected.
            //
            if((ucGestureDetected==0) && ((ucConvertedWheelPosition<=1) ||
                                          (ucConvertedWheelPosition==0x11) ||
                                          (ucConvertedWheelPosition==0x10)))
            {
                if (ulPreviousWheelPosition != ILLEGAL_SLIDER_WHEEL_POSITION)
                    g_ulWheelPosition = ulPreviousWheelPosition;
                ucConvertedWheelPosition = 0;
            }

            //
            //Turn on corresponding LED(s)
            //
            LEDOutput(g_ulWheelPosition);

            //
            // A gesture has been detected
            //
            if((ucConvertedWheelPosition != 0) &&
               (ucConvertedWheelPosition != 16) &&
               (ucConvertedWheelPosition != INVALID_CONVERTED_POSITION))
            {
                //
                // Starting of a new gesture sequence
                //
                if (ucGestureDetected ==0)
                {
                    ucGestureDetected = 1;

                    //
                    // Transmit gesture start status update & position via UART
                    // to PC
                    //
                    UARTCharPut(UART0_BASE, GESTURE_START);
                    UARTCharPut(UART0_BASE,ulPreviousWheelPosition +
                                           GESTURE_POSITION_OFFSET);
                }

                //
                // Transmit gesture & position via UART to PC
                //
                UARTCharPut(UART0_BASE,ucConvertedWheelPosition);
                UARTCharPut(UART0_BASE,g_ulWheelPosition +
                                       GESTURE_POSITION_OFFSET);
            }
            else
            {
                //
                // If no gesture was detected, this is constituted as a
                // touch/tap
                //
                if (ucGestureDetected==0)
                { 
                    if (++ulWheelTouchCounter >= WHEEL_TOUCH_DELAY)
                    {
                        //
                        // Transmit wheel position [twice] via UART to PC
                        //
                        ulWheelTouchCounter = 0;
                        UARTCharPut(UART0_BASE,g_ulWheelPosition +
                                               WHEEL_POSITION_OFFSET);
                        UARTCharPut(UART0_BASE,g_ulWheelPosition +
                                               WHEEL_POSITION_OFFSET);
                    }
                }
                else
                {
                    ulWheelTouchCounter = WHEEL_TOUCH_DELAY - 1;
                }
            }

            ulPreviousWheelPosition = g_ulWheelPosition;
        }
        else
        {
            //
            // no wheel position was detected
            //

            if(TI_CAPT_Button(&g_sMiddleButton))
            {
                //
                // Middle button was touched
                //
                if (ucCenterButtonTouched==0)
                {
                    //
                    // Transmit center button code [twice] via UART to PC
                    //
                    UARTCharPut(UART0_BASE,MIDDLE_BUTTON_CODE);
                    UARTCharPut(UART0_BASE,MIDDLE_BUTTON_CODE);

                    ucCenterButtonTouched = 1;

                    if(GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_5))
                    {
                        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0);
                    }
                    else
                    {
                        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, GPIO_PIN_5);
                    }
                }
            }
            else
            {
                //
                // No touch was registered at all [Not wheel or center button]
                //
                ucCenterButtonTouched = 0;

                if((ucConvertedWheelPosition == INVALID_CONVERTED_POSITION) ||
                   (ucGestureDetected ==0))
                {
                    //
                    // No gesture was registered previously
                    //
                    if(ulPreviousWheelPosition !=
                       ILLEGAL_SLIDER_WHEEL_POSITION)
                    {
                        //
                        // Transmit last wheel position [twice] via UART to PC
                        //
                        UARTCharPut(UART0_BASE,ulPreviousWheelPosition +
                                               WHEEL_POSITION_OFFSET );
                        UARTCharPut(UART0_BASE,ulPreviousWheelPosition +
                                               WHEEL_POSITION_OFFSET );
                        ulWheelTouchCounter = WHEEL_TOUCH_DELAY - 1;
                    }
                }

                if (ucGestureDetected == 1)
                {
                    //
                    // A gesture was registered previously
                    //

                    //
                    // Transmit status update: stop gesture tracking [twice]
                    // via UART to PC
                    //
                    UARTCharPut(UART0_BASE,GESTURE_STOP);
                    UARTCharPut(UART0_BASE,GESTURE_STOP);
                }

            }

            //
            // Reset all touch conditions, turn off LEDs,
            //
            LEDOutput(0);
            ulPreviousWheelPosition= ILLEGAL_SLIDER_WHEEL_POSITION;
            ucConvertedWheelPosition = INVALID_CONVERTED_POSITION;
            ucGestureDetected = 0;

        }

        //
        // Option: Add delay/sleep cycle here to reduce active duty cycle. This
        // lowers power consumption but sacrifices wheel responsiveness.
        // Additional timing refinement must be taken into consideration when
        // interfacing with PC applications GUI to retain proper communication
        // protocol.
        //

        DelayMs(50);

    }
}
