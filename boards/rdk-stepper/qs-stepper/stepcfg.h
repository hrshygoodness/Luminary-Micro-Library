//*****************************************************************************
//
// stepcfg.h - Definitions for stepper configuration
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
// This is part of revision 8555 of the RDK-STEPPER Firmware Package.
//
//*****************************************************************************

#ifndef __STEPCFG_H__
#define __STEPCFG_H__

//*****************************************************************************
//
//! \page stepcfg_intro Introduction
//!
//! The <tt>stepcfg.h</tt> file contains all the configuration macros
//! for assigning the microcontroller resources used by the Stepper
//! RDK firmware.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup stepcfg_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Computes the raw ADC counts that represent a current value.
//!
//! \param m is the current in milliamps.
//!
//! This macro will convert a value of current in milliamps to the
//! value in ADC counts. This is used to get the ADC value that represents
//! the threshold current for chopper mode.
//!
//! \return Returns the value of the current in units of ADC counts.
//
//*****************************************************************************
#define MILLIAMPS2COUNTS(m) (((m) * 11253) / 30000)

//*****************************************************************************
//
//! Computes the current in milliamps from raw ADC counts.
//!
//! \param c is the current in raw ADC counts.
//!
//! This macro will convert a value of current in raw ADC counts as read from
//! the A/D converter, to the value in milliamps.
//!
//! \return Returns the value of the current in units of milliamps.
//
//*****************************************************************************
#define COUNTS2MILLIAMPS(c) (((c) * 30000) / 11253)

//*****************************************************************************
//
//! Defines the processor clock frequency. In order to change the processor
//! clock, the initialization code in main() must be changed, and this macro
//! must be changed to match.
//
//*****************************************************************************
#define SYSTEM_CLOCK                50000000L

//*****************************************************************************
//
//! Defines the interrupt priority for the current fault comparator.
//
//*****************************************************************************
#define COMP_INT_PRI                0x00

//*****************************************************************************
//
//! Defines the interrupt priority for the step timer.
//
//*****************************************************************************
#define STEP_TMR_INT_PRI            0x80

//*****************************************************************************
//
//! Defines the interrupt priority for the fixed-interval timers.
//
//*****************************************************************************
#define FIXED_TMR_INT_PRI           0x80

//*****************************************************************************
//
//! Defines the interrupt priority for the ADC interrupt for current sampling.
//
//*****************************************************************************
#define ADC_INT_PRI                 0x80

//*****************************************************************************
//
//! Defines the interrupt priority for the system tick timer.
//
//*****************************************************************************
#define SYSTICK_INT_PRI             0xa0

//*****************************************************************************
//
//! Defines the interrupt priority for the serial user interface and UART.
//
//*****************************************************************************
#define UI_SER_INT_PRI              0xc0

//*****************************************************************************
//
//! Defines the timer used for the fixed-interval timer.
//
//*****************************************************************************
#define FIXED_TMR_BASE              TIMER1_BASE
#define FIXED_TMR_PERIPH            SYSCTL_PERIPH_TIMER1
#define FIXED_TMR_INT_A             INT_TIMER1A
#define FIXED_TMR_INT_B             INT_TIMER1B

//*****************************************************************************
//
//! Defines the timer used for the step timer.
//
//*****************************************************************************
#define STEP_TMR_BASE               TIMER0_BASE
#define STEP_TMR_PERIPH             SYSCTL_PERIPH_TIMER0
#define STEP_TMR_INT                INT_TIMER0A

//*****************************************************************************
//
//! Defines the ADC sequencer to be used for measurements in the
//! user interface.
//
//*****************************************************************************
#define UI_ADC_SEQUENCER            0
#define UI_ADC_PRIORITY             UI_ADC_SEQUENCER

//*****************************************************************************
//
//! Defines the ADC sequencer to be used for chopping winding A.
//
//*****************************************************************************
#define WINDING_A_ADC_SEQUENCER     1
#define WINDING_A_ADC_PRIORITY      WINDING_A_ADC_SEQUENCER
#define WINDING_A_ADC_INT           (INT_ADC0SS0 + WINDING_A_ADC_SEQUENCER)

//*****************************************************************************
//
//! Defines the ADC sequencer to be used for chopping winding B.
//
//*****************************************************************************
#define WINDING_B_ADC_SEQUENCER     2
#define WINDING_B_ADC_PRIORITY      WINDING_B_ADC_SEQUENCER
#define WINDING_B_ADC_INT           (INT_ADC0SS0 + WINDING_B_ADC_SEQUENCER)

//*****************************************************************************
//
//! Defines the ADC channel for winding A current sense.
//
//*****************************************************************************
#define WINDING_A_ADC_CHANNEL       ADC_CTL_CH0

