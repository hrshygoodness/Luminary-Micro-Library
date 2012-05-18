//*****************************************************************************
//
// kitronix320x240x16_fpga.c - Display driver for the Kitronix K350QVG-V1-F TFT
//                             display with an SSD2119 controller attached via
//                             the FPGA/Camera daughter board.
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
// This is part of revision 8555 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/epi.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "drivers/kitronix320x240x16_fpga.h"
#include "drivers/set_pinout.h"
#include "drivers/camerafpga.h"

//*****************************************************************************
//
// Values used to set the ENTRY_MODE register.
//
//*****************************************************************************
#define HORIZ_DIRECTION 0x30
#define VERT_DIRECTION  0x38

//*****************************************************************************
//
// Various definitions controlling coordinate space mapping and drawing
// direction.
//
//*****************************************************************************
#define MAPPED_X(x, y) (x)
#define MAPPED_Y(x, y) (y)
#define SPAN_X_INC     1
#define SPAN_Y_INC     LCD_HORIZONTAL_MAX

//*****************************************************************************
//
// For now, we need to introduce a delay after each access to the memory
// aperture data register so we merely read into this dummy variable.
//
//*****************************************************************************
volatile unsigned short g_usDelayDummy;

#if 0
#define APERTURE_WRITE_DELAY                                \
{                                                           \
    g_usDelayDummy = HWREGH(FPGA_SYSCTRL_REG);              \
}
#else
#define APERTURE_WRITE_DELAY
#endif

//*****************************************************************************
//
// Various internal SD2119 registers name labels
//
//*****************************************************************************
#define SSD2119_DEVICE_CODE_READ_REG  0x00
#define SSD2119_OSC_START_REG         0x00
#define SSD2119_OUTPUT_CTRL_REG       0x01
#define SSD2119_LCD_DRIVE_AC_CTRL_REG 0x02
#define SSD2119_PWR_CTRL_1_REG        0x03
#define SSD2119_DISPLAY_CTRL_REG      0x07
#define SSD2119_FRAME_CYCLE_CTRL_REG  0x0B
#define SSD2119_PWR_CTRL_2_REG        0x0C
#define SSD2119_PWR_CTRL_3_REG        0x0D
#define SSD2119_PWR_CTRL_4_REG        0x0E
#define SSD2119_GATE_SCAN_START_REG   0x0F
#define SSD2119_SLEEP_MODE_1_REG      0x10
#define SSD2119_ENTRY_MODE_REG        0x11
#define SSD2119_SLEEP_MODE_2_REG      0x12
#define SSD2119_GEN_IF_CTRL_REG       0x15
#define SSD2119_PWR_CTRL_5_REG        0x1E
#define SSD2119_RAM_DATA_REG          0x22
#define SSD2119_FRAME_FREQ_REG        0x25
#define SSD2119_ANALOG_SET_REG        0x26
#define SSD2119_VCOM_OTP_1_REG        0x28
#define SSD2119_VCOM_OTP_2_REG        0x29
#define SSD2119_GAMMA_CTRL_1_REG      0x30
#define SSD2119_GAMMA_CTRL_2_REG      0x31
#define SSD2119_GAMMA_CTRL_3_REG      0x32
#define SSD2119_GAMMA_CTRL_4_REG      0x33
#define SSD2119_GAMMA_CTRL_5_REG      0x34
#define SSD2119_GAMMA_CTRL_6_REG      0x35
#define SSD2119_GAMMA_CTRL_7_REG      0x36
#define SSD2119_GAMMA_CTRL_8_REG      0x37
#define SSD2119_GAMMA_CTRL_9_REG      0x3A
#define SSD2119_GAMMA_CTRL_10_REG     0x3B
#define SSD2119_V_RAM_POS_REG         0x44
#define SSD2119_H_RAM_START_REG       0x45
#define SSD2119_H_RAM_END_REG         0x46
#define SSD2119_X_RAM_ADDR_REG        0x4E
#define SSD2119_Y_RAM_ADDR_REG        0x4F

#define ENTRY_MODE_DEFAULT 0x6830
#define MAKE_ENTRY_MODE(x) ((ENTRY_MODE_DEFAULT & 0xFF00) | (x))

//*****************************************************************************
//
// The dimensions of the LCD panel.
//
//*****************************************************************************
#define LCD_VERTICAL_MAX 240
#define LCD_HORIZONTAL_MAX 320

//*****************************************************************************
//
// The number of bytes per pixel.
//
//*****************************************************************************
#define PIXEL_SIZE 2

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
//*****************************************************************************
#define DPYCOLORTRANSLATE(c)    ((((c) & 0x00f80000) >> 8) |               \
                                 (((c) & 0x0000fc00) >> 5) |               \
                                 (((c) & 0x000000f8) >> 3))

