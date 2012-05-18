//*****************************************************************************
//
// fileio.c - File I/O routines for ZIP.
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
// This is part of revision 8555 of the EK-LM3S9D90 Firmware Package.
//
//*****************************************************************************

#include "zip/ztypes.h"
#include "adventure/advent.h"

//*****************************************************************************
//
// This function opens the story file.  This is a NOP since there is no
// mechanism to store multiple story files.
//
//*****************************************************************************
void
open_story(const char *storyname)
{
}

//*****************************************************************************
//
// This function closes the story file.
//
//*****************************************************************************
void
close_story(void)
{
}

//*****************************************************************************
//
// This function gets the size of the story file.
//
//*****************************************************************************
unsigned int
get_story_size(void)
{
    return(sizeof(g_pulAdventure));
}

//*****************************************************************************
//
// This function reads a page from the story file.
//
//*****************************************************************************
void
read_page(int page, void *buffer)
{
    memcpy(buffer, g_pulAdventure + (page * (PAGE_SIZE / 4)), PAGE_SIZE);
}

//*****************************************************************************
//
// This function verifies the integrity of the story file.
//
//*****************************************************************************
void
verify(void)
{
}

//*****************************************************************************
//
// This function saves the current game state.  This is a NOP since saving is
// not supported.
//
//*****************************************************************************
int
save(void)
{
    store_operand(0);
    return(1);
}

//*****************************************************************************
//
// This function restores the game state.
//
//*****************************************************************************
int
restore(void)
{
    store_operand(0);
    return(1);
}

//*****************************************************************************
//
// This function undoes a save operation.
//
//*****************************************************************************
void
undo_save(void)
{
    store_operand((zword_t)-1);
}

//*****************************************************************************
//
// This function undoes a restore operation.
//
//*****************************************************************************
void
undo_restore(void)
{
    store_operand((zword_t)-1);
}

//*****************************************************************************
//
// This function opens a script file used to save the output of the game.  This
// is a NOP since script files are not supported.
//
//*****************************************************************************
void
open_script(void)
{
}

//*****************************************************************************
//
// This function closes a script file.
//
//*****************************************************************************
void
close_script(void)
{
}

//*****************************************************************************
//
// This function writes a character to the script file.
//
//*****************************************************************************
void
script_char(int c)
{
}

//*****************************************************************************
//
// This function writes a string to the script file.
//
//*****************************************************************************
void
script_string(const char *s)
{
}

//*****************************************************************************
//
// This function writes a line to the script file.
//
//*****************************************************************************
void
script_line(const char *s)
{
}

//*****************************************************************************
//
// This function writes an end-of-line to the script file.
//
//*****************************************************************************
void
script_new_line(void)
{
}

//*****************************************************************************
//
// This function opens a record file used to save the sequence of commands
// provided by the user.  This is a NOP since record files are not supported.
//
//*****************************************************************************
void
open_record(void)
{
}

//*****************************************************************************
//
// This function writes a line to the record file.
//
//*****************************************************************************
void
record_line(const char *s)
{
}

//*****************************************************************************
//
// This function writes a character to the record file.
//
//*****************************************************************************
void
record_key(int c)
{
}

//*****************************************************************************
//
// This function closes the record file.
//
//*****************************************************************************
void
close_record(void)
{
}

//*****************************************************************************
//
// This function opens a record file in order to playback the sequence of
// commands.  This is a NOP since record files are not supported.
//
//*****************************************************************************
void
open_playback(int arg)
{
}

//*****************************************************************************
//
// This function reads a line from the record file.
//
//*****************************************************************************
int
playback_line(int buflen, char *buffer, int *read_size)
{
    return(-1);
}

//*****************************************************************************
//
// This function reads a character from the record file.
//
//*****************************************************************************
int
playback_key(void)
{
    return(-1);
}
