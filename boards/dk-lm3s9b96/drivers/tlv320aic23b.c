//*****************************************************************************
//
// tlv320aic23b.c - Driver for the TI TLV320AIC23B DAC
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "tlv320aic23b.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"

//*****************************************************************************
//
// The I2C pins that are used by this application.
//
//*****************************************************************************
#define DAC_I2C_PERIPH              (SYSCTL_PERIPH_I2C0)
#define DAC_I2C_MASTER_BASE         (I2C0_MASTER_BASE)
#define DAC_I2CSCL_GPIO_PERIPH      (SYSCTL_PERIPH_GPIOB)
#define DAC_I2CSCL_GPIO_PORT        (GPIO_PORTB_BASE)
#define DAC_I2CSCL_PIN              (GPIO_PIN_2)

#define DAC_I2CSDA_GPIO_PERIPH      (SYSCTL_PERIPH_GPIOB)
#define DAC_I2CSDA_GPIO_PORT        (GPIO_PORTB_BASE)
#define DAC_I2CSDA_PIN              (GPIO_PIN_3)

//*****************************************************************************
//
//  Register offsets.
//
//*****************************************************************************
#define TI_LEFT_LINEIN_VC       0x00        // Left line input channel volume
                                            // control.
#define TI_RIGHT_LINEIN_VC      0x02        // Right line input channel volume
                                            // control.
#define TI_LEFT_HP_VC           0x04        // Left channel headphone volume
                                            // control.
#define TI_RIGHT_HP_VC          0x06        // Right channel headphone volume
                                            // control.
#define TI_ANALOG_AP            0x08        // Analog audio path control.
#define TI_DIGITAL_AP           0x0a        // Digital audio path control.
#define TI_POWER_DOWN           0x0c        // Power down control.
#define TI_DIGITAL_AI           0x0e        // Digital audio interface format.
#define TI_SRC                  0x10        // Sample rate control
#define TI_DIGITAL_ACTIVATE     0x12        // Digital interface activation.
#define TI_RESET                0x1e        // Reset register


//*****************************************************************************
//
//  TI_LEFT_LINEIN_VC
//
//*****************************************************************************
#define TI_LEFT_LINEIN_VC_LRS   0x100       // Simultaneous update.
#define TI_LEFT_LINEIN_VC_LIM   0x080       // Left line input mute
#define TI_LEFT_LINEIN_VC_LIV_M 0x01f       // Left line input volume control.

//*****************************************************************************
//
//  TI_RIGHT_LINEIN_VC
//
//*****************************************************************************
#define TI_RIGHT_LINEIN_VC_RLS  0x100       // Simultaneous update.
#define TI_RIGHT_LINEIN_VC_RIM  0x080       // Right line input mute
#define TI_RIGHT_LINEIN_VC_RIV  0x01f       // Right line input volume control.

//*****************************************************************************
//
//  TI_LEFT_HP_VC
//
//*****************************************************************************
#define TI_LEFT_HP_VC_LRS       0x100           // Simultaneous update.
#define TI_LEFT_HP_VC_LZC       0x080           // Left headphone zero cross.
#define TI_LEFT_HP_VC_LHV       0x07f           // Left headphone volume.
#define TI_LEFT_HP_VC_0DB       0x079           // Left headphone volume 0db.

//*****************************************************************************
//
//  TI_RIGHT_HP_VC
//
//*****************************************************************************
#define TI_RIGHT_HP_VC_RLS      0x100           // Simultaneous update.
#define TI_RIGHT_HP_VC_RZC      0x080           // Right headphone mute
#define TI_RIGHT_HP_VC_RHV      0x07f           // Right headphone volume.
#define TI_RIGHT_HP_VC_0DB      0x079           // Right headphone volume 0db.

//*****************************************************************************
//
//  TI_ANALOG_AP
//
//*****************************************************************************
#define TI_ANALOG_AP_STA        0x1c0           // Side Tone.
#define TI_ANALOG_AP_STE        0x020           // Side Tone enable.
#define TI_ANALOG_AP_DAC        0x010           // DAC select.
#define TI_ANALOG_AP_BYP        0x008           // Bypass.
#define TI_ANALOG_AP_INSEL      0x004           // Input select for ADC.
#define TI_ANALOG_AP_MICM       0x002           // Microphone mute.
#define TI_ANALOG_AP_MICB       0x001           // Microphone boost.

