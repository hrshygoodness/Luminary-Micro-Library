//*****************************************************************************
//
// cfal96x64x16.c - Display driver for the Crystalfontz CFAL9664-F-B1 OLED
//                  display with an SSD1332.  This version uses an SSI
//                  interface to the display controller.
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
// This is part of revision 9453 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup display_api
//! @{
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"

//*****************************************************************************
//
// Defines the SSI and GPIO peripherals that are used for this display.
//
//*****************************************************************************
#define DISPLAY_SSI_PERIPH          SYSCTL_PERIPH_SSI2
#define DISPLAY_SSI_GPIO_PERIPH     SYSCTL_PERIPH_GPIOH
#define DISPLAY_RST_GPIO_PERIPH     SYSCTL_PERIPH_GPIOG

//*****************************************************************************
//
// Defines the GPIO pin configuration macros for the pins that are used for
// the SSI function.
//
//*****************************************************************************
#define DISPLAY_PINCFG_SSICLK       GPIO_PH4_SSI2CLK
#define DISPLAY_PINCFG_SSIFSS       GPIO_PH5_SSI2FSS
#define DISPLAY_PINCFG_SSITX        GPIO_PH7_SSI2TX

//*****************************************************************************
//
// Defines the port and pins for the SSI peripheral.
//
//*****************************************************************************
#define DISPLAY_SSI_PORT            GPIO_PORTH_BASE
#define DISPLAY_SSI_PINS            (GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7)

//*****************************************************************************
//
// Defines the port and pins for the display voltage enable signal.
//
//*****************************************************************************
#define DISPLAY_ENV_PORT            GPIO_PORTG_BASE
#define DISPLAY_ENV_PIN             GPIO_PIN_0

//*****************************************************************************
//
// Defines the port and pins for the display reset signal.
//
//*****************************************************************************
#define DISPLAY_RST_PORT            GPIO_PORTG_BASE
#define DISPLAY_RST_PIN             GPIO_PIN_1

//*****************************************************************************
//
// Defines the port and pins for the display Data/Command (D/C) signal.
//
//*****************************************************************************
#define DISPLAY_D_C_PORT            GPIO_PORTH_BASE
#define DISPLAY_D_C_PIN             GPIO_PIN_6

//*****************************************************************************
//
// Defines the SSI peripheral used and the data speed.
//
//*****************************************************************************
#define DISPLAY_SSI_BASE            SSI2_BASE // SSI2
#define DISPLAY_SSI_CLOCK           4000000

//*****************************************************************************
//
// An array that holds a set of commands that are sent to the display when
// it is initialized.
//
//*****************************************************************************
static
unsigned char g_ucDisplayInitCommands[] =
{
//    0xAE,         // display off
    0x87, 0x07,     // master control current 7/16
    0x81, 0xA0,     // contrast A control
    0x82, 0x60,     // contrast B control
    0x83, 0xB0,     // contrast C control
    0xA0, 0x20,//00 // remap and data format - use 8-bit color mode
    0xBB, 0x1F,     // Vpa
    0xBC, 0x1F,     // Vpb
    0xBD, 0x1F,     // Vpc
//    0xAD, 0x8E,   // internal Vp, external supply
    0x26, 0x01,     // rectangle fill enabled
    0xAF            // display on
};
#define NUM_INIT_BYTES sizeof(g_ucDisplayInitCommands)

//*****************************************************************************
//
// Translates a 24-bit RGB color to a display driver-specific color.
//
// \param c is the 24-bit RGB color.  The least-significant byte is the blue
// channel, the next byte is the green channel, and the third byte is the red
// channel.
//
// This macro translates a 24-bit RGB color into a value that can be written
// into the display's frame buffer in order to reproduce that color, or the
// closest possible approximation of that color.
//
// \return Returns the display-driver specific color.
//
// 24-bit format: XXXX XXXX RRRR RRRR GGGG GGGG BBBB BBBB
// 16-bit format: ---- ---- ---- ---- RRRR RGGG GGGB BBBB
//  8-bit format: ---- ---- ---- ---- ---- ---- RRRG GGBB
//
//
//*****************************************************************************
#define DPYCOLORTRANSLATE16(c)  ((((c) & 0x00f80000) >> 8) |                  \
                                 (((c) & 0x0000fc00) >> 5) |                  \
                                 (((c) & 0x000000f8) >> 3))
