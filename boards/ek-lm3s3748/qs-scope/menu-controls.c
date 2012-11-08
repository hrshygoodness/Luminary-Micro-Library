//*****************************************************************************
//
// menu-controls.c - Definitions associated with the user interface menu
//                   used in the oscilloscope application.
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

#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "grlib/grlib.h"
#include "utils/ustdlib.h"
#include "data-acq.h"
#include "menu.h"
#include "menu-controls.h"
#include "qs-scope.h"
#include "renderer.h"
#include "string.h"

//*****************************************************************************
//
// Positions and sizes of the two areas of the screen used to display the
// currently focused control and its value.
//
//*****************************************************************************
#define CONTROL_TOP             (WAVEFORM_BOTTOM + 1)
#define CONTROL_BOTTOM          (128 - 1)

#define CONTROL_NAME_TOP        CONTROL_TOP
#define CONTROL_NAME_BOTTOM     CONTROL_BOTTOM
#define CONTROL_NAME_LEFT       0
#define CONTROL_NAME_RIGHT      (128 / 2)

#define CONTROL_VALUE_LEFT      CONTROL_NAME_RIGHT
#define CONTROL_VALUE_RIGHT     (128 - 1)
#define CONTROL_VALUE_TOP       CONTROL_TOP
#define CONTROL_VALUE_BOTTOM    CONTROL_BOTTOM

tRectangle rectCtrlName =
{
    CONTROL_NAME_LEFT,
    CONTROL_NAME_TOP,
    CONTROL_NAME_RIGHT,
    CONTROL_NAME_BOTTOM
};

tRectangle rectCtrlValue =
{
    CONTROL_VALUE_LEFT,
    CONTROL_VALUE_TOP,
    CONTROL_VALUE_RIGHT,
    CONTROL_VALUE_BOTTOM
};

tOutlineTextColors g_sControlColors =
{
    ClrDarkBlue,
    ClrWhite,
    ClrWhite
};

//*****************************************************************************
//
// Structures containing information on allowed values for various
// menu controls
//
//*****************************************************************************
typedef struct
{
    const char *pszChoice;
    unsigned long ulValue;
}
tControlChoice;

typedef struct
{
    unsigned short usCommand;
    unsigned short usNumChoices;
    tBoolean bRedrawNeeded;
    tBoolean bAllowWrap;
    tControlChoice *pChoices;
}
tChoiceData;

typedef struct
{
    long lMinimum;
    long lMaximum;
    long lStep;
    tBoolean bRedrawNeeded;
    unsigned short usCommand;
    const char *pcUnits;
    const char *pcUnits1000;
}
tBoundedIntegerData;

//*****************************************************************************
//
// Vertical position control data
//
//*****************************************************************************
tBoundedIntegerData g_sCh1VertPositionData =
{
    -ADC_MAX_MV,
    ADC_MAX_MV,
    100,
    false,
    SCOPE_CH1_POS,
    "mV",
    "V"
};

tBoundedIntegerData g_sCh2VertPositionData =
{
    -ADC_MAX_MV,
    ADC_MAX_MV,
    100,
    false,
    SCOPE_CH2_POS,
    "mV",
    "V"
};

//*****************************************************************************
//
// Horizontal position control data
//
//*****************************************************************************
tBoundedIntegerData g_sHorzPositionData =
{
    -(WAVEFORM_WIDTH / 2),
    (WAVEFORM_WIDTH / 2),
    1,
    false,
    SCOPE_TRIGGER_POS,
    "", ""
};

//*****************************************************************************
//
// Trigger level control data
//
//*****************************************************************************
tBoundedIntegerData g_sTriggerLevelData =
{
    -(ADC_MAX_MV / 2),
    (ADC_MAX_MV / 2),
    100,
    false,
    SCOPE_TRIGGER_LEVEL,
    "mV", "V"
};

//*****************************************************************************
//
// Channel 2 display choices.
//
//*****************************************************************************
tControlChoice g_psChannel2Choices[] =
{
    {"ON", true},
    {"OFF", false}
};

#define NUM_CHANNEL2_CHOICES    (sizeof(g_psChannel2Choices) / \
                                 sizeof(tControlChoice))

tChoiceData g_sChannel2Choices =
{
    SCOPE_CH2_DISPLAY,
    NUM_CHANNEL2_CHOICES,
    false,
    true,
    g_psChannel2Choices
};

//*****************************************************************************
//
// Trigger type choices.
//
//*****************************************************************************
tControlChoice g_psTriggerTypeChoices[] =
{
    { "Rising", TRIGGER_RISING },
    { "Falling", TRIGGER_FALLING },
    { "Level", TRIGGER_LEVEL },
    { "Always", TRIGGER_ALWAYS }
};

