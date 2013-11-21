//*****************************************************************************
//
// wm8510.c - Driver for the Wolfson WM8510 DAC/ADC
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

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "wm8510_regs.h"
#include "wm8510.h"

//*****************************************************************************
//
// Note: This driver is intended solely to allow use of a speaker attached to
// the IDM-SBC reference design kit board.  As a result, it enables only the
// speaker output and leaves all audio inputs disabled.  If you application
// requires audio input or the use of the mono output, you will need to modify
// this driver accordingly.
//
//*****************************************************************************

//*****************************************************************************
//
// The I2C pins that are used by this application.
//
//*****************************************************************************
#define I2C0SCL_PERIPH          (SYSCTL_PERIPH_GPIOB)
#define I2C0SCL_PORT            (GPIO_PORTB_BASE)
#define I2C0SCL_PIN             (GPIO_PIN_2)

#define I2C0SDA_PERIPH          (SYSCTL_PERIPH_GPIOB)
#define I2C0SDA_PORT            (GPIO_PORTB_BASE)
#define I2C0SDA_PIN             (GPIO_PIN_3)

//*****************************************************************************
//
// The lower limit of our volume control (corresponding to 0%) is defined to
// be -50dB at the DAC.  Volume levels below this are as good as muted and offer
// no real benefit to the user.
//
//*****************************************************************************
#define DAC_VOLUME_LOWER_LIMIT (DAC_VOLUME_0DB - (50 * 2))

//*****************************************************************************
//
// Global Volumes are needed because the device is not readable.
//
//*****************************************************************************
static unsigned long g_ulVolume = 100;
static unsigned char g_ucEnabled = 0;

//*****************************************************************************
//
// Write a register in the WM8510 DAC.
//
// \param ucRegister is the offset to the register to write.
// \param ulData is the data to be written to the DAC register.
//
// This function will write the register passed in /e ucAddr with the value
// passed in to /e ulData.  The data in \e ulData is actually 9 bits and the
// value in /e ucRegister is interpreted as 7 bits.
//
// \return Returns \b true on success or \b false on error.
//
//*****************************************************************************
static tBoolean
WM8510WriteRegister(unsigned char ucRegister, unsigned long ulData)
{
    //
    // Set the slave address.
    //
    ROM_I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, WM8510_I2C_ADDR_0, false);

    //
    // Write the next byte to the controller.
    //
    ROM_I2CMasterDataPut(I2C0_MASTER_BASE,
                         (ucRegister << 1) | ((ulData >> 8) & 1));

    //
    // Continue the transfer.
    //
    ROM_I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(I2C0_MASTER_BASE, false) == 0)
    {
    }

    if(ROM_I2CMasterErr(I2C0_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    {
        ROM_I2CMasterIntClear(I2C0_MASTER_BASE);
        return(false);
    }

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(I2C0_MASTER_BASE, false))
    {
        ROM_I2CMasterIntClear(I2C0_MASTER_BASE);
    }

    //
    // Write the next byte to the controller.
    //
    ROM_I2CMasterDataPut(I2C0_MASTER_BASE, ulData);

    //
    // End the transfer.
    //
    ROM_I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(I2C0_MASTER_BASE, false) == 0)
    {
    }

    if(ROM_I2CMasterErr(I2C0_MASTER_BASE) != I2C_MASTER_ERR_NONE)
    {
        return(false);
    }

    while(ROM_I2CMasterIntStatus(I2C0_MASTER_BASE, false))
    {
        ROM_I2CMasterIntClear(I2C0_MASTER_BASE);
    }

    return(true);
}

