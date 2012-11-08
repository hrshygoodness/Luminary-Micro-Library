//*****************************************************************************
//
// jpgwidget.h - Prototypes for the JPEG image display/button widget class.
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

#ifndef __JPGWIDGET_H__
#define __JPGWIDGET_H__

//*****************************************************************************
//
//! \addtogroup jpgwidget_api
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
//! The structure containing workspace fields used by the JPEG widget in
//! decompressing and displaying the JPEG image.  This structure must not be
//! modified by the application using the widget.
//
//*****************************************************************************
typedef struct
{
    //
    //! The width of the decompressed JPEG image in pixels.
    //
    unsigned short usWidth;

    //
    //! The height of the decompressed JPEG image in lines.
    //
    unsigned short usHeight;

    //
    //! The current X image display offset (pan).
    //
    short sXOffset;

    //
    //! The current Y image display offset (scan).
    //
    short sYOffset;

    //
    //! The x coordinate of the screen position corresponding to the last
    //! scrolling calculation check for a JPEGCanvas type widget.
    //
    short sXStart;

    //
    //! The y coordinate of the screen position corresponding to the last
    //! scrolling calculation check for a JPEGCanvas type widget.
    //
    short sYStart;

    //
    //! A pointer to the SDRAM buffer containing the decompressed JPEG image.
    //
    unsigned short *pusImage;
}
tJPEGInst;

//*****************************************************************************
//
//! The structure that describes a JPEG widget.
//
//*****************************************************************************
typedef struct
{
    //
    //! The generic widget information.
    //
    tWidget sBase;

    //
    //! The style for this widget.  This is a set of flags defined by
    //! JW_STYLE_xxx.
    //
    unsigned long ulStyle;

    //
    //! The 24-bit RGB color used to fill this JPEG widget, if JW_STYLE_FILL is
    //! selected.
    //
    unsigned long ulFillColor;

    //
    //! The 24-bit RGB color used to outline this JPEG widget, if
    //! JW_STYLE_OUTLINE is selected.
    //
    unsigned long ulOutlineColor;

    //
    //! The 24-bit RGB color used to draw text on this JPEG widget, if
    //! JW_STYLE_TEXT is selected.
    //
    unsigned long ulTextColor;

    //
    //! A pointer to the font used to render the JPEG widget text, if
    //! JW_STYLE_TEXT is selected.
    //
    const tFont *pFont;

    //
    //! A pointer to the text to draw on this JPEG widget, if JW_STYLE_TEXT is
    //! selected.
    //
    const char *pcText;

    //
    //! A pointer to the compressed JPEG image to be drawn onto this widget.
    //! If NULL, the widget will be filled with the provided background color
    //! if painted.
    //
    const unsigned char *pucImage;

    //
    //! The number of bytes of compressed data in the image pointed to by
    //! pucImage.
    //
    unsigned long ulImageLen;

    //
    //! The width of the border to be drawn around the widget.  This is ignored
    //! if JW_STYLE_OUTLINE is not set.
    //
    unsigned char ucBorderWidth;

    //
    //! A pointer to the function to be called when the button is pressed
    //! This is ignored if JW_STYLE_BUTTON is not set.
    //
    void (*pfnOnClick)(tWidget *pWidget);

    //
    //! A pointer to the function to be called if the user scrolls the
    //! displayed image.  This is ignored if JW_STYLE_BUTTON is set.
    //
    void (*pfnOnScroll)(tWidget *pWidget, short sX, short sY);

    //
    //! The following structure contains all the workspace fields required
    //! by the widget.  The client must initialize this with a valid pointer
    //! to a read/write structure.
    //
    tJPEGInst *psJPEGInst;
}
tJPEGWidget;

//*****************************************************************************
//
//! This flag indicates that the widget should be outlined.
//
//*****************************************************************************
#define JW_STYLE_OUTLINE        0x00000001

