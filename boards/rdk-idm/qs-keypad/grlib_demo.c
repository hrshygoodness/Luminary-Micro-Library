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
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/ustdlib.h"
#include "drivers/formike240x320x16_ili9320.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "demo_images.h"
#include "images.h"
#include "qs-keypad.h"

//*****************************************************************************
//
// The main widget in the keypad widget tree.
//
//*****************************************************************************
extern tCanvasWidget g_sBlackBackground;

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
                  ClrDarkRed, ClrSilver, 0, 0, 0, 0, 0, 0, OnSliderChange),
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
    CanvasStruct(0, 0, g_psSliders, &g_sFormike240x320x16_ILI9320, 0,
                 24, 240, 246, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0),
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
    "     Sliders     "
};

//*****************************************************************************
//
// The buttons and text across the bottom of the screen.
//
//*****************************************************************************
extern tCanvasWidget g_sDemoBackground;

RectangularButton(g_sNext, &g_sDemoBackground, 0, 0,
                  &g_sFormike240x320x16_ILI9320, 190, 270, 50, 50,
                  PB_STYLE_IMG | PB_STYLE_TEXT, ClrBlack, ClrBlack, 0,
                  ClrSilver, g_pFontCm20, "+", g_pucBlue50x50,
                  g_pucBlue50x50Press, 0, 0, OnNext);
Canvas(g_sTitle, &g_sDemoBackground, &g_sNext, 0,
       &g_sFormike240x320x16_ILI9320, 50, 270, 140, 50, (CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_OPAQUE), 0, 0, ClrSilver, g_pFontCm20, 0, 0, 0);
RectangularButton(g_sPrevious, &g_sDemoBackground, &g_sTitle, 0,
                  &g_sFormike240x320x16_ILI9320, 0, 270, 50, 50,
                  PB_STYLE_IMG | PB_STYLE_TEXT, ClrBlack, ClrBlack, 0,
                  ClrSilver, g_pFontCm20, "-", g_pucBlue50x50,
                  g_pucBlue50x50Press, 0, 0, OnPrevious);

