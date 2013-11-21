//*****************************************************************************
//
// display96x16x1.c - Driver for the 96x16 monochrome graphical OLED
//                    displays used on the ek-evalbot board.
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

//*****************************************************************************
//
//! \addtogroup display_api
//! @{
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "drivers/display96x16x1.h"

//*****************************************************************************
//
// The I2C slave address of the SSD controllers on the OLED displays.
//
//*****************************************************************************
#define SSD_ADDR            0x3c

//*****************************************************************************
//
// A 5x7 font (in a 6x8 cell, where the sixth column is omitted from this
// table) for displaying text on the OLED display.  The data is organized as
// bytes from the left column to the right column, with each byte containing
// the top row in the LSB and the bottom row in the MSB.
//
//*****************************************************************************
static const unsigned char g_pucFont[95][5] =
{
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, // " "
    { 0x00, 0x00, 0x4f, 0x00, 0x00 }, // !
    { 0x00, 0x07, 0x00, 0x07, 0x00 }, // "
    { 0x14, 0x7f, 0x14, 0x7f, 0x14 }, // #
    { 0x24, 0x2a, 0x7f, 0x2a, 0x12 }, // $
    { 0x23, 0x13, 0x08, 0x64, 0x62 }, // %
    { 0x36, 0x49, 0x55, 0x22, 0x50 }, // &
    { 0x00, 0x05, 0x03, 0x00, 0x00 }, // '
    { 0x00, 0x1c, 0x22, 0x41, 0x00 }, // (
    { 0x00, 0x41, 0x22, 0x1c, 0x00 }, // )
    { 0x14, 0x08, 0x3e, 0x08, 0x14 }, // *
    { 0x08, 0x08, 0x3e, 0x08, 0x08 }, // +
    { 0x00, 0x50, 0x30, 0x00, 0x00 }, // ,
    { 0x08, 0x08, 0x08, 0x08, 0x08 }, // -
    { 0x00, 0x60, 0x60, 0x00, 0x00 }, // .
    { 0x20, 0x10, 0x08, 0x04, 0x02 }, // /
    { 0x3e, 0x51, 0x49, 0x45, 0x3e }, // 0
    { 0x00, 0x42, 0x7f, 0x40, 0x00 }, // 1
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, // 2
    { 0x21, 0x41, 0x45, 0x4b, 0x31 }, // 3
    { 0x18, 0x14, 0x12, 0x7f, 0x10 }, // 4
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, // 5
    { 0x3c, 0x4a, 0x49, 0x49, 0x30 }, // 6
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, // 7
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, // 8
    { 0x06, 0x49, 0x49, 0x29, 0x1e }, // 9
    { 0x00, 0x36, 0x36, 0x00, 0x00 }, // :
    { 0x00, 0x56, 0x36, 0x00, 0x00 }, // ;
    { 0x08, 0x14, 0x22, 0x41, 0x00 }, // <
    { 0x14, 0x14, 0x14, 0x14, 0x14 }, // =
    { 0x00, 0x41, 0x22, 0x14, 0x08 }, // >
    { 0x02, 0x01, 0x51, 0x09, 0x06 }, // ?
    { 0x32, 0x49, 0x79, 0x41, 0x3e }, // @
    { 0x7e, 0x11, 0x11, 0x11, 0x7e }, // A
    { 0x7f, 0x49, 0x49, 0x49, 0x36 }, // B
    { 0x3e, 0x41, 0x41, 0x41, 0x22 }, // C
    { 0x7f, 0x41, 0x41, 0x22, 0x1c }, // D
    { 0x7f, 0x49, 0x49, 0x49, 0x41 }, // E
    { 0x7f, 0x09, 0x09, 0x09, 0x01 }, // F
    { 0x3e, 0x41, 0x49, 0x49, 0x7a }, // G
    { 0x7f, 0x08, 0x08, 0x08, 0x7f }, // H
    { 0x00, 0x41, 0x7f, 0x41, 0x00 }, // I
    { 0x20, 0x40, 0x41, 0x3f, 0x01 }, // J
    { 0x7f, 0x08, 0x14, 0x22, 0x41 }, // K
    { 0x7f, 0x40, 0x40, 0x40, 0x40 }, // L
    { 0x7f, 0x02, 0x0c, 0x02, 0x7f }, // M
    { 0x7f, 0x04, 0x08, 0x10, 0x7f }, // N
    { 0x3e, 0x41, 0x41, 0x41, 0x3e }, // O
    { 0x7f, 0x09, 0x09, 0x09, 0x06 }, // P
    { 0x3e, 0x41, 0x51, 0x21, 0x5e }, // Q
    { 0x7f, 0x09, 0x19, 0x29, 0x46 }, // R
    { 0x46, 0x49, 0x49, 0x49, 0x31 }, // S
    { 0x01, 0x01, 0x7f, 0x01, 0x01 }, // T
    { 0x3f, 0x40, 0x40, 0x40, 0x3f }, // U
    { 0x1f, 0x20, 0x40, 0x20, 0x1f }, // V
    { 0x3f, 0x40, 0x38, 0x40, 0x3f }, // W
    { 0x63, 0x14, 0x08, 0x14, 0x63 }, // X
    { 0x07, 0x08, 0x70, 0x08, 0x07 }, // Y
    { 0x61, 0x51, 0x49, 0x45, 0x43 }, // Z
    { 0x00, 0x7f, 0x41, 0x41, 0x00 }, // [
    { 0x02, 0x04, 0x08, 0x10, 0x20 }, // "\"
    { 0x00, 0x41, 0x41, 0x7f, 0x00 }, // ]
    { 0x04, 0x02, 0x01, 0x02, 0x04 }, // ^
    { 0x40, 0x40, 0x40, 0x40, 0x40 }, // _
    { 0x00, 0x01, 0x02, 0x04, 0x00 }, // `
    { 0x20, 0x54, 0x54, 0x54, 0x78 }, // a
    { 0x7f, 0x48, 0x44, 0x44, 0x38 }, // b
    { 0x38, 0x44, 0x44, 0x44, 0x20 }, // c
    { 0x38, 0x44, 0x44, 0x48, 0x7f }, // d
    { 0x38, 0x54, 0x54, 0x54, 0x18 }, // e
    { 0x08, 0x7e, 0x09, 0x01, 0x02 }, // f
    { 0x0c, 0x52, 0x52, 0x52, 0x3e }, // g
    { 0x7f, 0x08, 0x04, 0x04, 0x78 }, // h
    { 0x00, 0x44, 0x7d, 0x40, 0x00 }, // i
    { 0x20, 0x40, 0x44, 0x3d, 0x00 }, // j
    { 0x7f, 0x10, 0x28, 0x44, 0x00 }, // k
    { 0x00, 0x41, 0x7f, 0x40, 0x00 }, // l
    { 0x7c, 0x04, 0x18, 0x04, 0x78 }, // m
    { 0x7c, 0x08, 0x04, 0x04, 0x78 }, // n
    { 0x38, 0x44, 0x44, 0x44, 0x38 }, // o
    { 0x7c, 0x14, 0x14, 0x14, 0x08 }, // p
    { 0x08, 0x14, 0x14, 0x18, 0x7c }, // q
    { 0x7c, 0x08, 0x04, 0x04, 0x08 }, // r
    { 0x48, 0x54, 0x54, 0x54, 0x20 }, // s
    { 0x04, 0x3f, 0x44, 0x40, 0x20 }, // t
    { 0x3c, 0x40, 0x40, 0x20, 0x7c }, // u
    { 0x1c, 0x20, 0x40, 0x20, 0x1c }, // v
    { 0x3c, 0x40, 0x30, 0x40, 0x3c }, // w
    { 0x44, 0x28, 0x10, 0x28, 0x44 }, // x
    { 0x0c, 0x50, 0x50, 0x50, 0x3c }, // y
    { 0x44, 0x64, 0x54, 0x4c, 0x44 }, // z
    { 0x00, 0x08, 0x36, 0x41, 0x00 }, // {
    { 0x00, 0x00, 0x7f, 0x00, 0x00 }, // |
    { 0x00, 0x41, 0x36, 0x08, 0x00 }, // }
    { 0x02, 0x01, 0x02, 0x04, 0x02 }, // ~
};