#define DPYCOLORTRANSLATE8(c)   ((((c) & 0x00e00000) >> 16) |                 \
                                 (((c) & 0x0000e000) >> 11) |                 \
                                 (((c) & 0x000000c0) >> 6))
#define DPYCOLORTRANSLATE DPYCOLORTRANSLATE8

//*****************************************************************************
//
//! Write a set of command bytes to the display controller.
//
//! \param pcCmd is a pointer to a set of command bytes.
//! \param ulCount is the count of command bytes.
//!
//! This function provides a way to send multiple command bytes to the display
//! controller.  It can be used for single commands, or multiple commands
//! chained together in a buffer.  It will wait for any previous operation to
//! finish, and then copy all the command bytes to the controller.  It will
//! not return until the last command byte has been written to the SSI FIFO,
//! but data could still be shifting out to the display controller when this
//! function returns.
//!
//! \return None.
//
//*****************************************************************************
static void
CFAL96x64x16WriteCommand(const unsigned char *pcCmd, unsigned long ulCount)
{
    //
    // Wait for any previous SSI operation to finish.
    //
    while(ROM_SSIBusy(DISPLAY_SSI_BASE))
    {
    }

    //
    // Set the D/C pin low to indicate command
    //
    ROM_GPIOPinWrite(DISPLAY_D_C_PORT, DISPLAY_D_C_PIN, 0);

    //
    // Send all the command bytes to the display
    //
    while(ulCount--)
    {
        ROM_SSIDataPut(DISPLAY_SSI_BASE, *pcCmd);
        pcCmd++;
    }
}

//*****************************************************************************
//
//! Write a set of data bytes to the display controller.
//
//! \param pcData is a pointer to a set of data bytes, containing pixel data.
//! \param ulCount is the count of command bytes.
//!
//! This function provides a way to send a set of pixel data to the display.
//! The data will draw pixels according to whatever the most recent col, row
//! settings are for the display.  It will wait for any previous operation to
//! finish, and then copy all the data bytes to the controller.  It will
//! not return until the last data byte has been written to the SSI FIFO,
//! but data could still be shifting out to the display controller when this
//! function returns.
//!
//! \return None.
//
//*****************************************************************************
static void
CFAL96x64x16WriteData(const unsigned char *pcData, unsigned long ulCount)
{
    //
    // Wait for any previous SSI operation to finish.
    //
    while(ROM_SSIBusy(DISPLAY_SSI_BASE))
    {
    }

    //
    // Set the D/C pin high to indicate data
    //
    ROM_GPIOPinWrite(DISPLAY_D_C_PORT, DISPLAY_D_C_PIN, DISPLAY_D_C_PIN);

    //
    // Send all the data bytes to the display
    //
    while(ulCount--)
    {
        ROM_SSIDataPut(DISPLAY_SSI_BASE, *pcData);
        pcData++;
    }
}

//*****************************************************************************
//
//! Draws a pixel on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the pixel.
//! \param lY is the Y coordinate of the pixel.
//! \param ulValue is the color of the pixel.
//!
//! This function sets the given pixel to a particular color.  The coordinates
//! of the pixel are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
CFAL96x64x16PixelDraw(void *pvDisplayData, long lX, long lY,
                      unsigned long ulValue)
{
    unsigned char ucCmd[8];

    //
    // Load column command, start and end column
    //
    ucCmd[0] = 0x15;
    ucCmd[1] = (unsigned char)lX;
    ucCmd[2] = (unsigned char)lX;

    //
    // Load row command, start and end row
    //
    ucCmd[3] = 0x75;
    ucCmd[4] = (unsigned char)lY;
    ucCmd[5] = (unsigned char)lY;

    //
    // Send the column, row commands to the display
    //
    CFAL96x64x16WriteCommand(ucCmd, 6);

    //
    // Send the data value representing the pixel to the display
    //
    CFAL96x64x16WriteData((unsigned char *)&ulValue, 1);
}