//*****************************************************************************
//
// Display-specific data structure.
//
//*****************************************************************************
typedef struct
{
    unsigned long ulFrameBuffer;
} tFPGADisplayData;

tFPGADisplayData g_sDispData;

//*****************************************************************************
//
// Writes a data word to the SSD2119 via the EPI interface as wired when using
// the development kit SRAM/flash daughter board.
//
//*****************************************************************************
static void
WriteData(unsigned short usData)
{
    HWREGH(FPGA_LCDDATA_REG) = usData;
}

//*****************************************************************************
//
// Writes a command word to the SSD2119 via the EPI interface as wired when
// using the development kit SRAM/flash daughter board.
//
//*****************************************************************************
static void
WriteCommand(unsigned char ucData)
{
    HWREGH(FPGA_LCDCMD_REG) = ucData;
}

//*****************************************************************************
//
// Initializes the display driver.
//
// \param ulFrameBufAddr is the address of the graphics frame buffer relative
// to the start of FPGA SRAM.
//
// This function initializes the SSD2119 display controller on the panel,
// preparing it to display data, and also configures the the buffer used to
// store the graphics image when operating in FPGA auto-refresh mode.  This
// buffer must be (320 * 240 * 2) bytes in size to hold one full screen image.
// Valid values for the \e ulFrameBuffAddr parameter are, therefore, between
// 0 and 1MB - (320 * 240 * 2).
//
// The display driver operates in one of two modes.  By default, the display
// controller on the LCD panel is accessed directly via writes to its command
// and data registers.  To allow use with video captured using the FPGA,
// however, a second mode is implemented where the graphics image is held in
// FPGA SRAM and the display is automatically refreshed, mixing video and
// graphics.  In this mode, the display driver renders the graphics into an
// offscreen buffer in FPGA SRAM rather than by writing to the LCD controller
// registers directly and the FPGA itself refreshes the LCD display.
//
// On exit from this function the system is in ``auto'' mode.  Further calls
// to the display driver will access the display via the graphics buffer
// managed by the FPGA.  In this mode, video capture and display is supported.
// To allow direct access to the LCD command and data registers, the
// application must call Kitronix320x240x16_FPGAModeSet(false) to switch
// to ``legacy'' mode.  In this mode, motion video display is not possible.
//
// \return None.
//
//*****************************************************************************
void
Kitronix320x240x16_FPGAInit(unsigned long ulFrameBufAddr)
{
    unsigned long ulClockMS, ulCount;
    tFPGADisplayData *psDisp;

    //
    // Save the frame buffer pointer.
    //
    psDisp = (tFPGADisplayData *)g_sKitronix320x240x16_FPGA.pvDisplayData;
    psDisp->ulFrameBuffer = ulFrameBufAddr;

    //
    // Get the current processor clock frequency.
    //
    ulClockMS = ROM_SysCtlClockGet() / (3 * 1000);

    //
    // Asserts the LCD reset signal.
    //
    HWREGH(FPGA_LCDCLR_REG) = LCD_CONTROL_NRESET;

    //
    // Delay for 1ms.
    //
    SysCtlDelay(ulClockMS);

    //
    // Deassert the LCD reset signal
    //
    HWREGH(FPGA_LCDSET_REG) = LCD_CONTROL_NRESET;

    //
    // Delay for 1ms while the LCD comes out of reset.
    //
    SysCtlDelay(ulClockMS);

    //
    // Enter sleep mode (if we are not already there).
    //
    WriteCommand(SSD2119_SLEEP_MODE_1_REG);
    WriteData(0x0001);

    //
    // Set initial power parameters.
    //
    WriteCommand(SSD2119_PWR_CTRL_5_REG);
    WriteData(0x00B2);
    WriteCommand(SSD2119_VCOM_OTP_1_REG);
    WriteData(0x0006);

    //
    // Start the oscillator.
    //
    WriteCommand(SSD2119_OSC_START_REG);
    WriteData(0x0001);

    //
    // Set pixel format and basic display orientation (scanning direction).
    //
    WriteCommand(SSD2119_OUTPUT_CTRL_REG);
    WriteData(0x30EF);
    WriteCommand(SSD2119_LCD_DRIVE_AC_CTRL_REG);
    WriteData(0x0600);

    //
    // Exit sleep mode.
    //
    WriteCommand(SSD2119_SLEEP_MODE_1_REG);
    WriteData(0x0000);

    //
    // Delay 30mS
    //
    SysCtlDelay(30 * ulClockMS);

    //
    // Configure pixel color format and MCU interface parameters.
    //
    WriteCommand(SSD2119_ENTRY_MODE_REG);
    WriteData(ENTRY_MODE_DEFAULT);

    //
    // Set analog parameters.
    //
    WriteCommand(SSD2119_SLEEP_MODE_2_REG);
    WriteData(0x0999);
    WriteCommand(SSD2119_ANALOG_SET_REG);
    WriteData(0x3800);

    //
    // Enable the display.
    //
    WriteCommand(SSD2119_DISPLAY_CTRL_REG);
    WriteData(0x0033);

    //
    // Set VCIX2 voltage to 6.1V.
    //
    WriteCommand(SSD2119_PWR_CTRL_2_REG);
    WriteData(0x0005);

    //
    // Configure gamma correction.
    //
    WriteCommand(SSD2119_GAMMA_CTRL_1_REG);
    WriteData(0x0000);
    WriteCommand(SSD2119_GAMMA_CTRL_2_REG);
    WriteData(0x0303);
    WriteCommand(SSD2119_GAMMA_CTRL_3_REG);
    WriteData(0x0407);
    WriteCommand(SSD2119_GAMMA_CTRL_4_REG);
    WriteData(0x0301);
    WriteCommand(SSD2119_GAMMA_CTRL_5_REG);
    WriteData(0x0301);
    WriteCommand(SSD2119_GAMMA_CTRL_6_REG);
    WriteData(0x0403);
    WriteCommand(SSD2119_GAMMA_CTRL_7_REG);
    WriteData(0x0707);
    WriteCommand(SSD2119_GAMMA_CTRL_8_REG);
    WriteData(0x0400);
    WriteCommand(SSD2119_GAMMA_CTRL_9_REG);
    WriteData(0x0a00);
    WriteCommand(SSD2119_GAMMA_CTRL_10_REG);
    WriteData(0x1000);
    //
    // Configure Vlcd63 and VCOMl.
    //
    WriteCommand(SSD2119_PWR_CTRL_3_REG);
    WriteData(0x000A);
    WriteCommand(SSD2119_PWR_CTRL_4_REG);
    WriteData(0x2E00);

    //
    // Set the display size and ensure that the GRAM window is set to allow
    // access to the full display buffer.
    //
    WriteCommand(SSD2119_V_RAM_POS_REG);
    WriteData((LCD_VERTICAL_MAX-1) << 8);
    WriteCommand(SSD2119_H_RAM_START_REG);
    WriteData(0x0000);
    WriteCommand(SSD2119_H_RAM_END_REG);
    WriteData(LCD_HORIZONTAL_MAX-1);
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(0x00);
    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(0x00);

    //
    // Set the initial graphics buffer pointer, source rectangle and
    // destination position on the screen.
    //
    HWREG(FPGA_LGML_REG) = ulFrameBufAddr;
    HWREGH(FPGA_LGMS_REG) = LCD_HORIZONTAL_MAX * PIXEL_SIZE;

    //
    // Set FPGA memory aperture 1 to allow access to the frame buffer.
    //
    HWREG(FPGA_MP1L_REG) = ulFrameBufAddr;
    HWREGH(FPGA_MP1S_REG) = LCD_HORIZONTAL_MAX * PIXEL_SIZE;
    HWREGH(FPGA_MP1ONC_REG) = LCD_HORIZONTAL_MAX;
    FPGA_AP1_XY_SET(0, 0);

    //
    // Clear the contents of the display buffers.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);
    for(ulCount = 0; ulCount < (320 * 240); ulCount++)
    {
        WriteData(0x0000);
        HWREGH(FPGA_MPORT1_REG) = 0x0000;
    }

    //
    // Set the aperture access address back to the origin.
    //
    FPGA_AP1_XY_SET(0, 0);

    //
    // Turn on automatic graphics refresh by the FPGA.  If legacy support
    // is required (where the driver works by writing directly to the
    // LCD controller command and data registers), this can be selected
    // by a call to Kitronix320x240x16_FPGAModeSet().
    //
    HWREGH(FPGA_SYSCTRL_REG) |= FPGA_SYSCTRL_GDEN;
}

