//*****************************************************************************
//
// clocksetwidget.c - A widget for setting clock date/time.
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "utils/uartstdio.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "utils/ustdlib.h"
#include "clocksetwidget.h"

//*****************************************************************************
//
//! \addtogroup clocksetwidget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This is a custom widget for setting the date/time of a clock.  The widget
// will display the year, month, day, hour and minute on the display.  The
// user can highlight the fields with the left/right keys, and can change the
// value of each with the up/down keys.  When finished the user highlights
// the OK field on the screen and presses the select button.
//
//*****************************************************************************

//*****************************************************************************
//
// Define indices for each of the fields used for date and time.
//
//*****************************************************************************
#define FIELD_YEAR 0
#define FIELD_MONTH 1
#define FIELD_DAY 2
#define FIELD_HOUR 3
#define FIELD_MINUTE 4
#define FIELD_OK 5
#define FIELD_CANCEL 6
#define FIELD_LAST 6
#define NUM_FIELDS 7

//*****************************************************************************
//
//! Paints the clock set widget on the display.
//!
//! \param pWidget is a pointer to the clock setting widget to be drawn.
//!
//! This function draws the date and time fields of the clock setting widget
//! onto the display.  One of the fields can be highlighted.  This is
//! called in response to a \b WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
ClockSetPaint(tWidget *pWidget)
{
    tClockSetWidget *pClockWidget;
    tContext sContext;
    tRectangle sRect;
    tTime *pTime;
    char cBuf[8];
    long lX, lY;
    long lWidth, lHeight;
    unsigned int uIdx;
    unsigned long ulFontHeight;
    unsigned long ulFontWidth;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);
    ASSERT(pWidget->pDisplay);

    //
    // Convert the generic widget pointer into a clock set widget pointer.
    //
    pClockWidget = (tClockSetWidget *)pWidget;
    ASSERT(pClockWidget->psTime);

    //
    // Get pointer to the time structure
    //
    pTime = pClockWidget->psTime;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sContext, pWidget->pDisplay);

    //
    // Initialize the clipping region based on the extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(pWidget->sPosition));

    //
    // Set the font for the context, and get font height and width - they
    // are used a lot later.
    //
    GrContextFontSet(&sContext, pClockWidget->pFont);
    ulFontHeight = GrFontHeightGet(pClockWidget->pFont);
    ulFontWidth = GrFontMaxWidthGet(pClockWidget->pFont);

    //
    // Fill the widget with the background color.
    //
    GrContextForegroundSet(&sContext, pClockWidget->ulBackgroundColor);
    GrRectFill(&sContext, &sContext.sClipRegion);

    //
    // Draw a border around the widget
    //
    GrContextForegroundSet(&sContext, pClockWidget->ulForegroundColor);
    GrContextBackgroundSet(&sContext, pClockWidget->ulBackgroundColor);
    GrRectDraw(&sContext, &sContext.sClipRegion);

    //
    // Compute a rectangle for the screen title.  Put it at the top of
    // the widget display, and sized to be the height of the font, plus
    // a few pixels of space.
    //
    sRect.sXMin = sContext.sClipRegion.sXMin;
    sRect.sXMax = sContext.sClipRegion.sXMax;
    sRect.sYMin = sContext.sClipRegion.sYMin;
    sRect.sYMax = ulFontHeight * 2;
    GrRectDraw(&sContext, &sRect);

    //
    // Print a title for the widget
    //
    GrContextFontSet(&sContext, pClockWidget->pFont);
    GrStringDrawCentered(&sContext, "CLOCK SET", -1,
                         (1 + sRect.sXMax - sRect.sXMin) / 2,
                         (1 + sRect.sYMax - sRect.sYMin) / 2, 1);

    //
    // Reset the rectangle to cover the non-title area of the display
    //
    sRect.sYMin = sRect.sYMax + 1;
    sRect.sYMax = sContext.sClipRegion.sYMax;

    //
    // Compute the width and height of the area remaining for showing the
    // clock fields.
    //
    lWidth = 1 + (sRect.sXMax - sRect.sXMin);
    lHeight = 1 + (sRect.sYMax - sRect.sYMin);

    //
    // Compute the X and Y starting point for the row that will show the
    // date.
    //
    lX = sRect.sXMin + (lWidth - (ulFontWidth * 10)) / 2;
    lY = sRect.sYMin + ((lHeight * 1) / 6) - (ulFontHeight / 2);

    //
    // Draw the date field separators on the date row.
    //
    GrStringDraw(&sContext, "/", -1, lX + (ulFontWidth * 4), lY, 0);
    GrStringDraw(&sContext, "/", -1, lX + (ulFontWidth * 7), lY, 0);

    //
    // Compute the X and Y starting point for the row that will show the
    // time.
    //
    lX = sRect.sXMin + (lWidth - (ulFontWidth * 5)) / 2;
    lY = sRect.sYMin + ((lHeight * 3) / 6) - (ulFontHeight / 2);

    //
    // Draw the time field separators on the time row.
    //
    GrStringDraw(&sContext, ":", -1, lX + (ulFontWidth * 2), lY, 0);

    //
    // Process each of the fields to be shown on the widget
    //
    for(uIdx = 0; uIdx < NUM_FIELDS; uIdx++)
    {
        unsigned long ulSelWidth;
        tRectangle sRectSel;

        //
        // Compute the X and Y for the text for each field, and print the
        // text into a buffer.
        //
        switch(uIdx)
        {
            //
            // Year
            //
            case FIELD_YEAR:
            {
                usnprintf(cBuf, sizeof(cBuf), "%4u", pTime->usYear);
                lX = sRect.sXMin + (lWidth - (ulFontWidth * 10)) / 2;
                lY = sRect.sYMin + ((lHeight * 1) / 6) - (ulFontHeight / 2);
                ulSelWidth = 4;
                break;
            }

            //
            // Month
            //
            case FIELD_MONTH:
            {
                usnprintf(cBuf, sizeof(cBuf), "%02u", pTime->ucMon + 1);
                lX += ulFontWidth * 5;
                ulSelWidth = 2;
                break;
            }

            //
            // Day
            //
            case FIELD_DAY:
            {
                usnprintf(cBuf, sizeof(cBuf), "%02u", pTime->ucMday);
                lX += ulFontWidth * 3;
                ulSelWidth = 2;
                break;
            }

            //
            // Hour
            //
            case FIELD_HOUR:
            {
                usnprintf(cBuf, sizeof(cBuf), "%02u", pTime->ucHour);
                lX = sRect.sXMin + (lWidth - (ulFontWidth * 5)) / 2;
                lY = sRect.sYMin + ((lHeight * 3) / 6) - (ulFontHeight / 2);
                ulSelWidth = 2;
                break;
            }

            //
            // Minute
            //
            case FIELD_MINUTE:
            {
                usnprintf(cBuf, sizeof(cBuf), "%02u", pTime->ucMin);
                lX += ulFontWidth * 3;
                ulSelWidth = 2;
                break;
            }

            //
            // OK
            //
            case FIELD_OK:
            {
                usnprintf(cBuf, sizeof(cBuf), "OK");
                lX = (lWidth - (ulFontWidth * 9)) / 2;
                lX += sRect.sXMin;
                lY = ((lHeight * 5) / 6) - (ulFontHeight / 2);
                lY += sRect.sYMin;
                ulSelWidth = 2;
                break;
            }

            //
            // CANCEL
            //
            case FIELD_CANCEL:
            {
                usnprintf(cBuf, sizeof(cBuf), "CANCEL");
                lX += ulFontWidth * 3;
                ulSelWidth = 6;
                break;
            }
        }


        //
        // If the current field index is the highlighted field, then this
        // text field will be drawn with highlighting.
        //
        if(uIdx == pClockWidget->ulHighlight)
        {
            //
            // Compute a rectangle for the highlight area.
            //
            sRectSel.sXMin = lX;
            sRectSel.sXMax = (ulSelWidth * ulFontWidth) + lX;
            sRectSel.sYMin = lY - 2;
            sRectSel.sYMax = ulFontHeight + lY + 2;

            //
            // Set the foreground color to the text color, and then fill the
            // highlight rectangle.  The text field will be highlighted by
            // inverting the normal colors.
            // Then draw the highlighting rectangle.
            //
            GrContextForegroundSet(&sContext, pClockWidget->ulForegroundColor);
            GrRectFill(&sContext, &sRectSel);

            //
            // Change the foreground color to the normal background color.
            // This will be used for drawing the text for the highlighted
            // field, which has the colors inverted (FG <--> BG)
            //
            GrContextForegroundSet(&sContext, pClockWidget->ulBackgroundColor);
        }

        //
        // Otherwise this text field is not highlighted so just set the normal
        // foreground color.
        //
        else
        {
            GrContextForegroundSet(&sContext, pClockWidget->ulForegroundColor);
        }

        //
        // Print the text from the buffer to the display at the computed
        // location.
        //
        GrStringDraw(&sContext, cBuf, -1, lX, lY, 0);
    }
}