//*****************************************************************************
//
//! Initialize the WM8510 DAC.
//!
//! This function initializes the I2C interface and the WM8510 DAC.
//!
//! \return None
//
//*****************************************************************************
void
WM8510Init(void)
{
    //
    // Configure the I2C SCL and SDA pins for I2C operation.
    //
    ROM_SysCtlPeripheralEnable(I2C0SCL_PERIPH);
    ROM_GPIOPinTypeI2C(I2C0SCL_PORT, I2C0SCL_PIN | I2C0SDA_PIN);

    //
    // Enable the I2C peripheral.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

    //
    // Initialize the I2C master.
    //
    ROM_I2CMasterInitExpClk(I2C0_MASTER_BASE, SysCtlClockGet(), 0);

    //
    // Allow the rest of the public APIs to make hardware changes.
    //
    g_ucEnabled = 1;

    //
    // Reset the audio codec.  This sets all registers to their default values.
    //
    WM8510WriteRegister(WM8510_RESET_REG, 0);

    //
    // 32 bit I2S slave mode.
    //
    WM8510WriteRegister(WM8510_AUD_IF_REG, (AUD_IF_FMT_I2S |
                        AUD_IF_32_BIT_WORDS));

    //
    // Set the GPIO/CSB pin to a mode where it is an output.  The pin is not
    // connected on the basic board so we don't want it to be configured as
    // an input (the default) since it's not clear (to me) whether it is pulled
    // up or down within the device and we don't want a floating input muting
    // the output occasionally.
    //
    WM8510WriteRegister(WM8510_GPIO_REG, GPIO_SEL_AMUTE_ACTIVE);

    //
    // We want to use MCLK to clock the device rather than its PLL.
    //
    WM8510WriteRegister(WM8510_CLK_CTRL_REG, (CLOCK_SLAVE |
                        CLOCK_MCLK_DIVIDE_1 | CLOCK_BCLK_DIVIDE_1));

    //
    // Enable 1.5x boost for the speaker outputs.
    //
    WM8510WriteRegister(WM8510_OUTPUT_CTRL_REG, (OUTPUT_SPKR_BOOST_EN |
                        OUTPUT_THERMAL_SHUTDN_EN));
    //
    // Set power options.
    //
    WM8510WriteRegister(WM8510_POWER1_REG, (POWER1_VMID_50K | POWER1_BUFIO_EN |
                        POWER1_BIAS_EN | POWER1_BUFDCOP_EN));

    //
    // Wait 500mS for the supply voltage to settle after turning on VMID.
    //
    SysCtlDelay(SysCtlClockGet() / 6);

    //
    // Disable the microphone input path.
    //
    WM8510WriteRegister(WM8510_INPUT_CTRL_REG, 0);

    //
    // Enable the DAC
    //
    WM8510WriteRegister(WM8510_POWER3_REG, POWER3_DAC_EN);

    //
    // Enable the speaker and mono mixers.
    //
    WM8510WriteRegister(WM8510_POWER3_REG, (POWER3_SPKR_MIX_EN |
                        POWER3_MONO_MIX_EN | POWER3_DAC_EN));

    //
    // Enable the speaker and mono outputs.
    //
    WM8510WriteRegister(WM8510_POWER3_REG, (POWER3_SPKR_MIX_EN |
                        POWER3_MONO_MIX_EN | POWER3_SPKR_P_EN |
                        POWER3_SPKR_N_EN | POWER3_MONO_EN | POWER3_DAC_EN));

    //
    // Set the speaker volume but leave it muted for now.
    //
    WM8510WriteRegister(WM8510_SPKR_VOL_CTRL_REG,
                        (SPKR_VOL_0DB | SPKR_VOL_MUTE));

    //
    // Set the initial digital volume.
    //
    WM8510VolumeSet(100);

    //
    // Ensure that the DAC output is routed to the speaker.
    //
    WM8510WriteRegister(WM8510_SPKR_MIX_CTRL_REG, SPKR_MIX_DAC_TO_SPK);

    //
    // Set the mono output mixer but leave it muted.
    //
    WM8510WriteRegister(WM8510_MONO_MIX_CTRL_REG, (MONO_MIX_DAC_TO_MONO |
                        MONO_MIX_MUTE));

    //
    // Unmute the DAC.  DEBUG - Turn on AMUTE to check whether this is
    // reflected in the GPIO output when playback stops.  This will give us
    // some indication that I2S data is being read correctly.
    //
    WM8510WriteRegister(WM8510_DAC_CTRL_REG, (DAC_OVERSAMPLE_64X |
                        DAC_DEEMPHASIS_NONE | DAC_AUTO_MUTE_EN));

    //
    // Unmute the speaker now that everything else is set up.
    //
    WM8510WriteRegister(WM8510_SPKR_VOL_CTRL_REG, SPKR_VOL_0DB);

    //
    // Unmute the mono output.
    //
    WM8510WriteRegister(WM8510_MONO_MIX_CTRL_REG, MONO_MIX_DAC_TO_MONO);
}

//*****************************************************************************
//
//! Sets the volume on the DAC.
//!
//! \param ulVolume is the volume level to set, specified as a percentage
//! between 0% (silence) and 100% (full volume), inclusive.
//!
//! This function adjusts the audio output up by the specified percentage.  The
//! adjusted volume will not go above 100% (full volume).  The volume is scaled
//! linearly in the range 0db to -50dB with the DAC muted if 0% is passed.
//!
//! \return None
//
//*****************************************************************************
void
WM8510VolumeSet(unsigned long ulVolume)
{
    unsigned long ulDACVolume;

    g_ulVolume = ulVolume;

    //
    // Cap the volume at 100%
    //
    if(g_ulVolume >= 100)
    {
        g_ulVolume = 100;
    }

    if(g_ucEnabled == 1)
    {
        //
        // Scale and translate the percentage volume supplied into the valid
        // range for the DAC.
        //
        if(ulVolume == 0)
        {
            ulDACVolume = DAC_VOLUME_MUTE;
        }
        else
        {
            //
            // We are not muted so scale the 1-100% passed into the volume
            // range between 0dB (100%) and -50dB (1%).
            //
            ulDACVolume = (((unsigned long) g_ulVolume *
                            (DAC_VOLUME_0DB - DAC_VOLUME_LOWER_LIMIT)) / 100) +
                            DAC_VOLUME_LOWER_LIMIT;
        }

        //
        // Set the left and right volumes with zero cross detect.
        //
        WM8510WriteRegister(WM8510_DAC_VOL_REG, ulDACVolume);
    }
}

//*****************************************************************************
//
//! Returns the volume on the DAC.
//!
//! This function returns the current volume, specified as a percentage between
//! 0% (silence) and 100% (full volume), inclusive.
//!
//! \return Returns the current volume.
//
//*****************************************************************************
unsigned long
WM8510VolumeGet(void)
{
    return(g_ulVolume);
}