//*****************************************************************************
//
//! This flag indicates that the widget should act as a button rather than as
//! a display surface.
//
//*****************************************************************************
#define JW_STYLE_BUTTON         0x00000002

//*****************************************************************************
//
//! This flag indicates that the JPEG widget should have text drawn on it.
//
//*****************************************************************************
#define JW_STYLE_TEXT           0x00000004

//*****************************************************************************
//
//! This flag indicates that the JPEG widget's background area should be filled
//! with color even when there is an image to display.
//
//*****************************************************************************
#define JW_STYLE_FILL           0x00000008

//*****************************************************************************
//
//! This flag indicates that the JPEG widget's image should be repainted as
//! the user scrolls over it.  This is CPU intensive but looks better than
//! the alternative which only repaints the image when the user ends their
//! touchscreen drag.
//
//*****************************************************************************
#define JW_STYLE_SCROLL        0x00000010

//*****************************************************************************
//
//! This flag indicates that the JPEG widget should ignore all touchscreen
//! activity.
//
//*****************************************************************************
#define JW_STYLE_LOCKED        0x00000020

//*****************************************************************************
//
//! This flag indicates that the JPEG widget is pressed.
//
//*****************************************************************************
#define JW_STYLE_PRESSED       0x00000040

//*****************************************************************************
//
//! This flag indicates that the JPEG widget callback should be made when
//! the widget is released rather than when it is pressed.  This style flag is
//! ignored if JW_STYLE_BUTTON is not set.
//
//*****************************************************************************
#define JW_STYLE_RELEASE_NOTIFY 0x00000080

