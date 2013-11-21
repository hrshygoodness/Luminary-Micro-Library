//*****************************************************************************
//
// dac.c - Functions supporting the TLV320AIC3107 audio DAC on EVALBOT.
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

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "dac.h"

//*****************************************************************************
//
//! \addtogroup dac_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The I2C pins that are used by this application.
//
//*****************************************************************************
#define DAC_I2C_PERIPH                  SYSCTL_PERIPH_I2C0
#define DAC_I2C_MASTER_BASE             I2C0_MASTER_BASE
#define DAC_I2CSCL_GPIO_PERIPH          SYSCTL_PERIPH_GPIOB
#define DAC_I2CSCL_GPIO_PORT            GPIO_PORTB_BASE
#define DAC_I2CSCL_PIN                  GPIO_PIN_2

#define DAC_RESET_GPIO_PERIPH           SYSCTL_PERIPH_GPIOA
#define DAC_RESET_GPIO_PORT             GPIO_PORTA_BASE
#define DAC_RESET_PIN                   GPIO_PIN_7

#define DAC_I2CSDA_GPIO_PERIPH          SYSCTL_PERIPH_GPIOB
#define DAC_I2CSDA_GPIO_PORT            GPIO_PORTB_BASE
#define DAC_I2CSDA_PIN                  GPIO_PIN_3

//*****************************************************************************
//
// The current output volume level.
//
//*****************************************************************************
static unsigned char g_ucHPVolume = 100;

//*****************************************************************************
//
// Writes a register in the TLV320AIC3107 DAC.
//
// \param ucRegister is the offset to the register to write.
// \param ulData is the data to be written to the DAC register.
//
//  This function will write the register passed in ucAddr with the value
//  passed in to ulData.  The data in ulData is actually 9 bits and the
//  value in ucAddr is interpreted as 7 bits.
//
// \return True on success or false on error.
//
//*****************************************************************************
static tBoolean
DACWriteRegister(unsigned char ucRegister, unsigned long ulData)
{
    //
    // Set the slave address.
    //
    ROM_I2CMasterSlaveAddrSet(DAC_I2C_MASTER_BASE, TI_TLV320AIC3107_ADDR,
                              false);

    //
    // Write the first byte to the controller (register)
    //
    ROM_I2CMasterDataPut(DAC_I2C_MASTER_BASE, ucRegister);

    //
    // Continue the transfer.
    //
    ROM_I2CMasterControl(DAC_I2C_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false) == 0)
    {
    }

    if(ROM_I2CMasterErr(DAC_I2C_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    {
        ROM_I2CMasterIntClear(DAC_I2C_MASTER_BASE);
        return(false);
    }

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false))
    {
        ROM_I2CMasterIntClear(DAC_I2C_MASTER_BASE);
    }

    //
    // Write the data byte to the controller.
    //
    ROM_I2CMasterDataPut(DAC_I2C_MASTER_BASE, ulData);

    //
    // End the transfer.
    //
    ROM_I2CMasterControl(DAC_I2C_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false) == 0)
    {
    }

    if(ROM_I2CMasterErr(DAC_I2C_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    {
        return(false);
    }

    while(ROM_I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false))
    {
        ROM_I2CMasterIntClear(DAC_I2C_MASTER_BASE);
    }

    return(true);
}