//*****************************************************************************
//
// Enables or disables the LCD display backlight.
//
// \param bEnable is \b true to enable the backlight or \b false to disable it.
//
// This function turns the display backlight on or off.
//
// \return None.
//
//*****************************************************************************
void
Kitronix320x240x16_FPGABacklight(tBoolean bEnable)
{
    if(bEnable)
    {
        //
        // Enable the LCD backlight
        //
        HWREGH(FPGA_LCDSET_REG) |= LCD_CONTROL_BKLIGHT;
    }
    else
    {
        //
        // Disable the LCD backlight
        //
        HWREGH(FPGA_LCDSET_REG) &= ~LCD_CONTROL_BKLIGHT;
    }
}

//*****************************************************************************
//
// Draws a pixel on the screen (FPGA auto refresh mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
// \param lX is the X coordinate of the pixel.
// \param lY is the Y coordinate of the pixel.
// \param ulValue is the color of the pixel.
//
// This function sets the given pixel to a particular color.  The coordinates
// of the pixel are assumed to be within the extents of the display.
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGAPixelDrawAuto(void *pvDisplayData, long lX, long lY,
                                     unsigned long ulValue)
{
    //
    // Set the aperture position and write the pixel value.
    //
    FPGA_AP1_XY_SET(lX, lY);
    HWREGH(FPGA_MPORT1_REG) = (unsigned short)ulValue;
}

