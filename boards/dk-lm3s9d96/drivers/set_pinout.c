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
// This is part of revision 10636 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/epi.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "set_pinout.h"
#include "camerafpga.h"

//*****************************************************************************
//
//! \addtogroup set_pinout_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// NOTE: This module can be built in two ways.  If the label SIMPLE_PINOUT_SET
// is not defined, the PinoutSet() function will attempt to read an I2C EEPROM
// to determine which daughter board is attached to the development kit board
// and use information from that EEPROM to dynamically configure the EPI
// appropriately.  In this case, if no EEPROM is found, the EPI configuration
// will default to that required to use the SDRAM daughter board which is
// included with the base development kit.
//
// If SIMPLE_PINOUT_SET is defined, however, all the dynamic configuration code
// is replaced with a very simple function which merely sets the pinout and EPI
// configuration statically.  This is a better representation of how a real-
// world application would likely initialize the pinout and EPI timing and
// takes significantly less code space than the dynamic, daughter-board
// detecting version.  The example offered here sets the pinout and EPI
// configuration appropriately for the Flash/SRAM/LCD or FPGA/Camera/LCD
// daughter board depending upon another label definition.  If EPI_CONFIG_FPGA
// is defined, the configuration for the FPGA daughter board is set, otherwise
// the Flash/SRAM/LCD daughter board configuration is used.
//
//*****************************************************************************

//*****************************************************************************
//
// A global variable indicating which daughter board, if any, is currently
// connected to the development board.
//
//*****************************************************************************
tDaughterBoard g_eDaughterType;

#ifndef SIMPLE_PINOUT_SET
//*****************************************************************************
//
// The maximum number of GPIO ports.
//
//*****************************************************************************
#define NUM_GPIO_PORTS  9

//*****************************************************************************
//
// Base addresses of the GPIO ports that may contain EPI signals.  The index
// into this array must correlate with the index in the ucPortIndex field of
// tEPIPinInfo.
//
//*****************************************************************************
const unsigned long g_pulGPIOBase[NUM_GPIO_PORTS] =
{
    GPIO_PORTA_BASE,
    GPIO_PORTB_BASE,
    GPIO_PORTC_BASE,
    GPIO_PORTD_BASE,
    GPIO_PORTE_BASE,
    GPIO_PORTF_BASE,
    GPIO_PORTG_BASE,
    GPIO_PORTH_BASE,
    GPIO_PORTJ_BASE
};

//*****************************************************************************
//
// Structure used to map an EPI signal to a GPIO port and pin on the target
// part.  The ucPortIndex member is the index into the g_pulGPIOBase array
// containing the base address of the port.
//
//*****************************************************************************
typedef struct
{
    unsigned char ucPortIndex;
    unsigned char ucPin;
    unsigned long ulConfig;
}
tEPIPinInfo;

//*****************************************************************************
//
// The maximum number of EPI interface signals (EPI0Sxx).
//
//*****************************************************************************
#define NUM_EPI_SIGNALS 32

//*****************************************************************************
//
// The number of EPI clock periods for a write access with no wait states.
//
//*****************************************************************************
#define EPI_WRITE_CYCLES 4

//*****************************************************************************
//
// The number of EPI clock periods for a read access with no wait states.
//
//*****************************************************************************
#define EPI_READ_CYCLES  4

//*****************************************************************************
//
// The number of EPI clock periods added for each wait state.
//
//*****************************************************************************
#define EPI_WS_CYCLES    2

//*****************************************************************************
//
// This array holds the information necessary to map an EPI signal to a
// particular GPIO port and pin on the target part and also the port control
// nibble required to enable that EPI signal.  The index into the array is the
// EPI signal number.
//
//*****************************************************************************
static const tEPIPinInfo g_psEPIPinInfo[NUM_EPI_SIGNALS] =
{
    { 7, 3, GPIO_PH3_EPI0S0 },
    { 7, 2, GPIO_PH2_EPI0S1 },
    { 2, 4, GPIO_PC4_EPI0S2 },
    { 2, 5, GPIO_PC5_EPI0S3 },
    { 2, 6, GPIO_PC6_EPI0S4 },
    { 2, 7, GPIO_PC7_EPI0S5 },
    { 7, 0, GPIO_PH0_EPI0S6 },
    { 7, 1, GPIO_PH1_EPI0S7 },
    { 4, 0, GPIO_PE0_EPI0S8 },
    { 4, 1, GPIO_PE1_EPI0S9 },
    { 7, 4, GPIO_PH4_EPI0S10 },
    { 7, 5, GPIO_PH5_EPI0S11 },
    { 5, 4, GPIO_PF4_EPI0S12 },
    { 6, 0, GPIO_PG0_EPI0S13 },
    { 6, 1, GPIO_PG1_EPI0S14 },
    { 5, 5, GPIO_PF5_EPI0S15 },
    { 8, 0, GPIO_PJ0_EPI0S16 },
    { 8, 1, GPIO_PJ1_EPI0S17 },
    { 8, 2, GPIO_PJ2_EPI0S18 },
    { 8, 3, GPIO_PJ3_EPI0S19 },
    { 3, 2, GPIO_PD2_EPI0S20 },
    { 3, 3, GPIO_PD3_EPI0S21 },
    { 1, 5, GPIO_PB5_EPI0S22 },
    { 1, 4, GPIO_PB4_EPI0S23 },
    { 4, 2, GPIO_PE2_EPI0S24 },
    { 4, 3, GPIO_PE3_EPI0S25 },
    { 7, 6, GPIO_PH6_EPI0S26 },
    { 7, 7, GPIO_PH7_EPI0S27 },
    { 8, 4, GPIO_PJ4_EPI0S28 },
    { 8, 5, GPIO_PJ5_EPI0S29 },
    { 8, 6, GPIO_PJ6_EPI0S30 },
    { 6, 7, GPIO_PG7_EPI0S31 }
};

