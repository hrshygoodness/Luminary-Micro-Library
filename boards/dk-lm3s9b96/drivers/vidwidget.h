//*****************************************************************************
//
// vidwidget.h - Prototypes for the motion video display widget class.
//
// Copyright (c) 2009-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#ifndef __VIDWIDGET_H__
#define __VIDWIDGET_H__

//*****************************************************************************
//
//! \addtogroup vidwidget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! This structure contains workspace fields used by the video widget.  It is
//! provided by the application when a widget is created but must not be
//! modified by the application after this.
//
//*****************************************************************************
typedef struct
{
    //
    //! The address of the start of the video capture buffer.  This address
    //! is relative to the start of the FPGA's 1MB SRAM memory.
    //
    unsigned long ulCapAddr;

    //
    //! The X coordinate of the saved start position used during dragging of
    //! the video image.
    //
    unsigned short sXStart;

    //
    //! The Y coordinate of the saved start position used during dragging of
    //! the video image.
    //
    unsigned short sYStart;

    //
    //! The width of the displayable video image in pixels.  This value will
    //! take into account whether or not video display downscaling is in use.
    //
    unsigned short usDispWidth;

    //
    //! The height of the displayable video image in pixels.  This value will
    //! take into account whether or not video display downscaling is in use.
    //
    unsigned short usDispHeight;

    //
    //! The width of the captured video image in pixels.
    //
    unsigned short usCapWidth;

    //
    //! The height of the captured video image in pixels.
    //
    unsigned short usCapHeight;

    //
    //! The stride of the video capture buffer in bytes.
    //
    unsigned short usStride;

    //! The X coordinate of the video image pixel corresponding to the top left
    //! of the display.
    //
    short sXOffset;

    //
    //! The Y coordinate of the video image pixel corresponding to the top left
    //! of the display.
    //
    short sYOffset;
}
tVideoInst;

//*****************************************************************************
//
//! This structure describes a video widget.  It contains the standard widget
//! fields in its sBase member followed by class-specific fields for the
//! video widget.
//
//*****************************************************************************
typedef struct
{
    //
    //! The generic widget information.  This structure is common to all widgets
    //! used by the Stellaris Graphics Library.
    //
    tWidget sBase;

    //
    //! The style for this widget.  This is a set of flags defined by
    //! VW_STYLE_xxx labels.
    //
    unsigned long ulStyle;

    //
    //! The 24-bit RGB color used as the video chromakey.  The widget
    //! background will be filled with this color and video will appear on top
    //! of areas colored using this value.  Note that the video plane occupies
    //! the whole screen regardless of the position and size of the video
    //! widget so video will show through areas of any other widgets which
    //! happen to use this color.  A good rule of thumb is to pick some color
    //! which is highly unlikely to appear in the user interface, such as
    //! bright magenta (ClrMagenta).
    //
    unsigned long ulKeyColor;

    //
    //! The 24-bit RGB color used to outline this video widget, if
    //! VW_STYLE_OUTLINE is selected.
    //
    unsigned long ulOutlineColor;

    //
    //! The width of the border to be drawn around the widget.  This is ignored
    //! if VW_STYLE_OUTLINE is not set.
    //
    unsigned char ucBorderWidth;

    //
    //! A pointer to the function to be called if the user scrolls the
    //! displayed video image.
    //
    void (*pfnOnScroll)(tWidget *pWidget, short sX, short sY);

    //
    //! A pointer to a structure in read/write memory that the widget can use
    //! to hold working variables.  The application must not modify the
    //! contents of this structure.
    //
    tVideoInst *psVideoInst;
}
tVideoWidget;

//*****************************************************************************
//
//! This flag indicates that the widget should be outlined.  The outline width
//! and color are controlled by the ulOutlineColor and ucBorderWidth fields of
//! the tVideoWidget structure.
//
//*****************************************************************************
#define VW_STYLE_OUTLINE        0x00000001

//*****************************************************************************
//
//! This flag indicates that the video widget should ignore all touchscreen
//! activity.  If not specified, the user will be able to scroll over the
//! captured video by dragging their finger over the touchscreen assuming
//! that the video capture size is greater than the widget area.
//
//*****************************************************************************
#define VW_STYLE_LOCKED        0x00000020

