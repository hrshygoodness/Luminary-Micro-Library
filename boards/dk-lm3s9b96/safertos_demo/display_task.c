//*****************************************************************************
//
// display_task.c - Task to display text and images on the LCD.
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
// This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "grlib/grlib.h"
#include "SafeRTOS/SafeRTOS_API.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "display_task.h"
#include "idle_task.h"
#include "priorities.h"

//*****************************************************************************
//
// This structure defines the messages that are sent to the display task.
//
//*****************************************************************************
typedef struct
{
    unsigned long ulType;
    unsigned short usX;
    unsigned short usY;
    const char *pcMessage;
}
tDisplayMessage;

//*****************************************************************************
//
// Defines to indicate the contents of the display message.
//
//*****************************************************************************
#define DISPLAY_IMAGE           1
#define DISPLAY_STRING          2
#define DISPLAY_MOVE            3
#define DISPLAY_DRAW            4

//*****************************************************************************
//
// The item size, queue size, and memory size for the display message queue.
//
//*****************************************************************************
#define DISPLAY_ITEM_SIZE       sizeof(tDisplayMessage)
#define DISPLAY_QUEUE_SIZE      10
#define DISPLAY_MEM_SIZE        ((DISPLAY_ITEM_SIZE * DISPLAY_QUEUE_SIZE) +   \
                                 portQUEUE_OVERHEAD_BYTES)

//*****************************************************************************
//
// A buffer to contain the contents of the display message queue.
//
//*****************************************************************************
static unsigned long g_pulDisplayQueueMem[(DISPLAY_MEM_SIZE + 3) / 4];

//*****************************************************************************
//
// The queue that holds messages sent to the display task.
//
//*****************************************************************************
static xQueueHandle g_pDisplayQueue;

//*****************************************************************************
//
// The stack for the display task.
//
//*****************************************************************************
static unsigned long g_pulDisplayTaskStack[128];

//*****************************************************************************
//
// The most recent position of the display pen.
//
//*****************************************************************************
static unsigned long g_ulDisplayX, g_ulDisplayY;

//*****************************************************************************
//
// This task receives messages from the other tasks and updates the display as
// directed.
//
//*****************************************************************************
static void
DisplayTask(void *pvParameters)
{
    tDisplayMessage sMessage;
    tContext sContext;

    //
    // Initialize the graphics library context.
    //
    GrContextInit(&sContext, &g_sKitronix320x240x16_SSD2119);
    GrContextForegroundSet(&sContext, ClrWhite);
    GrContextBackgroundSet(&sContext, ClrBlack);
    GrContextFontSet(&sContext, &g_sFontFixed6x8);

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Read the next message from the queue.
        //
        if(xQueueReceive(g_pDisplayQueue, &sMessage, portMAX_DELAY) == pdPASS)
        {
            //
            // Determine the message type.
            //
            switch(sMessage.ulType)
            {
                //
                // The drawing of an image has been requested.
                //
                case DISPLAY_IMAGE:
                {
                    //
                    // Draw this image on the display.
                    //
                    GrImageDraw(&sContext, (unsigned char *)sMessage.pcMessage,
                                sMessage.usX, sMessage.usY);

                    //
                    // This message has been handled.
                    //
                    break;
                }

                //
                // The drawing of a string has been requested.
                //
                case DISPLAY_STRING:
                {
                    //
                    // Draw this string on the display.
                    //
                    GrStringDraw(&sContext, sMessage.pcMessage, -1,
                                 sMessage.usX, sMessage.usY, 1);

                    //
                    // This message has been handled.
                    //
                    break;
                }

                //
                // A move of the pen has been requested.
                //
                case DISPLAY_MOVE:
                {
                    //
                    // Save the new pen position.
                    //
                    g_ulDisplayX = sMessage.usX;
                    g_ulDisplayY = sMessage.usY;

                    //
                    // This message has been handled.
                    //
                    break;
                }

                //
                // A draw with the pen has been requested.
                //
                case DISPLAY_DRAW:
                {
                    //
                    // Draw a line from the previous pen position to the new
                    // pen position.
                    //
                    GrLineDraw(&sContext, g_ulDisplayX, g_ulDisplayY,
                               sMessage.usX, sMessage.usY);

                    //
                    // Save the new pen position.
                    //
                    g_ulDisplayX = sMessage.usX;
                    g_ulDisplayY = sMessage.usY;

                    //
                    // This message has been handled.
                    //
                    break;
                }
            }
        }
    }
}