//*****************************************************************************
//
//  TI_DIGITAL_AP - Digital Audio Path Control
//
//*****************************************************************************
#define TI_DIGITAL_AP_DACM      0x008           // DAC soft mute
#define TI_DIGITAL_AP_DEEMP_DIS 0x000           // De-emphasis Disabled
#define TI_DIGITAL_AP_DEEMP_32K 0x002           // De-emphasis 32 kHz
#define TI_DIGITAL_AP_DEEMP_44K 0x004           // De-emphasis 44.1 kHz
#define TI_DIGITAL_AP_DEEMP_48K 0x005           // De-emphasis 48 kHz
#define TI_DIGITAL_AP_ADCHP     0x001           // ADC high-pass filter.

//*****************************************************************************
//
//  TI_POWER_DOWN - Power Down Control
//
//*****************************************************************************
#define TI_POWER_DOWN_OFF       0x080           // Device off
#define TI_POWER_DOWN_CLK       0x040           // Clock
#define TI_POWER_DOWN_OSC       0x020           // Oscillator
#define TI_POWER_DOWN_OUT       0x010           // Outputs
#define TI_POWER_DOWN_DAC       0x008           // DAC
#define TI_POWER_DOWN_ADC       0x004           // ADC
#define TI_POWER_DOWN_MIC       0x002           // Microphone input
#define TI_POWER_DOWN_LINE      0x001           // Line input

//*****************************************************************************
//
//  TI_DIGITAL_AI - Digital Audio Interface Format
//
//*****************************************************************************
#define TI_DIGITAL_AI_SLAVE     0x000           // Master mode
#define TI_DIGITAL_AI_MASTER    0x040           // Slave mode
#define TI_DIGITAL_AI_LRSWAP    0x020           // DAC left/right swap
#define TI_DIGITAL_AI_LRP       0x010           // DAC left/right phase
#define TI_DIGITAL_AI_IWL_16    0x000           // 16 bit data.
#define TI_DIGITAL_AI_IWL_20    0x004           // 20 bit data.
#define TI_DIGITAL_AI_IWL_24    0x008           // 24 bit data.
#define TI_DIGITAL_AI_IWL_32    0x00c           // 32 bit data.
#define TI_DIGITAL_AI_FOR_RA    0x000           // MSB first, right aligned
#define TI_DIGITAL_AI_FOR_LA    0x001           // MSB first, left aligned
#define TI_DIGITAL_AI_FOR_I2S   0x002           // I2S format, MSB first,
                                                // left aligned
#define TI_DIGITAL_AI_FOR_DSP   0x003           // DSP format

//*****************************************************************************
//
// TI_SRC - Sample Rate Control
//
//*****************************************************************************
#define TI_SRC_CLKOUT_DIV2      0x080           // Clock output divider
#define TI_SRC_CLKIN_DIV2       0x040           // Clock input divider
#define TI_SRC_SR               0x03c           // Sampling rate control
#define TI_SRC_SR_48000         0x000           // 12.288MHz MCLK 48KHz Sample
                                                // Rate.
#define TI_SRC_BOSR             0x002           // Base oversampling rate
#define TI_SRC_USB              0x001           // Clock mode select.
#define TI_SRC_NORMAL           0x000           // Clock mode select.

//*****************************************************************************
//
// TI_DIGITAL_ACTIVATE - Digital Interface Activation
//
//*****************************************************************************
#define TI_DIGITAL_ACTIVATE_EN  0x001           // Activate interface

//*****************************************************************************
//
// I2C Addresses for the TI DAC.
//
//*****************************************************************************
#define TI_TLV320AIC23B_ADDR_0   0x01a
#define TI_TLV320AIC23B_ADDR_1   0x01b


//*****************************************************************************
//
// Global Volumes are needed because the device is not readable.
//
//*****************************************************************************
static unsigned char g_ucHPVolume = 100;
static unsigned char g_ucEnabled = 0;

//*****************************************************************************
//
// This is the volume control settings table to use to scale the dB settings
// to a 0-100% scale.  There are 13 entries because 100/8 scaling is 12.5 steps
// which requires 13 entries.
//
//*****************************************************************************
static const unsigned char pucVolumeTable[13] =
{
     0x00,
     0x30,
     0x38,
     0x40,
     0x48,
     0x50,
     0x58,
     0x60,
     0x64,
     0x68,
     0x70,
     0x74,
     0x79, // TI_LEFT_HP_VC_0DB,
};

