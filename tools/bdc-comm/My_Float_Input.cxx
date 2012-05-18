//*****************************************************************************
//
// My_Float_Input.cxx - The My_Float_Input FLTK class.
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

#include "My_Float_Input.H"
#include <FL/fl_ask.H>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//*****************************************************************************
//
// The constructor for the My_Float_Class.
//
//*****************************************************************************
My_Float_Input::My_Float_Input(int x, int y, int w, int h,
                               const char *label) :
    Fl_Input(x, y, w, h, label)
{
    type(FL_FLOAT_INPUT);
}

//*****************************************************************************
//
// The event handler for the My_Float_Input class.  This function only handles
// the FL_UNFOCUS event and leave passes all other events to the parent class.
//
//*****************************************************************************
int
My_Float_Input::handle(int event)
{
    //
    // Handle the FL_UNFOCUS event or pass it on to the parent class.
    //
    if(event == FL_UNFOCUS)
    {
        char *pEnd, pcBuffer[64];
        const char *pValue;
        double dValue;

        //
        // Get the text version of the value.
        //
        pValue = Fl_Input::value();

        //
        // Convert the text to a decimal value.
        //
        dValue = strtod(pValue, &pEnd);

        //
        // The end value for the string could not be found.
        //
        if(*pEnd != '\0')
        {
            fl_alert("An invalid value was entered.");
            dValue = value_;
        }

        //
        // Round the decimal value to power of the significant digits.
        //
        dValue = rint(dValue * pow(10, digits_)) / pow(10, digits_);

        //
        // Limit the value to the valid range for this number.
        //
        if(dValue < min_)
        {
            dValue = min_;
        }
        if(dValue > max_)
        {
            dValue = max_;
        }

        //
        // Set the value.
        //
        value(dValue);
    }

    return(Fl_Input::handle(event));
}

//*****************************************************************************
//
// This member function sets the valid range of values for the floating point
// number represented by this instance.
//
//*****************************************************************************
void
My_Float_Input::range(double min, double max)
{
    //
    // This just saves the minimum and maximum values for this floating point
    // number.
    //
    min_ = min;
    max_ = max;
}

//*****************************************************************************
//
// This member function sets the precision of the floating point number in
// significant digits.
//
//*****************************************************************************
void
My_Float_Input::precision(int digits)
{
    //
    // This just saves the significant digits for this floating point
    // number.
    //
    digits_ = digits;
}

//*****************************************************************************
//
// This member function returns the textual value for the field associated with
// this class.
//
//*****************************************************************************
const char *
My_Float_Input::value(void) const
{
    return(Fl_Input::value());
}

//*****************************************************************************
//
// This member function sets the textual value for the field associated with
// this class.
//
//*****************************************************************************
int
My_Float_Input::value(const char *val)
{
    return(Fl_Input::value(val));
}

//*****************************************************************************
//
// This member function sets the textual value for the field associated with
// this class and allows it to be truncated at i value.
//
//*****************************************************************************
int
My_Float_Input::value(const char *val, int i)
{
    return(Fl_Input::value(val, i));
}

//*****************************************************************************
//
// This member function returns the floating point value.
//
//*****************************************************************************
double
My_Float_Input::value(void)
{
    return(value_);
}

//*****************************************************************************
//
// This member function will take a floating point value and store it in the
// string value for this class.
//
//*****************************************************************************
int
My_Float_Input::value(double val)
{
    char pcBuffer[128];
    int ret = 1;

    //
    // There is no change so just return.
    //
    if(val == value_)
    {
        ret = 0;
    }

    //
    // Save the numeric value.
    //
    value_ = val;

    //
    // Convert the value to a string.
    //
    if(digits_ == 0)
    {
        snprintf(pcBuffer, sizeof(pcBuffer), "%ld", (long)value_);
    }
    else
    {
        snprintf(pcBuffer, sizeof(pcBuffer), "%0.*f", digits_, value_);
    }

    //
    // Set the value in the class and mark it as changed.
    //
    value(pcBuffer);
    set_changed();

    return(ret);
}