//*****************************************************************************
//
// Sends a request to the display task to draw an image on the display.
//
//*****************************************************************************
void
DisplayImage(unsigned long ulX, unsigned long ulY,
             const unsigned char *pucImage)
{
    tDisplayMessage sMessage;

    //
    // Construct the message to be sent.
    //
    sMessage.ulType = DISPLAY_IMAGE;
    sMessage.usX = ulX;
    sMessage.usY = ulY;
    sMessage.pcMessage = (char *)pucImage;

    //
    // Send the image draw request to the display task.
    //
    xQueueSend(g_pDisplayQueue, &sMessage, portMAX_DELAY);
}

//*****************************************************************************
//
// Sends a request to the display task to draw a string on the display.
//
//*****************************************************************************
void
DisplayString(unsigned long ulX, unsigned long ulY, const char *pcString)
{
    tDisplayMessage sMessage;

    //
    // Construct the message to be sent.
    //
    sMessage.ulType = DISPLAY_STRING;
    sMessage.usX = ulX;
    sMessage.usY = ulY;
    sMessage.pcMessage = pcString;

    //
    // Send the string draw request to the display task.
    //
    xQueueSend(g_pDisplayQueue, &sMessage, portMAX_DELAY);
}

//*****************************************************************************
//
// Sends a request to the display task to move the pen.
//
//*****************************************************************************
void
DisplayMove(unsigned long ulX, unsigned long ulY)
{
    tDisplayMessage sMessage;

    //
    // Construct the message to be sent.
    //
    sMessage.ulType = DISPLAY_MOVE;
    sMessage.usX = ulX;
    sMessage.usY = ulY;

    //
    // Send the pen move request to the display task.
    //
    xQueueSend(g_pDisplayQueue, &sMessage, portMAX_DELAY);
}

//*****************************************************************************
//
// Sends a request to the display task to draw with the pen.
//
//*****************************************************************************
void
DisplayDraw(unsigned long ulX, unsigned long ulY)
{
    tDisplayMessage sMessage;

    //
    // Construct the message to be sent.
    //
    sMessage.ulType = DISPLAY_DRAW;
    sMessage.usX = ulX;
    sMessage.usY = ulY;

    //
    // Send the pen draw request to the display task.
    //
    xQueueSend(g_pDisplayQueue, &sMessage, portMAX_DELAY);
}

//*****************************************************************************
//
// Initializes the display task.
//
//*****************************************************************************
unsigned long
DisplayTaskInit(void)
{
    //
    // Create a queue for sending messages to the display task.
    //
    if(xQueueCreate((signed portCHAR *)g_pulDisplayQueueMem, DISPLAY_MEM_SIZE,
                    DISPLAY_QUEUE_SIZE, DISPLAY_ITEM_SIZE,
                    &g_pDisplayQueue) != pdPASS)
    {
        return(1);
    }

    //
    // Create the display task.
    //
    if(xTaskCreate(DisplayTask, (signed portCHAR *)"Display",
                   (signed portCHAR *)g_pulDisplayTaskStack,
                   sizeof(g_pulDisplayTaskStack), NULL, PRIORITY_DISPLAY_TASK,
                   NULL) != pdPASS)
    {
        return(1);
    }
    TaskCreated();

    //
    // Success.
    //
    return(0);
}
