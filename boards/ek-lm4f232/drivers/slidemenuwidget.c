//*****************************************************************************
//
// slidemenuwidget.c - A sliding menu drawing widget.
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

#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "utils/uartstdio.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "slidemenuwidget.h"

//*****************************************************************************
//
//! \addtogroup slidemenuwidget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This is a custom widget for drawing a menu system on the display.  The
// widget presents the menus using a "sliding" animation.  The menu items
// are shown in a vertical list, and as the user scrolls through the list
// of menu items, the menu slides up and down the display.  When a menu item
// is selected to descend in the menu tree, the widget slides the old menu
// off the to left while the new menu slides in from the right.  Likewise,
// going up in the menu tree, the higher level menu slides back onto the
// screen from the left.
//
// Additional structures are provided to implement a menu, and menu items.
// Each menu contains menu items, and each menu item can have a child menu.
// These structures can be used to build a menu tree.  The menu widget will
// show one menu at any given time, the menu that is displayed on the screen.
//
// In addition to child menus, any menu item can have instead a child widget.
// If this is used, then when the user selects a menu item, a new widget can
// be activated to perform some function.  When the function of the child
// widget completes, then the widget slides back off the screen (to the right)
// and the parent menu is displayed again.
//
// A given menu can have menu items that are individually selectable or
// multiple-selectable.  For individually selectable menu items, the item
// is selected by leaving the menu with the focus on the selected item.  For
// example navigating down to a submenu with choices A, B and C, and then
// navigating until the focus is on item B will cause item B to be selected.
// The menu will remember that item B was selected even when navigating away
// from that menu.
//
// If a menu is configured to be multiple-selectable, then each menu item has
// a check box that is checked by pressing the select button.  When the item
// is selected the box will show an X.  Any or all or none can be selected in
// this way.  When a menu is configured to be multi-selectable, the menu items
// cannot have any child menus or widgets.
//
// The menu widget provides some visual clues to the user about how to
// navigate the menu tree.  Whenever a menu item has a child menu or child
// widget, then a small right arrow is shown on the right side of the menu
// item that has the focus.  This tells the user to press the "right" button
// to descend to the next menu or widget.  When it is possible to go up a
// level in the menu tree (when showing a child menu), a small left arrow
// will be shown on the menu item with the focus.  This is an indication to the
// user that they should press the "left" button.
//
// This widget is meant to work with key/button presses.  It expects there
// to be up/down/left/right and select buttons.  The widget will need to be
// modified in order to work with a pointer input.
//
// In order to perform the sliding animation, the menu widget requires that
// it be provided with two off-screen displays.  The menu widget renders the
// two menus (the old and the new) into the two buffers, and then repeatedly
// paints both to the physical display while adjusting the coordinates as
// appropriate.  This will cause the menus to appear animated and move across
// the display.  When the menus are being animated, the menu widget is taking
// all the non-interrupt processor time in order to draw the buffers to the
// display.  This operation occurs in response to the widget processing of the
// key/button events and will occur in the thread that calls
// WidgetMessageQueueProcess().  The programmer should be aware of this
// processing burden when designing an application that uses the sliding
// menu widget.
//
//*****************************************************************************

//*****************************************************************************
//
// A graphics image of a small right arrow icon.
//
//*****************************************************************************
const unsigned char g_ucRtArrow[] =
{
    IMAGE_FMT_1BPP_UNCOMP,
    4, 0,
    8, 0,

    0x80,
    0xC0,
    0xE0,
    0xF0,
    0xE0,
    0xC0,
    0x80,
    0
};

//*****************************************************************************
//
// A graphics image of a small left arrow icon.
//
//*****************************************************************************
const unsigned char g_ucLtArrow[] =
{
    IMAGE_FMT_1BPP_UNCOMP,
    4, 0,
    8, 0,

    0x10,
    0x30,
    0x70,
    0xF0,
    0x70,
    0x30,
    0x10,
    0
};

//*****************************************************************************
//
// A graphics image of a small unchecked box icon.
//
//*****************************************************************************
const unsigned char g_ucUnchecked[] =
{
    IMAGE_FMT_1BPP_UNCOMP,
    7, 0,
    8, 0,

    0xFE,
    0x82,
    0x82,
    0x82,
    0x82,
    0x82,
    0xFE,
    0
};

//*****************************************************************************
//
// A graphics image of a small checked box icon.
//
//*****************************************************************************
const unsigned char g_ucChecked[] =
{
    IMAGE_FMT_1BPP_UNCOMP,
    7, 0,
    8, 0,

    0xFE,
    0xC6,
    0xAA,
    0x92,
    0xAA,
    0xC6,
    0xFE,
    0
};

