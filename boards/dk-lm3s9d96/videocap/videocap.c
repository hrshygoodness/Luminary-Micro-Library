//*****************************************************************************
//
// videocap.c - An example application supporting video capture using the
//              FPGA/Camera Daughter Board.
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
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/imgbutton.h"
#include "grlib/slider.h"
#include "drivers/vidwidget.h"
#include "drivers/kitronix320x240x16_fpga.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"
#include "drivers/camera.h"
#include "utils/ustdlib.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"
#include "images.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Video Capture (videocap)</h1>
//!
//! This example application makes use of the optional FGPA daughter board to
//! capture and display motion video on the LCD display. VGA resolution
//! (640x480) video is captured from the daughter board camera and shown
//! either scaled to the QVGA (320x240) resolution of the display or full size,
//! in which case 25% of the image is visible and the user may scroll over the
//! full image by dragging a finger on the video area of the touchscreen.
//!
//! The main screen of the application offers the following controls:
//! <dl>
//! <dt>Scale/Zoom</dt>
//! <dd>This button toggles the video display between scaled and zoomed modes.
//! In scaled mode, the 640x480 VGA video captured from the camera is
//! downscaled by a factor of two in each dimension making it fit on the
//! 320x240 QVGA display.  In zoomed mode, the video image is shown without
//! scaling and is clipped before being placed onto the display.  The user can
//! drag a finger or stylus over the touchscreen to scroll the area of the video
//! which is visible.</dd>
//! <dt>Freeze/Unfreeze</dt>
//! <dd>Use this button to freeze and unfreeze the video on the display.  When
//! the video is frozen, a copy of the image may be saved to SDCard as a
//! Windows bitmap file by pressing the ``Save'' button.</dd>
//! <dt>Controls/Save</dt>
//! <dd>When motion video is being displayed, this button displays ``Controls''
//! and allows you to adjust picture brightness, saturation and contrast by
//! means of three slider controls which are shown when the button is pressed.
//! Once you are finished with image adjustments, pressing the ``Main'' button
//! will return you to the main controls screen.  When video is frozen, this
//! button shows ``Save'' and pressing it will save the currently displayed
//! video image onto a microSD card if one is installed.</dd>
//! <dt>Hide</dt>
//! <dd>This button hides all user interface elements to offer a clearer view
//! of the video.  To show the buttons again, press the small, red ``Show''
//! button displayed in the bottom right corner of the screen.</dd>
//! </dl>
//!
//! Note that jumper ``PB4/POT'' on the main development kit board must be
//! removed when using the FPGA/Camera/LCD daughter board since the EPI signal
//! available on this pin is required for correct operation of the board.
//
//*****************************************************************************

//*****************************************************************************
//
// Macro used to pack the elements of a structure.
//
//*****************************************************************************
#if defined(ccs) ||             \
    defined(codered) ||         \
    defined(gcc) ||             \
    defined(rvmdk) ||           \
    defined(__ARMCC_VERSION) || \
    defined(sourcerygxx)
#define PACKED __attribute__ ((packed))
#elif defined(ewarm)
#define PACKED
#else
#error Unrecognized COMPILER!
#endif

//*****************************************************************************
//
// Structures related to Windows bitmaps
//
//*****************************************************************************
#ifdef ewarm
#pragma pack(1)
#endif

typedef struct
{
    unsigned short usType;
    unsigned long  ulSize;
    unsigned short usReserved1;
    unsigned short usReserved2;
    unsigned long  ulOffBits;
}
PACKED tBitmapFileHeader;

typedef struct
{
    unsigned long ulSize;
    long lWidth;
    long lHeight;
    unsigned short usPlanes;
    unsigned short usBitCount;
    unsigned long  ulCompression;
    unsigned long  ulSizeImage;
    long lXPelsPerMeter;
    long lYPelsPerMeter;
    unsigned long ulClrUsed;
    unsigned long ulClrImportant;
}
PACKED tBitmapInfoHeader;

#ifdef ewarm
#pragma pack()
#endif

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 10

