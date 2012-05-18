//*****************************************************************************
//
// grlib_demo.c - Demonstration of the Stellaris Graphics Library.
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
// This is part of revision 8555 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/checkbox.h"
#include "grlib/container.h"
#include "grlib/pushbutton.h"
#include "grlib/radiobutton.h"
#include "grlib/slider.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/ustdlib.h"
#include "drivers/formike240x320x16_ili9320.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "images.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Graphics Library Demonstration (grlib_demo)</h1>
//!
//! This application provides a demonstration of the capabilities of the
//! Stellaris Graphics Library.  A series of panels show different features of
//! the library.  For each panel, the bottom provides a forward and back button
//! (when appropriate), along with a brief description of the contents of the
//! panel.
//!
//! The first panel provides some introductory text and basic instructions for
//! operation of the application.
//!
//! The second panel shows the available drawing primitives: lines, circles,
//! rectangles, strings, and images.
//!
//! The third panel shows the canvas widget, which provides a general drawing
//! surface within the widget hierarchy.  A text, image, and application-drawn
//! canvas are displayed.
//!
//! The fourth panel shows the check box widget, which provides a means of
//! toggling the state of an item.  Four check boxes are provided, with each
//! having a red ``LED'' to the right.  The state of the LED tracks the state
//! of the check box via an application callback.
//!
//! The fifth panel shows the container widget, which provides a grouping
//! construct typically used for radio buttons.  Containers with a title, a
//! centered title, and no title are displayed.
//!
//! The sixth panel shows the push button widget.  Two columns of push buttons
//! are provided; the appearance of each column is the same but the left column
//! does not utilize auto-repeat while the right column does.  Each push button
//! has a red ``LED'' to its left, which is toggled via an application callback
//! each time the push button is pressed.
//!
//! The seventh panel shows the radio button widget.  Two groups of radio
//! buttons are displayed, the first using text and the second using images for
//! the selection value.  Each radio button has a red ``LED'' to its right,
//! which tracks the selection state of the radio buttons via an application
//! callback.  Only one radio button from each group can be selected at a time,
//! though the radio buttons in each group operate independently.
//!
//! The eighth panel shows the slider widget.  Six sliders constructed using
//! the various supported style options are shown.  The slider value callback
//! is used to update two widgets to reflect the values reported by sliders.
//! A canvas widget in the top right of the display tracks the value of the
//! red and green image-based slider to its left and the text of the grey slider
//! on the left side of the panel is update to show its own value.  The
//! rightmost slider is configured as an indicator which tracks the state of
//! the upper slider and ignores user input.
//!
//! The final panel provides instructions and information necessary to update
//! the board firmware via ethernet using the LM Flash Programmer application.
//! When using a version of LM Flash Programmer with a build number greater
//! than 560, software updates will occur automatically without user
//! intervention being required in the application.  If using an earlier
//! version of LM Flash Programmer which does not send the ``magic packet''
//! signalling an update request, the ``Update'' button on the final screen may
//! be pressed to transfer control to the boot loader in preparation for a
//! firmware download.
//
//*****************************************************************************

//*****************************************************************************
//
// A global flag used to indicate if a remote firmware update request has been
// received.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate = false;

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
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{

    //
    // Call the lwIP timer.
    //
    lwIPTimer(1000 / TICKS_PER_SECOND);
}

//*****************************************************************************
//
// This function is called by the software update module whenever a remote
// host requests to update the firmware on this board.  We set a flag that
// will cause the bootloader to be entered the next time the user enters a
// command on the console.
//
//*****************************************************************************
void SoftwareUpdateRequestCallback(void)
{
    g_bFirmwareUpdate = true;
}

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
void OnFirmwarePaint(tWidget *pWidget, tContext *pContext);
void OnFirmwareUpdate(tWidget *pWidget);
void OnPrimitivePaint(tWidget *pWidget, tContext *pContext);
void OnCanvasPaint(tWidget *pWidget, tContext *pContext);
void OnCheckChange(tWidget *pWidget, unsigned long bSelected);
void OnButtonPress(tWidget *pWidget);
void OnRadioChange(tWidget *pWidget, unsigned long bSelected);
void OnSliderChange(tWidget *pWidget, long lValue);
extern tCanvasWidget g_psPanels[];

//*****************************************************************************
//
// The first panel, which contains introductory text explaining the
// application.
//
//*****************************************************************************
Canvas(g_sIntroduction, g_psPanels, 0, 0, &g_sFormike240x320x16_ILI9320, 0, 24,
       240, 246, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnIntroPaint);

//*****************************************************************************
//
// The second panel, which demonstrates the graphics primitives.
//
//*****************************************************************************
Canvas(g_sPrimitives, g_psPanels + 1, 0, 0, &g_sFormike240x320x16_ILI9320, 0,
       24, 240, 246, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0,
       OnPrimitivePaint);

