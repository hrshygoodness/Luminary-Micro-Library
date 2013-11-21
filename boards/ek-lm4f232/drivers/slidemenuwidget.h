//*****************************************************************************
//
// slidemenuwidget.h - Prototypes for a sliding menu widget.
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

#ifndef __SLIDEMENUWIDGET_H__
#define __SLIDEMENUWIDGET_H__

//*****************************************************************************
//
//! \addtogroup slidemenuwidget_api
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
//! The structure that describes a menu item in the menu tree.
//
//*****************************************************************************
typedef struct _SlideMenuItem
{
    //
    //! A pointer to text to be rendered within the node.
    //
    char *pcText;

    //
    //! A child menu that is activated by this menu item, if any.  Can be NULL.
    //
    struct _SlideMenu *pChildMenu;

    //
    //! A child widget that is activated by this menu item, if any.  Can be
    //! NULL.  If both child menu and child widget are specified, the child
    //! menu will be used.
    //
    tWidget *pChildWidget;

    //
    //! A color that is used when a child widget is activated.  This is the
    //! color that is used as a background when the menu slides off to make
    //! the screen available for the child widget.  By choosing this color to
    //! match the background that is used in the child widget, the sliding
    //! animation and widget painting appears smoother.
    //
    unsigned long ulChildWidgetColor;
}
tSlideMenuItem;

//*****************************************************************************
//
//! The structure that describes a menu.
//
//*****************************************************************************
typedef struct _SlideMenu
{
    //
    //! The parent menu of this menu.
    //
    struct _SlideMenu *pParent;

    //
    //! The total number of items in this menu.
    //
    unsigned long ulItems;

    //
    //! A pointer to the array of menu item structures.
    //
    tSlideMenuItem *pSlideMenuItems;

    //
    //! The menu item index of the item shown on the center of the screen.
    //! Normally this is the same as the menu item that has the focus, but
    //! can be different during the time when the menu is "sliding".  When this
    //! is 0, the first menu item is shown in the center of the screen and the
    //! successive items are shown below it.  When non-zero, then this menu
    //! item is shown in the center, with preceding menu items shown above
    //! and successive items shown below.
    //
    unsigned long ulCenterIndex;

    //
    //! The menu item index that has the focus.
    //
    unsigned long ulFocusIndex;

    //
    //! A flag to indicate if more than one menu item is selectable.
    //
    tBoolean bMultiSelectable;

    //
    //! A set of bit flags to indicate which menu items are selected.
    //
    unsigned long ulSelectedFlags;
} tSlideMenu;

//*****************************************************************************
//
//! The structure that describes a slide menu widget.
//
//*****************************************************************************
typedef struct
{
    //
    //! The generic widget information.
    //
    tWidget sBase;

    //
    //! A pointer to an off-screen display that is used for rendering the
    //! menus prior to showing on the widget's area of the screen.  There
    //! are two off-screen displays.  Each should be the size of the widget
    //! area.  The palette should include any colors that are used by this
    //! widget.
    //
    tDisplay *pDisplayA;

    //
    //! A pointer to a second off-screen display.
    //
    tDisplay *pDisplayB;

    //
    //! The height, in pixels, of a single menu item (a cell).
    //
    unsigned long ulMenuItemHeight;

    //
    //! The color used for drawing menu item cell boundaries and text.
    //
    unsigned long ulColorForeground;

    //
    //! The background color of menu item cells.
    //
    unsigned long ulColorBackground;

    //
    //! The color of a highlighted menu item.
    //
    unsigned long ulColorHighlight;

    //
    //! The font to use for menu text.
    //
    const tFont *pFont;

    //
    //! The current menu to display.
    //
    tSlideMenu *pSlideMenu;

    //
    // A function to call when a child widget becomes active or inactive
    //
    void (*pfnActive)(tWidget *pWidget, tSlideMenuItem *pMenuItem,
                      tBoolean bActivated);
}
tSlideMenuWidget;

