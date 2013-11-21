//*****************************************************************************
//
// fonttest.c - Simple font testcase.
//
// Copyright (c) 2010-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-IDM-L35 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "drivers/kitronix320x240x16_ssd2119.h"
#include "drivers/touch.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "fatwrapper.h"
#include "third_party/fonts/ofl/ofl_fonts.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Font Viewer (fontview)</h1>
//!
//! This example displays the contents of a Stellaris graphics library font
//! on the DK board's LCD touchscreen.  By default, the application shows a
//! test font containing ASCII, the Japanese Hiragana and Katakana alphabets,
//! and a group of Korean Hangul characters.  If an SDCard is installed and
//! the root directory contains a file named <tt>font.bin</tt>, this file is
//! opened and used as the display font instead.  In this case, the graphics
//! library font wrapper feature is used to access the font from the file
//! system rather than from internal memory.
//!
//! When the ``Update'' button is pressed, the application transfers control to
//! the boot loader to allow a new application image to be downloaded.  The
//! LMFlash serial data rate must be set to 115200bps and the "Program Address
//! Offset" to 0x800.
//!
//! UART0, which is connected to the 6 pin header on the underside of the
//! IDM-L35 RDK board (J8), is configured for 115200bps, and 8-n-1 mode.  The
//! USB-to-serial cable supplied with the IDM-L35 RDK may be used to connect
//! this TTL-level UART to the host PC to allow firmware update.
//
//*****************************************************************************

//*****************************************************************************
//
// Change the following to set the font whose characters you want to view if
// no "font.bin" is found in the root directory of the SDCard.
//
//*****************************************************************************
#define FONT_TO_USE g_pFontCjktest20pt

//*****************************************************************************
//
// A pointer to the font we intend to use.  This will be set depending upon
// whether we are using a font from the SDCard or the internal font defined
// above.
//
//*****************************************************************************
const tFont *g_pFont;

//*****************************************************************************
//
// The font wrapper structure used to describe the SDCard-based font to grlib.
//
//*****************************************************************************
tFontWrapper g_sFontWrapper =
{
    FONT_FMT_WRAPPED,
    0,
    &g_sFATFontAccessFuncs
};

//*****************************************************************************
//
// A global flag used to indicate if a remote firmware update request has been
// received.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_sHeading;
extern tCanvasWidget g_sBackground;
extern tCanvasWidget g_sBlockNumCanvas;
extern tCanvasWidget g_sCharNumCanvas;
extern tCanvasWidget g_sCharCanvas;
extern tPushButtonWidget g_sBlockIncBtn;
extern tPushButtonWidget g_sBlockDecBtn;
extern tPushButtonWidget g_sCharIncBtn;
extern tPushButtonWidget g_sCharDecBtn;
extern tPushButtonWidget g_sFirmwareUpdateBtn;

//*****************************************************************************
//
// Buffers for various strings.
//
//*****************************************************************************
char g_pcBlocks[20];
char g_pcStartChar[32];

//*****************************************************************************
//
// Forward reference to various handlers and internal functions.
//
//*****************************************************************************
void SetBlockNum(unsigned long ulBlockNum);
void OnBlockButtonPress(tWidget *pWidget);
void OnCharButtonPress(tWidget *pWidget);
void OnFirmwareUpdate(tWidget *pWidget);
void PaintFontGlyphs(tWidget *pWidget, tContext *pContext);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, 0, &g_sHeading,
       &g_sKitronix320x240x16_SSD2119, 0, 23, 320, (240 - 23),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, &g_sBackground, &g_sCharCanvas, &g_sBlockNumCanvas,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE| CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "fontview", 0, 0);

//*****************************************************************************
//
// The canvas containing the rendered characters.
//
//*****************************************************************************
Canvas(g_sCharCanvas, &g_sBackground, &g_sFirmwareUpdateBtn, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 48, 320, 168,
       CANVAS_STYLE_APP_DRAWN,
       ClrDarkBlue, ClrWhite, ClrWhite, 0, 0, 0, PaintFontGlyphs);

RectangularButton(g_sFirmwareUpdateBtn, &g_sBackground, 0, 0,
                   &g_sKitronix320x240x16_SSD2119, 90, 220, 140, 20,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL), ClrNavy, ClrBlue, ClrSilver, ClrSilver,
                   g_pFontCm16, "Update", 0, 0, 0, 0, OnFirmwareUpdate);

