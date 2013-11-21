//*****************************************************************************
//
// lang_demo.c - Demonstration of the Stellaris Graphics Library's string
//               table support.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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

#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "driverlib/rom.h"
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
//! The string table and custom fonts used by this application can be found
//! under /third_party/fonts/lang_demo.  The original strings that the
//! application intends displaying are found in the language.csv file (encoded
//! in UTF8 format to allow accented characters and Asian language ideographs
//! to be included.  The mkstringtable tool is used to generate two versions
//! of the string table, one which remains encoded in UTF8 format and the other
//! which has been remapped to a custom codepage allowing the table to be
//! reduced in size compared to the original UTF8 text.  The tool also produces
//! character map files listing each character used in the string table.  These
//! are then provided as input to the ftrasterize tool which generates two
//! custom fonts for the application, one indexed using Unicode and a smaller
//! one indexed using the custom codepage generated for this string table.
//!
//! The command line parameters required for mkstringtable and ftrasterize
//! can be found in the makefile in third_party/fonts/lang_demo.
//!
//! By default, the application builds to use the custom codepage version of
//! the string table and its matching custom font.  To build using the UTF8
//! string table and Unicode-indexed custom font, ensure that the definition of
//! \b USE_REMAPPED_STRINGS at the top of the lang_demo.c source file is
//! commented out.
//!
//*****************************************************************************

//*****************************************************************************
//
// Comment the following line to use a version of the string table and custom
// font that does not use codepage remapping.  In this version, the font is
// somewhat larger and character lookup will be slower but it has the advantage
// that the strings you retrieve from the string table are encoded exactly
// as they were in the original CSV file and are generally readable in your
// debugger (since they use a standard codepage like ISO8859-1 or UTF8).
//
//*****************************************************************************
#define USE_REMAPPED_STRINGS

#ifdef USE_REMAPPED_STRINGS
#include "langremap.h"
extern const unsigned char g_pucCustomr14pt[];
extern const unsigned char g_pucCustomr20pt[];

#define SPACE_CHAR MAP8000_CHAR_000020
#define FONT_20PT (const tFont *)g_pucCustomr20pt
#define FONT_14PT (const tFont *)g_pucCustomr14pt
#define STRING_TABLE g_pucTablelangremap
#define GRLIB_INIT_STRUCT g_GrLibDefaultlangremap

#else

#include "language.h"

extern const unsigned char g_pucCustom14pt[];
extern const unsigned char g_pucCustom20pt[];

#define SPACE_CHAR 0x20
#define FONT_20PT (const tFont *)g_pucCustom20pt
#define FONT_14PT (const tFont *)g_pucCustom14pt

#define STRING_TABLE g_pucTablelanguage
#define GRLIB_INIT_STRUCT g_GrLibDefaultlanguage

#endif

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
typedef struct
{
    unsigned short usLanguage;
    tBoolean bBreakOnSpace;
}
tLanguageParams;

const tLanguageParams g_pLanguageTable[] =
{
    { GrLangEnUS, true },
    { GrLangDe, true },
    { GrLangEsSP, true },
    { GrLangIt, true },
    { GrLangZhPRC, false },
    { GrLangKo, true },
    { GrLangJp, false }
};

#define NUM_LANGUAGES (sizeof(g_LanguageTable) / sizeof(tLanguageParams))

//*****************************************************************************
//
// The index of the current language in the g_pLanguageTable array.
//
//*****************************************************************************
unsigned long g_ulLangIdx;

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
       320, 164, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnIntroPaint);

//*****************************************************************************
//
// Storage for language name strings.  Note that we could hardcode these into
// the relevant widget initialization macros but since we may be using a
// custom font and remapped codepage, keeping the strings in the string table
// and loading them when the app starts is likely to create less confusion and
// prevents the risk of seeing garbled output if you accidentally use ASCII or
// ISO8859-1 text strings with the custom font.
//
//*****************************************************************************
#define MAX_LANGUAGE_NAME_LEN 10
char g_pcEnglish[MAX_LANGUAGE_NAME_LEN];
char g_pcDeutsch[MAX_LANGUAGE_NAME_LEN];
char g_pcEspanol[MAX_LANGUAGE_NAME_LEN];
char g_pcItaliano[MAX_LANGUAGE_NAME_LEN];
char g_pcChinese[MAX_LANGUAGE_NAME_LEN];
char g_pcKorean[MAX_LANGUAGE_NAME_LEN];
char g_pcJapanese[MAX_LANGUAGE_NAME_LEN];

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
                      &g_sKitronix320x240x16_SSD2119, 10, 54, 120, 25,
                      RB_STYLE_TEXT | RB_STYLE_SELECTED, 16, 0, ClrSilver,
                      ClrSilver, FONT_20PT, g_pcEnglish, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 2, 0,
                      &g_sKitronix320x240x16_SSD2119, 10, 82, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcDeutsch, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 3, 0,
                      &g_sKitronix320x240x16_SSD2119, 180, 54, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcEspanol, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 4, 0,
                      &g_sKitronix320x240x16_SSD2119, 180, 82, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcItaliano, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 5, 0,
                      &g_sKitronix320x240x16_SSD2119, 10, 110, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcChinese, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 6, 0,
                      &g_sKitronix320x240x16_SSD2119, 180, 110, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcKorean, 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, 0, 0,
                      &g_sKitronix320x240x16_SSD2119, 10, 138, 120, 25,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, FONT_20PT,
                      g_pcJapanese, 0, OnRadioChange),
};