//*****************************************************************************
//
//! Declares an initialized slide menu widget data structure.
//!
//! \param pParent is a pointer to the parent widget.
//! \param pNext is a pointer to the sibling widget.
//! \param pChild is a pointer to the first child widget.
//! \param pDisplay is a pointer to the off-screen display on which to draw.
//! \param lX is the X coordinate of the upper left corner of the canvas.
//! \param lY is the Y coordinate of the upper left corner of the canvas.
//! \param lWidth is the width of the canvas.
//! \param lHeight is the height of the canvas.
//! \param pDisplayA is one of two off-screen displays for rendering menus.
//! \param pDisplayB is one of two off-screen displays for rendering menus.
//! \param ulMenuItemHeight is the height of a menu item.
//! \param ulForeground is the foreground color for menu item boundaries and
//! text.
//! \param ulBackground is the background color of the menu items.
//! \param ulHighlight is the color of a highlighted menu item.
//! \param pFont is the font to use for the menu text.
//! \param pMenu is the initial menu to display.
//! \param pfnWidgetActive is a pointer to a function that will be called when
//! a child widget is activated or deactivated.
//!
//! This macro provides an initialized slide menu widget data structure, which
//! can be used to construct the widget tree at compile time in global variables
//! (as opposed to run-time via function calls).  This must be assigned to a
//! variable, such as:
//!
//! \verbatim
//!     tSlideMenuWidget g_sSlideMenu = SlideMenuStruct(...);
//! \endverbatim
//!
//! Or, in an array of variables:
//!
//! \verbatim
//!     tSlideMenuWidget g_psSlideMenu[] =
//!     {
//!         SlideMenuStruct(...),
//!         SlideMenuStruct(...)
//!     };
//! \endverbatim
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define SlideMenuStruct(pParent, pNext, pChild, pDisplay,                     \
                        lX, lY, lWidth, lHeight,                              \
                        pDisplayA, pDisplayB,                                 \
                        ulMenuItemHeight, ulForeground, ulBackground,         \
                        ulHighlight, pFont, pMenu, pfnWidgetActive)           \
        {                                                                     \
            {                                                                 \
                sizeof(tSlideMenuWidget),                                     \
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
                SlideMenuMsgProc                                              \
            },                                                                \
            pDisplayA,                                                        \
            pDisplayB,                                                        \
            ulMenuItemHeight,                                                 \
            ulForeground,                                                     \
            ulBackground,                                                     \
            ulHighlight,                                                      \
            pFont,                                                            \
            pMenu,                                                            \
            pfnWidgetActive                                                   \
        }

//*****************************************************************************
//
//! Declares an initialized variable containing a slide menu widget data
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
//! \param pDisplayA is one of two off-screen displays for rendering menus.
//! \param pDisplayB is one of two off-screen displays for rendering menus.
//! \param ulMenuItemHeight is the height of a menu item.
//! \param ulForeground is the foreground color for menu item boundaries and
//! text.
//! \param ulBackground is the background color of the menu items.
//! \param ulHighlight is the color of a highlighted menu item.
//! \param pFont is the font to use for the menu text.
//! \param pMenu is the initial menu to display.
//! \param pfnWidgetActive is a pointer to a function that will be called when
//! a child widget is activated or deactivated.
//!
//! This macro declares a variable containing an initialized slide menu widget
//! data structure, which can be used to construct the widget tree at compile
//! time in global variables (as opposed to run-time via function calls).
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define SlideMenu(sName, pParent, pNext, pChild, pDisplay,                    \
                lX, lY, lWidth, lHeight,                                      \
                pDisplayA, pDisplayB,                                         \
                ulMenuItemHeight, ulForeground, ulBackground,                 \
                ulHighlight, pFont, pMenu, pfnWidgetActive)                   \
        tSlideMenuWidget sName =                                              \
            SlideMenuStruct(pParent, pNext, pChild, pDisplay,                 \
                            lX, lY, lWidth,  lHeight,                         \
                            pDisplayA, pDisplayB,                             \
                            ulMenuItemHeight, ulForeground, ulBackground,     \
                            ulHighlight, pFont, pMenu, pfnWidgetActive)