//*****************************************************************************
//
// The canvas widget displaying the maximum and current block number.
//
//*****************************************************************************
Canvas(g_sBlockNumCanvas, &g_sHeading, &g_sCharNumCanvas, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 24, 200, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT),
       ClrBlack, 0, ClrWhite,
       g_pFontFixed6x8, g_pcBlocks, 0, 0);

//*****************************************************************************
//
// The canvas widget displaying the start codepoint for the block.
//
//*****************************************************************************
Canvas(g_sCharNumCanvas, &g_sHeading, &g_sBlockDecBtn, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 34, 200, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT),
       ClrBlack, 0, ClrWhite,
       g_pFontFixed6x8, g_pcStartChar, 0, 0);

//*****************************************************************************
//
// The button used to decrement the block number.
//
//*****************************************************************************
RectangularButton(g_sBlockDecBtn, &g_sHeading, &g_sBlockIncBtn, 0,
                  &g_sKitronix320x240x16_SSD2119, 200, 26, 20, 20,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL),
                   ClrDarkBlue, ClrRed, ClrWhite, ClrWhite,
                   g_pFontFixed6x8, "<", 0, 0, 0, 0,
                   OnBlockButtonPress);

//*****************************************************************************
//
// The button used to increment the block number.
//
//*****************************************************************************
RectangularButton(g_sBlockIncBtn, &g_sHeading, &g_sCharDecBtn, 0,
                  &g_sKitronix320x240x16_SSD2119, 230, 26, 20, 20,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL),
                   ClrDarkBlue, ClrRed, ClrWhite, ClrWhite,
                   g_pFontFixed6x8, ">", 0, 0, 0, 0,
                   OnBlockButtonPress);

//*****************************************************************************
//
// The button used to decrement the character row number.
//
//*****************************************************************************
RectangularButton(g_sCharDecBtn, &g_sHeading, &g_sCharIncBtn, 0,
                  &g_sKitronix320x240x16_SSD2119, 260, 26, 20, 20,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_AUTO_REPEAT),
                   ClrDarkBlue, ClrRed, ClrWhite, ClrWhite,
                   g_pFontFixed6x8, "^", 0, 0, 70, 20,
                   OnCharButtonPress);

//*****************************************************************************
//
// The button used to increment the character row number.
//
//*****************************************************************************
RectangularButton(g_sCharIncBtn, &g_sHeading, 0, 0,
                  &g_sKitronix320x240x16_SSD2119, 290, 26, 20, 20,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_AUTO_REPEAT),
                   ClrDarkBlue, ClrRed, ClrWhite, ClrWhite,
                   g_pFontFixed6x8, "v", 0, 0, 70, 20,
                   OnCharButtonPress);

//*****************************************************************************
//
// Text codepage mapping functions.
//
//*****************************************************************************
tCodePointMap g_psCodepointMappings[] =
{
    {CODEPAGE_ISO8859_1, CODEPAGE_UNICODE, GrMapISO8859_1_Unicode},
    {CODEPAGE_UTF_8,     CODEPAGE_UNICODE, GrMapUTF8_Unicode},
    {CODEPAGE_UNICODE,   CODEPAGE_UNICODE, GrMapUnicode_Unicode}
};

#define NUM_CHAR_MAPPINGS (sizeof(g_psCodepointMappings)/sizeof(tCodePointMap))

//*****************************************************************************
//
// Default text rendering parameters.  The only real difference between the
// grlib defaults and this set is the addition of a mapping function to allow
// 32 bit Unicode source.
//
//*****************************************************************************
tGrLibDefaults g_psGrLibSettingDefaults =
{
    GrDefaultStringRenderer,
    g_psCodepointMappings,
    CODEPAGE_UTF_8,
    NUM_CHAR_MAPPINGS,
    0
};

//*****************************************************************************
//
// Top left corner of the grid we use to draw the characters.
//
//*****************************************************************************
#define TOP                     50
#define LEFT                    40