//*****************************************************************************
//
//! Determine the number of days in a month.
//!
//! \param ucMon is the month number to use for determining the number of days.
//! The month begins with 0 meaning January and 11 meaning December.
//!
//! This function returns the highest day number for the specified month.
//! It does not account for leap year, so February always returns 28 days.
//!
//! \return Returns the highest day number for the month.
//
//*****************************************************************************
static unsigned char
MaxDayOfMonth(unsigned char ucMon)
{
    //
    // Process the month
    //
    switch(ucMon)
    {
        //
        // February returns 28 days
        //
        case 1:
        {
            return(28);
        }

        //
        // April, June, September and November return 30
        //
        case 3:
        case 5:
        case 8:
        case 10:
        {
            return(30);
        }

        //
        // Remaining months have 31 days.
        //
        default:
        {
            return(31);
        }
    }
}

//*****************************************************************************
//
//! Handle the UP button event.
//!
//! \param pWidget is a pointer to the clock setting widget on which to
//! operate.
//!
//! This function handles the event when the user has pressed the up button.
//! It will increment the currently highlighted date/time field if it is not
//! already at the maximum value.  If the month or day of the month is being
//! changed then it enforces the maximum number of days for the month.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static long
ClockSetKeyUp(tClockSetWidget *pWidget)
{
    //
    // Get pointer to the time structure to be modified.
    //
    tTime *pTime = pWidget->psTime;

    //
    // Determine which field is highlighted.
    //
    switch(pWidget->ulHighlight)
    {
        //
        // Increment the year.  Cap it at 2037 to keep things simple.
        //
        case FIELD_YEAR:
        {
            if(pTime->usYear < 2037)
            {
                pTime->usYear++;
            }
            break;
        }

        //
        // Increment the month.  Adjust the day of the month if needed.
        //
        case FIELD_MONTH:
        {
            if(pTime->ucMon < 11)
            {
                pTime->ucMon++;
            }
            if(pTime->ucMday > MaxDayOfMonth(pTime->ucMon))
            {
                pTime->ucMday = MaxDayOfMonth(pTime->ucMon);
            }
            break;
        }

        //
        // Increment the day.  Cap it at the max number of days for the
        // current value of month.
        //
        case FIELD_DAY:
        {
            if(pTime->ucMday < MaxDayOfMonth(pTime->ucMon))
            {
                pTime->ucMday++;
            }
            break;
        }

        //
        // Increment the hour.
        //
        case FIELD_HOUR:
        {
            if(pTime->ucHour < 23)
            {
                pTime->ucHour++;
            }
            break;
        }

        //
        // Increment the minute.
        //
        case FIELD_MINUTE:
        {
            if(pTime->ucMin < 59)
            {
                pTime->ucMin++;
            }
            break;
        }

        //
        // Bad value for field index - ignore.
        //
        default:
        {
            break;
        }
    }

    //
    // Since something may have been changed in the clock value, request
    // a repaint of the widget.
    //
    WidgetPaint(&pWidget->sBase);

    //
    // Return indication that the button event was handled.
    //
    return(1);
}