//*****************************************************************************
//
//! Declares an initialized JPEG image widget data structure.
//!
//! \param pParent is a pointer to the parent widget.
//! \param pNext is a pointer to the sibling widget.
//! \param pChild is a pointer to the first child widget.
//! \param pDisplay is a pointer to the display on which to draw the push
//! button.
//! \param lX is the X coordinate of the upper left corner of the JPEG widget.
//! \param lY is the Y coordinate of the upper left corner of the JPEG widget.
//! \param lWidth is the width of the JPEG widget.
//! \param lHeight is the height of the JPEG widget.
//! \param ulStyle is the style to be applied to the JPEG widget.
//! \param ulFillColor is the color used to fill in the JPEG widget.
//! \param ulOutlineColor is the color used to outline the JPEG widget.
//! \param ulTextColor is the color used to draw text on the JPEG widget.
//! \param pFont is a pointer to the font to be used to draw text on the push
//! button.
//! \param pcText is a pointer to the text to draw on this JPEG widget.
//! \param pucImage is a pointer to the compressed image to draw on this JPEG
//! widget.
//! \param ulImgLen is the length of the data pointed to by pucImage.
//! \param ucBorderWidth is the width of the border to paint if \b
//! #JW_STYLE_OUTLINE is specified.
//! \param pfnOnClick is a pointer to the function that is called when the JPEG
//! button is pressed assuming \b #JW_STYLE_BUTTON is specified.
//! \param pfnOnScroll is a pointer to the function that is called when the
//! image is scrolled assuming \b #JW_STYLE_BUTTON is not specified.
//! \param psInst is a pointer to a read/write tJPEGInst structure that the
//! widget can use for workspace.
//!
//! This macro provides an initialized jpeg image widget data structure, which
//! can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).  This must be
//! assigned to a variable, such as:
//!
//! \verbatim
//!     tJPEGWidget g_sImageButton = JPEGWidgetStruct(...);
//! \endverbatim
//!
//! Or, in an array of variables:
//!
//! \verbatim
//!     tJPEGWidget g_psImageButtons[] =
//!     {
//!         JPEGWidgetStruct(...),
//!         JPEGWidgetStruct(...)
//!     };
//! \endverbatim
//!
//! \e ulStyle is the logical OR of the following:
//!
//! - \b #JW_STYLE_OUTLINE to indicate that the JPEG widget should be outlined.
//! - \b #JW_STYLE_BUTTON to indicate that the JPEG widget should act as a
//!   button and that calls should be made to pfnOnClick when it is pressed or
//!   released (depending upon the state of \b #JW_STYLE_RELEASE_NOTIFY).
//!   If absent, the widget acts as a canvas which allows the image, if larger
//!   than the widget display area to be scrolled by dragging a finger on the
//!   touchscreen.  In this case, the pfnOnScroll callback will be called when
//!   any scrolling is needed.
//! - \b #JW_STYLE_TEXT to indicate that the JPEG widget should have text drawn
//!   on it (using \e pFont and \e pcText).
//! - \b #JW_STYLE_FILL to indicate that the JPEG widget should have its
//!   background filled with color (specified by \e ulFillColor).
//! - \b #JW_STYLE_SCROLL to indicate that the JPEG widget should be redrawn
//!   automatically each time the pointer is moved (touchscreen is dragged)
//!   rather than waiting for the gesture to end then redrawing once.  A client
//!   may chose to omit this style flag and call WidgetPaint() from within the
//!   pfnOnScroll callback at a rate deemed acceptable for the application.
//! - \b #JW_STYLE_LOCKED to indicate that the JPEG widget should ignore all
//!   user input and merely display the image.  If this flag is set,
//!   #JW_STYLE_SCROLL is ignored.
//! - \b #JW_STYLE_RELEASE_NOTIFY to indicate that the callback should be made
//!   when the button-style widget is released.  If absent, the callback is
//!   called when the widget is initially pressed.  This style flag is ignored
//!   unless \b #JW_STYLE_BUTTON is specified.
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define JPEGWidgetStruct(pParent, pNext, pChild, pDisplay, lX, lY, lWidth,\
                         lHeight, ulStyle, ulFillColor, ulOutlineColor,   \
                         ulTextColor, pFont, pcText, pucImage, ulImgLen,  \
                         ucBorderWidth, pfnOnClick, pfnOnScroll, psInst)  \
        {                                                                 \
            {                                                             \
                sizeof(tJPEGWidget),                                      \
                (tWidget *)(pParent),                                     \
                (tWidget *)(pNext),                                       \
                (tWidget *)(pChild),                                      \
                pDisplay,                                                 \
                {                                                         \
                    lX,                                                   \
                    lY,                                                   \
                    (lX) + (lWidth) - 1,                                  \
                    (lY) + (lHeight) - 1                                  \
                },                                                        \
                JPEGWidgetMsgProc,                                        \
            },                                                            \
            ulStyle,                                                      \
            ulFillColor,                                                  \
            ulOutlineColor,                                               \
            ulTextColor,                                                  \
            pFont,                                                        \
            pcText,                                                       \
            pucImage,                                                     \
            ulImgLen,                                                     \
            ucBorderWidth,                                                \
            pfnOnClick,                                                   \
            pfnOnScroll,                                                  \
            psInst                                                        \
        }

