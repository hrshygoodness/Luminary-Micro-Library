//*****************************************************************************
//
// pins.h - Maps logical pin names to physical device pins.
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

#ifndef __PINS_H__
#define __PINS_H__

//*****************************************************************************
//
// \page pins_intro Introduction
//
// The pins on the microcontroller are connected to a variety of external
// devices.  The defines in this file provide a name more descriptive than
// "PB0" for the various devices connected to the microcontroller pins.
//
// These defines are provided in <tt>pins.h</tt>.
//
//*****************************************************************************

//*****************************************************************************
//
// \defgroup pins_api Definitions
// @{
//
//*****************************************************************************

//*****************************************************************************
//
// The GPIO port on which the UART0 Rx pin resides.
//
//*****************************************************************************
#define PIN_UART0RX_PORT        GPIO_PORTA_BASE

//*****************************************************************************
//
// The GPIO pin on which the UART0 Rx pin resides.
//
//*****************************************************************************
#define PIN_UART0RX_PIN         GPIO_PIN_0

//*****************************************************************************
//
// The GPIO port on which the UART0 Tx pin resides.
//
//*****************************************************************************
#define PIN_UART0TX_PORT        GPIO_PORTA_BASE

//*****************************************************************************
//
// The GPIO pin on which the UART0 Tx pin resides.
//
//*****************************************************************************
#define PIN_UART0TX_PIN         GPIO_PIN_1

//*****************************************************************************
//
// The GPIO port on which the phase A low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEA_LOW_PORT     GPIO_PORTF_BASE

//*****************************************************************************
//
// The GPIO pin on which the phase A low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEA_LOW_PIN      GPIO_PIN_1

//*****************************************************************************
//
// The PWM channel on which the phase A low side resides.
//
//*****************************************************************************
#define PWM_PHASEA_LOW          (1 << 1)

//*****************************************************************************
//
// The GPIO port on which the phase A high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEA_HIGH_PORT    GPIO_PORTF_BASE

//*****************************************************************************
//
// The GPIO pin on which the phase A high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEA_HIGH_PIN     GPIO_PIN_0

//*****************************************************************************
//
// The PWM channel on which the phase A high side resides.
//
//*****************************************************************************
#define PWM_PHASEA_HIGH         (1 << 0)

//*****************************************************************************
//
// The GPIO port on which the quadrature encoder index pin resides.
//
//*****************************************************************************
#define PIN_INDEX_PORT          GPIO_PORTB_BASE

//*****************************************************************************
//
// The GPIO pin on which the quadrature encoder index pin resides.
//
//*****************************************************************************
#define PIN_INDEX_PIN           GPIO_PIN_2

//*****************************************************************************
//
// The GPIO port on which the user push button resides.
//
//*****************************************************************************
#define PIN_SWITCH_PORT         GPIO_PORTB_BASE

//*****************************************************************************
//
// The GPIO pin on which the user push button resides.
//
//*****************************************************************************
#define PIN_SWITCH_PIN          GPIO_PIN_3

//*****************************************************************************
//
// The bit lane of the GPIO pin on which the user push button resides.
//
//*****************************************************************************
#define PIN_SWITCH_PIN_BIT      3

//*****************************************************************************
//
// The GPIO port on which the run LED resides.
//
//*****************************************************************************
#define PIN_LEDRUN_PORT         GPIO_PORTG_BASE

//*****************************************************************************
//
// The GPIO pin on which the run LED resides.
//
//*****************************************************************************
#define PIN_LEDRUN_PIN          GPIO_PIN_0

//*****************************************************************************
//
// The GPIO port on which the fault LED resides.
//
//*****************************************************************************
#define PIN_LEDFAULT_PORT       GPIO_PORTC_BASE

//*****************************************************************************
//
// The GPIO pin on which the fault LED resides.
//
//*****************************************************************************
#define PIN_LEDFAULT_PIN        GPIO_PIN_5

//*****************************************************************************
//
// The GPIO port on which the quadrature encoder channel A pin resides.
//
//*****************************************************************************
#define PIN_ENCA_PORT           GPIO_PORTC_BASE