//*****************************************************************************
//
// The third panel, which demonstrates the canvas widget.
//
//*****************************************************************************
Canvas(g_sCanvas3, g_psPanels + 2, 0, 0, &g_sFormike240x320x16_ILI9320, 5, 191,
       230, 76, CANVAS_STYLE_OUTLINE | CANVAS_STYLE_APP_DRAWN, 0, ClrGray, 0,
       0, 0, 0, OnCanvasPaint);
Canvas(g_sCanvas2, g_psPanels + 2, &g_sCanvas3, 0,
       &g_sFormike240x320x16_ILI9320, 5, 109, 230, 76,
       CANVAS_STYLE_OUTLINE | CANVAS_STYLE_IMG, 0, ClrGray, 0, 0, 0, g_pucLogo,
       0);
Canvas(g_sCanvas1, g_psPanels + 2, &g_sCanvas2, 0,
       &g_sFormike240x320x16_ILI9320, 5, 27, 230, 76,
       CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT,
       ClrMidnightBlue, ClrGray, ClrSilver, g_pFontCm22, "Text", 0, 0);

//*****************************************************************************
//
// The fourth panel, which demonstrates the checkbox widget.
//
//*****************************************************************************
tCanvasWidget g_psCheckBoxIndicators[] =
{
    CanvasStruct(g_psPanels + 3, g_psCheckBoxIndicators + 1, 0,
                 &g_sFormike240x320x16_ILI9320, 190, 30, 50, 50,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psPanels + 3, g_psCheckBoxIndicators + 2, 0,
                 &g_sFormike240x320x16_ILI9320, 190, 90, 50, 50,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psPanels + 3, g_psCheckBoxIndicators + 3, 0,
                 &g_sFormike240x320x16_ILI9320, 190, 150, 50, 50,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psPanels + 3, 0, 0,
                 &g_sFormike240x320x16_ILI9320, 190, 210, 50, 50,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0)
};
tCheckBoxWidget g_psCheckBoxes[] =
{
    CheckBoxStruct(g_psPanels + 3, g_psCheckBoxes + 1, 0,
                   &g_sFormike240x320x16_ILI9320, 0, 30, 185, 50,
                   CB_STYLE_OUTLINE | CB_STYLE_FILL | CB_STYLE_TEXT, 16,
                   ClrMidnightBlue, ClrGray, ClrSilver, g_pFontCm22, "Select",
                   0, OnCheckChange),
    CheckBoxStruct(g_psPanels + 3, g_psCheckBoxes + 2, 0,
                   &g_sFormike240x320x16_ILI9320, 0, 90, 185, 50,
                   CB_STYLE_IMG, 16, 0, ClrGray, 0, 0, 0, g_pucLogo,
                   OnCheckChange),
    CheckBoxStruct(g_psPanels + 3, g_psCheckBoxes + 3, 0,
                   &g_sFormike240x320x16_ILI9320, 0, 150, 185, 50,
                   CB_STYLE_OUTLINE | CB_STYLE_TEXT, 32, 0,
                   ClrGreen, ClrSpringGreen, g_pFontCm22, "Select",
                   0, OnCheckChange),
    CheckBoxStruct(g_psPanels + 3, g_psCheckBoxIndicators, 0,
                   &g_sFormike240x320x16_ILI9320, 0, 210, 185, 50,
                   CB_STYLE_IMG, 32, 0, ClrDarkRed, 0, 0, 0, g_pucLogo,
                   OnCheckChange)
};
#define NUM_CHECK_BOXES         (sizeof(g_psCheckBoxes) /   \
                                 sizeof(g_psCheckBoxes[0]))

//*****************************************************************************
//
// The fifth panel, which demonstrates the container widget.
//
//*****************************************************************************
Container(g_sContainer3, g_psPanels + 4, 0, 0, &g_sFormike240x320x16_ILI9320,
          5, 191, 230, 76, CTR_STYLE_OUTLINE | CTR_STYLE_FILL, ClrMidnightBlue,
          ClrGray, 0, 0, 0);
Container(g_sContainer2, g_psPanels + 4, &g_sContainer3, 0,
          &g_sFormike240x320x16_ILI9320, 5, 109, 230, 76,
          (CTR_STYLE_OUTLINE | CTR_STYLE_FILL | CTR_STYLE_TEXT |
           CTR_STYLE_TEXT_CENTER), ClrMidnightBlue, ClrGray, ClrSilver,
          g_pFontCm22, "Group2");
Container(g_sContainer1, g_psPanels + 4, &g_sContainer2, 0,
          &g_sFormike240x320x16_ILI9320, 5, 27, 230, 76,
          CTR_STYLE_OUTLINE | CTR_STYLE_FILL | CTR_STYLE_TEXT, ClrMidnightBlue,
          ClrGray, ClrSilver, g_pFontCm22, "Group1");

