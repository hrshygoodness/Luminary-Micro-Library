//*****************************************************************************
//
// My_Float_Input.H - Prototypes for the My_Float_Input FLTK class.
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
// This is part of revision 8555 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#ifndef __MY_FLOAT_INPUT_H__
#define __MY_FLOAT_INPUT_H__

#include <FL/x.H>
#include <FL/Fl_Input.H>

class FL_EXPORT My_Float_Input : public Fl_Input
{
  public:
    My_Float_Input(int x, int y, int w, int h, const char *label = 0);
    int handle(int event);
    void range(double min, double max);
    void precision(int digits);
    double value(void);
    int value(double val);
  private:
    const char *value(void) const;
    int value(const char *val);
    int value(const char *val, int i);
    double min_;
    double max_;
    double value_;
    int digits_;
};

#endif
