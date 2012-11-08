//*****************************************************************************
//
// imageview.c - JPEG image viewer function for the Tempest checkout
//               application
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
// This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/pushbutton.h"
#include "grlib/imgbutton.h"
#include "grlib/canvas.h"
#include "grlib/slider.h"
#include "grlib/container.h"
#include "utils/ustdlib.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "drivers/jpgwidget.h"
#include "images.h"
#include "file.h"
#include "gui_widgets.h"
#include "qs-checkout.h"
#include "grlib_demo.h"

//*****************************************************************************
//
// Forward references.
//
//*****************************************************************************
static void OnJPEGScroll(tWidget *pWidget, short sX, short sY);
static void OnBtnImageToHome(tWidget *pWidget);
static void OnBtnImageNext(tWidget *pWidget);
static void OnBtnImagePrevious(tWidget *pWidget);
static tBoolean ImageViewerGetImage(unsigned long ulIndex, tBoolean bPaint);

//*****************************************************************************
//
// The number of JPEG images found in the /ram/images directory.
//
//*****************************************************************************
static unsigned long g_ulJPGsFound;

//*****************************************************************************
//
// The index of the JPEG currently being displayed.
//
//*****************************************************************************
static unsigned long g_ulJPGIndex;

//*****************************************************************************
//
// Workspace structure for the main JPEG image viewing widget.
//
//*****************************************************************************
tJPEGInst g_sMainJPEGInst;

//*****************************************************************************
//
// The widget forming the main image display area.
//
//*****************************************************************************
JPEGCanvas(g_sMainImage, &g_sImageScreen, 0, 0,
           &g_sKitronix320x240x16_SSD2119, 0, 24, 320, 184,
           (JW_STYLE_OUTLINE | JW_STYLE_TEXT), ClrBlack, ClrWhite, ClrWhite,
           g_pFontCmss22b, "", 0, 0, 1, OnJPEGScroll, &g_sMainJPEGInst);

//*****************************************************************************
//
// The push button widget used to move to the next image.
//
//*****************************************************************************
RectangularButton(g_sImageNextBtn, &g_sImageScreen, &g_sMainImage, 0,
                  &g_sKitronix320x240x16_SSD2119, 220, 210, 90, 24,
                  ( PB_STYLE_TEXT | PB_STYLE_IMG | PB_STYLE_RELEASE_NOTIFY),
                   0, 0, 0, CLR_TEXT, g_pFontCmss18b, "Next",
                   g_pucRedButton_90x24_Up, g_pucRedButton_90x24_Down, 0, 0,
                   OnBtnImageNext);

//*****************************************************************************
//
// The push button widget used to move to the previous image.
//
//*****************************************************************************
RectangularButton(g_sImagePreviousBtn, &g_sImageScreen, &g_sImageNextBtn, 0,
                  &g_sKitronix320x240x16_SSD2119, 10, 210, 90, 24,
                  ( PB_STYLE_TEXT | PB_STYLE_IMG | PB_STYLE_RELEASE_NOTIFY),
                   0, 0, 0, CLR_TEXT, g_pFontCmss18b, "Previous",
                   g_pucRedButton_90x24_Up, g_pucRedButton_90x24_Down, 0, 0,
                   OnBtnImagePrevious);

//*****************************************************************************
//
// The push button widget used to return to the main menu.
//
//*****************************************************************************
RectangularButton(g_sImageHomeBtn, &g_sImageScreen, &g_sImagePreviousBtn, 0,
                  &g_sKitronix320x240x16_SSD2119, 115, 210, 90, 24,
                  ( PB_STYLE_TEXT | PB_STYLE_IMG | PB_STYLE_RELEASE_NOTIFY),
                   0, 0, 0, CLR_TEXT, g_pFontCmss18b, "Home",
                   g_pucRedButton_90x24_Up, g_pucRedButton_90x24_Down, 0, 0,
                   OnBtnImageToHome);

//*****************************************************************************
//
// This callback is made whenever someone scrolls the main JPEG canvas widget.
//
//*****************************************************************************
static void
OnJPEGScroll(tWidget *pWidget, short sX, short sY)
{
    static unsigned long ulLastRedraw = 0;

    //
    // We use this callback as a way to pace the repainting of the JPEG
    // image in the widget.  We could set JW_STYLE_SCROLL and have the widget
    // repaint itself every time it receives a pointer move message but these
    // are very frequent so this is likely to result in a waste of CPU.
    // Instead, we monitor callbacks and repaint only if 200mS has passed
    // since the last time we repainted.
    //

    //
    // How long has it been since we last redrew?
    //
    if((g_ulSysTickCount - ulLastRedraw) > JPEG_REDRAW_TIMEOUT)
    {
        WidgetPaint((tWidget *)&g_sMainImage);
        ulLastRedraw = g_ulSysTickCount;
    }
}