//*****************************************************************************
//
//! Draws the current menu into a drawing context, off-screen buffer.
//!
//! \param pMenuWidget points at the SlideMenuWidget being processed.
//! \param pContext points to the context where all drawing should be done.
//! \param lOffsetY is the Y offset for drawing the menu.
//!
//! This function renders a menu (set of menu items), into a drawing context.
//! It assumes that the drawing context is an off-screen buffer, and that
//! the entire buffer belongs to this widget.  The vertical position of the
//! menu can be adjusted by using the parameter lOffsetY.  This value can be
//! positive or negative and can cause the menu to be rendered above or below
//! the normal position in the display.
//!
//! \return None.
//
//*****************************************************************************
void
SlideMenuDraw(tSlideMenuWidget *pMenuWidget, tContext *pContext, long lOffsetY)
{
    tSlideMenu *pMenu;
    unsigned long ulIdx;
    tRectangle sRect;

    //
    // Check the arguments
    //
    ASSERT(pMenuWidget);
    ASSERT(pContext);

    //
    // Set the foreground color for the rectangle fill to match what we want
    // as the menu background.
    //
    GrContextForegroundSet(pContext, pMenuWidget->ulColorBackground);
    GrRectFill(pContext, &pContext->sClipRegion);

    //
    // Get the current menu that is being displayed
    //
    pMenu = pMenuWidget->pSlideMenu;

    //
    // Set the foreground to the color we want for the menu item boundaries
    // and text color, text font.
    //
    GrContextForegroundSet(pContext, pMenuWidget->ulColorForeground);
    GrContextFontSet(pContext, pMenuWidget->pFont);

    //
    // Set the rectangle bounds for the first menu item.
    // The starting Y value is calculated based on which menu item is currently
    // centered.  Y coordinates are subtracted to find the Y start location
    // of the first menu item, which could even be off the display.
    //
    // Set the X coords of the menu item to the extents of the display
    //
    sRect.sXMin = 0;
    sRect.sXMax = pContext->sClipRegion.sXMax;

    //
    // Find the Y coordinate of the centered menu item
    //
    sRect.sYMin = (pContext->pDisplay->usHeight / 2) -
                  (pMenuWidget->ulMenuItemHeight / 2);

    //
    // Adjust to find Y coordinate of first menu item
    //
    sRect.sYMin -= pMenu->ulCenterIndex * pMenuWidget->ulMenuItemHeight;

    //
    // Now adjust for the offset that was passed in by caller.  This allows
    // for drawing menu items above or below the main display.
    //
    sRect.sYMin += lOffsetY;

    //
    // Find the ending Y coordinate of first menu item
    //
    sRect.sYMax = sRect.sYMin + pMenuWidget->ulMenuItemHeight - 1;

    //
    // Start the index at the first menu item.  It is possible that this
    // menu item is off the display.
    //
    ulIdx = 0;

    //
    // Loop through all menu items, drawing on the display.  Note that some
    // may not be on the screen, but they will be clipped.
    //
    while(ulIdx < pMenu->ulItems)
    {
        //
        // If this index is the one that is highlighted, then change the
        // background
        //
        if(ulIdx == pMenu->ulFocusIndex)
        {
            //
            // Set the foreground to the highlight color, and fill the
            // rectangle of the background of this menu item.
            //
            GrContextForegroundSet(pContext, pMenuWidget->ulColorHighlight);
            GrRectFill(pContext, &sRect);

            //
            // Set the new foreground to the normal foreground color, and
            // set the background to the highlight color.  This is so
            // remaining drawing operations will have the correct background
            // and foreground colors for this highlighted menu item cell.
            //
            GrContextForegroundSet(pContext, pMenuWidget->ulColorForeground);
            GrContextBackgroundSet(pContext, pMenuWidget->ulColorHighlight);

            //
            // If this menu has a parent, then draw a left arrow icon on the
            // focused menu item.
            //
            if(pMenu->pParent)
            {
                GrImageDraw(pContext, g_ucLtArrow, sRect.sXMin + 4,
                            sRect.sYMin +
                            (pMenuWidget->ulMenuItemHeight / 2) - 4);
            }

            //
            // If this menu has a child menu or child widget, then draw a
            // right arrow icon on the focused menu item.
            //
            if(pMenu->pSlideMenuItems[ulIdx].pChildMenu ||
               pMenu->pSlideMenuItems[ulIdx].pChildWidget)
            {
                GrImageDraw(pContext, g_ucRtArrow, sRect.sXMax - 8,
                            sRect.sYMin +
                            (pMenuWidget->ulMenuItemHeight / 2) - 4);
            }
        }

        //
        // Otherwise this is a normal, non-highlighted menu item cell,
        // so set the normal background color.
        //
        else
        {
            GrContextBackgroundSet(pContext, pMenuWidget->ulColorBackground);
        }

        //
        // If the current menu is multi-selectable, then draw a checkbox on
        // the menu item.  Draw a checked or unchecked box depending on whether
        // the item has been selected.
        //
        if(pMenu->bMultiSelectable)
        {
            if(pMenu->ulSelectedFlags & (1 << ulIdx))
            {
                GrImageDraw(pContext, g_ucChecked, sRect.sXMax - 12,
                            sRect.sYMin +
                            (pMenuWidget->ulMenuItemHeight / 2) - 4);
            }
            else
            {
                GrImageDraw(pContext, g_ucUnchecked, sRect.sXMax - 12,
                            sRect.sYMin +
                            (pMenuWidget->ulMenuItemHeight / 2) - 4);
            }

        }

        //
        // Draw the rectangle representing the menu item
        //
        GrRectDraw(pContext, &sRect);

        //
        // Draw the text for this menu item in the middle of the menu item
        // rectangle (cell).
        //
        GrStringDrawCentered(pContext, pMenu->pSlideMenuItems[ulIdx].pcText,
                             -1, pMenuWidget->sBase.pDisplay->usWidth / 2,
                             sRect.sYMin +
                             (pMenuWidget->ulMenuItemHeight / 2) - 1, 0);

        //
        // Advance to the next menu item, and update the menu item rectangle
        // bounds to the next position
        //
        ulIdx++;
        sRect.sYMin += pMenuWidget->ulMenuItemHeight;
        sRect.sYMax += pMenuWidget->ulMenuItemHeight;

        //
        // Note that this may attempt to render menu items that run off the
        // bottom of the drawing area, but these will just be clipped and a
        // little bit of processing time is wasted.
        //
    }
}