//*****************************************************************************
//
// A canvas widget below the logo at the top to provide a dividing line between
// the logo and the main display.
//
//*****************************************************************************
Canvas(g_sTopLine, &g_sDemoBackground, &g_sPrevious, 0,
       &g_sFormike240x320x16_ILI9320, 0, 20, 240, 1, CANVAS_STYLE_FILL,
       ClrSilver, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// A canvas widget across the top of the screen to display the Texas
// Instruments logo.
//
//*****************************************************************************
Canvas(g_sDemoBanner, &g_sDemoBackground, &g_sTopLine, 0,
       &g_sFormike240x320x16_ILI9320, 0, 4, 240, 13, CANVAS_STYLE_IMG, 0, 0, 0,
       0, 0, g_pucTIName, 0);

//*****************************************************************************
//
// The background canvas widget for the demo.
//
//*****************************************************************************
Canvas(g_sDemoBackground, 0, 0, &g_sDemoBanner, &g_sFormike240x320x16_ILI9320,
       0, 0, 240, 320, CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

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
    // If this button is pressed when the first panel is displayed, we exit
    // back to keypad display.
    //
    if(g_ulPanel == 0)
    {
        //
        // Play the key click sound.
        //
        SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);

        //
        // Fix up the widget tree to move us back to keypad mode.
        //
        GraphicsDemoHide();
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
    WidgetAdd((tWidget *)&g_sDemoBackground,
              (tWidget *)(g_psPanels + g_ulPanel));
    WidgetPaint((tWidget *)(g_psPanels + g_ulPanel));

    //
    // Set the title of this panel.
    //
    CanvasTextSet(&g_sTitle, g_pcPanelNames[g_ulPanel]);
    WidgetPaint((tWidget *)&g_sTitle);

    //
    // Set the button text to read "X" (exit) if this is the first panel,
    // otherwise revert to the usual "-".
    //
    PushButtonTextSet(&g_sPrevious, (g_ulPanel == 0) ? "X" : "-");
    WidgetPaint((tWidget *)&g_sPrevious);

    //
    // Set the next button text to read "X" (exit) if this is the last panel,
    // otherwise revert to the usual "+".
    //
    PushButtonTextSet(&g_sNext, (g_ulPanel == (NUM_PANELS - 1)) ? "X" : "+");
    WidgetPaint((tWidget *)&g_sNext);

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
    // If this button is pressed when the last panel is displayed, we exit
    // back to keypad display.
    //
    if(g_ulPanel == (NUM_PANELS - 1))
    {
        //
        // Play the key click sound.
        //
        SoundPlay(g_pusKeyClick, sizeof(g_pusKeyClick) / 2);

        //
        // Fix up the widget tree to move us back to keypad mode.
        //
        GraphicsDemoHide();
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
    WidgetAdd((tWidget *)&g_sDemoBackground,
              (tWidget *)(g_psPanels + g_ulPanel));
    WidgetPaint((tWidget *)(g_psPanels + g_ulPanel));

    //
    // Set the title of this panel.
    //
    CanvasTextSet(&g_sTitle, g_pcPanelNames[g_ulPanel]);
    WidgetPaint((tWidget *)&g_sTitle);

    //
    // Set the button text to read "X" (exit) if this is the last panel,
    // otherwise revert to the usual "+".
    //
    PushButtonTextSet(&g_sNext, (g_ulPanel == (NUM_PANELS - 1)) ? "X" : "+");
    WidgetPaint((tWidget *)&g_sNext);

    //
    // Set the "previous" button text to read "X" if this is the first panel,
    // otherwise revert to the usual "-".
    //
    PushButtonTextSet(&g_sPrevious, (g_ulPanel == 0) ? "X" : "-");
    WidgetPaint((tWidget *)&g_sPrevious);

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
    GrContextFontSet(pContext, g_pFontCm20b);
    GrContextForegroundSet(pContext, ClrSilver);
    GrStringDraw(pContext, "Stellaris Graphics Library", -1, 0, 35, 0);

    GrContextFontSet(pContext, g_pFontCm18);
    GrStringDraw(pContext, "Each panel shows a different", -1, 0, 68, 0);
    GrStringDraw(pContext, "feature of the graphics library.", -1, 0, 86, 0);
    GrStringDraw(pContext, "Widgets on the panels are fully", -1, 0, 104, 0);
    GrStringDraw(pContext, "operational; pressing them will", -1, 0, 122, 0);
    GrStringDraw(pContext, "result in visible feedback of", -1, 0, 140, 0);
    GrStringDraw(pContext, "some kind.", -1, 0, 158, 0);
    GrStringDraw(pContext, "Press the + and - buttons at", -1, 0, 186, 0);
    GrStringDraw(pContext, "the bottom of the screen to", -1, 0, 204, 0);
    GrStringDraw(pContext, "move between the panels and", -1, 0, 222, 0);
    GrStringDraw(pContext, "X to return to the keypad.", -1, 0, 240, 0);
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
    // Is this the widget whose value we mirror in the canvas widget?
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
// Fill either the whole screen (psRect == NULL) or a subrectangle (psRect !=
// NULL) with the supplied color.
//
//*****************************************************************************
void
GraphicsDemoCls(tRectangle *psRect, unsigned long ulColor)
{
    tContext sContext;
    tRectangle sRect;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sContext, &g_sFormike240x320x16_ILI9320);

    //
    // Are we being asked to fill the whole screen or a subrectangle?
    //
    if(!psRect)
    {
        //
        // Get the full screen dimensions.
        //
        sRect.sXMin = 0;
        sRect.sYMin = 0;
        sRect.sXMax = GrContextDpyWidthGet(&sContext) - 1;
        sRect.sYMax = GrContextDpyHeightGet(&sContext) - 1;
    }
    else
    {
        //
        // Filling a subrectangle so just copy the rectangle passed.
        //
        sRect = *psRect;
    }

    //
    // Set the foreground color.
    //
    GrContextForegroundSet(&sContext, ulColor);

    //
    // Fill the screen with the required background color.
    //
    GrRectFill(&sContext, &sRect);

}

//*****************************************************************************
//
// Show the graphics demo
//
//*****************************************************************************
void
GraphicsDemoShow(void)
{
    //
    // Remove the keypad widget tree completely.
    //
    WidgetRemove((tWidget *)&g_sBlackBackground);

    //
    // Remember that we are showing the graphics demo.
    //
    g_ulMode = MODE_DEMO;

    //
    // Set the previous and next button text correctly for the first screen.
    //
    PushButtonTextSet(&g_sNext, "+");
    PushButtonTextSet(&g_sPrevious, "X");

    //
    // Add the title block and the previous and next buttons to the widget
    // tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sDemoBackground);

    //
    // Add the first panel to the widget tree.
    //
    g_ulPanel = 0;
    WidgetAdd((tWidget *)&g_sDemoBackground, (tWidget *)g_psPanels);
    CanvasTextSet(&g_sTitle, g_pcPanelNames[0]);

    //
    // Issue the initial paint request to the widgets.
    //
    WidgetPaint(WIDGET_ROOT);
}

//*****************************************************************************
//
// End the graphics demo and return to the keypad.
//
//*****************************************************************************
void
GraphicsDemoHide(void)
{

    //
    // Remove the current panel from the demo tree.
    //
    WidgetRemove((tWidget *)(g_psPanels + g_ulPanel));

    //
    // Remove all the demo widgets.
    //
    WidgetRemove((tWidget *)&g_sDemoBackground);

     //
     // Reinstate the keypad widget tree.
     //
     WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBlackBackground);

     //
     // Issue the initial paint request to the widgets.
     //
     WidgetPaint(WIDGET_ROOT);

     //
     // Revert to keypad operation.
     //
     g_ulMode = MODE_KEYPAD;
}
