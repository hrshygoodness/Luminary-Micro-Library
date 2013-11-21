//*****************************************************************************
//
// menu.c - Functions handling the user interface menu and controls for the
//          oscilloscope application.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "drivers/buttons.h"
#include "drivers/class-d.h"
#include "drivers/formike128x128x16.h"
#include "click.h"
#include "data-acq.h"
#include "menu.h"
#include "menu-controls.h"
#include "qs-scope.h"
#include "renderer.h"
#include "string.h"

//*****************************************************************************
//
// Colors used by the menu controls.
//
//*****************************************************************************
tOutlineTextColors g_psBtnColors =
{
    ClrBlack,
    ClrDarkGray,
    ClrDarkGray
};

tOutlineTextColors g_psFocusColors =
{
    ClrBlack,
    ClrWhite,
    ClrRed
};

#define MENU_BACKGROUND_COLOR   ClrDarkBlue
#define MENU_BORDER_COLOR       ClrWhite

//*****************************************************************************
//
// Labels defining the position and size of the menu window and the buttons
// it contains.
//
//*****************************************************************************
#define MENU_LR_BORDER          12
#define MENU_TOP_BORDER         4
#define MENU_BTN_LR_BORDER      3
#define MENU_BTN_SPACING        2
#define MENU_BTN_HEIGHT         16

#define MENU_LEFT               MENU_LR_BORDER
#define MENU_RIGHT              (g_sFormike128x128x16.usWidth - \
                                 (MENU_LR_BORDER + 1))
#define MENU_TOP                MENU_TOP_BORDER
#define MENU_BOTTOM(btns)       (MENU_TOP + (MENU_BTN_HEIGHT * (btns)) + \
                                 ((btns + 1) * MENU_BTN_SPACING) - 1)
#define MENU_BTN_X              (MENU_LEFT + MENU_LR_BORDER)
#define MENU_BTN_Y(index)       (MENU_TOP + (((index) + 1) *     \
                                             MENU_BTN_SPACING) + \
                                 ((index) * MENU_BTN_HEIGHT))
#define MENU_BTN_WIDTH          (MENU_RIGHT - (MENU_LEFT + 2 * MENU_LR_BORDER))

//*****************************************************************************
//
// This flag indicates whether or not the menu is currently displayed.  Since
// we overlay the menu on top of the waveform display area, this tells the
// main application to temporarily stop capturing data if continuous capture
// is in use.
//
//*****************************************************************************
tBoolean g_bMenuShown = false;

//*****************************************************************************
//
// This flag determines whether or not to play the key click sound.  The user
// can enable or disable this via a menu control.
//
//*****************************************************************************
tBoolean g_bClicksEnabled = true;

//*****************************************************************************
//
// Draw a single button in the menu using the supplied colors.
//
//*****************************************************************************
static void
MenuDrawGroupButton(tMenu *psMenu, unsigned char ucIndex,
    tOutlineTextColors *psColors)
{
    tRectangle rectBtn;

    //
    // Determine the bounds of this button.
    //
    rectBtn.sXMin = MENU_BTN_X;
    rectBtn.sXMax = MENU_BTN_X + MENU_BTN_WIDTH - 1;
    rectBtn.sYMin = MENU_BTN_Y(ucIndex);
    rectBtn.sYMax = MENU_BTN_Y(ucIndex) + MENU_BTN_HEIGHT - 1;

    DrawTextBox((char *)(psMenu->ppcGroups[ucIndex]->pcName), &rectBtn,
                psColors);
}