//*****************************************************************************
//
//! Paints a menu, menu items on a display.
//!
//! \param pWidget is a pointer to the slide menu widget to be drawn.
//!
//! This function draws the contents of a slide menu on the display.  This is
//! called in response to a \b WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
SlideMenuPaint(tWidget *pWidget)
{
    tSlideMenuWidget *pMenuWidget;
    tContext sContext;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // If this widget has a child widget, that means that the menu has
    // slid off the screen and the child widget is in control.  Therefore
    // there is nothing to paint here.  Just exit and the child widget will
    // be painted.
    //
    if(pWidget->pChild)
    {
        return;
    }

    //
    // Convert the generic widget pointer into a slide menu widget pointer,
    // and get a pointer to its context.
    //
    pMenuWidget = (tSlideMenuWidget *)pWidget;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, pMenuWidget->pDisplayA);

    //
    // Render the menu into the off-screen buffer, using normal vertical
    // position.
    //
    SlideMenuDraw(pMenuWidget, &sContext, 0);

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, pWidget->pDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(pWidget->sPosition));

    //
    // Now copy the rendered menu into the physical display. This will show
    // the menu on the display.
    //
    GrImageDraw(&sContext, pMenuWidget->pDisplayA->pvDisplayData,
                pWidget->sPosition.sXMin, pWidget->sPosition.sYMin);
}