//*****************************************************************************
//
//! Defines the ADC channel for winding B current sense.
//
//*****************************************************************************
#define WINDING_B_ADC_CHANNEL       ADC_CTL_CH1

//*****************************************************************************
//
//! Defines the ADC channel to be used for the bus voltage.
//
//*****************************************************************************
#define BUSV_ADC_CHAN               ADC_CTL_CH3

//*****************************************************************************
//
//! Defines the ADC channel to be used for the potentiometer.
//
//*****************************************************************************
#define POT_ADC_CHAN                ADC_CTL_CH4

//*****************************************************************************
//
//! Defines the GPIO port for the user button.
//
//*****************************************************************************
#define USER_BUTTON_GPIO_PERIPH SYSCTL_PERIPH_GPIOB
#define USER_BUTTON_PORT        GPIO_PORTB_BASE

//*****************************************************************************
//
//! Defines the GPIO pin for the user button.
//
//*****************************************************************************
#define USER_BUTTON_PIN_NUM     2
#define USER_BUTTON_PIN         (1 << USER_BUTTON_PIN_NUM)

//*****************************************************************************
//
//! Defines the GPIO port for the status LED.
//
//*****************************************************************************
#define STATUS_LED_PORT         GPIO_PORTD_BASE
#define LED_GPIO_PERIPH         SYSCTL_PERIPH_GPIOD

//*****************************************************************************
//
//! Defines the GPIO pin for the status LED.
//
//*****************************************************************************
#define STATUS_LED_PIN_NUM      4
#define STATUS_LED_PIN          (1 << STATUS_LED_PIN_NUM)
#define STATUS_LED              0

//*****************************************************************************
//
//! Defines the GPIO port for the mode LED.
//
//*****************************************************************************
#define MODE_LED_PORT           GPIO_PORTD_BASE

//*****************************************************************************
//
//! Defines the GPIO pin for the status LED.
//
//*****************************************************************************
#define MODE_LED_PIN_NUM        5
#define MODE_LED_PIN            (1 << MODE_LED_PIN_NUM)
#define MODE_LED                1

//*****************************************************************************
//
//! The address of the first block of flash to be used for storing parameters.
//
//*****************************************************************************
#define FLASH_PB_START              0x00007000

//*****************************************************************************
//
//! The address of the last block of flash to be used for storing parameters.
//! Since the end of flash is used for parameters, this is actually the first
//! address past the end of flash.
//
//*****************************************************************************
#define FLASH_PB_END                0x00008000

//*****************************************************************************
//
//! The size of the parameter block to save. This must be a power of 2,
//! and should be large enough to contain the tDriveParameters structure
//! (see the uiparms.h file).
//
//*****************************************************************************
#define FLASH_PB_SIZE               64

//*****************************************************************************
//
//! Defines the index value for winding A. This is used in places as an
//! index for looking up values needed for winding A.
//
//*****************************************************************************
#define WINDING_ID_A                0

//*****************************************************************************
//
//! Defines the index value for winding B. This is used in places as an
//! index for looking up values needed for winding B.
//
//*****************************************************************************
#define WINDING_ID_B                1

//*****************************************************************************
//
//! Defines a value indicating that the fast decay mode should be used.
//
//*****************************************************************************
#define DECAY_MODE_FAST             0

//*****************************************************************************
//
//! Defines a value indicating that the slow decay mode should be used.
//
//*****************************************************************************
#define DECAY_MODE_SLOW             1

//*****************************************************************************
//
//! Defines a value meaning that the Open-loop PWM current control method
//! should be used.
//
//*****************************************************************************
#define CONTROL_MODE_OPENPWM        0

//*****************************************************************************
//
//! Defines a value indicating that the chopper current control method should
//! be used.
//
//*****************************************************************************
#define CONTROL_MODE_CHOP           1

//*****************************************************************************
//
//! Defines a value meaning that the Closed-loop PWM current control method
//! should be used.
//
//*****************************************************************************
#define CONTROL_MODE_CLOSEDPWM      2

//*****************************************************************************
//
//! Defines a value meaning that full (normal) stepping should be used.
//
//*****************************************************************************
#define STEP_MODE_FULL              0

//*****************************************************************************
//
//! Defines a value meaning that half stepping should be used.
//
//*****************************************************************************
#define STEP_MODE_HALF              1

//*****************************************************************************
//
//! Defines a value meaning that micro stepping should be used.
//
//*****************************************************************************
#define STEP_MODE_MICRO             2

//*****************************************************************************
//
//! Defines a value meaning that wave drive stepping should be used.
//! Wave drive is whole stepping, with one winding on at a time.
//
//*****************************************************************************
#define STEP_MODE_WAVE              3

//*****************************************************************************
//
//! Defines the fault flag indicating an over-current fault occurred.
//
//*****************************************************************************
#define FAULT_FLAG_CURRENT          0x01

#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