//*****************************************************************************
//
//! This flag indicates that the video widget should show the chromakey
//! color rather than the captured video image.  If absent, the widget will
//! show the current contents of the video capture buffer, the last
//! captured video image if #VW_STYLE_FREEZE is set or motion video otherwise.
//
//*****************************************************************************
#define VW_STYLE_BLANK         0x00000040

//*****************************************************************************
//
//! This flag allows motion video capture to be stopped or started.  When set,
//! motion video will be captured into the current capture buffer.  When clear,
//! no motion video will be captured.  In this case, if #VW_STYLE_BLANK
//! is not also set, the display will show the current contents of the video
//! capture buffer, usually the last video frame captured.
//
//*****************************************************************************
#define VW_STYLE_FREEZE        0x00000080

//*****************************************************************************
//
//! This flag sets the camera to capture VGA (640x480) resolution images.  If
//! absent, QVGA (320x240) images are captured.
//
//*****************************************************************************
#define VW_STYLE_VGA           0x00000100

//*****************************************************************************
//
//! This flag causes the captured video to be downscaled by a factor of 2 in
//! both width and height on display.  If #VW_STYLE_VGA is set, this flag can
//! be used to scale the video for display on the 320x240 display while
//! maintaining the full resolution image in the video capture buffer.  If
//! #VW_STYLE_VGA is not set and the system is capturing QVGA resolution video,
//! this flag will allow the QVGA image to be downscaled and shown in the
//! top left quadrant of the display.
//!
//! This flag does not affect the size of the video image captured.
//
//*****************************************************************************
#define VW_STYLE_DOWNSCALE     0x00000200

//*****************************************************************************
//
//! This flag causes the captured video to be mirrored (flipped horizontally)
//! and is useful for video-conferencing-type applications where a user is
//! viewing video of themselves.  In these situations people are typically
//! more comfortable viewing a mirror image.  The flag affects the image stored
//! in the video capture buffer.
//
//*****************************************************************************
#define VW_STYLE_MIRROR        0x00000400

//*****************************************************************************
//
//! This flag causes the captured video to be flipped vertically.  Used in
//! combination with #VW_STYLE_MIRROR, this flag allows video to be rotated
//! 180 degrees.  It affects the image stored in the video capture buffer.
//
//*****************************************************************************
#define VW_STYLE_FLIP          0x00000800