//*****************************************************************************
//
// The GPIO pin on which the quadrature encoder channel A pin resides.
//
//*****************************************************************************
#define PIN_ENCA_PIN            GPIO_PIN_4

//*****************************************************************************
//
// The GPIO port on which the quadrature encoder channel B pin resides.
//
//*****************************************************************************
#define PIN_ENCB_PORT           GPIO_PORTC_BASE

//*****************************************************************************
//
// The GPIO pin on which the quadrature encoder channel B pin resides.
//
//*****************************************************************************
#define PIN_ENCB_PIN            GPIO_PIN_7

//*****************************************************************************
//
// The GPIO port on which the brake pin resides.
//
//*****************************************************************************
#define PIN_BRAKE_PORT          GPIO_PORTC_BASE

//*****************************************************************************
//
// The GPIO pin on which the brake pin resides.
//
//*****************************************************************************
#define PIN_BRAKE_PIN           GPIO_PIN_6

//*****************************************************************************
//
// The GPIO port on which the phase B low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEB_LOW_PORT     GPIO_PORTD_BASE

//*****************************************************************************
//
// The GPIO pin on which the phase B low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEB_LOW_PIN      GPIO_PIN_3

//*****************************************************************************
//
// The PWM channel on which the phase B low side resides.
//
//*****************************************************************************
#define PWM_PHASEB_LOW          (1 << 3)

//*****************************************************************************
//
// The GPIO port on which the phase B high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEB_HIGH_PORT    GPIO_PORTD_BASE

//*****************************************************************************
//
// The GPIO pin on which the phase B high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEB_HIGH_PIN     GPIO_PIN_2

//*****************************************************************************
//
// The PWM channel on which the phase B high side resides.
//
//*****************************************************************************
#define PWM_PHASEB_HIGH         (1 << 2)

//*****************************************************************************
//
// The GPIO port on which the Ethernet status zero LED resides.
//
//*****************************************************************************
#define PIN_LEDSTATUS0_PORT     GPIO_PORTF_BASE

//*****************************************************************************
//
// The GPIO pin on which the Ethernet status zero LED resides.
//
//*****************************************************************************
#define PIN_LEDSTATUS0_PIN      GPIO_PIN_3

//*****************************************************************************
//
// The GPIO port on which the Ethernet status one LED resides.
//
//*****************************************************************************
#define PIN_LEDSTATUS1_PORT     GPIO_PORTF_BASE

//*****************************************************************************
//
// The GPIO pin on which the Ethernet status one LED resides.
//
//*****************************************************************************
#define PIN_LEDSTATUS1_PIN      GPIO_PIN_2

//*****************************************************************************
//
// The GPIO port on which the phase C low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEC_LOW_PORT     GPIO_PORTE_BASE

//*****************************************************************************
//
// The GPIO pin on which the phase C low side pin resides.
//
//*****************************************************************************
#define PIN_PHASEC_LOW_PIN      GPIO_PIN_1

//*****************************************************************************
//
// The PWM channel on which the phase C low side resides.
//
//*****************************************************************************
#define PWM_PHASEC_LOW          (1 << 5)

//*****************************************************************************
//
// The GPIO port on which the phase C high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEC_HIGH_PORT    GPIO_PORTE_BASE

//*****************************************************************************
//
// The GPIO pin on which the phase C high side pin resides.
//
//*****************************************************************************
#define PIN_PHASEC_HIGH_PIN     GPIO_PIN_0

//*****************************************************************************
//
// The PWM channel on which the phase C high side resides.
//
//*****************************************************************************
#define PWM_PHASEC_HIGH         (1 << 4)

//*****************************************************************************
//
// The GPIO port on which the CAN0 Rx pin resides.
//
//*****************************************************************************
#define PIN_CAN0RX_PORT         GPIO_PORTD_BASE

//*****************************************************************************
//
// The GPIO pin on which the CAN0 Rx pin resides.
//
//*****************************************************************************
#define PIN_CAN0RX_PIN          GPIO_PIN_0

//*****************************************************************************
//
// The GPIO port on which the CAN0 Tx pin resides.
//
//*****************************************************************************
#define PIN_CAN0TX_PORT         GPIO_PORTD_BASE