//*****************************************************************************
//
// Image line buffer used when saving bitmaps.  This buffer is sized to hold
// a single 640-pixel wide line of 24bpp pixels.
//
//*****************************************************************************
#define SIZE_LINE_BUFFER (640 * 3)
unsigned char g_pucImgLineBuffer[SIZE_LINE_BUFFER];

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tCanvasWidget g_psScreens[];
extern tVideoWidget g_sBackground;
extern tCanvasWidget g_sTILogo;
extern tCanvasWidget g_sTIHorizLogo;
extern tCanvasWidget g_sAppName;
extern tCanvasWidget g_sPictureHeading;
extern tCanvasWidget g_sCtrlsHeading;
extern tCanvasWidget g_sErrorMessage;
extern tCanvasWidget g_sStellarisWareLogo;
extern tImageButtonWidget g_sFreezeButton;
extern tImageButtonWidget g_sScaleButton;
extern tImageButtonWidget g_sSavePictCtrlButton;
extern tImageButtonWidget g_sHideButton;
extern tImageButtonWidget g_sPCHideButton;
extern tImageButtonWidget g_sMainButton;
extern tImageButtonWidget g_sMirrorButton;
extern tImageButtonWidget g_sFlipButton;
extern tSliderWidget g_sBrightness;
extern tSliderWidget g_sSaturation;
extern tSliderWidget g_sContrast;

//*****************************************************************************
//
// Forward reference to the button press handlers.
//
//*****************************************************************************
void OnFreezeBtnPress(tWidget *pWidget);
void OnScaleBtnPress(tWidget *pWidget);
void OnPictureCtrlSaveBtnPress(tWidget *pWidget);
void OnHideBtnPress(tWidget *pWidget);
void OnMainBtnPress(tWidget *pWidget);
void OnMirrorBtnPress(tWidget *pWidget);
void OnFlipBtnPress(tWidget *pWidget);
void OnShowBtnPress(tWidget *pWidget);
void OnBrightnessChange(tWidget *pWidget, long lValue);
void OnSaturationChange(tWidget *pWidget, long lValue);
void OnContrastChange(tWidget *pWidget, long lValue);

//*****************************************************************************
//
// Canvas widgets forming the backgrounds to the various screens.
//
//*****************************************************************************
tCanvasWidget g_psScreens[] =
{
    //
    // The background for the main menu screen.
    //
    CanvasStruct(&g_sBackground, 0, &g_sTILogo,
           &g_sKitronix320x240x16_FPGA, 200, 0, 120, 240,
           CANVAS_STYLE_FILL, ClrMagenta, 0, 0, 0, 0, 0, 0),

    //
    // The background for the picture controls screen.
    //
    CanvasStruct(&g_sBackground, 0, &g_sTIHorizLogo,
           &g_sKitronix320x240x16_FPGA, 40, 40, 240, 160,
           CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0, 0, 0)
};

#define NUM_SCREENS (sizeof(g_psScreens)/sizeof(tCanvasWidget));
#define MAIN_SCREEN       0
#define PICT_CTRL_SCREEN  1

//*****************************************************************************
//
// The video widget acting as the background to the entire display.
//
//*****************************************************************************
tVideoInst sVideoInst;
VideoWidget(g_sBackground, WIDGET_ROOT, 0, &g_psScreens[MAIN_SCREEN],
       &g_sKitronix320x240x16_FPGA, 0, 0, 320, 240,
       VW_STYLE_VGA, ClrMagenta, 0, 0, 0, &sVideoInst);

//*****************************************************************************
//
// Button allowing the user to redisplay the main menu.
//
//*****************************************************************************
ImageButton(g_sShowButton, &g_sBackground, 0, 0,
            &g_sKitronix320x240x16_FPGA, 270, 220, 50, 20,
            (IB_STYLE_FILL | IB_STYLE_TEXT | IB_STYLE_IMAGE_OFF |
             IB_STYLE_KEYCAP_OFF | IB_STYLE_RELEASE_NOTIFY), ClrWhite,
             ClrRed, ClrRed, g_pFontCmss18, "Menu", 0, 0,
             0, 2, 2, 0, 0, OnShowBtnPress);

//*****************************************************************************
//
// Widgets forming the menu shown on the main screen of the application.
//
//*****************************************************************************
Canvas(g_sTILogo, &g_psScreens[MAIN_SCREEN], &g_sAppName, 0,
       &g_sKitronix320x240x16_FPGA, 0, 0, 120, 42,
       CANVAS_STYLE_IMG | CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0,
       g_pucTILogoStack120, 0);

