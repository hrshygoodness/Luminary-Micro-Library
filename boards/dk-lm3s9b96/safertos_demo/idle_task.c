//*****************************************************************************
//
// idle_task.c - The SafeRTOS idle task.
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
// This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#include "SafeRTOS/SafeRTOS_API.h"
#include "utils/lwiplib.h"
#include "lwip/stats.h"
#include "display_task.h"
#include "idle_task.h"

//*****************************************************************************
//
// The stack for the idle task.
//
//*****************************************************************************
unsigned long g_pulIdleTaskStack[128];

//*****************************************************************************
//
// The number of tasks that are running.
//
//*****************************************************************************
static unsigned long g_ulTasks;

//*****************************************************************************
//
// The number of tasks that existed the last time the display was updated (used
// to detect when the display should be updated again).
//
//*****************************************************************************
static unsigned long g_ulPreviousTasks;

//*****************************************************************************
//
// The number of seconds that the application has been running.  This is
// initialized to -1 in order to get the initial display updated as soon as
// possible.
//
//*****************************************************************************
static unsigned long g_ulSeconds = 0xffffffff;

//*****************************************************************************
//
// The current IP address.  This is initialized to -1 in order to get the
// initial display updated as soon as possible.
//
//*****************************************************************************
static unsigned long g_ulIPAddress = 0xffffffff;

//*****************************************************************************
//
// The number of packets that have been transmitted.  This is initialized to -1
// in order to get the initial display updated as soon as possible.
//
//*****************************************************************************
static unsigned long g_ulTXPackets = 0xffffffff;

//*****************************************************************************
//
// The number of packets that have been received.  This is initialized to -1 in
// order to get the initial display updated as soon as possible.
//
//*****************************************************************************
static unsigned long g_ulRXPackets = 0xffffffff;

//*****************************************************************************
//
// A buffer to contain the string versions of the information displayed at the
// bottom of the display.
//
//*****************************************************************************
static char g_pcTimeString[12];
static char g_pcTaskString[4];
static char g_pcIPString[24];
static char g_pcTxString[8];
static char g_pcRxString[8];

//*****************************************************************************
//
// This function is called by the application whenever it creates a task.
//
//*****************************************************************************
void
TaskCreated(void)
{
    //
    // Increment the count of running tasks.
    //
    g_ulTasks++;
}

//*****************************************************************************
//
// This hook is called by SafeRTOS when a task is deleted.
//
//*****************************************************************************
void
SafeRTOSTaskDeleteHook(xTaskHandle xTaskToDelete)
{
    //
    // Decrement the count of running tasks.
    //
    g_ulTasks--;
}

//*****************************************************************************
//
// Displays the IP address in a human-readable format.
//
//*****************************************************************************
static void
DisplayIP(unsigned long ulIP)
{
    unsigned long ulLoop, ulIdx, ulValue;

    //
    // If there is no IP address, indicate that one is being acquired.
    //
    if(ulIP == 0)
    {
        DisplayString(115, 240 - 10, "  Acquiring...  ");
        return;
    }

    //
    // Set the initial index into the string that is being constructed.
    //
    ulIdx = 0;

    //
    // Start the string with four spaces.  Not all will necessarily be used,
    // depending upon the length of the IP address string.
    //
    for(ulLoop = 0; ulLoop < 4; ulLoop++)
    {
        g_pcIPString[ulIdx++] = ' ';
    }

    //
    // Loop through the four bytes of the IP address.
    //
    for(ulLoop = 0; ulLoop < 32; ulLoop += 8)
    {
        //
        // Extract this byte from the IP address word.
        //
        ulValue = (ulIP >> ulLoop) & 0xff;

        //
        // Convert this byte into ASCII, using only the characters required.
        //
        if(ulValue > 99)
        {
            g_pcIPString[ulIdx++] = '0' + (ulValue / 100);
        }
        if(ulValue > 9)
        {
            g_pcIPString[ulIdx++] = '0' + ((ulValue / 10) % 10);
        }
        g_pcIPString[ulIdx++] = '0' + (ulValue % 10);

        //
        // Add a dot to separate this byte from the next.
        //
        g_pcIPString[ulIdx++] = '.';
    }

    //
    // Fill the remainder of the string buffer with spaces.
    //
    for(ulLoop = ulIdx - 1; ulLoop < 20; ulLoop++)
    {
        g_pcIPString[ulLoop] = ' ';
    }

    //
    // Null terminate the string at the appropriate place, based on the length
    // of the string version of the IP address.  There may or may not be
    // trailing spaces that remain.
    //
    g_pcIPString[ulIdx + 3 - ((ulIdx - 12) / 2)] = '\0';

    //
    // Display the string.  The horizontal position and the number of leading
    // spaces utilized depend on the length of the string version of the IP
    // address.  The end result is the IP address centered in the provided
    // space with leading/trailing spaces as required to clear the remainder of
    // the space.
    //
    DisplayString(118 - ((ulIdx & 1) * 3), 240 - 10,
                  g_pcIPString + ((ulIdx - 12) / 2));
}