//*****************************************************************************
//
//! Performs the sliding menu operation, in response to the "down" button.
//!
//! \param pWidget is a pointer to the slide menu widget to move down.
//!
//! This function will respond to the "down" key/button event.  The down
//! button is used to select the next menu item down the list, and the effect
//! is that the menu itself slides up, leaving the highlighted menu item
//! in the middle of the screen.
//!
//! This function repeatedly draws the menu onto the display until the sliding
//! animation is finished and will not return to the caller until then.  This
//! function is usually called from the thread context of
//! WidgetMessageQueueProcess().
//!
//! \return Returns a non-zero value if the menu was moved or was not moved
//! because it is already at the last position.  If a child widget is active
//! then this function does nothing and returns a 0.
//
//*****************************************************************************
static long
SlideMenuDown(tWidget *pWidget)
{
    tSlideMenuWidget *pMenuWidget;
    tSlideMenu *pMenu;
    tContext sContext;
    unsigned long ulMenuHeight;
    unsigned int uY;

    //
    // If this menu widget has a child widget, that means the child widget
    // is in control of the display, and there is nothing to do here.
    //
    if(pWidget->pChild)
    {
        return(0);
    }

    //
    // Get handy pointers to the menu widget, and the menu that is currently
    // displayed.
    //
    pMenuWidget = (tSlideMenuWidget *)pWidget;
    pMenu = pMenuWidget->pSlideMenu;

    //
    // If we are already at the end of the list of menu items, then there
    // is nothing else to do.
    //
    if(pMenu->ulFocusIndex >= (pMenu->ulItems - 1))
    {
        return(1);
    }

    //
    // Increment focus menu item.  This has the effect of selecting the next
    // menu item in the list.
    //
    pMenu->ulFocusIndex++;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, pMenuWidget->pDisplayA);

    //
    // Render the menu into the off-screen buffer.  This will be the same
    // menu appearance as before, except the highlighted item has changed
    // to the next menu item down.
    //
    SlideMenuDraw(pMenuWidget, &sContext, 0);

    //
    // Draw a continuation of this menu in the second offscreen buffer.
    // This is the part of the menu that would be drawn if the display were
    // twice as tall.  We are effectively creating a virtual display that is
    // twice as tall as the physical display.
    //
    GrContextInit(&sContext, pMenuWidget->pDisplayB);
    SlideMenuDraw(pMenuWidget, &sContext, -1 * 
                  (pMenuWidget->sBase.sPosition.sYMax - 
                  pMenuWidget->sBase.sPosition.sYMin));

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, pWidget->pDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(pWidget->sPosition));

    //
    // Get the height of the displayed part of the menu.
    //
    ulMenuHeight = pMenuWidget->pDisplayA->usHeight;

    //
    // Now copy the rendered menu into the physical display
    //
    // Iterate over the Y displacement of one menu item cell.  This loop
    // will repeatedly draw both off screen buffers to the physical display,
    // adjusting the position of each by one pixel each time it is drawn.  Each
    // time the offset is changed so that both buffers are drawn one higher
    // than the previous time.  This will have the effect of "sliding" the
    // entire menu up by the height of one menu item cell.
    // The speed of the animation is controlled entirely by the speed of the
    // processor and the speed of the interface to the physical display.
    //
    for(uY = 0; uY <= pMenuWidget->ulMenuItemHeight; uY++)
    {
        GrImageDraw(&sContext, pMenuWidget->pDisplayA->pvDisplayData,
                    pWidget->sPosition.sXMin, pWidget->sPosition.sYMin - uY);
        GrImageDraw(&sContext, pMenuWidget->pDisplayB->pvDisplayData,
                    pWidget->sPosition.sXMin,
                    pWidget->sPosition.sYMin + ulMenuHeight - uY);
    }

    //
    // Increment centered menu item.  This will now match the menu item with
    // the focus.  When the menu is repainted again, the newly selected
    // menu item will be centered and highlighted.
    //
    pMenu->ulCenterIndex = pMenu->ulFocusIndex;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, pMenuWidget->pDisplayA);

    //
    // Render the menu into the off-screen buffer.  This will be the same
    // menu appearance as before, except the highlighted item has changed
    // to the next menu item down.  Now when a repaint occurs the menu
    // will be redrawn with the newly highlighted menu item.
    //
    SlideMenuDraw(pMenuWidget, &sContext, 0);

    //
    // Return indication that we handled the key event.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs the sliding menu operation, in response to the "up" button.
//!
//! \param pWidget is a pointer to the slide menu widget to move up.
//!
//! This function will respond to the "up" key/button event.  The up
//! button is used to select the previous menu item down the list, and the
//! effect is that the menu itself slides down, leaving the highlighted menu
//! item in the middle of the screen.
//!
//! This function repeatedly draws the menu onto the display until the sliding
//! animation is finished and will not return to the caller until then.  This
//! function is usually called from the thread context of
//! WidgetMessageQueueProcess().
//!
//! \return Returns a non-zero value if the menu was moved or was not moved
//! because it is already at the first position.  If a child widget is active
//! then this function does nothing and returns a 0.
//
//*****************************************************************************
static long
SlideMenuUp(tWidget *pWidget)
{
    tSlideMenuWidget *pMenuWidget;
    tSlideMenu *pMenu;
    tContext sContext;
    unsigned long ulMenuHeight;
    unsigned int uY;

    //
    // If this menu widget has a child widget, that means the child widget
    // is in control of the display, and there is nothing to do here.
    //
    if(pWidget->pChild)
    {
        return(0);
    }

    //
    // Get handy pointers to the menu widget, and the menu that is currently
    // displayed.
    //
    pMenuWidget = (tSlideMenuWidget *)pWidget;
    pMenu = pMenuWidget->pSlideMenu;

    //
    // If we are already at the start of the list of menu items, then there
    // is nothing else to do.
    //
    if(pMenu->ulFocusIndex == 0)
    {
        return(1);
    }

    //
    // Decrement the focus menu item.  This has the effect of selecting the
    // previous menu item in the list.
    //
    pMenu->ulFocusIndex--;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, pMenuWidget->pDisplayA);

    //
    // Render the menu into the off-screen buffer.  This will be the same
    // menu appearance as before, except the highlighted item has changed
    // to the previous menu item up.
    //
    SlideMenuDraw(pMenuWidget, &sContext, 0);

    //
    // Draw a continuation of this menu in the second offscreen buffer.
    // This is the part of the menu that would be drawn above this menu if the
    // display were twice as tall.  We are effectively creating a virtual
    // display that is twice as tall as the physical display.
    //
    GrContextInit(&sContext, pMenuWidget->pDisplayB);
    SlideMenuDraw(pMenuWidget, &sContext,
                  (pMenuWidget->sBase.sPosition.sYMax - 
                  pMenuWidget->sBase.sPosition.sYMin));

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, pWidget->pDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(pWidget->sPosition));

    //
    // Get the height of the displayed part of the menu.
    //
    ulMenuHeight = pMenuWidget->pDisplayA->usHeight;

    //
    // Now copy the rendered menu into the physical display
    //
    // Iterate over the Y displacement of one menu item cell.  This loop
    // will repeatedly draw both off screen buffers to the physical display,
    // adjusting the position of each by one pixel each time it is drawn.  Each
    // time the offset is changed so that both buffers are drawn one lower
    // than the previous time.  This will have the effect of "sliding" the
    // entire menu down by the height of one menu item cell.
    // The speed of the animation is controlled entirely by the speed of the
    // processor and the speed of the interface to the physical display.
    //
    for(uY = 0; uY <= pMenuWidget->ulMenuItemHeight; uY++)
    {
        GrImageDraw(&sContext, pMenuWidget->pDisplayB->pvDisplayData,
                    pWidget->sPosition.sXMin,
                    pWidget->sPosition.sYMin + uY - ulMenuHeight);
        GrImageDraw(&sContext, pMenuWidget->pDisplayA->pvDisplayData,
                    pWidget->sPosition.sXMin, pWidget->sPosition.sYMin + uY);
    }

    //
    // Decrement the  centered menu item.  This will now match the menu item
    // with the focus.  When the menu is repainted again, the newly selected
    // menu item will be centered and highlighted.
    //
    pMenu->ulCenterIndex = pMenu->ulFocusIndex;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, pMenuWidget->pDisplayA);

    //
    // Render the menu into the off-screen buffer.  This will be the same
    // menu appearance as before, except the highlighted item has changed
    // to the next menu item up.  Now when a repaint occurs the menu
    // will be redrawn with the newly highlighted menu item.
    //
    SlideMenuDraw(pMenuWidget, &sContext, 0);

    //
    // Return indication that we handled the key event.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs the sliding menu operation, in response to the "right" button.