//*****************************************************************************
//
// The sixth panel, which contains a selection of push buttons.
//
//*****************************************************************************
tCanvasWidget g_psPushButtonIndicators[] =
{
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 1, 0,
                 &g_sFormike240x320x16_ILI9320, 5, 45, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 2, 0,
                 &g_sFormike240x320x16_ILI9320, 125, 45, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 3, 0,
                 &g_sFormike240x320x16_ILI9320, 5, 105, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 4, 0,
                 &g_sFormike240x320x16_ILI9320, 125, 105, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 5, 0,
                 &g_sFormike240x320x16_ILI9320, 5, 165, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 6, 0,
                 &g_sFormike240x320x16_ILI9320, 125, 165, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 7, 0,
                 &g_sFormike240x320x16_ILI9320, 5, 205, 110, 24,
                 CANVAS_STYLE_TEXT, 0, 0, ClrSilver, g_pFontCm20, "Non-auto",
                 0, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 8, 0,
                 &g_sFormike240x320x16_ILI9320, 5, 225, 110, 24,
                 CANVAS_STYLE_TEXT, 0, 0, ClrSilver, g_pFontCm20, "repeat",
                 0, 0),
    CanvasStruct(g_psPanels + 5, g_psPushButtonIndicators + 9, 0,
                 &g_sFormike240x320x16_ILI9320, 125, 205, 110, 24,
                 CANVAS_STYLE_TEXT, 0, 0, ClrSilver, g_pFontCm20, "Auto",
                 0, 0),
    CanvasStruct(g_psPanels + 5, 0, 0,
                 &g_sFormike240x320x16_ILI9320, 125, 225, 110, 24,
                 CANVAS_STYLE_TEXT, 0, 0, ClrSilver, g_pFontCm20, "repeat",
                 0, 0),
};
tPushButtonWidget g_psPushButtons[] =
{
    RectangularButtonStruct(g_psPanels + 5, g_psPushButtons + 1, 0,
                            &g_sFormike240x320x16_ILI9320, 30, 30, 50, 50,
                            PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
                            ClrMidnightBlue, ClrBlack, ClrGray, ClrSilver,
                            g_pFontCm22, "1", 0, 0, 0, 0, OnButtonPress),
    RectangularButtonStruct(g_psPanels + 5, g_psPushButtons + 2, 0,
                            &g_sFormike240x320x16_ILI9320, 150, 30, 50, 50,
                            (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT |
                             PB_STYLE_AUTO_REPEAT), ClrMidnightBlue, ClrBlack,
                            ClrGray, ClrSilver, g_pFontCm22, "2", 0, 0, 125,
                            25, OnButtonPress),
    CircularButtonStruct(g_psPanels + 5, g_psPushButtons + 3, 0,
                         &g_sFormike240x320x16_ILI9320, 55, 115, 25,
                         PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT,
                         ClrMidnightBlue, ClrBlack, ClrGray, ClrSilver,
                         g_pFontCm22, "3", 0, 0, 0, 0, OnButtonPress),
    CircularButtonStruct(g_psPanels + 5, g_psPushButtons + 4, 0,
                         &g_sFormike240x320x16_ILI9320, 175, 115, 25,
                         (PB_STYLE_FILL | PB_STYLE_OUTLINE | PB_STYLE_TEXT |
                          PB_STYLE_AUTO_REPEAT), ClrMidnightBlue, ClrBlack,
                         ClrGray, ClrSilver, g_pFontCm22, "4", 0, 0, 125, 25,
                         OnButtonPress),
    RectangularButtonStruct(g_psPanels + 5, g_psPushButtons + 5, 0,
                            &g_sFormike240x320x16_ILI9320, 30, 150, 50, 50,
                            PB_STYLE_IMG | PB_STYLE_TEXT, 0, 0, 0, ClrSilver,
                            g_pFontCm22, "5", g_pucBlue50x50,
                            g_pucBlue50x50Press, 0, 0, OnButtonPress),
    RectangularButtonStruct(g_psPanels + 5, g_psPushButtonIndicators, 0,
                            &g_sFormike240x320x16_ILI9320, 150, 150, 50, 50,
                            (PB_STYLE_IMG | PB_STYLE_TEXT |
                             PB_STYLE_AUTO_REPEAT), 0, 0, 0, ClrSilver,
                            g_pFontCm22, "6", g_pucBlue50x50,
                            g_pucBlue50x50Press, 125, 25, OnButtonPress),
};
#define NUM_PUSH_BUTTONS        (sizeof(g_psPushButtons) /   \
                                 sizeof(g_psPushButtons[0]))
unsigned long g_ulButtonState;

