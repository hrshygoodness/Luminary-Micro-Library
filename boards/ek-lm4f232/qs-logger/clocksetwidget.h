//*****************************************************************************
//
// clocksetwidget.h - Prototypes for a widget to set a clock date/time.
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#ifndef __CLOCKSETWIDGET_H__
#define __CLOCKSETWIDGET_H__

//*****************************************************************************
//
//! \addtogroup clocksetwidget_api
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
//! A structure that represents a clock setting widget.
//
//*****************************************************************************
typedef struct _ClockSetWidget
{
    //
    //! The generic widget information.
    //
    tWidget sBase;

    //
    //! The font to use for drawing text on the widget.
    //
    const tFont *pFont;

    //
    //! The foreground color of the widget.  This is the color that will be
    //! used for drawing text and lines, and will also be used as the highlight
    //! color for a selected field on the widget.
    //
    unsigned long ulForegroundColor;

    //
    //! The background color of the widget.
    //
    unsigned long ulBackgroundColor;

    //
    //! An index for the date/time field that is highlighted
    //
    unsigned long ulHighlight;

    //
    //! A pointer to a time structure that is used for showing and editing
    //! the date and time.  The application should supply the storage for this
    //! structure, and this widget will modify it as the user changes the
    //! date/time.
    //
    tTime *psTime;

    //
    //! A pointer to the function to be called when the OK or cancel button is
    //! selected.  The OK button is used to indicate the user is done setting
    //! the time.  The CANCEL button is used to indicate that the user does
    //! not want to update the time.  The flag bOk is true if the OK button
    //! was selected, false otherwise.  The callback function can be used by
    //! the application to detect when the clock setting widget can be removed
    //! from the screen and whether or not to update the time.
    //
    void (*pfnOnOkClick)(tWidget *pWidget, tBoolean bOk);
} tClockSetWidget;

//*****************************************************************************
//
//! Declares an initialized strip chart widget data structure.
//!
//! \param pParent is a pointer to the parent widget.
//! \param pNext is a pointer to the sibling widget.
//! \param pChild is a pointer to the first child widget.
//! \param pDisplay is a pointer to the off-screen display on which to draw.
//! \param lX is the X coordinate of the upper left corner of the canvas.
//! \param lY is the Y coordinate of the upper left corner of the canvas.
//! \param lWidth is the width of the canvas.
//! \param lHeight is the height of the canvas.
//! \param pFont is the font used for rendering text on the widget.
//! \param ulForegroundColor is the foreground color for the widget.
//! \param ulBackgroundColor is the background color for the chart.
//! \param psTime is a pointer to storage for the time structure that the
//! widget will modify.
//! \param pfnOnOkClick is a callback function that is called when the user
//! selects the OK or CANCEL button on the widget.
//!
//! This macro provides an initialized clock setting widget data structure,
//! which can be used to construct the widget tree at compile time in global
//! variables (as opposed to run-time via function calls).  This must be
//! assigned to a variable, such as:
//!
//! \verbatim
//!     tClockSetWidget g_sClockSetter = ClockSetStruct(...);
//! \endverbatim
//!
//! Or, in an array of variables:
//!
//! \verbatim
//!     tClockSetWidget g_psClockSetters[] =
//!     {
//!         ClockSetStruct(...),
//!         ClockSetStruct(...)
//!     };
//! \endverbatim
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define ClockSetStruct(pParent, pNext, pChild, pDisplay,                      \
                       lX, lY, lWidth, lHeight,                               \
                       pFont, ulForegroundColor, ulBackgroundColor, psTime,   \
                       pfnOnOkClick)                                          \
        {                                                                     \
            {                                                                 \
                sizeof(tClockSetWidget),                                      \
                (tWidget *)(pParent),                                         \
                (tWidget *)(pNext),                                           \
                (tWidget *)(pChild),                                          \
                pDisplay,                                                     \
                {                                                             \
                    lX,                                                       \
                    lY,                                                       \
                    (lX) + (lWidth) - 1,                                      \
                    (lY) + (lHeight) - 1                                      \
                },                                                            \
                ClockSetMsgProc                                               \
            },                                                                \
            pFont, ulForegroundColor, ulBackgroundColor, 6, psTime,           \
            pfnOnOkClick                                                      \
        }

