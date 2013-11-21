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
// This is part of revision 10636 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/gpio.h"
#include "driverlib/flash.h"
#include "driverlib/interrupt.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/pushbutton.h"
#include "drivers/formike240x320x16_ili9320.h"
#include "drivers/touch.h"
#include "utils/ustdlib.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/locator.h"
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
//! This application supports remote software update over Ethernet using the
//! LM Flash Programmer application.  A firmware update is initiated using the
//! remote update request ``magic packet'' from LM Flash Programmer.
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
extern tCanvasWidget g_sIPAddr;
extern tPushButtonWidget g_sBlockIncBtn;
extern tPushButtonWidget g_sBlockDecBtn;
extern tPushButtonWidget g_sCharIncBtn;
extern tPushButtonWidget g_sCharDecBtn;

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
void PaintFontGlyphs(tWidget *pWidget, tContext *pContext);

//*****************************************************************************
//
// The canvas widget acting as the background to the display.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, &g_sIPAddr, &g_sHeading,
       &g_sFormike240x320x16_ILI9320, 0, 23, 240, (320 - 23),
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The canvas widget used to display the MAC address.
//
//*****************************************************************************
#define SIZE_MAC_ADDR_BUFFER 32
char g_pcMACString[SIZE_MAC_ADDR_BUFFER];
Canvas(g_sMACAddr, WIDGET_ROOT, 0, 0,
       &g_sFormike240x320x16_ILI9320, 0, 310, 240, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_pFontFixed6x8, g_pcMACString, 0, 0);

//*****************************************************************************
//
// The canvas widget used to display the IP address.
//
//*****************************************************************************
#define SIZE_IP_ADDR_BUFFER 24
char g_pcIPString[SIZE_IP_ADDR_BUFFER];
Canvas(g_sIPAddr, WIDGET_ROOT, &g_sMACAddr, 0,
       &g_sFormike240x320x16_ILI9320, 0, 300, 240, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       ClrBlack, 0, ClrWhite, g_pFontFixed6x8, g_pcIPString, 0, 0);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, &g_sBackground, &g_sCharCanvas, &g_sBlockNumCanvas,
       &g_sFormike240x320x16_ILI9320, 0, 0, 240, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE| CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "fontview", 0, 0);

//*****************************************************************************
//
// The canvas containing the rendered characters.
//
//*****************************************************************************
Canvas(g_sCharCanvas, &g_sBackground, 0, 0,
       &g_sFormike240x320x16_ILI9320, 0, 68, 240, 232,
       CANVAS_STYLE_APP_DRAWN,
       ClrDarkBlue, ClrWhite, ClrWhite, 0, 0, 0, PaintFontGlyphs);

//*****************************************************************************
//
// The canvas widget displaying the maximum and current block number.
//
//*****************************************************************************
Canvas(g_sBlockNumCanvas, &g_sHeading, &g_sCharNumCanvas, 0,
       &g_sFormike240x320x16_ILI9320, 0, 24, 240, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT),
       ClrBlack, 0, ClrWhite,
       g_pFontFixed6x8, g_pcBlocks, 0, 0);

//*****************************************************************************
//
// The canvas widget displaying the start codepoint for the block.
//
//*****************************************************************************
Canvas(g_sCharNumCanvas, &g_sHeading, &g_sBlockDecBtn, 0,
       &g_sFormike240x320x16_ILI9320, 0, 34, 240, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT),
       ClrBlack, 0, ClrWhite,
       g_pFontFixed6x8, g_pcStartChar, 0, 0);

//*****************************************************************************
//
// The button used to decrement the block number.
//
//*****************************************************************************
RectangularButton(g_sBlockDecBtn, &g_sHeading, &g_sBlockIncBtn, 0,
                  &g_sFormike240x320x16_ILI9320, 16, 45, 40, 20,
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
                  &g_sFormike240x320x16_ILI9320, 72, 45, 40, 20,
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
                  &g_sFormike240x320x16_ILI9320, 128, 45, 40, 20,
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
                  &g_sFormike240x320x16_ILI9320, 184, 45, 40, 20,
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
#define TOP                     68
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
// A signal used to tell the main loop to transfer control to the boot loader
// so that a firmware update can be performed over Ethernet.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
// This function is called by the swupdate module whenever it receives a
// signal indicating that a remote firmware update request is being made.  This
// notification occurs in the context of the Ethernet interrupt handler so it
// is vital that you DO NOT transfer control to the boot loader directly from
// this function (since the boot loader does not like being entered in interrupt
// context).
//
//*****************************************************************************
void
SoftwareUpdateRequestCallback(void)
{
    //
    // Set the flag that tells the main task to transfer control to the boot
    // loader.
    //
    g_bFirmwareUpdate = true;
}

//*****************************************************************************
//
// Initialize the Ethernet hardware and lwIP TCP/IP stack and set up to listen
// for remote firmware update requests.
//
//*****************************************************************************
unsigned long
TCPIPStackInit(void)
{
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACAddr[6];

    //
    // Get the MAC address from the UART0 and UART1 registers in NV ram.
    //
    FlashUserGet(&ulUser0, &ulUser1);

    //
    // Convert the 24/24 split MAC address from NV ram into a MAC address
    // array.
    //
    pucMACAddr[0] = ulUser0 & 0xff;
    pucMACAddr[1] = (ulUser0 >> 8) & 0xff;
    pucMACAddr[2] = (ulUser0 >> 16) & 0xff;
    pucMACAddr[3] = ulUser1 & 0xff;
    pucMACAddr[4] = (ulUser1 >> 8) & 0xff;
    pucMACAddr[5] = (ulUser1 >> 16) & 0xff;

    //
    // Format this address into a string and display it.
    //
    usnprintf(g_pcMACString, SIZE_MAC_ADDR_BUFFER,
              "MAC: %02X-%02X-%02X-%02X-%02X-%02X",
              pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
              pucMACAddr[4], pucMACAddr[5]);

    //
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACAddr);
    LocatorAppTitleSet("RDK-IDM-SBC fontview");

    //
    // Start monitoring for the special packet that tells us a software
    // download is being requested.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Return our initial IP address.  This is 0 for now since we have not
    // had one assigned yet.
    //
    return(0);
}