//*****************************************************************************
//
//! Declares an initialized variable containing a JPEG button data structure.
//!
//! \param sName is the name of the variable to be declared.
//! \param pParent is a pointer to the parent widget.
//! \param pNext is a pointer to the sibling widget.
//! \param pChild is a pointer to the first child widget.
//! \param pDisplay is a pointer to the display on which to draw the push
//! button.
//! \param lX is the X coordinate of the upper left corner of the JPEG widget.
//! \param lY is the Y coordinate of the upper left corner of the JPEG widget.
//! \param lWidth is the width of the JPEG widget.
//! \param lHeight is the height of the JPEG widget.
//! \param ulStyle is the style to be applied to the JPEG widget.
//! \param ulFillColor is the color used to fill in the JPEG widget.
//! \param ulOutlineColor is the color used to outline the JPEG widget.
//! \param ulTextColor is the color used to draw text on the JPEG widget.
//! \param pFont is a pointer to the font to be used to draw text on the push
//! button.
//! \param pcText is a pointer to the text to draw on this JPEG widget.
//! \param pucImage is a pointer to the compressed image to draw on this JPEG
//! widget.
//! \param ulImgLen is the length of the data pointed to by pucImage.
//! \param ucBorderWidth is the width of the border to paint if
//! \e JW_STYLE_OUTLINE is specified.
//! \param pfnOnClick is a pointer to the function that is called when the JPEG
//! button is pressed or released.
//! \param psInst is a pointer to a read/write tJPEGInst structure that the
//! widget can use for workspace.
//!
//! This macro provides an initialized JPEG button widget data structure, which
//! can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).
//!
//! A JPEG button displays an image centered within the widget area and
//! sends OnClick messages to the client whenever a user presses or releases
//! the touchscreen within the widget area (depending upon the state of the
//! \e #JW_STYLE_RELEASE_NOTIFY style flag).  A JPEG button does not support
//! image scrolling.
//!
//! \e ulStyle is the logical OR of the following:
//!
//! - \b #JW_STYLE_OUTLINE to indicate that the JPEG widget should be outlined.
//! - \b #JW_STYLE_TEXT to indicate that the JPEG widget should have text drawn
//!   on it (using \e pFont and \e pcText).
//! - \b #JW_STYLE_FILL to indicate that the JPEG widget should have its
//!   background filled with color (specified by \e ulFillColor).
//! - \b #JW_STYLE_SCROLL to indicate that the JPEG widget should be redrawn
//!   automatically each time the pointer is moved (touchscreen is dragged)
//!   rather than waiting for the gesture to end then redrawing once.  A client
//!   may chose to omit this style flag and call WidgetPaint() from within the
//!   pfnOnScroll callback at a rate deemed acceptable for the application.
//! - \b #JW_STYLE_LOCKED to indicate that the JPEG widget should ignore all
//!   user input and merely display the image.  If this flag is set,
//!   #JW_STYLE_SCROLL is ignored.
//! - \b #JW_STYLE_RELEASE_NOTIFY to indicate that the callback should be made
//!   when the button-style widget is released.  If absent, the callback is
//!   called when the widget is initially pressed.  This style flag is ignored
//!   unless \b #JW_STYLE_BUTTON is specified.
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define JPEGButton(sName, pParent, pNext, pChild, pDisplay, lX, lY, lWidth,  \
                   lHeight, ulStyle, ulFillColor, ulOutlineColor,            \
                   ulTextColor, pFont, pcText, pucImage, ulImgLen,           \
                   ucBorderWidth, pfnOnClick, psInst)                        \
        tJPEGWidget sName =                                                  \
            JPEGWidgetStruct(pParent, pNext, pChild, pDisplay, lX, lY,       \
                             lWidth, lHeight, (ulStyle | JW_STYLE_BUTTON),   \
                             ulFillColor, ulOutlineColor, ulTextColor, pFont,\
                             pcText, pucImage, ulImgLen, ucBorderWidth,      \
                             pfnOnClick, 0, psInst)

