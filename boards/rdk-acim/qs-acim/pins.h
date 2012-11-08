//*****************************************************************************
//
// pins.h - Maps logical pin names to physical device pins.
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
// This is part of revision 9453 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#ifndef __PINS_H__
#define __PINS_H__

//*****************************************************************************
//
//! \page pins_intro Introduction
//!
//! The pins on the microcontroller are connected to a variety of external
//! devices.  The defines in this file provide a name more descriptive than
//! "PB0" for the various devices connected to the microcontroller pins.
//!
//! These defines are provided in <tt>pins.h</tt>.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup pins_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The GPIO port on which the UART0 Rx pin resides.
//
//*****************************************************************************
#define PIN_UART0RX_PORT        GPIO_PORTA_BASE

//*****************************************************************************
//
//! The GPIO pin on which the UART0 Rx pin resides.
//
//*****************************************************************************
#define PIN_UART0RX_PIN         GPIO_PIN_0

//*****************************************************************************
//
//! The GPIO port on which the UART0 Tx pin resides.
//
//*****************************************************************************
#define PIN_UART0TX_PORT        GPIO_PORTA_BASE

//*****************************************************************************
//
//! The GPIO pin on which the UART0 Tx pin resides.
//
//*****************************************************************************
#define PIN_UART0TX_PIN         GPIO_PIN_1

//*****************************************************************************
//
//! The GPIO port on which the phase V low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEV_LOW_PORT     GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the phase V low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEV_LOW_PIN      GPIO_PIN_0

//*****************************************************************************
//
//! The GPIO port on which the phase V high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEV_HIGH_PORT    GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the phase V high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEV_HIGH_PIN     GPIO_PIN_1

//*****************************************************************************
//
//! The GPIO port on which the quadrature encoder index pin resides.
//
//*****************************************************************************
#define PIN_INDEX_PORT          GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the quadrature encoder index pin resides.
//
//*****************************************************************************
#define PIN_INDEX_PIN           GPIO_PIN_2

//*****************************************************************************
//
//! The GPIO port on which the user push button resides.
//
//*****************************************************************************
#define PIN_SWITCH_PORT         GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the user push button resides.
//
//*****************************************************************************
#define PIN_SWITCH_PIN          GPIO_PIN_2

//*****************************************************************************
//
//! The bit lane of the GPIO pin on which the user push button resides.
//
//*****************************************************************************
#define PIN_SWITCH_PIN_BIT      2

//*****************************************************************************
//
//! The GPIO port on which the PWM fault pin resides.
//
//*****************************************************************************
#define PIN_FAULT_PORT          GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the PWM fault pin resides.
//
//*****************************************************************************
#define PIN_FAULT_PIN           GPIO_PIN_3

//*****************************************************************************
//
//! The GPIO port on which the overall current sense pin resides.
//
//*****************************************************************************
#define PIN_ISENSE_PORT         GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the overall current sense pin resides.
//
//*****************************************************************************
#define PIN_ISENSE_PIN          GPIO_PIN_4

//*****************************************************************************
//
//! The GPIO port on which the run LED resides.
//
//*****************************************************************************
#define PIN_LEDRUN_PORT         GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the run LED resides.
//
//*****************************************************************************
#define PIN_LEDRUN_PIN          GPIO_PIN_5

//*****************************************************************************
//
//! The GPIO port on which the fault LED resides.
//
//*****************************************************************************
#define PIN_LEDFAULT_PORT       GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the fault LED resides.
//
//*****************************************************************************
#define PIN_LEDFAULT_PIN        GPIO_PIN_6

//*****************************************************************************
//
//! The GPIO port on which the quadrature encoder channel A pin resides.
//
//*****************************************************************************
#define PIN_ENCA_PORT           GPIO_PORTC_BASE

//*****************************************************************************
//
//! The GPIO pin on which the quadrature encoder channel A pin resides.
//
//*****************************************************************************
#define PIN_ENCA_PIN            GPIO_PIN_4

//*****************************************************************************
//
//! The GPIO port on which the CCP1 pin resides.
//
//*****************************************************************************
#define PIN_CCP1_PORT           GPIO_PORTC_BASE