Canvas(g_sAppName, &g_psScreens[MAIN_SCREEN], &g_sScaleButton, 0,
       &g_sKitronix320x240x16_FPGA, 120, 0, 200, 42,
       CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucFPGACameraImage, 0);

ImageButton(g_sScaleButton, &g_psScreens[MAIN_SCREEN], &g_sFreezeButton, 0,
            &g_sKitronix320x240x16_FPGA, 220, 60, 80, 30,
            (IB_STYLE_TEXT | IB_STYLE_KEYCAP_OFF |
            IB_STYLE_RELEASE_NOTIFY), ClrWhite, ClrRed, ClrRed,
            g_pFontCmss18, "Scale", g_pucRedBtn80x30Up, g_pucRedBtn80x30Down,
            0, 2, 2, 0, 0, OnScaleBtnPress);

ImageButton(g_sFreezeButton, &g_psScreens[MAIN_SCREEN], &g_sSavePictCtrlButton,
            0, &g_sKitronix320x240x16_FPGA, 220, 100, 80, 30,
            (IB_STYLE_TEXT | IB_STYLE_KEYCAP_OFF |
            IB_STYLE_RELEASE_NOTIFY), ClrWhite, ClrRed, ClrRed,
            g_pFontCmss18, "Freeze", g_pucRedBtn80x30Up, g_pucRedBtn80x30Down,
            0, 2, 2, 0, 0, OnFreezeBtnPress);

ImageButton(g_sSavePictCtrlButton, &g_psScreens[MAIN_SCREEN], &g_sHideButton, 0,
            &g_sKitronix320x240x16_FPGA, 220, 140, 80, 30,
            (IB_STYLE_TEXT | IB_STYLE_KEYCAP_OFF |
            IB_STYLE_RELEASE_NOTIFY), ClrWhite, ClrRed, ClrRed,
            g_pFontCmss18, "Controls", g_pucRedBtn80x30Up,
            g_pucRedBtn80x30Down, 0, 2, 2, 0, 0, OnPictureCtrlSaveBtnPress);

ImageButton(g_sHideButton, &g_psScreens[MAIN_SCREEN], &g_sErrorMessage, 0,
            &g_sKitronix320x240x16_FPGA, 220, 180, 80, 30,
            (IB_STYLE_TEXT | IB_STYLE_KEYCAP_OFF |
            IB_STYLE_RELEASE_NOTIFY), ClrWhite, ClrRed, ClrRed,
            g_pFontCmss18, "Hide", g_pucRedBtn80x30Up, g_pucRedBtn80x30Down,
            0, 2, 2, 0, 0, OnHideBtnPress);

Canvas(g_sErrorMessage, &g_psScreens[MAIN_SCREEN], 0, 0,
       &g_sKitronix320x240x16_FPGA, 10, 120, 200, 24,
       (CANVAS_STYLE_TEXT | CANVAS_STYLE_FILL | CANVAS_STYLE_TEXT_HCENTER |
       CANVAS_STYLE_TEXT_VCENTER), ClrMagenta, 0, ClrBlack, g_pFontCmss18b,
       "", 0, 0);

//*****************************************************************************
//
// Widgets forming the picture controls screen for the application.
//
//*****************************************************************************
Canvas(g_sTIHorizLogo, &g_psScreens[PICT_CTRL_SCREEN], &g_sStellarisWareLogo,
       0, &g_sKitronix320x240x16_FPGA, 40, 40, 240, 55,
       CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucTILogoHoriz240, 0);

Canvas(g_sStellarisWareLogo, &g_psScreens[PICT_CTRL_SCREEN], &g_sFlipButton, 0,
       &g_sKitronix320x240x16_FPGA, 58, 172, 143, 20,
       CANVAS_STYLE_IMG, 0, 0, 0, 0, 0, g_pucStellarisWare, 0);

ImageButton(g_sFlipButton, &g_psScreens[PICT_CTRL_SCREEN], &g_sMirrorButton, 0,
            &g_sKitronix320x240x16_FPGA, 220, 95, 50, 20,
            (IB_STYLE_FILL | IB_STYLE_TEXT | IB_STYLE_IMAGE_OFF |
            IB_STYLE_KEYCAP_OFF | IB_STYLE_RELEASE_NOTIFY), ClrWhite,
            ClrRed, ClrRed, g_pFontCmss16, "Flip", 0, 0,
            0, 2, 2, 0, 0, OnFlipBtnPress);