//*****************************************************************************
//
// The character cell size used when redrawing the display.
//
//*****************************************************************************
unsigned long g_ulCellWidth;
unsigned long g_ulCellHeight;
unsigned long g_ulLinesPerPage;
unsigned long g_ulCharsPerLine;
unsigned long g_ulStartLine;
unsigned long g_ulNumBlocks;
unsigned long g_ulStartChar;
unsigned long g_ulNumBlockChars;
unsigned long g_ulBlockNum;

//*****************************************************************************
//
// Macros used to derive the screen position given the character's X and Y
// coordinates in the grid.
//
//*****************************************************************************
#define POSX(x) (LEFT + (g_ulCellWidth / 2) + (g_ulCellWidth * (x)))
#define POSY(y) (TOP + (g_ulCellHeight / 2) + (g_ulCellHeight * (y)))

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
    UARTprintf("Runtime error at line %d of %s!\n", ulLine, pcFilename);
    while(1)
    {
    }
}
#endif

//*****************************************************************************
//
// This function is called by the graphics library widget manager in the
// context of WidgetMessageQueueProcess whenever the user releases the
// ">" or "<" button.
//
//*****************************************************************************
void
OnBlockButtonPress(tWidget *pWidget)
{
    tBoolean bRedraw;

    //
    // Assume no redraw until we determine otherwise.
    //
    bRedraw = false;

    //
    // Are we incrementing or decrementing the block number?
    //
    if(pWidget == (tWidget *)&g_sBlockIncBtn)
    {
        //
        // We are incrementing.  Have we already reached the top block?
        //
        if((g_ulBlockNum + 1) < g_ulNumBlocks)
        {
            //
            // No - increment the block number.
            //
            g_ulBlockNum++;
            bRedraw = true;
        }
    }
    else
    {
        //
        // We are decrementing.  Are we already showing the first block?
        //
        if(g_ulBlockNum)
        {
            //
            // No - move back 1 block.
            //
            g_ulBlockNum--;
            bRedraw = true;
        }
    }

    //
    // If we made a change, set things up to display the new block.
    //
    if(bRedraw)
    {
        SetBlockNum(g_ulBlockNum);
    }
}

//*****************************************************************************
//
// This function is called by the graphics library widget manager in the
// context of WidgetMessageQueueProcess whenever the user releases the
// "^" or "v" button.
//
//*****************************************************************************
void
OnCharButtonPress(tWidget *pWidget)
{
    tBoolean bRedraw;

    //
    // Assume no redraw until we determine otherwise.
    //
    bRedraw = false;

    //
    // Were we asked to scroll up or down?
    //
    if(pWidget == (tWidget *)&g_sCharIncBtn)
    {
        //
        // Scroll down if there are more characters to display.
        //
        if(((g_ulStartLine + g_ulLinesPerPage) * g_ulCharsPerLine) <
            g_ulNumBlockChars)
        {
            g_ulStartLine++;
            bRedraw = true;
        }
    }
    else
    {
        //
        // Scroll up if we're not already showing the first line.
        //
        if(g_ulStartLine)
        {
            g_ulStartLine--;
            bRedraw = true;
        }
    }

    //
    // If we made a change, redraw the character area.
    //
    if(bRedraw)
    {
        WidgetPaint((tWidget *)&g_sCharCanvas);
    }
}


//*****************************************************************************
//
// Handles press notifications for the push button widgets.
//
//*****************************************************************************
void
OnFirmwareUpdate(tWidget *pWidget)
{
    //
    // Change the button text to show that the update is starting
    //
    PushButtonTextSet(&g_sFirmwareUpdateBtn, "Updating...");
    WidgetPaint((tWidget *)&g_sFirmwareUpdateBtn);

    //
    // Trigger a software update
    //
    g_bFirmwareUpdate = true;
}

