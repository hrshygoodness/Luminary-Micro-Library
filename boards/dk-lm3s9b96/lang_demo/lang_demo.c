//*****************************************************************************
//
// lang_demo.c - Demonstration of the Stellaris Graphics Library's string
//               table support.
//
// Copyright (c) 2008-2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/checkbox.h"
#include "grlib/container.h"
#include "grlib/pushbutton.h"
#include "grlib/radiobutton.h"
#include "grlib/slider.h"
#include "utils/ustdlib.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"
#include "images.h"
#include "language.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Graphics Library String Table Demonstration (lang_demo)</h1>
//!
//! This application provides a demonstration of the capabilities of the
//! Stellaris Graphics Library's string table functions.  Two panels show
//! different implementations of features of the string table functions.
//! For each panel, the bottom provides a forward and back button
//! (when appropriate).
//!
//! The first panel provides a large string with introductory text and basic
//! instructions for operation of the application.
//!
//! The second panel shows the available languages and allows them to be
//! switched between English, German, Spanish and Italian.
//!
//*****************************************************************************

//*****************************************************************************
//
// The names for each of the panels, which is displayed at the bottom of the
// screen.
//
//*****************************************************************************
static const unsigned long g_ulPanelNames[3] =
{
    STR_INTRO,
    STR_CONFIG,
    STR_UPDATE
};

//*****************************************************************************
//
// This string holds the title of the group of languages.  The size is fixed
// by LANGUAGE_MAX_SIZE since the names of the languages in this application
// are not larger than LANGUAGE_MAX_SIZE bytes.
//
//*****************************************************************************
#define LANGUAGE_MAX_SIZE       16
char g_pcLanguage[LANGUAGE_MAX_SIZE];

//*****************************************************************************
//
// This is a generic buffer that is used to retrieve strings from the string
// table.  This forces its size to be at least as big as the largest string
// in the string table which is defined by the SCOMP_MAX_STRLEN value.
//
//*****************************************************************************
char g_pcBuffer[SCOMP_MAX_STRLEN];

//*****************************************************************************
//
// This string holds the title of each "panel" in the application.  The size is
// fixed by TITLE_MAX_SIZE since the names of the panels in this application
// are not larger than TITLE_MAX_SIZE bytes.
//
//*****************************************************************************
#define TITLE_MAX_SIZE          20
char g_pcTitle[TITLE_MAX_SIZE];

//*****************************************************************************
//
// This table holds the array of languages supported.
//
//*****************************************************************************
const unsigned short g_eLanguageTable[4] =
{
    GrLangEnUS,
    GrLangDe,
    GrLangEsSP,
    GrLangIt,
};

//*****************************************************************************
//
// The DMA control structure table used by the sound driver.
//
//*****************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable sDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(sDMAControlTable, 1024)
tDMAControlTable sDMAControlTable[64];
#else
tDMAControlTable sDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// The sound effect that is played when a key is pressed.
//
//*****************************************************************************
static const unsigned short g_pusKeyClick[] =
{
    0, G5,
    25, SILENCE
};

//*****************************************************************************
//
// Forward declarations for the globals required to define the widgets at
// compile-time.
//
//*****************************************************************************
void OnPrevious(tWidget *pWidget);
void OnNext(tWidget *pWidget);
void OnIntroPaint(tWidget *pWidget, tContext *pContext);
void OnRadioChange(tWidget *pWidget, unsigned long bSelected);
extern tCanvasWidget g_psPanels[];

//*****************************************************************************
//
// The first panel, which contains introductory text explaining the
// application.
//
//*****************************************************************************
Canvas(g_sIntroduction, g_psPanels, 0, 0, &g_sKitronix320x240x16_SSD2119, 0, 26,
       320, 166, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnIntroPaint);


//*****************************************************************************
//
// The language selection panel, which contains a selection of radio buttons
// for each language.
//
//*****************************************************************************
tContainerWidget g_psRadioContainers[];

tRadioButtonWidget g_psRadioButtons1[] =
{
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 1, 0,
                      &g_sKitronix320x240x16_SSD2119, 10, 50, 120, 45,
                      RB_STYLE_TEXT | RB_STYLE_SELECTED, 16, 0, ClrSilver,
                      ClrSilver, &g_sFontCm20, "English", 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 2, 0,
                      &g_sKitronix320x240x16_SSD2119, 10, 95, 120, 45,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, &g_sFontCm20,
                      "Deutsch", 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 3, 0,
                      &g_sKitronix320x240x16_SSD2119, 180, 50, 120, 45,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, &g_sFontCm20,
                      "Espanol", 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, 0, 0,
                      &g_sKitronix320x240x16_SSD2119, 180, 95, 120, 45,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, &g_sFontCm20,
                      "Italiano", 0, OnRadioChange)
};
#define NUM_RADIO1_BUTTONS      (sizeof(g_psRadioButtons1) /   \
                                 sizeof(g_psRadioButtons1[0]))

