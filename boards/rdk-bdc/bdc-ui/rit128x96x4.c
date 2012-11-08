//*****************************************************************************
//
// rit128x96x4.c - Graphics library driver for the RIT 128x96x4 graphical OLED
//                 display.
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
// This is part of revision 9453 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "rit128x96x4.h"

//*****************************************************************************
//
// Macros that define the peripheral, port, and pin used for the OLEDDC
// panel control signal.
//
//*****************************************************************************
#define SYSCTL_PERIPH_GPIO_OLEDDC   SYSCTL_PERIPH_GPIOC
#define GPIO_OLEDDC_BASE            GPIO_PORTC_BASE
#define GPIO_OLEDDC_PIN             GPIO_PIN_7
#define GPIO_OLEDEN_PIN             GPIO_PIN_6

//*****************************************************************************
//
// Flags to indicate the state of the SSI interface to the display.
//
//*****************************************************************************
static volatile unsigned long g_ulSSIFlags;
#define FLAG_SSI_ENABLED        0
#define FLAG_DC_HIGH            1

//*****************************************************************************
//
// Memory that is used as the local frame buffer.
//
//*****************************************************************************
static unsigned char g_pucBuffer[GrOffScreen4BPPSize(128, 96)];

//*****************************************************************************
//
// The graphics library display structure for the OLED display.
//
//*****************************************************************************
tDisplay g_sRIT128x96x4Display;

//*****************************************************************************
//
// The command sequence to prepare the OLED display to receive and display the
// frame buffer contents.
//
//*****************************************************************************
static const unsigned char g_pucFlushCmd[] =
{
    0x15, 0x00, 0x3f, 0x75, 0x00, 0x5f, 0xa0, 0x52
};

//*****************************************************************************
//
// The sequence of commands used to initialize the SSD1329 controller.  Each
// command is described as follows:  there is a byte specifying the number of
// bytes in the command sequence, followed by that many bytes of command data.
// Note:  This initialization sequence is derived from RIT App Note for
// the P14201.  Values used are from the RIT app note, except where noted.
//
//*****************************************************************************
static const unsigned char g_pucRIT128x96x4Init[] =
{
    //
    // Unlock commands
    //
    3, 0xFD, 0x12, 0xe3,

    //
    // Display off
    //
    2, 0xAE, 0xe3,

    //
    // Icon off
    //
    3, 0x94, 0, 0xe3,

    //
    // Multiplex ratio
    //
    3, 0xA8, 95, 0xe3,

    //
    // Contrast
    //
    3, 0x81, 0xb7, 0xe3,

    //
    // Pre-charge current
    //
    3, 0x82, 0x3f, 0xe3,

    //
    // Display Re-map
    //
    3, 0xA0, 0x52, 0xe3,

    //
    // Display Start Line
    //
    3, 0xA1, 0, 0xe3,

    //
    // Display Offset
    //
    3, 0xA2, 0x00, 0xe3,

    //
    // Display Mode Normal
    //
    2, 0xA4, 0xe3,

    //
    // Phase Length
    //
    3, 0xB1, 0x11, 0xe3,

    //
    // Frame frequency
    //
    3, 0xB2, 0x23, 0xe3,

    //
    // Front Clock Divider
    //
    3, 0xB3, 0xe2, 0xe3,

    //
    // Set gray scale table.  App note uses default command:
    // 2, 0xB7, 0xe3
    // This gray scale attempts some gamma correction to reduce the
    // the brightness of the low levels.
    //
    17, 0xB8, 1, 2, 3, 4, 5, 6, 8, 10, 12, 14, 16, 19, 22, 26, 30, 0xe3,

    //
    // Second pre-charge period.  App note uses value 0x04.
    //
    3, 0xBB, 0x01, 0xe3,

    //
    // Pre-charge voltage
    //
    3, 0xBC, 0x3f, 0xe3,

    //
    // Display ON
    //
    2, 0xAF, 0xe3,
};