//*****************************************************************************
//
// This handler is called whenever the "Home" button is released from the image
// viewer screen.  It resets the widget tree to ensure that the home screen is
// displayed.
//
//*****************************************************************************
static void
OnBtnImageToHome(tWidget *pWidget)
{
    //
    // This is a separate handler since we may want to do some tidy up on
    // exiting from the image viewer.  Currently we don't do anything
    // special but we may so...
    //

    //
    // Switch back to the home screen.
    //
    ShowUIScreen(HOME_SCREEN);

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, g_ulKeyClickLen);
}

//*****************************************************************************
//
// This handler is called whenever the "Next" button is released from the image
// viewer screen.  It find the next JPEG image in the /ram/images directory
// and displays it on the screen.
//
//*****************************************************************************
static void
OnBtnImageNext(tWidget *pWidget)
{
    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, g_ulKeyClickLen);

    //
    // Determine which image to show next.  If we are showing the first, cycle
    // back to the last.
    //
    g_ulJPGIndex = ((g_ulJPGIndex == (g_ulJPGsFound - 1)) ? 0 :
                    (g_ulJPGIndex + 1));

    //
    // Get the new image and decompress it.
    //
    ImageViewerGetImage(g_ulJPGIndex, true);
}

//*****************************************************************************
//
// This handler is called whenever the "Previous" button is released from the
// image viewer screen.  It find the previous JPEG image in the /ram/images
// directory and displays it on the screen.
//
//*****************************************************************************
static void
OnBtnImagePrevious(tWidget *pWidget)
{
    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, g_ulKeyClickLen);

    //
    // Determine which image to show next.  If we are showing the first, cycle
    // back to the last.
    //
    g_ulJPGIndex = ((g_ulJPGIndex == 0) ? (g_ulJPGsFound - 1) :
                    (g_ulJPGIndex - 1));

    //
    // Get the new image and decompress it.
    //
    ImageViewerGetImage(g_ulJPGIndex, true);
}

//*****************************************************************************
//
// Read the ulIndex-th JPEG image and pass it to the JPEG canvas widget for
// decompression.  If bPaint is true, repaint the widget to show the new image
// or the error information string.
//
//*****************************************************************************
static tBoolean
ImageViewerGetImage(unsigned long ulIndex, tBoolean bPaint)
{
    tBoolean bRetcode;
    unsigned long ulLen, ulError;
    char *pcName;
    unsigned char *pucData;

    //
    // Get a pointer to the file data and its length.
    //
    bRetcode = FileGetJPEGFileInfo(ulIndex, &pcName, &ulLen, &pucData);

    //
    // Did we get the file information successfully?
    //
    if(bRetcode)
    {
        //
        // If we have been asked to paint the image, display text on top of
        // the existing image indicating that decompression is going on.
        //
        if(bPaint)
        {
            JPEGWidgetTextSet(&g_sMainImage, "Decompressing...");
            WidgetPaint((tWidget *)&g_sMainImage);
            WidgetMessageQueueProcess();
        }

        //
        // We got the file information so now pass it to the JPEG canvas
        // widget to have it decompressed.
        //
        ulError = JPEGWidgetImageSet((tWidget *)&g_sMainImage, pucData, ulLen);

        //
        // Did the decompression go as planned?
        //
        if(ulError)
        {
            //
            // No - something went wrong.  Set an error message.
            //
            JPEGWidgetTextSet(&g_sMainImage, "Decompression Error!");
            bRetcode = false;
        }
        else
        {
            //
            // The image was decompressed successfully so remove any error
            // string that the control may have been displaying.
            //
            JPEGWidgetTextSet(&g_sMainImage, "");
        }
    }

    //
    // If we have been asked to repaint the widget, do so.
    //
    if(bPaint)
    {
        WidgetPaint((tWidget *)&g_sMainImage);
    }

    //
    // Tell the caller how things went.
    //
    return(bRetcode);
}

//*****************************************************************************
//
// A simple demonstration of the features of the Stellaris Graphics Library.
//
//*****************************************************************************
void
ImageViewerInit(void)
{
    //
    // Is the SDRAM file system image present?
    //
    if(FileIsExternalImagePresent())
    {
        //
        // Count the number of JPEG images in the images directory of the
        // SDRAM file system image.
        //
        g_ulJPGsFound = FileCountJPEGFiles();

        //
        // Did we find any JPEG images?
        //
        if(g_ulJPGsFound)
        {
            //
            // We found at least 1 JPEG file in the SDRAM file system so
            // decompress this one.
            //
            g_ulJPGIndex = 0;
            ImageViewerGetImage(g_ulJPGIndex, false);
        }
        else
        {
            //
            // There are no images in the "images" directory of the SDRAM
            // file system or the directory does not exist.
            //
            JPEGWidgetTextSet(&g_sMainImage,
                              "No images found.");
        }
    }
    else
    {
        //
        // There is no SDRAM file system image present.
        //
        JPEGWidgetTextSet(&g_sMainImage,
                          "No file system image.");
    }

    //
    // Add the viewer widgets to the main application widget tree.
    //
    WidgetAdd((tWidget *)&g_sImageScreen, (tWidget *)&g_sImageHomeBtn);
}