//*****************************************************************************
//
// Draws a horizontal sequence of pixels on the screen (FPGA auto refresh
// mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
// \param lX is the X coordinate of the first pixel.
// \param lY is the Y coordinate of the first pixel.
// \param lX0 is sub-pixel offset within the pixel data, which is valid for 1
// or 4 bit per pixel formats.
// \param lCount is the number of pixels to draw.
// \param lBPP is the number of bits per pixel; must be 1, 4, or 8.
// \param pucData is a pointer to the pixel data.  For 1 and 4 bit per pixel
// formats, the most significant bit(s) represent the left-most pixel.
// \param pucPalette is a pointer to the palette used to draw the pixels.
//
// This function draws a horizontal sequence of pixels on the screen, using
// the supplied palette.  For 1 bit per pixel format, the palette contains
// pre-translated colors; for 4 and 8 bit per pixel formats, the palette
// contains 24-bit RGB values that must be translated before being written to
// the display.
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGAPixelDrawMultipleAuto(void *pvDisplayData, long lX,
                                             long lY, long lX0, long lCount,
                                             long lBPP,
                                             const unsigned char *pucData,
                                             const unsigned char *pucPalette)
{
    unsigned long ulByte;

    //
    // Set the start position for the line of pixels.
    //
    FPGA_AP1_XY_SET(lX, lY);

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
                    HWREGH(FPGA_MPORT1_REG) = (unsigned short)
                                (pucPalette[(ulByte >> (7 - lX0)) & 1]);
                    APERTURE_WRITE_DELAY;
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
                        HWREGH(FPGA_MPORT1_REG) =
                            (unsigned short)DPYCOLORTRANSLATE(ulByte);
                        APERTURE_WRITE_DELAY;

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
                            HWREGH(FPGA_MPORT1_REG) =
                                (unsigned short)DPYCOLORTRANSLATE(ulByte);
                            APERTURE_WRITE_DELAY;

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
                //
                // Get the next byte of pixel data and extract the
                // corresponding entry from the palette.
                //
                ulByte = *pucData++ * 3;
                ulByte = *(unsigned long *)(pucPalette + ulByte) & 0x00ffffff;

                //
                // Translate this palette entry and write it to the screen.
                //
                HWREGH(FPGA_MPORT1_REG) =
                            (unsigned short)DPYCOLORTRANSLATE(ulByte);
                APERTURE_WRITE_DELAY;
            }

            //
            // The image data has been drawn.
            //
            break;
        }

        //
        // We are being passed data in the display's native format.  Merely
        // write it directly to the display.  This is a special case which is
        // not used by the graphics library but which is helpful to
        // applications which may want to handle, for example, JPEG images.
        //
        case 16:
        {
            unsigned short usPixel;

            //
            // Loop while there are more pixels to draw.
            //
            while(lCount--)
            {
                //
                // Get the next byte of pixel data and extract the
                // corresponding entry from the palette.
                //
                usPixel = *((unsigned short *)pucData);
                pucData += 2;

                //
                // Write the pixel to the screen.
                //
                HWREGH(FPGA_MPORT1_REG) = usPixel;
                APERTURE_WRITE_DELAY;
            }
        }
    }
}

//*****************************************************************************
//
// Draws a horizontal line (FPGA auto refresh mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
// \param lX1 is the X coordinate of the start of the line.
// \param lX2 is the X coordinate of the end of the line.
// \param lY is the Y coordinate of the line.
// \param ulValue is the color of the line.
//
// This function draws a horizontal line on the display.  The coordinates of
// the line are assumed to be within the extents of the display.
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGALineDrawHAuto(void *pvDisplayData, long lX1, long lX2,
                                     long lY, unsigned long ulValue)
{
    //
    // Set the start position for the line of pixels.
    //
    FPGA_AP1_XY_SET(lX1, lY);

    //
    // Loop through the pixels of this horizontal line.
    //
    while(lX1++ <= lX2)
    {
        //
        // Write the pixel value.
        //
        HWREGH(FPGA_MPORT1_REG) = (unsigned short)ulValue;
        APERTURE_WRITE_DELAY;
    }
}