//*****************************************************************************
//
// The sequence of commands used to initialize the SSD1300 controller as found
// on the RIT displays used with ek-evalbot board.
//
//*****************************************************************************
static const unsigned char g_pucRITInit[] =
{
    //
    // Turn off the panel
    //
    0x08, 0x80, 0xae,

    //
    // Internal dc/dc on/off
    //
    0x80, 0xad, 0x80, 0x8a, 0x80, 0xe3,

    //
    // Multiplex ratio
    //
    0x06, 0x80, 0xa8, 0x80, 0x1f, 0x80, 0xe3,

    //
    // COM out scan direction
    //
    0x1e, 0x80, 0xc8,

    //
    // Segment map
    //
    0x80, 0xa0,

    //
    // Set area color mode
    //
    0x80, 0xd8,

    //
    // Low power save mode
    //
    0x80, 0x05,

    //
    // Start line
    //
    0x80, 0x40,

    //
    // Contrast setting
    //
    0x80, 0x81, 0x80, 0x5d,

    //
    // Pre-charge/discharge
    //
    0x80, 0xd9, 0x80, 0x11,

    //
    // Set display clock
    //
    0x80, 0xd5, 0x80, 0x01,

    //
    // Display offset
    //
    0x80, 0xd3, 0x80, 0x00,

    //
    // Display on
    //
    0x80, 0xaf, 0x80, 0xe3,
};