//*****************************************************************************
//
// Bit mask defining the EPI signals (EPI0Snn, for 0 <= n < 32) required for
// the default configuration (in this case, we assume the SDRAM daughter board
// is present).
//
//*****************************************************************************
#define EPI_PINS_SDRAM 0xF00FFFFF

//*****************************************************************************
//
// I2C connections for the EEPROM device used on DK daughter boards to provide
// an ID to applications.
//
//*****************************************************************************
#define ID_I2C_PERIPH              (SYSCTL_PERIPH_I2C0)
#define ID_I2C_MASTER_BASE         (I2C0_MASTER_BASE)
#define ID_I2CSCL_GPIO_PERIPH      (SYSCTL_PERIPH_GPIOB)
#define ID_I2CSCL_GPIO_PORT        (GPIO_PORTB_BASE)
#define ID_I2CSCL_PIN              (GPIO_PIN_2)

#define ID_I2CSDA_GPIO_PERIPH      (SYSCTL_PERIPH_GPIOB)
#define ID_I2CSDA_GPIO_PORT        (GPIO_PORTB_BASE)
#define ID_I2CSDA_PIN              (GPIO_PIN_3)

#define ID_I2C_ADDR                0x50

//*****************************************************************************
//
// Reads from the I2C-attached EEPROM device.
//
// \param pucData points to storage for the data read from the EEPROM.
// \param ulOffset is the EEPROM address of the first byte to read.
// \param ulCount is the number of bytes of data to read from the EEPROM.
//
// This function reads one or more bytes of data from a given address in the
// ID EEPROM found on several of the development kit daughter boards.  The
// return code indicates whether the read was successful.
//
// \return Returns \b true on success of \b false on failure.
//
//*****************************************************************************
static tBoolean
EEPROMReadPolled(unsigned char *pucData, unsigned long ulOffset,
                 unsigned long ulCount)
{
    unsigned long ulToRead, ulErr;

    //
    // Clear any previously signalled interrupts.
    //
    ROM_I2CMasterIntClear(ID_I2C_MASTER_BASE);

    //
    // Start with a dummy write to get the address set in the EEPROM.
    //
    ROM_I2CMasterSlaveAddrSet(ID_I2C_MASTER_BASE, ID_I2C_ADDR, false);

    //
    // Place the address to be written in the data register.
    //
    ROM_I2CMasterDataPut(ID_I2C_MASTER_BASE, ulOffset);

    //
    // Perform a single send, writing the address as the only byte.
    //
    ROM_I2CMasterControl(ID_I2C_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(ID_I2C_MASTER_BASE, false) == 0)
    {
    }

    //
    // Was any error reported during the transaction?
    //
    ulErr = I2CMasterErr(ID_I2C_MASTER_BASE);
    if(ulErr != I2C_MASTER_ERR_NONE)
    {
        //
        // Clear the error.
        //
        ROM_I2CMasterIntClear(ID_I2C_MASTER_BASE);

        //
        // Is the arbitration lost error set?
        //
        if(ulErr & I2C_MASTER_ERR_ARB_LOST)
        {
            //
            // Kick the controller hard to clear the arbitration lost error.
            //
            ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);
            SysCtlDelay(10);

            //
            // Restore the I2C state.
            //
            ROM_I2CMasterInitExpClk(ID_I2C_MASTER_BASE, ROM_SysCtlClockGet(),
                                    0);
            ROM_I2CMasterControl(ID_I2C_MASTER_BASE,
                                 I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
        }

        //
        // Send a stop condition to get the controller back to the idle
        // state and release SDA and SCL.
        //
        ROM_I2CMasterControl(ID_I2C_MASTER_BASE,
                             I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);

        //
        // Tell the caller we had a problem.
        //
        return(false);
    }

    //
    // Clear any interrupts set.
    //
    ROM_I2CMasterIntClear(ID_I2C_MASTER_BASE);

    //
    // Put the I2C master into receive mode.
    //
    ROM_I2CMasterSlaveAddrSet(ID_I2C_MASTER_BASE, ID_I2C_ADDR, true);

    //
    // Start the receive.
    //
    ROM_I2CMasterControl(ID_I2C_MASTER_BASE,
                     ((ulCount > 1) ? I2C_MASTER_CMD_BURST_RECEIVE_START :
                     I2C_MASTER_CMD_SINGLE_RECEIVE));

    //
    // Receive the required number of bytes.
    //
    ulToRead = ulCount;

    while(ulToRead)
    {
        //
        // Wait until the current byte has been read.
        //
        while(ROM_I2CMasterIntStatus(ID_I2C_MASTER_BASE, false) == 0)
        {
        }

        //
        // Read the received character.
        //
        *pucData++ = ROM_I2CMasterDataGet(ID_I2C_MASTER_BASE);
        ulToRead--;

        //
        // Clear pending interrupt notifications.
        //
        ROM_I2CMasterIntClear(ID_I2C_MASTER_BASE);

        //
        // Set up for the next byte if any more remain.
        //
        if(ulToRead)
        {
            ROM_I2CMasterControl(ID_I2C_MASTER_BASE,
                                 ((ulToRead == 1) ?
                                  I2C_MASTER_CMD_BURST_RECEIVE_FINISH :
                                  I2C_MASTER_CMD_BURST_RECEIVE_CONT));
        }
    }

    //
    // Clear pending interrupt notification.
    //
    ROM_I2CMasterIntClear(ID_I2C_MASTER_BASE);

    //
    // Tell the caller we read the required data.
    //
    return(true);
}