ImageButton(g_sMirrorButton, &g_psScreens[PICT_CTRL_SCREEN], &g_sMainButton, 0,
            &g_sKitronix320x240x16_FPGA, 220, 120, 50, 20,
            (IB_STYLE_FILL | IB_STYLE_TEXT | IB_STYLE_IMAGE_OFF |
            IB_STYLE_KEYCAP_OFF | IB_STYLE_RELEASE_NOTIFY), ClrWhite,
            ClrRed, ClrRed, g_pFontCmss16, "Mirror", 0, 0,
            0, 2, 2, 0, 0, OnMirrorBtnPress);

ImageButton(g_sMainButton, &g_psScreens[PICT_CTRL_SCREEN], &g_sPCHideButton, 0,
            &g_sKitronix320x240x16_FPGA, 220, 145, 50, 20,
            (IB_STYLE_FILL | IB_STYLE_TEXT | IB_STYLE_IMAGE_OFF |
            IB_STYLE_KEYCAP_OFF | IB_STYLE_RELEASE_NOTIFY), ClrWhite,
            ClrRed, ClrRed, g_pFontCmss16, "Main", 0, 0,
            0, 2, 2, 0, 0, OnMainBtnPress);

ImageButton(g_sPCHideButton, &g_psScreens[PICT_CTRL_SCREEN], &g_sBrightness, 0,
            &g_sKitronix320x240x16_FPGA, 220, 170, 50, 20,
            (IB_STYLE_FILL | IB_STYLE_TEXT | IB_STYLE_IMAGE_OFF |
            IB_STYLE_KEYCAP_OFF | IB_STYLE_RELEASE_NOTIFY), ClrWhite,
            ClrRed, ClrRed, g_pFontCmss16, "Hide", 0, 0,
            0, 2, 2, 0, 0, OnHideBtnPress);

Slider(g_sBrightness, &g_psScreens[PICT_CTRL_SCREEN], &g_sSaturation, 0,
       &g_sKitronix320x240x16_FPGA, 50, 95, 160, 20, 0, 255, 128,
       (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_TEXT |
       SL_STYLE_BACKG_TEXT | SL_STYLE_TEXT_OPAQUE |
       SL_STYLE_BACKG_TEXT_OPAQUE),
       ClrRed, ClrBlack, 0, ClrWhite, ClrWhite,
       g_pFontCm16, "Brightness", 0, 0, OnBrightnessChange);

Slider(g_sSaturation, &g_psScreens[PICT_CTRL_SCREEN], &g_sContrast, 0,
       &g_sKitronix320x240x16_FPGA, 50, 120, 160, 20, 0, 255, 128,
       (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_TEXT |
       SL_STYLE_BACKG_TEXT | SL_STYLE_TEXT_OPAQUE |
       SL_STYLE_BACKG_TEXT_OPAQUE),
       ClrRed, ClrBlack, 0, ClrWhite, ClrWhite,
       g_pFontCm16, "Saturation", 0, 0, OnSaturationChange);

Slider(g_sContrast, &g_psScreens[PICT_CTRL_SCREEN], 0, 0,
       &g_sKitronix320x240x16_FPGA, 50, 145, 160, 20, 0, 255, 128,
       (SL_STYLE_FILL | SL_STYLE_BACKG_FILL | SL_STYLE_TEXT |
       SL_STYLE_BACKG_TEXT | SL_STYLE_TEXT_OPAQUE |
       SL_STYLE_BACKG_TEXT_OPAQUE),
       ClrRed, ClrBlack, 0, ClrWhite, ClrWhite,
       g_pFontCm16, "Contrast", 0, 0, OnContrastChange);

//*****************************************************************************
//
// The index of the screen which is currently active.
//
//*****************************************************************************
unsigned long g_ulScreenIndex = 0;

//*****************************************************************************
//
// Various state flags
//
//*****************************************************************************
#define STATE_FLIP      0x01
#define STATE_FREEZE    0x02
#define STATE_MIRROR    0x04
#define STATE_DOWNSCALE 0x08

unsigned long g_ulStateFlags;

//*****************************************************************************
//
// Flags indicating commands that the main task must process.
//
//*****************************************************************************
volatile unsigned long g_ulCommandFlags;

//*****************************************************************************
//
// Bit numbers for each of the command flags set in g_ulCommandFlags.
//
//*****************************************************************************
#define COMMAND_SAVE 0