//*****************************************************************************
//
// Draws a vertical line (FPGA auto refresh mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
// \param lX is the X coordinate of the line.
// \param lY1 is the Y coordinate of the start of the line.
// \param lY2 is the Y coordinate of the end of the line.
// \param ulValue is the color of the line.
//
// This function draws a vertical line on the display.  The coordinates of the
// line are assumed to be within the extents of the display.
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGALineDrawVAuto(void *pvDisplayData, long lX, long lY1,
                                     long lY2, unsigned long ulValue)
{
    //
    // If using the memory aperture, set the start position for the line of
    // pixels.
    //
    FPGA_AP1_XY_SET(lX, lY1);

    //
    // Also set the aperture to increment vertically rather than horizontally.
    //
    HWREGH(FPGA_SYSCTRL_REG) |= FPGA_SYSCTRL_MPVI1;

    //
    // Loop through the pixels of this vertical line.
    //
    while(lY1++ <= lY2)
    {
        //
        // Write the pixel value.
        //
        HWREGH(FPGA_MPORT1_REG) = (unsigned short)ulValue;
        APERTURE_WRITE_DELAY;
    }

    //
    // Revert to memory aperture horizontal increment mode.
    //
    HWREGH(FPGA_SYSCTRL_REG) &= ~FPGA_SYSCTRL_MPVI1;
}

//*****************************************************************************
//
// Fills a rectangle (FPGA auto refresh mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
// \param pRect is a pointer to the structure describing the rectangle.
// \param ulValue is the color of the rectangle.
//
// This function fills a rectangle on the display.  The coordinates of the
// rectangle are assumed to be within the extents of the display, and the
// rectangle specification is fully inclusive (in other words, both sXMin and
// sXMax are drawn, along with sYMin and sYMax).
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGARectFillAuto(void *pvDisplayData, const tRectangle *pRect,
                                    unsigned long ulValue)
{
    long lX, lY, lWidth;
    tRectangle sRectDraw;

    //
    // Map the supplied rectangle to the physical display coordinate space.
    //
    sRectDraw = *pRect;

    //
    // Calculate the number of pixels to draw.
    //
    lWidth = (sRectDraw.sXMax - sRectDraw.sXMin) + 1;

    //
    // Loop through the pixels of this filled rectangle.
    //
    for(lY = sRectDraw.sYMin; lY <= sRectDraw.sYMax; lY++)
    {
        //
        // Set the start coordinate for this line of pixels
        //
        FPGA_AP1_XY_SET(sRectDraw.sXMin, lY);

        for(lX = 0; lX < lWidth; lX++)
        {
            //
            // Write the pixel value.
            //
            HWREGH(FPGA_MPORT1_REG) = (unsigned short)ulValue;
            APERTURE_WRITE_DELAY;
        }
    }
}

//*****************************************************************************
//
// Flushes any cached drawing operations (FPGA auto refresh mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
//
// This functions flushes any cached drawing operations to the display.  This
// is useful when a local frame buffer is used for drawing operations, and the
// flush would copy the local frame buffer to the display.  For the SSD2119
// driver, the flush is a no operation.
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGAFlushAuto(void *pvDisplayData)
{
    //
    // Nothing to do currently.
    //
}

//*****************************************************************************
//
// Draws a pixel on the screen (direct LCD access mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
// \param lX is the X coordinate of the pixel.
// \param lY is the Y coordinate of the pixel.
// \param ulValue is the color of the pixel.
//
// This function sets the given pixel to a particular color.  The coordinates
// of the pixel are assumed to be within the extents of the display.
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGAPixelDraw(void *pvDisplayData, long lX, long lY,
                                 unsigned long ulValue)
{
    //
    // Set the X address of the display cursor.
    //
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(MAPPED_X(lX, lY));

    //
    // Set the Y address of the display cursor.
    //
    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(MAPPED_Y(lX, lY));

    //
    // Write the pixel value.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);
    WriteData(ulValue);
}