//*****************************************************************************
//
//! Declares an initialized variable containing a JPEG canvas data structure.
//!
//! \param sName is the name of the variable to be declared.
//! \param pParent is a pointer to the parent widget.
//! \param pNext is a pointer to the sibling widget.
//! \param pChild is a pointer to the first child widget.
//! \param pDisplay is a pointer to the display on which to draw the push
//! button.
//! \param lX is the X coordinate of the upper left corner of the JPEG widget.
//! \param lY is the Y coordinate of the upper left corner of the JPEG widget.
//! \param lWidth is the width of the JPEG widget.
//! \param lHeight is the height of the JPEG widget.
//! \param ulStyle is the style to be applied to the JPEG widget.
//! \param ulFillColor is the color used to fill in the JPEG widget.
//! \param ulOutlineColor is the color used to outline the JPEG widget.
//! \param ulTextColor is the color used to draw text on the JPEG widget.
//! \param pFont is a pointer to the font to be used to draw text on the push
//! button.
//! \param pcText is a pointer to the text to draw on this JPEG widget.
//! \param pucImage is a pointer to the compressed image to draw on this JPEG
//! widget.
//! \param ulImgLen is the length of the data pointed to by pucImage.
//! \param ucBorderWidth is the width of the border to paint if JW_STYLE_OUTLINE
//! is specified.
//! \param pfnOnScroll is a pointer to the function that is called when the
//! user drags a finger or stylus across the widget area.  The values reported
//! as parameters to the callback indicate the number of pixels of offset
//! from center that will be applied to the image next time it is redrawn.
//! \param psInst is a pointer to a read/write tJPEGInst structure that the
//! widget can use for workspace.
//!
//! This macro provides an initialized JPEG canvas widget data structure, which
//! can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).
//!
//! A JPEG canvas widget acts as an image display surface.  User input via the
//! touch screen controls the image positioning, allowing scrolling of a large
//! image within a smaller area of the display.  Image redraw can either be
//! carried out automatically whenever scrolling is required or can be delegated
//! to the application via the OnScroll callback which is called whenever the
//! user requests an image position change.
//!
//! \e ulStyle is the logical OR of the following:
//!
//! - \b #JW_STYLE_OUTLINE to indicate that the JPEG widget should be outlined.
//! - \b #JW_STYLE_TEXT to indicate that the JPEG widget should have text drawn
//!   on it (using \e pFont and \e pcText).
//! - \b #JW_STYLE_FILL to indicate that the JPEG widget should have its
//!   background filled with color (specified by \e ulFillColor).
//! - \b #JW_STYLE_SCROLL to indicate that the JPEG widget should be redrawn
//!   automatically each time the pointer is moved (touchscreen is dragged)
//!   rather than waiting for the gesture to end then redrawing once.  A client
//!   may chose to omit this style flag and call WidgetPaint() from within the
//!   pfnOnScroll callback at a rate deemed acceptable for the application.
//! - \b #JW_STYLE_LOCKED to indicate that the JPEG widget should ignore all
//!   user input and merely display the image.  If this flag is set,
//!   #JW_STYLE_SCROLL is ignored.
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define JPEGCanvas(sName, pParent, pNext, pChild, pDisplay, lX, lY, lWidth,  \
                   lHeight, ulStyle, ulFillColor, ulOutlineColor,            \
                   ulTextColor, pFont, pcText, pucImage, ulImgLen,           \
                   ucBorderWidth, pfnOnScroll, psInst)                       \
        tJPEGWidget sName =                                                  \
            JPEGWidgetStruct(pParent, pNext, pChild, pDisplay, lX, lY,       \
                             lWidth, lHeight, (ulStyle & ~(JW_STYLE_BUTTON | \
                             JW_STYLE_RELEASE_NOTIFY)), ulFillColor,         \
                             ulOutlineColor, ulTextColor, pFont, pcText,     \
                             pucImage, ulImgLen, ucBorderWidth, 0,           \
                             pfnOnScroll, psInst)

//*****************************************************************************
//
//! Sets the function to call when the JPEG image is scrolled.
//!
//! \param pWidget is a pointer to the JPEG widget to modify.
//! \param pfnOnScrll is a pointer to the function to call.
//!
//! This function sets the function to be called when this widget is scrolled
//! by dragging a finger or stylus over the image area (assuming that
//! \b #JW_STYLE_BUTTON is clear).
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetScrollCallbackSet(pWidget, pfnOnScrll)  \
        do                                                \
        {                                                 \
            tJPEGWidget *pW = pWidget;                    \
            pW->pfnOnScroll = pfnOnScrll;                 \
        }                                                 \
        while(0)

