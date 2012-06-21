//*****************************************************************************
//
// dac32sound.c - Audio Abstraction for Bluetooth A2DP Demo application.
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
// This is part of revision 8555 of the DK-LM3S9B96-EM2-CC2560-BLUETOPIA Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/i2s.h"
#include "driverlib/i2c.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"

#include "dac32sound.h"
#include "BTPSKRNL.h"

//*****************************************************************************
//
// The following defines the prototype of the Display function.  This
// is used to send debug messages out the Debug Port.
//
//*****************************************************************************
#define Display(_x)                  DBG_MSG(DBG_ZONE_DEVELOPMENT, _x)

//*****************************************************************************
//
// Flag values for I2C functions.
//
//*****************************************************************************
#define WRITE_TO_SLAVE               0
#define READ_FROM_SLAVE              1

//*****************************************************************************
//
// Flag values for interrupt status functions.
//
//*****************************************************************************
#define READ_INTERRUPT_RAW_STATUS    0
#define READ_INTERRUPT_MASKED_STATUS 1

//*****************************************************************************
//
// The following define the I2C port information used to communicate
// with the DAC.
//
//*****************************************************************************
#define DAC_I2C_PERIPH          (SYSCTL_PERIPH_I2C0)
#define DAC_I2C_MASTER_BASE     (I2C0_MASTER_BASE)
#define DAC_I2CSCL_GPIO_PERIPH  (SYSCTL_PERIPH_GPIOB)
#define DAC_I2CSCL_GPIO_PORT    (GPIO_PORTB_BASE)
#define DAC_I2CSCL_PIN          (GPIO_PIN_2)

#define DAC_I2CSDA_GPIO_PERIPH  (SYSCTL_PERIPH_GPIOB)
#define DAC_I2CSDA_GPIO_PORT    (GPIO_PORTB_BASE)
#define DAC_I2CSDA_PIN          (GPIO_PIN_3)

//*****************************************************************************
//
// Register addresses for the TLV320AIC23 DAC that is on the development board.
//
//*****************************************************************************
#define TI_LEFT_LINEIN_VC       0x00
#define TI_RIGHT_LINEIN_VC      0x02
#define TI_LEFT_HP_VC           0x04
#define TI_RIGHT_HP_VC          0x06
#define TI_ANALOG_AP            0x08
#define TI_DIGITAL_AP           0x0a
#define TI_POWER_DOWN           0x0c
#define TI_DIGITAL_AI           0x0e
#define TI_SRC                  0x10
#define TI_DIGITAL_ACTIVATE     0x12
#define TI_RESET                0x1e

//*****************************************************************************
//
// Register values for the TLV320AIC23 DAC.
//
//*****************************************************************************
#define TLV_LINEIN_VC_MAX       0x1f
#define TLV_LINEIN_VC_MIN       0x00
#define TLV_LINEIN_VC_0DB       0x17
#define TLV_LINEIN_VC_MUTE      0x80

//*****************************************************************************
//
// The current DAC register page.
//
//*****************************************************************************
static char g_cCurrentPage;

//*****************************************************************************
//
// The current DAC volume level.
//
//*****************************************************************************
static unsigned int g_uiVolume;