//*****************************************************************************
//
// The seventh panel, which contains a selection of radio buttons.
//
//*****************************************************************************
tContainerWidget g_psRadioContainers[];
tCanvasWidget g_psRadioButtonIndicators[] =
{
    CanvasStruct(g_psRadioContainers, g_psRadioButtonIndicators + 1, 0,
                 &g_sFormike240x320x16_ILI9320, 95, 62, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psRadioContainers, g_psRadioButtonIndicators + 2, 0,
                 &g_sFormike240x320x16_ILI9320, 95, 107, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psRadioContainers, g_psRadioButtonIndicators + 3, 0,
                 &g_sFormike240x320x16_ILI9320, 210, 62, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psRadioContainers, 0, 0,
                 &g_sFormike240x320x16_ILI9320, 210, 107, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psRadioContainers + 1, g_psRadioButtonIndicators + 5, 0,
                 &g_sFormike240x320x16_ILI9320, 95, 177, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psRadioContainers + 1, g_psRadioButtonIndicators + 6, 0,
                 &g_sFormike240x320x16_ILI9320, 95, 222, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psRadioContainers + 1, g_psRadioButtonIndicators + 7, 0,
                 &g_sFormike240x320x16_ILI9320, 210, 177, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
    CanvasStruct(g_psRadioContainers + 1, 0, 0,
                 &g_sFormike240x320x16_ILI9320, 210, 222, 20, 20,
                 CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucLightOff, 0),
};
tRadioButtonWidget g_psRadioButtons1[] =
{
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 1, 0,
                      &g_sFormike240x320x16_ILI9320, 10, 50, 80, 45,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, g_pFontCm20,
                      "One", 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 2, 0,
                      &g_sFormike240x320x16_ILI9320, 10, 95, 80, 45,
                      RB_STYLE_TEXT, 16, 0, ClrSilver, ClrSilver, g_pFontCm20,
                      "Two", 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtons1 + 3, 0,
                      &g_sFormike240x320x16_ILI9320, 125, 50, 80, 45,
                      RB_STYLE_TEXT, 24, 0, ClrSilver, ClrSilver, g_pFontCm20,
                      "Three", 0, OnRadioChange),
    RadioButtonStruct(g_psRadioContainers, g_psRadioButtonIndicators, 0,
                      &g_sFormike240x320x16_ILI9320, 125, 95, 80, 45,
                      RB_STYLE_TEXT, 24, 0, ClrSilver, ClrSilver, g_pFontCm20,
                      "Four", 0, OnRadioChange)
};
#define NUM_RADIO1_BUTTONS      (sizeof(g_psRadioButtons1) /   \
                                 sizeof(g_psRadioButtons1[0]))
tRadioButtonWidget g_psRadioButtons2[] =
{
    RadioButtonStruct(g_psRadioContainers + 1, g_psRadioButtons2 + 1, 0,
                      &g_sFormike240x320x16_ILI9320, 10, 165, 80, 45,
                      RB_STYLE_IMG, 16, 0, ClrSilver, 0, 0, 0, g_pucLogo,
                      OnRadioChange),
    RadioButtonStruct(g_psRadioContainers + 1, g_psRadioButtons2 + 2, 0,
                      &g_sFormike240x320x16_ILI9320, 10, 210, 80, 45,
                      RB_STYLE_IMG, 24, 0, ClrSilver, 0, 0, 0, g_pucLogo,
                      OnRadioChange),
    RadioButtonStruct(g_psRadioContainers + 1, g_psRadioButtons2 + 3, 0,
                      &g_sFormike240x320x16_ILI9320, 125, 165, 80, 45,
                      RB_STYLE_IMG, 16, 0, ClrSilver, 0, 0, 0, g_pucLogo,
                      OnRadioChange),
    RadioButtonStruct(g_psRadioContainers + 1, g_psRadioButtonIndicators + 4,
                      0, &g_sFormike240x320x16_ILI9320, 125, 210, 80, 45,
                      RB_STYLE_IMG, 24, 0, ClrSilver, 0, 0, 0, g_pucLogo,
                      OnRadioChange)
};
#define NUM_RADIO2_BUTTONS      (sizeof(g_psRadioButtons2) /   \
                                 sizeof(g_psRadioButtons2[0]))
tContainerWidget g_psRadioContainers[] =
{
    ContainerStruct(g_psPanels + 6, g_psRadioContainers + 1, g_psRadioButtons1,
                    &g_sFormike240x320x16_ILI9320, 5, 30, 230, 111,
                    CTR_STYLE_OUTLINE | CTR_STYLE_TEXT, 0, ClrGray, ClrSilver,
                    g_pFontCm20, "Group One"),
    ContainerStruct(g_psPanels + 6, 0, g_psRadioButtons2,
                    &g_sFormike240x320x16_ILI9320, 5, 145, 230, 111,
                    CTR_STYLE_OUTLINE | CTR_STYLE_TEXT, 0, ClrGray, ClrSilver,
                    g_pFontCm20, "Group Two")
};

//*****************************************************************************
//
// The eighth panel, which demonstrates the slider widget.
//
//*****************************************************************************
Canvas(g_sSliderValueCanvas, g_psPanels + 7, 0, 0,
       &g_sFormike240x320x16_ILI9320, 200, 40, 40, 30,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, ClrBlack, 0, ClrSilver,
       g_pFontCm20, "50%",
       0, 0);