//*****************************************************************************
//
// The sequence of commands used to set the cursor to the first column of the
// first and second rows of the display for each of the supported displays.
//
//*****************************************************************************
static const unsigned char g_pucRITRow1[] =
{
    0xb0, 0x80, 0x04, 0x80, 0x10, 0x40
};
static const unsigned char g_pucRITRow2[] =
{
    0xb1, 0x80, 0x04, 0x80, 0x10, 0x40
};

//*****************************************************************************
//
//! \internal
//!
//! Start a transfer to the SSD1300 controller.
//!
//! \param ucChar is the first byte to be written to the controller.
//!
//! This function will start a transfer to the display controller via the I2C
//! bus.  The data is written in a polled fashion; this function will not
//! return until the byte has been written to the controller.
//!
//! \return None.
//
//*****************************************************************************
static void
Display96x16x1WriteFirst(unsigned char ucChar)
{
    //
    // Set the slave address.
    //
    ROM_I2CMasterSlaveAddrSet(I2C1_MASTER_BASE, SSD_ADDR, false);

    //
    // Write the first byte to the controller.
    //
    ROM_I2CMasterDataPut(I2C1_MASTER_BASE, ucChar);

    //
    // Start the transfer.
    //
    ROM_I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);
}

//*****************************************************************************
//
//! \internal
//!
//! Write a byte to the SSD1300 controller.
//!
//! \param ucChar is the byte to be transmitted to the controller.
//!
//! This function continues a transfer to the display controller by writing
//! another byte over the I2C bus.  This must only be called after calling
//! Display96x16x1WriteFirst(), but before calling Display96x16x1WriteFinal().
//!
//! The data is written in a polled fashion; this function will not return
//! until the byte has been written to the controller.
//!
//! \return None.
//
//*****************************************************************************
static void
Display96x16x1WriteByte(unsigned char ucChar)
{
    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(I2C1_MASTER_BASE, false) == 0)
    {
    }

    //
    // Clear the I2C interrupt.
    //
    ROM_I2CMasterIntClear(I2C1_MASTER_BASE);

    //
    // Write the next byte to the controller.
    //
    ROM_I2CMasterDataPut(I2C1_MASTER_BASE, ucChar);

    //
    // Continue the transfer.
    //
    ROM_I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
}

//*****************************************************************************
//
//! \internal
//!
//! Write a sequence of bytes to the SD1300 controller.
//!
//! This function continues a transfer to the display controller by writing a
//! sequence of bytes over the I2C bus.  This must only be called after calling
//! Display96x16x1WriteFirst(), but before calling Display96x16x1WriteFinal().
//!
//! The data is written in a polled fashion; this function will not return
//! until the entire byte sequence has been written to the controller.
//!
//! \return None.
//
//*****************************************************************************
static void
Display96x16x1WriteArray(const unsigned char *pucBuffer, unsigned long ulCount)
{
    //
    // Loop while there are more bytes left to be transferred.
    //
    while(ulCount != 0)
    {
        //
        // Wait until the current byte has been transferred.
        //
        while(ROM_I2CMasterIntStatus(I2C1_MASTER_BASE, false) == 0)
        {
        }

        //
        // Clear the I2C interrupt.
        //
        ROM_I2CMasterIntClear(I2C1_MASTER_BASE);

        //
        // Write the next byte to the controller.
        //
        ROM_I2CMasterDataPut(I2C1_MASTER_BASE, *pucBuffer++);
        ulCount--;

        //
        // Continue the transfer.
        //
        ROM_I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
    }
}