#define NUM_RADIO1_BUTTONS      (sizeof(g_psRadioButtons1) /   \
                                 sizeof(g_psRadioButtons1[0]))

tContainerWidget g_psRadioContainers[] =
{
    ContainerStruct(g_psPanels + 1, 0, g_psRadioButtons1,
                    &g_sKitronix320x240x16_SSD2119, 5, 30, 310, 150,
                    CTR_STYLE_OUTLINE | CTR_STYLE_TEXT, 0, ClrGray, ClrSilver,
                    FONT_20PT, g_pcLanguage),
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
                 320, 164, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, g_psRadioContainers, &g_sKitronix320x240x16_SSD2119, 0,
                 26, 320, 164, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
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
char g_pcPlus[2];
char g_pcMinus[2];

RectangularButton(g_sPrevious, 0, 0, 0, &g_sKitronix320x240x16_SSD2119, 0, 190,
                  50, 50, PB_STYLE_FILL, ClrBlack, ClrBlack, 0, ClrSilver,
                  FONT_20PT, g_pcMinus, g_pucBlue50x50, g_pucBlue50x50Press, 0, 0,
                  OnPrevious);
Canvas(g_sTitle, 0, 0, 0, &g_sKitronix320x240x16_SSD2119, 50, 190, 220, 50,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE | CANVAS_STYLE_FILL, 0, 0,
       ClrSilver, FONT_20PT, 0, 0, 0);
RectangularButton(g_sNext, 0, 0, 0, &g_sKitronix320x240x16_SSD2119, 270, 190,
                  50, 50, PB_STYLE_IMG | PB_STYLE_TEXT, ClrBlack, ClrBlack, 0,
                  ClrSilver, FONT_20PT, g_pcPlus, g_pucBlue50x50,
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
// \param bSplitOnSpace is true if strings in the current language must be
// split only on space characters or false if they may be split between any
// two characters.
//
// \return Returns the number of lines that were printed due to this string.
//
//*****************************************************************************
unsigned long
DrawStringWrapped(tContext *pContext, char *pcString,
                  long lLineHeight, long lx, long ly, tBoolean bSplitOnSpace)
{
    unsigned long ulWidth, ulCharWidth, ulStrWidth, ulChar;
    unsigned long ulLines, ulSkip;
    char *pcStart, *pcEnd;
    char *pcLastSpace;

    ulLines = 0;

    //
    // Get the number of pixels we have to fit the string into across the
    // screen.
    //
    ulWidth = GrContextDpyWidthGet(pContext) - lx;

    //
    // Get a pointer to the terminating zero.
    //
    pcEnd = pcString;
    while(*pcEnd)
    {
        pcEnd++;
    }

    //
    // The first substring we draw will start at the beginning of the string.
    //
    pcStart = pcString;
    pcLastSpace = pcString;
    ulStrWidth = 0;

    //
    // Keep processing until we have no more characters to display.
    //
    do
    {
        //
        // Get the next character in the string.
        //
        ulChar = GrStringNextCharGet(pContext, pcString, (pcEnd - pcString),
                                     &ulSkip);

        //
        // Did we reach the end of the string?
        //
        if(ulChar)
        {
            //
            // No - how wide is this character?
            //
            ulCharWidth = GrStringWidthGet(pContext, pcString, ulSkip);

            //
            // Have we run off the edge of the display?
            //
            if((ulStrWidth + ulCharWidth) > ulWidth)
            {
                //
                // If we are splitting on spaces, rewind the string pointer to
                // the byte after the last space.
                //
                if(bSplitOnSpace)
                {
                    pcString = pcLastSpace;
                }

                //
                // Yes - draw the substring.
                //
                GrStringDraw(pContext, pcStart, (pcString - pcStart), lx, ly,
                             0);

                //
                // Increment the line count and move the y position down by the
                // current font's line height.
                //
                ulLines++;
                ly += lLineHeight;
                ulStrWidth = 0;

                //
                // The next string we draw will start at the current position.
                //
                pcStart = pcString;
            }
            else
            {
                //
                // No - update the width and move on to the next character.
                //
                ulStrWidth += ulCharWidth;
                pcString += ulSkip;

                //
                // If this is a space, remember where we are.
                //
                if(ulChar == SPACE_CHAR)
                {
                    pcLastSpace = pcString;
                }
            }
        }
        else
        {
            //
            // Do we have any remaining chunk of string to draw?
            //
            if(pcStart != pcString)
            {
                //
                // Yes - draw the last section of string.
                //
                GrStringDraw(pContext, pcStart, -1, lx, ly, 0);
                ulLines++;
            }
        }
    } while(ulChar);

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

    lLineHeight = GrFontHeightGet(FONT_14PT);
    lOffset = 32;

    //
    // Display the introduction text in the canvas.
    //
    GrContextFontSet(pContext, FONT_14PT);
    GrContextForegroundSet(pContext, ClrSilver);

    //
    // Write the first paragraph of the introduction page.
    //
    GrStringGet(STR_INTRO_1, g_pcBuffer, SCOMP_MAX_STRLEN);
    ulLines = DrawStringWrapped(pContext, g_pcBuffer, lLineHeight, 1,
                                lOffset,
                                g_pLanguageTable[g_ulLangIdx].bBreakOnSpace );
    //
    // Move down by 1/4 of a line between paragraphs.
    //
    lOffset += lLineHeight/4;

    //
    // Write the second paragraph of the introduction page.
    //
    GrStringGet(STR_INTRO_2, g_pcBuffer, SCOMP_MAX_STRLEN);
    ulLines += DrawStringWrapped(pContext, g_pcBuffer, lLineHeight, 1,
                                 lOffset + (ulLines * lLineHeight),
                                 g_pLanguageTable[g_ulLangIdx].bBreakOnSpace );
    //
    // Move down by 1/4 of a line between paragraphs.
    //
    lOffset += lLineHeight/4;

    //
    // Write the third paragraph of the introduction page.
    //
    GrStringGet(STR_INTRO_3, g_pcBuffer, SCOMP_MAX_STRLEN);
    DrawStringWrapped(pContext, g_pcBuffer, lLineHeight, 1, lOffset +
        (ulLines * lLineHeight),
        g_pLanguageTable[g_ulLangIdx].bBreakOnSpace );
}

//*****************************************************************************
//
// Handles change notifications for the radio button widgets.
//
//*****************************************************************************
void
OnRadioChange(tWidget *pWidget, unsigned long bSelected)
{
    //
    // Find the index of this radio button in the first group.
    //
    for(g_ulLangIdx = 0; g_ulLangIdx < NUM_RADIO1_BUTTONS; g_ulLangIdx++)
    {
        if(pWidget == (tWidget *)(g_psRadioButtons1 + g_ulLangIdx))
        {
            break;
        }
    }

    //
    // Change any dynamic language strings.
    //
    ChangeLanguage(g_pLanguageTable[g_ulLangIdx].usLanguage);

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
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
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
    // Set graphics library text rendering defaults.
    //
    GrLibInit(&GRLIB_INIT_STRUCT);

    //
    // Set the string table and the default language.
    //
    GrStringTableSet(STRING_TABLE);

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
    // Load the static strings from the string table.  These strings are
    // independent of the language in use but we store them in the string
    // table nonetheless since (a) we may be using codepage remapping in
    // which case it would be difficult to hardcode them into the app source
    // anyway (ASCII or ISO8859-1 text would not render properly with the
    // remapped custom font) and (b) even if we're not using codepage remapping,
    // we may have generated a custom font from the string table output and
    // we want to make sure that all glyphs required by the application are
    // present in that font.  If we hardcode some text in the application
    // source and don't put it in the string table, we run the risk of having
    // characters missing in the font.
    //
    GrStringGet(STR_ENGLISH, g_pcEnglish, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_DEUTSCH, g_pcDeutsch, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_ESPANOL, g_pcEspanol, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_ITALIANO, g_pcItaliano, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_CHINESE, g_pcChinese, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_KOREAN, g_pcKorean, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_JAPANESE, g_pcJapanese, MAX_LANGUAGE_NAME_LEN);
    GrStringGet(STR_PLUS, g_pcPlus, 2);
    GrStringGet(STR_MINUS, g_pcMinus, 2);

    //
    // Put the application name in the middle of the banner.
    //
    GrStringGet(STR_APPNAME, g_pcBuffer, SCOMP_MAX_STRLEN);
    GrContextFontSet(&sContext, FONT_20PT);
    GrStringDrawCentered(&sContext, g_pcBuffer, -1,
                         GrContextDpyWidthGet(&sContext) / 2, 10, 0);

    //
    // Configure and enable uDMA for use by the sound driver.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    ROM_uDMAControlBaseSet(&sDMAControlTable[0]);
    ROM_uDMAEnable();

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