//*****************************************************************************
//
//! Handle the DOWN button event.
//!
//! \param pWidget is a pointer to the clock setting widget on which to
//! operate.
//!
//! This function handles the event when the user has pressed the down button.
//! It will decrement the currently highlighted date/time field if it is not
//! already at the minimum value.  If the month is being changed then it
//! enforces the maximum number of days for the month.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static long
ClockSetKeyDown(tClockSetWidget *pWidget)
{
    //
    // Get pointer to the time structure to be modified.
    //
    tTime *pTime = pWidget->psTime;

    //
    // Determine which field is highlighted.
    //
    switch(pWidget->ulHighlight)
    {
        //
        // Decrement the year.  Minimum year is 1970.
        //
        case FIELD_YEAR:
        {
            if(pTime->usYear > 1970)
            {
                pTime->usYear--;
            }
            break;
        }

        //
        // Decrement the month.  If the month has changed, check that the
        // day is valid for this month, and enforce the maximum day number
        // for this month.
        //
        case FIELD_MONTH:
        {
            if(pTime->ucMon > 0)
            {
                pTime->ucMon--;
            }
            if(pTime->ucMday > MaxDayOfMonth(pTime->ucMon))
            {
                pTime->ucMday = MaxDayOfMonth(pTime->ucMon);
            }
            break;
        }

        //
        // Decrement the day
        //
        case FIELD_DAY:
        {
            if(pTime->ucMday > 1)
            {
                pTime->ucMday--;
            }
            break;
        }

        //
        // Decrement the hour
        //
        case FIELD_HOUR:
        {
            if(pTime->ucHour > 0)
            {
                pTime->ucHour--;
            }
            break;
        }

        //
        // Decrement the minute
        //
        case FIELD_MINUTE:
        {
            if(pTime->ucMin > 0)
            {
                pTime->ucMin--;
            }
            break;
        }

        //
        // Bad value for field index - ignore.
        //
        default:
        {
            break;
        }
    }

    //
    // Since something may have been changed in the clock value, request
    // a repaint of the widget.
    //
    WidgetPaint(&pWidget->sBase);

    //
    // Return indication that the button event was handled.
    //
    return(1);
}

