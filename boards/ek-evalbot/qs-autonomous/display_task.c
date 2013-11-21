//****************************************************************************
//
// display_task.c - Display task for EVALBOT autonomous example.
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
//****************************************************************************

#include "inc/hw_types.h"
#include "drivers/display96x16x1.h"
#include "utils/scheduler.h"

//*****************************************************************************
//
// A bitmap for the Texas Instruments logo type.  The bitmap is as follows:
//
//    123456781234567812345678123456781234567812345678123456781234567812345678
//  0 .xxxxxxxxxxxx..............................................xxxxxx.......
//  1 .xxxxxxxxxxxxx.............................................xxxxx........
//  2 xx....xxx...xx..............................................xxx.........
//  3 x.....xxx..xxxxxxxxx.xxxxxx.xxxxx...xxx.......xxxxx.........xxx..xxx....
//  4 ......xxx..xxxxxxxxx.xxxxx..xxxxx...xxxx....xxxxxxxx........xxx..xxxx...
//  5 ......xxx...xxx...xxx.xxxx...xx....xxxxx....xxx.xxxx........xxx...xxxx..
//  6 ......xxx...xxx.....x..xxxx.xx.....xxxxx...xxx....xx........xxx...xxxxx.
//  7 ......xxx...xxx....x....xxxxxx.....xx.xxx..xxxxx............xxx...xxxxxx
//  0 ......xxx...xxxxxxxx.....xxxx.....xx..xxx...xxxxxxx.........xxx...xx.xxx
//  1 ......xxx...xxxxxxxx.....xxxx.....xx..xxxx...xxxxxxx........xxx...xx..xx
//  2 ......xxx...xxx....x....xxxxxx...xxxxxxxxx.....xxxxxx.......xxx...xx...x
//  3 ......xxx...xxx.........xx.xxx...xxxxxxxxx.xx.....xxx.......xxx...xx....
//  4 ......xxx...xxx.....xx.xx...xxx..xx....xxxxxx.....xxx.......xxx...xx....
//  5 .....xxxxx..xxxxxxxxx.xxx...xxxxxxx.....xxx.xxxxxxxx.......xxxxx..xx....
//  6 .....xxxxx.xxxxxxxxxxxxxx..xxxxxxxx....xxxxxxxxxxxx........xxxxx.xxxx...
//  7 ........................................................................
//
// Continued...
//    123456781234567812345678123456781234567812345678123456781234567812345678
//  0 ........................................................................
//  1 ........................................................................
//  2 ........................................................................
//  3 ..xxxx..xxxx...xxxxxxxxx.xxxxxxx....xxxxx...xxxxxxxx......xxxxxxxxxxxxx.
//  4 ..xxx..xxxxxxx.xxxxxxxxxxxxxxxxxxxx.xxxxx...xxxxxxxxx....xxxx.xxxxxxxxx.
//  5 ..xx.xxx..xxx.xx.xxx.xxx.xxxxxxxxx..xxx.....xx..xxxx....xxxxx..xxx...xxx
//  6 ..xx.xx.....xxx..xxx...x.xxx...xxxx.xxx.....xx..xxxxx...xxxxx..xxx.....x
//  7 ..xx.xxxx........xxx.....xxx...xxxx.xxx.....xx..xxxxx...xxxxx..xxx...xx.
//  0 x.xx.xxxxxxx.....xxx.....xxx..xxxx..xxx.....xx..xx.xxx.xx.xxx..xxxxxxxx.
//  1 xxxx..xxxxxxxx...xxx.....xxxxxxxx...xxx.....xx..xx.xxx.xx.xxx..xxx...xx.
//  2 xxxx.....xxxxx...xxx.....xxx.xxxx...xxx.....xx..xx.xxxxx..xxx..xxx......
//  3 xxxx.x.....xxx...xxx.....xxx..xxxx..xxx.....xx..xx..xxxx..xxx..xxx......
//  4 xxxx.xx... xxx...xxx.....xxx...xxx..xxx.....xx..xx..xxxx..xxx..xxx.....x
//  5 .xxx.xxxxxxxxx...xxx.....xxx...xxxx..xxxxxxxxx..xx....xx..xxx..xxxxxxxxx
//  6 ..xx..xxxxxx....xxxxx...xxxxx...xxxx..xxxxxxx..xxxx...xx.xxxxxxxxxxxxxxx
//  7 ........................................................................
//
// Continued...
//
//    12345678123456781234567812345678
//  0 ................................
//  1 ................................
//  2 ................................
//  3 xxxx....xxxx.xxxxxxxx...xxxx....
//  4 xxxxx...xxxxxxxxxxxxxx.xxxxxxxx.
//  5 .xxxxx...xx.xx. xxx.xx.xxx..xxx.
//  6 .xxxxx...xx.x...xxx..xxxx.....x.
//  7 .xx.xxx..xx.....xxx....xxxx.....
//  0 .xx.xxxx.xx.....xxx....xxxxxxx..
//  1 .xx..xxxxxx.....xxx.....xxxxxxx.
//  2 .xx...xxxxx.....xxx........xxxxx
//  3 .xx....xxxx.....xxx...xx.....xxx
//  4 .xx.....xxx.....xxx...xxx....xxx
//  5 .xx......xx.....xxx....xxxxxxxx.
//  6 xxxx.....xx....xxxxx...xxxxxxx..
//  7 ................................
//
//*****************************************************************************
static const unsigned char g_pucTILogo[] =
{
    //
    // Top Row (blank columns added to left and right edges)
    //
    0x00,
    0x0c, 0x07, 0x03, 0x03, 0x03, 0x03, 0xff, 0xff,
    0xff, 0x03, 0x03, 0x1b, 0xfe, 0xfe, 0xf8, 0x18,
    0x18, 0x18, 0x38, 0xb8, 0x60, 0x18, 0x38, 0x78,
    0xf8, 0xf8, 0xc8, 0x80, 0xd8, 0xf8, 0x38, 0x18,
    0x18, 0x00, 0x00, 0xe0, 0xf8, 0x78, 0xf8, 0xf0,
    0x80, 0x00, 0x00, 0xc0, 0xf0, 0xf0, 0xb8, 0x98,
    0x38, 0x38, 0x78, 0x70, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0x03,
    0x01, 0x18, 0xf8, 0xf8, 0xf0, 0xe0, 0xc0, 0x80,

    0x00, 0x00, 0xf8, 0xf8, 0x18, 0xe8, 0xe0, 0xb0,
    0x98, 0x18, 0x38, 0x38, 0x70, 0x50, 0x60, 0x38,
    0x18, 0xf8, 0xf8, 0xf8, 0x18, 0x38, 0x38, 0x78,
    0x10, 0xf8, 0xf8, 0xf8, 0x38, 0x38, 0x38, 0xf8,
    0xf0, 0xf0, 0xd0, 0x00, 0xf8, 0xf8, 0xf8, 0x18,
    0x18, 0x00, 0x00, 0x00, 0xf8, 0xf8, 0x18, 0x18,
    0xf8, 0xf8, 0xf8, 0xf8, 0xd0, 0x00, 0x00, 0x00,
    0xe0, 0xf0, 0xf8, 0xf8, 0xf8, 0x08, 0x18, 0xf8,
    0xf8, 0xf8, 0x18, 0x18, 0x18, 0xb8, 0xb8, 0x60,

    0x18, 0xf8, 0xf8, 0x78, 0xf0, 0xe0, 0x80, 0x00,
    0x18, 0xf8, 0xf8, 0x18, 0x70, 0x38, 0x18, 0x18,
    0xf8, 0xf8, 0xf8, 0x18, 0x38, 0x70, 0x40, 0xf0,
    0xf8, 0xb8, 0x98, 0x18, 0x38, 0x30, 0x70, 0x00,
    0x00,

    //
    // Second row (blank columns added to left and right edges)
    //
    0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x7f, 0x7f,
    0x7f, 0x60, 0x00, 0x40, 0x7f, 0x7f, 0x7f, 0x63,
    0x63, 0x63, 0x63, 0x67, 0x70, 0x50, 0x60, 0x70,
    0x7c, 0x0f, 0x07, 0x4f, 0x7f, 0x7c, 0x70, 0x60,
    0x60, 0x7c, 0x7f, 0x0f, 0x0c, 0x0c, 0x0f, 0x5f,
    0x7f, 0x7e, 0x70, 0x58, 0x79, 0x63, 0x63, 0x67,
    0x67, 0x67, 0x7f, 0x3e, 0x1c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x60, 0x7f, 0x7f, 0x7f, 0x60,
    0x00, 0x40, 0x7f, 0x7f, 0x40, 0x01, 0x03, 0x07,

    0x1f, 0x3e, 0x7f, 0x7f, 0x00, 0x39, 0x73, 0x63,
    0x63, 0x67, 0x67, 0x7f, 0x3e, 0x3e, 0x00, 0x00,
    0x40, 0x7f, 0x7f, 0x7f, 0x40, 0x00, 0x00, 0x00,
    0x40, 0x7f, 0x7f, 0x7f, 0x42, 0x06, 0x0f, 0x3f,
    0x7f, 0x79, 0x60, 0x40, 0x1f, 0x3f, 0x7f, 0x60,
    0x60, 0x60, 0x60, 0x60, 0x7f, 0x3f, 0x00, 0x40,
    0x7f, 0x7f, 0x40, 0x07, 0x1f, 0x1f, 0x7c, 0x7f,
    0x03, 0x40, 0x7f, 0x7f, 0x7f, 0x40, 0x40, 0x7f,
    0x7f, 0x7f, 0x61, 0x61, 0x61, 0x63, 0x63, 0x70,

    0x40, 0x7f, 0x7f, 0x40, 0x01, 0x03, 0x07, 0x0f,
    0x0e, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x40,
    0x7f, 0x7f, 0x7f, 0x40, 0x00, 0x00, 0x18, 0x79,
    0x73, 0x63, 0x61, 0x63, 0x63, 0x7f, 0x3e, 0x1c,
    0x00
};