//*****************************************************************************
//
//! \internal
//!
//! Finish a transfer to the SD1300 controller.
//!
//! \param ucChar is the final byte to be written to the controller.
//!
//! This function will finish a transfer to the display controller via the I2C
//! bus.  This must only be called after calling Display96x16x1WriteFirst().
//!
//! The data is written in a polled fashion; this function will not return
//! until the byte has been written to the controller.
//!
//! \return None.
//
//*****************************************************************************
static void
Display96x16x1WriteFinal(unsigned char ucChar)
{
    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(I2C1_MASTER_BASE, false) == 0)
    {
    }

    //
    // Clear the I2C interrupt.
    //
    ROM_I2CMasterIntClear(I2C1_MASTER_BASE);

    //
    // Write the final byte to the controller.
    //
    ROM_I2CMasterDataPut(I2C1_MASTER_BASE, ucChar);

    //
    // Finish the transfer.
    //
    ROM_I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

    //
    // Wait until the final byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(I2C1_MASTER_BASE, false) == 0)
    {
    }

    //
    // Clear the I2C interrupt.
    //
    ROM_I2CMasterIntClear(I2C1_MASTER_BASE);
}

//*****************************************************************************
//
//! Clears a single line on the OLED display.
//!
//! \param ulY is the display line to be cleared, 0 or 1.
//!
//! This function will clear one text line of the display.
//!
//! \return None.
//
//*****************************************************************************
void
Display96x16x1ClearLine(unsigned long ulY)
{
    unsigned long ulIdx;

    //
    // Move the display cursor to the first column of the specified row.
    //
    Display96x16x1WriteFirst(0x80);
    Display96x16x1WriteArray(ulY ? g_pucRITRow2 : g_pucRITRow1,
                             sizeof(g_pucRITRow1));

    //
    // Fill this row with zeros.
    //
    for(ulIdx = 0; ulIdx < 95; ulIdx++)
    {
        Display96x16x1WriteByte(0x00);
    }
    Display96x16x1WriteFinal(0x00);
}

//*****************************************************************************
//
//! Clears the OLED display.
//!
//! This function clears the OLED display, turning all pixels off.
//!
//! \return None.
//
//*****************************************************************************
void
Display96x16x1Clear(void)
{
    //
    // Clear both lines of the display
    //
    Display96x16x1ClearLine(0);
    Display96x16x1ClearLine(1);
}

//*****************************************************************************
//
//! Displays a length-restricted string on the OLED display.
//!
//! \param pcStr is a pointer to the string to display.
//! \param ulLen is the number of characters to display.
//! \param ulX is the horizontal position to display the string, specified in
//! columns from the left edge of the display.
//! \param ulY is the vertical position to display the string, specified in
//! eight scan line blocks from the top of the display (that is, only 0 and 1
//! are valid).
//!
//! This function will draw a specified number of characters of a string on the
//! display.  Only the ASCII characters between 32 (space) and 126 (tilde) are
//! supported; other characters will result in random data being draw on the
//! display (based on whatever appears before/after the font in memory).  The
//! font is mono-spaced, so characters such as ``i'' and ``l'' have more white
//! space around them than characters such as ``m'' or ``w''.
//!
//! If the drawing of the string reaches the right edge of the display, no more
//! characters will be drawn.  Therefore, special care is not required to avoid
//! supplying a string that is ``too long'' to display.
//!
//! This function is similar to Display96x16x1SringDraw() except that the//
//! length of the string to display can be specified.
//!
//! \return None.
//
//*****************************************************************************
void
Display96x16x1StringDrawLen(const char *pcStr, unsigned long ulLen,
                            unsigned long ulX, unsigned long ulY)
{
    //
    // Check the arguments.
    //
    ASSERT(pcStr);
    ASSERT(ulX < 96);
    ASSERT(ulY < 2);

    //
    // Move the display cursor to the requested position on the display.
    //
    Display96x16x1WriteFirst(0x80);
    Display96x16x1WriteByte((ulY == 0) ? 0xb0 : 0xb1);
    Display96x16x1WriteByte(0x80);
    Display96x16x1WriteByte((ulX + 4) & 0x0f);
    Display96x16x1WriteByte(0x80);
    Display96x16x1WriteByte(0x10 | (((ulX + 4) >> 4) & 0x0f));
    Display96x16x1WriteByte(0x40);

    //
    // Loop while there are more characters in the string and the specified
    // length has not been exceeded.
    //
    while(ulLen && (*pcStr != 0))
    {
        //
        // See if there is enough space on the display for this entire
        // character.
        //
        if(ulX <= 90)
        {
            //
            // Write the contents of this character to the display.
            //
            Display96x16x1WriteArray(g_pucFont[*pcStr - ' '], 5);

            //
            // See if this is the last character to display (either because the
            // right edge has been reached or because there are no more
            // characters).
            //
            if((ulX == 90) || (pcStr[1] == 0))
            {
                //
                // Write the final column of the display.
                //
                Display96x16x1WriteFinal(0x00);

                //
                // The string has been displayed.
                //
                return;
            }

            //
            // Write the inter-character padding column.
            //
            Display96x16x1WriteByte(0x00);
        }
        else
        {
            //
            // Write the portion of the character that will fit onto the
            // display.
            //
            Display96x16x1WriteArray(g_pucFont[*pcStr - ' '], 95 - ulX);
            Display96x16x1WriteFinal(g_pucFont[*pcStr - ' '][95 - ulX]);

            //
            // The string has been displayed.
            //
            return;
        }

        //
        // Advance to the next character.
        //
        pcStr++;

        //
        // Decrement the character count
        //
        ulLen--;

        //
        // Increment the X coordinate by the six columns that were just
        // written.
        //
        ulX += 6;
    }
}

