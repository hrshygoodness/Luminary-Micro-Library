//*****************************************************************************
//
// menu.h - Functions handling the user interface menu and controls for the
//          oscilloscope application.
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
// This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#ifndef __MENU_H__
#define __MENU_H__

//*****************************************************************************
//
//! This enum defines the events that can be sent to a control from the
//! menu processing functions.
//
//*****************************************************************************
typedef enum
{
    MENU_EVENT_RIGHT,
    MENU_EVENT_LEFT,
    MENU_EVENT_UP,
    MENU_EVENT_DOWN,
    MENU_EVENT_RIGHT_RELEASE,
    MENU_EVENT_LEFT_RELEASE,
    MENU_EVENT_SELECT,
    MENU_EVENT_ACTIVATE
}
tEvent;

//*****************************************************************************
//
//! This type defines a very simple container for the three colors required
//! when drawing text into an outlined box.
//
//*****************************************************************************
typedef struct
{
    unsigned long ulBackground;
    unsigned long ulText;
    unsigned long ulBorder;
}
tOutlineTextColors;

//*****************************************************************************
//
//! This type defines a very simple control in terms of a label and an event
//! handler function.
//
//*****************************************************************************
typedef struct _tControl
{
    //
    //! A pointer to a string containing a name for this control.
    //
    const char *pcName;

    //
    //! The event handling procedure for this control.
    //
    tBoolean (*pfnControlEventProc)(struct _tControl *, tEvent);

    //
    //! A pointer that may be used to pass some control-specific data to
    //! the event handler function.
    //
    void *pvUserData;
}
tControl;

//*****************************************************************************
//
//! This structure defines a group of controls
//
//*****************************************************************************
typedef struct _tGroup
{
    //
    //! The number of controls in this group.
    //
    unsigned char ucNumControls;

    //
    //! The index of the control which currently has input focus.
    //
    unsigned char ucFocusControl;

    //
    //! A pointer to a string containing a name for this group.
    //
    const char *pcName;

    //
    //! A pointer to an array of ucNumControls controls comprising the group.
    //
    tControl **ppControls;

    //
    //! A pointer to the event handler for this group
    //
    tBoolean (*pfnGroupEventProc)(struct _tGroup *, tEvent);
}
tGroup;

//*****************************************************************************
//
//! The top level menu is defined in the following structure.  This contains
//! pointers to each of the control groups managed by the menu.  A control
//! group will be represented by a single button on the menu.
//
//*****************************************************************************
typedef struct _tMenu
{
    //
    // The number of groups (buttons) offered in this menu.
    //
    unsigned char ucNumGroups;

    //
    // The index of the button/group which currently has the input focus.
    //
    unsigned char ucFocusGroup;

    //
    // A pointer to an array of ucNumGroups groups comprising this menu.
    //
    tGroup **ppcGroups;
}
tMenu;

//*****************************************************************************
//
// Flag indicating whether the menu is currently displayed or not.
//
//*****************************************************************************
extern tBoolean g_bMenuShown;

//*****************************************************************************
//
// Flag indicating whether button clicks are enabled or not.
//
//*****************************************************************************
extern tBoolean g_bClicksEnabled;

//*****************************************************************************
//
// This function initializes the menu and displays the first control on
// the display.
//
//*****************************************************************************
extern void MenuInit(void);

//*****************************************************************************
//
// Top level function called when any key changes state.  This updates the
// relevant control or controls on the screen and processes the key
// appropriately for the control that currently has focus.
//
//*****************************************************************************
extern tBoolean MenuProcess(unsigned char ucButtons, unsigned char ucChanged,
    unsigned char ucRepeat);

//*****************************************************************************
//
// Top level function called by the application to tell the menu to refresh
// the display of the current focus control.
//
//*****************************************************************************
extern void MenuRefresh(void);

//*****************************************************************************
//
// Prototypes for other helpful functions exported from this file.
//
//*****************************************************************************
extern void DrawTextBox(const char *pszText, tRectangle *prectOutline,
                        tOutlineTextColors *psColors);
extern void DrawTextBoxWithMarkers(const char *pszText,
                                   tRectangle *prectOutline,
                                   tOutlineTextColors *psColors,
                                   tBoolean bLeftRight);
extern void DrawDirectionMarkers(tRectangle *prectOutline, tBoolean bLeftRight,
                                 unsigned long ulColor);

#endif // __MENU_H__