//*****************************************************************************
//
// Frame buffer used for rendering the display content before being sent to
// the display.
//
//*****************************************************************************
static unsigned char g_pucFrame[192];

//*****************************************************************************
//
// Scrolls a wide image across the display.  The first time this function is
// called, the width of the image must be specified in the ulImageWidth
// parameter.  This initializes the image for scrolling.  Then, this function
// should be called successively with the ulImageWidth parameter set to 0.
// This causes the image to advance by one pixel across the screen each time
// it is called.  By calling this function periodically the image can be made
// to appear to smoothly scroll across the screen.
//
// The image passed should be an image that is suitable to pass to the display
// driver "ImageDraw" function.
//
// As long as the image has not scrolled off the screen, this function will
// return a non-zero value.  Once the image has completely scrolled off the
// screen, it will return 0.
//
//*****************************************************************************
static unsigned long
ScrollImage(const unsigned char *pucImage, unsigned long ulImageWidth)
{
    unsigned long ulLoop;
    static unsigned long ulIdx = 0;
    static unsigned long ulWidth;

    //
    // If an image width is specified, then initialize the scrolling image
    // display variables.
    //
    if(ulImageWidth)
    {
        ulWidth = ulImageWidth;
        ulIdx = 0;
    }

    //
    // Otherwise, the ulImageWidth parameter was 0, which means to advance
    // the image across the screen by one pixel.
    //
    else
    {
        //
        // Clear the local frame buffer.
        //
        for(ulLoop = 0; ulLoop < 192; ulLoop += 4)
        {
            *(unsigned long *)(g_pucFrame + ulLoop) = 0;
        }

        //
        // See if the image has reached the left side of the display.
        //
        if(ulIdx <= 96)
        {
            //
            // Copy the first N columns of the image to the right side of the
            // local frame buffer.
            //
            for(ulLoop = 0; ulLoop < ulIdx; ulLoop++)
            {
                g_pucFrame[ulLoop + 96 - ulIdx] = pucImage[ulLoop];
                g_pucFrame[ulLoop + 96 - ulIdx + 96] =
                    pucImage[ulLoop + ulWidth];
            }
        }

        //
        // See if the right side of the image has reached the right side of the
        // display.
        //
        else if(ulIdx < ulWidth)
        {
            //
            // Copy 96 columns from the middle of the image to the local frame
            // buffer.
            //
            for(ulLoop = 0; ulLoop < 96; ulLoop++)
            {
                g_pucFrame[ulLoop] = pucImage[ulLoop + ulIdx - 96];
                g_pucFrame[ulLoop + 96] =
                    pucImage[ulLoop + ulIdx - 96 + ulWidth];
            }
        }

        //
        // Otherwise, the right side of the image has already reached the right
        // side of the display.
        //
        else
        {
            //
            // Copy the right N columns of the image to the left side of the
            // local frame buffer.
            //
            for(ulLoop = 0; ulLoop < (ulWidth + 96 - ulIdx); ulLoop++)
            {
                g_pucFrame[ulLoop] = pucImage[ulLoop + ulIdx - 96];
                g_pucFrame[ulLoop + 96] =
                    pucImage[ulLoop + ulIdx - 96 + ulWidth];
            }
        }

        //
        // Display the local frame buffer on the display.
        //
        Display96x16x1ImageDraw(g_pucFrame, 0, 0, 96, 2);
    }

    //
    // Increment the scrolling column index.
    //
    ulIdx++;

    //
    // Return 1 if the image is still scrolling, 0 if done
    //
    return((ulIdx > (ulWidth + 96)) ? 1 : 0);
}
//*****************************************************************************
//
// This function is the display "task" that is called periodically by the
// Scheduler from the application main processing loop.
// This function displays a cycle of several messages on the display.
//
// Odd values are used for timeouts for some of the displayed messaged.  For
// example a message may be shown for 5.3 seconds.  This is done to keep the
// changes on the display out of sync with the LED blinking which occurs
// once per second.
//
//*****************************************************************************
void
DisplayTask(void *pvParam)
{
    static unsigned long ulState = 0;
    static unsigned long ulLastTick = 0;
    static unsigned long ulTimeout = 0;

    //
    // Check to see if the timeout has expired and it is time to change the
    // display.
    //
    if(SchedulerElapsedTicksGet(ulLastTick) > ulTimeout)
    {
        //
        // Save the current tick value to use for comparison next time through
        //
        ulLastTick = SchedulerTickCountGet();

        //
        // Switch based on the display task state
        //
        switch(ulState)
        {
            //
            // In this state, the scrolling TI logo is initialized, and the
            // state changed to next state.  A short timeout is used so that
            // the scrolling image will start next time through this task.
            //
            case 0:
            {
                ScrollImage(g_pucTILogo, sizeof(g_pucTILogo) / 2);
                ulTimeout = 1;
                ulState = 1;
                break;
            }

            //
            // In this state the TI logo is scrolled across the screen with
            // a scroll rate of this task period (each time this task is
            // called by the scheduler it advances the scroll by one pixel
            // column).
            //
            case 1:
            {
                if(ScrollImage(g_pucTILogo, 0))
                {
                    //
                    // If the scrolling is done, then advance the state and
                    // set the timeout for 1.3 seconds (next state will start
                    // in 1.3 seconds).
                    //
                    ulTimeout = 130;
                    ulState = 2;
                }
                break;
            }

            //
            // This state shows the text "STELLARIS" on the display for 5.3
            // seconds.
            //
            case 2:
            {
                Display96x16x1StringDraw("STELLARIS", 21, 0);
                ulTimeout = 530;
                ulState = 3;
                break;
            }

            //
            // This state clears the screen for 1.3 seconds.
            //
            case 3:
            {
                Display96x16x1Clear();
                ulTimeout = 130;
                ulState = 4;
                break;
            }

            //
            // This state shows "EVALBOT" for 5.3 seconds.
            //
            case 4:
            {
                Display96x16x1StringDraw("EVALBOT", 27, 0);
                ulTimeout = 530;
                ulState = 5;
                break;
            }

            //
            // This state clears the screen for 1.3 seconds.
            //
            case 5:
            {
                Display96x16x1Clear();
                ulTimeout = 130;
                ulState = 0;
                break;
            }

            //
            // The default state should not occur, but if it does, then reset
            // back to the beginning state.
            //
            default:
            {
                ulTimeout = 130;
                ulState = 0;
                break;
            }
        }
    }
}

//*****************************************************************************
//
// This function initializes the display task.  It should be called from the
// application initialization code.
//
//*****************************************************************************
void
DisplayTaskInit(void *pvParam)
{
    //
    // Initialize the display driver and turn the display on.
    //
    Display96x16x1Init(false);
    Display96x16x1DisplayOn();
}