tContainerWidget g_psRadioContainers[] =
{
    ContainerStruct(g_psPanels + 1, 0, g_psRadioButtons1,
                    &g_sKitronix320x240x16_SSD2119, 5, 30, 310, 120,
                    CTR_STYLE_OUTLINE | CTR_STYLE_TEXT, 0, ClrGray, ClrSilver,
                    &g_sFontCm20, g_pcLanguage),
};

//*****************************************************************************
//
// An array of canvas widgets, one per panel.  Each canvas is filled with
// black, overwriting the contents of the previous panel.
//
//*****************************************************************************
tCanvasWidget g_psPanels[] =
{
    CanvasStruct(0, 0, &g_sIntroduction, &g_sKitronix320x240x16_SSD2119, 0, 26,
                 320, 166, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, g_psRadioContainers, &g_sKitronix320x240x16_SSD2119, 0,
                 26, 320, 166, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
};

//*****************************************************************************
//
// The number of panels.
//
//*****************************************************************************
#define NUM_PANELS              (sizeof(g_psPanels) / sizeof(g_psPanels[0]))

//*****************************************************************************
//
// The buttons and text across the bottom of the screen.
//
//*****************************************************************************
RectangularButton(g_sPrevious, 0, 0, 0, &g_sKitronix320x240x16_SSD2119, 0, 190,
                  50, 50, PB_STYLE_FILL, ClrBlack, ClrBlack, 0, ClrSilver,
                  &g_sFontCm20, "-", g_pucBlue50x50, g_pucBlue50x50Press, 0, 0,
                  OnPrevious);
Canvas(g_sTitle, 0, 0, 0, &g_sKitronix320x240x16_SSD2119, 50, 190, 220, 50,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE | CANVAS_STYLE_FILL, 0, 0,
       ClrSilver, &g_sFontCm20, 0, 0, 0);
RectangularButton(g_sNext, 0, 0, 0, &g_sKitronix320x240x16_SSD2119, 270, 190,
                  50, 50, PB_STYLE_IMG | PB_STYLE_TEXT, ClrBlack, ClrBlack, 0,
                  ClrSilver, &g_sFontCm20, "+", g_pucBlue50x50,
                  g_pucBlue50x50Press, 0, 0, OnNext);

//*****************************************************************************
//
// The panel that is currently being displayed.
//
//*****************************************************************************
unsigned long g_ulPanel;

//*****************************************************************************
//
// Handles presses of the previous panel button.
//
//*****************************************************************************
void
OnPrevious(tWidget *pWidget)
{
    //
    // There is nothing to be done if the first panel is already being
    // displayed.
    //
    if(g_ulPanel == 0)
    {
        return;
    }

    //
    // Remove the current panel.
    //
    WidgetRemove((tWidget *)(g_psPanels + g_ulPanel));

    //
    // Decrement the panel index.
    //
    g_ulPanel--;

    //
    // Add and draw the new panel.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ulPanel));
    WidgetPaint((tWidget *)(g_psPanels + g_ulPanel));

    //
    // Set the title of this panel.
    //
    GrStringGet(g_ulPanelNames[g_ulPanel], g_pcTitle, TITLE_MAX_SIZE);
    WidgetPaint((tWidget *)&g_sTitle);

    //
    // See if this is the first panel.
    //
    if(g_ulPanel == 0)
    {
        //
        // Clear the previous button from the display since the first panel is
        // being displayed.
        //
        PushButtonImageOff(&g_sPrevious);
        PushButtonTextOff(&g_sPrevious);
        PushButtonFillOn(&g_sPrevious);
        WidgetPaint((tWidget *)&g_sPrevious);
    }

    //
    // See if the previous panel was the last panel.
    //
    if(g_ulPanel == (NUM_PANELS - 2))
    {
        //
        // Display the next button.
        //
        PushButtonImageOn(&g_sNext);
        PushButtonTextOn(&g_sNext);
        PushButtonFillOff(&g_sNext);
        WidgetPaint((tWidget *)&g_sNext);
    }

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);
}