//*****************************************************************************
//
// Determines which daughter board is currently attached to the development
// board and returns the daughter board's information block as
//
// This function determines which of the possible daughter boards are attached
// to the development board.  It recognizes Flash/SRAM and FPGA daughter
// boards, each of which contains an I2C device which may be queried to
// identify the board.  In cases where the SDRAM daughter board is attached,
// this function will return \b NONE and the determination of whether or not
// the board is present is left to function SDRAMInit() in extram.c.
//
// \return Returns \b DAUGHTER_FPGA if the FPGA daugher is detected, \b
// DAUGHTER_SRAM_FLASH if the SRAM and flash board is detected, \b
// DAUGHTER_EM2 if the EM2 LPRF board is detected and \b DAUGHTER_NONE if
//! no board could be identified (covering the cases where either no
//! board or the SDRAM board is attached).
//
//*****************************************************************************
static tDaughterBoard
DaughterBoardTypeGet(tDaughterIDInfo *psInfo)
{
    tBoolean bRetcode;

    //
    // Enable the I2C controller used to interface to the daughter board ID
    // EEPROM (if present) and reset it.  Note that the I2C master must be
    // clocked for SysCtlPeripheralReset() to reset the block so we need
    // to call I2CMasterEnable() between the two SysCtl calls.
    //
    ROM_SysCtlPeripheralEnable(ID_I2C_PERIPH);
    SysCtlDelay(1);
    ROM_I2CMasterEnable(ID_I2C_MASTER_BASE);
    SysCtlDelay(1);
    ROM_SysCtlPeripheralReset(ID_I2C_PERIPH);

    //
    // Configure the I2C SCL and SDA pins for I2C operation.
    //
    ROM_GPIOPinTypeI2C(ID_I2CSCL_GPIO_PORT, ID_I2CSCL_PIN | ID_I2CSDA_PIN);

    //
    // Initialize the I2C master.
    //
    ROM_I2CMasterInitExpClk(ID_I2C_MASTER_BASE, ROM_SysCtlClockGet(), 0);

    //
    // Read the ID information from the I2C EEPROM.
    //
    bRetcode = EEPROMReadPolled((unsigned char *)psInfo, 0,
                                sizeof(tDaughterIDInfo));

    //
    // If there was an error, we try once more.  This is a workaround for an
    // erratum on Tempest which can cause occasional "arbitration lost" errors
    // from I2C.  Trying twice doesn't absolutely guarantee that we work around
    // the problem but it occurs very seldom so this gives us pretty good
    // immunity.
    //
    if(!bRetcode)
    {
        bRetcode = EEPROMReadPolled((unsigned char *)psInfo, 0,
                                    sizeof(tDaughterIDInfo));
    }

    //
    // Did we read the ID information successfully?
    //
    if(bRetcode)
    {
        //
        // Yes.  Check that the structure marker is what we expect.
        //
        if((psInfo->pucMarker[0] == 'I') && (psInfo->pucMarker[1] == 'D'))
        {
            //
            // Marker is fine so return the board ID.
            //
            return((tDaughterBoard)psInfo->usBoardID);
        }
    }

    //
    // We experienced an error reading the ID EEPROM or read no valid info
    // structure from the device.  This likely indicates that no daughter
    // board is present.  Set the return structure to configure the system
    // assuming that the default (SDRAM) daughter board is present.
    //
    psInfo->usBoardID = (unsigned short)DAUGHTER_NONE;
    psInfo->ulEPIPins = EPI_PINS_SDRAM;
    psInfo->ucEPIMode = EPI_MODE_SDRAM;
    psInfo->ulConfigFlags = (EPI_SDRAM_FULL_POWER | EPI_SDRAM_SIZE_64MBIT);
    psInfo->ucAddrMap = (EPI_ADDR_RAM_SIZE_256MB | EPI_ADDR_RAM_BASE_6);
    psInfo->usRate0nS = 20;
    psInfo->usRate1nS = 20;
    psInfo->ucRefreshInterval = 64;
    psInfo->usNumRows = 4096;
    return(DAUGHTER_NONE);
}

//*****************************************************************************
//
// Given the system clock rate and a desired EPI rate, calculate the divider
// necessary to set the EPI clock at or lower than but as close as possible to
// the desired rate.  The divider is returned and the desired rate is updated
// to give the actual EPI clock rate (in nanoseconds) that will result from
// the use of the calculated divider.
//
//*****************************************************************************
static unsigned short
EPIDividerFromRate(unsigned short *pusDesiredRate, unsigned long ulClknS)
{
    unsigned long ulDivider, ulDesired;

    //
    // If asked for an EPI clock that is at or above the system clock rate,
    // set the divider to 0 and update the EPI rate to match the system clock
    // rate.
    //
    if((unsigned long)*pusDesiredRate <= ulClknS)
    {
        *pusDesiredRate = (unsigned short)ulClknS;
        return(0);
    }

    //
    // The desired EPI rate is slower than the system clock so determine
    // the divider value to use to achieve this as best we can.  The divider
    // generates the EPI clock using the following formula:
    //
    //                     System Clock
    // EPI Clock =   -----------------------
    //                ((Divider/2) + 1) * 2
    //
    // The formula for ulDivider below is determined by reforming this
    // equation and including a (ulClknS - 1) term to ensure that we round
    // the correct way, generating an EPI clock that is never faster than
    // the requested rate.
    //
    ulDesired = (unsigned long)*pusDesiredRate;
    ulDivider = 2 * ((((ulDesired + (ulClknS - 1)) / ulClknS) / 2) - 1) + 1;

    //
    // Now calculate the actual EPI clock period based on the divider we
    // just chose.
    //
    *pusDesiredRate = (unsigned short)(ulClknS * (2 * ((ulDivider / 2) + 1)));

    //
    // Return the divider we calculated.
    //
    return((unsigned short)ulDivider);
}