//!
//! \param pWidget is a pointer to the slide menu widget to move to the right.
//!
//! This function will respond to the "right" key/button event.  The right
//! button is used to select the next menu level below the current menu item,
//! or a widget that is activated by the menu item.  The effect is that the
//! menu itself slides off to the left, and the new menu or widget slides in
//! from the right.
//!
//! This function repeatedly draws the menu onto the display until the sliding
//! animation is finished and will not return to the caller until then.  This
//! function is usually called from the thread context of
//! WidgetMessageQueueProcess().
//!
//! \return Returns a non-zero value if the menu was moved or was not moved
//! because it is already at the last position.  If a child widget is active
//! then this function does nothing and returns a 0.
//
//*****************************************************************************
static long
SlideMenuRight(tWidget *pWidget)
{
    tSlideMenuWidget *pMenuWidget;
    tSlideMenu *pMenu;
    tSlideMenu *pChildMenu;
    tContext sContext;
    tWidget *pChildWidget;
    unsigned int uX;
    unsigned long ulMenuWidth;

    //
    // If this menu widget has a child widget, that means the child widget
    // is in control of the display, and there is nothing to do here.
    //
    if(pWidget->pChild)
    {
        return(0);
    }

    //
    // Get handy pointers to the menu widget, and the current menu, and the
    // child menu and widget if they exist.
    //
    pMenuWidget = (tSlideMenuWidget *)pWidget;
    pMenu = pMenuWidget->pSlideMenu;
    pChildMenu = pMenu->pSlideMenuItems[pMenu->ulFocusIndex].pChildMenu;
    pChildWidget = pMenu->pSlideMenuItems[pMenu->ulFocusIndex].pChildWidget;

    //
    // Initialize a context for the secondary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, pMenuWidget->pDisplayB);

    //
    // Render the current menu into off-screen buffer B.  This
    // will be the same menu appearance as is already being shown.
    //
    SlideMenuDraw(pMenuWidget, &sContext, 0);

    //
    // Now set up context for drawing into off-screen buffer A
    //
    GrContextInit(&sContext, pMenuWidget->pDisplayA);

    //
    // Process child menu of this menu item
    //
    if(pChildMenu)
    {
        //
        // Switch the active menu for this SlideMenuWidget to be the child
        // menu
        //
        pMenuWidget->pSlideMenu = pChildMenu;

        //
        // Draw the new (child) menu into off-screen buffer A
        //
        SlideMenuDraw(pMenuWidget, &sContext, 0);
    }

    //
    // Process child widget of this menu item.  This only happens if there
    // is no child menu.
    //
    else if(pChildWidget)
    {
        //
        // Call the widget activated callback function.  This will notify
        // the application that a child widget has been activated by the
        // menu system.
        //
        if(pMenuWidget->pfnActive)
        {
            pMenuWidget->pfnActive(pChildWidget,
                                   &pMenu->pSlideMenuItems[pMenu->ulFocusIndex],
                                   1);
        }

        //
        // Link the new child widget into this SlideMenuWidget so
        // it appears as a child to this widget.  Normally the menu widget
        // has no child widget.
        //
        pWidget->pChild = pChildWidget;
        pChildWidget->pParent = pWidget;

        //
        // Fill a rectangle with the new child widget background color.
        // This is done in off-screen buffer A.  When the menu slides off,
        // it will be replaced by a blank background that will then be
        // controlled by the new child widget.
        //
        GrContextForegroundSet(
            &sContext,
            pMenu->pSlideMenuItems[pMenu->ulFocusIndex].ulChildWidgetColor);
        GrRectFill(&sContext, &sContext.sClipRegion);

        //
        // Request a repaint for the child widget so it can draw itself once
        // the menu slide is done.
        //
        WidgetPaint(pChildWidget);
    }

    //
    // There is no child menu or child widget, so there is nothing to change
    // on the display.
    //
    else
    {
        return(1);
    }

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, pWidget->pDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(pWidget->sPosition));

    //
    // Get the width of the menu widget which is used in calculations below
    //
    ulMenuWidth = pMenuWidget->pDisplayA->usWidth;

    //
    // The following loop draws the two off-screen buffers onto the physical
    // display using a right-to-left-wipe.  This will provide an appearance
    // of sliding to the left.  The new child menu, or child widget background
    // will slide in from the right.  The "old" menu is being held in
    // off-screen buffer B and the new one is in buffer A.  So when we are
    // done, the correct image will be in buffer A.
    //
    for(uX = 0; uX <= ulMenuWidth; uX += 8)
    {
        GrImageDraw(&sContext, pMenuWidget->pDisplayB->pvDisplayData,
                    pWidget->sPosition.sXMin - uX, pWidget->sPosition.sYMin);
        GrImageDraw(&sContext, pMenuWidget->pDisplayA->pvDisplayData,
                    pWidget->sPosition.sXMin + ulMenuWidth - uX,
                    pWidget->sPosition.sYMin);
    }

    //
    // Return indication that we handled the key event.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs the sliding menu operation, in response to the "left" button.