//*****************************************************************************
//
// Reads a register in the TLV320AIC3107 DAC.
//
// \param ucRegister is the offset to the register to write.
// \param pucData is a pointer to the returned data.
//
//  \return \b true on success or \b false on error.
//
//*****************************************************************************
static tBoolean
DACReadRegister(unsigned char ucRegister, unsigned char *pucData)
{
    //
    // Set the slave address and "WRITE"
    //
    ROM_I2CMasterSlaveAddrSet(DAC_I2C_MASTER_BASE, TI_TLV320AIC3107_ADDR,
                              false);

    //
    // Write the first byte to the controller (register)
    //
    ROM_I2CMasterDataPut(DAC_I2C_MASTER_BASE, ucRegister);

    //
    // Continue the transfer.
    //
    ROM_I2CMasterControl(DAC_I2C_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false) == 0)
    {
    }

    if(ROM_I2CMasterErr(DAC_I2C_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    {
        ROM_I2CMasterIntClear(DAC_I2C_MASTER_BASE);
        return(false);
    }

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false))
    {
        ROM_I2CMasterIntClear(DAC_I2C_MASTER_BASE);
    }

    //
    // Set the slave address and "READ"/true.
    //
    ROM_I2CMasterSlaveAddrSet(DAC_I2C_MASTER_BASE, TI_TLV320AIC3107_ADDR, true);

    //
    // Read Data Byte.
    //
    ROM_I2CMasterControl(DAC_I2C_MASTER_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false) == 0)
    {
    }

    if(ROM_I2CMasterErr(DAC_I2C_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    {
        ROM_I2CMasterIntClear(DAC_I2C_MASTER_BASE);
        return(false);
    }

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(DAC_I2C_MASTER_BASE, false))
    {
        ROM_I2CMasterIntClear(DAC_I2C_MASTER_BASE);
    }

    //
    // Read the value received.
    //
    *pucData  = ROM_I2CMasterDataGet(DAC_I2C_MASTER_BASE);

    return(true);
}