tSliderWidget g_psSliders[] =
{
    SliderStruct(g_psPanels + 7, g_psSliders + 1, 0,
                 &g_sFormike240x320x16_ILI9320, 5, 153, 150, 30, 0, 100, 25,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT | SL_STYLE_BACKG_TEXT),
                 ClrGray, ClrBlack, ClrSilver, ClrWhite, ClrWhite,
                 g_pFontCm20, "25%", 0, 0, OnSliderChange),
    SliderStruct(g_psPanels + 7, g_psSliders + 2, 0,
                 &g_sFormike240x320x16_ILI9320, 5, 210, 150, 30, 0, 100, 25,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_OUTLINE |
                  SL_STYLE_TEXT),
                 ClrWhite, ClrBlueViolet, ClrSilver, ClrBlack, 0,
                 g_pFontCm20, "Foreground", 0, 0, OnSliderChange),
    SliderStruct(g_psPanels + 7, g_psSliders + 3, 0,
                 &g_sFormike240x320x16_ILI9320, 205, 90, 30, 160, 0, 100, 50,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_VERTICAL |
                  SL_STYLE_OUTLINE | SL_STYLE_LOCKED), ClrDarkGreen,
                  ClrDarkRed, ClrSilver, 0, 0, 0, 0, 0, 0, 0),
    SliderStruct(g_psPanels + 7, g_psSliders + 4, 0,
                 &g_sFormike240x320x16_ILI9320, 165, 90, 30, 162, 0, 100, 75,
                 (SL_STYLE_IMG | SL_STYLE_BACKG_IMG | SL_STYLE_VERTICAL |
                 SL_STYLE_OUTLINE), 0, ClrBlack, ClrSilver, 0, 0, 0,
                 0, g_pucGettingHotter28x160, g_pucGettingHotter28x160Mono,
                 OnSliderChange),
    SliderStruct(g_psPanels + 7, g_psSliders + 5, 0,
                 &g_sFormike240x320x16_ILI9320, 0, 40, 195, 37, 0, 100, 50,
                 SL_STYLE_IMG | SL_STYLE_BACKG_IMG, 0, 0, 0, 0, 0, 0,
                 0, g_pucGreenSlider195x37, g_pucRedSlider195x37,
                 OnSliderChange),
    SliderStruct(g_psPanels + 7, &g_sSliderValueCanvas, 0,
                 &g_sFormike240x320x16_ILI9320, 5, 96, 150, 30, 0, 100, 50,
                 (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_TEXT |
                  SL_STYLE_BACKG_TEXT | SL_STYLE_TEXT_OPAQUE |
                  SL_STYLE_BACKG_TEXT_OPAQUE),
                 ClrBlue, ClrYellow, ClrSilver, ClrYellow, ClrBlue,
                 g_pFontCm20, "Slider with text", 0, 0, OnSliderChange),
};

#define SLIDER_TEXT_VAL_INDEX   0
#define SLIDER_LOCKED_INDEX     2
#define SLIDER_CANVAS_VAL_INDEX 4

#define NUM_SLIDERS (sizeof(g_psSliders) / sizeof(g_psSliders[0]))

//*****************************************************************************
//
// The ninth panel, which contains text describing how to perform a firmware
// update and a button to initiate the process.
//
//*****************************************************************************
Canvas(g_sFirmwareUpdate, g_psPanels + 8, 0, 0, &g_sFormike240x320x16_ILI9320,
       0, 24, 240, 246, CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0,
       OnFirmwarePaint);
RectangularButton(g_sFirmwareUpdateBtn, g_psPanels + 8, &g_sFirmwareUpdate,
                  0, &g_sFormike240x320x16_ILI9320, 50, 200, 140, 40,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL), ClrNavy, ClrBlue, ClrSilver, ClrSilver,
                   g_pFontCm20, "Update", 0, 0, 0, 0, OnFirmwareUpdate);

//*****************************************************************************
//
// An array of canvas widgets, one per panel.  Each canvas is filled with
// black, overwriting the contents of the previous panel.
//
//*****************************************************************************
tCanvasWidget g_psPanels[] =
{
    CanvasStruct(0, 0, &g_sIntroduction, &g_sFormike240x320x16_ILI9320, 0, 24,
                 240, 246, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_sPrimitives, &g_sFormike240x320x16_ILI9320, 0, 24,
                 240, 246, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_sCanvas1, &g_sFormike240x320x16_ILI9320, 0, 24, 240,
                 246, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, g_psCheckBoxes, &g_sFormike240x320x16_ILI9320, 0, 24,
                 240, 246, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_sContainer1, &g_sFormike240x320x16_ILI9320, 0, 24,
                 240, 246, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, g_psPushButtons, &g_sFormike240x320x16_ILI9320, 0, 24,
                 240, 246, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, g_psRadioContainers, &g_sFormike240x320x16_ILI9320, 0,
                 24, 240, 246, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, g_psSliders, &g_sFormike240x320x16_ILI9320, 0, 24, 240,
                 246, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, &g_sFirmwareUpdateBtn, &g_sFormike240x320x16_ILI9320, 0,
                 24, 240, 246, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0)
};