//*****************************************************************************
//
// Calculate the divider parameter required by EPIDividerSet() based on the
// current system clock rate and the desired EPI rates supplied in the
// usRate0nS and usRate1nS fields of the daughter board information structure.
//
// The dividers are calculated to ensure that the EPI rate is no faster than
// the requested rate and the rate fields in psInfo are updated to reflect the
// actual rate that will be used based on the calculated divider.
//
//*****************************************************************************
static unsigned long
CalcEPIDivider(tDaughterIDInfo *psInfo, unsigned long ulClknS)
{
    unsigned short usDivider0, usDivider1;
    unsigned short pusRate[2];

    //
    // Calculate the dividers required for the two rates specified.
    //
    pusRate[0] = psInfo->usRate0nS;
    pusRate[1] = psInfo->usRate1nS;
    usDivider0 = EPIDividerFromRate(&(pusRate[0]), ulClknS);
    usDivider1 = EPIDividerFromRate(&(pusRate[1]), ulClknS);
    psInfo->usRate0nS = pusRate[0];
    psInfo->usRate1nS = pusRate[1];

    //
    // Munge the two dividers together into a format suitable to pass to
    // EPIDividerSet().
    //
    return((unsigned long)usDivider0 | (((unsigned long)usDivider1) << 16));
}

//*****************************************************************************
//
// Returns the configuration parameter for EPIConfigHB8Set() based on the
// config flags and read and write access times found in the psInfo structure,
// and the current EPI clock clock rate as found in the usRate0nS field of the
// psInfo structure.
//
// The EPI clock rate is used to determine the number of wait states required
// so CalcEPIDivider() must have been called before this function to ensure
// that the usRate0nS field has been updated to reflect the actual EPI clock in
// use.  Note, also, that there is only a single read and write wait state
// setting even if dual chip selects are in use.  In this case, the caller
// must ensure that the dividers and access times provided generate suitable
// cycles for the devices attached to both chip selects.
//
//*****************************************************************************
static unsigned long
HB8ConfigGet(tDaughterIDInfo *psInfo)
{
    unsigned long ulConfig, ulWrWait, ulRdWait;

    //
    // Start with the config flags provided in the information structure.
    //
    ulConfig = psInfo->ulConfigFlags;

    //
    // How many write wait states do we need?
    //
    if((unsigned long)psInfo->ucWriteAccTime >
       (EPI_WRITE_CYCLES * (unsigned long)psInfo->usRate0nS))
    {
        //
        // The access time is more than 4 EPI clock cycles so we need to
        // introduce some wait states.  How many?
        //
        ulWrWait = (unsigned long)psInfo->ucWriteAccTime -
                   (EPI_WRITE_CYCLES * psInfo->usRate0nS);
        ulWrWait += ((EPI_WS_CYCLES * psInfo->usRate0nS) - 1);
        ulWrWait /= (EPI_WS_CYCLES * psInfo->usRate0nS);

        //
        // The hardware only allows us to specify 0, 1, 2 or 3 wait states.  If
        // we end up with a number greater than 3, we have a problem.  This
        // indicates an error in the daughter board information structure.
        //
        ASSERT(ulWrWait < 4);

        //
        // Set the configuration flag indicating the desired number of write
        // wait states.
        //
        switch(ulWrWait)
        {
            case 0:
            {
                break;
            }

            case 1:
            {
                ulConfig |= EPI_HB8_WRWAIT_1;
                break;
            }

            case 2:
            {
                ulConfig |= EPI_HB8_WRWAIT_2;
                break;
            }

            case 3:
            default:
            {
                ulConfig |= EPI_HB8_WRWAIT_3;
                break;
            }
        }
    }

    //
    // How many read wait states do we need?
    //
    if((unsigned long)psInfo->ucReadAccTime >
       (EPI_READ_CYCLES * (unsigned long)psInfo->usRate0nS))
    {
        //
        // The access time is more than 3 EPI clock cycles so we need to
        // introduce some wait states.  How many?
        //
        ulRdWait = (unsigned long)psInfo->ucReadAccTime -
                   (EPI_READ_CYCLES * psInfo->usRate0nS);
        ulRdWait += ((EPI_WS_CYCLES * psInfo->usRate0nS) - 1);
        ulRdWait /= (EPI_WS_CYCLES * psInfo->usRate0nS);

        //
        // The hardware only allows us to specify 0, 1, 2 or 3 wait states.  If
        // we end up with a number greater than 3, we have a problem.  This
        // indicates an error in the daughter board information structure.
        //
        ASSERT(ulRdWait < 4);

        //
        // Set the configuration flag indicating the desired number of read
        // wait states.
        //
        switch(ulRdWait)
        {
            case 0:
            {
                break;
            }

            case 1:
            {
                ulConfig |= EPI_HB8_RDWAIT_1;
                break;
            }

            case 2:
            {
                ulConfig |= EPI_HB8_RDWAIT_2;
                break;
            }

            case 3:
            default:
            {
                ulConfig |= EPI_HB8_RDWAIT_3;
                break;
            }
        }
    }

    //
    // Return the configuration flags back to the caller.
    //
    return(ulConfig);
}

//*****************************************************************************
//
// Returns the configuration parameters for EPIConfigSDRAMSet() based on the
// config flags, device size and refresh interval provided in psInfo and the
// system clock rate provided in ulClkHz.
//
//*****************************************************************************
static unsigned long
SDRAMConfigGet(tDaughterIDInfo *psInfo, unsigned long ulClkHz,
               unsigned long *pulRefresh)
{
    unsigned long ulConfig;

    //
    // Start with the config flags provided to us.
    //
    ulConfig = psInfo->ulConfigFlags;

    //
    // Set the SDRAM core frequency depending upon the system clock rate.
    //
    if(ulClkHz < 15000000)
    {
        ulConfig |= EPI_SDRAM_CORE_FREQ_0_15;
    }
    else if(ulClkHz < 30000000)
    {
        ulConfig |= EPI_SDRAM_CORE_FREQ_15_30;
    }
    else if(ulClkHz < 50000000)
    {
        ulConfig |= EPI_SDRAM_CORE_FREQ_30_50;
    }
    else
    {
        ulConfig |= EPI_SDRAM_CORE_FREQ_50_100;
    }

    //
    // Now determine the correct refresh count required to refresh the entire
    // device in the time specified.
    //
    *pulRefresh = ((ulClkHz / psInfo->usNumRows) *
                  (unsigned long)psInfo->ucRefreshInterval) / 1000;

    //
    // Return the calculated configuration parameter to the caller.
    //
    return(ulConfig);
}