//*****************************************************************************
//
//! Sets the function to call when the button-style widget is pressed.
//!
//! \param pWidget is a pointer to the JPEG widget to modify.
//! \param pfnOnClik is a pointer to the function to call.
//!
//! This function sets the function to be called when this widget is
//! pressed (assuming \b #JW_STYLE_BUTTON is set).  The supplied function is
//! called when the button is pressed if \b #JW_STYLE_RELEASE_NOTIFY is clear
//! or when the button is released if this style flag is set.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetClickCallbackSet(pWidget, pfnOnClik)  \
        do                                              \
        {                                               \
            tJPEGWidget *pW = pWidget;                  \
            pW->pfnOnClick = pfnOnClik;                 \
        }                                               \
        while(0)

//*****************************************************************************
//
//! Sets the fill color of a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to be modified.
//! \param ulColor is the 24-bit RGB color to use to fill the JPEG widget.
//!
//! This function changes the color used to fill the JPEG widget on the
//! display.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetFillColorSet(pWidget, ulColor) \
        do                                       \
        {                                        \
            tJPEGWidget *pW = pWidget;           \
            pW->ulFillColor = ulColor;           \
        }                                        \
        while(0)


//*****************************************************************************
//
//! Disables background color fill for JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to modify.
//!
//! This function disables background color fill for a JPEG widget.  The display
//! is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetFillOff(pWidget)              \
        do                                      \
        {                                       \
            tJPEGWidget *pW = pWidget;          \
            pW->ulStyle &= ~(JW_STYLE_FILL);    \
        }                                       \
        while(0)

//*****************************************************************************
//
//! Enables background color fill for a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to modify.
//!
//! This function enables background color fill for JPEG widget.  The display
//! is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetFillOn(pWidget)            \
        do                                   \
        {                                    \
            tJPEGWidget *pW = pWidget;       \
            pW->ulStyle |= JW_STYLE_FILL;    \
        }                                    \
        while(0)

//*****************************************************************************
//
//! Sets the font for a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to modify.
//! \param pFnt is a pointer to the font to use to draw text on the push
//! button.
//!
//! This function changes the font used to draw text on the JPEG widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetFontSet(pWidget, pFnt)     \
        do                                   \
        {                                    \
            tJPEGWidget *pW = pWidget;       \
            const tFont *pF = pFnt;          \
            pW->pFont = pF;                  \
        }                                    \
        while(0)

//*****************************************************************************
//
//! Disables the image on a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to modify.
//!
//! This function disables the drawing of an image on a JPEG widget.
//! The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetImageOff(pWidget)          \
        do                                   \
        {                                    \
            tJPEGWidget *pW = pWidget;       \
            pW->ulStyle &= ~(JW_STYLE_IMG);  \
        }                                    \
        while(0)

//*****************************************************************************
//
//! Sets the outline color of a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to be modified.
//! \param ulColor is the 24-bit RGB color to use to outline the widget.
//!
//! This function changes the color used to outline the JPEG widget on the
//! display.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetOutlineColorSet(pWidget, ulColor) \
        do                                          \
        {                                           \
            tJPEGWidget *pW = pWidget;              \
            pW->ulOutlineColor = ulColor;           \
        }                                           \
        while(0)


//*****************************************************************************
//
//! Sets the outline width of a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to be modified.
//! \param ucWidth
//!
//! This function changes the width of the border around the JPEG widget.
//! The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetOutlineWidthSet(pWidget, ucWidth) \
        do                                          \
        {                                           \
            tJPEGWidget *pW = pWidget;              \
            pW->ucBorderWidth = ucWidth;            \
        }                                           \
        while(0)

//*****************************************************************************
//
//! Disables outlining of a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to modify.
//!
//! This function disables the outlining of a JPEG widget.  The display
//! is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetOutlineOff(pWidget)           \
        do                                      \
        {                                       \
            tJPEGWidget *pW = pWidget;          \
            pW->ulStyle &= ~(JW_STYLE_OUTLINE); \
        }                                       \
        while(0)