//*****************************************************************************
//
//! Declares an initialized video widget data structure.
//!
//! \param pParent is a pointer to the parent widget.
//! \param pNext is a pointer to the sibling widget.
//! \param pChild is a pointer to the first child widget.
//! \param pDisplay is a pointer to the display on which to draw the video
//! widget.
//! \param lX is the X coordinate of the upper left corner of the video widget.
//! \param lY is the Y coordinate of the upper left corner of the video widget.
//! \param lWidth is the width of the video widget.
//! \param lHeight is the height of the video widget.
//! \param ulStyle is the style to be applied to the video widget.
//! \param ulKeyColor is the color used to fill in the video widget and
//! represents the color above which video image pixels will be shown.
//! \param ulOutlineColor is the color used to outline the video widget if \b
//! #VW_STYLE_OUTLINE is specified.
//! \param ucBorderWidth is the width of the border to paint if \b
//! #VW_STYLE_OUTLINE is specified.
//! \param pfnOnScroll is a pointer to the function that is called when the
//! image is scrolled by the user.  Scrolling is enabled if style flag \b
//! #VW_STYLE_LOCKED is not specified and the video capture size is larger
//! than the display size.
//! \param psInst is a pointer to a structure in read/write memory that the
//! widget can use to hold working variables.
//!
//! This macro provides an initialized video widget data structure, which
//! can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).  This must be
//! assigned to a variable, such as:
//!
//! \verbatim
//!     tVideoWidget g_sVideoArea = VideoWidgetStruct(...);
//! \endverbatim
//!
//! \e ulStyle is the logical OR of the following:
//!
//! - \b #VW_STYLE_OUTLINE to indicate that the video widget should be outlined.
//! - \b #VW_STYLE_LOCKED to indicate that the video widget should ignore all
//!   user input and merely display the image.  If this flag is clear, the user
//!   may scroll the video image within the widget by dragging a finger on the
//!   touchscreen.
//! - \b #VW_STYLE_BLANK to indicate that video is not to be shown.  If set, the
//!   widget will display the key color specified in ulKeyColor instead of
//!   any captured video image.  This flag may be used to hide or show the
//!   video image.
//! - \b #VW_STYLE_FREEZE to indicate that motion video capture is
//!   disabled.  If set, the widget will show the last captured image, if any
//!   exists.  This flag may be used to freeze and unfreeze motion video.
//! - \b #VW_STYLE_VGA to indicate that 640x480 resolution video should be
//!   captured.  If clear, QVGA (320x240) video will be captured.
//! - \b #VW_STYLE_DOWNSCALE to indicate that the video should be downscaled by
//!   a factor of two in both width and height on display.  This flag may be
//!   used to scale VGA video appropriately for display on a QVGA display or
//!   to show QVGA video in the top left quadrant of the display.   This flag
//!   does not affect the image stored in the video capture buffer.
//! - \b #VW_STYLE_MIRROR to indicate that the captured video must be flipped
//!   horizontally.  This flag affects the image stored in the video capture
//!   buffer.
//! - \b #VW_STYLE_FLIP to indicate that the captured video must be flipped
//!   vertically.  This flag affects the image stored in the video capture
//!   buffer.
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define VideoWidgetStruct(pParent, pNext, pChild, pDisplay, lX, lY, lWidth,\
                         lHeight, ulStyle, ulKeyColor, ulOutlineColor,     \
                         ucBorderWidth, pfnOnScroll, psInst)               \
        {                                                                  \
            {                                                              \
                sizeof(tVideoWidget),                                      \
                (tWidget *)(pParent),                                      \
                (tWidget *)(pNext),                                        \
                (tWidget *)(pChild),                                       \
                pDisplay,                                                  \
                {                                                          \
                    lX,                                                    \
                    lY,                                                    \
                    (lX) + (lWidth) - 1,                                   \
                    (lY) + (lHeight) - 1                                   \
                },                                                         \
                VideoWidgetMsgProc,                                        \
            },                                                             \
            ulStyle,                                                       \
            ulKeyColor,                                                    \
            ulOutlineColor,                                                \
            ucBorderWidth,                                                 \
            pfnOnScroll,                                                   \
            psInst                                                         \
        }