//*****************************************************************************
//
// Configures all pins associated with the Extended Peripheral Interface (EPI).
//
// \param eDaughter identifies the attached daughter board (if any).
//
// This function configures all pins forming part of the EPI on the device and
// configures the EPI peripheral appropriately for whichever hardware we
// detect is connected to it. On exit, the EPI peripheral is enabled and all
// pins associated with the interface are configured as EPI signals. Drive
// strength is set to 8mA.
//
//*****************************************************************************
static void
EPIPinConfigSet(tDaughterIDInfo *psInfo)
{
    unsigned long ulLoop, ulClk, ulNsPerTick, ulRefresh;
    unsigned char pucPins[NUM_GPIO_PORTS];

    //
    // Enable the EPI peripheral
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_EPI0);

    //
    // Clear our pin bit mask array.
    //
    for(ulLoop = 0; ulLoop < NUM_GPIO_PORTS; ulLoop++)
    {
        pucPins[ulLoop] = 0;
    }

    //
    // Determine the pin bit masks for the EPI pins for each GPIO port.
    //
    for(ulLoop = 0; ulLoop < NUM_EPI_SIGNALS; ulLoop++)
    {
        //
        // Is this EPI signal required?
        //
        if(psInfo->ulEPIPins & (1 << ulLoop))
        {
            //
            // Yes - set the appropriate bit in our pin bit mask array.
            //
            pucPins[g_psEPIPinInfo[ulLoop].ucPortIndex] |=
                (1 << g_psEPIPinInfo[ulLoop].ucPin);
        }
    }

    //
    // At this point, pucPins contains bit masks for each GPIO port with 1s in
    // the positions of every required EPI signal.  Now we need to configure
    // those pins appropriately.  Cycle through each port configuring EPI pins
    // in any port which contains them.
    //
    for(ulLoop = 0; ulLoop < NUM_GPIO_PORTS; ulLoop++)
    {
        //
        // Are there any EPI pins used in this port?
        //
        if(pucPins[ulLoop])
        {
            //
            // Yes - configure the EPI pins.
            //
            ROM_GPIOPadConfigSet(g_pulGPIOBase[ulLoop], pucPins[ulLoop],
                                 GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
            ROM_GPIODirModeSet(g_pulGPIOBase[ulLoop], pucPins[ulLoop],
                               GPIO_DIR_MODE_HW);
        }
    }

    //
    // Now set the EPI operating mode for the daughter board detected.  We need
    // to determine some timing information based on the ID block we have and
    // also the current system clock.
    //
    ulClk = ROM_SysCtlClockGet();
    ulNsPerTick = 1000000000 / ulClk;

    //
    // If the EPI is not disabled (the daughter board may, for example, want
    // to use all the pins for GPIO), configure the interface as required.
    //
    if(psInfo->ucEPIMode != EPI_MODE_DISABLE)
    {
        //
        // Set the EPI clock divider to ensure a basic EPI clock rate no faster
        // than defined via the ucRate0nS and ucRate1nS fields in the info
        // structure.
        //
        EPIDividerSet(EPI0_BASE, CalcEPIDivider(psInfo, ulNsPerTick));

        //
        // Set the basic EPI operating mode based on the value from the info
        // structure.
        //
        EPIModeSet(EPI0_BASE, psInfo->ucEPIMode);

        //
        // Carry out mode-dependent configuration.
        //
        switch(psInfo->ucEPIMode)
        {
            //
            // The daughter board must be configured for SDRAM operation.
            //
            case EPI_MODE_SDRAM:
            {
                //
                // Work out the SDRAM configuration settings based on the
                // supplied ID structure and system clock rate.
                //
                ulLoop = SDRAMConfigGet(psInfo, ulClk, &ulRefresh);

                //
                // Set the SDRAM configuration.
                //
                EPIConfigSDRAMSet(EPI0_BASE, ulLoop, ulRefresh);
                break;
            }

            //
            // The daughter board must be configured for HostBus8 operation.
            //
            case EPI_MODE_HB8:
            {
                //
                // Determine the number of read and write wait states required
                // to meet the supplied access timing.
                //
                ulLoop = HB8ConfigGet(psInfo);

                //
                // Set the HostBus8 configuration.
                //
                EPIConfigHB8Set(EPI0_BASE, ulLoop, psInfo->ucMaxWait);
                break;
            }

            //
            // The daughter board must be configured for Non-Moded/General
            // Purpose operation.
            //
            case EPI_MODE_GENERAL:
            {
                EPIConfigGPModeSet(EPI0_BASE, psInfo->ulConfigFlags,
                                   psInfo->ucFrameCount, psInfo->ucMaxWait);
                break;
            }
        }

        //
        // Set the EPI address mapping.
        //
        EPIAddressMapSet(EPI0_BASE, psInfo->ucAddrMap);
    }
}