//*****************************************************************************
//
// Draws a horizontal sequence of pixels on the screen (direct LCD access
// mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
// \param lX is the X coordinate of the first pixel.
// \param lY is the Y coordinate of the first pixel.
// \param lX0 is sub-pixel offset within the pixel data, which is valid for 1
// or 4 bit per pixel formats.
// \param lCount is the number of pixels to draw.
// \param lBPP is the number of bits per pixel; must be 1, 4, or 8.
// \param pucData is a pointer to the pixel data.  For 1 and 4 bit per pixel
// formats, the most significant bit(s) represent the left-most pixel.
// \param pucPalette is a pointer to the palette used to draw the pixels.
//
// This function draws a horizontal sequence of pixels on the screen, using
// the supplied palette.  For 1 bit per pixel format, the palette contains
// pre-translated colors; for 4 and 8 bit per pixel formats, the palette
// contains 24-bit RGB values that must be translated before being written to
// the display.
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGAPixelDrawMultiple(void *pvDisplayData, long lX,
                                        long lY, long lX0, long lCount,
                                        long lBPP,
                                        const unsigned char *pucData,
                                        const unsigned char *pucPalette)
{
    unsigned long ulByte;

    //
    // Set the cursor increment to left to right, followed by top to bottom.
    //
    WriteCommand(SSD2119_ENTRY_MODE_REG);
    WriteData(MAKE_ENTRY_MODE(HORIZ_DIRECTION));

    //
    // Set the starting X address of the display cursor.
    //
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(MAPPED_X(lX, lY));

    //
    // Set the Y address of the display cursor.
    //
    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(MAPPED_Y(lX, lY));

    //
    // Write the data RAM write command.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);

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
                    WriteData(((unsigned long *)pucPalette)[(ulByte >>
                                                             (7 - lX0)) & 1]);
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
                        WriteData(DPYCOLORTRANSLATE(ulByte));

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
                            WriteData(DPYCOLORTRANSLATE(ulByte));

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
                //
                // Get the next byte of pixel data and extract the
                // corresponding entry from the palette.
                //
                ulByte = *pucData++ * 3;
                ulByte = *(unsigned long *)(pucPalette + ulByte) & 0x00ffffff;

                //
                // Translate this palette entry and write it to the screen.
                //
                WriteData(DPYCOLORTRANSLATE(ulByte));
            }

            //
            // The image data has been drawn.
            //
            break;
        }

        //
        // We are being passed data in the display's native format.  Merely
        // write it directly to the display.  This is a special case which is
        // not used by the graphics library but which is helpful to
        // applications which may want to handle, for example, JPEG images.
        //
        case 16:
        {
            unsigned short usByte;

            //
            // Loop while there are more pixels to draw.
            //
            while(lCount--)
            {
                //
                // Get the next byte of pixel data and extract the
                // corresponding entry from the palette.
                //
                usByte = *((unsigned short *)pucData);
                pucData += 2;

                //
                // Translate this palette entry and write it to the screen.
                //
                WriteData(usByte);
            }
        }
    }
}

//*****************************************************************************
//
// Draws a horizontal line (direct LCD access mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
// \param lX1 is the X coordinate of the start of the line.
// \param lX2 is the X coordinate of the end of the line.
// \param lY is the Y coordinate of the line.
// \param ulValue is the color of the line.
//
// This function draws a horizontal line on the display.  The coordinates of
// the line are assumed to be within the extents of the display.
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGALineDrawH(void *pvDisplayData, long lX1, long lX2,
                                 long lY, unsigned long ulValue)
{
    //
    // Set the cursor increment to left to right, followed by top to bottom.
    //
    WriteCommand(SSD2119_ENTRY_MODE_REG);
    WriteData(MAKE_ENTRY_MODE(HORIZ_DIRECTION));

    //
    // Set the starting X address of the display cursor.
    //
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(MAPPED_X(lX1, lY));

    //
    // Set the Y address of the display cursor.
    //
    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(MAPPED_Y(lX1, lY));

    //
    // Write the data RAM write command.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);

    //
    // Loop through the pixels of this horizontal line.
    //
    while(lX1++ <= lX2)
    {
        //
        // Write the pixel value.
        //
        WriteData(ulValue);
    }
}

//*****************************************************************************
//
// Draws a vertical line (direct LCD access mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
// \param lX is the X coordinate of the line.
// \param lY1 is the Y coordinate of the start of the line.
// \param lY2 is the Y coordinate of the end of the line.
// \param ulValue is the color of the line.
//
// This function draws a vertical line on the display.  The coordinates of the
// line are assumed to be within the extents of the display.
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGALineDrawV(void *pvDisplayData, long lX, long lY1,
                                 long lY2, unsigned long ulValue)
{
    //
    // Set the cursor increment to top to bottom, followed by left to right.
    //
    WriteCommand(SSD2119_ENTRY_MODE_REG);
    WriteData(MAKE_ENTRY_MODE(VERT_DIRECTION));

    //
    // Set the X address of the display cursor.
    //
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(MAPPED_X(lX, lY1));

    //
    // Set the starting Y address of the display cursor.
    //
    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(MAPPED_Y(lX, lY1));

    //
    // Write the data RAM write command.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);

    //
    // Loop through the pixels of this vertical line.
    //
    while(lY1++ <= lY2)
    {
        //
        // Write the pixel value.
        //
        WriteData(ulValue);
    }
}