//*****************************************************************************
//
// The number of panels.
//
//*****************************************************************************
#define NUM_PANELS              (sizeof(g_psPanels) / sizeof(g_psPanels[0]))

//*****************************************************************************
//
// The names for each of the panels, which is displayed at the bottom of the
// screen.
//
//*****************************************************************************
char *g_pcPanelNames[] =
{
    "     Introduction     ",
    "     Primitives     ",
    "     Canvas     ",
    "     Checkbox     ",
    "     Container     ",
    "     Push Buttons     ",
    "     Radio Buttons     ",
    "     Sliders     ",
    "     S/W Update    "
};

//*****************************************************************************
//
// The buttons and text across the bottom of the screen.
//
//*****************************************************************************
RectangularButton(g_sPrevious, 0, 0, 0, &g_sFormike240x320x16_ILI9320, 0, 270,
                  50, 50, PB_STYLE_FILL, ClrBlack, ClrBlack, 0, ClrSilver,
                  g_pFontCm20, "-", g_pucBlue50x50, g_pucBlue50x50Press, 0, 0,
                  OnPrevious);
Canvas(g_sTitle, 0, 0, 0, &g_sFormike240x320x16_ILI9320, 50, 270, 140, 50,
       CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE, 0, 0, ClrSilver,
       g_pFontCm20, 0, 0, 0);