#define NUM_TRIGGER_CHOICES     (sizeof(g_psTriggerTypeChoices) / \
                                 sizeof(tControlChoice))
tChoiceData g_sTriggerChoices =
{
    SCOPE_CHANGE_TRIGGER,
    NUM_TRIGGER_CHOICES,
    false,
    true,
    g_psTriggerTypeChoices
};

//*****************************************************************************
//
// Trigger channel choices.
//
//*****************************************************************************
tControlChoice g_psTriggerChannelChoices[] =
{
    { "1", CHANNEL_1 },
    { "2", CHANNEL_2 },
};

#define NUM_TRIGGER_CHANNEL_CHOICES                                  \
                                (sizeof(g_psTriggerChannelChoices) / \
                                 sizeof(tControlChoice))
tChoiceData g_sTriggerChannelChoices =
{
    SCOPE_SET_TRIGGER_CH,
    NUM_TRIGGER_CHANNEL_CHOICES,
    false,
    true,
    g_psTriggerChannelChoices
};
//*****************************************************************************
//
// Timebase choices.
//
//*****************************************************************************
tControlChoice g_psTimebaseChoices[] =
{
    { "2uS",   2 },
    { "5uS",   5 },
    { "10uS",  10 },
    { "25uS",  25 },
    { "50uS",  50 },
    { "100uS", 100 },
    { "250uS", 250 },
    { "500uS", 500 },
    { "1mS",   1000 },
    { "2.5mS", 2500 },
    { "5mS",   5000 },
    { "10mS",  10000 },
    { "25mS",  25000 },
    { "50mS",  50000 }
};

#define NUM_TIMEBASE_CHOICES    (sizeof(g_psTimebaseChoices) / \
                                 sizeof(tControlChoice))

tChoiceData g_sTimebaseChoices =
{
    SCOPE_CHANGE_TIMEBASE,
    NUM_TIMEBASE_CHOICES,
    false,
    false,
    g_psTimebaseChoices
};

//*****************************************************************************
//
// Vertical (voltage) scaling choices.
//
//*****************************************************************************
tControlChoice g_psScaleChoices[] =
{
    { "100mV",  100 },
    { "200mV",  200 },
    { "500mV",  500 },
    { "1V",     1000 },
    { "2V",     2000 },
    { "5V",     5000 },
    { "10V",    10000 }
};

#define NUM_SCALE_CHOICES       (sizeof(g_psScaleChoices) / \
                                 sizeof(tControlChoice))

tChoiceData g_sCh1ScaleChoices =
{
    SCOPE_CH1_SCALE,
    NUM_SCALE_CHOICES,
    false,
    false,
    g_psScaleChoices
};

tChoiceData g_sCh2ScaleChoices =
{
    SCOPE_CH2_SCALE,
    NUM_SCALE_CHOICES,
    false,
    false,
    g_psScaleChoices
};

//*****************************************************************************
//
// USB mode choices.
//
//*****************************************************************************
tControlChoice g_psUSBModeChoices[] =
{
    { "Host",   1 },
    { "Device", 0 }
};

#define NUM_USB_MODE_CHOICES    (sizeof(g_psUSBModeChoices) / \
                                 sizeof(tControlChoice))

tChoiceData g_sUSBModeChoices =
{
    SCOPE_SET_USB_MODE,
    NUM_USB_MODE_CHOICES,
    false,
    true,
    g_psUSBModeChoices
};

//*****************************************************************************
//
// Help screen display options.
//
//*****************************************************************************
tControlChoice g_psShowHelpChoices[] =
{
    { "Show",   0 },
    { "Hide",   1 }
};

#define NUM_SHOW_HELP_CHOICES   (sizeof(g_psShowHelpChoices) / \
                                 sizeof(tControlChoice))

tChoiceData g_sShowHelpChoices =
{
    SCOPE_SHOW_HELP,
    NUM_SHOW_HELP_CHOICES,
    false,
    true,
    g_psShowHelpChoices
};

//*****************************************************************************
//
// Prototypes for each of the group and control event handlers.
//
//*****************************************************************************
tBoolean GroupEventHandler(tGroup *pGroup, tEvent eEvent);
tBoolean DummyControlProc(tControl *psControl, tEvent eEvent);
tBoolean BooleanDisplayControlProc(tControl *psControl, tEvent eEvent);
tBoolean FixedChoiceSetControlProc(tControl *psControl, tEvent eEvent);
tBoolean BoundedIntegerControlProc(tControl *psControl, tEvent eEvent);
tBoolean TriggerModeControlProc(tControl *psControl, tEvent eEvent);
tBoolean TriggerAcquireControlProc(tControl *psControl, tEvent eEvent);
tBoolean FindSignalControlProc(tControl *psControl, tEvent eEvent);
tBoolean SaveControlProc(tControl *psControl, tEvent eEvent);