//*****************************************************************************
//
// Fills a rectangle (direct LCD access mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
// \param pRect is a pointer to the structure describing the rectangle.
// \param ulValue is the color of the rectangle.
//
// This function fills a rectangle on the display.  The coordinates of the
// rectangle are assumed to be within the extents of the display, and the
// rectangle specification is fully inclusive (in other words, both sXMin and
// sXMax are drawn, along with sYMin and sYMax).
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGARectFill(void *pvDisplayData, const tRectangle *pRect,
                                unsigned long ulValue)
{
    long lCount;

    //
    // Write the Y extents of the rectangle.
    //
    WriteCommand(SSD2119_ENTRY_MODE_REG);
    WriteData(MAKE_ENTRY_MODE(HORIZ_DIRECTION));

    //
    // Write the X extents of the rectangle.
    //
    WriteCommand(SSD2119_H_RAM_START_REG);
    WriteData(MAPPED_X(pRect->sXMin, pRect->sYMin));

    WriteCommand(SSD2119_H_RAM_END_REG);
    WriteData(MAPPED_X(pRect->sXMax, pRect->sYMax));

    //
    // Write the Y extents of the rectangle
    //
    WriteCommand(SSD2119_V_RAM_POS_REG);
    WriteData(MAPPED_Y(pRect->sXMin, pRect->sYMin) |
             (MAPPED_Y(pRect->sXMax, pRect->sYMax) << 8));

    //
    // Set the display cursor to the upper left of the rectangle (in application
    // coordinate space).
    //
    WriteCommand(SSD2119_X_RAM_ADDR_REG);
    WriteData(MAPPED_X(pRect->sXMin, pRect->sYMin));

    WriteCommand(SSD2119_Y_RAM_ADDR_REG);
    WriteData(MAPPED_Y(pRect->sXMin, pRect->sYMin));

    //
    // Tell the controller we are about to write data into its RAM.
    //
    WriteCommand(SSD2119_RAM_DATA_REG);

    //
    // Loop through the pixels of this filled rectangle.
    //
    for(lCount = ((pRect->sXMax - pRect->sXMin + 1) *
                  (pRect->sYMax - pRect->sYMin + 1)); lCount >= 0; lCount--)
    {
        //
        // Write the pixel value.
        //
        WriteData(ulValue);
    }

    //
    // Reset the X extents to the entire screen.
    //
    WriteCommand(SSD2119_H_RAM_START_REG);
    WriteData(0x0000);
    WriteCommand(SSD2119_H_RAM_END_REG);
    WriteData(0x013F);

    //
    // Reset the Y extent to the full screen
    //
    WriteCommand(SSD2119_V_RAM_POS_REG);
    WriteData(0xEF00);
}

//*****************************************************************************
//
// Flushes any cached drawing operations (direct LCD access mode).
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
//
// This functions flushes any cached drawing operations to the display.  This
// is useful when a local frame buffer is used for drawing operations, and the
// flush would copy the local frame buffer to the display.  For the SSD2119
// driver, the flush is a no operation.
//
// \return None.
//
//*****************************************************************************
static void
Kitronix320x240x16_FPGAFlush(void *pvDisplayData)
{
    //
    // No flush required.
    //
}

//*****************************************************************************
//
// Translates a 24-bit RGB color to a display driver-specific color.
//
// \param pvDisplayData is a pointer to the driver-specific data for this
// display driver.
// \param ulValue is the 24-bit RGB color.  The least-significant byte is the
// blue channel, the next byte is the green channel, and the third byte is the
// red channel.
//
// This function translates a 24-bit RGB color into a value that can be
// written into the display's frame buffer in order to reproduce that color,
// or the closest possible approximation of that color.
//
// \return Returns the display-driver specific color.
//
//*****************************************************************************
static unsigned long
Kitronix320x240x16_FPGAColorTranslate(void *pvDisplayData,
                                      unsigned long ulValue)
{
    //
    // Translate from a 24-bit RGB color to a 5-6-5 RGB color.
    //
    return(DPYCOLORTRANSLATE(ulValue));
}

//*****************************************************************************
//
// Translates a 24-bit RGB color to a display driver-specific color.
//
// \param ulValue is the 24-bit RGB color.  The least-significant byte is the
// blue channel, the next byte is the green channel, and the third byte is the
// red channel.
//
// This function translates a 24-bit RGB color into a value that can be
// written into the display's frame buffer in order to reproduce that color,
// or the closest possible approximation of that color.  Functionally, it is
// exactly equivalent to Kitronix320x240x16_FPGAColorTranslate() except that
// it does not require the driver-specific data structure pointer to be
// passed and is, hence, callable from clients other than the graphics
// library.
//
// \return Returns the display-driver specific color.
//
//*****************************************************************************
unsigned long
Kitronix320x240x16_FPGAColorMap(unsigned long ulValue)
{
    //
    // Translate from a 24-bit RGB color to a 5-6-5 RGB color.
    //
    return(DPYCOLORTRANSLATE(ulValue));
}