//*****************************************************************************
//
// Handles presses of the next panel button.
//
//*****************************************************************************
void
OnNext(tWidget *pWidget)
{
    //
    // There is nothing to be done if the last panel is already being
    // displayed.
    //
    if(g_ulPanel == (NUM_PANELS - 1))
    {
        return;
    }

    //
    // Remove the current panel.
    //
    WidgetRemove((tWidget *)(g_psPanels + g_ulPanel));

    //
    // Increment the panel index.
    //
    g_ulPanel++;

    //
    // Add and draw the new panel.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psPanels + g_ulPanel));
    WidgetPaint((tWidget *)(g_psPanels + g_ulPanel));

    //
    // Set the title of this panel.
    //
    GrStringGet(g_ulPanelNames[g_ulPanel], g_pcTitle, TITLE_MAX_SIZE);
    WidgetPaint((tWidget *)&g_sTitle);

    //
    // See if the previous panel was the first panel.
    //
    if(g_ulPanel == 1)
    {
        //
        // Display the previous button.
        //
        PushButtonImageOn(&g_sPrevious);
        PushButtonTextOn(&g_sPrevious);
        PushButtonFillOff(&g_sPrevious);
        WidgetPaint((tWidget *)&g_sPrevious);
    }

    //
    // See if this is the last panel.
    //
    if(g_ulPanel == (NUM_PANELS - 1))
    {
        //
        // Clear the next button from the display since the last panel is being
        // displayed.
        //
        PushButtonImageOff(&g_sNext);
        PushButtonTextOff(&g_sNext);
        PushButtonFillOn(&g_sNext);
        WidgetPaint((tWidget *)&g_sNext);
    }

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);
}

//*****************************************************************************
//
// Switch out all of the dynamic strings.
//
//*****************************************************************************
void
ChangeLanguage(unsigned short usLanguage)
{
    //
    // Change the language.
    //
    GrStringLanguageSet(usLanguage);

    //
    // Update the Language string.
    //
    GrStringGet(STR_LANGUAGE, g_pcLanguage, LANGUAGE_MAX_SIZE);

    //
    // Update the title string.
    //
    GrStringGet(g_ulPanelNames[g_ulPanel], g_pcTitle, TITLE_MAX_SIZE);
}

//*****************************************************************************
//
// Handles wrapping a string within an area.
//
// \param pContext is the context of the area to update.
// \param pcString is the string to print out.
// \param lLineHeight is the height of a character in the currrent font.
// \param lx is the x position to start printing this string.
// \param ly is the y position to start printing this string.
//
// \return Returns the number of lines that were printed due to this string.
//
//*****************************************************************************
unsigned long
DrawStringWrapped(tContext *pContext, const char *pcString,
                  long lLineHeight, long lx, long ly)
{
    unsigned long ulWidth;
    unsigned long ulLines;
    int iRemaining, iIdx, iCurrent;

    ulLines = 0;

    //
    // Get the number of characters that will fit across the screen.
    //
    ulWidth = GrContextDpyWidthGet(pContext) - lx;

    //
    // Get the number of characters in the string.
    //
    iRemaining = 0;

    while(pcString[iRemaining++] != 0)
    {
        if(iRemaining >= SCOMP_MAX_STRLEN)
        {
            break;
        }
    }

    while(iRemaining > 0)
    {
        //
        // Reset the current search position.
        //
        iCurrent = 0;

        for(iIdx = 0; iIdx < iRemaining; iIdx++)
        {
            //
            // Found a space.
            //
            if(pcString[iIdx] == ' ')
            {
                //
                // See if the current string will fit if not then its time to
                // print and move the pointer up.
                //
                if(GrStringWidthGet(pContext, pcString, iIdx) < ulWidth)
                {
                    iCurrent = iIdx;
                }
                else
                {
                    break;
                }
            }
        }

        //
        // Handle the case where there are no more remaining characters
        if(iIdx >= iRemaining)
        {
            //
            // See if the current string will fit if not then its time to
            // print and move the pointer up.  Othewise use the previous
            // position and iterate throught he string again.
            //
            if(GrStringWidthGet(pContext, pcString, iIdx) < ulWidth)
            {
                iCurrent = iIdx;
            }
        }

        //
        // If the pointer never advanced then the word was very long and just
        // needs to be printed by itself.  This will crop the word.
        //
        if(iCurrent == 0)
        {
            iCurrent = iIdx;
        }

        //
        // Print the sub-string out to the screen at the current position.
        //
        GrStringDraw(pContext, pcString, iCurrent, lx, ly, 0);

        //
        // Increment the line count and move the y position down by the
        // current font's line height.
        //
        ulLines++;
        ly += lLineHeight;

        //
        // Skip the space if there was one.
        //
        iRemaining -= (iCurrent + 1);
        pcString = &pcString[iCurrent + 1];
    }
    return(ulLines);
}