//*****************************************************************************
//
//! Initializes the TLV320AIC3107 DAC.
//!
//! This function initializes the I2C interface and the TLV320AIC3107 DAC.  It
//! must be called prior to any other API in the DAC module.
//!
//! \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
DACInit(void)
{
    tBoolean bRetcode;
    unsigned char ucTest;

    //
    // Enable the GPIO port containing the I2C pins and set the SDA pin as a
    // GPIO input for now and engage a weak pull-down.  If the daughter board
    // is present, the pull-up on the board should easily overwhelm
    // the pull-down and we should read the line state as high.
    //
    ROM_SysCtlPeripheralEnable(DAC_I2CSCL_GPIO_PERIPH);
    ROM_GPIOPinTypeGPIOInput(DAC_I2CSCL_GPIO_PORT, DAC_I2CSDA_PIN);
    ROM_GPIOPadConfigSet(DAC_I2CSCL_GPIO_PORT, DAC_I2CSDA_PIN,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

    //
    // Enable the I2C peripheral.
    //
    ROM_SysCtlPeripheralEnable(DAC_I2C_PERIPH);

    //
    // Delay a while to ensure that we read a stable value from the SDA
    // GPIO pin.  If we read too quickly, the result is unpredictable.
    // This delay is around 2mS.
    //
    SysCtlDelay(ROM_SysCtlClockGet() / (3 * 500));

    //
    // Configure the pin mux.
    //
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    //
    // Configure the I2C SCL and SDA pins for I2C operation.
    //
    ROM_GPIOPinTypeI2C(DAC_I2CSCL_GPIO_PORT, DAC_I2CSCL_PIN | DAC_I2CSDA_PIN);

    //
    // Initialize the I2C master.
    //
    ROM_I2CMasterInitExpClk(DAC_I2C_MASTER_BASE, SysCtlClockGet(), 0);

    //
    // Enable the I2C peripheral.
    //
    ROM_SysCtlPeripheralEnable(DAC_RESET_GPIO_PERIPH);

    //
    // Configure the PH2 as a GPIO output.
    //
    ROM_GPIOPinTypeGPIOOutput(DAC_RESET_GPIO_PORT, DAC_RESET_PIN);

    //
    // Reset the DAC
    //
    ROM_GPIOPinWrite(DAC_RESET_GPIO_PORT , DAC_RESET_PIN, 0);
    ROM_GPIOPinWrite(DAC_RESET_GPIO_PORT , DAC_RESET_PIN, DAC_RESET_PIN);

    //
    // Reset the DAC.  Check the return code on this call since we use it to
    // indicate whether or not the DAC is present.  If the register write
    // fails, we assume the I2S daughter board and DAC are not present and
    // return false.
    //
    bRetcode = DACWriteRegister(TI_SOFTWARE_RESET_R, 0x80);
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Codec Datapath Setup Register
    // ----------------------
    // D7     = 1  : Fsref = 44.1-kHz
    // D6     = 0  : ADC Dual rate mode is disabled
    // D5     = 0  : DAC Dual rate mode is disabled
    // D[4:3] = 11 : Left DAC datapath plays mono mix of left and right channel
    //               input data
    // D[1:1] = 00 : Right DAC datapath is off
    // D0     = 0  : reserved
    // ----------------------
    // D[7:0] =  10011010
    //
    DACWriteRegister(TI_CODEC_DATAPATH_R, 0x98);

    //
    // Audio Serial Data Interface Control Register A
    // ----------------------
    // D7     = 0  : BCLK is an input (slave mode)
    // D6     = 0  : WCLK (or GPIO1 if programmed as WCLK) is an input
    //               (slave mode)
    // D5     = 0  : Do not 3-state DOUT when valid data is not being sent
    // D4     = 0  : BCLK / WCLK (or GPIO1 if programmed as WCLK) will not
    //               continue to be transmitted when running in master mode if codec is powered down
    // D3     = 0  : Reserved.
    // D2     = 0  : Disable 3-D digital effect processing
    // D[1:0] = 00 : reserved
    // ----------------------
    // D[7:0] = 00000000
    //
    DACWriteRegister(TI_ASDI_CTL_A_R, 0x00);

    //
    // Audio Serial Data Interface Control Register B
    // ----------------------
    // D[7:6] = 00 : Serial data bus uses I2S mode
    // D[5:4] = 00 : Audio data word length = 16-bits
    // D3     = 0  : Continuous-transfer mode used to determine master mode
    //               bit clock rate
    // D2     = 0  : Don't Care
    // D1     = 0  : Don't Care
    // D0     = 0  : Re-Sync is done without soft-muting the channel. (ADC/DAC)
    // ----------------------
    // D[7:0] = 00000000
    //
    DACWriteRegister(TI_ASDI_CTL_B_R, 0x00);

    //
    // Audio Serial Data Interface Control Register C
    // ----------------------
    // D[7:0] = 00000000 : Data offset = 0 bit clocks
    // ----------------------
    // D[7:0] = 00000000
    //
    DACWriteRegister(TI_ASDI_CTL_C_R, 0x00);

    //
    // DAC Power and Output Driver Control Register
    // ----------------------
    // D7     = 1  : Left DAC is powered up
    // D6     = 1  : Right DAC is powered up
    // D[5:4] = 00 : HPCOM configured as differential of HPLOUT
    // D[3:0] = 0  : reserved
    // ----------------------
    // D[7:0] = 11000000
    //
    DACWriteRegister(TI_DACPOD_CTL_R, 0xC0);

    //
    // Left DAC Digital Volume Control Register
    // ----------------------
    // D7     = 0  : The left DAC channel is not muted
    // D[6:0] = 0  :
    // ----------------------
    // D[7:0] =
    //
    DACWriteRegister(TI_LEFT_DAC_DIG_VOL_CTL_R, 0x00);

    //
    // Right DAC Digital Volume Control Register
    // ----------------------
    // D7     = 0  : The right DAC channel is not muted
    // D[6:0] = 0  :
    // ----------------------
    // D[7:0] =
    //
    DACWriteRegister(TI_RIGHT_DAC_DIG_VOL_CTL_R, 0x00);

    //
    // DAC_L1 to LEFT_LOP Volume Control Register
    // ----------------------
    // D7     = 1  : DAC_L1 is routed to LEFT_LOP
    // D[6:0] = 0110010 (50)  : Gain
    // ----------------------
    // D[7:0] = 10110010
    //
    DACWriteRegister(TI_DAC_L1_LEFT_LOP_VOL_CTL_R, 0xA0);

    //
    // LEFT_LOP Output Level Control Register
    // ----------------------
    // D[7:4] = 0110  : Output level control = 6 dB
    // D3     = 1     :    LEFT_LOP is not muted
    // D2     = 0     :    Reserved.
    // D1     = 0     :    All programmed gains to LEFT_LOP have been applied
    // D0     = 1     :    LEFT_LOP is fully powered up
    // ----------------------
    // D[7:0] = 00001001
    //
    DACWriteRegister(TI_LEFT_LOP_OUTPUT_LVL_CTL_R, 0xC9);

    //
    // From the TLV320AIC3107 datasheet:
    // The following initialization sequence must be written to the AIC3107
    // registers prior to enabling the class-D amplifier:
    // register data:
    // 1. 0x00 0x0D
    // 2. 0x0D 0x0D
    // 3. 0x08 0x5C
    // 4. 0x08 0x5D
    // 5. 0x08 0x5C
    // 6. 0x00 0x00
    //
    DACWriteRegister(0x00, 0x0D);
    DACWriteRegister(0x0D, 0x0D);
    DACWriteRegister(0x08, 0x5C);
    DACWriteRegister(0x08, 0x5D);
    DACWriteRegister(0x08, 0x5C);
    DACWriteRegister(0x00, 0x00);

    //
    // Class-D and Bypass Switch Control Register
    // ----------------------
    // D[7:6] = 01 : Left Class-D amplifier gain = 6.0 dB
    // D[5:4] = 00 : Right Class-D amplifier gain = 0.0 dB
    // D3     = 1  : enable left class-D channel
    // D2     = 0  : disable right class-D channel
    // D1     = 0  : disable bypass switch
    // D0     = 0  : disable bypass switch bootstrap clock
    // ----------------------
    // D[7:0] = 01001000
    //
    DACWriteRegister(TI_CLASSD_BYPASS_SWITCH_CTL_R, 0x40);

    //
    //Read Module Power Status Register
    //
    bRetcode = DACReadRegister(TI_MODULE_PWR_STAT_R, &ucTest);
    if(!bRetcode)
    {
        return(bRetcode);
    }

    return(true);
}

//*****************************************************************************
//
//! Sets the volume on the DAC.
//!
//! \param ulVolume is the volume to set, specified as a percentage between 0%
//!         (silence) and 100% (full volume), inclusive.
//!
//! This function adjusts the audio output volume to the specified percentage.
//! The adjusted volume will not go above 100% (full volume).
//!
//! \return None.
//
//*****************************************************************************
void
DACVolumeSet(unsigned long ulVolume)
{
    //
    // Make sure we have been passed a valid volume value.
    //
    ASSERT(ulVolume <= 100);

    //
    // Update our local copy of the volume.
    //
    g_ucHPVolume = (unsigned char)ulVolume;

    //
    // Cap the volume at 100%
    //
    if(g_ucHPVolume >= 100)
    {
        g_ucHPVolume = 100;
    }

    //
    // Invert the % value.  This is because the max volume is at 0x00 and
    // minimum volume is at 0x7F.
    //
    ulVolume = 100 - ulVolume;

    //
    //Find what % of (0x7F) to set in the register.
    //
    ulVolume = (0x7F * ulVolume) / 100;

    //
    // DAC_L1 to LEFT_LOP Volume Control Register
    // ----------------------
    // D7     = 1  : DAC_L1 is routed to LEFT_LOP
    // D[6:0] =    : Gain
    // ----------------------
    // D[7:0] = 1XXXXXXX
    //
    DACWriteRegister(TI_DAC_L1_LEFT_LOP_VOL_CTL_R,
                     (0x80 | (unsigned char)ulVolume));
}

//*****************************************************************************
//
//! Returns the current DAC volume setting.
//!
//! This function may be called to determine the current DAC volume setting.
//! The value returned is expressed as a percentage of full volume.
//!
//! \return Returns the current volume setting as a percentage between 0 and
//!         100 inclusive.
//
//*****************************************************************************
unsigned long
DACVolumeGet(void)
{
    return(g_ucHPVolume);
}

//*****************************************************************************
//
//! Enables the class D amplifier in the DAC.
//!
//! This function enables the class D amplifier in the DAC.
//!
//! \return None.
//
//*****************************************************************************
void
DACClassDEn(void)
{
    DACWriteRegister(TI_CLASSD_BYPASS_SWITCH_CTL_R, 0x48);
}

//*****************************************************************************
//
//! Disables the class D amplifier in the DAC.
//!
//! This function disables the class D amplifier in the DAC.
//!
//! \return None.
//
//*****************************************************************************
void
DACClassDDis(void)
{
    DACWriteRegister(TI_CLASSD_BYPASS_SWITCH_CTL_R, 0x40);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