//*****************************************************************************
//
// Definitions of the controls in the display group and the group itself.
//
//*****************************************************************************
tControl g_sControlDisplayCh2 =
{
    "Channel 2",
    FixedChoiceSetControlProc,
    &g_sChannel2Choices
};
tControl g_sControlTimebase =
{
    "Timebase",
    FixedChoiceSetControlProc,
    &g_sTimebaseChoices
};
tControl g_sControlCh1Scale =
{
    "Ch1 Scale",
    FixedChoiceSetControlProc,
    &g_sCh1ScaleChoices
};
tControl g_sControlCh2Scale =
{
    "Ch2 Scale",
    FixedChoiceSetControlProc,
    &g_sCh2ScaleChoices
};
tControl g_sControlCh1Offset =
{
    "Ch1 Offset",
    BoundedIntegerControlProc,
    &g_sCh1VertPositionData
};
tControl g_sControlCh2Offset =
{
    "Ch2 Offset",
    BoundedIntegerControlProc,
    &g_sCh2VertPositionData
};

tControl *g_psDisplayControls[] =
{
    &g_sControlDisplayCh2,
    &g_sControlTimebase,
    &g_sControlCh1Scale,
    &g_sControlCh2Scale,
    &g_sControlCh1Offset,
    &g_sControlCh2Offset
};

#define NUM_DISPLAY_CONTROLS    (sizeof(g_psDisplayControls) / \
                                 sizeof(tControl *))

tGroup g_sGroupDisplay =
{
    NUM_DISPLAY_CONTROLS, 0, "Display", g_psDisplayControls, GroupEventHandler
};

//*****************************************************************************
//
// Definitions of the controls in the trigger group and the group itself.
//
//*****************************************************************************
tControl g_sControlTriggerType =
{
    "Trigger",
    FixedChoiceSetControlProc,
    &g_sTriggerChoices
};
tControl g_sControlTriggerChannel =
{
    "Trig Channel",
    FixedChoiceSetControlProc,
    &g_sTriggerChannelChoices
};
tControl g_sControlTriggerLevel =
{
    "Trig Level",
    BoundedIntegerControlProc,
    &g_sTriggerLevelData
};
tControl g_sControlTriggerPos =
{
    "Trig Pos",
    BoundedIntegerControlProc,
    &g_sHorzPositionData
};
tControl g_sControlTriggerMode =
{
    "Mode",
    TriggerModeControlProc,
    0
};
tControl g_sControlTriggerAcquire =
{
    "One Shot",
    TriggerAcquireControlProc,
    0
};

tControl *g_psTriggerControls[] =
{
    &g_sControlTriggerType,
    &g_sControlTriggerChannel,
    &g_sControlTriggerLevel,
    &g_sControlTriggerPos,
    &g_sControlTriggerMode,
    &g_sControlTriggerAcquire
};

#define NUM_TRIGGER_CONTROLS    (sizeof(g_psTriggerControls) / \
                                 sizeof(tControl *))

tGroup g_sGroupTrigger =
{
    NUM_TRIGGER_CONTROLS, 0, "Trigger", g_psTriggerControls, GroupEventHandler
};

//*****************************************************************************
//
// Definitions of the controls in the setup group and the group itself.
//
//*****************************************************************************
tControl g_sControlSetupCaptions =
{
    "Captions",
    BooleanDisplayControlProc,
    &g_sRender.bShowCaptions
};
tControl g_sControlSetupVoltages =
{
    "Voltages",
    BooleanDisplayControlProc,
    &g_sRender.bShowMeasurements
};
tControl g_sControlSetupGrid =
{
    "Grid",
    BooleanDisplayControlProc,
    &g_sRender.bDrawGraticule
};
tControl g_sControlSetupGround =
{
    "Ground",
    BooleanDisplayControlProc,
    &g_sRender.bDrawGround
};
tControl g_sControlSetupTrigLevel =
{
    "Trig Level",
    BooleanDisplayControlProc,
    &g_sRender.bDrawTrigLevel
};
tControl g_sControlSetupTrigPos =
{
    "Trig Pos",
    BooleanDisplayControlProc,
    &g_sRender.bDrawTrigPos
};
tControl g_sControlSetupClick =
{
    "Clicks",
    BooleanDisplayControlProc,
    &g_bClicksEnabled
};
tControl g_sControlSetupUSB =
{
    "USB Mode",
    FixedChoiceSetControlProc,
    &g_sUSBModeChoices
};

tControl *g_psSetupControls[] =
{
    &g_sControlSetupCaptions,
    &g_sControlSetupVoltages,
    &g_sControlSetupGrid,
    &g_sControlSetupGround,
    &g_sControlSetupTrigLevel,
    &g_sControlSetupTrigPos,
    &g_sControlSetupClick,
    &g_sControlSetupUSB
};