//*****************************************************************************
//
// Writes a sequence of command bytes to the SSD1329 controller.
//
//*****************************************************************************
static void
RITWriteCommand(const unsigned char *pucBuffer, unsigned long ulCount)
{
    //
    // Return if SSI port is not enabled for RIT display.
    //
    if(!HWREGBITW(&g_ulSSIFlags, FLAG_SSI_ENABLED))
    {
        return;
    }

    //
    // See if data mode is enabled.
    //
    if(HWREGBITW(&g_ulSSIFlags, FLAG_DC_HIGH))
    {
        //
        // Wait until the SSI is not busy, meaning that all previous data has
        // been transmitted.
        //
        while(SSIBusy(SSI0_BASE))
        {
        }

        //
        // Clear the command/control bit to enable command mode.
        //
        GPIOPinWrite(GPIO_OLEDDC_BASE, GPIO_OLEDDC_PIN, 0);
        HWREGBITW(&g_ulSSIFlags, FLAG_DC_HIGH) = 0;
    }

    //
    // Loop while there are more bytes left to be transferred.
    //
    while(ulCount != 0)
    {
        //
        // Write the next byte to the controller.
        //
        SSIDataPut(SSI0_BASE, *pucBuffer++);

        //
        // Decrement the BYTE counter.
        //
        ulCount--;
    }
}

//*****************************************************************************
//
// Writes a sequence of data bytes to the SSD1329 controller.
//
//*****************************************************************************
static void
RITWriteData(const unsigned char *pucBuffer, unsigned long ulCount)
{
    //
    // Return if SSI port is not enabled for RIT display.
    //
    if(!HWREGBITW(&g_ulSSIFlags, FLAG_SSI_ENABLED))
    {
        return;
    }

    //
    // See if command mode is enabled.
    //
    if(!HWREGBITW(&g_ulSSIFlags, FLAG_DC_HIGH))
    {
        //
        // Wait until the SSI is not busy, meaning that all previous commands
        // have been transmitted.
        //
        while(SSIBusy(SSI0_BASE))
        {
        }

        //
        // Set the command/control bit to enable data mode.
        //
        GPIOPinWrite(GPIO_OLEDDC_BASE, GPIO_OLEDDC_PIN, GPIO_OLEDDC_PIN);
        HWREGBITW(&g_ulSSIFlags, FLAG_DC_HIGH) = 1;
    }

    //
    // Loop while there are more bytes left to be transferred.
    //
    while(ulCount != 0)
    {
        //
        // Write the next byte to the controller.
        //
        SSIDataPut(SSI0_BASE, *pucBuffer++);

        //
        // Decrement the BYTE counter.
        //
        ulCount--;
    }
}

//*****************************************************************************
//
// Flushes the off-screen frame buffer to the OLED display.
//
//*****************************************************************************
void
RIT128x96x4Flush(void *pvDisplayData)
{
    //
    // Setup the window to cover the entire screen.
    //
    RITWriteCommand(g_pucFlushCmd, sizeof(g_pucFlushCmd));

    //
    // Write the local display buffer to the screen.
    //
    RITWriteData(g_pucBuffer + GrOffScreen4BPPSize(128, 96) - (128 * 96 / 2),
                 128 * 96 / 2);
}

//*****************************************************************************
//
// Enable the OLED display driver.
//
//*****************************************************************************
void
RIT128x96x4Enable(unsigned long ulFrequency)
{
    //
    // Disable the SSI port.
    //
    SSIDisable(SSI0_BASE);

    //
    // Configure the SSI0 port for master mode.
    //
    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_3,
                       SSI_MODE_MASTER, ulFrequency, 8);

    //
    // (Re)Enable SSI control of the FSS pin.
    //
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_3);
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_STRENGTH_8MA,
                     GPIO_PIN_TYPE_STD_WPU);

    //
    // Enable the SSI port.
    //
    SSIEnable(SSI0_BASE);

    //
    // Indicate that the RIT driver can use the SSI Port.
    //
    HWREGBITW(&g_ulSSIFlags, FLAG_SSI_ENABLED) = 1;
}