//*****************************************************************************
//
// Error message tick counter.
//
//*****************************************************************************
volatile unsigned long g_ulErrMessageTimer;

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static FIL g_sFileObject;

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
// This is the handler for this SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Is the error message timer running?
    //
    if(g_ulErrMessageTimer)
    {
        //
        // Yes - decrement it.
        //
        g_ulErrMessageTimer--;

        //
        // Has the timer expired?
        //
        if(!g_ulErrMessageTimer)
        {
            //
            // Yes - clear the current error message and redraw the widget.
            //
            CanvasFillColorSet(&g_sErrorMessage, ClrMagenta);
            CanvasTextSet(&g_sErrorMessage, "");
            WidgetPaint((tWidget *)&g_sErrorMessage);
        }
    }
}

//*****************************************************************************
//
// Show an error message on the screen for a particular time.
//
//*****************************************************************************
void
SetErrorMessage(char *pcMsg, unsigned long ulTimeout, tBoolean bImmediate)
{
    //
    // If we have been asked to update the message immediately, ensure that
    // the message queue has been emptied before we change the error message.
    //
    if(bImmediate)
    {
        WidgetMessageQueueProcess();
    }

    //
    // Have we been passed an empty string?
    //
    if(pcMsg[0] == '\0')
    {
        //
        // Yes - paint the widget background with the chromakey color so that
        // it disappears.
        //
        CanvasFillColorSet(&g_sErrorMessage, ClrMagenta);
    }
    else
    {
        //
        // Make the widget background white so that it highlights the text.
        //
        CanvasFillColorSet(&g_sErrorMessage, ClrWhite);
    }

    //
    // Set the new error message text and repaint the widget.
    //
    g_ulErrMessageTimer = ulTimeout;
    CanvasTextSet(&g_sErrorMessage, pcMsg);
    WidgetPaint((tWidget *)&g_sErrorMessage);

    //
    // If we have been asked to update the message immediately, ensure that
    // the repaint happens before we return.
    //
    if(bImmediate)
    {
        WidgetMessageQueueProcess();
    }
}

//*****************************************************************************
//
// Finds an unused, unique filename for use in the SD card file system.
//
// \param pcFilename points to a buffer into which the filename will be
// written.
// \param ulLen provides the length of the \e pcFilename buffer in bytes.  This
// must be at least 17 for the function to work correctly.
// \param pcExt is a pointer to a string containing the desired 3 character
// filename extension.
//
// This function queries the content of the root directory on the SD card
// file system and returns a new filename of the form "D:/imageXXX.EXT" where
// "D" is the logical drive number, "XXX" is a 3 digit decimal number and
// "EXT" is the extension passed in \e pcExt.
//
// \note The value "XXX" will be the lowest number which allows a new,
// unused filename to be created.  If, for example, files "image000.bmp" and
// "image002.bmp" exist in the directory, this function will return
// "image001.bmp" if passed "bmp" in the \e pcExt parameter rather than
// "image003.bmp".
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
FindNextFilename(char *pcFilename, unsigned long ulLen, const char *pcExt)
{
    unsigned long ulLoop;
    FRESULT eResult;

    //
    // Loop through the possible filenames until we find one we can't open.
    //
    for(ulLoop = 0; ulLoop < 1000; ulLoop++)
    {
        //
        // Generate a possible filename.
        //
        usnprintf(pcFilename, ulLen, "0:/image%03d.%s", ulLoop,
                  pcExt);

        //
        // Try to open this file.
        //
        eResult = f_open(&g_sFileObject, pcFilename, FA_OPEN_EXISTING);

        //
        // If the file doesn't exist, we've found a suitable filename.
        //
        if(eResult == FR_NO_FILE)
        {
            break;
        }
        else
        {
            //
            // If the return code is FR_OK, this implies that the file already
            // exists so we need to close this file and try the next possible
            // filename.
            //
            if(eResult == FR_OK)
            {
                //
                // Close the file.
                //
                f_close(&g_sFileObject);
            }
            else
            {
                //
                // Some other error was reported.  Abort the function and
                // return an error.
                //
                return(false);
            }
        }
    }

    //
    // If we drop out with ulLoop == 1000, we found the root directory
    // contains 1000 files called D:/image????.<ext> already.  Fail the call.
    //
    if(ulLoop == 1000)
    {
        return(false);
    }
    else
    {
        //
        // If we get here, the pcFilename buffer now contains a suitable name
        // for a file which does not already exist.  Tell the caller that all
        // is well.
        //
        return(true);
    }
}