#define NUM_SETUP_CONTROLS      (sizeof(g_psSetupControls) / \
                                 sizeof(tControl *))

tGroup g_sGroupSetup =
{
    NUM_SETUP_CONTROLS, 0, "Setup", g_psSetupControls, GroupEventHandler
};

//*****************************************************************************
//
// Definitions of the controls in the file group and the group itself.
//
//*****************************************************************************
tControl g_sControlFileSaveCSV_SD =
{
    "CSV on SD",
    SaveControlProc,
    (void *)(SCOPE_SAVE_CSV | SCOPE_SAVE_SD)
};
tControl g_sControlFileSaveCSV_USB =
{
    "CSV on USB",
    SaveControlProc,
    (void *)(SCOPE_SAVE_CSV | SCOPE_SAVE_USB)
};
tControl g_sControlFileSaveBMP_SD =
{
    "BMP on SD",
    SaveControlProc,
    (void *)(SCOPE_SAVE_BMP | SCOPE_SAVE_SD)
};
tControl g_sControlFileSaveBMP_USB =
{
    "BMP on USB",
    SaveControlProc,
    (void *)(SCOPE_SAVE_BMP | SCOPE_SAVE_USB)
};

tControl *g_psFileControls[] =
{
    &g_sControlFileSaveCSV_SD,
    &g_sControlFileSaveCSV_USB,
    &g_sControlFileSaveBMP_SD,
    &g_sControlFileSaveBMP_USB
};

#define NUM_FILE_CONTROLS       (sizeof(g_psFileControls) / sizeof(tControl *))

tGroup g_sGroupFile =
{
    NUM_FILE_CONTROLS, 0, "File", g_psFileControls, GroupEventHandler
};

//*****************************************************************************
//
// Definitions of the controls in the Help group and the group itself.
//
//*****************************************************************************
tControl g_sControlHelp =
{
    "Help",
    FixedChoiceSetControlProc,
    &g_sShowHelpChoices
};
tControl g_sControlWaveformFindCh1 =
{
    "Channel 1",
    FindSignalControlProc,
    (void *)CHANNEL_1
};
tControl g_sControlWaveformFindCh2 =
{
    "Channel 2",
    FindSignalControlProc,
    (void *)CHANNEL_2
};

tControl *g_psHelpControls[] =
{
    &g_sControlHelp,
    &g_sControlWaveformFindCh1,
    &g_sControlWaveformFindCh2
};

#define NUM_HELP_CONTROLS       (sizeof(g_psHelpControls) / sizeof(tControl *))

tGroup g_sGroupHelp =
{
    NUM_HELP_CONTROLS, 0, "Help", g_psHelpControls,
    GroupEventHandler
};

//*****************************************************************************
//
// Definition of the top level menu structure.
//
//*****************************************************************************
tGroup *g_psGroups[] =
{
    &g_sGroupDisplay,
    &g_sGroupTrigger,
    &g_sGroupSetup,
    &g_sGroupFile,
    &g_sGroupHelp
};

#define NUM_GROUPS              (sizeof(g_psGroups) / sizeof(tGroup *))

tMenu g_sMenu =
{
    NUM_GROUPS, 0, g_psGroups
};

//*****************************************************************************
//
// Performs all one-off initialization for the controls used in the menu.
//
// This function is called from MenuInit() to perform any one-off
// initialization required by the individual controls in the menu.
//
//*****************************************************************************
void
MenuControlsInit(void)
{
    //
    // Currently, there is nothing we need to do here.
    //
}

//*****************************************************************************
//
// Finds the closest higher supported scaling factor to the value passed.
//
// \param ulScaledmV is the requested scaling factor.
//
// This function find the closest scaling factor from the list of specific
// scales supported that is just larger than the desired scale passed in
// the ulScalemV parameter.
//
// \return Returns the closest supported scaling factor to the desired
// value.
//
//*****************************************************************************
unsigned long
ClosestSupportedScaleFactor(unsigned long ulScalemV)
{
    unsigned long ulLoop;
    unsigned long ulSupported = 0;

    //
    // Now find the closest supported scaling to our desired value.
    //
    for(ulLoop = 0; ulLoop < NUM_SCALE_CHOICES; ulLoop++)
    {
        if(g_psScaleChoices[ulLoop].ulValue > ulScalemV)
        {
            ulSupported = g_psScaleChoices[ulLoop].ulValue;
            break;
        }
    }

    //
    // If we dropped out, pick the largest supported scale factor (not
    // that this should ever happen, though).
    //
    if(ulLoop == NUM_SCALE_CHOICES)
    {
        ulSupported = g_psScaleChoices[ulLoop - 1].ulValue;
    }

    return(ulSupported);
}

