//*****************************************************************************
//
// io.c - I/O routines for ZIP.
//
// Copyright (c) 2009-2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6075 of the EK-LM3S9B92 Firmware Package.
//
//*****************************************************************************

#include "zip/ztypes.h"
#include "common.h"

//*****************************************************************************
//
// The column in which the cursor resides.
//
//*****************************************************************************
static long g_lCurColumn = 1;

//*****************************************************************************
//
// The saved cursor column.
//
//*****************************************************************************
static long g_lSavedColumn;

//*****************************************************************************
//
// A flag that is non-zero if there is a saved cursor column.
//
//*****************************************************************************
static long g_lCursorSaved = 0;

//*****************************************************************************
//
// A flag that is non-zero if characters should be displayed on the screen.
// This is used to prevent the status line from being displayed.
//
//*****************************************************************************
static long g_lDisplay = 1;

//*****************************************************************************
//
// The character that was most recently read from the player.
//
//*****************************************************************************
static long g_lPrevChar = 0;

//*****************************************************************************
//
// This function initializes the screen.
//
//*****************************************************************************
void
initialize_screen(void)
{
    //
    // Set the dimensions of the screen.
    //
    screen_cols = 79;
    screen_rows = 25;

    //
    // Set the current column to the beginning of the line.
    //
    g_lCurColumn = 1;

    //
    // Discard any saved cursor column.
    //
    g_lCursorSaved = 0;

    //
    // Characters should be displayed.
    //
    g_lDisplay = 1;

    //
    // Set the type of ZIP interpreter.
    //
    h_interpreter = INTERP_GENERIC;
}

//*****************************************************************************
//
// This function restarts the screen.
//
//*****************************************************************************
void
restart_screen(void)
{
    //
    // Set the current column to the beginning of the line.
    //
    g_lCurColumn = 1;

    //
    // Discard any saved cursor column.
    //
    g_lCursorSaved = 0;

    //
    // Characters should be displayed.
    //
    g_lDisplay = 1;

    //
    // The the ZIP configuration flags based on the type of the game.
    //
    if(h_type < V4)
    {
        set_byte(H_CONFIG, get_byte(H_CONFIG) | CONFIG_WINDOWS);
    }
    else
    {
        set_byte(H_CONFIG,
                 get_byte(H_CONFIG) | CONFIG_EMPHASIS | CONFIG_WINDOWS);
    }

    //
    // Set the ZIP flags.
    //
    set_word(H_FLAGS, (get_word(H_FLAGS) & ~GRAPHICS_FLAG));
}

//*****************************************************************************
//
// This function resets the screen.
//
//*****************************************************************************
void
reset_screen(void)
{
    //
    // Select the text window.
    //
    select_text_window();

    //
    // Return the character rendering to normal.
    //
    set_attribute(NORMAL);
}

//*****************************************************************************
//
// This function clears the screen.
//
//*****************************************************************************
void
clear_screen(void)
{
    //
    // Send the ANSI sequence to clear the screen.
    //
    display_char('\033');
    display_char('[');
    display_char('2');
    display_char('J');

    //
    // Set the current column to the beginning of the line.
    //
    g_lCurColumn = 1;
}

//*****************************************************************************
//
// This function selects the status window.
//
//*****************************************************************************
void
select_status_window(void)
{
    //
    // Stop displaying characters since the status window is not supported.
    //
    g_lDisplay = 0;

    //
    // Save the cursor position.
    //
    save_cursor_position();
}

//*****************************************************************************
//
// This function selects the text window.
//
//*****************************************************************************
void
select_text_window(void)
{
    //
    // Start displaying characters.
    //
    g_lDisplay = 1;

    //
    // Restore the cursor position.
    //
    restore_cursor_position();
}

//*****************************************************************************
//
// This function creates the status window.  This is a NOP since the status
// window is not supported.
//
//*****************************************************************************
void
create_status_window(void)
{
}

//*****************************************************************************
//
// This function deletes the status window.  This is a NOP since the status
// window is not supported.
//
//*****************************************************************************
void
delete_status_window(void)
{
}

//*****************************************************************************
//
// This function clears the current line.
//
//*****************************************************************************
void
clear_line(void)
{
    long lColumn;

    //
    // Save the current column.
    //
    lColumn = g_lCurColumn;

    //
    // Send the ANSI sequence to clear the current line.
    //
    display_char('\033');
    display_char('[');
    display_char('2');
    display_char('K');

    //
    // Restore the current column.
    //
    g_lCurColumn = lColumn;
}