//*****************************************************************************
//
// The following function is used to write a single value to a specified
// register.  It returns zero on success, or I2C_ERROR_CODE if there is an
// error.
//
//*****************************************************************************
static short
I2C_Write(unsigned char ucRegNum, unsigned char ucRegVal)
{
    short sRetVal;

    //
    // Clear the Interrupt Flag.
    //
    ROM_I2CMasterIntClear(DAC_I2C_MASTER_BASE);

    //
    // To Read from the DAC We must first prepare a write that identifies the
    // DAC by its address and the address where we want to read from.
    //
    ROM_I2CMasterSlaveAddrSet(DAC_I2C_MASTER_BASE, I2C_DAC_ADDR, WRITE_TO_SLAVE);
    ROM_I2CMasterDataPut(DAC_I2C_MASTER_BASE, ucRegNum);

    //
    // Wait for the Bus to be Idle
    //
    while(ROM_I2CMasterBusBusy(DAC_I2C_MASTER_BASE))
    {
    }

    //
    // Perform a single send, writing the address as the Register Number Only.
    //
    ROM_I2CMasterControl(DAC_I2C_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //
    // Wait for the operation to Complete.
    //
    while(ROM_I2CMasterIntStatus(DAC_I2C_MASTER_BASE,
                             READ_INTERRUPT_RAW_STATUS) == 0)
    {
    }

    //
    // Check to see if the initial transfer was successful.
    //
    if(ROM_I2CMasterErr(DAC_I2C_MASTER_BASE) == I2C_MASTER_ERR_NONE)
    {
        //
        // Clear pending interrupt notifications.
        //
        ROM_I2CMasterIntClear(DAC_I2C_MASTER_BASE);

        //
        // Perform a single send, writing the Data for the Register.
        //
        ROM_I2CMasterDataPut(DAC_I2C_MASTER_BASE, ucRegVal);
        ROM_I2CMasterControl(DAC_I2C_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

        //
        // Wait for the operation to Complete.
        //
        while(ROM_I2CMasterIntStatus(DAC_I2C_MASTER_BASE,
                                     READ_INTERRUPT_RAW_STATUS) == 0)
        {
        }

        //
        // If the final operation was successful, then load the success
        // return code, otherwise load an error return code.
        //
        if(ROM_I2CMasterErr(DAC_I2C_MASTER_BASE) == I2C_MASTER_ERR_NONE)
        {
            sRetVal = 0;
        }
        else
        {
            sRetVal = (short)I2C_ERROR_CODE;
        }
    }

    //
    // The initial transfer had an error so load an error return code.
    //
    else
    {
        sRetVal = (short)I2C_ERROR_CODE;
    }

    //
    // Return with error code indicating success or failure.
    //
    return(sRetVal);
}

//*****************************************************************************
//
// The following function is higher level function used to write a value to a
// specified register.  The function takes as its parameters the Page on which
// the register is located, the register address and the value to write to the
// register.  The function returns Zero on success.
//
//*****************************************************************************
static short
DAC_Register_Write(unsigned char ucPage,
                   unsigned char ucRegister,
                   unsigned char ucValue)
{
    short sRetVal = 0;

    //
    // Check to see if we need to change the Page.
    //
    if(ucPage != g_cCurrentPage)
    {
        //
        // Send a Change Page command.
        //
        sRetVal = I2C_Write(0, ucPage);
        if(!sRetVal)
        {
            g_cCurrentPage = ucPage;
        }
    }

    //
    // If there has not been an error, write the value to the specified
    // register.
    //
    if(!sRetVal)
    {
        sRetVal = I2C_Write(ucRegister, ucValue);
    }

    //
    // Return error or success indication.
    //
    return(sRetVal);
}

//*****************************************************************************
//
// The following function is used to initialize the TLV320AIC23 DAC that is
// found on the development board.
//
//*****************************************************************************
static void
DAC320_Init(void)
{
    g_cCurrentPage = DEFAULT_CURRENT_PAGE;

    //
    // Setup the DAC on the development board.
    //
    DAC_Register_Write(0, TI_RESET,            0x00);
    DAC_Register_Write(0, TI_POWER_DOWN,       0x60);
    DAC_Register_Write(0, TI_SRC,              0x00);
    DAC_Register_Write(0, TI_DIGITAL_AP,       0x05);
    DAC_Register_Write(0, TI_ANALOG_AP,        0x12);
    DAC_Register_Write(0, TI_DIGITAL_AI,       0x02);
    DAC_Register_Write(0, TI_LEFT_HP_VC,       (unsigned char)(0x80 | 0x64));
    DAC_Register_Write(0, TI_RIGHT_HP_VC,      (unsigned char)(0x80 | 0x64));
    DAC_Register_Write(0, TI_LEFT_LINEIN_VC,   TLV_LINEIN_VC_0DB);
    DAC_Register_Write(0, TI_RIGHT_LINEIN_VC,  TLV_LINEIN_VC_0DB);
    DAC_Register_Write(0, TI_DIGITAL_ACTIVATE, 0x01);
}

//*****************************************************************************
//
// The following function is responsible for initializing the DAC and setting
// up the registers with default values.  The Volume is set to the default
// level.
//
//*****************************************************************************
void
DAC32SoundInit(void)
{
    unsigned long ulFormat;

    //
    // Enable and configure the GPIO port and pins used for I2C interface
    // to the DAC.
    //
    ROM_SysCtlPeripheralEnable(DAC_I2CSCL_GPIO_PERIPH);
    ROM_GPIOPinTypeGPIOInput(DAC_I2CSCL_GPIO_PORT, DAC_I2CSDA_PIN);
    ROM_GPIOPadConfigSet(DAC_I2CSCL_GPIO_PORT,
                         DAC_I2CSDA_PIN,
                         GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPD);
    ROM_GPIOPinTypeI2C(DAC_I2CSCL_GPIO_PORT, DAC_I2CSCL_PIN | DAC_I2CSDA_PIN);

    //
    // Enable and configure the I2C peripheral
    //
    ROM_SysCtlPeripheralEnable(DAC_I2C_PERIPH);
    ROM_I2CMasterInitExpClk(DAC_I2C_MASTER_BASE, ROM_SysCtlClockGet(), 0);

    //
    // Enable the I2S Port used to send the audio data to the DAC.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2S0);
    GPIOPinTypeI2S(GPIO_PORTB_BASE, GPIO_PIN_6);
    GPIOPinTypeI2S(GPIO_PORTD_BASE, GPIO_PIN_4);
    GPIOPinTypeI2S(GPIO_PORTE_BASE, (GPIO_PIN_4 | GPIO_PIN_5));
    GPIOPinTypeI2S(GPIO_PORTF_BASE, GPIO_PIN_1);

    //
    // Configure the I2S FIFO and clock
    //
    I2STxFIFOLimitSet(I2S0_BASE, 4);
    I2SMasterClockSelect(I2S0_BASE, 0);

    //
    // Configure the I2S format
    //
    ulFormat  = I2S_CONFIG_FORMAT_I2S |
                I2S_CONFIG_CLK_MASTER;
    ulFormat |= (I2S_CONFIG_WIRE_SIZE_16    |
                 I2S_CONFIG_MODE_COMPACT_16 |
                 I2S_CONFIG_SAMPLE_SIZE_16);
    I2STxConfigSet(I2S0_BASE, ulFormat);
    ulFormat = ((ulFormat & ~I2S_CONFIG_FORMAT_MASK) |
                I2S_CONFIG_FORMAT_LEFT_JUST);
    I2SRxConfigSet(I2S0_BASE, ulFormat);

    //
    // Indicate that we will source the MCLK.
    //
    I2SMasterClockSelect(I2S0_BASE, I2S_TX_MCLK_INT | I2S_RX_MCLK_INT);

    //
    // Enable I2S interrupts.
    //
    I2STxEnable(I2S0_BASE);
    ROM_IntEnable(INT_I2S0);

    //
    // Setup the DAC for operation.
    //
    Display(("DAC Init\r\n"));
    DAC320_Init();

    //
    // Set the volume to the default level.
    //
    SoundVolumeSet(DEFAULT_POWERUP_VOLUME);
}

//*****************************************************************************
//
// The following Configures the DAC go generate a frame sync clock at the
// specified sample reate.
//
//*****************************************************************************
void
SoundSetFormat(unsigned long ulSampleRate)
{
    //
    // Set the MCLK rate.  The multiply by 8 is due to a 4X oversample rate
    // plus a factor of two since the data is always stereo on the I2S
    // interface.
    //
    SysCtlI2SMClkSet(0, ulSampleRate * 16 * 8);
}

//*****************************************************************************
//
// The following function is used to sent the Headset Audio Volume level.  The
// Volume can be set from 0 dB (100%) to -78 dB (8%).  Anything below 8% will
// Mute the audio.
//
//*****************************************************************************
void
SoundVolumeSet(unsigned int uiPercent)
{
    //
    // Cap the Volume at 100%.
    //
    if(uiPercent > 100)
    {
        uiPercent = 100;
    }

    //
    // Calculate the value to be loaded to produce the requested volume level.
    //
    uiPercent = (unsigned int)((48+((uiPercent * 73)/100)) | 0x80);

    //
    // Command the DAC volume
    //
    DAC_Register_Write(0, TI_LEFT_HP_VC, (Byte_t)uiPercent);
    DAC_Register_Write(0, TI_RIGHT_HP_VC, (Byte_t)uiPercent);

    //
    // Save the current Volume Level.
    //
    g_uiVolume = uiPercent;
}

//*****************************************************************************
//
// The following function is used to retrieve the current Headset Audio Volume
// level.
//
//*****************************************************************************
unsigned int
SoundVolumeGet(void)
{
    return(g_uiVolume);
}