//*****************************************************************************
//
// Handles events sent to control groups.
//
// \param pGroup points to the control group which is being sent this event.
// \param eEvent is the event being sent to the control group.
//
// This function is called from MenuProcess() to notify control groups of
// events they may need to process.  The function is responsible for
// activating the focus control (telling it to draw itself in the bottom
// portion of the display) and moving the focus between the various controls
// in the group.
//
// \return Returns \b true if the act of processing an event results in
// a change that requires the waveform display area of the screen to be
// redrawn.  If no redraw is required, \b false is returned.
//
//*****************************************************************************
tBoolean
GroupEventHandler(struct _tGroup *pGroup, tEvent eEvent)
{
    tBoolean bRetcode;
    tControl *pFocusControl;

    switch(eEvent)
    {
        //
        // When this group is given focus or gets a keystroke that will be
        // handled by one of its controls we merely pass the message on.
        //
        case MENU_EVENT_ACTIVATE:
        case MENU_EVENT_LEFT:
        case MENU_EVENT_RIGHT:
        case MENU_EVENT_LEFT_RELEASE:
        case MENU_EVENT_RIGHT_RELEASE:
        {
            pFocusControl = pGroup->ppControls[pGroup->ucFocusControl];

            bRetcode = (pFocusControl->pfnControlEventProc)(pFocusControl,
                                                            eEvent);
            break;
        }

        case MENU_EVENT_UP:
        {
            //
            // Cycle backwards to the previous control in this group, wrapping
            // if we reach the top of the list.
            //
            if(pGroup->ucFocusControl == 0)
            {
                pGroup->ucFocusControl = pGroup->ucNumControls - 1;
            }
            else
            {
                pGroup->ucFocusControl--;
            }

            //
            // Tell the new control that it is being activated.
            //
            pFocusControl = pGroup->ppControls[pGroup->ucFocusControl];
            bRetcode = (pFocusControl->pfnControlEventProc)(pFocusControl,
                                                         MENU_EVENT_ACTIVATE);
            break;
        }

        case MENU_EVENT_DOWN:
        {
            //
            // Cycle to the next control in this group, wrapping if we reach
            // the end of the list.
            //
            pGroup->ucFocusControl++;
            if(pGroup->ucFocusControl == pGroup->ucNumControls)
            {
                pGroup->ucFocusControl = 0;
            }

            //
            // Tell the new control that it is being activated.
            //
            pFocusControl = pGroup->ppControls[pGroup->ucFocusControl];
            bRetcode = (pFocusControl->pfnControlEventProc)(pFocusControl,
                                                         MENU_EVENT_ACTIVATE);
            break;
        }

        //
        // Ignore all other messages and return false to indicate no need to
        // refresh the waveform display.
        //
        default:
        {
            bRetcode = false;
            break;
        }
    }

    return(bRetcode);
}

//*****************************************************************************
//
// Handles events sent to individual controls.
//
// \param psControl points to the control which is being sent this event.
// \param eEvent is the event being sent to the control.
//
// This function is called to notify a control of some event that it may
// need to process.  Events indicate when a control is being activated (given
// user input focus) and when buttons are pressed.  Typically, left and right
// button presses are used to cycle through the valid values for a control.
// This particular function is merely a stub used during development.
//
// \return Returns \b true if the act of processing an event results in
// a change that requires the waveform display area of the screen to be
// redrawn.  If no redraw is required, \b false is returned.
//
//*****************************************************************************
tBoolean
DummyControlProc(tControl *psControl, tEvent eEvent)
{
    switch(eEvent)
    {
        case MENU_EVENT_ACTIVATE:
        {
            DrawTextBoxWithMarkers(psControl->pcName, &rectCtrlName,
                                   &g_sControlColors, false);
            DrawTextBoxWithMarkers("????", &rectCtrlValue, &g_sControlColors,
                                   true);
            break;
        }

        case MENU_EVENT_LEFT:
        case MENU_EVENT_RIGHT:
        default:
        {
            break;
        }
    }

    //
    // Indicate that no refresh of the waveform display is required.
    //
    return(false);
}