//*****************************************************************************
//
//! Displays a string on the OLED display.
//!
//! \param pcStr is a pointer to the string to display.
//! \param ulX is the horizontal position to display the string, specified in
//! columns from the left edge of the display.
//! \param ulY is the vertical position to display the string, specified in
//! eight scan line blocks from the top of the display (that is, only 0 and 1
//! are valid).
//!
//! This function will draw a string on the display.  Only the ASCII characters
//! between 32 (space) and 126 (tilde) are supported; other characters will
//! result in random data being draw on the display (based on whatever appears
//! before/after the font in memory).  The font is mono-spaced, so characters
//! such as ``i'' and ``l'' have more white space around them than characters
//! such as ``m'' or ``w''.
//!
//! If the drawing of the string reaches the right edge of the display, no more
//! characters will be drawn.  Therefore, special care is not required to avoid
//! supplying a string that is ``too long'' to display.
//!
//! \return None.
//
//*****************************************************************************
void
Display96x16x1StringDraw(const char *pcStr, unsigned long ulX,
                         unsigned long ulY)
{
    //
    // Check the arguments.
    //
    ASSERT(pcStr);
    ASSERT(ulX < 96);
    ASSERT(ulY < 2);

    //
    // Call the length restricted variant of this function, using a large
    // number for the length.
    //
    Display96x16x1StringDrawLen(pcStr, 32, ulX, ulY);
}

//*****************************************************************************
//
//! Draws a string horizontally centered on the OLED display.
//!
//! \param pcStr points to the NULL terminated string to be displayed.
//! \param ulY is the vertical position of the string specified in terms of
//!     8 pixel character cells.  Valid values are 0 and 1.
//! \param bClear is \b true if all uncovered areas of the display line are to
//!     be cleared or \b false if they are to be left unaffected.
//!
//! This function displays a string centered on a given line of the OLED
//! display and optionally clears sections of the display line to the left and
//! right of the provided string.
//!
//! \return None.
//
//*****************************************************************************
void
Display96x16x1StringDrawCentered(const char *pcStr, unsigned long ulY,
                                 tBoolean bClear)
{
    unsigned long ulLen, ulClip;

    //
    // Check the arguments.
    //
    ASSERT(pcStr);
    ASSERT(ulY < 2);

    //
    // How long is the supplied string?
    //
    ulLen = strlen(pcStr);

    //
    // Is the string too wide to fit on the display?  If so, clip it left and
    // right to fit.
    //
    if(ulLen > 16)
    {
        ulClip = (ulLen - 16) / 2;
        ulLen = 16;
    }
    else
    {
        ulClip = 0;
    }

    //
    // If we've been asked to clear the background, clear it now.
    //
    if(bClear)
    {
        Display96x16x1ClearLine(ulY);
    }

    //
    // Now draw the string at the desired position.
    //
    Display96x16x1StringDrawLen(pcStr + ulClip, ulLen,
                                ((96 - (ulLen * 6)) / 2), ulY);
}