//*****************************************************************************
//
// Draw the whole menu onto the display.
//
//*****************************************************************************
static tBoolean
MenuDisplay(tMenu *psMenu)
{
    unsigned long ulLoop;
    tRectangle rectMenu;

    //
    // Erase the rectangle of the display that will contain the menu.
    //
    rectMenu.sXMin = MENU_LEFT;
    rectMenu.sXMax = MENU_RIGHT;
    rectMenu.sYMin = MENU_TOP;
    rectMenu.sYMax = MENU_BOTTOM(psMenu->ucNumGroups);
    GrContextForegroundSet(&g_sContext, MENU_BACKGROUND_COLOR);
    GrRectFill(&g_sContext, &rectMenu);
    GrContextForegroundSet(&g_sContext, MENU_BORDER_COLOR);
    GrRectDraw(&g_sContext, &rectMenu);

    //
    // Draw a rectangle around the edge of the menu area.
    //

    //
    // Draw each of the buttons corresponding to the groups.
    //
    for(ulLoop = 0; ulLoop < psMenu->ucNumGroups; ulLoop++)
    {
        MenuDrawGroupButton(psMenu, ulLoop,
         (ulLoop == psMenu->ucFocusGroup) ? &g_psFocusColors : &g_psBtnColors);
    }

    return(true);
}

//*****************************************************************************
//
// Draw a string of text centered within an outlined rectangle.
//
// \param pszText is a pointer to the zero-terminated ASCII string which will
// be displayed within the given rectangle.
// \param prectOutline points to the rectangle within which the test is to be
// displayed.
// \param psColors points to a structure defining the colors to be used for
// the background, outline and text.
//
// This function draws a text string centered within a given rectangle.  The
// rectangle is filled with a given color and outlined in another color prior
// to drawing the text.
//
// \return None.
//
//*****************************************************************************
void
DrawTextBox(const char *pszText, tRectangle *prectOutline,
            tOutlineTextColors *psColors)
{
    //
    // Set the clipping region to guard against text strings that are too
    // long for the supplied rectangle.
    //
    GrContextClipRegionSet(&g_sContext, prectOutline);

    //
    // Draw the background area
    //
    GrContextForegroundSet(&g_sContext, psColors->ulBackground);
    GrRectFill(&g_sContext, prectOutline);

    //
    // Draw the border
    //
    GrContextForegroundSet(&g_sContext, psColors->ulBorder);
    GrRectDraw(&g_sContext, prectOutline);

    //
    // Draw the text
    //
    GrContextForegroundSet(&g_sContext, psColors->ulText);
    GrStringDrawCentered(&g_sContext, (char *)pszText, strlen(pszText),
                         (prectOutline->sXMax + prectOutline->sXMin) / 2,
                         (prectOutline->sYMax + prectOutline->sYMin) / 2,
                         false);

    //
    // Remove our clipping area.
    //
    GrContextClipRegionSet(&g_sContext, &g_sRectDisplay);
}

//*****************************************************************************
//
// Draws left/right or up/down arrows within the supplied rectangle.
//
// \param prectOutline is a pointer to a rectangle defining the area of the
// control which is to be marked with arrows.
// \param bLeftRight is \b true of left and right arrow annotations are to
// be drawn or \b false for up and down arrows.
// \param ulColor defines the color of the arrows that will be drawn.
//
// This function is used to annotate a text box with two small arrows
// indicating keys that may be used to modify the boxes content.  If
// \e bLeftRight is \b true, a small arrow pointing left is drawn on the left
// side of the rectangle, indented by 2 pixels to avoid the outline rectangle.
// A similar, right pointing arrow is drawn on the right side of the
// rectangle.  If \e bLeftRight is \b false, small up and down pointing arrows
// are drawn on the left side of the rectangle.
//
// \note It is assumed that the caller has set a clip region which includes
// the required rectangle prior to calling this function.
//
// \return None.
//
//*****************************************************************************
void
DrawDirectionMarkers(tRectangle *prectOutline, tBoolean bLeftRight,
                     unsigned long ulColor)
{
    short sAverage;

    //
    // Set the color we will use for drawing.
    //
    GrContextForegroundSet(&g_sContext, ulColor);

    //
    // Draw the arrows using 3 pixels each.
    //
    if(bLeftRight)
    {
        //
        // Determine the Y coordinate half way between the top and bottom of
        // the rectangle.
        //
        sAverage = (prectOutline->sYMax + prectOutline->sYMin) / 2;

        //
        // Draw the left-pointing arrow.
        //
        GrPixelDraw(&g_sContext, prectOutline->sXMin + 2, sAverage);
        GrPixelDraw(&g_sContext, prectOutline->sXMin + 3, sAverage - 1);
        GrPixelDraw(&g_sContext, prectOutline->sXMin + 3, sAverage + 1);

        //
        // Draw the right-pointing arrow.
        //
        GrPixelDraw(&g_sContext, prectOutline->sXMax - 2, sAverage);
        GrPixelDraw(&g_sContext, prectOutline->sXMax - 3, sAverage - 1);
        GrPixelDraw(&g_sContext, prectOutline->sXMax - 3, sAverage + 1);
    }
    else
    {
        //
        // Draw the upward-pointing arrow
        //
        GrPixelDraw(&g_sContext, prectOutline->sXMin + 2,
                    prectOutline->sYMin + 3);
        GrPixelDraw(&g_sContext, prectOutline->sXMin + 3,
                    prectOutline->sYMin + 2);
        GrPixelDraw(&g_sContext, prectOutline->sXMin + 4,
                    prectOutline->sYMin + 3);

        //
        // Draw the downward-pointing arrow.
        //
        GrPixelDraw(&g_sContext, prectOutline->sXMin + 2,
                    prectOutline->sYMax - 3);
        GrPixelDraw(&g_sContext, prectOutline->sXMin + 3,
                    prectOutline->sYMax - 2);
        GrPixelDraw(&g_sContext, prectOutline->sXMin + 4,
                    prectOutline->sYMax - 3);
    }
}