void
OnFreezeBtnPress(tWidget *pWidget)
{
    //
    // Toggle our freeze state flag.
    //
    g_ulStateFlags ^= STATE_FREEZE;

    //
    // Freeze or unfreeze the video as required.
    //
    VideoWidgetFreezeSet((tWidget *)&g_sBackground,
                         (g_ulStateFlags & STATE_FREEZE) ? true : false);

    //
    // Change the button text to reflect the new state.
    //
    ImageButtonTextSet(pWidget, ((g_ulStateFlags & STATE_FREEZE) ?
                       "Unfreeze" : "Freeze"));

    //
    // Change the picture control/save button text to indicate the correct
    // operation of the button.
    //
    ImageButtonTextSet((tWidget *)&g_sSavePictCtrlButton,
                       ((g_ulStateFlags & STATE_FREEZE) ?
                       "Save" : "Controls"));

    //
    // Repaint the buttons.
    //
    WidgetPaint(pWidget);
    WidgetPaint((tWidget *)&g_sSavePictCtrlButton);
}

//*****************************************************************************
//
// Save the current video image to a bitmap file on the SDCard.  This function
// is called in the context of the main loop.
//
//*****************************************************************************
tBoolean
SaveImage(void)
{
    FRESULT fresult;
    tBoolean bRetcode;
    tBitmapFileHeader sBmpHdr;
    tBitmapInfoHeader sBmpInfo;
    unsigned short usCount;
    long lLoop;
    char pcFilename[20];

    //
    // Get a suitable filename for the new bitmap.
    //
    bRetcode = FindNextFilename(pcFilename, 20, "bmp");

    if(!bRetcode)
    {
        //
        // We can't find a suitable filename or the SDCard is not present.
        //
        SetErrorMessage("Can't create new file!", 2 * TICKS_PER_SECOND, true);
        return(false);
    }

    //
    // Open the file for reading.
    //
    fresult = f_open(&g_sFileObject, pcFilename, FA_WRITE | FA_CREATE_NEW);

    //
    // If there was some problem opening the file, then return
    // an error.
    //
    if(fresult != FR_OK)
    {
        SetErrorMessage("Error creating file!", 2 * TICKS_PER_SECOND, true);
        return(false);
    }

    //
    // Write the BITMAPFILEHEADER structure to the file
    //
    sBmpHdr.usType = 0x4D42; // "BM"
    sBmpHdr.ulSize = (sizeof(tBitmapFileHeader) + sizeof(tBitmapInfoHeader) +
                     (480 * 640 * 3));
    sBmpHdr.usReserved1 = 0;
    sBmpHdr.usReserved2 = 0;
    sBmpHdr.ulOffBits = sizeof(tBitmapFileHeader) +
                        sizeof(tBitmapInfoHeader);

    fresult = f_write(&g_sFileObject, &sBmpHdr, sizeof(tBitmapFileHeader),
                      &usCount);
    if(fresult != FR_OK)
    {
        SetErrorMessage("Error writing file!", 2 * TICKS_PER_SECOND, true);
        f_close(&g_sFileObject);
        return(false);
    }

    //
    // Write the BITMAPINFO structure to the file
    //
    sBmpInfo.ulSize = sizeof(tBitmapInfoHeader);
    sBmpInfo.lWidth = 640;
    sBmpInfo.lHeight = 480;
    sBmpInfo.usPlanes = 1;
    sBmpInfo.usBitCount = 24;
    sBmpInfo.ulCompression = 0; // BI_RGB
    sBmpInfo.ulSizeImage = (640 * 480 * 3);
    sBmpInfo.lXPelsPerMeter = 20000;
    sBmpInfo.lYPelsPerMeter = 20000;
    sBmpInfo.ulClrUsed = 0;
    sBmpInfo.ulClrImportant = 0;

    fresult = f_write(&g_sFileObject, &sBmpInfo, sizeof(tBitmapInfoHeader),
                      &usCount);
    if(fresult != FR_OK)
    {
        SetErrorMessage("Error writing header!", 2 * TICKS_PER_SECOND, true);
        f_close(&g_sFileObject);
        return(false);
    }

    //
    // Read and save the image one line at a time.  Note that Windows bitmaps
    // are saved upside down so we start with the bottom line and work upwards.
    //
    SetErrorMessage("Writing image...", 0, true);
    for(lLoop = 479; lLoop >= 0; lLoop--)
    {
        //
        // Read a line of pixels into our line buffer.
        //
        VideoWidgetImageDataGet((tWidget *)&g_sBackground, 0, lLoop, 640,
                                 (unsigned short *)g_pucImgLineBuffer, true);

        //
        // Write the line to the file.
        //
        fresult = f_write(&g_sFileObject, g_pucImgLineBuffer,
                          SIZE_LINE_BUFFER, &usCount);
        if(fresult != FR_OK)
        {
            f_close(&g_sFileObject);
            SetErrorMessage("Error writing data!\n", 2 * TICKS_PER_SECOND, true);
            return(false);
        }
    }

    //
    // Close the file.
    //
    fresult = f_close(&g_sFileObject);
    if(fresult != FR_OK)
    {
        SetErrorMessage("Error closing file!", 2 * TICKS_PER_SECOND, true);
    }
    else
    {
        SetErrorMessage("Bitmap saved", 2 * TICKS_PER_SECOND, true);
    }

    return(true);
}