//*****************************************************************************
//
//! Sets the active menu of the slide menu widget.
//!
//! \param pSlideMenuWidget is a pointer to the slide menu widget to modify.
//! \param pMenu is the new slide menu to make active.
//!
//! This function sets the active menu for the widget.
//!
//! \return None.
//
//*****************************************************************************
#define SlideMenuMenuSet(pSlideMenuWidget, pMenu)                             \
    do                                                                        \
    {                                                                         \
        (pSlideMenuWidget)->pSlideMenu = (pMenu);                             \
    } while(0)

//*****************************************************************************
//
//! Sets the active callback function for a slide menu widget.
//!
//! \param pSlideMenuWidget is a pointer to the slide menu widget to modify.
//! \param pfnActivated is a function pointer to the function that should
//! be called when the menu system activates or deactivates a child widget.
//!
//! This function sets the child widget active callback function.
//!
//! \return None.
//
//*****************************************************************************
#define SlideMenuActiveCallbackSet(pSlideMenuWidget, pfnActivated)            \
    do                                                                        \
    {                                                                         \
        (pSlideMenuWidget)->pfnActive = (pfnActivated);                       \
    } while(0)

//*****************************************************************************
//
//! Gets the index of the menu item that has the focus.
//!
//! \param pSlideMenu is a pointer to the menu to query for item index.
//!
//! This function returns the index of the menu item that has the focus for
//! the specified menu.
//!
//! \return Index of the menu item that has the focus.
//
//*****************************************************************************
#define SlideMenuFocusItemGet(pSlideMenu)                                     \
        ((pSlideMenu)->ulFocusIndex)

//*****************************************************************************
//
//! Gets the selected items mask for a menu.
//!
//! \param pSlideMenu is a pointer to the menu to query for selected items.
//!
//! This function returns a value that is a bit mask of any menu items that
//! are selected in the current menu.  This is meant to work when a menu is
//! configured to be selectable.  Then multiple menu items can be selected and
//! the selection mask indicates which are selected.
//!
//! \return Selection bit mask of selected menu items.
//
//*****************************************************************************
#define SlideMenuSelectedGet(pSlideMenu)                                      \
    ((pSlideMenu)->ulSelectedFlags)

//*****************************************************************************
//
//! Sets the focus item index for a menu.
//!
//! \param pSlideMenu is a pointer to the menu to set the focus item.
//! \param ulFocus is the index of the menu item that should have the focus.
//!
//! This function is used to specify which menu item should have the focus.
//!
//! \return None.
//
//*****************************************************************************
#define SlideMenuFocusItemSet(pSlideMenu, ulFocus)                            \
    do                                                                        \
    {                                                                         \
        (pSlideMenu)->ulFocusIndex = (ulFocus);                               \
        (pSlideMenu)->ulCenterIndex = (ulFocus);                              \
    } while(0)

//*****************************************************************************
//
//! Sets the selected items bit mask for a menu.
//!
//! \param pSlideMenu is a pointer to the menu to set the selection mask.
//! \param ulSelected is a bit mask indicating which menu items should be
//! marked as selected.
//!
//! This function is used to specify which menu items are pre-selected.
//!
//! \return None.
//
//*****************************************************************************
#define SlideMenuSelectedSet(pSlideMenu, ulSelected)                          \
    do                                                                        \
    {                                                                         \
        (pSlideMenu)->ulSelectedFlags = (ulSelected);                         \
    } while(0)

//*****************************************************************************
//
// Prototypes for the slide menu widget APIs.
//
//*****************************************************************************
extern long SlideMenuMsgProc(tWidget *pWidget, unsigned long ulMsg,
                             unsigned long ulParam1, unsigned long ulParam2);
void SlideMenuDraw(tSlideMenuWidget *pMenuWidget, tContext *pContext,
                   long lOffsetY);
extern void SlideMenuInit(tSlideMenuWidget *pWidget, const tDisplay *pDisplay,
                          long lX, long lY, long lWidth, long lHeight,
                          tDisplay *pDisplayOffA, tDisplay *pDisplayOffB,
                          unsigned long ulItemHeight,
                          unsigned long ulForeground,
                          unsigned long ulBackground,
                          unsigned long ulHighlight,
                          tFont *pFont, tSlideMenu *pMenu);

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

#endif // __SLIDEMENUWIDGET_H__