//*****************************************************************************
//
// Write a register in the TLV320AIC23B DAC.
//
// \param ucRegister is the offset to the register to write.
// \param ulData is the data to be written to the DAC register.
//
// This function will write the register passed in /e ucAddr with the value
// passed in to /e ulData.  The data in \e ulData is actually 9 bits and the
// value in /e ucAddr is interpreted as 7 bits.
//
// \return Returns \b true on success or \b false on error.
//
//*****************************************************************************
static tBoolean
TLV320AIC23BWriteRegister(unsigned char ucRegister, unsigned long ulData)
{
    //
    // Set the slave address.
    //
    I2CMasterSlaveAddrSet(DAC_I2C_MASTER_BASE, TI_TLV320AIC23B_ADDR_0, false);

    //
    // Write the next byte to the controller.
    //
    I2CMasterDataPut(DAC_I2C_MASTER_BASE, ucRegister | ((ulData >> 8) & 1));

    //
    // Continue the transfer.
    //
    I2CMasterControl(DAC_I2C_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //
    // Wait until the current byte has been transferred.
    //
    while(I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false) == 0)
    {
    }

    if(I2CMasterErr(DAC_I2C_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    {
        I2CMasterIntClear(DAC_I2C_MASTER_BASE);
        return(false);
    }

    //
    // Wait until the current byte has been transferred.
    //
    while(I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false))
    {
        I2CMasterIntClear(DAC_I2C_MASTER_BASE);
    }

    //
    // Write the next byte to the controller.
    //
    I2CMasterDataPut(DAC_I2C_MASTER_BASE, ulData);

    //
    // End the transfer.
    //
    I2CMasterControl(DAC_I2C_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

    //
    // Wait until the current byte has been transferred.
    //
    while(I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false) == 0)
    {
    }

    if(I2CMasterErr(DAC_I2C_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    {
        return(false);
    }

    while(I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false))
    {
        I2CMasterIntClear(DAC_I2C_MASTER_BASE);
    }

    return(true);
}

//*****************************************************************************
//
// Initialize the TLV320AIC23B DAC.
//
// This function initializes the I2C interface and the TLV320AIC23B DAC.
//
// \return Returns \b true on success of \b false if the I2S daughter board
// is not present.
//
//*****************************************************************************
tBoolean
TLV320AIC23BInit(void)
{
    tBoolean bRetcode;

    //
    // Enable the GPIO port containing the I2C pins and set the SDA pin as a
    // GPIO input for now and engage a weak pull-down.  If the daughter board
    // is present, the pull-up on the board should easily overwhelm
    // the pull-down and we should read the line state as high.
    //
    SysCtlPeripheralEnable(DAC_I2CSCL_GPIO_PERIPH);
    GPIOPinTypeGPIOInput(DAC_I2CSCL_GPIO_PORT, DAC_I2CSDA_PIN);
    GPIOPadConfigSet(DAC_I2CSCL_GPIO_PORT, DAC_I2CSDA_PIN, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPD);

    //
    // Enable the I2C peripheral.
    //
    SysCtlPeripheralEnable(DAC_I2C_PERIPH);

    //
    // Delay a while to ensure that we read a stable value from the SDA
    // GPIO pin.  If we read too quickly, the result is unpredictable.
    // This delay is around 2mS.
    //
    SysCtlDelay(SysCtlClockGet() / (3 * 500));

    //
    // Read the current state of the I2C1SDA line.  If it is low, the
    // daughter board must not be present since it should be pulled high.
    //
    if(!(GPIOPinRead(DAC_I2CSCL_GPIO_PORT, DAC_I2CSDA_PIN) & DAC_I2CSDA_PIN))
    {
        return(false);
    }

    //
    // Configure the I2C SCL and SDA pins for I2C operation.
    //
    GPIOPinTypeI2C(DAC_I2CSCL_GPIO_PORT, DAC_I2CSCL_PIN | DAC_I2CSDA_PIN);

    //
    // Initialize the I2C master.
    //
    I2CMasterInitExpClk(DAC_I2C_MASTER_BASE, SysCtlClockGet(), 0);

    //
    // Allow the rest of the public APIs to make hardware changes.
    //
    g_ucEnabled = 1;

    //
    // Reset the DAC.  Check the return code on this call since we use it to
    // indicate whether or not the DAC is present.  If the register write
    // fails, we assume the I2S daughter board and DAC are not present and
    // return false.
    //
    bRetcode = TLV320AIC23BWriteRegister(TI_RESET, 0);
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Power up the device and the DAC.
    //
    TLV320AIC23BWriteRegister(TI_POWER_DOWN, TI_POWER_DOWN_CLK |
                                             TI_POWER_DOWN_OSC);

    //
    // Set the sample rate.
    //
    TLV320AIC23BWriteRegister(TI_SRC, TI_SRC_SR_48000);

    //
    // Unmute the DAC.
    //
    TLV320AIC23BWriteRegister(TI_DIGITAL_AP, TI_DIGITAL_AP_DEEMP_48K |
                              TI_DIGITAL_AP_ADCHP);

    //
    // Enable the DAC path and insure the Mic input stays muted.
    //
    TLV320AIC23BWriteRegister(TI_ANALOG_AP, TI_ANALOG_AP_DAC |
                                            TI_ANALOG_AP_MICM);

    //
    // 16 bit I2S slave mode.
    //
    TLV320AIC23BWriteRegister(TI_DIGITAL_AI,
                              (TI_DIGITAL_AI_LRSWAP | TI_DIGITAL_AI_IWL_16 |
                               TI_DIGITAL_AI_FOR_I2S | TI_DIGITAL_AI_SLAVE));

    //
    // Set the Headphone volume.
    //
    TLV320AIC23BHeadPhoneVolumeSet(100);

    //
    // Unmute the Line input to the ADC.
    //
    TLV320AIC23BLineInVolumeSet(TLV_LINEIN_VC_0DB);

    //
    // Turn on the digital interface.
    //
    TLV320AIC23BWriteRegister(TI_DIGITAL_ACTIVATE, TI_DIGITAL_ACTIVATE_EN);

    return(true);
}

//*****************************************************************************
//
// Sets the Line In volume.
//
// \param ucVolume is the volume to set for the line input.
//
// This function adjusts the audio output up by the specified percentage.  The
// TI_LEFT_LINEIN_* values should be used for the \e ucVolume parameter.
//
// \return None
//
//*****************************************************************************
void
TLV320AIC23BLineInVolumeSet(unsigned char ucVolume)
{
    //
    // Unmute the line inputs and set the mixer to 0db.
    //
    TLV320AIC23BWriteRegister(TI_LEFT_LINEIN_VC, ucVolume);
    TLV320AIC23BWriteRegister(TI_RIGHT_LINEIN_VC, ucVolume);
}

//*****************************************************************************
//
// Sets the Headphone volume on the DAC.
//
// \param ulVolume is the volume to set, specified as a percentage between 0%
// (silence) and 100% (full volume), inclusive.
//
// This function adjusts the audio output up by the specified percentage.  The
// adjusted volume will not go above 100% (full volume).
//
// \return None
//
//*****************************************************************************
void
TLV320AIC23BHeadPhoneVolumeSet(unsigned long ulVolume)
{
    g_ucHPVolume = (unsigned char)ulVolume;

    //
    // Cap the volume at 100%
    //
    if(g_ucHPVolume >= 100)
    {
        g_ucHPVolume = 100;
    }

    if(g_ucEnabled == 1)
    {
        //
        // Set the left and right volumes with zero cross detect.
        //
        TLV320AIC23BWriteRegister(TI_LEFT_HP_VC,
                                  (TI_LEFT_HP_VC_LZC |
                                   pucVolumeTable[ulVolume >> 3]));
        TLV320AIC23BWriteRegister(TI_RIGHT_HP_VC,
                                  (TI_LEFT_HP_VC_LZC |
                                   pucVolumeTable[ulVolume >> 3]));
    }
}

//*****************************************************************************
//
// Returns the Headphone volume on the DAC.
//
// This function returns the current volume, specified as a percentage between
// 0% (silence) and 100% (full volume), inclusive.
//
// \return Returns the current volume.
//
//*****************************************************************************
unsigned long
TLV320AIC23BHeadPhoneVolumeGet(void)
{
    return(g_ucHPVolume);
}