//*****************************************************************************
//
//! Draws a horizontal sequence of pixels on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the first pixel.
//! \param lY is the Y coordinate of the first pixel.
//! \param lX0 is sub-pixel offset within the pixel data, which is valid for 1
//! or 4 bit per pixel formats.
//! \param lCount is the number of pixels to draw.
//! \param lBPP is the number of bits per pixel; must be 1, 4, or 8.
//! \param pucData is a pointer to the pixel data.  For 1 and 4 bit per pixel
//! formats, the most significant bit(s) represent the left-most pixel.
//! \param pucPalette is a pointer to the palette used to draw the pixels.
//!
//! This function draws a horizontal sequence of pixels on the screen, using
//! the supplied palette.  For 1 bit per pixel format, the palette contains
//! pre-translated colors; for 4 and 8 bit per pixel formats, the palette
//! contains 24-bit RGB values that must be translated before being written to
//! the display.
//!
//! \return None.
//
//*****************************************************************************
static void
CFAL96x64x16PixelDrawMultiple(void *pvDisplayData, long lX, long lY, long lX0,
                              long lCount, long lBPP,
                              const unsigned char *pucData,
                              const unsigned char *pucPalette)
{
    unsigned long ulByte;
    unsigned char ucCmd[8];

    //
    // Load column command.  Use the specified X for the start and just set
    // the end to the rightmost column since we dont know where the data ends.
    //
    ucCmd[0] = 0x15;
    ucCmd[1] = (unsigned char)lX;
    ucCmd[2] = 95;

    //
    // Load row command.  Use the specified Y for the start row and just set
    // the end row to the bottom row since we dont know where the data ends.
    //
    ucCmd[3] = 0x75;
    ucCmd[4] = (unsigned char)lY;
    ucCmd[5] = 63;

    //
    // Send the column, row commands to the display
    //
    CFAL96x64x16WriteCommand(ucCmd, 6);

    //
    // Determine how to interpret the pixel data based on the number of bits
    // per pixel.
    //
    switch(lBPP)
    {
        //
        // The pixel data is in 1 bit per pixel format.
        //
        case 1:
        {
            //
            // Loop while there are more pixels to draw.
            //
            while(lCount)
            {
                //
                // Get the next byte of image data.
                //
                ulByte = *pucData++;

                //
                // Loop through the pixels in this byte of image data.
                //
                for(; (lX0 < 8) && lCount; lX0++, lCount--)
                {
                    //
                    // Draw this pixel in the appropriate color.
                    //
                    unsigned char ucBPP = ((unsigned long *)pucPalette)
                                            [(ulByte >> (7 - lX0)) & 1];
                    CFAL96x64x16WriteData(&ucBPP, 1);
                }

                //
                // Start at the beginning of the next byte of image data.
                //
                lX0 = 0;
            }

            //
            // The image data has been drawn.
            //
            break;
        }

        //
        // The pixel data is in 4 bit per pixel format.
        //
        case 4:
        {
            //
            // Loop while there are more pixels to draw.  "Duff's device" is
            // used to jump into the middle of the loop if the first nibble of
            // the pixel data should not be used.  Duff's device makes use of
            // the fact that a case statement is legal anywhere within a
            // sub-block of a switch statement.  See
            // http://en.wikipedia.org/wiki/Duff's_device for detailed
            // information about Duff's device.
            //
            switch(lX0 & 1)
            {
                case 0:
                    while(lCount)
                    {
                        unsigned char ucColor;

                        //
                        // Get the upper nibble of the next byte of pixel data
                        // and extract the corresponding entry from the
                        // palette.
                        //
                        ulByte = (*pucData >> 4) * 3;
                        ulByte = (*(unsigned long *)(pucPalette + ulByte) &
                                  0x00ffffff);

                        //
                        // Translate this palette entry and write it to the
                        // screen.
                        //
                        ucColor = DPYCOLORTRANSLATE(ulByte);
                        CFAL96x64x16WriteData(&ucColor, 1);

                        //
                        // Decrement the count of pixels to draw.
                        //
                        lCount--;

                        //
                        // See if there is another pixel to draw.
                        //
                        if(lCount)
                        {
                case 1:
                            //
                            // Get the lower nibble of the next byte of pixel
                            // data and extract the corresponding entry from
                            // the palette.
                            //
                            ulByte = (*pucData++ & 15) * 3;
                            ulByte = (*(unsigned long *)(pucPalette + ulByte) &
                                      0x00ffffff);

                            //
                            // Translate this palette entry and write it to the
                            // screen.
                            //
                            ucColor = DPYCOLORTRANSLATE(ulByte);
                            CFAL96x64x16WriteData(&ucColor, 1);

                            //
                            // Decrement the count of pixels to draw.
                            //
                            lCount--;
                        }
                    }
            }

            //
            // The image data has been drawn.
            //
            break;
        }

        //
        // The pixel data is in 8 bit per pixel format.
        //
        case 8:
        {
            //
            // Loop while there are more pixels to draw.
            //
            while(lCount--)
            {
                unsigned char ucColor;

                //
                // Get the next byte of pixel data and extract the
                // corresponding entry from the palette.
                //
                ulByte = *pucData++ * 3;
                ulByte = *(unsigned long *)(pucPalette + ulByte) & 0x00ffffff;

                //
                // Translate this palette entry and write it to the screen.
                //
                ucColor = DPYCOLORTRANSLATE(ulByte);
                CFAL96x64x16WriteData(&ucColor, 1);
            }

            //
            // The image data has been drawn.
            //
            break;
        }
    }
}