//*****************************************************************************
//
// Disable the OLED display driver.
//
//*****************************************************************************
void
RIT128x96x4Disable(void)
{
    unsigned long ulTemp;

    //
    // Indicate that the RIT driver can no longer use the SSI Port.
    //
    HWREGBITW(&g_ulSSIFlags, FLAG_SSI_ENABLED) = 0;

    //
    // Drain the receive fifo.
    //
    while(SSIDataGetNonBlocking(SSI0_BASE, &ulTemp) != 0)
    {
    }

    //
    // Disable the SSI port.
    //
    SSIDisable(SSI0_BASE);

    //
    // Disable SSI control of the FSS pin.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_3);
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_STRENGTH_8MA,
                     GPIO_PIN_TYPE_STD_WPU);
    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3);
}

//*****************************************************************************
//
// Initialize the OLED display.
//
//*****************************************************************************
void
RIT128x96x4Init(unsigned long ulFrequency)
{
    unsigned long ulIdx;

    //
    // Enable the SSI0 and GPIO port blocks as they are needed by this driver.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIO_OLEDDC);

    //
    // Configure the SSI0CLK and SSIOTX pins for SSI operation.
    //
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5);
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5,
                     GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);

    //
    // Configure the GPIO port pin used as a D/Cn signal for OLED device,
    // and the port pin used to enable power to the OLED panel.
    //
    GPIOPinTypeGPIOOutput(GPIO_OLEDDC_BASE, GPIO_OLEDDC_PIN | GPIO_OLEDEN_PIN);
    GPIOPadConfigSet(GPIO_OLEDDC_BASE, GPIO_OLEDDC_PIN | GPIO_OLEDEN_PIN,
                     GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
    GPIOPinWrite(GPIO_OLEDDC_BASE, GPIO_OLEDDC_PIN | GPIO_OLEDEN_PIN,
                 GPIO_OLEDDC_PIN | GPIO_OLEDEN_PIN);
    HWREGBITW(&g_ulSSIFlags, FLAG_DC_HIGH) = 1;

    //
    // Configure and enable the SSI0 port for master mode.
    //
    RIT128x96x4Enable(ulFrequency);

    //
    // Initialize the SSD1329 controller.  Loop through the initialization
    // sequence array, sending each command "string" to the controller.
    //
    for(ulIdx = 0; ulIdx < sizeof(g_pucRIT128x96x4Init);
        ulIdx += g_pucRIT128x96x4Init[ulIdx] + 1)
    {
        //
        // Send this command.
        //
        RITWriteCommand(g_pucRIT128x96x4Init + ulIdx + 1,
                        g_pucRIT128x96x4Init[ulIdx] - 1);
    }

    GrOffScreen4BPPInit(&g_sRIT128x96x4Display, g_pucBuffer, 128, 96);
    for(ulIdx = 0; ulIdx < 0x01000000; ulIdx += 0x00111111)
    {
        GrOffScreen4BPPPaletteSet(&g_sRIT128x96x4Display, &ulIdx, ulIdx & 15,
                                  1);
    }
    g_sRIT128x96x4Display.pfnFlush = RIT128x96x4Flush;
}

//*****************************************************************************
//
// Turns on the OLED display.
//
//*****************************************************************************
void
RIT128x96x4DisplayOn(void)
{
    unsigned long ulIdx;

    //
    // Initialize the SSD1329 controller.  Loop through the initialization
    // sequence array, sending each command "string" to the controller.
    //
    for(ulIdx = 0; ulIdx < sizeof(g_pucRIT128x96x4Init);
        ulIdx += g_pucRIT128x96x4Init[ulIdx] + 1)
    {
        //
        // Send this command.
        //
        RITWriteCommand(g_pucRIT128x96x4Init + ulIdx + 1,
                        g_pucRIT128x96x4Init[ulIdx] - 1);
    }
}

//*****************************************************************************
//
// Turns off the OLED display.
//
//*****************************************************************************
void
RIT128x96x4DisplayOff(void)
{
    static const unsigned char pucCommand1[] =
    {
        0xAE, 0xe3
    };

    //
    // Put the display to sleep.
    //
    RITWriteCommand(pucCommand1, sizeof(pucCommand1));
}