//!
//! \param pWidget is a pointer to the slide menu widget to move to the left.
//!
//! This function will respond to the "left" key/button event.  The left
//! button is used to ascend to the next menu up in the menu tree.  The effect
//! is that the current menu, or active widget, slides off to the right, while
//! the parent menu slides in from the left.
//!
//! This function repeatedly draws the menu onto the display until the sliding
//! animation is finished and will not return to the caller until then.  This
//! function is usually called from the thread context of
//! WidgetMessageQueueProcess().
//!
//! \return Returns a non-zero value if the menu was moved or was not moved
//! because it is already at the last position.  If a child widget is active
//! then this function does nothing and returns a 0.
//
//*****************************************************************************
static long
SlideMenuLeft(tWidget *pWidget)
{
    tSlideMenuWidget *pMenuWidget;
    tSlideMenu *pMenu;
    tSlideMenu *pParentMenu;
    tContext sContext;
    unsigned int uX;
    unsigned long ulMenuWidth;

    //
    // Get handy pointers to the menu widget and active menu, and the parent
    // menu if there is one.
    //
    pMenuWidget = (tSlideMenuWidget *)pWidget;
    pMenu = pMenuWidget->pSlideMenu;
    pParentMenu = pMenu->pParent;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, pMenuWidget->pDisplayB);

    //
    // If this widget has a child, that means that the child widget is in
    // control, and we are requested to go back to the previous menu item.
    // Process the child widget.
    //
    if(pWidget->pChild)
    {
        //
        // Call the widget de-activated callback function.  This notifies the
        // application that the widget is being deactivated.
        //
        if(pMenuWidget->pfnActive)
        {
            pMenuWidget->pfnActive(pWidget->pChild,
                                   &pMenu->pSlideMenuItems[pMenu->ulFocusIndex],
                                   0);
        }

        //
        // Unlink the child widget from the slide menu widget.  The menu
        // widget will now no longer have a child widget.
        //
        pWidget->pChild->pParent = 0;
        pWidget->pChild = 0;

        //
        // Fill a rectangle with the child widget background color.  This will
        // erase everything else that is shown on the widget but leave the
        // background, which will make the change visually less jarring.
        // This is done in off-screen buffer B, which is the buffer that is
        // going to be slid off the screen.
        //
        GrContextForegroundSet(
            &sContext,
            pMenu->pSlideMenuItems[pMenu->ulFocusIndex].ulChildWidgetColor);
        GrRectFill(&sContext, &sContext.sClipRegion);
    }

    //
    // Otherwise there is not a child widget in control, so process the parent
    // menu, if there is one.
    //
    else if(pParentMenu)
    {
        //
        // Render the current menu into the off-screen buffer B.  This will be
        // the same menu appearance that is currently on the display.
        //
        SlideMenuDraw(pMenuWidget, &sContext, 0);

        //
        // Now switch the widget to the parent menu
        //
        pMenuWidget->pSlideMenu = pParentMenu;
    }

    //
    // Otherwise, we are already at the top level menu and there is nothing
    // else to do.
    //
    else
    {
        return(1);
    }

    //
    // Draw the new menu in the second offscreen buffer.  This is the menu
    // that will be on the display when the animation is over.
    //
    GrContextInit(&sContext, pMenuWidget->pDisplayA);
    SlideMenuDraw(pMenuWidget, &sContext, 0);

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, pWidget->pDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(pWidget->sPosition));

    //
    // Get the width of the menu widget.
    //
    ulMenuWidth = pMenuWidget->pDisplayA->usWidth;

    //
    // The following loop draws the two off-screen buffers onto the physical
    // display using a left-to-right.  This will provide an appearance
    // of sliding to the right.  The parent menu will slide in from the left.
    // The "old" child menu is being held in off-screen buffer B and the new
    // one is in buffer A.  So when we are done, the correct image will be in
    // buffer A.
    //
    for(uX = 0; uX <= ulMenuWidth; uX += 8)
    {
        GrImageDraw(&sContext, pMenuWidget->pDisplayB->pvDisplayData,
                    pWidget->sPosition.sXMin + uX, pWidget->sPosition.sYMin);
        GrImageDraw(&sContext, pMenuWidget->pDisplayA->pvDisplayData,
                    pWidget->sPosition.sXMin + uX - ulMenuWidth,
                    pWidget->sPosition.sYMin);
    }

    //
    // Return indication that we handled the key event.
    //
    return(1);
}