//*****************************************************************************
//
// Handles paint requests for the introduction canvas widget.
//
//*****************************************************************************
void
OnIntroPaint(tWidget *pWidget, tContext *pContext)
{
    long lLineHeight, lOffset;
    unsigned long ulLines;

    lLineHeight = GrFontHeightGet(&g_sFontCm16);
    lOffset = 26;

    //
    // Display the introduction text in the canvas.
    //
    GrContextFontSet(pContext, &g_sFontCm16);
    GrContextForegroundSet(pContext, ClrSilver);

    //
    // Write the first paragraph of the introduction page.
    //
    GrStringGet(STR_INTRO_1, g_pcBuffer, SCOMP_MAX_STRLEN);
    ulLines = DrawStringWrapped(pContext, g_pcBuffer, lLineHeight, 1,
                                lOffset);
    //
    // Move down by 1/4 of a line between paragraphs.
    //
    lOffset += lLineHeight/4;

    //
    // Write the second paragraph of the introduction page.
    //
    GrStringGet(STR_INTRO_2, g_pcBuffer, SCOMP_MAX_STRLEN);
    ulLines += DrawStringWrapped(pContext, g_pcBuffer, lLineHeight, 1,
                                 lOffset + (ulLines * lLineHeight));
    //
    // Move down by 1/4 of a line between paragraphs.
    //
    lOffset += lLineHeight/4;

    //
    // Write the third paragraph of the introduction page.
    //
    GrStringGet(STR_INTRO_3, g_pcBuffer, SCOMP_MAX_STRLEN);
    DrawStringWrapped(pContext, g_pcBuffer, lLineHeight, 1, lOffset +
        (ulLines * lLineHeight));
}

//*****************************************************************************
//
// Handles change notifications for the radio button widgets.
//
//*****************************************************************************
void
OnRadioChange(tWidget *pWidget, unsigned long bSelected)
{
    unsigned long ulIdx;

    //
    // Find the index of this radio button in the first group.
    //
    for(ulIdx = 0; ulIdx < NUM_RADIO1_BUTTONS; ulIdx++)
    {
        if(pWidget == (tWidget *)(g_psRadioButtons1 + ulIdx))
        {
            break;
        }
    }

    //
    // Change any dynamic language strings.
    //
    ChangeLanguage(g_eLanguageTable[ulIdx]);

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);
}

//*****************************************************************************
//
// A simple demonstration of the features of the Stellaris Graphics Library.
//
//*****************************************************************************
int
main(void)
{
    tContext sContext;
    tRectangle sRect;

    //
    // Set the system clock to run at 50MHz from the PLL
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Set the string table and the default language.
    //
    GrStringTableSet(g_pucTablelanguage);

    //
    // Set the default language.
    //
    ChangeLanguage(GrLangEnUS);

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sKitronix320x240x16_SSD2119);

    //
    // Fill the top 26 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.sYMax = 25;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&sContext, ClrWhite);
    GrRectDraw(&sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, &g_sFontCm20);
    GrStringDrawCentered(&sContext, "lang demo", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 10, 0);

    //
    // Configure and enable uDMA for use by the sound driver.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    uDMAControlBaseSet(&sDMAControlTable[0]);
    uDMAEnable();

    //
    // Initialize the sound driver.
    //
    SoundInit(0);

    //
    // Initialize the touch screen driver and have it route its messages to the
    // widget tree.
    //
    TouchScreenInit();
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the title block and the previous and next buttons to the widget
    // tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sPrevious);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sTitle);
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sNext);

    //
    // Add the first panel to the widget tree.
    //
    g_ulPanel = 0;
    WidgetAdd(WIDGET_ROOT, (tWidget *)g_psPanels);

    //
    // Set the string for the title.
    //
    CanvasTextSet(&g_sTitle, g_pcTitle);

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Loop forever, processing widget messages.
    //
    while(1)
    {
        //
        // Process any messages in the widget message queue.
        //
        WidgetMessageQueueProcess();
    }

}