//*****************************************************************************
//
// Displays a value in a human-readable format.  This does not need to deal
// with leading/trailing spaces sicne the values displayed are monotonically
// increasing.
//
//*****************************************************************************
static void
DisplayValue(char *pcBuffer, unsigned long ulValue, unsigned long ulX,
             unsigned long ulY)
{
    //
    // See if the value is less than 10.
    //
    if(ulValue < 10)
    {
        //
        // Display the value using only a single digit.
        //
        pcBuffer[0] = '0' + ulValue;
        pcBuffer[1] = '\0';
        DisplayString(ulX + 15, ulY, pcBuffer);
    }

    //
    // Otherwise, see if the value is less than 100.
    //
    else if(ulValue < 100)
    {
        //
        // Display the value using two digits.
        //
        pcBuffer[0] = '0' + (ulValue / 10);
        pcBuffer[1] = '0' + (ulValue % 10);
        pcBuffer[2] = '\0';
        DisplayString(ulX + 12, ulY, pcBuffer);
    }

    //
    // Otherwise, see if the value is less than 1,000.
    //
    else if(ulValue < 1000)
    {
        //
        // Display the value using three digits.
        //
        pcBuffer[0] = '0' + (ulValue / 100);
        pcBuffer[1] = '0' + ((ulValue / 10) % 10);
        pcBuffer[2] = '0' + (ulValue % 10);
        pcBuffer[3] = '\0';
        DisplayString(ulX + 9, ulY, pcBuffer);
    }

    //
    // Otherwise, see if the value is less than 10,000.
    //
    else if(ulValue < 10000)
    {
        //
        // Display the value using four digits.
        //
        pcBuffer[0] = '0' + (ulValue / 1000);
        pcBuffer[1] = '0' + ((ulValue / 100) % 10);
        pcBuffer[2] = '0' + ((ulValue / 10) % 10);
        pcBuffer[3] = '0' + (ulValue % 10);
        pcBuffer[4] = '\0';
        DisplayString(ulX + 6, ulY, pcBuffer);
    }

    //
    // Otherwise, see if the value is less than 100,000.
    //
    else if(ulValue < 100000)
    {
        //
        // Display the value using five digits.
        //
        pcBuffer[0] = '0' + (ulValue / 10000);
        pcBuffer[1] = '0' + ((ulValue / 1000) % 10);
        pcBuffer[2] = '0' + ((ulValue / 100) % 10);
        pcBuffer[3] = '0' + ((ulValue / 10) % 10);
        pcBuffer[4] = '0' + (ulValue % 10);
        pcBuffer[5] = '\0';
        DisplayString(ulX + 3, ulY, pcBuffer);
    }

    //
    // Otherwise, the value is between 100,000 and 999,999.
    //
    else
    {
        //
        // Display the value using six digits.
        //
        pcBuffer[0] = '0' + ((ulValue / 100000) % 10);
        pcBuffer[1] = '0' + ((ulValue / 10000) % 10);
        pcBuffer[2] = '0' + ((ulValue / 1000) % 10);
        pcBuffer[3] = '0' + ((ulValue / 100) % 10);
        pcBuffer[4] = '0' + ((ulValue / 10) % 10);
        pcBuffer[5] = '0' + (ulValue % 10);
        pcBuffer[6] = '\0';
        DisplayString(ulX, ulY, pcBuffer);
    }
}