RectangularButton(g_sNext, 0, 0, 0, &g_sFormike240x320x16_ILI9320, 190, 270,
                  50, 50, PB_STYLE_IMG | PB_STYLE_TEXT, ClrBlack, ClrBlack, 0,
                  ClrSilver, g_pFontCm20, "+", g_pucBlue50x50,
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
    CanvasTextSet(&g_sTitle, g_pcPanelNames[g_ulPanel]);
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
    CanvasTextSet(&g_sTitle, g_pcPanelNames[g_ulPanel]);
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
// Handles paint requests for the introduction canvas widget.
//
//*****************************************************************************
void
OnIntroPaint(tWidget *pWidget, tContext *pContext)
{
    //
    // Display the introduction text in the canvas.
    //
    GrContextFontSet(pContext, g_pFontCm18);
    GrContextForegroundSet(pContext, ClrSilver);
    GrStringDraw(pContext, "This application demonstrates", -1, 0, 32, 0);
    GrStringDraw(pContext, "the capabilities of the Stellaris", -1, 0, 50, 0);
    GrStringDraw(pContext, "Graphics Library.", -1, 0, 68, 0);
    GrStringDraw(pContext, "Each panel shows a different", -1, 0, 94, 0);
    GrStringDraw(pContext, "feature of the graphics library.", -1, 0, 112, 0);
    GrStringDraw(pContext, "Widgets on the panels are fully", -1, 0, 130, 0);
    GrStringDraw(pContext, "operational; pressing them will", -1, 0, 148, 0);
    GrStringDraw(pContext, "result in a visible feedback of", -1, 0, 166, 0);
    GrStringDraw(pContext, "some kind.", -1, 0, 184, 0);
    GrStringDraw(pContext, "Press the + and - buttons at", -1, 0, 210, 0);
    GrStringDraw(pContext, "the bottom of the screen to", -1, 0, 228, 0);
    GrStringDraw(pContext, "move between the panels.", -1, 0, 246, 0);
}

//*****************************************************************************
//
// Handles paint requests for the firmware update canvas widget.
//
//*****************************************************************************
void
OnFirmwarePaint(tWidget *pWidget, tContext *pContext)
{
    char pcBuffer[32];
    unsigned char pucMACAddr[6];
    unsigned long ulIPAddr, ulUser0, ulUser1;

    //
    // Display the firmware update instruction text in the canvas.
    //
    GrContextFontSet(pContext, g_pFontCm18);
    GrContextForegroundSet(pContext, ClrSilver);
    GrStringDraw(pContext, "You may replace the software", -1, 0, 32, 0);
    GrStringDraw(pContext, "image flashed by pressing the", -1, 0, 50, 0);
    GrStringDraw(pContext, "\"Update\" button after setting", -1, 0, 68, 0);
    GrStringDraw(pContext, "up the LMFlash utility with", -1, 0, 86, 0);
    GrStringDraw(pContext, "the following information:", -1, 0, 104, 0);

    //
    // Get the current IP address.
    //
    ulIPAddr = lwIPLocalIPAddrGet();

    //
    // Format the address as a string and display it.
    //
    if(ulIPAddr)
    {
        usprintf(pcBuffer, "IP: %d.%d.%d.%d",
                 ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                 (ulIPAddr >> 16) & 0xff, ulIPAddr >> 24);
    }
    else
    {
        usprintf(pcBuffer, "IP: Not yet assigned");
    }
    GrStringDraw(pContext, pcBuffer, -1, 0, 148, 0);

    //
    // Get the MAC address from the user registers in NV ram.
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
    // Format the MAC address string
    //
    usprintf(pcBuffer,
             "MAC: %02x-%02x-%02x-%02x-%02x-%02x",
             pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
             pucMACAddr[4], pucMACAddr[5]);
    GrStringDraw(pContext, pcBuffer, -1, 0, 170, 0);
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
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);

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
// Handles paint requests for the primitives canvas widget.
//
//*****************************************************************************
void
OnPrimitivePaint(tWidget *pWidget, tContext *pContext)
{
    unsigned long ulIdx;
    tRectangle sRect;

    //
    // Draw a vertical sweep of lines from red to green.
    //
    for(ulIdx = 0; ulIdx <= 10; ulIdx++)
    {
        GrContextForegroundSet(pContext,
                               (((((10 - ulIdx) * 255) / 10) << ClrRedShift) |
                                (((ulIdx * 255) / 10) << ClrGreenShift)));
        GrLineDraw(pContext, 115, 139, 5, 139 - (11 * ulIdx));
    }

    //
    // Draw a horizontal sweep of lines from green to blue.
    //
    for(ulIdx = 1; ulIdx <= 10; ulIdx++)
    {
        GrContextForegroundSet(pContext,
                               (((((10 - ulIdx) * 255) / 10) <<
                                 ClrGreenShift) |
                                (((ulIdx * 255) / 10) << ClrBlueShift)));
        GrLineDraw(pContext, 115, 139, 5 + (ulIdx * 11), 29);
    }

    //
    // Draw a filled circle with an overlapping circle.
    //
    GrContextForegroundSet(pContext, ClrBrown);
    GrCircleFill(pContext, 165, 69, 40);
    GrContextForegroundSet(pContext, ClrSkyBlue);
    GrCircleDraw(pContext, 195, 99, 40);

    //
    // Draw a filled rectangle with an overlapping rectangle.
    //
    GrContextForegroundSet(pContext, ClrSlateGray);
    sRect.sXMin = 5;
    sRect.sYMin = 149;
    sRect.sXMax = 75;
    sRect.sYMax = 219;
    GrRectFill(pContext, &sRect);
    GrContextForegroundSet(pContext, ClrSlateBlue);
    sRect.sXMin += 40;
    sRect.sYMin += 40;
    sRect.sXMax += 40;
    sRect.sYMax += 40;
    GrRectDraw(pContext, &sRect);

    //
    // Draw a piece of text in fonts of increasing size.
    //
    GrContextForegroundSet(pContext, ClrSilver);
    GrContextFontSet(pContext, g_pFontCm14);
    GrStringDraw(pContext, "Strings", -1, 125, 149, 0);
    GrContextFontSet(pContext, g_pFontCm18);
    GrStringDraw(pContext, "Strings", -1, 125, 163, 0);
    GrContextFontSet(pContext, g_pFontCm22);
    GrStringDraw(pContext, "Strings", -1, 125, 181, 0);
    GrContextFontSet(pContext, g_pFontCm26);
    GrStringDraw(pContext, "Strings", -1, 125, 203, 0);
    GrContextFontSet(pContext, g_pFontCm30);
    GrStringDraw(pContext, "Strings", -1, 125, 229, 0);

    //
    // Draw an image.
    //
    GrImageDraw(pContext, g_pucLogo, 190, 149);
}

//*****************************************************************************
//
// Handles paint requests for the canvas demonstration widget.
//
//*****************************************************************************
void
OnCanvasPaint(tWidget *pWidget, tContext *pContext)
{
    unsigned long ulIdx;

    //
    // Draw a set of radiating lines.
    //
    GrContextForegroundSet(pContext, ClrGoldenrod);
    for(ulIdx = 10; ulIdx <= 230; ulIdx += 10)
    {
        GrLineDraw(pContext, ulIdx, 196, 240 - ulIdx, 261);
    }

    //
    // Indicate that the contents of this canvas were drawn by the application.
    //
    GrContextFontSet(pContext, g_pFontCm12);
    GrStringDraw(pContext, "App Drawn", -1, 10, 223, 1);
}

//*****************************************************************************
//
// Handles change notifications for the check box widgets.
//
//*****************************************************************************
void
OnCheckChange(tWidget *pWidget, unsigned long bSelected)
{
    unsigned long ulIdx;

    //
    // Find the index of this check box.
    //
    for(ulIdx = 0; ulIdx < NUM_CHECK_BOXES; ulIdx++)
    {
        if(pWidget == (tWidget *)(g_psCheckBoxes + ulIdx))
        {
            break;
        }
    }

    //
    // Return if the check box could not be found.
    //
    if(ulIdx == NUM_CHECK_BOXES)
    {
        return;
    }

    //
    // Set the matching indicator based on the selected state of the check box.
    //
    CanvasImageSet(g_psCheckBoxIndicators + ulIdx,
                   bSelected ? g_pucLightOn : g_pucLightOff);
    WidgetPaint((tWidget *)(g_psCheckBoxIndicators + ulIdx));

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);
}