//*****************************************************************************
//
// Update the display for a new font block.
//
//*****************************************************************************
void
SetBlockNum(unsigned long ulBlockNum)
{
    unsigned long ulStart, ulChars;

    //
    // Set the new block number.
    //
    ulChars = GrFontBlockCodepointsGet(g_pFont, ulBlockNum, &ulStart);

    //
    // If this block exists, update our state.
    //
    if(ulChars)
    {
        //
        // Remember details of the new block.
        //
        g_ulBlockNum = ulBlockNum;
        g_ulStartChar = ulStart;
        g_ulNumBlockChars = ulChars;
        g_ulStartLine = 0;

        //
        // Update the valid block display and start character.
        //
        usnprintf(g_pcBlocks, sizeof(g_pcBlocks), "Block %d of %d  ",
                  g_ulBlockNum + 1, g_ulNumBlocks);
        usnprintf(g_pcStartChar, sizeof(g_pcStartChar), "%d chars from 0x%08x",
                 g_ulNumBlockChars, g_ulStartChar);
    }

    //
    // Repaint the display
    //
    WidgetPaint((tWidget *)WIDGET_ROOT);
}

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.  FatFs requires a
// timer tick every 10 ms for internal timing purposes.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Call the FAT module to provide its tick.
    //
    FATWrapperSysTickHandler();
}

//*****************************************************************************
//
// Passes control to the bootloader and initiates a remote software update
// over the serial connection.
//
// This function passes control to the bootloader and initiates an update of
// the main application firmware image via UART0.
//
// \return Never returns.
//
//*****************************************************************************
void
JumpToBootLoader(void)
{
    //
    // Disable all processor interrupts.  Instead of disabling them
    // one at a time (and possibly missing an interrupt if new sources
    // are added), a direct write to NVIC is done to disable all
    // peripheral interrupts.
    //
    IntMasterDisable();
    SysTickIntDisable();
    HWREG(NVIC_DIS0) = 0xffffffff;
    HWREG(NVIC_DIS1) = 0xffffffff;

    //
    // Return control to the boot loader.  This is a call to the SVC
    // handler in the boot loader.
    //
    (*((void (*)(void))(*(unsigned long *)0x2c)))();
}

//*****************************************************************************
//
// Main entry function for the fontview application.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulHeight, ulWidth;

    //
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    // a workaround to allow the PLL to operate reliably.
    //
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }

    //
    // Set the clocking to run from the PLL.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    SysTickPeriodSet(SysCtlClockGet() / TICKS_PER_SECOND);
    SysTickEnable();
    SysTickIntEnable();

    //
    // Enable Interrupts
    //
    IntMasterEnable();

    //
    // Set GPIO A0 and A1 as UART.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART as a console for text I/O.
    //
    UARTStdioInit(0);
    UARTprintf("FontView example running...\n");

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Turn on the backlight.
    //
    Kitronix320x240x16_SSD2119BacklightOn(255);

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Set graphics library text rendering defaults.
    //
    GrLibInit(&g_psGrLibSettingDefaults);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBackground);

    //
    // Paint the widget tree to make sure they all appear on the display.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Initialize the FAT file system font wrapper.
    //
    if(!FATFontWrapperInit())
    {
        UARTprintf("ERROR: Can't initialize FAT file system!\n");
        while(1)
        {
            //
            // Hang.
            //
        }
    }

    //
    // See if there is a file called "font.bin" in the root directory of the
    // SDCard.  If there is, use this as the font to display rather than the
    // one defined via FONT_TO_USE.
    //
    g_sFontWrapper.pucFontId = FATFontWrapperLoad("/font.bin");
    if(g_sFontWrapper.pucFontId)
    {
        UARTprintf("Using font from SDCard.\n");
        g_pFont = (tFont *)&g_sFontWrapper;
    }
    else
    {
        UARTprintf("No font found on SDCard. Displaying internal font.\n");
        g_pFont = FONT_TO_USE;
    }

    //
    // How big is the test font character cell?
    //
    ulHeight = GrFontHeightGet(g_pFont);
    ulWidth = GrFontMaxWidthGet(g_pFont);

    //
    // Determine the size of the character cell to use for this font. We
    // limit the cell size such that we either get 8 or 16 characters per line.
    //
    g_ulCharsPerLine = (ulWidth > ((320 - LEFT) / 16)) ? 8 : 16;
    g_ulCellWidth = (320 - LEFT) / g_ulCharsPerLine;
    g_ulCellHeight = ulHeight + 4;
    g_ulLinesPerPage = ((g_sCharCanvas.sBase.sPosition.sYMax -
                        g_sCharCanvas.sBase.sPosition.sYMin) + 1) /
                        g_ulCellHeight;
    g_ulStartLine = (0x20 / g_ulCharsPerLine);

    //
    // Get the number of blocks in the font and set up to display the content
    // of the first.
    //
    g_ulNumBlocks = GrFontNumBlocksGet(g_pFont);
    SetBlockNum(0);

    //
    // Loop forever, processing widget messages.
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // Process any messages from or for the widgets.
        //
        WidgetMessageQueueProcess();
    }


    //
    // If we drop out, a firmware update request has been made.  We call
    // WidgetMessageQueueProcess once more to ensure that any final messages
    // are processed then jump into the bootloader.
    //
    WidgetMessageQueueProcess();

    //
    // Tell the user what we are doing.
    //
    UARTprintf("Serial firmware update requested.\n");

    //
    // Warn the user about closing their terminal.
    //
    UARTprintf("Transfering control to boot loader...\n\n");
    UARTprintf("***********************************\n");
    UARTprintf("*** Close your serial terminal ****\n");
    UARTprintf("***   before running LMFlash.  ****\n");
    UARTprintf("***********************************\n\n");
    UARTFlushTx(false);

    //
    // Pass control to the bootloader.
    //
    JumpToBootLoader();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// Paints the main font glyph section of the display.
