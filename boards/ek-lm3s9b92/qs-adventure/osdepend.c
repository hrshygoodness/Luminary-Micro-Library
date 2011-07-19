//*****************************************************************************
//
// osdepend.c - OS-dependent routines for ZIP.
//
// Copyright (c) 2009-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 7611 of the EK-LM3S9B92 Firmware Package.
//
//*****************************************************************************

#include "zip/ztypes.h"

//*****************************************************************************
//
// This function plays a sound file.  This is a NOP since sound playback is not
// supported (nor is it used by Adventure).
//
//*****************************************************************************
void
sound(int argc, zword_t *argv)
{
}

//*****************************************************************************
//
// This function is used to report a fatal error in ZIP.
//
//*****************************************************************************
void
fatal(const char *s)
{
}

//*****************************************************************************
//
// This function determines if a given line fits in the available space.
//
//*****************************************************************************
int
fit_line(const char *line_buffer, int pos, int max)
{
    return(pos < max);
}

//*****************************************************************************
//
// This function prints the status line for type 3 games.  This is a NOP since
// Adventure is a type 5 game.
//
//*****************************************************************************
int
print_status(int argc, char *argv[])
{
    return(FALSE);
}

//*****************************************************************************
//
// This function sets the character font.  This is a NOP since a non-graphical
// interface is used.
//
//*****************************************************************************
void
set_font(int font_type)
{
}

//*****************************************************************************
//
// This function sets the foreground and background color of the screen.  This
// is a NOP since a non-graphical interface is used.
//
//*****************************************************************************
void
set_colours(int foreground, int background)
{
}

//*****************************************************************************
//
// This function translates Z-code character to machine specific characters.
// This is a NOP since no translations are provided.
//
//*****************************************************************************
int
codes_to_text(int c, char *s)
{
    return(1);
}
