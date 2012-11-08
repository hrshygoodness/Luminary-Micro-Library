//*****************************************************************************
//
// gui_widgets.c - Functions and structures related to the graphical user
//                 interface for the qs-checkout application.
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
// This is part of revision 9107 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include <stdarg.h>
#include "inc/hw_types.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/checkbox.h"
#include "grlib/container.h"
#include "grlib/listbox.h"
#include "grlib/pushbutton.h"
#include "grlib/imgbutton.h"
#include "grlib/slider.h"
#include "utils/ustdlib.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/sound.h"
#include "qs-checkout.h"
#include "gui_widgets.h"
#include "grlib_demo.h"
#include "audioplay.h"
#include "images.h"

//*****************************************************************************
//
// Storage for the strings which appear in the status box at the bottom of the
// display.
//
//****************************************************************************
#define NUM_STATUS_STRINGS 10
#define MAX_STATUS_STRING_LEN (23 + 1)
char g_pcStatus[NUM_STATUS_STRINGS][MAX_STATUS_STRING_LEN];

//*****************************************************************************
//
// Strings used to show the current mode of the USB mouse.  These strings must
// match the indices labeled MOUSE_MODE_STR_x in gui_widgets.h.
//
//****************************************************************************
char *g_pcMouseModes[3] =
{
    "None",
    "Host",
    "Device"
};

//*****************************************************************************
//
// Storage for the mouse position string.
//
//****************************************************************************
char g_pcMousePos[MAX_MOUSE_POS_LEN];

//*****************************************************************************
//
// Storage for the status listbox widget string table.
//
//*****************************************************************************
const char *g_ppcStatusStrings[NUM_STATUS_STRINGS] =
{
    g_pcStatus[0],
    g_pcStatus[1],
    g_pcStatus[2],
    g_pcStatus[3],
    g_pcStatus[4],
    g_pcStatus[5],
    g_pcStatus[6],
    g_pcStatus[7],
    g_pcStatus[8],
    g_pcStatus[9]
};
unsigned long g_ulStatusStringIndex = 0;

//*****************************************************************************
//
// Forward references for various widgets.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tContainerWidget g_sIOScreen;
extern tContainerWidget g_sDemoScreen;
extern tContainerWidget g_sHomeScreen;
extern tContainerWidget g_sStatusContainer;
extern tContainerWidget g_sEthernetContainer;
extern tContainerWidget g_sBoardIOContainer;
extern tContainerWidget g_sUSBContainer;

//*****************************************************************************
//*****************************************************************************
//
// Start of widget definitions for the home selection screen.
//
//*****************************************************************************
//*****************************************************************************

Canvas(g_sLMSymbol, &g_sHomeScreen, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 220, 75, 100, 100,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_IMG),
       CLR_BACKGROUND, 0, 0, 0, 0, g_pucTISymbol_80x75, 0);

Canvas(g_sStellarisWare, &g_sHomeScreen, &g_sLMSymbol, 0,
       &g_sKitronix320x240x16_SSD2119, 60, 210, 200, 28,
       CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucStellarisWare_200x28, 0);

Canvas(g_sEKTitle, &g_sHomeScreen, &g_sStellarisWare, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 34, 320, 20,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_HCENTER),
       CLR_BACKGROUND, 0, CLR_TEXT, g_pFontCmss22b,
       "LM3S9B96 Development Kit", 0, 0);

ImageButton(g_sIOExamplesBtn, &g_sHomeScreen, &g_sEKTitle, 0,
            &g_sKitronix320x240x16_SSD2119, 20, 64, 180, 32,
            ( IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY),
             CLR_TEXT, CLR_TEXT, ClrOrange, g_pFontCmss18b, "IO Examples",
             g_pucRedButton_180x32_Up, g_pucRedButton_180x32_Down, 0, 3, 3,
             0, 0, OnBtnShowIOScreen);

ImageButton(g_sDemoBtn, &g_sHomeScreen, &g_sIOExamplesBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 20, 100, 180, 32,
            ( IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY),
             CLR_TEXT, CLR_TEXT, ClrOrange, g_pFontCmss18b, "Graphics Demo",
             g_pucRedButton_180x32_Up, g_pucRedButton_180x32_Down, 0, 3, 3,
             0, 0, OnBtnShowDemoScreen);