//*****************************************************************************
//
// This function clears the text window.
//
//*****************************************************************************
void
clear_text_window(void)
{
    //
    // Clear the entire screen.
    //
    clear_screen();
}

//*****************************************************************************
//
// This function clears the status window.  This is a NOP since the status
// window is not supported.
//
//*****************************************************************************
void
clear_status_window(void)
{
}

//*****************************************************************************
//
// This function moves the cursor to the specified position.
//
//*****************************************************************************
void
move_cursor(int row, int col)
{
    long lDiff;

    //
    // See if the text should be displayed, which means that the text window is
    // selected.
    //
    if(g_lDisplay)
    {
        //
        // See if the cursor should be moved to the left.
        //
        if(col < g_lCurColumn)
        {
            //
            // Determine how many columns to move the cursor to the left.
            //
            lDiff = g_lCurColumn - col;

            //
            // Send the ANSI sequence to move the cursor to the left.
            //
            display_char('\033');
            display_char('[');
            if(lDiff > 9)
            {
                display_char('0' + (lDiff / 10));
            }
            display_char('0' + (lDiff % 10));
            display_char('D');
        }

        //
        // See if the cursor should be moved to the right.
        //
        else if(col > g_lCurColumn)
        {
            //
            // Determine how many columns to move the cursor to the right.
            //
            lDiff = col - g_lCurColumn;

            //
            // Send the ANSI sequence to move the cursor to the right.
            //
            display_char('\033');
            display_char('[');
            if(lDiff > 9)
            {
                display_char('0' + (lDiff / 10));
            }
            display_char('0' + (lDiff % 10));
            display_char('C');
        }

        //
        // Save the new cursor column.
        //
        g_lCurColumn = col;
    }
}

//*****************************************************************************
//
// This function returns the current cursor position.
//
//*****************************************************************************
void
get_cursor_position(int *row, int *col)
{
    //
    // Return the current cursor position.  The row is always assumed to be the
    // "bottom" row.
    //
    *row = 25;
    *col = g_lCurColumn;
}

//*****************************************************************************
//
// This function saves the cursor position.
//
//*****************************************************************************
void
save_cursor_position(void)
{
    //
    // Only save the cursor position if it has not already been saved.
    //
    if(!g_lCursorSaved)
    {
        //
        // Save the current cursor position.
        //
        g_lSavedColumn = g_lCurColumn;

        //
        // Indicate that the cursor position is saved.
        //
        g_lCursorSaved = 1;
    }
}

//*****************************************************************************
//
// This function restores the saved cursor position.
//
//*****************************************************************************
void
restore_cursor_position(void)
{
    //
    // See if there is a saved cursor position.
    //
    if(g_lCursorSaved)
    {
        //
        // Move the cursor to the saved cursor position.
        //
        move_cursor(1, g_lSavedColumn);

        //
        // Indicate that the cursor postiion is no longer saved.
        //
        g_lCursorSaved = 0;
    }
}

//*****************************************************************************
//
// This function sets the character rendering attributes.
//
//*****************************************************************************
void
set_attribute(int attribute)
{
    int lColumn;

    //
    // See if the text should be displayed, which means that the text window is
    // selected.
    //
    if(g_lDisplay)
    {
        //
        // Save the current cursor position.
        //
        lColumn = g_lCurColumn;

        //
        // See if the text attributes be returned to normal.
        //
        if(attribute == NORMAL)
        {
            //
            // Send the ANSI sequence to turn off all character attributes.
            //
            display_char('\033');
            display_char('[');
            display_char('m');
        }

        //
        // See if the text should be in reverse.
        //
        if(attribute & REVERSE)
        {
            //
            // Send the ANSI sequence to reverse the video.
            //
            display_char('\033');
            display_char('[');
            display_char('7');
            display_char('m');
        }

        //
        // See if the text should be in bold.
        //
        if(attribute & BOLD)
        {
            //
            // Send the ANSI sequence to select bold characters.
            //
            display_char('\033');
            display_char('[');
            display_char('1');
            display_char('m');
        }

        //
        // See if the text should be in emphasis.
        //
        if(attribute & EMPHASIS)
        {
            //
            // Send the ANSI sequence to select underline characters.
            //
            display_char('\033');
            display_char('[');
            display_char('4');
            display_char('m');
        }

        //
        // Restore the current cursor position.
        //
        g_lCurColumn = lColumn;
    }
}