//
//*****************************************************************************
void
PaintFontGlyphs(tWidget *pWidget, tContext *pContext)
{
    unsigned long ulX, ulY, ulChar;
    tRectangle sRect;
    tCanvasWidget *pCanvas;
    char pcBuffer[6];

    //
    // Tell the graphics library we will be using UTF-8 text for now.
    //
    GrStringCodepageSet(pContext, CODEPAGE_UTF_8);

    //
    // Erase the background.
    //
    pCanvas = (tCanvasWidget *)pWidget;
    GrContextForegroundSet(pContext, pCanvas->ulFillColor);
    sRect = pCanvas->sBase.sPosition;
    GrRectFill(pContext, &sRect);

    //
    // Draw the character indices.
    //
    GrContextForegroundSet(pContext, ClrYellow);
    GrContextFontSet(pContext, g_pFontFixed6x8);

    for(ulX = 0; ulX < g_ulCharsPerLine; ulX++ )
    {
        usprintf(pcBuffer, "%x", ulX);
        GrStringDrawCentered(pContext, pcBuffer, -1,
                             POSX(ulX), (TOP - 20), 0);
    }

    for(ulY = 0; ulY < g_ulLinesPerPage; ulY++ )
    {
        usprintf(pcBuffer, "%06x", g_ulStartChar + ((ulY + g_ulStartLine) *
                 g_ulCharsPerLine) );
        GrStringDraw(pContext, pcBuffer, -1, 0, POSY(ulY), 0);
    }

    //
    // Tell the graphics library to render pure, 32 bit Unicode source
    // text.
    //
    GrStringCodepageSet(pContext, CODEPAGE_UNICODE);

    //
    // Draw the required characters at their positions in the grid.
    //
    GrContextFontSet(pContext, g_pFont);
    GrContextForegroundSet(pContext, ClrWhite);

    for(ulY = 0; ulY < g_ulLinesPerPage; ulY++)
    {
        for(ulX = 0; ulX < g_ulCharsPerLine; ulX++)
        {
            //
            // Which character are we about to show?
            //
            ulChar = g_ulStartChar +
                     ((g_ulStartLine + ulY) * g_ulCharsPerLine) + ulX;

            //
            // Fill the character cell with the background color.
            //
            sRect.sXMin = LEFT + (ulX * g_ulCellWidth);
            sRect.sYMin = TOP + (ulY * g_ulCellHeight);
            sRect.sXMax = sRect.sXMin + g_ulCellWidth;
            sRect.sYMax = sRect.sYMin + g_ulCellHeight;
            GrContextForegroundSet(pContext, pCanvas->ulFillColor);
            GrRectFill(pContext, &sRect);
            GrContextForegroundSet(pContext, ClrWhite);

            //
            // Have we run off the end of the end of the block?
            //
            if((ulChar - g_ulStartChar) < g_ulNumBlockChars)
            {
                //
                // No - display the character.
                //

                GrStringDrawCentered(pContext, (char const *)&ulChar, 4,
                                     POSX(ulX), POSY(ulY), 0);
            }
        }
    }
}