//*****************************************************************************
//
//! Enables outlining of a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to modify.
//!
//! This function enables the outlining of a JPEG widget.  The display
//! is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetOutlineOn(pWidget)         \
        do                                   \
        {                                    \
            tJPEGWidget *pW = pWidget;       \
            pW->ulStyle |= JW_STYLE_OUTLINE; \
        }                                    \
        while(0)

//*****************************************************************************
//
//! Sets the text color of a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to be modified.
//! \param ulColor is the 24-bit RGB color to use to draw text on the push
//! button.
//!
//! This function changes the color used to draw text on the JPEG widget on the
//! display.  The display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetTextColorSet(pWidget, ulColor) \
        do                                       \
        {                                        \
            tJPEGWidget *pW = pWidget;           \
            pW->ulTextColor = ulColor;           \
        }                                        \
        while(0)

//*****************************************************************************
//
//! Disables the text on a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to modify.
//!
//! This function disables the drawing of text on a JPEG widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetTextOff(pWidget)           \
        do                                   \
        {                                    \
            tJPEGWidget *pW = pWidget;       \
            pW->ulStyle &= ~(JW_STYLE_TEXT); \
        }                                    \
        while(0)

//*****************************************************************************
//
//! Enables the text on a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to modify.
//!
//! This function enables the drawing of text on a JPEG widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetTextOn(pWidget)            \
        do                                   \
        {                                    \
            tJPEGWidget *pW = pWidget;       \
            pW->ulStyle |= JW_STYLE_TEXT;    \
        }                                    \
        while(0)

//*****************************************************************************
//
//! Changes the text drawn on a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to be modified.
//! \param pcTxt is a pointer to the text to draw onto the JPEG widget.
//!
//! This function changes the text that is drawn onto the JPEG widget.  The
//! display is not updated until the next paint request.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetTextSet(pWidget, pcTxt)    \
        do                                   \
        {                                    \
            tJPEGWidget *pW = pWidget;       \
            const char *pcT = pcTxt;         \
            pW->pcText = pcT;                \
        }                                    \
        while(0)

//*****************************************************************************
//
//! Locks a JPEG widget making it ignore pointer input.
//!
//! \param pWidget is a pointer to the widget to modify.
//!
//! This function locks a JPEG widget and makes it ignore all pointer input.
//! When locked, a widget acts as a passive canvas.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetLock(pWidget)                 \
        do                                      \
        {                                       \
            tJPEGWidget *pW = (pWidget);        \
            pW->ulStyle |= JW_STYLE_LOCKED;     \
        }                                       \
        while(0)

//*****************************************************************************
//
//! Unlocks a JPEG widget making it pay attention to pointer input.
//!
//! \param pWidget is a pointer to the widget to modify.
//!
//! This function unlocks a JPEG widget.  When unlocked, a JPEG widget will
//! respond to pointer input by scrolling or making press callbacks depending
//! upon the style flags and the image it is currently displaying.
//!
//! \return None.
//
//*****************************************************************************
#define JPEGWidgetUnlock(pWidget)               \
        do                                      \
        {                                       \
            tJPEGWidget *pW = (pWidget);        \
            pW->ulStyle &= ~(JW_STYLE_LOCKED);  \
        }                                       \
        while(0)

//*****************************************************************************
//
// Prototypes for the JPEG widget APIs.
//
//*****************************************************************************
extern long JPEGWidgetMsgProc(tWidget *pWidget, unsigned long ulMsg,
                              unsigned long ulParam1, unsigned long ulParam2);
extern void JPEGWidgetInit(tJPEGWidget *pWidget,
                           const tDisplay *pDisplay, long lX, long lY,
                           long lWidth, long lHeight);
extern long JPEGWidgetImageSet(tWidget *pWidget, const unsigned char *pImg,
                               unsigned long ulImgLen);
extern long JPEGWidgetImageDecompress(tWidget *pWidget);
extern void JPEGWidgetImageDiscard(tWidget *pWidget);

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
#endif // __JPGWIDGET_H__