//*****************************************************************************
//
// Set the GPIO port control registers appropriately for the hardware.
//
// This function determines the correct port control settings to enable the
// basic peripheral signals for the development board on their respective pins
// and also ensures that all required EPI signals are correctly routed.  The
// EPI signal configuration is determined from the daughter board information
// structure passed via the \e psInfo parameter.
//
//*****************************************************************************
static void
PortControlSet(tDaughterIDInfo *psInfo)
{
    unsigned long ulLoop;

    //
    // To begin with, we set the port control values for all the non-EPI
    // peripherals.
    //

    //
    // GPIO Port A pins
    //
    // To use CAN0, these calls must be changed.  This enables USB
    // functionality instead of CAN. For CAN, use:
    //
    //     GPIOPinConfigure(GPIO_PA6_CAN0RX);
    //     GPIOPinConfigure(GPIO_PA7_CAN0TX);
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);
    GPIOPinConfigure(GPIO_PA6_USB0EPEN);
    GPIOPinConfigure(GPIO_PA7_USB0PFLT);

    //
    // GPIO Port B pins
    //
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    GPIOPinConfigure(GPIO_PB6_I2S0TXSCK);
    GPIOPinConfigure(GPIO_PB7_NMI);

    //
    // GPIO Port D pins.
    //
    GPIOPinConfigure(GPIO_PD0_I2S0RXSCK);
    GPIOPinConfigure(GPIO_PD1_I2S0RXWS);
    GPIOPinConfigure(GPIO_PD4_I2S0RXSD);
    GPIOPinConfigure(GPIO_PD5_I2S0RXMCLK);

    //
    // GPIO Port E pins
    //
    GPIOPinConfigure(GPIO_PE4_I2S0TXWS);
    GPIOPinConfigure(GPIO_PE5_I2S0TXSD);

    //
    // GPIO Port F pins
    //
    GPIOPinConfigure(GPIO_PF1_I2S0TXMCLK);
    GPIOPinConfigure(GPIO_PF2_LED1);
    GPIOPinConfigure(GPIO_PF3_LED0);

    //
    // Now we configure each of the EPI pins if it is needed.
    //
    for(ulLoop = 0; ulLoop < NUM_EPI_SIGNALS; ulLoop++)
    {
        //
        // Is this EPI pin used by this daughter board?
        //
        if(psInfo->ulEPIPins & (1 << ulLoop))
        {
            //
            // Yes - configure the corresponding pin.
            //
            GPIOPinConfigure(g_psEPIPinInfo[ulLoop].ulConfig);
        }
    }
}

//*****************************************************************************
//
//! Configures the device pinout for the development board.
//!
//! This function configures each pin of the device to route the! appropriate
//! peripheral signal as required by the design of the development board.
//!
//! \note This module can be built in two ways.  If the label SIMPLE_PINOUT_SET
//! is not defined, the PinoutSet() function will attempt to read an I2C EEPROM
//! to determine which daughter board is attached to the development kit board
//! and use information from that EEPROM to dynamically configure the EPI
//! appropriately.  In this case, if no EEPROM is found, the EPI configuration
//! will default to that required to use the SDRAM daughter board which is
//! included with the base development kit.
//!
//! If SIMPLE_PINOUT_SET is defined, however, all the dynamic configuration
//! code is replaced with a very simple function which merely sets the pinout
//! and EPI configuration statically.  This is a better representation of how a
//! real-world application would likely initialize the pinout and EPI timing
//! and takes significantly less code space than the dynamic, daughter-board
//! detecting version.  The example offered here sets the pinout and EPI
//! configuration appropriately for the Flash/SRAM/LCD daughter board.
//!
//! \return None.
//
//*****************************************************************************
void
PinoutSet(void)
{
    tDaughterIDInfo sInfo;

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
    // Determine which daughter board (if any) is currently attached to the
    // development board.
    //
    g_eDaughterType = DaughterBoardTypeGet(&sInfo);

    //
    // Determine the port control settings required to enable the EPI pins
    // and other peripheral signals for this daughter board and set all the
    // GPIO port control registers.
    //
    PortControlSet(&sInfo);

    //
    // Set the pin configuration for the Extended Peripheral Interface (EPI)
    //
    EPIPinConfigSet(&sInfo);

    //
    // It's somewhat unfortunate that this needs to be here since it breaks
    // the attempt to keep this file completely daughter-board agnostic but
    // the Camera/FPGA daughter board needs to be reset before it can be used
    // and, if we don't do it here, we end up with the problem of deciding how
    // to handle it if using both the camera and display driver for the board.
    // Both of these drivers could be used independently and both of them
    // require that the board be reset but both are also messed up if the other
    // resets the board after they have been initialized.  The simplest
    // solution, therefore, is merely to reset the board once as soon after
    // booting as possible.
    //
    if(g_eDaughterType == DAUGHTER_FPGA)
    {
        //
        // Configure the FPGA reset signal.
        //
        ROM_GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, GPIO_PIN_6);

        //
        // Configure the interrupt line from the FPGA.
        //
        ROM_GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_6);

        //
        // Assert the FPGA reset for a while.
        //
        ROM_GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_6, 0);
        SysCtlDelay(10);
        ROM_GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_6, GPIO_PIN_6);

        //
        // Wait 600mS for the device to be completely ready.  This time
        // allows the FPGA to load its image from EEPROM after a power-on-
        // reset.
        //
        SysCtlDelay(ROM_SysCtlClockGet() / 5);

        //
        // Perform a write to the "read only" version register.  This is a
        // special case - the FPGA uses this access to determine whether it is
        // connected to a Tempest rev B or rev C.  The EPI timings are
        // different between these two revisions.
        //
        HWREGH(FPGA_VERSION_REG) = 0;
    }
}
#else
//*****************************************************************************
//
// The following simple implementation merely sets the pinout and EPI
// configuration based on a hardcoded set of parameters.  This is less
// flexible but more likely to be the code that is used in a real world
// application where you don't have to worry about supporting multiple
// different daughter boards connected to the EPI.
//
//*****************************************************************************

//*****************************************************************************
//
// Use the following to specify the GPIO pins used by the EPI bus when
// configured in HostBus8 mode.  These basic definitions set up the pins
// required for the Flash/SRAM/LCD daughter board.
//
//*****************************************************************************
#define EPI_PORTA_PINS          0x00
#define EPI_PORTB_PINS          0x30
#define EPI_PORTC_PINS          0xf0
#define EPI_PORTD_PINS          0x0C
#define EPI_PORTE_PINS          0x0F
#define EPI_PORTF_PINS          0x30
#define EPI_PORTG_PINS          0x83
#ifdef EPI_CONFIG_FPGA
#define EPI_PORTH_PINS          0xBF
#else
#define EPI_PORTH_PINS          0xFF
#endif
#define EPI_PORTJ_PINS          0x7f

//*****************************************************************************
//
// Deviations from the standard EPI pin configuration required for use with
// the SRAM/Flash daughter board which uses EPI HostBus8 mode and repurposes
// some of the LCD control signal GPIOs for this.
//
//*****************************************************************************