//*****************************************************************************
//
//! Handle the LEFT button event.
//!
//! \param pWidget is a pointer to the clock setting widget on which to
//! operate.
//!
//! This function handles the event when the user has pressed the left button.
//! It will change the highlighted field to the previous field.  If it is
//! at the first field in the display, it will wrap around to the last.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static long
ClockSetKeyLeft(tClockSetWidget *pWidget)
{
    //
    // If not already at the minimum, decrement the highlighted field index.
    //
    if(pWidget->ulHighlight)
    {
        pWidget->ulHighlight--;
    }

    //
    // If already at the first field, then reset to the last field.
    //
    else
    {
        pWidget->ulHighlight = FIELD_LAST;
    }

    //
    // The highlighted field changed, so request a repaint of the widget.
    //
    WidgetPaint(&pWidget->sBase);

    //
    // Return indication that the button event was handled.
    //
    return(1);
}

//*****************************************************************************
//
//! Handle the RIGHT button event.
//!
//! \param pWidget is a pointer to the clock setting widget on which to
//! operate.
//!
//! This function handles the event when the user has pressed the right button.
//! It will change the highlighted field to the next field.  If it is already
//! at the last field in the display, it will wrap around to the first.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static long
ClockSetKeyRight(tClockSetWidget *pWidget)
{
    //
    // If not already at the last field, increment the highlighted field index.
    //
    if(pWidget->ulHighlight < FIELD_LAST)
    {
        pWidget->ulHighlight++;
    }

    //
    // If already at the last field, then reset to the first field.
    //
    else
    {
        pWidget->ulHighlight = 0;
    }

    //
    // The highlighted field changed, so request a repaint of the widget.
    //
    WidgetPaint(&pWidget->sBase);

    //
    // Return indication that the button event was handled.
    //
    return(1);
}