//*****************************************************************************
//
//! Declares an initialized variable containing a video widget data structure.
//!
//! \param sName is the name of the variable to be declared.
//! \param pParent is a pointer to the parent widget.
//! \param pNext is a pointer to the sibling widget.
//! \param pChild is a pointer to the first child widget.
//! \param pDisplay is a pointer to the display on which to draw the video
//! widget.
//! \param lX is the X coordinate of the upper left corner of the video widget.
//! \param lY is the Y coordinate of the upper left corner of the video widget.
//! \param lWidth is the width of the video widget.
//! \param lHeight is the height of the video widget.
//! \param ulStyle is the style to be applied to the video widget.
//! \param ulKeyColor is the color used to fill in the video widget and
//! represents the color above which video image pixels will be shown.
//! \param ulOutlineColor is the color used to outline the video widget if \b
//! #VW_STYLE_OUTLINE is specified.
//! \param ucBorderWidth is the width of the border to paint if \b
//! #VW_STYLE_OUTLINE is specified.
//! \param pfnOnScroll is a pointer to the function that is called when the
//! image is scrolled by the user.  Scrolling is enabled if style flag \b
//! #VW_STYLE_LOCKED is not specified and the video capture size is larger
//! than the display size.
//! \param psInst is a pointer to a structure in read/write memory that the
//! widget can use to hold working variables.
//!
//! This macro provides an initialized video widget data structure, which
//! can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).
//!
//! \e ulStyle is the logical OR of the following:
//!
//! - \b #VW_STYLE_OUTLINE to indicate that the video widget should be outlined.
//! - \b #VW_STYLE_LOCKED to indicate that the video widget should ignore all
//!   user input and merely display the image.  If this flag is clear, the user
//!   may scroll the video image within the widget by dragging a finger on the
//!   touchscreen.
//! - \b #VW_STYLE_BLANK to indicate that video is not to be shown.  If set, the
//!   widget will display the key color specified in ulKeyColor instead of
//!   any captured video image.  This flag may be used to hide or show the
//!   video image.
//! - \b #VW_STYLE_FREEZE to indicate that motion video capture is
//!   disabled.  If set, the widget will show the last captured image, if any
//!   exists.  This flag may be used to freeze and unfreeze motion video.
//! - \b #VW_STYLE_VGA to indicate that 640x480 resolution video should be
//!   captured.  If clear, QVGA (320x240) video will be captured.
//! - \b #VW_STYLE_DOWNSCALE to indicate that the video should be downscaled by
//!   a factor of two in both width and height on display.  This flag may be
//!   used to scale VGA video appropriately for display on a QVGA display or
//!   to show QVGA video in the top left quadrant of the display.   This flag
//!   does not affect the image stored in the video capture buffer.
//! - \b #VW_STYLE_MIRROR to indicate that the captured video must be flipped
//!   horizontally.  This flag affects the image stored in the video capture
//!   buffer.
//! - \b #VW_STYLE_FLIP to indicate that the captured video must be flipped
//!   vertically.  This flag affects the image stored in the video capture
//!   buffer.
//!
//! \note The FPGA on the daughter board does not support positioning of the
//! video image origin anywhere on the display other than at the top left.  As
//! a result, if the video widget is placed elsewhere on the screen, it will
//! show the portion of the underlying video corresponding to its area but it
//! will not be possible to reposition the video such that it is aligned with
//! the top left corner of the widget.  It is recommended, therefore, that the
//! video widget be positioned in the top left corner of the display and take
//! up the full area of the display.  In this case, there are three video
//! display possibilities:
//! <ol>
//! <li>QVGA capture with scaling disabled or VGA capture with scaling
//! enabled results in the full image being visible on the display.</li>
//! <li>VGA capture with scaling disabled results in a quarter of the video
//! occupying the full screen area.  In this case, the visible portion of the
//! video image may be chosen by scrolling.</li>
//! <li>QVGA capture with scaling enabled results in the full video image being
//! visible in the top left quadrant of the display.</li>
//! </ol>
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define VideoWidget(sName, pParent, pNext, pChild, pDisplay, lX, lY, lWidth, \
                   lHeight, ulStyle, ulKeyColor, ulOutlineColor,             \
                   ucBorderWidth, pfnOnScroll, psInst)                       \
        tVideoWidget sName =                                                 \
            VideoWidgetStruct(pParent, pNext, pChild, pDisplay, lX, lY,      \
                             lWidth, lHeight, ulStyle, ulKeyColor,           \
                             ulOutlineColor, ucBorderWidth, pfnOnScroll,     \
                             psInst)

//*****************************************************************************
//
//! Sets the function to call when the video image is scrolled.
//!
//! \param pWidget is a pointer to the video widget to modify.
//! \param pfnOnScrll is a pointer to the scroll callback function.
//!
//! This macro sets the callback function to be notified when this widget's
//! video image is scrolled by dragging a finger or stylus over the video area.
//! This function will be called any time the video image is repositioned as
//! a result of user touchscreen input.
//!
//! \return None.
//
//*****************************************************************************
#define VideoWidgetScrollCallbackSet(pWidget, pfnOnScrll) \
        do                                                \
        {                                                 \
            tVideoWidget *pW = pWidget;                   \
            pW->pfnOnScroll = pfnOnScrll;                 \
        }                                                 \
        while(0)

//*****************************************************************************
//
//! Sets the outline color of a video widget.
//!
//! \param pWidget is a pointer to the video widget to be modified.
//! \param ulColor is the 24-bit RGB color to use to outline the widget.
//!
//! This function changes the color used to outline the video widget on the
//! display.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define VideoWidgetOutlineColorSet(pWidget, ulColor) \
        do                                           \
        {                                            \
            tVideoWidget *pW = pWidget;              \
            pW->ulOutlineColor = ulColor;            \
        }                                            \
        while(0)

//*****************************************************************************
//
//! Sets the outline width of a video widget.
//!
//! \param pWidget is a pointer to the video widget to be modified.
//! \param ucWidth
//!
//! This function changes the width of the border around the video widget.
//! The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define VideoWidgetOutlineWidthSet(pWidget, ucWidth) \
        do                                           \
        {                                            \
            tVideoWidget *pW = pWidget;              \
            pW->ucBorderWidth = ucWidth;             \
        }                                            \
        while(0)

