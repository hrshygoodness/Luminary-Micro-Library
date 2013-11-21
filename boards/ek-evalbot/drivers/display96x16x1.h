//*****************************************************************************
//
// display96x16x1.h - Prototypes for the driver for the 96x16 monochrome
//                    graphical OLED display found on the ek-evalbot board.
//
// Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#ifndef __DISPLAY96X16X1_H__
#define __DISPLAY96X16X1_H__

//*****************************************************************************
//
//! The number of pixel columns across the display.
//
//*****************************************************************************
#define DISPLAY_WIDTH 96

//*****************************************************************************
//
//! The width of a character cell in pixels.  This applies to the font embedded
//! within the display driver.
//
//*****************************************************************************
#define CHAR_CELL_WIDTH 6

//*****************************************************************************
//
//! The number of characters that can be printed on a single line of the
//! display assuming a 6 pixel wide character cell.
//
//*****************************************************************************
#define CHARS_PER_LINE (DISPLAY_WIDTH / CHAR_CELL_WIDTH)

//*****************************************************************************
//
// Prototypes for the driver APIs.
//
//*****************************************************************************
extern void Display96x16x1Clear(void);
extern void Display96x16x1ClearLine(unsigned long ulY);
extern void Display96x16x1StringDraw(const char *pcStr, unsigned long ulX,
                                   unsigned long ulY);
extern void Display96x16x1StringDrawLen(const char *pcStr, unsigned long ulLen,
                                        unsigned long ulX, unsigned long ulY);
extern void Display96x16x1StringDrawCentered(const char *pcStr,
                                             unsigned long ulY,
                                             tBoolean bClear);
extern void Display96x16x1ImageDraw(const unsigned char *pucImage,
                                  unsigned long ulX, unsigned long ulY,
                                  unsigned long ulWidth,
                                  unsigned long ulHeight);
extern void Display96x16x1Init(tBoolean bFast);
extern void Display96x16x1DisplayOn(void);
extern void Display96x16x1DisplayOff(void);

#endif // __DISPLAY96X16X1_H__