//*****************************************************************************
//
// Handles press notifications for the push button widgets.
//
//*****************************************************************************
void
OnButtonPress(tWidget *pWidget)
{
    unsigned long ulIdx;

    //
    // Find the index of this push button.
    //
    for(ulIdx = 0; ulIdx < NUM_PUSH_BUTTONS; ulIdx++)
    {
        if(pWidget == (tWidget *)(g_psPushButtons + ulIdx))
        {
            break;
        }
    }

    //
    // Return if the push button could not be found.
    //
    if(ulIdx == NUM_PUSH_BUTTONS)
    {
        return;
    }

    //
    // Toggle the state of this push button indicator.
    //
    g_ulButtonState ^= 1 << ulIdx;

    //
    // Set the matching indicator based on the selected state of the check box.
    //
    CanvasImageSet(g_psPushButtonIndicators + ulIdx,
                   (g_ulButtonState & (1 << ulIdx)) ? g_pucLightOn :
                   g_pucLightOff);
    WidgetPaint((tWidget *)(g_psPushButtonIndicators + ulIdx));

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);
}

//*****************************************************************************
//
// Handles notifications from the slider controls.
//
//*****************************************************************************
void
OnSliderChange(tWidget *pWidget, long lValue)
{
    static char pcCanvasText[5];
    static char pcSliderText[5];

    //
    // Is this the widget whose value we mirror in the canvas widget and the
    // locked slider?
    //
    if(pWidget == (tWidget *)&g_psSliders[SLIDER_CANVAS_VAL_INDEX])
    {
        //
        // Yes - update the canvas to show the slider value.
        //
        usprintf(pcCanvasText, "%3d%%", lValue);
        CanvasTextSet(&g_sSliderValueCanvas, pcCanvasText);
        WidgetPaint((tWidget *)&g_sSliderValueCanvas);

        //
        // Also update the value of the locked slider to reflect this one.
        //
        SliderValueSet(&g_psSliders[SLIDER_LOCKED_INDEX], lValue);
        WidgetPaint((tWidget *)&g_psSliders[SLIDER_LOCKED_INDEX]);
    }

    if(pWidget == (tWidget *)&g_psSliders[SLIDER_TEXT_VAL_INDEX])
    {
        //
        // Yes - update the canvas to show the slider value.
        //
        usprintf(pcSliderText, "%3d%%", lValue);
        SliderTextSet(&g_psSliders[SLIDER_TEXT_VAL_INDEX], pcSliderText);
        WidgetPaint((tWidget *)&g_psSliders[SLIDER_TEXT_VAL_INDEX]);
    }
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
    // See if the radio button was found.
    //
    if(ulIdx == NUM_RADIO1_BUTTONS)
    {
        //
        // Find the index of this radio button in the second group.
        //
        for(ulIdx = 0; ulIdx < NUM_RADIO2_BUTTONS; ulIdx++)
        {
            if(pWidget == (tWidget *)(g_psRadioButtons2 + ulIdx))
            {
                break;
            }
        }

        //
        // Return if the radio button could not be found.
        //
        if(ulIdx == NUM_RADIO2_BUTTONS)
        {
            return;
        }

        //
        // Sind the radio button is in the second group, offset the index to
        // the indicators associated with the second group.
        //
        ulIdx += NUM_RADIO1_BUTTONS;
    }

    //
    // Set the matching indicator based on the selected state of the radio
    // button.
    //
    CanvasImageSet(g_psRadioButtonIndicators + ulIdx,
                   bSelected ? g_pucLightOn : g_pucLightOff);
    WidgetPaint((tWidget *)(g_psRadioButtonIndicators + ulIdx));

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
    unsigned long ulUser0, ulUser1;
    unsigned char pucMACAddr[6];

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
    // Enable SysTick to provide a periodic interrupt.  This is used to provide
    // a tick for the TCP/IP stack.
    //
    SysTickPeriodSet(SysCtlClockGet() / TICKS_PER_SECOND);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Get the MAC address from the user registers in NV ram.
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
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACAddr);
    LocatorAppTitleSet("RDK-IDM grlib_demo");

    //
    // Start the remote software update module.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Initialize the display driver.
    //
    Formike240x320x16_ILI9320Init();

    //
    // Turn on the backlight.
    //
    Formike240x320x16_ILI9320BacklightOn();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sFormike240x320x16_ILI9320);

    //
    // Fill the top 24 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.sYMax = 23;
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
    GrContextFontSet(&sContext, g_pFontCm20);
    GrStringDrawCentered(&sContext, "grlib demo", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 11, 0);

    //
    // Initialize the sound driver.
    //
    SoundInit();

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
    CanvasTextSet(&g_sTitle, g_pcPanelNames[0]);

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Loop forever unless we receive a signal that a firmware update has been
    // requested.
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // Process any messages in the widget message queue.
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
    // Wait a while for the last keyboard click sound to finish.  This is about
    // 500mS since the delay loop is 3 cycles long.
    //
    SysCtlDelay(SysCtlClockGet() / 6);

    //
    // Pass control to the bootloader.
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