//*****************************************************************************
//
//! The GPIO pin on which the CCP1 pin resides.
//
//*****************************************************************************
#define PIN_CCP1_PIN            GPIO_PIN_5

//*****************************************************************************
//
//! The GPIO port on which the quadrature encoder channel B pin resides.
//
//*****************************************************************************
#define PIN_ENCB_PORT           GPIO_PORTC_BASE

//*****************************************************************************
//
//! The GPIO pin on which the quadrature encoder channel B pin resides.
//
//*****************************************************************************
#define PIN_ENCB_PIN            GPIO_PIN_6

//*****************************************************************************
//
//! The GPIO port on which the brake pin resides.
//
//*****************************************************************************
#define PIN_BRAKE_PORT          GPIO_PORTC_BASE

//*****************************************************************************
//
//! The GPIO pin on which the brake pin resides.
//
//*****************************************************************************
#define PIN_BRAKE_PIN           GPIO_PIN_7

//*****************************************************************************
//
//! The GPIO port on which the phase U low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEU_LOW_PORT     GPIO_PORTD_BASE

//*****************************************************************************
//
//! The GPIO pin on which the phase U low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEU_LOW_PIN      GPIO_PIN_0

//*****************************************************************************
//
//! The GPIO port on which the phase U high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEU_HIGH_PORT    GPIO_PORTD_BASE

//*****************************************************************************
//
//! The GPIO pin on which the phase U high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEU_HIGH_PIN     GPIO_PIN_1

//*****************************************************************************
//
//! The GPIO port on which the status one LED resides.
//
//*****************************************************************************
#define PIN_LEDSTATUS1_PORT     GPIO_PORTD_BASE

//*****************************************************************************
//
//! The GPIO pin on which the status one LED resides.
//
//*****************************************************************************
#define PIN_LEDSTATUS1_PIN      GPIO_PIN_2

//*****************************************************************************
//
//! The GPIO port on which the status two LED resides.
//
//*****************************************************************************
#define PIN_LEDSTATUS2_PORT     GPIO_PORTD_BASE

//*****************************************************************************
//
//! The GPIO pin on which the status two LED resides.
//
//*****************************************************************************
#define PIN_LEDSTATUS2_PIN      GPIO_PIN_3

//*****************************************************************************
//
//! The GPIO port on which the CCP0 pin resides.
//
//*****************************************************************************
#define PIN_CCP0_PORT           GPIO_PORTD_BASE

//*****************************************************************************
//
//! The GPIO pin on which the CCP0 pin resides.
//
//*****************************************************************************
#define PIN_CCP0_PIN            GPIO_PIN_4

//*****************************************************************************
//
//! The GPIO port on which the potentiometer oscillator resides.
//
//*****************************************************************************
#define PIN_POTENTIOMETER_PORT  GPIO_PORTD_BASE

//*****************************************************************************
//
//! The GPIO pin on which the potentiometer oscillator resides.
//
//*****************************************************************************
#define PIN_POTENTIOMETER_PIN   GPIO_PIN_5

//*****************************************************************************
//
//! The GPIO port on which the phase W low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEW_LOW_PORT     GPIO_PORTE_BASE

//*****************************************************************************
//
//! The GPIO pin on which the phase W low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEW_LOW_PIN      GPIO_PIN_0

//*****************************************************************************
//
//! The GPIO port on which the phase W high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEW_HIGH_PORT    GPIO_PORTE_BASE

//*****************************************************************************
//
//! The GPIO pin on which the phase W high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEW_HIGH_PIN     GPIO_PIN_1

//*****************************************************************************
//
//! The ADC channel on which the phase U current sense resides.
//
//*****************************************************************************
#define PIN_I_PHASEU            ADC_CTL_CH0

//*****************************************************************************
//
//! The ADC channel on which the phase V current sense resides.
//
//*****************************************************************************
#define PIN_I_PHASEV            ADC_CTL_CH1

//*****************************************************************************
//
//! The ADC channel on which the phase W current sense resides.
//
//*****************************************************************************
#define PIN_I_PHASEW            ADC_CTL_CH2

//*****************************************************************************
//
//! The ADC channel on which the DC bus voltage sense resides.
//
//*****************************************************************************
#define PIN_VSENSE              ADC_CTL_CH3

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif // __PINS_H__