ImageButton(g_sAudioPlayBtn, &g_sHomeScreen, &g_sDemoBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 20, 136, 180, 32,
            ( IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY),
             CLR_TEXT, CLR_TEXT, ClrOrange, g_pFontCmss18b, "Audio Player",
             g_pucRedButton_180x32_Up, g_pucRedButton_180x32_Down, 0, 3, 3,
             0, 0, OnBtnShowAudioScreen);

ImageButton(g_sImageShowBtn, &g_sHomeScreen, &g_sAudioPlayBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 20, 172, 180, 32,
            ( IB_STYLE_TEXT | IB_STYLE_RELEASE_NOTIFY),
             CLR_TEXT, CLR_TEXT, ClrOrange, g_pFontCmss18b, "Image Viewer",
             g_pucRedButton_180x32_Up, g_pucRedButton_180x32_Down, 0, 3, 3,
             0, 0, OnBtnShowImageScreen);

#define HOME_ROOT_BTN &g_sImageShowBtn

//*****************************************************************************
//
// The container widget acting as the root for the IO screen widget subtree.
//
//*****************************************************************************
Container(g_sHomeScreen, &g_sBackground, 0, HOME_ROOT_BTN,
          &g_sKitronix320x240x16_SSD2119, 0, 24, 320, 217,
          0, 0, 0, 0, 0, 0);

//*****************************************************************************
//*****************************************************************************
//
// Start of widget definitions for graphics demo screen.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// The container widget acting as the root for the graphics demo widget subtree.
//
//*****************************************************************************
Container(g_sDemoScreen, &g_sBackground, 0, 0,
          &g_sKitronix320x240x16_SSD2119, 0, 24, 320, 217,
          0, 0, 0, 0, 0, 0);

//*****************************************************************************
//*****************************************************************************
//
// Start of widget definitions for image viewer screen.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// The container widget acting as the root for the image viewer widget subtree.
//
//*****************************************************************************
Container(g_sImageScreen, &g_sBackground, 0, 0,
          &g_sKitronix320x240x16_SSD2119, 0, 24, 320, 185,
          0, 0, 0, 0, 0, 0);

//*****************************************************************************
//*****************************************************************************
//
// Start of widget definitions for audio player screen.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// The container widget acting as the root for the audio player widget subtree.
//
//*****************************************************************************
Container(g_sAudioScreen, &g_sBackground, 0, 0,
          &g_sKitronix320x240x16_SSD2119, 0, 24, 320, 185,
          0, 0, 0, 0, 0, 0);

//*****************************************************************************
//*****************************************************************************
//
// Start of widget definitions for I/O Checkout screen.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// The slider tracking the current position of the thumbwheel.
//
//*****************************************************************************
Slider(g_sThumbwheelSlider, &g_sIOScreen, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 290, 40, 20, 190, 0, 3000, 0,
       (SL_STYLE_IMG | SL_STYLE_BACKG_IMG | SL_STYLE_VERTICAL |
       SL_STYLE_LOCKED), 0, 0, 0, 0, 0,
       0, 0, g_pucRedVertSlider190x20Image, g_pucGreenVertSlider190x20Image, 0);

//*****************************************************************************
//
// The canvas widget used to show the USB mouse position title.
//
//*****************************************************************************
Canvas(g_sMouseTitle, &g_sUSBContainer, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 178, 180, 38, 20,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT),
       CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8, "Mouse:", 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the mouse position.
//
//*****************************************************************************
Canvas(g_sMousePos, &g_sUSBContainer, &g_sMouseTitle, 0,
       &g_sKitronix320x240x16_SSD2119, 216, 180, 63, 20,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT),
       CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8, g_pcMousePos, 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the USB mode title
//
//*****************************************************************************
Canvas(g_sModeTitle, &g_sUSBContainer, &g_sMousePos, 0,
       &g_sKitronix320x240x16_SSD2119, 180, 160, 36, 20,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT),
       CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8, "Mode:", 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the mouse mode, device or host.
//
//*****************************************************************************
Canvas(g_sModeString, &g_sUSBContainer, &g_sModeTitle, 0,
       &g_sKitronix320x240x16_SSD2119, 216, 160, 63, 20,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT),
       CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8, "None", 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the state of button 1.
//
//*****************************************************************************
Canvas(g_sMouseBtn1, &g_sUSBContainer, &g_sModeString, 0,
       &g_sKitronix320x240x16_SSD2119, 188, 210, 14, 14,
       (CANVAS_STYLE_IMG | CANVAS_STYLE_TEXT), 0, 0, CLR_TEXT,
       g_pFontFixed6x8, "1", g_pucGreyLED14x14Image, 0);