//*****************************************************************************
//
// Handles events sent to controls which offer a boolean ON/OFF choice.
//
// \param psControl points to the control which is being sent this event.
// \param eEvent is the event being sent to the control.
//
// This function is called to notify a control of some event that it may
// need to process.  Events indicate when a control is being activated (given
// user input focus) and when buttons are pressed.  Typically, left and right
// button presses are used to cycle through the valid values for a control.
// This particular function is used to support all controls which offer a
// simple ON/OFF choice.
//
// \return Returns \b true if the act of processing an event results in
// a change that requires the waveform display area of the screen to be
// redrawn.  If no redraw is required, \b false is returned.
//
//*****************************************************************************
tBoolean
BooleanDisplayControlProc(tControl *psControl, tEvent eEvent)
{
    tBoolean *pbFlag;

    //
    // Determine from the control which rendering parameter we are to
    // modify.
    //
    pbFlag = (tBoolean *)psControl->pvUserData;

    switch(eEvent)
    {
        case MENU_EVENT_ACTIVATE:
        {
            DrawTextBoxWithMarkers(psControl->pcName, &rectCtrlName,
                                   &g_sControlColors, false);
            DrawTextBoxWithMarkers(*pbFlag ? "ON" : "OFF",
                        &rectCtrlValue, &g_sControlColors, true);

            //
            // No waveform area redraw is required.
            //
            return(false);
        }

        case MENU_EVENT_LEFT:
        case MENU_EVENT_RIGHT:
        {
            *pbFlag = !*pbFlag;

            DrawTextBoxWithMarkers(*pbFlag ? "ON" : "OFF",
                        &rectCtrlValue, &g_sControlColors, true);

            //
            // The waveform display will need refreshed as a result of this
            // event.
            //
            return(true);
        }

        //
        // We ignore other events and tell the caller that no refresh of the
        // waveform display is required.
        //
        default:
        {
            return(false);
        }
    }
}

//*****************************************************************************
//
// \internal
//
// Determines which of a fixed set of choices are currently in use for a
// a particular control.
//
// \param pbChoices points to a control data structure which is to be searched
// to find the index of the current parameter value.
//
// This function searches through a structure containing an array of valid
// values for a control and returns the index of the element whose value
// matches the current setting for that control.  If no match can be found,
// 0 is returned.
//
// \return Returns the index of the current choice or 0 if no match can be
// found.
//
//*****************************************************************************
static unsigned long
FindCurrentChoice(tChoiceData *pbChoices)
{
    unsigned long ulLoop;

    //
    // Look through each of the choices given.
    //
    for(ulLoop = 0; ulLoop < pbChoices->usNumChoices; ulLoop++)
    {
        //
        // Does the current value of the parameter (as stored in the main
        // loop command parameter array) match this entry in the choice
        // structure?
        //
        if(g_ulCommandParam[pbChoices->usCommand] ==
           pbChoices->pChoices[ulLoop].ulValue)
        {
            //
            // Yes - we've found our index so return it.
            //
            return(ulLoop);
        }
    }

    //
    // If we don't find the current value in the list, default to showing
    // the first option.
    //
    return(0);
}

//*****************************************************************************
//
// Handles events sent to controls whose values are part of a fixed set of
// options.
//
// \param psControl points to the control which is being sent this event.
// \param eEvent is the event being sent to the control.
//
// This function is called to notify a control of some event that it may
// need to process.  Events indicate when a control is being activated (given
// user input focus) and when buttons are pressed.  Typically, left and right
// button presses are used to cycle through the valid values for a control.
// This particular function is used to support all controls which offer the
// user the choice of one of a fixed set of values.
//
// \return Returns \b true if the act of processing an event results in
// a change that requires the waveform display area of the screen to be
// redrawn.  If no redraw is required, \b false is returned.
//
//*****************************************************************************
tBoolean
FixedChoiceSetControlProc(tControl *psControl, tEvent eEvent)
{
    tChoiceData *pbChoices;
    unsigned long ulIndex;

    //
    // Determine the current choice.
    //
    pbChoices = (tChoiceData *)psControl->pvUserData;
    ulIndex = FindCurrentChoice(pbChoices);

    switch(eEvent)
    {
        case MENU_EVENT_ACTIVATE:
        {
            DrawTextBoxWithMarkers(psControl->pcName, &rectCtrlName,
                                   &g_sControlColors, false);
            DrawTextBoxWithMarkers(pbChoices->pChoices[ulIndex].pszChoice,
                                   &rectCtrlValue, &g_sControlColors, true);

            //
            // No waveform area redraw is required.
            //
            return(false);
        }

        case MENU_EVENT_LEFT:
        {
            //
            // Move to the previous choice for the control.
            //
            if(ulIndex > 0)
            {
                ulIndex--;
            }
            else
            {
                //
                // If we're at the first choice and wrapping is allowed,
                // wrap to the last one.
                //
                if(pbChoices->bAllowWrap)
                {
                    ulIndex = pbChoices->usNumChoices - 1;
                }
                else
                {
                    //
                    // Wrapping is not allowed so just return signalling no
                    // redraw.
                    //
                    return(false);
                }
            }
            break;
        }

        case MENU_EVENT_RIGHT:
        {
            //
            // If we are not at the last control, move forward to the next
            // one in the list.
            //
            if(ulIndex < (pbChoices->usNumChoices - 1))
            {
                ulIndex++;
            }
            else
            {
                //
                // We are already at the last choice.
                //
                if(pbChoices->bAllowWrap)
                {
                    //
                    // Wrapping is allowed for this control so wrap to the
                    // first choice.
                    //
                    ulIndex = 0;
                }
                else
                {
                    //
                    // No wrapping allowed.  Return indicating that a redraw is
                    // not required.
                    //
                    return(false);
                }
            }
            break;
        }

        //
        // We ignore other events and tell the caller that no refresh of the
        // waveform display is required.
        //
        default:
        {
            return(false);
        }
    }

    //
    // If we drop out, we need to send a command.
    //
    DrawTextBoxWithMarkers(pbChoices->pChoices[ulIndex].pszChoice,
                           &rectCtrlValue, &g_sControlColors,
                           true);
    COMMAND_FLAG_WRITE(pbChoices->usCommand,
                       pbChoices->pChoices[ulIndex].ulValue);

    return(pbChoices->bRedrawNeeded);
}