//*****************************************************************************
//
// This function prints a character on the screen.
//
//*****************************************************************************
void
display_char(int c)
{
    //
    // See if the text should be displayed, which means that the text window is
    // selected.
    //
    if(g_lDisplay)
    {
        //
        // See if USB is being used to play the game.
        //
        if(g_ulGameIF == GAME_IF_USB)
        {
            //
            // If this is a newline, send a carriage return first.
            //
            if(c == '\n')
            {
                USBIFWrite('\r');
            }

            //
            // Send this character.
            //
            USBIFWrite(c);
        }

        //
        // See if Ethernet is being used to play the game.
        //
        else if(g_ulGameIF == GAME_IF_ENET)
        {
            //
            // If this is a newline, send a carriage return first.
            //
            if(c == '\n')
            {
                EnetIFWrite('\r');
            }

            //
            // Send this character.
            //
            EnetIFWrite(c);
        }

        //
        // Increment the current cursor column, not allowing it to exceed the
        // right edge of the screen.
        //
        if(++g_lCurColumn > screen_cols)
        {
            g_lCurColumn = screen_cols;
        }
    }
}

//*****************************************************************************
//
// This function scrolls the screen by one line.
//
//*****************************************************************************
void
scroll_line(void)
{
    //
    // See if the text should be displayed, which means that the text window is
    // selected.
    //
    if(g_lDisplay)
    {
        //
        // Send a newline character.
        //
        display_char('\n');

        //
        // Set the current column to the beginning of the line.
        //
        g_lCurColumn = 1;
    }
}

//*****************************************************************************
//
// This function reads a character from the player.
//
//*****************************************************************************
int
input_character(int timeout)
{
    int iChar = 0;

    //
    // Loop while the ZIP interpreter has not been halted or is not being
    // restarted.
    //
    while(!halt && !g_ulRestart)
    {
        //
        // See if USB is being used to play the game.
        //
        if(g_ulGameIF == GAME_IF_USB)
        {
            //
            // Read a character from USB.
            //
            iChar = USBIFRead();
        }

        //
        // See if Ethernet is being used to play the game.
        //
        else if(g_ulGameIF == GAME_IF_ENET)
        {
            //
            // Read a character from Ethernet.
            //
            iChar = EnetIFRead();
        }

        //
        // Otherwise, set the read character to a newline.
        //
        else
        {
            iChar = '\n';
        }

        //
        // Stop looping if a character was available.
        //
        if(iChar != 0)
        {
            break;
        }
    }

    //
    // Return the character that was read.
    //
    return(iChar);
}

//*****************************************************************************
//
// This function reads a line of text from the player.
//
//*****************************************************************************
int
input_line(int buflen, char *buffer, int timeout, int *read_size)
{
    int iRow, iColumn;
    long lChar;

    //
    // Loop forever.  This loop will be explicitly when appropriate.
    //
    while(1)
    {
        //
        // Read a character.
        //
        lChar = input_character(timeout);

        //
        // If the ZIP interpreter has been halted, then return immediately.
        //
        if(halt)
        {
            return('\n');
        }

        //
        // See if a backsapce or delete character was read.
        //
        if((lChar == '\b') || (lChar == 0x7f))
        {
            //
            // See if there are any characters in the buffer.
            //
            if(*read_size != 0)
            {
                //
                // Decrement the number of characters in the buffer.
                //
                (*read_size)--;

                //
                // Get the cursor position.
                //
                get_cursor_position(&iRow, &iColumn);

                //
                // Move the cursor one character to the left.
                //
                move_cursor(iRow, --iColumn);

                //
                // Display a space to erase the previous character.
                //
                display_char(' ');

                //
                // Move the cursor back to the left.
                //
                move_cursor(iRow, iColumn);
            }
        }

        //
        // See if this a carriage return or newline character.
        //
        else if((lChar == '\n') || (lChar == '\r'))
        {
            //
            // Ignore this character if the previous character was the opposite
            // of the CR/LF pair.
            //
            if(((lChar == '\n') && (g_lPrevChar != '\r')) ||
               ((lChar == '\r') && (g_lPrevChar != '\n')))
            {
                //
                // Save this character as the previous character.
                //
                g_lPrevChar = lChar;

                //
                // Scroll the screen.
                //
                scroll_line();

                //
                // Return the most recently read character.
                //
                return(lChar);
            }
        }

        //
        // See if there is space in the buffer for another character.
        //
        else if(*read_size != (buflen - 1))
        {
            //
            // Save this character in the buffer.
            //
            buffer[(*read_size)++] = lChar;

            //
            // Display this character.
            //
            display_char(lChar);
        }

        //
        // Save this character as the previous character.
        //
        g_lPrevChar = lChar;
    }
}