//*****************************************************************************
//
// The canvas widget used to show the state of button 2.
//
//*****************************************************************************
Canvas(g_sMouseBtn2, &g_sUSBContainer, &g_sMouseBtn1, 0,
       &g_sKitronix320x240x16_SSD2119, 219, 210, 14, 14,
       (CANVAS_STYLE_IMG | CANVAS_STYLE_TEXT), 0, 0, CLR_TEXT,
       g_pFontFixed6x8, "2", g_pucGreyLED14x14Image, 0);

//*****************************************************************************
//
// The canvas widget used to show the state of button 3.
//
//*****************************************************************************
Canvas(g_sMouseBtn3, &g_sUSBContainer, &g_sMouseBtn2, 0,
       &g_sKitronix320x240x16_SSD2119, 248, 210, 14, 14,
       (CANVAS_STYLE_IMG | CANVAS_STYLE_TEXT), 0, 0, CLR_TEXT,
       g_pFontFixed6x8, "3", g_pucGreyLED14x14Image, 0);

//*****************************************************************************
//
// The containers widget acting as the background to the USB area.
//
//*****************************************************************************
Container(g_sUSBContainer, &g_sIOScreen, &g_sThumbwheelSlider, &g_sMouseBtn3,
          &g_sKitronix320x240x16_SSD2119, 170, 150, 110, 80,
          (CTR_STYLE_OUTLINE | CTR_STYLE_FILL | CTR_STYLE_TEXT),
          CLR_BOX, CLR_OUTLINE, CLR_OUTLINE, g_pFontFixed6x8,
          "USB");

//*****************************************************************************
//
// The checkbox widget used to turn the board LED on and off.
//
//*****************************************************************************
CheckBox(g_sLEDCheckbox, &g_sBoardIOContainer, 0, 0,
         &g_sKitronix320x240x16_SSD2119, 200, 92, 50, 12,
         CB_STYLE_TEXT | CB_STYLE_FILL, 12, CLR_BOX, CLR_TEXT, CLR_TEXT,
         g_pFontFixed6x8, " LED", 0, OnCheckLED);

//*****************************************************************************
//
// The canvas widget used to show the touch title.
//
//*****************************************************************************
Canvas(g_sTouchTitle, &g_sBoardIOContainer, &g_sLEDCheckbox, 0,
       &g_sKitronix320x240x16_SSD2119, 178, 70, 38, 20,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT),
       CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8, "Touch:", 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the touch position.
//
//*****************************************************************************
Canvas(g_sTouchPos, &g_sBoardIOContainer, &g_sTouchTitle, 0,
       &g_sKitronix320x240x16_SSD2119, 216, 70, 63, 20,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT),
       CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8, g_pcTouchCoordinates, 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the first line of the SD card status.
//
//*****************************************************************************
Canvas(g_sSDCard1, &g_sBoardIOContainer, &g_sTouchPos, 0,
       &g_sKitronix320x240x16_SSD2119, 180, 110, 90, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT),
       CLR_BOX, 0, CLR_ABSENT, g_pFontFixed6x8, "MicroSD Card", 0, 0);

//*****************************************************************************
//
// The canvas widget used to show second line of the SD card status.
//
//*****************************************************************************
Canvas(g_sSDCard2, &g_sBoardIOContainer, &g_sSDCard1, 0,
       &g_sKitronix320x240x16_SSD2119, 180, 120, 90, 10,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT), CLR_BOX, 0, CLR_ABSENT,
       g_pFontFixed6x8, "", 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the potentiometer title.
//
//*****************************************************************************
Canvas(g_sPotTitle, &g_sBoardIOContainer, &g_sSDCard2, 0,
       &g_sKitronix320x240x16_SSD2119, 180, 50, 36, 20,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT),
       CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8, "Pot:", 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the potentiometer position.
//
//*****************************************************************************
Canvas(g_sPotPos, &g_sBoardIOContainer, &g_sPotTitle, 0,
       &g_sKitronix320x240x16_SSD2119, 216, 50, 63, 20,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT |
       CANVAS_STYLE_TEXT_OPAQUE), CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8,
       g_pucThumbwheelString, 0, 0);

//*****************************************************************************
//
// The container widget acting as the background to the board I/O area.
//
//*****************************************************************************
Container(g_sBoardIOContainer, &g_sIOScreen, &g_sUSBContainer, &g_sPotPos,
          &g_sKitronix320x240x16_SSD2119, 170, 40, 110, 100,
          (CTR_STYLE_OUTLINE | CTR_STYLE_FILL | CTR_STYLE_TEXT),
          CLR_BOX, CLR_OUTLINE, CLR_OUTLINE, g_pFontFixed6x8,
          "Board I/O");