//*****************************************************************************
//
// Draws centered text within a rectangle and annotates with left/right or
// up/down arrows.
//
// \param pszText is a pointer to the zero-terminated ASCII string which will
// be displayed within the given rectangle.
// \param prectOutline is a pointer to a rectangle defining the area of the
// control which is to be drawn.
// \param bLeftRight is \b true if left and right arrow annotations are to
// be drawn or \b false for up and down arrows.
// \param psColors points to a structure defining the colors to be used for
// the background, outline and text.
//
// This function is a combination of DrawTextBox() and DrawDirectionMarkers()
// and is used to display controls on the screen.  It fills the given rectangle
// with a background color, outlines it, renders the supplied text string in
// the center of the rectangle then annotates the rectangle with direction
// markers.
//
// If \e bLeftRight is \b true, a small arrow pointing left is drawn on the
// left side of the rectangle, indented by 2 pixels to avoid the outline
// rectangle.  A similar, right pointing arrow is drawn on the right side of
// the rectangle.  If \e bLeftRight is \b false, small up and down pointing
// arrows are drawn on the left side of the rectangle.
//
// \return None.
//
//*****************************************************************************
void
DrawTextBoxWithMarkers(const char *pszText, tRectangle *prectOutline,
                       tOutlineTextColors *psColors, tBoolean bLeftRight)
{
    DrawTextBox(pszText, prectOutline, psColors);
    DrawDirectionMarkers(prectOutline, bLeftRight, psColors->ulBorder);
}

//*****************************************************************************
//
// Performs all initialization for the menu and displays the default focus
// control on the screen.
//
// This function must be called during application initialization to display
// the first control on the screen and perform any menu-specific
// initialization.
//
//*****************************************************************************
void
MenuInit(void)
{
    tGroup *pFocusGroup;

    //
    // Perform any one-off initialization required by the menu controls.
    //
    MenuControlsInit();

    //
    // All we do here is activate the default control group.  This action will
    // cause the group to display its default control at the bottom of the
    // screen.
    //
    pFocusGroup = g_sMenu.ppcGroups[g_sMenu.ucFocusGroup];
    (pFocusGroup->pfnGroupEventProc)(pFocusGroup, MENU_EVENT_ACTIVATE);
}