//*****************************************************************************
//
// Configures all pins associated with the Extended Peripheral Interface (EPI).
//
// \param eDaughter identifies the attached daughter board (if any).
//
// This function configures all pins forming part of the EPI on the device and
// configures the EPI peripheral appropriately for whichever hardware we
// detect is connected to it. On exit, the EPI peripheral is enabled and all
// pins associated with the interface are configured as EPI signals. Drive
// strength is set to 8mA for all pins.
//
//*****************************************************************************
static void
EPIPinConfigSet(void)
{
    //
    // Enable the EPI peripheral
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EPI0);

    //
    // Configure the EPI pins that are to be used on this board.
    //
#if (EPI_PORTA_PINS != 0x00)
    ROM_GPIOPadConfigSet(GPIO_PORTA_BASE, EPI_PORTA_PINS, GPIO_STRENGTH_8MA,
                         GPIO_PIN_TYPE_STD);
    ROM_GPIODirModeSet(GPIO_PORTA_BASE, EPI_PORTA_PINS, GPIO_DIR_MODE_HW);
#endif
#if (EPI_PORTC_PINS != 0x00)
    ROM_GPIOPadConfigSet(GPIO_PORTC_BASE, EPI_PORTC_PINS, GPIO_STRENGTH_8MA,
                         GPIO_PIN_TYPE_STD);
    ROM_GPIODirModeSet(GPIO_PORTC_BASE, EPI_PORTC_PINS, GPIO_DIR_MODE_HW);
#endif
#if EPI_PORTE_PINS
    ROM_GPIOPadConfigSet(GPIO_PORTE_BASE, EPI_PORTE_PINS, GPIO_STRENGTH_8MA,
                         GPIO_PIN_TYPE_STD);
    ROM_GPIODirModeSet(GPIO_PORTE_BASE, EPI_PORTE_PINS, GPIO_DIR_MODE_HW);
#endif
#if EPI_PORTF_PINS
    ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, EPI_PORTF_PINS, GPIO_STRENGTH_8MA,
                         GPIO_PIN_TYPE_STD);
    ROM_GPIODirModeSet(GPIO_PORTF_BASE, EPI_PORTF_PINS, GPIO_DIR_MODE_HW);
#endif
#if EPI_PORTG_PINS
    ROM_GPIOPadConfigSet(GPIO_PORTG_BASE, EPI_PORTG_PINS, GPIO_STRENGTH_8MA,
                        GPIO_PIN_TYPE_STD);
    ROM_GPIODirModeSet(GPIO_PORTG_BASE, EPI_PORTG_PINS, GPIO_DIR_MODE_HW);
#endif
#if EPI_PORTJ_PINS
    ROM_GPIOPadConfigSet(GPIO_PORTJ_BASE, EPI_PORTJ_PINS, GPIO_STRENGTH_8MA,
                         GPIO_PIN_TYPE_STD);
    ROM_GPIODirModeSet(GPIO_PORTJ_BASE, EPI_PORTJ_PINS, GPIO_DIR_MODE_HW);
#endif

#if (EPI_PORTB_PINS != 0x00)
    ROM_GPIOPadConfigSet(GPIO_PORTB_BASE, EPI_PORTB_PINS, GPIO_STRENGTH_8MA,
                         GPIO_PIN_TYPE_STD);
    ROM_GPIODirModeSet(GPIO_PORTB_BASE, EPI_PORTB_PINS,
                   GPIO_DIR_MODE_HW);
#endif
#if (EPI_PORTD_PINS != 0x00)
    ROM_GPIOPadConfigSet(GPIO_PORTD_BASE, EPI_PORTD_PINS, GPIO_STRENGTH_8MA,
                         GPIO_PIN_TYPE_STD);
    ROM_GPIODirModeSet(GPIO_PORTD_BASE, EPI_PORTD_PINS,
                   GPIO_DIR_MODE_HW);
#endif
#if EPI_PORTH_PINS
    ROM_GPIOPadConfigSet(GPIO_PORTH_BASE, EPI_PORTH_PINS,GPIO_STRENGTH_8MA,
                         GPIO_PIN_TYPE_STD);
    ROM_GPIODirModeSet(GPIO_PORTH_BASE, EPI_PORTH_PINS,
                   GPIO_DIR_MODE_HW);
#endif

//
// If EPI_CONFIG_FPGA is defined, the SIMPLE_PINOUT_SET case will configure
// EPI correctly for the FPGA/Camera daughter board, otherwise the
// configuration will be set for the Flash/SRAM/LCD daughter.
//
#ifndef EPI_CONFIG_FPGA
    //
    // Set the EPI operating mode for the Flash/SRAM/LCD daughter board.
    // The values used here set the EPI to run at the system clock rate and
    // will allow the board memories and LCD interface to be timed correctly
    // as long as the system clock is no higher than 50MHz.
    //
    EPIModeSet(EPI0_BASE, EPI_MODE_HB8);
    EPIDividerSet(EPI0_BASE, 0);
    EPIConfigHB8Set(EPI0_BASE, (EPI_HB8_MODE_ADMUX | EPI_HB8_WRWAIT_1 |
                    EPI_HB8_RDWAIT_1 | EPI_HB8_WORD_ACCESS), 0);
    EPIAddressMapSet(EPI0_BASE, (EPI_ADDR_RAM_SIZE_256MB |
                     EPI_ADDR_RAM_BASE_6) );
#else
    //
    // Set the EPI operating mode for the FPGA/Camera/LCD daughter board.
    // The values used here set the EPI to run at the system clock rate and
    // will allow correct accesses to the FPGA as long as the system clock is
    // 50MHz.
    //
    EPIModeSet(EPI0_BASE, EPI_MODE_GENERAL);
    EPIDividerSet(EPI0_BASE, 1);
    EPIConfigGPModeSet(EPI0_BASE, (EPI_GPMODE_DSIZE_16 | EPI_GPMODE_ASIZE_12 |
                       EPI_GPMODE_WORD_ACCESS | EPI_GPMODE_READWRITE |
                       EPI_GPMODE_READ2CYCLE | EPI_GPMODE_CLKPIN |
                       EPI_GPMODE_RDYEN ), 0, 0);
    EPIAddressMapSet(EPI0_BASE, EPI_ADDR_PER_SIZE_64KB | EPI_ADDR_PER_BASE_A);