//*****************************************************************************
//
// Toggle the zoom/scale state of the video.
//
//*****************************************************************************
void
OnScaleBtnPress(tWidget *pWidget)
{
    //
    // Toggle our downscale state flag.
    //
    g_ulStateFlags ^= STATE_DOWNSCALE;

    //
    // Tell the video widget to downscale or not as required.
    //
    VideoWidgetDownscaleSet((tWidget *)&g_sBackground,
                            (g_ulStateFlags & STATE_DOWNSCALE) ? true : false);

    //
    // Fix up the button text to indicate what it will do next time it is
    // pressed.
    //
    ImageButtonTextSet(pWidget, ((g_ulStateFlags & STATE_DOWNSCALE) ?
                       "Zoom" : "Scale"));

    //
    // Repaint the button.
    //
    WidgetPaint(pWidget);
}

//*****************************************************************************
//
// Toggle the video flip (vertical reflection) state.
//
//*****************************************************************************
void
OnFlipBtnPress(tWidget *pWidget)
{
    //
    // Toggle our flip state flag.
    //
    g_ulStateFlags ^= STATE_FLIP;

    //
    // Set the video mirror state appropriately.
    //
    VideoWidgetCameraFlipSet((tWidget *)&g_sBackground,
                             (g_ulStateFlags & STATE_FLIP) ? true : false);
}

//*****************************************************************************
//
// Toggle the video mirror (horizontal reflection) state.
//
//*****************************************************************************
void
OnMirrorBtnPress(tWidget *pWidget)
{
    //
    // Toggle our mirror state flag.
    //
    g_ulStateFlags ^= STATE_MIRROR;

    //
    // Set the video mirror state appropriately.
    //
    VideoWidgetCameraMirrorSet((tWidget *)&g_sBackground,
                               (g_ulStateFlags & STATE_MIRROR) ? true : false);
}

void
OnBrightnessChange(tWidget *pWidget, long lValue)
{
    VideoWidgetBrightnessSet((tWidget *)&g_sBackground, (unsigned char)lValue);
}

void
OnSaturationChange(tWidget *pWidget, long lValue)
{
    VideoWidgetSaturationSet((tWidget *)&g_sBackground, (unsigned char)lValue);
}

void
OnContrastChange(tWidget *pWidget, long lValue)
{
    VideoWidgetContrastSet((tWidget *)&g_sBackground, (unsigned char)lValue);
}

//*****************************************************************************
//
// Change the user interface to display the provided screen.
//
//*****************************************************************************
void
ShowScreen(unsigned long ulIndex)
{
    //
    // Remove the current screen widgets from the tree.
    //
    WidgetRemove((tWidget *)&g_psScreens[g_ulScreenIndex]);

    //
    // Replace them with the picture control screen widgets.
    //
    g_ulScreenIndex = ulIndex;
    WidgetAdd((tWidget *)&g_sBackground,
              (tWidget *)&g_psScreens[g_ulScreenIndex]);

    //
    // Force a screen repaint.
    //
    WidgetPaint(WIDGET_ROOT);
}