//*****************************************************************************
//
// Performs all necessary menu and control processing based on new button
// states.
//
// \param ucButtons contains the current state of each of the front panel
// buttons.  A 1 in a bit position indicates that the corresponding button
// is released while a 0 indicates that the button is pressed.
// \param ucChanged contains bit flags showing which button states changed
// since the last call to this function.  A 1 indicates that the corresponding
// button state changed while a 0 indicates that it remains unchanged.
// \param ucRepeat contains bit flags indicating whether a key autorepeat
// event is being signalled for each key.  A 1 indicates that an autorepeat is
// being signalled while a 0 indicates that it is not.
//
// This is the top level function called when any key changes state.  This
// updates the relevant control or controls on the screen and processes
// the key appropriately for the control that currently has focus.
//
// \return Returns \b true if the menu was dismissed as a result of this
// call (in which case the caller should refresh the main waveform
// display area) or any control reported that a display update is required.
// Returns \b false if the menu is still being displayed or if it
// was not being displayed when the function was called and no control
// reported needing a display update.
//
//*****************************************************************************
tBoolean
MenuProcess(unsigned char ucButtons, unsigned char ucChanged,
            unsigned char ucRepeat)
{
    tGroup *pFocusGroup;
    tBoolean bRetcode;
    tBoolean bRedrawNeeded;

    //
    // Assume we won't be dismissing the menu until we find out otherwise.
    //
    bRedrawNeeded = false;

    //
    // Which group has focus?
    //
    pFocusGroup = g_sMenu.ppcGroups[g_sMenu.ucFocusGroup];

    //
    // Is the menu currently visible?
    //
    if(!g_bMenuShown)
    {
        //
        // The menu is not currently shown.  First check to see if we need to
        // show it and, if so, do this.  We look for a release of the select
        // button to trigger the display of the menu.  Note that we
        // deliberately ignore all other key notifications that may be present
        // at the same time.
        //
        if(BUTTON_RELEASED(SELECT_BUTTON, ucButtons, ucChanged))
        {
            //
            // Draw the menu.
            //
            g_bMenuShown = MenuDisplay(&g_sMenu);

            //
            // Get rid of any alert message that may currently be displayed.
            //
            RendererClearAlert();
        }
        else
        {
            //
            // We were not being asked to show the menu so we pass the
            // various events on to the group for it to decide what to do
            // with them.  For the group, we pass on any button press or auto-
            // repeat indication.  We ignore button releases in this case.
            //

            //
            // Left button press or repeat.
            //
            if(BUTTON_PRESSED(LEFT_BUTTON, ucButtons, ucChanged) ||
               BUTTON_REPEAT(LEFT_BUTTON, ucRepeat))
            {
                bRetcode = pFocusGroup->pfnGroupEventProc(pFocusGroup,
                                                          MENU_EVENT_LEFT);
                if(bRetcode)
                {
                    bRedrawNeeded = true;
                }
            }

            //
            // Right button press or repeat.
            //
            if(BUTTON_PRESSED(RIGHT_BUTTON, ucButtons, ucChanged) ||
               BUTTON_REPEAT(RIGHT_BUTTON, ucRepeat))
            {
                bRetcode = pFocusGroup->pfnGroupEventProc(pFocusGroup,
                                                          MENU_EVENT_RIGHT);
                if(bRetcode)
                {
                    bRedrawNeeded = true;
                }
            }

            //
            // Left button released.
            //
            if(BUTTON_RELEASED(LEFT_BUTTON, ucButtons, ucChanged))
            {
                bRetcode = pFocusGroup->pfnGroupEventProc(pFocusGroup,
                                                     MENU_EVENT_LEFT_RELEASE);
                if(bRetcode)
                {
                    bRedrawNeeded = true;
                }
            }

            //
            // Right button released.
            //
            if(BUTTON_RELEASED(RIGHT_BUTTON, ucButtons, ucChanged))
            {
                bRetcode = pFocusGroup->pfnGroupEventProc(pFocusGroup,
                                                     MENU_EVENT_RIGHT_RELEASE);
                if(bRetcode)
                {
                    bRedrawNeeded = true;
                }
            }

            //
            // Up button press.
            //
            if(BUTTON_PRESSED(UP_BUTTON, ucButtons, ucChanged) ||
               BUTTON_REPEAT(UP_BUTTON, ucRepeat))
            {
                bRetcode = pFocusGroup->pfnGroupEventProc(pFocusGroup,
                                                          MENU_EVENT_UP);
                if(bRetcode)
                {
                    bRedrawNeeded = true;
                }
            }

            //
            // Down button press.
            //
            if(BUTTON_PRESSED(DOWN_BUTTON, ucButtons, ucChanged) ||
               BUTTON_REPEAT(DOWN_BUTTON, ucRepeat))
            {
                bRetcode = pFocusGroup->pfnGroupEventProc(pFocusGroup,
                                                          MENU_EVENT_DOWN);
                if(bRetcode)
                {
                    bRedrawNeeded = true;
                }
            }
        }
    }
    else
    {
        //
        // The menu is already visible so we ignore left and right keys and
        // use up/down/select only to change the focus group.
        //

        //
        // If this button press will result in a group focus change we need to
        // redraw the current group in its non-focused colors, update the
        // focus to the new group then send an activate event to the group.
        //
        if(BUTTON_PRESSED(DOWN_BUTTON, ucButtons, ucChanged) ||
           BUTTON_REPEAT(DOWN_BUTTON, ucRepeat) ||
           BUTTON_PRESSED(UP_BUTTON, ucButtons, ucChanged) ||
           BUTTON_REPEAT(UP_BUTTON, ucRepeat))
        {
            //
            // Redraw the current focus button in its original colors
            //
            MenuDrawGroupButton(&g_sMenu, g_sMenu.ucFocusGroup,
                                &g_psBtnColors);

            //
            // Update the group with the focus
            //
            if(BUTTON_PRESSED(DOWN_BUTTON, ucButtons, ucChanged) ||
               BUTTON_REPEAT(DOWN_BUTTON, ucRepeat))
            {
                if(g_sMenu.ucFocusGroup < (g_sMenu.ucNumGroups - 1))
                {
                    g_sMenu.ucFocusGroup++;
                }
            }
            else
            {
                if(g_sMenu.ucFocusGroup > 0)
                {
                    g_sMenu.ucFocusGroup--;
                }
            }

            //
            // Redraw the new focus button with the focus colors.
            //
            MenuDrawGroupButton(&g_sMenu, g_sMenu.ucFocusGroup,
                                &g_psFocusColors);

            //
            // Tell the new group that it has been activated.
            //
            pFocusGroup = g_sMenu.ppcGroups[g_sMenu.ucFocusGroup];
            bRetcode = (pFocusGroup->pfnGroupEventProc)(pFocusGroup,
                                                        MENU_EVENT_ACTIVATE);
            if(bRetcode)
            {
                bRedrawNeeded = true;
            }
        }

        //
        // Now look for a release of the SELECT key.  This indicates that we
        // must dismiss the menu.
        //
        if(BUTTON_RELEASED(SELECT_BUTTON, ucButtons, ucChanged))
        {
            //
            // We did receive a release message for the SELECT button so
            // tell the caller that the menu has been dismissed.
            //
            g_bMenuShown = false;
            bRedrawNeeded = true;
        }
    }

    //
    // Play the button click sound if any button was just pressed.
    //
    if((~ucButtons & ucChanged) && g_bClicksEnabled)
    {
        ClassDPlayADPCM(g_pucADPCMClick, sizeof(g_pucADPCMClick));
    }

    return(bRedrawNeeded);
}

//*****************************************************************************
//
// Causes the current focus control to be refreshed.
//
// This function is called by the main command handler after any command is
// processed.  It allows the current focus control to be redrawn to reflect
// any necessary change of state as would occur, for example, if the USB host
// sent a command which had the side effect of changing the current focus
// control's state.
//
// \return None.
//
//*****************************************************************************
void
MenuRefresh(void)
{
    tGroup *pFocusGroup;

    //
    // Which group has focus?
    //
    pFocusGroup = g_sMenu.ppcGroups[g_sMenu.ucFocusGroup];

    //
    // Tell the group to refresh it's current control.
    //
    (pFocusGroup->pfnGroupEventProc)(pFocusGroup,
                                     MENU_EVENT_ACTIVATE);
}
