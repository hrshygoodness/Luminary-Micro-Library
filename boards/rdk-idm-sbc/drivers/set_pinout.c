//*****************************************************************************
//
// set_pinout.c - Functions related to configuration of the device pinout.
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "set_pinout.h"

//*****************************************************************************
//
//! \addtogroup set_pinout_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Configures the LM3S9B92 device pinout for the RDK-IDM-SBC board.
//!
//! This function configures each pin of the lm3s9b92 device to route the
//! appropriate peripheral signal as required by the design of the rdk-idm-sbc
//! development board.
//!
//! \return None.
//
//*****************************************************************************
void
PinoutSet(void)
{
    //
    // Enable all GPIO banks.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    //
    // GPIO Port A pins
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);
    GPIOPinConfigure(GPIO_PA6_USB0EPEN);
    GPIOPinConfigure(GPIO_PA7_USB0PFLT);

    //
    // GPIO Port B pins
    //
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    GPIOPinConfigure(GPIO_PB4_CAN0RX);
    GPIOPinConfigure(GPIO_PB5_CAN0TX);
    GPIOPinConfigure(GPIO_PB6_I2S0TXSCK);

    //
    // GPIO Port C pins
    //
    GPIOPinConfigure(GPIO_PC4_EPI0S2);
    GPIOPinConfigure(GPIO_PC5_EPI0S3);
    GPIOPinConfigure(GPIO_PC6_EPI0S4);
    GPIOPinConfigure(GPIO_PC7_EPI0S5);

    //
    // GPIO Port E pins
    //
    GPIOPinConfigure(GPIO_PE0_EPI0S8);
    GPIOPinConfigure(GPIO_PE1_EPI0S9);
    GPIOPinConfigure(GPIO_PE4_I2S0TXWS);
    GPIOPinConfigure(GPIO_PE5_I2S0TXSD);

    //
    // GPIO Port F pins
    //
    GPIOPinConfigure(GPIO_PF1_I2S0TXMCLK);
    GPIOPinConfigure(GPIO_PF2_LED1);
    GPIOPinConfigure(GPIO_PF3_LED0);
    GPIOPinConfigure(GPIO_PF4_EPI0S12);
    GPIOPinConfigure(GPIO_PF5_EPI0S15);

    //
    // GPIO Port G pins
    //
    GPIOPinConfigure(GPIO_PG0_EPI0S13);
    GPIOPinConfigure(GPIO_PG1_EPI0S14);
    GPIOPinConfigure(GPIO_PG7_EPI0S31);

    //
    // GPIO Port H pins
    //
    GPIOPinConfigure(GPIO_PH0_EPI0S6);
    GPIOPinConfigure(GPIO_PH1_EPI0S7);
    GPIOPinConfigure(GPIO_PH2_EPI0S1);
    GPIOPinConfigure(GPIO_PH3_EPI0S0);
    GPIOPinConfigure(GPIO_PH4_EPI0S10);
    GPIOPinConfigure(GPIO_PH5_EPI0S11);

    //
    // GPIO Port J pins
    //
    GPIOPinConfigure(GPIO_PJ0_EPI0S16);
    GPIOPinConfigure(GPIO_PJ1_EPI0S17);
    GPIOPinConfigure(GPIO_PJ2_EPI0S18);
    GPIOPinConfigure(GPIO_PJ3_EPI0S19);
    GPIOPinConfigure(GPIO_PJ4_EPI0S28);
    GPIOPinConfigure(GPIO_PJ5_EPI0S29);
    GPIOPinConfigure(GPIO_PJ6_EPI0S30);
    GPIOPinConfigure(GPIO_PJ7_CCP0);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
