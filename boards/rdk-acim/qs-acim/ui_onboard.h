//*****************************************************************************
//
// ui_onboard.h - Prototypes for the on-board control interface.
//
// Copyright (c) 2006-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#ifndef __UI_ONBOARD_H__
#define __UI_ONBOARD_H__

//*****************************************************************************
//
//! \addtogroup ui_onboard_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! This structure contains a set of variables that describe the properties of
//! a switch.
//
//*****************************************************************************
typedef struct
{
    //
    //! The bit position of this switch.
    //
    unsigned char ucBit;

    //
    //! The number of sample periods which the switch must be held in order to
    //! invoke the hold function.
    //
    unsigned long ulHoldTime;

    //
    //! A pointer to the function to be called when the switch is pressed.
    //! For switches that do not have a hold function, this is called as soon
    //! as the switch is pressed.  For switches that have a hold function, it
    //! is called when the switch is released only if it was held for less than
    //! the hold time (if held longer, this function will not be called).  If
    //! no press function is required then this can be NULL.
    //
    void (*pfnPress)(void);

    //
    //! A pointer to the function to be called when the switch is released.
    //! if no release function is required then this can be NULL.
    //
    void (*pfnRelease)(void);

    //
    //! A pointer to the function to be called when the switch is held for the
    //! hold time.  If no hold function is required then this can be NULL.
    //
    void (*pfnHold)(void);
}
tUIOnboardSwitch;

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// An array of the switches supported by this target.  This array must be
// supplied by the application.
//
//*****************************************************************************
extern const tUIOnboardSwitch g_sUISwitches[];

//*****************************************************************************
//
// The number of switches on this target.  This value must be supplied by the
// application.
//
//*****************************************************************************
extern const unsigned long g_ulUINumButtons;

//*****************************************************************************
//
// This is the count of the number of samples during which the switches have
// been pressed; it is used to distinguish a switch press from a switch
// hold.  This array must be supplied by the application.
//
//*****************************************************************************
extern unsigned long g_pulUIHoldCount[];

//*****************************************************************************
//
// Functions exported by the on-board user interface.
//
//*****************************************************************************
extern void UIOnboardSwitchDebouncer(unsigned long ulSwitches);
extern unsigned long UIOnboardPotentiometerFilter(unsigned long ulValue);
extern void UIOnboardInit(unsigned long ulSwitches,
                          unsigned long ulPotentiometer);

#endif // __UI_ONBOARD_H__
