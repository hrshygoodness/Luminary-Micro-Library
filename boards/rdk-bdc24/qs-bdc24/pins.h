//*****************************************************************************
//
// pins.h - Defines the board-level pin connections.
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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#ifndef __PINS_H__
#define __PINS_H__

//*****************************************************************************
//
// Defines for the board connections to the Stellaris microcontroller.
//
//*****************************************************************************
#define UART_RX_PORT            GPIO_PORTA_BASE
#define UART_RX_PIN             GPIO_PIN_0

#define UART_TX_PORT            GPIO_PORTA_BASE
#define UART_TX_PIN             GPIO_PIN_1

#define LED_GREEN_PORT          GPIO_PORTA_BASE
#define LED_GREEN_PIN           GPIO_PIN_2
#define LED_GREEN_ON            LED_GREEN_PIN
#define LED_GREEN_OFF           0

#define SERVO_PORT              GPIO_PORTA_BASE
#define SERVO_PIN               GPIO_PIN_3
#define SERVO_INT               INT_GPIOA

#define CAN_RX_PORT             GPIO_PORTA_BASE
#define CAN_RX_PIN              GPIO_PIN_4

#define CAN_TX_PORT             GPIO_PORTA_BASE
#define CAN_TX_PIN              GPIO_PIN_5

#define HBRIDGE_AHI_PORT        GPIO_PORTA_BASE
#define HBRIDGE_AHI_PIN         GPIO_PIN_6

#define HBRIDGE_ALO_PORT        GPIO_PORTA_BASE
#define HBRIDGE_ALO_PIN         GPIO_PIN_7

#define HBRIDGE_BHI_PORT        GPIO_PORTB_BASE
#define HBRIDGE_BHI_PIN         GPIO_PIN_0

#define HBRIDGE_BLO_PORT        GPIO_PORTB_BASE
#define HBRIDGE_BLO_PIN         GPIO_PIN_1

#define I2C_SCL_PORT            GPIO_PORTB_BASE
#define I2C_SCL_PIN             GPIO_PIN_2

#define I2C_SDA_PORT            GPIO_PORTB_BASE
#define I2C_SDA_PIN             GPIO_PIN_3

#define GATE_RESET_PORT         GPIO_PORTB_BASE
#define GATE_RESET_PIN          GPIO_PIN_4

#define FAN_PORT                GPIO_PORTB_BASE
#define FAN_PIN                 GPIO_PIN_5
#define FAN_ON                  FAN_PIN
#define FAN_OFF                 0

#define LIMIT_REV_PORT          GPIO_PORTB_BASE
#define LIMIT_REV_PIN           GPIO_PIN_6
#define LIMIT_REV_OK            0
#define LIMIT_REV_ERR           LIMIT_REV_PIN

#define LIMIT_FWD_PORT          GPIO_PORTB_BASE
#define LIMIT_FWD_PIN           GPIO_PIN_7
#define LIMIT_FWD_OK            0
#define LIMIT_FWD_ERR           LIMIT_FWD_PIN

#define QEI_PHA_PORT            GPIO_PORTC_BASE
#define QEI_PHA_PIN             GPIO_PIN_4
#define QEI_PHA_INT             INT_GPIOC

#define GATE_FAULT_PORT         GPIO_PORTC_BASE
#define GATE_FAULT_PIN          GPIO_PIN_5

#define QEI_PHB_PORT            GPIO_PORTC_BASE
#define QEI_PHB_PIN             GPIO_PIN_6

#define LED_RED_PORT            GPIO_PORTC_BASE
#define LED_RED_PIN             GPIO_PIN_7
#define LED_RED_ON              LED_RED_PIN
#define LED_RED_OFF             0

#define QEI_INDEX_PORT          GPIO_PORTD_BASE
#define QEI_INDEX_PIN           GPIO_PIN_0

#define BRAKECOAST_PORT         GPIO_PORTD_BASE
#define BRAKECOAST_PIN          GPIO_PIN_2
#define BRAKECOAST_COAST        BRAKECOAST_PIN
#define BRAKECOAST_BRAKE        0

#define ADC_CURRENT_PORT        GPIO_PORTE_BASE
#define ADC_CURRENT_PIN         GPIO_PIN_0
#define ADC_CURRENT_CH          ADC_CTL_CH3

#define ADC_POSITION_PORT       GPIO_PORTE_BASE
#define ADC_POSITION_PIN        GPIO_PIN_2
#define ADC_POSITION_CH         ADC_CTL_CH1

#define ADC_VBUS_PORT           GPIO_PORTE_BASE
#define ADC_VBUS_PIN            GPIO_PIN_3
#define ADC_VBUS_CH             ADC_CTL_CH0

#define BUTTON_PORT             GPIO_PORTE_BASE
#define BUTTON_PIN              GPIO_PIN_4
#define BUTTON_DOWN             0
#define BUTTON_UP               BUTTON_PIN

#endif // __PINS_H__