//*****************************************************************************
//
//! Declares an initialized variable containing a clock setting widget data
//! structure.
//!
//! \param sName is the name of the variable to be declared.
//! \param pParent is a pointer to the parent widget.
//! \param pNext is a pointer to the sibling widget.
//! \param pChild is a pointer to the first child widget.
//! \param pDisplay is a pointer to the off-screen display on which to draw.
//! \param lX is the X coordinate of the upper left corner of the canvas.
//! \param lY is the Y coordinate of the upper left corner of the canvas.
//! \param lWidth is the width of the canvas.
//! \param lHeight is the height of the canvas.
//! \param pFont is the font used for rendering text on the widget.
//! \param ulForegroundColor is the foreground color for the widget.
//! \param ulBackgroundColor is the background color for the chart.
//! \param psTime is a pointer to storage for the time structure that the
//! widget will modify.
//! \param pfnOnOkClick is a callback function that is called when the user
//! selects the OK or CANCEL button on the widget.
//!
//! This macro declares a variable containing an initialized clock setting
//! widget data structure, which can be used to construct the widget tree at
//! compile time in global variables (as opposed to run-time via function
//! calls).
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define ClockSet(sName, pParent, pNext, pChild, pDisplay,                     \
                 lX, lY, lWidth, lHeight,                                     \
                 pFont, ulForegroundColor, ulBackgroundColor, psTime,         \
                 pfnOnOkClick)                                                \
        tClockSetWidget sName =                                               \
            ClockSetStruct(pParent, pNext, pChild, pDisplay,                  \
                            lX, lY, lWidth,  lHeight,                         \
                            pFont, ulForegroundColor, ulBackgroundColor,      \
                            psTime, pfnOnOkClick)

//*****************************************************************************
//
//! Sets the pointer to the time structure for the clock set widget.
//!
//! \param pClockSetWidget is a pointer to the clock set widget to modify.
//! \param pTime is a pointer to the a time structure to use for clock
//! setting.
//!
//! This function sets the time structure used by the widget.
//!
//! \return None.
//
//*****************************************************************************
#define ClockSetTimePtrSet(pClockSetWidget, pTime)                            \
    do                                                                        \
    {                                                                         \
        (pClockSetWidget)->psTime = (pTime);                                  \
    } while(0)

//*****************************************************************************
//
//! Sets the callback function to be used when OK or CANCEL is selected.
//!
//! \param pClockSetWidget is a pointer to the clock set widget to modify.
//! \param pfnCB is a pointer to function to call when OK is selected by the
//! user.
//!
//! This function sets the OK click callback function used by the widget.
//!
//! \return None.
//
//*****************************************************************************
#define ClockSetCallbackSet(pClockSetWidget, pfnCB)                           \
    do                                                                        \
    {                                                                         \
        (pClockSetWidget)->pfnOnOkClick = (pfnCB);                            \
    } while(0)

//*****************************************************************************
//
// Prototypes for the clock set widget APIs.
//
//*****************************************************************************
extern void ClockSetInit(tClockSetWidget *pWidget, const tDisplay *pDisplay,
                         long lX, long lY, long lWidth, long lHeight,
                         tFont *pFont, unsigned long ulForegroundColor,
                         unsigned long ulBackgroundColor, tTime *psTime,
                         void (*pfnOnOkClick)(tWidget *pWidget, tBoolean bOk));
extern long ClockSetMsgProc(tWidget *pWidget, unsigned long ulMsg,
                            unsigned long ulParam1, unsigned long ulParam2);

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

#endif // __CLOCKSETWIDGET_H__