//*****************************************************************************
//
// Handles events sent to controls whose values are part of a fixed set of
// options.
//
// \param psControl points to the control which is being sent this event.
// \param eEvent is the event being sent to the control.
//
// This function is called to notify a control of some event that it may
// need to process.  Events indicate when a control is being activated (given
// user input focus) and when buttons are pressed.  Typically, left and right
// button presses are used to cycle through the valid values for a control.
// This particular function is used to support all controls which offer the
// user the choice of one of a fixed set of values.
//
// \return Returns \b true if the act of processing an event results in
// a change that requires the waveform display area of the screen to be
// redrawn.  If no redraw is required, \b false is returned.
//
//*****************************************************************************
tBoolean
BoundedIntegerControlProc(tControl *psControl, tEvent eEvent)
{
    tBoundedIntegerData *psCtlInfo;
    long lValue;
    char pcBuffer[16];

    //
    // Get the bounds data for this control and its current value
    //
    psCtlInfo = (tBoundedIntegerData *)psControl->pvUserData;
    lValue = (long)g_ulCommandParam[psCtlInfo->usCommand];

    switch(eEvent)
    {
        case MENU_EVENT_ACTIVATE:
        {
            //
            // Convert the current control value to an ASCII string.
            //
            RendererFormatDisplayString(pcBuffer, 16, "", psCtlInfo->pcUnits,
                                        psCtlInfo->pcUnits1000, lValue);

            DrawTextBoxWithMarkers(psControl->pcName, &rectCtrlName,
                                   &g_sControlColors, false);
            DrawTextBoxWithMarkers(pcBuffer, &rectCtrlValue,
                                   &g_sControlColors, true);

            //
            // No waveform area redraw is required.
            //
            return(false);
        }

        case MENU_EVENT_LEFT:
        {
            if(lValue >= (psCtlInfo->lMinimum + psCtlInfo->lStep))
            {
                lValue -= psCtlInfo->lStep;
            }
            break;
        }

        case MENU_EVENT_RIGHT:
        {
            if(lValue <= (psCtlInfo->lMaximum - psCtlInfo->lStep))
            {
                lValue += psCtlInfo->lStep;
            }
            break;
        }

        //
        // We ignore other events and tell the caller that no refresh of the
        // waveform display is required.
        //
        default:
        {
            return(false);
        }
    }

    //
    // If we drop out of the switch, we must have to update the control and
    // send a command so do it now.
    //
    RendererFormatDisplayString(pcBuffer, 16, "", psCtlInfo->pcUnits,
                                psCtlInfo->pcUnits1000, lValue);
    DrawTextBoxWithMarkers(pcBuffer, &rectCtrlValue,
                           &g_sControlColors, true);
    COMMAND_FLAG_WRITE(psCtlInfo->usCommand, (unsigned long)lValue);

    return(psCtlInfo->bRedrawNeeded);
}