//*****************************************************************************
//
//! Draws a horizontal line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX1 is the X coordinate of the start of the line.
//! \param lX2 is the X coordinate of the end of the line.
//! \param lY is the Y coordinate of the line.
//! \param ulValue is the color of the line.
//!
//! This function draws a horizontal line on the display.  The coordinates of
//! the line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
CFAL96x64x16LineDrawH(void *pvDisplayData, long lX1, long lX2, long lY,
                      unsigned long ulValue)
{
    unsigned char ucLineBuf[16];
    unsigned int uIdx;

    //
    // Send command for starting row and column
    //
    ucLineBuf[0] = 0x15;
    ucLineBuf[1] = lX1 < lX2 ? lX1 : lX2;
    ucLineBuf[2] = 95;
    ucLineBuf[3] = 0x75;
    ucLineBuf[4] = lY;
    ucLineBuf[5] = 63;
    CFAL96x64x16WriteCommand(ucLineBuf, 6);

    //
    // Use buffer of pixels to draw line, so multiple bytes can be sent at
    // one time.  Fill the buffer with the line color.
    //
    for(uIdx = 0; uIdx < sizeof(ucLineBuf); uIdx++)
    {
        ucLineBuf[uIdx] = ulValue;
    }

    uIdx = (lX1 < lX2) ? (lX2 - lX1) : (lX1 - lX2);
    uIdx += 1;
    while(uIdx)
    {
        CFAL96x64x16WriteData(ucLineBuf, (uIdx < 16) ? uIdx : 16);
        uIdx -= (uIdx < 16) ? uIdx : 16;
    }
}