//*****************************************************************************
//
// Check to see if the IP address has changed and, if so, update the
// display.
//
//*****************************************************************************
unsigned long IPAddressChangeCheck(unsigned long ulCurrentIP)
{
    unsigned long ulIPAddr;

    //
    // What is our current IP address?
    //
    ulIPAddr = lwIPLocalIPAddrGet();

    //
    // Has the IP address changed?
    //
    if(ulIPAddr != ulCurrentIP)
    {
        //
        // Yes - the address changed so update the display.
        //
        usprintf(g_pcIPString, "IP: %d.%d.%d.%d",
                 ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                 (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
        WidgetPaint((tWidget *)&g_sIPAddr);
    }

    //
    // Return our current IP address.
    //
    return(ulIPAddr);
}

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

    //
    // Call the lwIP timer.
    //
    lwIPTimer(1000 / TICKS_PER_SECOND);
}

//*****************************************************************************
//
// Main entry function for the fontview application.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulHeight, ulWidth, ulIPAddr, ulCharBoxH, ulCharBoxW;

    //
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is
    // a workaround to allow the PLL to operate reliably.
    //
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }

    //
    // Set the system clock to run at 50MHz from the PLL.
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
    // Initialize the display driver.
    //
    Formike240x320x16_ILI9320Init();

    //
    // Turn on the backlight.
    //
    Formike240x320x16_ILI9320BacklightOn();

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Initialize the Ethernet hardware and lwIP TCP/IP stack.
    //
    ulIPAddr = TCPIPStackInit();

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
        g_pFont = (tFont *)&g_sFontWrapper;
    }
    else
    {
        g_pFont = FONT_TO_USE;
    }

    //
    // How big is the test font character cell? We add some padding in the
    // width here to ensure characters are not bunched together.
    //
    ulHeight = GrFontHeightGet(g_pFont) + 4;
    ulWidth = GrFontMaxWidthGet(g_pFont) + 4;

    //
    // Determine the size of the character cell to use for this font. We
    // limit the cell size such that we either get 8 or 16 characters per line.
    //
    ulCharBoxH = (g_sCharCanvas.sBase.sPosition.sYMax -
                 g_sCharCanvas.sBase.sPosition.sYMin) + 1;
    ulCharBoxW = (g_sCharCanvas.sBase.sPosition.sXMax -
                 g_sCharCanvas.sBase.sPosition.sXMin) + 1 - LEFT;

    //
    // How many characters can we fit across one line on the screen? Try for
    // 16 to begin with.
    //
    if(ulCharBoxW >= (ulWidth * 16))
    {
        g_ulCharsPerLine = 16;
    }
    //
    // We can't get 16 across. How about 8?
    //
    else if(ulCharBoxW >= (ulWidth * 8))
    {
        g_ulCharsPerLine = 8;
    }
    else
    //
    // We can't fit 8 so just take what we can get and forget about nice
    // round numbers.
    //
    {
        g_ulCharsPerLine = ulCharBoxW / ulWidth;

        //
        // Is even a single character too wide? (this is enormously unlikely!)
        //
        if(!g_ulCharsPerLine)
        {
            //
            // Yes - show 1 character per line and just clip it.
            //
            g_ulCharsPerLine = 1;
        }
    }

    g_ulCellWidth =  ulCharBoxW / g_ulCharsPerLine;
    g_ulCellHeight = ulHeight;
    g_ulLinesPerPage = ulCharBoxH / g_ulCellHeight;
    g_ulStartLine = 0;

    //
    // Get the number of blocks in the font and set up to display the content
    // of the first.
    //
    g_ulNumBlocks = GrFontNumBlocksGet(g_pFont);
    SetBlockNum(0);

    //
    // Loop until someone requests a firmware update.
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // Process any messages from or for the widgets.
        //
        WidgetMessageQueueProcess();

        //
        // Check for assignment of an IP address or a change in the address.
        //
        ulIPAddr = IPAddressChangeCheck(ulIPAddr);
    }

    //
    // If we drop out, a remote firmware update request has been received.
    // Let the user know what is going on then transfer control to the boot
    // loader.
    //
    CanvasTextSet(&g_sHeading, "Updating Firmware");
    WidgetPaint((tWidget *)&g_sHeading);
    WidgetMessageQueueProcess();

    //
    // Transfer control to the bootloader.
    //
    SoftwareUpdateBegin();

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
