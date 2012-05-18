//*****************************************************************************
//
// can_common.h - Holds the common definitions for the CAN portion of the
//                quick start application.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the EK-LM3S2965 Firmware Package.
//
//*****************************************************************************

#ifndef __CAN_COMMON_H__
#define __CAN_COMMON_H__

//****************************************************************************
//
// These is the valid 4 byte response to a ping request.
//
//****************************************************************************
#define CURRENT_VERSION         0x00000001

//****************************************************************************
//
// This is the message identifier to use for receiving button update
// requestes.
//
//****************************************************************************
#define MSGOBJ_ID_BUTTON        0x10

//****************************************************************************
//
// This is the message identifier to use for receiving LED update requests
// from the host.
//
//****************************************************************************
#define MSGOBJ_ID_LED           0x12

//****************************************************************************
//
// This is the message identifier to use for sending data in the form of
// commmands or command responses.
//
//****************************************************************************
#define MSGOBJ_ID_DATA_0        0x20

//****************************************************************************
//
// This is the message identifier to use for receiving data in the form of
// commmands or command responses.
//
//****************************************************************************
#define MSGOBJ_ID_DATA_1        0x21

//****************************************************************************
//
// This is the command used to get the firmware version.
//
//****************************************************************************
#define CMD_GET_VERSION         0x01

//****************************************************************************
//
// This event indicates that a button was pressed, it should be followed by
// the number of the button that was pressed.
//
//****************************************************************************
#define EVENT_BUTTON_PRESS      0x10

//****************************************************************************
//
// This event indicates that a button was pressed, it should be followed by
// the number of the button that was released.
//
//****************************************************************************
#define EVENT_BUTTON_RELEASED   0x11

//****************************************************************************
//
// This is the identifier for the target boards up button.
//
//****************************************************************************
#define TARGET_BUTTON_UP        0

//****************************************************************************
//
// This is the identifier for the target boards down button.
//
//****************************************************************************
#define TARGET_BUTTON_DN        1

//****************************************************************************
//
// This is the maximum level for the LED brightness on the target board. The
// value can range from 0 to MAX_LED_BRIGHTNESS.
//
//****************************************************************************
#define MAX_LED_BRIGHTNESS      1

//*****************************************************************************
//
// The upper bit of the brightness level indicates that the LED should flash
// once mode and not a remain in a contant on or off state.
//
//*****************************************************************************
#define LED_FLASH_ONCE          0x80

//*****************************************************************************
//
// This mask is used to get the actual LED level from the value sent by LED
// brightness commands.
//
//*****************************************************************************
#define LED_FLASH_VALUE_MASK    0x7f

#endif // __CAN_COMMON_H__