//*****************************************************************************
//
//! Handles menu selection, in response to the "select" button.
//!
//! \param pWidget is a pointer to the slide menu widget to use for a
//! select operation.
//!
//! This function will allow for checking or unchecking multi-selectable
//! menu items.  If the menu does not allow multiple selection, then it
//! treats it as a "right" button press.
//!
//! \return Returns a non-zero value if the key was handled.  Returns 0 if the
//! key was not handled.
//
//*****************************************************************************
static long
SlideMenuClick(tWidget *pWidget)
{
    tSlideMenuWidget *pMenuWidget;
    tSlideMenu *pMenu;

    //
    // If a child widget is in control then there is nothing to do.
    //
    if(pWidget->pChild)
    {
        return(0);
    }

    //
    // Get handy pointers to the menu widget and current menu.
    //
    pMenuWidget = (tSlideMenuWidget *)pWidget;
    pMenu = pMenuWidget->pSlideMenu;

    //
    // Check to see if this menu allows multiple selection.
    //
    if(pMenu->bMultiSelectable)
    {
        //
        // Toggle the selection status of the currently highlighted menu
        // item, and then repaint it.
        //
        pMenu->ulSelectedFlags ^= 1 << pMenu->ulFocusIndex;
        SlideMenuPaint(pWidget);

        //
        // We are done so return indication that we handled the key event.
        //
        return(1);
    }

    //
    // Otherwise, treat the select button the same as a right button.
    //
    return(SlideMenuRight(pWidget));
}

//*****************************************************************************
//
//! Process key/button event to decide how to move the sliding menu.
//!
//! \param pWidget is a pointer to the slide menu widget to process.
//! \param ulMsg is the message containing the key event.
//!
//! This function is used to specifically handle key events destined for the
//! slide menu widget.  It decides which menu movement function should be
//! called for each key event.
//!
//! \return Returns an indication if the key was handled.  Non-zero if the
//! key event was handled or else 0.
//
//*****************************************************************************
static long
SlideMenuMove(tWidget *pWidget, unsigned long ulMsg)
{
    //
    // Process the key event.
    //
    switch(ulMsg)
    {
        //
        // User presses select button.
        //
        case WIDGET_MSG_KEY_SELECT:
        {
            return(SlideMenuClick(pWidget));
        }

        //
        // User presses up button.
        //
        case WIDGET_MSG_KEY_UP:
        {
            return(SlideMenuUp(pWidget));
        }

        //
        // User presses down button.
        //
        case WIDGET_MSG_KEY_DOWN:
        {
            return(SlideMenuDown(pWidget));
        }

        //
        // User presses left button.
        //
        case WIDGET_MSG_KEY_LEFT:
        {
            return(SlideMenuLeft(pWidget));
        }

        //
        // User presses right button.
        //
        case WIDGET_MSG_KEY_RIGHT:
        {
            return(SlideMenuRight(pWidget));
        }

        //
        // This is an unexpected event.  Return an indication that the event
        // was not handled.
        //
        default:
        {
            return(0);
        }
    }
}