//*****************************************************************************
//
// This hook is called by the SafeRTOS idle task when no other tasks are
// runnable.
//
//*****************************************************************************
void
SafeRTOSIdleHook(void)
{
    unsigned long ulTemp;

    //
    // See if this is the first time that the idle task has been called.
    //
    if(g_ulSeconds == 0xffffffff)
    {
        //
        // Draw the boxes for the statistics that are displayed.
        //
        DisplayMove(0, 240 - 20);
        DisplayDraw(319, 240 - 20);
        DisplayDraw(319, 239);
        DisplayDraw(0, 239);
        DisplayDraw(0, 240 - 20);
        DisplayMove(64, 240 - 20);
        DisplayDraw(64, 239);
        DisplayMove(110, 240 - 20);
        DisplayDraw(110, 239);
        DisplayMove(215, 240 - 20);
        DisplayDraw(215, 239);
        DisplayMove(267, 240 - 20);
        DisplayDraw(267, 239);

        //
        // Place the statistics titles in the boxes.
        //
        DisplayString(14, 240 - 18, "Uptime");
        DisplayString(72, 240 - 18, "Tasks");
        DisplayString(133, 240 - 18, "IP Address");
        DisplayString(235, 240 - 18, "TX");
        DisplayString(287, 240 - 18, "RX");
    }

    //
    // Get the number of seconds that the application has been running.
    //
    ulTemp = xTaskGetTickCount() / (1000 / portTICK_RATE_MS);

    //
    // See if the number of seconds has changed.
    //
    if(ulTemp != g_ulSeconds)
    {
        //
        // Update the local copy of the run time.
        //
        g_ulSeconds = ulTemp;

        //
        // Convert the number of seconds into a text string.
        //
        g_pcTimeString[0] = '0' + ((ulTemp / 36000) % 10);
        g_pcTimeString[1] = '0' + ((ulTemp / 3600) % 10);
        g_pcTimeString[2] = ':';
        g_pcTimeString[3] = '0' + ((ulTemp / 600) % 6);
        g_pcTimeString[4] = '0' + ((ulTemp / 60) % 10);
        g_pcTimeString[5] = ':';
        g_pcTimeString[6] = '0' + ((ulTemp / 10) % 6);
        g_pcTimeString[7] = '0' + (ulTemp % 10);
        g_pcTimeString[8] = '\0';

        //
        // Have the display task write this string onto the display.
        //
        DisplayString(8, 240 - 10, g_pcTimeString);
    }

    //
    // See if the number of tasks has changed.
    //
    if(g_ulTasks != g_ulPreviousTasks)
    {
        //
        // Update the local copy of the number of tasks.
        //
        ulTemp = g_ulPreviousTasks = g_ulTasks;

        //
        // Convert the number of tasks into a text string and display it.
        //
        if(ulTemp < 10)
        {
            g_pcTaskString[0] = ' ';
            g_pcTaskString[1] = '0' + (ulTemp % 10);
            g_pcTaskString[2] = ' ';
            g_pcTaskString[3] = '\0';
            DisplayString(78, 240 - 10, g_pcTaskString);
        }
        else
        {
            g_pcTaskString[0] = '0' + ((ulTemp / 10) % 10);
            g_pcTaskString[1] = '0' + (ulTemp % 10);
            g_pcTaskString[2] = '\0';
            DisplayString(81, 240 - 10, g_pcTaskString);
        }
    }

    //
    // Get the current IP address.
    //
    ulTemp = lwIPLocalIPAddrGet();

    //
    // See if the IP address has changed.
    //
    if(ulTemp != g_ulIPAddress)
    {
        //
        // Save the current IP address.
        //
        g_ulIPAddress = ulTemp;

        //
        // Update the display of the IP address.
        //
        DisplayIP(ulTemp);
    }

    //
    // See if the number of transmitted packets has changed.
    //
    if(lwip_stats.link.xmit != g_ulTXPackets)
    {
        //
        // Save the number of transmitted packets.
        //
        ulTemp = g_ulTXPackets = lwip_stats.link.xmit;

        //
        // Update the display of transmitted packets.
        //
        DisplayValue(g_pcTxString, ulTemp, 223, 240 - 10);
    }

    //
    // See if the number of received packets has changed.
    //
    if(lwip_stats.link.recv != g_ulRXPackets)
    {
        //
        // Save the number of received packets.
        //
        ulTemp = g_ulRXPackets = lwip_stats.link.recv;

        //
        // Update the display of received packets.
        //
        DisplayValue(g_pcRxString, ulTemp, 275, 240 - 10);
    }
}