//*****************************************************************************
//
// The canvas widget used to show the MAC address.
//
//*****************************************************************************
Canvas(g_sMACAddr, &g_sEthernetContainer, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 50, 70, 108, 18,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT),
       CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8, g_pucMACAddrString, 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the IP address.
//
//*****************************************************************************
Canvas(g_sIPAddr, &g_sEthernetContainer, &g_sMACAddr, 0,
       &g_sKitronix320x240x16_SSD2119,
       50, 50, 108, 20, (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_LEFT),
       CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8, g_pucIPAddrString, 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the MAC address heading text.
//
//*****************************************************************************
Canvas(g_sMACTitle, &g_sEthernetContainer, &g_sIPAddr, 0,
       &g_sKitronix320x240x16_SSD2119, 20, 70, 30, 18,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_RIGHT),
       CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8, "MAC:", 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the IP address heading text.
//
//*****************************************************************************
Canvas(g_sIPTitle, &g_sEthernetContainer, &g_sMACTitle, 0,
       &g_sKitronix320x240x16_SSD2119,
       20, 50, 30, 20, (CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT |
       CANVAS_STYLE_TEXT_RIGHT),
       CLR_BOX, 0, CLR_TEXT, g_pFontFixed6x8, "IP:", 0, 0);

//*****************************************************************************
//
// The container widget acting as the background to the status list box.
//
//*****************************************************************************
Container(g_sEthernetContainer, &g_sIOScreen, &g_sBoardIOContainer,
          &g_sIPTitle, &g_sKitronix320x240x16_SSD2119, 10, 40, 150, 50,
          (CTR_STYLE_OUTLINE | CTR_STYLE_FILL | CTR_STYLE_TEXT),
          CLR_BOX, CLR_OUTLINE, CLR_OUTLINE, g_pFontFixed6x8,
          "Ethernet");

//*****************************************************************************
//
// The listbox widget used to show scrolling status messages.
//
//*****************************************************************************
ListBox(g_sStatusList, &g_sStatusContainer, 0, 0,
        &g_sKitronix320x240x16_SSD2119, 20, 112, 130, 80,
        (LISTBOX_STYLE_LOCKED | LISTBOX_STYLE_WRAP),
        CLR_BACKGROUND, CLR_BACKGROUND, CLR_TEXT, CLR_TEXT, 0,
        g_pFontFixed6x8, g_ppcStatusStrings,  NUM_STATUS_STRINGS,
        0, 0);

//*****************************************************************************
//
// The container widget acting as the background to the status list box.
//
//*****************************************************************************
Container(g_sStatusContainer, &g_sIOScreen, &g_sEthernetContainer,
          &g_sStatusList, &g_sKitronix320x240x16_SSD2119, 10, 100, 150, 100,
          (CTR_STYLE_OUTLINE | CTR_STYLE_FILL | CTR_STYLE_TEXT),
          CLR_BOX, CLR_OUTLINE, CLR_OUTLINE, g_pFontFixed6x8,
          "Status");

//*****************************************************************************
//
// The push button widget used to return to the main menu.
//
//*****************************************************************************
RectangularButton(g_sHomeBtn, &g_sIOScreen, &g_sStatusContainer, 0,
                  &g_sKitronix320x240x16_SSD2119, 10, 210, 90, 24,
                  ( PB_STYLE_TEXT | PB_STYLE_IMG | PB_STYLE_RELEASE_NOTIFY),
                   0, 0, 0, CLR_TEXT, g_pFontCmss18b, "Home",
                   g_pucRedButton_90x24_Up, g_pucRedButton_90x24_Down, 0, 0,
                   OnBtnHome);

//*****************************************************************************
//
// The container widget acting as the root for the IO screen widget subtree.
//
//*****************************************************************************
Container(g_sIOScreen, &g_sBackground, 0, &g_sHomeBtn,
          &g_sKitronix320x240x16_SSD2119, 0, 24, 320, 217,
          0, 0, 0, 0, 0, 0);

//*****************************************************************************
//*****************************************************************************
//
// End of widget definitions for I/O Checkout screen.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//*****************************************************************************
//
// Start of top level widget definitions.  These are visible regardless
// of the screen that is currently displayed.
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
// The canvas widget acting as the background to the area of the screen under
// the logo masthead.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, 0, &g_sHomeScreen,
       &g_sKitronix320x240x16_SSD2119, 0, 23, 320, 217,
       CANVAS_STYLE_FILL, CLR_BACKGROUND, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The heading containing the Texas Instruments masthead.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sBackground, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_IMG),
       CLR_BACKGROUND, 0, 0, 0, 0, g_pucTIName, 0);