tBoolean
TriggerAcquireControlProc(tControl *psControl, tEvent eEvent)
{
    switch(eEvent)
    {
        case MENU_EVENT_ACTIVATE:
        {
            DrawTextBoxWithMarkers(psControl->pcName, &rectCtrlName,
                                   &g_sControlColors, false);

            //
            // If we are capturing continuously...
            //
            if(g_bContinuousCapture)
            {
                //
                // ...then single shot capture is not available.
                //
                DrawTextBox("N/A", &rectCtrlValue,  &g_sControlColors);
            }
            else
            {
                //
                // ...otherwise it is.
                //
                DrawTextBoxWithMarkers("Capture...", &rectCtrlValue,
                                       &g_sControlColors, true);
            }

            //
            // No waveform area redraw is required.
            //
            return(false);
        }

        //
        // We trigger a one-shot capture on the button release event rather
        // than the button press.  This is to ensure that any bouncing that
        // goes on will not trigger the abort mechanism used to get out of
        // locked-up captures (where the trigger is set to some level that
        // never occurs in the signal).
        //
        case MENU_EVENT_LEFT_RELEASE:
        case MENU_EVENT_RIGHT_RELEASE:
        {
            //
            // If we are not capturing continuously...
            //
            if(!g_bContinuousCapture)
            {
                //
                // Tell the main loop to arm for a single-shot capture.
                //
                COMMAND_FLAG_WRITE(SCOPE_CAPTURE, 0);
            }
            break;
        }

        //
        // We ignore other events and tell the caller that no refresh of the
        // waveform display is required.
        //
        default:
        {
            break;
        }
    }
    return(false);
}

tBoolean
TriggerModeControlProc(tControl *psControl, tEvent eEvent)
{
    switch(eEvent)
    {
        case MENU_EVENT_ACTIVATE:
        {
            DrawTextBoxWithMarkers(psControl->pcName, &rectCtrlName,
                                   &g_sControlColors, false);
            DrawTextBoxWithMarkers((g_bContinuousCapture ? "Running" :
                                   "Stopped"), &rectCtrlValue,
                                   &g_sControlColors, true);
            //
            // No waveform area redraw is required.
            //
            return(false);
        }

        case MENU_EVENT_LEFT:
        case MENU_EVENT_RIGHT:
        {
            DrawTextBoxWithMarkers((g_bContinuousCapture ? "Stopped" :
                                   "Running"), &rectCtrlValue,
                                   &g_sControlColors, true);
            COMMAND_FLAG_WRITE(
                    (g_bContinuousCapture ? SCOPE_STOP : SCOPE_START), 0);
            break;
        }

        //
        // We ignore other events and tell the caller that no refresh of the
        // waveform display is required.
        //
        default:
        {
            break;
        }
    }

    //
    // No waveform redraw is required.
    //
    return(false);
}

//*****************************************************************************
//
// Handles events sent to the channel1 or channel2 "Find" controls.
//
// \param psControl points to the control which is being sent this event.
// \param eEvent is the event being sent to the control.
//
// This function is called to notify a control of some event that it may
// need to process.  Events indicate when a control is being activated (given
// user input focus) and when buttons are pressed.  Typically, left and right
// button presses are used to cycle through the valid values for a control.
// This particular function is used to support the menu controls used to
// find the waveform signals.  It translates left and right key presses into
// SCOPE_FIND commands sent to the main application loop.
//
// \return Returns \b false to indicate that no immediate redraw is necessary.
//
//*****************************************************************************
tBoolean
FindSignalControlProc(tControl *psControl, tEvent eEvent)
{
    int iChannel;

    //
    // Which channel are we to find?
    //
    iChannel = (int)psControl->pvUserData;

    switch(eEvent)
    {
        case MENU_EVENT_ACTIVATE:
        {
            DrawTextBoxWithMarkers(psControl->pcName, &rectCtrlName,
                                   &g_sControlColors, false);
            DrawTextBoxWithMarkers("Find...", &rectCtrlValue,
                                   &g_sControlColors, true);

            //
            // No waveform area redraw is required.
            //
            return(false);
        }

        case MENU_EVENT_LEFT:
        case MENU_EVENT_RIGHT:
        {
            COMMAND_FLAG_WRITE(SCOPE_FIND, (unsigned long)iChannel);
            return(false);
        }

        //
        // We ignore other events and tell the caller that no refresh of the
        // waveform display is required.
        //
        default:
        {
            break;
        }
    }
    return(false);
}

tBoolean
SaveControlProc(tControl *psControl, tEvent eEvent)
{
    switch(eEvent)
    {
        case MENU_EVENT_ACTIVATE:
        {
            DrawTextBoxWithMarkers(psControl->pcName, &rectCtrlName,
                                   &g_sControlColors, false);
            DrawTextBoxWithMarkers("Save...", &rectCtrlValue,
                                   &g_sControlColors, true);

            //
            // No waveform area redraw is required.
            //
            return(false);
        }

        case MENU_EVENT_LEFT:
        case MENU_EVENT_RIGHT:
        {
            //
            // Tell the main loop to save the file.  We can't do it here
            // since we need to be sure that we are not in the middle of
            // acquiring data.
            //
            COMMAND_FLAG_WRITE(SCOPE_SAVE,
                               (unsigned long)psControl->pvUserData;);
            break;
        }

        //
        // We ignore other events and tell the caller that no refresh of the
        // waveform display is required.
        //
        default:
        {
            break;
        }
    }
    return(false);
}