//*****************************************************************************
//
// The GPIO pin on which the CAN0 Tx pin resides.
//
//*****************************************************************************
#define PIN_CAN0TX_PIN          GPIO_PIN_1

//*****************************************************************************
//
// The GPIO port on which the CFG0 pin resides.
//
//*****************************************************************************
#define PIN_CFG0_PORT           GPIO_PORTA_BASE

//*****************************************************************************
//
// The GPIO pin on which the CFG0 pin resides.
//
//*****************************************************************************
#define PIN_CFG0_PIN            GPIO_PIN_2

//*****************************************************************************
//
// The GPIO port on which the CFG1 pin resides.
//
//*****************************************************************************
#define PIN_CFG1_PORT           GPIO_PORTA_BASE

//*****************************************************************************
//
// The GPIO pin on which the CFG1 pin resides.
//
//*****************************************************************************
#define PIN_CFG1_PIN            GPIO_PIN_3

//*****************************************************************************
//
// The GPIO port on which the CFG2 pin resides.
//
//*****************************************************************************
#define PIN_CFG2_PORT           GPIO_PORTA_BASE

//*****************************************************************************
//
// The GPIO pin on which the CFG2 pin resides.
//
//*****************************************************************************
#define PIN_CFG2_PIN            GPIO_PIN_4

//*****************************************************************************
//
// The GPIO port on which the HALL A pin resides.
//
//*****************************************************************************
#define PIN_HALLA_PORT          GPIO_PORTB_BASE

//*****************************************************************************
//
// The GPIO pin on which the HALL A pin resides.
//
//*****************************************************************************
#define PIN_HALLA_PIN           GPIO_PIN_4

//*****************************************************************************
//
// The GPIO port on which the HALL B pin resides.
//
//*****************************************************************************
#define PIN_HALLB_PORT          GPIO_PORTB_BASE

//*****************************************************************************
//
// The GPIO pin on which the HALL B pin resides.
//
//*****************************************************************************
#define PIN_HALLB_PIN           GPIO_PIN_5

//*****************************************************************************
//
// The GPIO port on which the HALL C pin resides.
//
//*****************************************************************************
#define PIN_HALLC_PORT          GPIO_PORTB_BASE

//*****************************************************************************
//
// The GPIO pin on which the HALL C pin resides.
//
//*****************************************************************************
#define PIN_HALLC_PIN           GPIO_PIN_6

//*****************************************************************************
//
// The ADC channel on which the DC bus voltage sense resides.
//
//*****************************************************************************
#define PIN_VSENSE              ADC_CTL_CH0

//*****************************************************************************
//
// The ADC channel on which the DC Back EMF (Phase A) voltage sense resides.
//
//*****************************************************************************
#define PIN_VBEMFA              ADC_CTL_CH1

//*****************************************************************************
//
// The ADC channel on which the DC Back EMF (Phase B) voltage sense resides.
//
//*****************************************************************************
#define PIN_VBEMFB              ADC_CTL_CH2

//*****************************************************************************
//
// The ADC channel on which the DC Back EMF (Phase C) voltage sense resides.
//
//*****************************************************************************
#define PIN_VBEMFC              ADC_CTL_CH3

//*****************************************************************************
//
// The ADC channel on which the phase A current sense resides.
//
//*****************************************************************************
#define PIN_IPHASEA             ADC_CTL_CH4

//*****************************************************************************
//
// The ADC channel on which the phase B current sense resides.
//
//*****************************************************************************
#define PIN_IPHASEB             ADC_CTL_CH5

//*****************************************************************************
//
// The ADC channel on which the phase C current sense resides.
//
//*****************************************************************************
#define PIN_IPHASEC             ADC_CTL_CH6

//*****************************************************************************
//
// The ADC channel on which the Analog Input voltage sense resides.
//
//*****************************************************************************
#define PIN_VANALOG             ADC_CTL_CH7

//*****************************************************************************
//
// Close the Doxygen group.
// @}
//
//*****************************************************************************

#endif // __PINS_H__