//*****************************************************************************
//
//! Disables outlining of a video widget.
//!
//! \param pWidget is a pointer to the video widget to modify.
//!
//! This function disables the outlining of a video widget.  The display
//! is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define VideoWidgetOutlineOff(pWidget)          \
        do                                      \
        {                                       \
            tVideoWidget *pW = pWidget;         \
            pW->ulStyle &= ~(VW_STYLE_OUTLINE); \
        }                                       \
        while(0)

//*****************************************************************************
//
//! Enables outlining of a video widget.
//!
//! \param pWidget is a pointer to the video widget to modify.
//!
//! This function enables the outlining of a video widget.  The display
//! is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define VideoWidgetOutlineOn(pWidget)        \
        do                                   \
        {                                    \
            tVideoWidget *pW = pWidget;      \
            pW->ulStyle |= VW_STYLE_OUTLINE; \
        }                                    \
        while(0)

//*****************************************************************************
//
//! Locks a video widget making it ignore pointer input.
//!
//! \param pWidget is a pointer to the widget to modify.
//!
//! This function locks a video widget and makes it ignore all pointer input.
//! When locked, a widget acts as a passive canvas and the video shown in the
//! widget cannot be scrolled.
//!
//! \return None.
//
//*****************************************************************************
#define VideoWidgetLock(pWidget)                \
        do                                      \
        {                                       \
            tVideoWidget *pW = (pWidget);       \
            pW->ulStyle |= VW_STYLE_LOCKED;     \
        }                                       \
        while(0)

//*****************************************************************************
//
//! Unlocks a video widget making it respond to touchscreen input.
//!
//! \param pWidget is a pointer to the widget to modify.
//!
//! This function unlocks a video widget.  When unlocked, a video widget will
//! respond to touchscreen input by scrolling the video it is currently
//! displaying assuming that the video image being displayed is larger than the
//! display dimensions.
//!
//! \return None.
//
//*****************************************************************************
#define VideoWidgetUnlock(pWidget)              \
        do                                      \
        {                                       \
            tVideoWidget *pW = (pWidget);       \
            pW->ulStyle &= ~(VW_STYLE_LOCKED);  \
        }                                       \
        while(0)

//*****************************************************************************
//
// Prototypes for the video widget APIs.
//
//*****************************************************************************
extern long VideoWidgetMsgProc(tWidget *pWidget, unsigned long ulMsg,
                               unsigned long ulParam1, unsigned long ulParam2);
extern void VideoWidgetInit(tVideoWidget *pWidget, const tDisplay *pDisplay,
                            unsigned long ulBufAddr, tBoolean bVGA, long lX,
                            long lY, long lWidth, long lHeight,
                            tVideoInst *psInst);
extern void VideoWidgetCameraInit(tVideoWidget *pWidget,
                                  unsigned long ulBufAddr);
extern void VideoWidgetKeyColorSet(tWidget *pWidget, unsigned long ulColor);
extern void VideoWidgetBlankSet(tWidget *pWidget, tBoolean bBlank);
extern void VideoWidgetResolutionSet(tWidget *pWidget, tBoolean bVGA);
extern void VideoWidgetFreezeSet(tWidget *pWidget, tBoolean bFreeze);
extern void VideoWidgetDownscaleSet(tWidget *pWidget, tBoolean bDownscale);
extern void VideoWidgetCameraFlipSet(tWidget *pWidget, tBoolean bFlip);
extern void VideoWidgetCameraMirrorSet(tWidget *pWidget, tBoolean bMirror);
extern void VideoWidgetBrightnessSet(tWidget *pWidget,
                                     unsigned char ucBrightness);
extern void VideoWidgetSaturationSet(tWidget *pWidget,
                                     unsigned char ucSaturation);
extern void VideoWidgetContrastSet(tWidget *pWidget,
                                   unsigned char ucContrast);
extern void VideoWidgetImageDataGet(tWidget *pWidget, unsigned short usX,
                                    unsigned short usY, unsigned long ulPixels,
                                    unsigned short *pusBuffer, tBoolean b24bpp);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
#endif // __VIDWIDGET_H__