//*****************************************************************************
//
// Sets the display driver mode.
//
// \param bAutoRefresh is \b true if the display driver should use the
// FPGA's frame-buffer-based automatic refresh mode or \b false to remain in
// direct LCD access mode.
//
// This display driver operates in one of two modes.  By default, the display
// controller on the LCD panel is accessed directly via writes to its command
// and data registers.  To allow use with video captured using the FPGA,
// however, a second mode is implemented where the graphics image is held in
// FPGA SRAM and the display is automatically refreshed, mixing video and
// graphics.  In this mode, the display driver renders the graphics into an
// offscreen buffer in FPGA SRAM rather than by writing to the LCD controller
// registers.
//
// \note When disabling auto refresh mode, the caller must wait for the FPGA
// to signal that it has stopped accessing the LCD controller before calling
// the display driver again.  Calls are safe following the next ``LCD transfer
// end interrupt'' from the FPGA.
//
// \return None.
//
//*****************************************************************************
void
Kitronix320x240x16_FPGAModeSet(tBoolean bAutoRefresh)
{
    //
    // Are we switching into auto refresh mode?
    //
    if(bAutoRefresh)
    {
        //
        // Yes - replace all the function pointers with the auto refresh
        // versions and set the control bit that tells the FPGA it is now
        // responsible for updating the graphics.
        //
        g_sKitronix320x240x16_FPGA.pfnPixelDraw =
                            Kitronix320x240x16_FPGAPixelDrawAuto;
        g_sKitronix320x240x16_FPGA.pfnPixelDrawMultiple =
                            Kitronix320x240x16_FPGAPixelDrawMultipleAuto;
        g_sKitronix320x240x16_FPGA.pfnLineDrawH =
                            Kitronix320x240x16_FPGALineDrawHAuto;
        g_sKitronix320x240x16_FPGA.pfnLineDrawV =
                            Kitronix320x240x16_FPGALineDrawVAuto;
        g_sKitronix320x240x16_FPGA.pfnRectFill =
                            Kitronix320x240x16_FPGARectFillAuto;
        g_sKitronix320x240x16_FPGA.pfnFlush =
                            Kitronix320x240x16_FPGAFlushAuto;
        HWREGH(FPGA_SYSCTRL_REG) |= FPGA_SYSCTRL_GDEN;
    }
    else
    {
        //
        // No - clear the FPGA control bit to tell it to stop accessing the
        // LCD and swap out the various drawing handlers.
        //
        HWREGH(FPGA_SYSCTRL_REG) &= ~FPGA_SYSCTRL_GDEN;
        g_sKitronix320x240x16_FPGA.pfnPixelDraw =
                            Kitronix320x240x16_FPGAPixelDraw;
        g_sKitronix320x240x16_FPGA.pfnPixelDrawMultiple =
                            Kitronix320x240x16_FPGAPixelDrawMultiple;
        g_sKitronix320x240x16_FPGA.pfnLineDrawH =
                            Kitronix320x240x16_FPGALineDrawH;
        g_sKitronix320x240x16_FPGA.pfnLineDrawV =
                            Kitronix320x240x16_FPGALineDrawV;
        g_sKitronix320x240x16_FPGA.pfnRectFill =
                            Kitronix320x240x16_FPGARectFill;
        g_sKitronix320x240x16_FPGA.pfnFlush =
                            Kitronix320x240x16_FPGAFlush;
    }
}

//*****************************************************************************
//
// The display structure that describes the driver for the Kitronix
// K350QVG-V1-F TFT panel with an SSD2119 controller attached via the
// FPGA/camera daughter board.
//
//*****************************************************************************
tDisplay g_sKitronix320x240x16_FPGA =
{
    sizeof(tDisplay),
    (void *)&g_sDispData,
    LCD_HORIZONTAL_MAX,
    LCD_VERTICAL_MAX,
    Kitronix320x240x16_FPGAPixelDrawAuto,
    Kitronix320x240x16_FPGAPixelDrawMultipleAuto,
    Kitronix320x240x16_FPGALineDrawHAuto,
    Kitronix320x240x16_FPGALineDrawVAuto,
    Kitronix320x240x16_FPGARectFillAuto,
    Kitronix320x240x16_FPGAColorTranslate,
    Kitronix320x240x16_FPGAFlushAuto
};

//*****************************************************************************
//
// Close the Doxygen group.
// @}
//
//*****************************************************************************