//*****************************************************************************
//
//! Draws a vertical line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the line.
//! \param lY1 is the Y coordinate of the start of the line.
//! \param lY2 is the Y coordinate of the end of the line.
//! \param ulValue is the color of the line.
//!
//! This function draws a vertical line on the display.  The coordinates of the
//! line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
CFAL96x64x16LineDrawV(void *pvDisplayData, long lX, long lY1, long lY2,
                      unsigned long ulValue)
{
    unsigned char ucLineBuf[16];
    unsigned int uIdx;

    //
    // Send command for starting row and column.  Also, set vertical
    // address increment.
    //
    ucLineBuf[0] = 0x15;
    ucLineBuf[1] = lX;
    ucLineBuf[2] = 95;
    ucLineBuf[3] = 0x75;
    ucLineBuf[4] = lY1 < lY2 ? lY1 : lY2;
    ucLineBuf[5] = 63;
    ucLineBuf[6] = 0xA0;
    ucLineBuf[7] = 0x21;
    CFAL96x64x16WriteCommand(ucLineBuf, 8);

    //
    // Use buffer of pixels to draw line, so multiple bytes can be sent at
    // one time.  Fill the buffer with the line color.
    //
    for(uIdx = 0; uIdx < sizeof(ucLineBuf); uIdx++)
    {
        ucLineBuf[uIdx] = ulValue;
    }

    uIdx = (lY1 < lY2) ? (lY2 - lY1) : (lY1 - lY2);
    uIdx += 1;
    while(uIdx)
    {
        CFAL96x64x16WriteData(ucLineBuf, (uIdx < 16) ? uIdx : 16);
        uIdx -= (uIdx < 16) ? uIdx : 16;
    }

    //
    // Restore horizontal address increment
    //
    ucLineBuf[0] = 0xA0;
    ucLineBuf[1] = 0x20;
    CFAL96x64x16WriteCommand(ucLineBuf, 2);
}

//*****************************************************************************
//
//! Fills a rectangle.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param pRect is a pointer to the structure describing the rectangle.
//! \param ulValue is the color of the rectangle.
//!
//! This function fills a rectangle on the display.  The coordinates of the
//! rectangle are assumed to be within the extents of the display, and the
//! rectangle specification is fully inclusive (in other words, both sXMin and
//! sXMax are drawn, along with sYMin and sYMax).
//!
//! \return None.
//
//*****************************************************************************
static void
CFAL96x64x16RectFill(void *pvDisplayData, const tRectangle *pRect,
                     unsigned long ulValue)
{
    unsigned int uY;

    for(uY = pRect->sYMin; uY <= pRect->sYMax; uY++)
    {
        CFAL96x64x16LineDrawH(0, pRect->sXMin, pRect->sXMax, uY, ulValue);
    }
}

//*****************************************************************************
//
//! Translates a 24-bit RGB color to a display driver-specific color.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param ulValue is the 24-bit RGB color.  The least-significant byte is the
//! blue channel, the next byte is the green channel, and the third byte is the
//! red channel.
//!
//! This function translates a 24-bit RGB color into a value that can be
//! written into the display's frame buffer in order to reproduce that color,
//! or the closest possible approximation of that color.
//!
//! \return Returns the display-driver specific color.
//
//*****************************************************************************
static unsigned long
CFAL96x64x16ColorTranslate(void *pvDisplayData, unsigned long ulValue)
{
    //
    // Translate from a 24-bit RGB color to a 3-3-2 RGB color.
    //
    return(DPYCOLORTRANSLATE(ulValue));
}

//*****************************************************************************
//
//! Flushes any cached drawing operations.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//!
//! This functions flushes any cached drawing operations to the display.  This
//! is useful when a local frame buffer is used for drawing operations, and the
//! flush would copy the local frame buffer to the display.  Since no memory
//! based frame buffer is used for this driver, the flush is a no operation.
//!
//! \return None.
//
//*****************************************************************************
static void
CFAL96x64x16Flush(void *pvDisplayData)
{
    //
    // There is nothing to be done.
    //
}