//*****************************************************************************
//
//! Displays an image on the OLED display.
//!
//! \param pucImage is a pointer to the image data.
//! \param ulX is the horizontal position to display this image, specified in
//! columns from the left edge of the display.
//! \param ulY is the vertical position to display this image, specified in
//! eight scan line blocks from the top of the display (that is, only 0 and 1
//! are valid).
//! \param ulWidth is the width of the image, specified in columns.
//! \param ulHeight is the height of the image, specified in eight row blocks
//! (that is, only 1 and 2 are valid).
//!
//! This function will display a bitmap graphic on the display.  The image to
//! be displayed must be a multiple of eight scan lines high (that is, one row)
//! and will be drawn at a vertical position that is a multiple of eight scan
//! lines (that is, scan line zero or scan line eight, corresponding to row
//! zero or row one).
//!
//! The image data is organized with the first row of image data appearing left
//! to right, followed immediately by the second row of image data.  Each byte
//! contains the data for the eight scan lines of the column, with the top scan
//! line being in the least significant bit of the byte and the bottom scan
//! line being in the most significant bit of the byte.
//!
//! For example, an image four columns wide and sixteen scan lines tall would
//! be arranged as follows (showing how the eight bytes of the image would
//! appear on the display):
//!
//! \verbatim
//!     +-------+  +-------+  +-------+  +-------+
//!     |   | 0 |  |   | 0 |  |   | 0 |  |   | 0 |
//!     | B | 1 |  | B | 1 |  | B | 1 |  | B | 1 |
//!     | y | 2 |  | y | 2 |  | y | 2 |  | y | 2 |
//!     | t | 3 |  | t | 3 |  | t | 3 |  | t | 3 |
//!     | e | 4 |  | e | 4 |  | e | 4 |  | e | 4 |
//!     |   | 5 |  |   | 5 |  |   | 5 |  |   | 5 |
//!     | 0 | 6 |  | 1 | 6 |  | 2 | 6 |  | 3 | 6 |
//!     |   | 7 |  |   | 7 |  |   | 7 |  |   | 7 |
//!     +-------+  +-------+  +-------+  +-------+
//!
//!     +-------+  +-------+  +-------+  +-------+
//!     |   | 0 |  |   | 0 |  |   | 0 |  |   | 0 |
//!     | B | 1 |  | B | 1 |  | B | 1 |  | B | 1 |
//!     | y | 2 |  | y | 2 |  | y | 2 |  | y | 2 |
//!     | t | 3 |  | t | 3 |  | t | 3 |  | t | 3 |
//!     | e | 4 |  | e | 4 |  | e | 4 |  | e | 4 |
//!     |   | 5 |  |   | 5 |  |   | 5 |  |   | 5 |
//!     | 4 | 6 |  | 5 | 6 |  | 6 | 6 |  | 7 | 6 |
//!     |   | 7 |  |   | 7 |  |   | 7 |  |   | 7 |
//!     +-------+  +-------+  +-------+  +-------+
//! \endverbatim
//!
//! \return None.
//
//*****************************************************************************
void
Display96x16x1ImageDraw(const unsigned char *pucImage, unsigned long ulX,
                      unsigned long ulY, unsigned long ulWidth,
                      unsigned long ulHeight)
{
    //
    // Check the arguments.
    //
    ASSERT(ulX < 96);
    ASSERT(ulY < 2);
    ASSERT((ulX + ulWidth) <= 96);
    ASSERT((ulY + ulHeight) <= 2);

    //
    // The first few columns of the LCD buffer are not displayed, so increment
    // the X coorddinate by this amount to account for the non-displayed frame
    // buffer memory.
    //
    ulX += 4;

    //
    // Loop while there are more rows to display.
    //
    while(ulHeight--)
    {
        //
        // Write the starting address within this row.
        //
        Display96x16x1WriteFirst(0x80);
        Display96x16x1WriteByte((ulY == 0) ? 0xb0 : 0xb1);
        Display96x16x1WriteByte(0x80);
        Display96x16x1WriteByte(ulX & 0x0f);
        Display96x16x1WriteByte(0x80);
        Display96x16x1WriteByte(0x10 | ((ulX >> 4) & 0x0f));
        Display96x16x1WriteByte(0x40);

        //
        // Write this row of image data.
        //
        Display96x16x1WriteArray(pucImage, ulWidth - 1);
        Display96x16x1WriteFinal(pucImage[ulWidth - 1]);

        //
        // Advance to the next row of the image.
        //
        pucImage += ulWidth;
        ulY++;
    }
}