//*****************************************************************************
//
// The widgets which form the root of each screen.  These are hooked as
// children of g_sBackground to activate the particular screen.
//
//*****************************************************************************
tWidget *g_psScreens[] =
{
    (tWidget *)&g_sHomeScreen,
    (tWidget *)&g_sIOScreen,
    (tWidget *)&g_sDemoScreen,
    (tWidget *)&g_sImageScreen,
    (tWidget *)&g_sAudioScreen,
};

#define NUM_SCREENS (sizeof(g_psScreens) / sizeof(tWidget *))

//*****************************************************************************
//
// The index of the screen which is currently hooked to WIDGET_ROOT and, hence,
// displayed.
//
//*****************************************************************************
unsigned long g_ulCurrentScreen = 0;

//*****************************************************************************
//
// This function is used to add a new string to the status list box at the
// bottom of the display.  This shows errors and echos user commands entered
// via the UART.
//
//*****************************************************************************
int
PrintfStatus(char *pcFormat, ...)
{
    int iRet;
    va_list vaArgP;

    //
    // Start the varargs processing.
    //
    va_start(vaArgP, pcFormat);

    //
    // Call vsnprintf to perform the conversion.
    //
    iRet = uvsnprintf(g_pcStatus[g_ulStatusStringIndex], MAX_STATUS_STRING_LEN,
                      pcFormat, vaArgP);

    //
    // End the varargs processing.
    //
    va_end(vaArgP);

    //
    // Add the new string to the status listbox.
    //
    ListBoxTextAdd(&g_sStatusList, g_pcStatus[g_ulStatusStringIndex]);

    //
    // Update our string index.
    //
    g_ulStatusStringIndex++;
    if(g_ulStatusStringIndex == NUM_STATUS_STRINGS)
    {
        g_ulStatusStringIndex = 0;
    }

    //
    // Repaint the status listbox if it is visible
    //
    if(g_ulCurrentScreen == IO_SCREEN)
    {
        WidgetPaint((tWidget *)&g_sStatusList);
    }

    //
    // Return the conversion count.
    //
    return(iRet);

}

//*****************************************************************************
//
// Show a particular user interface screen.
//
//*****************************************************************************
void
ShowUIScreen(unsigned long ulIndex)
{
    //
    // Unhook the current screen from the widget root.  This removes all the
    // screen controls from the display.
    //
    WidgetRemove(g_psScreens[g_ulCurrentScreen]);

    //
    // Hook the home screen to the main tree.
    //
    g_ulCurrentScreen = ulIndex;
    WidgetAdd( (tWidget *)&g_sBackground, g_psScreens[g_ulCurrentScreen]);

    //
    // Repaint the display.
    //
    WidgetPaint(WIDGET_ROOT);
}

//*****************************************************************************
//
// This handler is called whenever the "Home" button is released.  It resets
// the widget tree to ensure that the home screen is displayed.
//
//*****************************************************************************
void
OnBtnHome(tWidget *pWidget)
{
    ShowUIScreen(HOME_SCREEN);

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, g_ulKeyClickLen);
}

//*****************************************************************************
//
// This handler is called whenever the "IO Examples" button is released.  It
// sets up the widget tree to show the relevant controls.
//
//*****************************************************************************
void
OnBtnShowIOScreen(tWidget *pWidget)
{
    ShowUIScreen(IO_SCREEN);

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, g_ulKeyClickLen);
}

//*****************************************************************************
//
// This handler is called whenever the "Graphics Demo" button is released.  It
// sets up the widget tree to show the relevant controls.
//
//*****************************************************************************
void
OnBtnShowDemoScreen(tWidget *pWidget)
{
    ShowUIScreen(DEMO_SCREEN);

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, g_ulKeyClickLen);
}

//*****************************************************************************
//
// This handler is called whenever the "Image Viewer" button is released.  It
// sets up the widget tree to show the relevant controls.
//
//*****************************************************************************
void
OnBtnShowImageScreen(tWidget *pWidget)
{
    //
    // Switch to the image viewer screen.
    //
    ShowUIScreen(IMAGE_SCREEN);

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, g_ulKeyClickLen);
}