//*****************************************************************************
//
//! Handle the select button event.
//!
//! \param pWidget is a pointer to the clock setting widget on which to
//! operate.
//!
//! This function handles the event when the user has pressed the select
//! button.  If either the OK or CANCEL fields is highlighted, then the
//! function will call the callback function to notify the application that
//! an action has been taken and the widget should be dismissed.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static long
ClockSetKeySelect(tClockSetWidget *pWidget)
{
    tBoolean bOk;

    //
    // Determine if the OK text field is highlighted and set a flag.
    //
    bOk = (pWidget->ulHighlight == FIELD_OK) ? true : false;

    //
    // If there is a callback function installed, and either the OK or CANCEL
    // fields is highlighted, then take action.
    //
    if(pWidget->pfnOnOkClick &&
       ((pWidget->ulHighlight == FIELD_OK) || (pWidget->ulHighlight == FIELD_CANCEL)))
    {
        //
        // Call the callback function and pass the flag to indicate if OK
        // was selected (otherwise it was CANCEL).
        //
        pWidget->pfnOnOkClick(&pWidget->sBase, bOk);

        //
        // Set the default highlighted field.  This is the field that will
        // be highlighted the next time this widget is activated.
        //
        pWidget->ulHighlight = FIELD_CANCEL;

        //
        // Return to caller, indicating the button event was handled.
        //
        return(1);
    }

    //
    // There is no callback function, or neither the OK or CANCEL fields is
    // highlighted.  In this case ingore the button event.
    //
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
//! Dispatch button events destined for this widget.
//!
//! \param pWidget is a pointer to the clock setting widget on which to
//! operate.
//! \param ulMsg is the widget message to process.
//!
//! This function receives button/key event messages that are meant for
//! this widget.  It then calls the appropriate function to handle the
//! button event.
//!
//! \return Returns non-zero if the button event was handled, and 0 if the
//! button event was not handled.
//
//*****************************************************************************
static long
ClockSetKeyHandler(tWidget *pWidget, unsigned long ulMsg)
{
    tClockSetWidget *pClockWidget;

    ASSERT(pWidget);

    //
    // Get pointer to the clock setting widget.
    //
    pClockWidget = (tClockSetWidget *)pWidget;

    //
    // Process the key event
    //
    switch(ulMsg)
    {
        //
        // Select key
        //
        case WIDGET_MSG_KEY_SELECT:
        {
            return(ClockSetKeySelect(pClockWidget));
        }

        //
        // Up button
        //
        case WIDGET_MSG_KEY_UP:
        {
            return(ClockSetKeyUp(pClockWidget));
        }

        //
        // Down button
        //
        case WIDGET_MSG_KEY_DOWN:
        {
            return(ClockSetKeyDown(pClockWidget));
        }

        //
        // Left button
        //
        case WIDGET_MSG_KEY_LEFT:
        {
            return(ClockSetKeyLeft(pClockWidget));
        }

        //
        // Right button
        //
        case WIDGET_MSG_KEY_RIGHT:
        {
            return(ClockSetKeyRight(pClockWidget));
        }

        //
        // This is an unexpected event.  Return an indication that the event
        // was not handled.
        //
        default:
        {
            return(0);
        }
    }
}

//*****************************************************************************
//
//! Handles messages for a clock setting  widget.
//!
//! \param pWidget is a pointer to the clock set widget.
//! \param ulMsg is the message.
//! \param ulParam1 is the first parameter to the message.
//! \param ulParam2 is the second parameter to the message.
//!
//! This function receives messages intended for this clock set widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
long
ClockSetMsgProc(tWidget *pWidget, unsigned long ulMsg, unsigned long ulParam1,
                unsigned long ulParam2)
{
    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Determine which message is being sent.
    //
    switch(ulMsg)
    {
        //
        // The widget paint request has been sent.
        //
        case WIDGET_MSG_PAINT:
        {
            //
            // Handle the widget paint request.
            //
            ClockSetPaint(pWidget);

            //
            // Return one to indicate that the message was successfully
            // processed.
            //
            return(1);
        }

        //
        // Process any button/key event messages.
        //
        case WIDGET_MSG_KEY_SELECT:
        case WIDGET_MSG_KEY_UP:
        case WIDGET_MSG_KEY_DOWN:
        case WIDGET_MSG_KEY_LEFT:
        case WIDGET_MSG_KEY_RIGHT:
        {
            //
            // If the key event is for this widget, then process the key event
            //
            if((tWidget *)ulParam1 == pWidget)
            {
                return(ClockSetKeyHandler(pWidget, ulMsg));
            }
        }

        //
        // An unknown request has been sent.
        //
        default:
        {
            //
            // Let the default message handler process this message.
            //
            return(WidgetDefaultMsgProc(pWidget, ulMsg, ulParam1, ulParam2));
        }
    }
}

//*****************************************************************************
//
//! Initializes a clock setting widget.
//!
//! \param pWidget is a pointer to the clock set widget to initialize.
//! \param pDisplay is a pointer to the display on which to draw the widget.
//! \param lX is the X coordinate of the upper left corner of the widget.
//! \param lY is the Y coordinate of the upper left corner of the widget.
//! \param lWidth is the width of the widget.
//! \param lHeight is the height of the widget.
//! \param pFont is the font to use for drawing text on the widget.
//! \param ulForegroundColor is the color of the text and lines on the widget.
//! \param ulBackgroundColor is the color of the widget background.
//! \param psTime is a pointer to the time structure to use for clock fields.
//! \param pfnOnOkClick is a callback function that is called when the user
//! selects the OK field on the display.
//!
//! This function initializes the caller provided clock setting widget.
//!
//! \return None.
//
//*****************************************************************************
void
ClockSetInit(tClockSetWidget *pWidget, const tDisplay *pDisplay,
             long lX, long lY, long lWidth, long lHeight,
             tFont *pFont, unsigned long ulForegroundColor,
              unsigned long ulBackgroundColor, tTime *psTime,
              void (*pfnOnOkClick)(tWidget *pWidget, tBoolean bOk))
{
    unsigned long ulIdx;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);
    ASSERT(pDisplay);

    //
    // Clear out the widget structure.
    //
    for(ulIdx = 0; ulIdx < sizeof(tClockSetWidget); ulIdx += 4)
    {
        ((unsigned long *)pWidget)[ulIdx / 4] = 0;
    }

    //
    // Set the size of the widget structure.
    //
    pWidget->sBase.lSize = sizeof(tClockSetWidget);

    //
    // Mark this widget as fully disconnected.
    //
    pWidget->sBase.pParent = 0;
    pWidget->sBase.pNext = 0;
    pWidget->sBase.pChild = 0;

    //
    // Save the display pointer.
    //
    pWidget->sBase.pDisplay = pDisplay;

    //
    // Set the extents of the display area.
    //
    pWidget->sBase.sPosition.sXMin = lX;
    pWidget->sBase.sPosition.sYMin = lY;
    pWidget->sBase.sPosition.sXMax = lX + lWidth - 1;
    pWidget->sBase.sPosition.sYMax = lY + lHeight - 1;

    //
    // Initialize the widget fields
    //
    pWidget->pFont = pFont;
    pWidget->ulForegroundColor = ulForegroundColor;
    pWidget->ulBackgroundColor = ulBackgroundColor;
    pWidget->psTime = psTime;
    pWidget->pfnOnOkClick = pfnOnOkClick;

    //
    // Use the clock set message handler to process messages to this widget.
    //
    pWidget->sBase.pfnMsgProc = ClockSetMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