//*****************************************************************************
//
//! The display structure that describes the driver for the Crystalfontz
//! CFAL9664-F-B1 OLED panel with SSD 1332 controller.
//
//*****************************************************************************
const tDisplay g_sCFAL96x64x16 =
{
    sizeof(tDisplay),
    0,
    96,
    64,
    CFAL96x64x16PixelDraw,
    CFAL96x64x16PixelDrawMultiple,
    CFAL96x64x16LineDrawH,
    CFAL96x64x16LineDrawV,
    CFAL96x64x16RectFill,
    CFAL96x64x16ColorTranslate,
    CFAL96x64x16Flush
};

//*****************************************************************************
//
//! Initializes the display driver.
//!
//! This function initializes the SSD1332 display controller on the panel,
//! preparing it to display data.
//!
//! \return None.
//
//*****************************************************************************
void
CFAL96x64x16Init(void)
{
    tRectangle sRect;

    //
    // Enable the peripherals used by this driver
    //
    ROM_SysCtlPeripheralEnable(DISPLAY_SSI_PERIPH);
    ROM_SysCtlPeripheralEnable(DISPLAY_SSI_GPIO_PERIPH);
    ROM_SysCtlPeripheralEnable(DISPLAY_RST_GPIO_PERIPH);

    //
    // Select the SSI function for the appropriate pins
    //
    ROM_GPIOPinConfigure(DISPLAY_PINCFG_SSICLK);
    ROM_GPIOPinConfigure(DISPLAY_PINCFG_SSIFSS);
    ROM_GPIOPinConfigure(DISPLAY_PINCFG_SSITX);


    //
    // Configure the pins for the SSI function
    //
    ROM_GPIOPinTypeSSI(DISPLAY_SSI_PORT, DISPLAY_SSI_PINS);

    //
    // Configure display control pins as GPIO output
    //
    ROM_GPIOPinTypeGPIOOutput(DISPLAY_RST_PORT, DISPLAY_RST_PIN);
    ROM_GPIOPinTypeGPIOOutput(DISPLAY_ENV_PORT, DISPLAY_ENV_PIN);
    ROM_GPIOPinTypeGPIOOutput(DISPLAY_D_C_PORT, DISPLAY_D_C_PIN);

    //
    // Reset pin high, power off
    //
    ROM_GPIOPinWrite(DISPLAY_RST_PORT, DISPLAY_RST_PIN, DISPLAY_RST_PIN);
    ROM_GPIOPinWrite(DISPLAY_ENV_PORT, DISPLAY_ENV_PIN, 0);
    ROM_SysCtlDelay(1000);

    //
    // Drive the reset pin low while we do other stuff
    //
    ROM_GPIOPinWrite(DISPLAY_RST_PORT, DISPLAY_RST_PIN, 0);

    //
    // Configure the SSI port
    //
    ROM_SSIDisable(DISPLAY_SSI_BASE);
    ROM_SSIConfigSetExpClk(DISPLAY_SSI_BASE, ROM_SysCtlClockGet(),
                           SSI_FRF_MOTO_MODE_3, SSI_MODE_MASTER,
                           DISPLAY_SSI_CLOCK, 8);
    ROM_SSIEnable(DISPLAY_SSI_BASE);

    //
    // Take the display out of reset
    //
    ROM_SysCtlDelay(1000);
    ROM_GPIOPinWrite(DISPLAY_RST_PORT, DISPLAY_RST_PIN, DISPLAY_RST_PIN);
    ROM_SysCtlDelay(1000);

    //
    // Enable display power supply
    //
    ROM_GPIOPinWrite(DISPLAY_ENV_PORT, DISPLAY_ENV_PIN, DISPLAY_ENV_PIN);
    ROM_SysCtlDelay(1000);

    //
    // Send the initial configuration command bytes to the display
    //
    CFAL96x64x16WriteCommand(g_ucDisplayInitCommands,
                             sizeof(g_ucDisplayInitCommands));
    ROM_SysCtlDelay(1000);

    //
    // Fill the entire display with a black rectangle, to clear it.
    //
    sRect.sXMin = 0;
    sRect.sXMax = 95;
    sRect.sYMin = 0;
    sRect.sYMax = 63;
    CFAL96x64x16RectFill(0, &sRect, 0);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