//*****************************************************************************
//
// Display the picture controls screen or save the current, frozen image
// (depending on the current state).
//
//*****************************************************************************
void
OnPictureCtrlSaveBtnPress(tWidget *pWidget)
{
    //
    // Is the image currently frozen?
    //
    if(g_ulStateFlags & STATE_FREEZE)
    {
        //
        // Yes - in this state, the button causes the image to be saved as a
        // bitmap.  Set a flag telling the main task to save the current image
        // to a file.
        //
        HWREGBITW(&g_ulCommandFlags, COMMAND_SAVE) = 1;
    }
    else
    {
        //
        // The image is not frozen so, in this state, the button calls up the
        // picture controls dialog.  We must make sure that we remove any
        // error message that is currently displayed before switching screen.
        //
        SetErrorMessage("", 0, true);
        ShowScreen(PICT_CTRL_SCREEN);
    }
}

//*****************************************************************************
//
// Return to the main screen.
//
//*****************************************************************************
void
OnMainBtnPress(tWidget *pWidget)
{
    ShowScreen(MAIN_SCREEN);
}

//*****************************************************************************
//
// Hide the user interface widgets.
//
//*****************************************************************************
void
OnHideBtnPress(tWidget *pWidget)
{
    //
    // Remove the current screen widgets from the tree.
    //
    WidgetRemove((tWidget *)&g_psScreens[g_ulScreenIndex]);

    //
    // Replace them with the full screen button so that we can reenable
    // them when the screen is tapped.
    //
    WidgetAdd((tWidget *)&g_sBackground, (tWidget *)&g_sShowButton);

    //
    // Force a screen repaint.
    //
    WidgetPaint(WIDGET_ROOT);
}

//*****************************************************************************
//
// Show the user interface widgets.
//
//*****************************************************************************
void
OnShowBtnPress(tWidget *pWidget)
{
    //
    // Remove the full screen button from the tree.
    //
    WidgetRemove((tWidget *)&g_sShowButton);

    //
    // Replace the widget subtree for the current screen.
    //
    WidgetAdd((tWidget *)&g_sBackground,
              (tWidget *)&g_psScreens[g_ulScreenIndex]);

    //
    // Force a screen repaint.
    //
    WidgetPaint(WIDGET_ROOT);
}

//*****************************************************************************
//
// Main application function for the FPGA Camera example.
//
//*****************************************************************************
int
main(void)
{
    FRESULT fresult;

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
    // Make sure we detected the FPGA daughter board since this application
    // requires it.
    //
    if(g_eDaughterType != DAUGHTER_FPGA)
    {
        //
        // We can't run - the FPGA daughter board doesn't seem to be there.
        //
        while(1)
        {
            //
            // Hang here on error.
            //
        }
    }

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Initialize the video capture widget.  This must be done before the
    // display driver is initialized.
    //
    VideoWidgetCameraInit(&g_sBackground, VIDEO_BUFF_BASE);

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_FPGAInit(GRAPHICS_BUFF_BASE);

    //
    // Enable the display backlight.
    //
    Kitronix320x240x16_FPGABacklight(true);

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBackground);

    //
    // Paint the widget tree to make sure they all appear on the display.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Now that everything is set up, turn on the video display.
    //
    VideoWidgetBlankSet((tWidget *)&g_sBackground, false);

    //
    // Mount the SDCard file system, using logical disk 0.
    //
    fresult = f_mount(0, &g_sFatFs);

    //
    // If we mounted the drive, try opening the root directory to see if there
    // is an SDCard present.
    //
    if(fresult == FR_OK)
    {
        DIR sDir;

        fresult = f_opendir(&sDir, "/");
    }

    //
    // Did we successfully mount and read the SDCard?
    //
    if(fresult != FR_OK)
    {
        SetErrorMessage("No SDCard found", TICKS_PER_SECOND * 3, false);
    }

    //
    // Loop forever, processing widget messages.
    //
    while(1)
    {
        //
        // Process any messages from or for the widgets.
        //
        WidgetMessageQueueProcess();

        //
        // Handle any operations which were signalled by widget handlers.
        //
        if(HWREGBITW(&g_ulCommandFlags, COMMAND_SAVE))
        {
            //
            // Save the current video image to a bitmap file on the SDCard.
            //
            SaveImage();
            HWREGBITW(&g_ulCommandFlags, COMMAND_SAVE) = 0;
        }
    }
}