//*****************************************************************************
//
//! Handles messages for a slide menu widget.
//!
//! \param pWidget is a pointer to the slide menu widget.
//! \param ulMsg is the message.
//! \param ulParam1 is the first parameter to the message.
//! \param ulParam2 is the second parameter to the message.
//!
//! This function receives messages intended for this slide menu widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
long
SlideMenuMsgProc(tWidget *pWidget, unsigned long ulMsg, unsigned long ulParam1,
              unsigned long ulParam2)
{
    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Determine which message is being sent.
    //
    switch(ulMsg)
    {
        //
        // The widget paint request has been sent.
        //
        case WIDGET_MSG_PAINT:
        {
            //
            // Handle the widget paint request.
            //
            SlideMenuPaint(pWidget);

            //
            // Return one to indicate that the message was successfully
            // processed.
            //
            return(1);
        }

        //
        // A key event has been received.  By convention, this widget will
        // process the key events if ulParam1 is set to this widget.
        // Otherwise a different widget has the "focus" for key events.
        //
        case WIDGET_MSG_KEY_SELECT:
        case WIDGET_MSG_KEY_UP:
        case WIDGET_MSG_KEY_DOWN:
        case WIDGET_MSG_KEY_LEFT:
        case WIDGET_MSG_KEY_RIGHT:
        {
            //
            // If this key event is for us, then process the event.
            //
            if((tWidget *)ulParam1 == pWidget)
            {
                return(SlideMenuMove(pWidget, ulMsg));
            }
        }

        //
        // An unknown request has been sent.  This widget does not handle
        // pointer events, so they get dumped here if they occur.
        //
        default:
        {
            //
            // Let the default message handler process this message.
            //
            return(WidgetDefaultMsgProc(pWidget, ulMsg, ulParam1, ulParam2));
        }
    }
}

//*****************************************************************************
//
//! Initializes a slide menu widget.
//!
//! \param pWidget is a pointer to the slide menu widget to initialize.
//! \param pDisplay is a pointer to the display on which to draw the menu.
//! \param lX is the X coordinate of the upper left corner of the canvas.
//! \param lY is the Y coordinate of the upper left corner of the canvas.
//! \param lWidth is the width of the canvas.
//! \param lHeight is the height of the canvas.
//! \param pDisplayOffA is one of two off-screen displays used for rendering.
//! \param pDisplayOffB is one of two off-screen displays used for rendering.
//! \param ulItemHeight is the height of a menu item
//! \param ulForeground is the foreground color used for menu item boundaries
//! and text.
//! \param ulBackground is the background color of a menu item.
//! \param ulHighlight is the color of a highlighted menu item.
//! \param pFont is a pointer to the font that should be used for text.
//! \param pMenu is the initial menu to display
//!
//! This function initializes the caller provided slide menu widget.
//!
//! \return None.
//
//*****************************************************************************
void
SlideMenuInit(tSlideMenuWidget *pWidget, const tDisplay *pDisplay,
              long lX, long lY, long lWidth, long lHeight,
              tDisplay *pDisplayOffA, tDisplay *pDisplayOffB,
              unsigned long ulItemHeight, unsigned long ulForeground,
              unsigned long ulBackground, unsigned long ulHighlight,
              tFont *pFont, tSlideMenu *pMenu)
{
    unsigned long ulIdx;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);
    ASSERT(pDisplay);
    ASSERT(pDisplayOffA);
    ASSERT(pDisplayOffB);
    ASSERT(pFont);
    ASSERT(pMenu);

    //
    // Clear out the widget structure.
    //
    for(ulIdx = 0; ulIdx < sizeof(tSlideMenuWidget); ulIdx += 4)
    {
        ((unsigned long *)pWidget)[ulIdx / 4] = 0;
    }

    //
    // Set the size of the widget structure.
    //
    pWidget->sBase.lSize = sizeof(tSlideMenuWidget);

    //
    // Mark this widget as fully disconnected.
    //
    pWidget->sBase.pParent = 0;
    pWidget->sBase.pNext = 0;
    pWidget->sBase.pChild = 0;

    //
    // Save the display pointer.
    //
    pWidget->sBase.pDisplay = pDisplay;

    //
    // Set the extents of the display area.
    //
    pWidget->sBase.sPosition.sXMin = lX;
    pWidget->sBase.sPosition.sYMin = lY;
    pWidget->sBase.sPosition.sXMax = lX + lWidth - 1;
    pWidget->sBase.sPosition.sYMax = lY + lHeight - 1;

    //
    // Initialize the widget fields
    //
    pWidget->pDisplayA = pDisplayOffA;
    pWidget->pDisplayB = pDisplayOffB;
    pWidget->ulMenuItemHeight = ulItemHeight;
    pWidget->ulColorForeground = ulForeground;
    pWidget->ulColorBackground = ulBackground;
    pWidget->ulColorHighlight = ulHighlight;
    pWidget->pFont = pFont;
    pWidget->pSlideMenu = pMenu;

    //
    // Use the slide menu message handler to process messages to this widget.
    //
    pWidget->sBase.pfnMsgProc = SlideMenuMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