#endif
}

//*****************************************************************************
//
//! Configures the device pinout for the development board.
//!
//! This function configures each pin of the device to route the appropriate
//! peripheral signal as required by the design of the development board.
//!
//! \return None.
//
//*****************************************************************************
void
PinoutSet(void)
{
//
// If EPI_CONFIG_FPGA is defined, the SIMPLE_PINOUT_SET case will configure
// EPI correctly for the FPGA/Camera daughter board, otherwise the
// configuration will be set for the Flash/SRAM/LCD daughter.
//
#ifndef EPI_CONFIG_FPGA
    //
    // Hardcode the daughter board type to the Flash/SRAM/LCD board since this
    // is the EPI configuration we set when SIMPLE_PINOUT_SET is defined.
    //
    g_eDaughterType = DAUGHTER_SRAM_FLASH;

#else
    //
    // Hardcode the daughter board type to the FPGA/Camera board since this
    // is the EPI configuration we set when SIMPLE_PINOUT_SET and
    // EPI_CONFIG_FPGA are both defined.
    //
    g_eDaughterType = DAUGHTER_FPGA;
#endif

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
    // To use CAN0, these calls must be changed.  This enables USB
    // functionality instead of CAN. For CAN, use:
    //
    //     GPIOPinConfigure(GPIO_PA6_CAN0RX);
    //     GPIOPinConfigure(GPIO_PA7_CAN0TX);
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);
    GPIOPinConfigure(GPIO_PA6_USB0EPEN);
    GPIOPinConfigure(GPIO_PA7_USB0PFLT);

    //
    // GPIO Port B pins
    //
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    GPIOPinConfigure(GPIO_PB4_EPI0S23);
    GPIOPinConfigure(GPIO_PB5_EPI0S22);
    GPIOPinConfigure(GPIO_PB6_I2S0TXSCK);
    GPIOPinConfigure(GPIO_PB7_NMI);

    //
    // GPIO Port C pins
    //
    GPIOPinConfigure(GPIO_PC4_EPI0S2);
    GPIOPinConfigure(GPIO_PC5_EPI0S3);
    GPIOPinConfigure(GPIO_PC6_EPI0S4);
    GPIOPinConfigure(GPIO_PC7_EPI0S5);

    //
    // GPIO Port D pins.
    //
    GPIOPinConfigure(GPIO_PD0_I2S0RXSCK);
    GPIOPinConfigure(GPIO_PD1_I2S0RXWS);
    GPIOPinConfigure(GPIO_PD2_EPI0S20);
    GPIOPinConfigure(GPIO_PD3_EPI0S21);
    GPIOPinConfigure(GPIO_PD4_I2S0RXSD);
    GPIOPinConfigure(GPIO_PD5_I2S0RXMCLK);

    //
    // GPIO Port E pins
    //
    GPIOPinConfigure(GPIO_PE0_EPI0S8);
    GPIOPinConfigure(GPIO_PE1_EPI0S9);
    GPIOPinConfigure(GPIO_PE2_EPI0S24);
    GPIOPinConfigure(GPIO_PE3_EPI0S25);
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
#ifndef EPI_CONFIG_FPGA
    GPIOPinConfigure(GPIO_PH6_EPI0S26);
#endif
    GPIOPinConfigure(GPIO_PH7_EPI0S27);

#ifdef EPI_CONFIG_FPGA
    //
    // GPIO Port J pins
    //
    // If configuring for the FPGA daughter board, we need to make
    // EPI30 a normal GPIO so that it is available for use as the
    // interrupt line from the FPGA.
    //
    GPIOPinConfigure(GPIO_PJ0_EPI0S16);
    GPIOPinConfigure(GPIO_PJ1_EPI0S17);
    GPIOPinConfigure(GPIO_PJ2_EPI0S18);
    GPIOPinConfigure(GPIO_PJ3_EPI0S19);
    GPIOPinConfigure(GPIO_PJ4_EPI0S28);
    GPIOPinConfigure(GPIO_PJ5_EPI0S29);
#else
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
#endif

    //
    // Configure pins and interface for the EPI-connected devices.
    //
    EPIPinConfigSet();

    //
    // It's somewhat unfortunate that this needs to be here since it breaks
    // the attempt to keep this file completely daughter-board agnostic but
    // the Camera/FPGA daughter board needs to be reset before it can be used
    // and, if we don't do it here, we end up with the problem of deciding how
    // to handle it if using both the camera and display driver for the board,
    // both of which could be used independently, both of which require that
    // the board has been reset but both of which get messed up if the other
    // resets the board after they are initialized.
    //
    if(g_eDaughterType == DAUGHTER_FPGA)
    {
        //
        // Configure the FPGA reset signal.
        //
        ROM_GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, GPIO_PIN_6);

        //
        // Configure the interrupt line from the FPGA.
        //
        ROM_GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_6);

        //
        // Assert the FPGA reset for a while.
        //
        ROM_GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_6, 0);
        SysCtlDelay(10);
        ROM_GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_6, GPIO_PIN_6);

        //
        // Wait 600mS for the device to be completely ready.  This time
        // allows the FPGA to load its image from EEPROM after a power-on-
        // reset.
        //
        SysCtlDelay(ROM_SysCtlClockGet() / 5);

        //
        // Perform a write to the "read only" version register.  This is a
        // special case - the FPGA uses this access to determine whether it is
        // connected to a Tempest rev B or rev C.  The EPI timings are
        // different between these two revisions.
        //
        HWREGH(FPGA_VERSION_REG) = 0;
    }
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