//*****************************************************************************
//
//! Initialize the OLED display.
//!
//! \param bFast is a boolean that is \e true if the I2C interface should be
//! run at 400 kbps and \e false if it should be run at 100 kbps.
//!
//! This function initializes the I2C interface to the OLED display and
//! configures the SSD0303 or SSD1300 controller on the panel.
//!
//! \return None.
//
//*****************************************************************************
void
Display96x16x1Init(tBoolean bFast)
{
    unsigned long ulIdx;

    //
    // The power supply for the OLED display comes from the motor power
    // supply, which must be turned on.  If the application is using the
    // motor then this is taken care of when the motor driver is initialized.
    // But if the motor driver is not used, then the motor power supply needs
    // to be turned on here so the OLED works properly.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_5);
    ROM_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_5, GPIO_PIN_5);

    //
    // Enable the I2C and GPIO peripherals needed for the display.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);

    //
    // Deassert the display controller reset signal (active low)
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);

    //
    // Wait a short delay, then drive the pin low to reset the controller
    //
    SysCtlDelay(32);
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);

    //
    // Leave it is reset for a short delay, then drive it high to deassert
    // reset.  Then the controller should be out of reset.
    //
    SysCtlDelay(32);
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);

    //
    // Configure the GPIO pins needed for the display as I2C
    //
    GPIOPinConfigure(GPIO_PG0_I2C1SCL);
    GPIOPinConfigure(GPIO_PG1_I2C1SDA);
    ROM_GPIOPinTypeI2C(GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Reset the I2C1 peripheral.
    //
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_I2C1);

    //
    // Initialize the I2C master.
    //
    ROM_I2CMasterInitExpClk(I2C1_MASTER_BASE, ROM_SysCtlClockGet(), bFast);

    //
    // Initialize the display controller.  Loop through the initialization
    // sequence doing a single I2C transfer for each command.
    //
    for(ulIdx = 0; ulIdx < sizeof(g_pucRITInit);
        ulIdx += g_pucRITInit[ulIdx] + 1)
    {
        //
        // Send this command.
        //
        Display96x16x1WriteFirst(g_pucRITInit[ulIdx + 1]);
        Display96x16x1WriteArray(g_pucRITInit + ulIdx + 2,
                                 g_pucRITInit[ulIdx] - 2);
        Display96x16x1WriteFinal(g_pucRITInit[ulIdx + g_pucRITInit[ulIdx]]);
    }

    //
    // Clear the frame buffer.
    //
    Display96x16x1Clear();

    //
    // Turn the display on.
    //
    Display96x16x1DisplayOn();
}

//*****************************************************************************
//
//! Turns on the OLED display.
//!
//! This function will turn on the OLED display, causing it to display the
//! contents of its internal frame buffer.
//!
//! \return None.
//
//*****************************************************************************
void
Display96x16x1DisplayOn(void)
{
    unsigned long ulIdx;

    //
    // Re-initialize the display controller.  Loop through the initialization
    // sequence doing a single I2C transfer for each command.
    //
    for(ulIdx = 0; ulIdx < sizeof(g_pucRITInit);
        ulIdx += g_pucRITInit[ulIdx] + 1)
    {
        //
        // Send this command.
        //
        Display96x16x1WriteFirst(g_pucRITInit[ulIdx + 1]);
        Display96x16x1WriteArray(g_pucRITInit + ulIdx + 2,
                                 g_pucRITInit[ulIdx] - 2);
        Display96x16x1WriteFinal(g_pucRITInit[ulIdx + g_pucRITInit[ulIdx]]);
    }
}

//*****************************************************************************
//
//! Turns off the OLED display.
//!
//! This function will turn off the OLED display.  This will stop the scanning
//! of the panel and turn off the on-chip DC-DC converter, preventing damage to
//! the panel due to burn-in (it has similar characters to a CRT in this
//! respect).
//!
//! \return None.
//
//*****************************************************************************
void
Display96x16x1DisplayOff(void)
{
    //
    // Turn off the DC-DC converter and the display.
    //
    Display96x16x1WriteFirst(0x80);
    Display96x16x1WriteByte(0xae);
    Display96x16x1WriteByte(0x80);
    Display96x16x1WriteByte(0xad);
    Display96x16x1WriteByte(0x80);
    Display96x16x1WriteFinal(0x8a);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
