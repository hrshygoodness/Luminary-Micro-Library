//*****************************************************************************
//
// update.c - Displays the "Firmware Update" panel.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "utils/ustdlib.h"
#include "boot_loader/bl_commands.h"
#include "shared/can_proto.h"
#include "bdc-ui.h"
#include "can_comm.h"
#include "menu.h"
#include "rit128x96x4.h"
#include "update.h"

//*****************************************************************************
//
// The current state of the UART update interpreter.
//
//*****************************************************************************
#define UART_STATE_IDLE         0
#define UART_STATE_HAVE_SIZE    1
#define UART_STATE_HAVE_CKSUM   2
#define UART_STATE_ACK_NAK      3
static unsigned long g_ulUARTState = UART_STATE_IDLE;

//*****************************************************************************
//
// A buffer to contain a packet of data received from the UART interface.
//
//*****************************************************************************
static unsigned char g_pucUpdateData[260];

//*****************************************************************************
//
// The size of the packet in g_pucUpdateData.
//
//*****************************************************************************
static unsigned long g_ulUpdateSize;

//*****************************************************************************
//
// The checksum of the packet in g_pucUpdateData.
//
//*****************************************************************************
static unsigned long g_ulUpdateCheckSum;

//*****************************************************************************
//
// The index into g_pucUpdateData where the next byte received from the UART
// will be written.
//
//*****************************************************************************
static unsigned long g_ulUpdateIdx;

//*****************************************************************************
//
// The status of the most recent command handled by the UART update.
//
//*****************************************************************************
static unsigned long g_ulUpdateStatus;

//*****************************************************************************
//
// The offset into flash where the next data will be programmed.
//
//*****************************************************************************
static unsigned long g_ulImageOffset;

//*****************************************************************************
//
// The size of the image to be programmed into flash.
//
//*****************************************************************************
static unsigned long g_ulImageSize;

//*****************************************************************************
//
// A buffer to contain the first three words of the firmware image; these are
// the image size, initial stack pointer, and reset vector pointer.  They are
// stored in this buffer and programmed after the remainder of the image is
// programmed.  This way, if the UART update is interrupted mid way through,
// these words will not be written and the in-flash image will be considered
// corrupt and therefore not used for the subsequent CAN update (which will not
// be allowed to occur).
//
//*****************************************************************************
static unsigned long g_pulFirstWords[3];

//*****************************************************************************
//
// Buffers to contain the string representation of the current device ID and
// the firmware version, along with an update status message.
//
//*****************************************************************************
static char g_pcIDBuffer[4];
static char g_pcVersionBuffer[16];
static char g_pcMessageBuffer[24];

//*****************************************************************************
//
// The widgets that make up the "Firmware Update" panel.
//
//*****************************************************************************
static tCanvasWidget g_psUpdateWidgets[] =
{
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 0, 128, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 "Firmware Update", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 24, 24, 18, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 g_pcIDBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 48, 36, 8,
                 CANVAS_STYLE_TEXT, ClrSelected, 0, ClrWhite, g_pFontFixed6x8,
                 "Start", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 12, 128, 1,
                 CANVAS_STYLE_FILL, ClrWhite, 0, 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 24, 18, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8, "ID:", 0,
                 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 36, 48, 8,
                 CANVAS_STYLE_TEXT, 0, 0, ClrWhite, g_pFontFixed6x8,
                 "Version:", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 54, 36, 48, 8,
                 CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT, 0, 0, ClrWhite,
                 g_pFontFixed6x8, g_pcVersionBuffer, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 68, 128, 8,
                 CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_LEFT, 0, 0, ClrWhite,
                 g_pFontFixed6x8, g_pcMessageBuffer, 0, 0)
};

//*****************************************************************************
//
// The number of widgets in the "Firmware Update" panel.
//
//*****************************************************************************
#define NUM_UPDATE_WIDGETS      (sizeof(g_psUpdateWidgets) /                  \
                                 sizeof(g_psUpdateWidgets[0]))

//*****************************************************************************
//
// The widgets that make up the update progress bar.
//
//*****************************************************************************
static tCanvasWidget g_psProgressWidgets[] =
{
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 68, 128, 8,
                 CANVAS_STYLE_TEXT | CANVAS_STYLE_FILL, ClrBlack, 0, ClrWhite,
                 g_pFontFixed6x8, "", 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 0, 76, 128, 7,
                 CANVAS_STYLE_OUTLINE | CANVAS_STYLE_FILL, ClrBlack, ClrWhite,
                 0, 0, 0, 0, 0),
    CanvasStruct(0, 0, 0, &g_sRIT128x96x4Display, 1, 77, 1, 5,
                 CANVAS_STYLE_FILL, ClrSelected, 0, 0, 0, 0, 0, 0)
};

//*****************************************************************************
//
// The number of widgets in the update progress bar.
//
//*****************************************************************************
#define NUM_PROGRESS_WIDGETS    (sizeof(g_psProgressWidgets) /                \
                                 sizeof(g_psProgressWidgets[0]))

//*****************************************************************************
//
// The offsets of the title and the progress bar in the list of update progress
// bar widgets.
//
//*****************************************************************************
#define PROGRESS_TITLE          0
#define PROGRESS_BAR            2

//*****************************************************************************
//
// The number of milliseconds to wait for a new UART packet before the serial
// download is aborted.
//
//*****************************************************************************
#define SERIAL_UPDATE_TIMEOUT   3000

//*****************************************************************************
//
// The base address of the update image in local flash.
//
//*****************************************************************************
#define IMAGE_BASE              0x00020000

//*****************************************************************************
//
// This function updates the position of the progress bar.
//
//*****************************************************************************
#define PROGRESS(ulProgress)                                                  \
        g_psProgressWidgets[PROGRESS_BAR].sBase.sPosition.sXMax =             \
            (g_psProgressWidgets[PROGRESS_BAR].sBase.sPosition.sXMin +        \
             ((ulProgress * 125) / 100))

//*****************************************************************************
//
// This function delays for the specified number of milliseconds.
//
//*****************************************************************************
static void
Delay(unsigned long ulNumMS)
{
    //
    // Loop one time per millisecond.
    //
    for(; ulNumMS != 0; ulNumMS--)
    {
        //
        // Clear the tick flag.
        //
        HWREGBITW(&g_ulFlags, FLAG_TICK) = 0;

        //
        // Wait until the tick flag is set.
        //
        while(HWREGBITW(&g_ulFlags, FLAG_TICK) == 0)
        {
        }
    }
}

//*****************************************************************************
//
// This function delays for the specified number of milliseconds, aborting the
// delay if the CAN update ACK is received.
//
//*****************************************************************************
static unsigned long
DelayAck(unsigned long ulNumMS)
{
    //
    // Loop one time per millisecond while the CAN update ACK has not been
    // received.
    //
    for(; (ulNumMS != 0) && (g_ulCANUpdateAck == 0); ulNumMS--)
    {
        //
        // Clear the tick flag.
        //
        HWREGBITW(&g_ulFlags, FLAG_TICK) = 0;

        //
        // Wait until the tick flag is set or the the CAN update ACK has been
        // received.
        //
        while((HWREGBITW(&g_ulFlags, FLAG_TICK) == 0) &&
              (g_ulCANUpdateAck == 0))
        {
        }

        //
        // If the CAN update ACK was received, return and indication of that
        // fact.
        //
        if(g_ulCANUpdateAck == 1)
        {
            return(1);
        }
    }

    //
    // The delay completed before the CAN update ACK was received.
    //
    return(0);
}

//*****************************************************************************
//
// This function changes the text color of the "Firmware Update" panel widgets
// that are not part of the title bar across the top of the screen.
//
//*****************************************************************************
static void
UpdateWidgetsColor(unsigned long ulColor)
{
    unsigned long ulIdx;

    //
    // Loop through the "Firmware Update" panel widgets.
    //
    for(ulIdx = 0; ulIdx < NUM_UPDATE_WIDGETS; ulIdx++)
    {
        //
        // Update the text color of this widget if it is not part of the title
        // bar.
        //
        if(g_psUpdateWidgets[ulIdx].sBase.sPosition.sYMin >= 16)
        {
            CanvasTextColorSet(g_psUpdateWidgets + ulIdx, ulColor);
        }
    }
}

//*****************************************************************************
//
// This function performs a CAN-based firmware update of a motor controller,
// displaying progress on the screen.
//
//*****************************************************************************
static void
CANUpdate(void)
{
    unsigned long ulIdx, ulLoop, *pulData, ulSize;

    //
    // Get a pointer to the local firmware image.
    //
    pulData = (unsigned long *)IMAGE_BASE;

    //
    // Get the size of the firmware image.
    //
    ulSize = *pulData++;

    //
    // Verify that the firmware image size is valid, the initial stack pointer
    // is valid, and the reset vector pointer is valid.
    //
    if((ulSize > 0x1f000) || ((pulData[0] & 0xffff0003) != 0x20000000) ||
       ((pulData[1] & 0xfff00001) != 0x00000001))
    {
        //
        // Indicate that the firmware image is not valid and return without
        // performing a firmware update.
        //
        usnprintf(g_pcMessageBuffer, sizeof(g_pcMessageBuffer),
                  "Invalid update image!");
        return;
    }

    //
    // Set the progress title to indicate that the motor controller is being
    // updated.
    //
    CanvasTextSet(g_psProgressWidgets + PROGRESS_TITLE, "Updating MDL-BDC");

    //
    // Add the progress bar widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_PROGRESS_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psProgressWidgets + ulIdx));
    }

    //
    // Gray the non-title widgets in the main panel.
    //
    UpdateWidgetsColor(ClrNotPresent);

    //
    // Set the progress bar to the beginning.
    //
    PROGRESS(0);

    //
    // Update the display.
    //
    DisplayFlush();

    //
    // Set the default message to a failure message.  This will be replaced
    // with a success message if the update is successful.
    //
    usnprintf(g_pcMessageBuffer, sizeof(g_pcMessageBuffer),
              "Update of %d failed!", g_ulCurrentID);

    //
    // A simple do/while(0) loop that is executed once (because of the while(0)
    // at the end) but can be easily exited with a break.  This allows a common
    // cleanup at the end (after the loop) while not having increasingly deep
    // nesting of the intervening code.
    //
    do
    {
        //
        // Tell the motor controller to enter firmware update mode.
        //
        CANUpdateStart();

        //
        // Delay for 50 milliseconds to allow it time to shut down and enter
        // the boot loader.
        //
        Delay(50);

        //
        // Try to ping the boot loader.
        //
        for(ulLoop = 10; (ulLoop != 0) && (g_ulCANUpdateAck == 0); ulLoop--)
        {
            //
            // Send a ping command.
            //
            g_ulCANUpdateAck = 0;
            CANUpdatePing();

            //
            // Delay up to 10 milliseconds waiting for the ACK.
            //
            DelayAck(10);
        }

        //
        // Abort the firmware update if the ACK was not received.
        //
        if(ulLoop == 0)
        {
            break;
        }

        //
        // Delay for 50 milliseconds.  If there were some CAN network delays,
        // it is possible that two pings were sent with the ACK from the first
        // ping being received right after the second ping was sent.  In this
        // case, this delay will allow time for the second ACK to be received
        // and processed; failure to do so would cause the remaining actions to
        // become out of sync.
        //
        Delay(50);

        //
        // Send the download command, causing the appropriate portion of flash
        // to be erased.
        //
        g_ulCANUpdateAck = 0;
        CANUpdateDownload(0x800, ulSize);

        //
        // Delay for up to 4 seconds waiting for the ACK.  Since flash is being
        // erased, it could take up to ~2.5 seconds (depending on the size of
        // the firmware image).
        //
        if(DelayAck(4000) == 0)
        {
            break;
        }

        //
        // Loop over the data in the firmware image.
        //
        for(ulLoop = 0; ulLoop < ulSize; ulLoop += 8)
        {
            //
            // Send the next two words of the firmware image.
            //
            g_ulCANUpdateAck = 0;
            CANUpdateSendData(((ulSize - ulLoop) > 8) ? 8 : ulSize - ulLoop,
                              pulData[0], pulData[1]);
            pulData += 2;

            //
            // Delay for up to 10 milliseconds waiting for the ACK.  Since
            // flash is being programming, it could take up to ~44
            // microseconds.
            //
            if(DelayAck(10) == 0)
            {
                break;
            }

            //
            // Update the progress bar and update the screen if the progress
            // bar has moved.
            //
            ulIdx = g_psProgressWidgets[PROGRESS_BAR].sBase.sPosition.sXMax;
            PROGRESS((ulLoop * 100) / ulSize);
            if(g_psProgressWidgets[PROGRESS_BAR].sBase.sPosition.sXMax !=
               ulIdx)
            {
                DisplayFlush();
            }
        }

        //
        // Abort the firmware update if the entire image was not programmed.
        //
        if(ulLoop < ulSize)
        {
            break;
        }

        //
        // Reset the motor controller.
        //
        CANUpdateReset();

        //
        // Delay for 500 milliseconds while the motor controller is being
        // reset.
        //
        Delay(500);

        //
        // Indicate that the firmware update was successful.
        //
        usnprintf(g_pcMessageBuffer, sizeof(g_pcMessageBuffer),
                  "Device %d updated.", g_ulCurrentID);

        //
        // Set the device ID so that any default configuration can be
        // performed.
        //
        CANSetID(g_ulCurrentID);
    }
    while(0);

    //
    // Remove the progress bar widgets.
    //
    for(ulIdx = 0; ulIdx < NUM_PROGRESS_WIDGETS; ulIdx++)
    {
        WidgetRemove((tWidget *)(g_psProgressWidgets + ulIdx));
    }

    //
    // Un-gray the non-title widgets in the main panel.
    //
    UpdateWidgetsColor(ClrWhite);
}

//*****************************************************************************
//
// This function monitors the UART-based firmware update and displays the
// progress on the screen.
//
//*****************************************************************************
static unsigned long
UARTUpdate(void)
{
    unsigned long ulIdx, ulCount;

    //
    // Set the progress title to indicate that the firmware is being downloaded
    // from the PC.
    //
    CanvasTextSet(g_psProgressWidgets + PROGRESS_TITLE, "Updating from PC");

    //
    // Add the progress bar widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_PROGRESS_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psProgressWidgets + ulIdx));
    }

    //
    // Gray the non-title widgets in the main panel.
    //
    UpdateWidgetsColor(ClrNotPresent);

    //
    // Set the progress bar to the beginning.
    //
    PROGRESS(0);

    //
    // Update the display.
    //
    DisplayFlush();

    //
    // Reset the counter of the amount of time since the last serial boot
    // loader packet was seen.
    //
    ulCount = 0;

    //
    // Loop while there is still data to be received from the UART.
    //
    while(g_ulImageSize)
    {
        //
        // Wait for a tick or a serial boot loader packet.
        //
        while((HWREGBITW(&g_ulFlags, FLAG_TICK) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 0))
        {
        }

        //
        // See if a serial boot loader packet was received.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 1)
        {
            //
            // Reset the timeout counter.
            //
            ulCount = 0;

            //
            // Clear the serial boot loader packet flag.
            //
            HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) = 0;
        }

        //
        // See if a timer tick has occurred.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_TICK) == 1)
        {
            //
            // Increment the timeout counter.
            //
            ulCount++;

            //
            // Stop waiting if the serial download has timed out.
            //
            if(ulCount == SERIAL_UPDATE_TIMEOUT)
            {
                break;
            }

            //
            // Clear the timer tick flag.
            //
            HWREGBITW(&g_ulFlags, FLAG_TICK) = 0;
        }

        //
        // Update the progress bar and update the screen if the progress bar
        // has moved.
        //
        ulIdx = g_psProgressWidgets[PROGRESS_BAR].sBase.sPosition.sXMax;
        PROGRESS((g_ulImageOffset * 100) / g_ulImageSize);
        if(g_psProgressWidgets[PROGRESS_BAR].sBase.sPosition.sXMax != ulIdx)
        {
            DisplayFlush();
        }
    }

    //
    // Clear the serial boot loader packet flag.
    //
    HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) = 0;

    //
    // Remove the progress bar widgets.
    //
    for(ulIdx = 0; ulIdx < NUM_PROGRESS_WIDGETS; ulIdx++)
    {
        WidgetRemove((tWidget *)(g_psProgressWidgets + ulIdx));
    }

    //
    // Un-gray the non-title widgets in the main panel.
    //
    UpdateWidgetsColor(ClrWhite);

    //
    // If the update timed out, set the message to indicate the failure and
    // return a failure.
    //
    if(ulCount == SERIAL_UPDATE_TIMEOUT)
    {
        usnprintf(g_pcMessageBuffer, sizeof(g_pcMessageBuffer),
                  "Update from PC failed");
        return(1);
    }

    //
    // Return indicating that the serial download was successful.
    //
    return(0);
}

//*****************************************************************************
//
// Displays the "Firmware Update" panel.  The returned valud is the ID of the
// panel to be displayed instead of the "Firmware Update" panel.
//
//*****************************************************************************
unsigned long
DisplayUpdate(void)
{
    unsigned long ulPos, ulIdx;

    //
    // Disable the widget fill for all the widgets except the one for device
    // ID 1.
    //
    for(ulIdx = 0; ulIdx < 3; ulIdx++)
    {
        CanvasFillOff(g_psUpdateWidgets + ulIdx);
    }
    CanvasFillOn(g_psUpdateWidgets + 1);

    //
    // Add the "Firmware Update" panel widgets to the widget list.
    //
    for(ulIdx = 0; ulIdx < NUM_UPDATE_WIDGETS; ulIdx++)
    {
        WidgetAdd(WIDGET_ROOT, (tWidget *)(g_psUpdateWidgets + ulIdx));
    }

    //
    // Set the default cursor position to the device ID selection.
    //
    ulPos = 1;

    //
    // Clear the message buffer.
    //
    g_pcMessageBuffer[0] = '\0';

    //
    // Loop forever.  This loop will be explicitly exited when the proper
    // condition is detected.
    //
    while(1)
    {
        //
        // Print out the current device ID.
        //
        usnprintf(g_pcIDBuffer, sizeof(g_pcIDBuffer), "%d", g_ulCurrentID);

        //
        // Print out the firmware version.
        //
        if(g_ulStatusFirmwareVersion == 0xffffffff)
        {
            usnprintf(g_pcVersionBuffer, sizeof(g_pcVersionBuffer), "---");
        }
        else
        {
            usnprintf(g_pcVersionBuffer, sizeof(g_pcVersionBuffer), "%d",
                      g_ulStatusFirmwareVersion);
        }

        //
        // Update the display.
        //
        DisplayFlush();

        //
        // Wait until a button is pressed, the firmware version is received, or
        // a serial download begins.
        //
        while((HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) == 0) &&
              (HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 0))
        {
        }

        //
        // See if the up button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not already at the top of the
            // screen.
            //
            if(ulPos != 0)
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psUpdateWidgets + ulPos);

                //
                // Decrement the cursor row.
                //
                ulPos--;

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillOn(g_psUpdateWidgets + ulPos);
            }

            //
            // Clear the press flag for the up button.
            //
            HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) = 0;
        }

        //
        // See if the down button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) == 1)
        {
            //
            // Only move the cursor if it is not already at the bottom of the
            // screen.
            //
            if(ulPos != 2)
            {
                //
                // Disable the widget fill for the currently selected widget.
                //
                CanvasFillOff(g_psUpdateWidgets + ulPos);

                //
                // Increment the cursor row.
                //
                ulPos++;

                //
                // Enable the widget fill for the newly selected widget.
                //
                CanvasFillOn(g_psUpdateWidgets + ulPos);
            }

            //
            // Clear the press flag for the down button.
            //
            HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) = 0;
        }

        //
        // See if the left button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) == 1)
        {
            //
            // Only change the device ID if it is greater than one.
            //
            if((ulPos == 1) && (g_ulCurrentID > 1))
            {
                //
                // Decrement the device ID.
                //
                CANSetID(g_ulCurrentID - 1);
            }

            //
            // Clear the press flag for the left button.
            //
            HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) = 0;
        }

        //
        // See if the right button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) == 1)
        {
            //
            // Only change the device ID if it is less than 63.
            //
            if((ulPos == 1) && (g_ulCurrentID < 63))
            {
                //
                // Increment the device ID.
                //
                CANSetID(g_ulCurrentID + 1);
            }

            //
            // Clear the press flag for the right button.
            //
            HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) = 0;
        }

        //
        // See if the select button was pressed.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) == 1)
        {
            //
            // Clear the press flag for the select button.
            //
            HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) = 0;

            //
            // See if the cursor is on the top row of the screen.
            //
            if(ulPos == 0)
            {
                //
                // Display the menu.
                //
                ulIdx = DisplayMenu(PANEL_UPDATE);

                //
                // See if another panel was selected.
                //
                if(ulIdx != PANEL_UPDATE)
                {
                    //
                    // Remove the "Firmware Update" panel widgets.
                    //
                    for(ulPos = 0; ulPos < NUM_UPDATE_WIDGETS; ulPos++)
                    {
                        WidgetRemove((tWidget *)(g_psUpdateWidgets + ulPos));
                    }

                    //
                    // Return the ID of the newly selected panel.
                    //
                    return(ulIdx);
                }

                //
                // Since the "Firmware Update" panel was selected from the
                // menu, move the cursor down one row.
                //
                CanvasFillOff(g_psUpdateWidgets);
                ulPos++;
                CanvasFillOn(g_psUpdateWidgets + 1);
            }

            //
            // See if the cursor is on the start button.
            //
            else if(ulPos == 2)
            {
                //
                // Turn off the fill on the start button.
                //
                CanvasFillOff(g_psUpdateWidgets + 2);

                //
                // Perform the CAN update.
                //
                CANUpdate();

                //
                // Turn on the fill on the start button.
                //
                CanvasFillOn(g_psUpdateWidgets + 2);

                //
                // Clear the button press flags so that any button presses that
                // occur during the firmware update are ignored.
                //
                HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) = 0;
                HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) = 0;
                HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) = 0;
                HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) = 0;
                HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) = 0;
            }
        }

        //
        // See if a serial download has begun.
        //
        if(HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) == 1)
        {
            //
            // Turn off the fill on the current cursor location.
            //
            CanvasFillOff(g_psUpdateWidgets + ulPos);

            //
            // Monitor the UART firmware download.
            //
            if(UARTUpdate() == 0)
            {
                //
                // The firmware was successfully downloaded via the UART, so
                // update the current motor controller with the new firmware.
                //
                CANUpdate();
            }

            //
            // Turn on the fill on the current cursor location.
            //
            CanvasFillOn(g_psUpdateWidgets + ulPos);

            //
            // Clear the button press flags so that any button presses that
            // occur during the firmware update are ignored.
            //
            HWREGBITW(&g_ulFlags, FLAG_UP_PRESSED) = 0;
            HWREGBITW(&g_ulFlags, FLAG_DOWN_PRESSED) = 0;
            HWREGBITW(&g_ulFlags, FLAG_LEFT_PRESSED) = 0;
            HWREGBITW(&g_ulFlags, FLAG_RIGHT_PRESSED) = 0;
            HWREGBITW(&g_ulFlags, FLAG_SELECT_PRESSED) = 0;
        }
    }
}

//*****************************************************************************
//
// This function processes a packet received from the UART.
//
//*****************************************************************************
void
ProcessPacket(void)
{
    unsigned long ulIdx, ulCheckSum, *pulData;

    //
    // Compute the checksum of the packet data.
    //
    for(ulIdx = 3, ulCheckSum = 0; ulIdx < (g_ulUpdateSize + 3); ulIdx++)
    {
        ulCheckSum += g_pucUpdateData[ulIdx];
    }

    //
    // If the checksum does not match, NAK the packet and return without any
    // further processing.
    //
    if((ulCheckSum & 0xff) != g_ulUpdateCheckSum)
    {
        UARTCharPut(UART0_BASE, 0x00);
        UARTCharPut(UART0_BASE, COMMAND_NAK);
        return;
    }

    //
    // Determine the command in this packet.
    //
    switch(g_pucUpdateData[3])
    {
        //
        // The PING command.
        //
        case COMMAND_PING:
        {
            //
            // Set the command status to success.
            //
            g_ulUpdateStatus = COMMAND_RET_SUCCESS;

            //
            // ACK this command.
            //
            UARTCharPut(UART0_BASE, 0x00);
            UARTCharPut(UART0_BASE, COMMAND_ACK);

            //
            // This command has been handled.
            //
            break;
        }

        //
        // The DOWNLOAD command.
        //
        case COMMAND_DOWNLOAD:
        {
            //
            // Set the command status to success.  If a failure is detected
            // during the processing of this command, the status will be
            // updated.
            //
            g_ulUpdateStatus = COMMAND_RET_SUCCESS;

            //
            // A simple do/while(0) loop that is executed once (because of the
            // while(0) at the end) but can be easily exited with a break.
            // This allows a common cleanup at the end (after the loop) while
            // not having increasingly deep nesting of the intervening code.
            //
            do
            {
                //
                // This command is invalid if there are not 8 data bytes in
                // addition to the command.
                //
                if(g_ulUpdateSize != 9)
                {
                    g_ulUpdateStatus = COMMAND_RET_INVALID_CMD;
                    break;
                }

                //
                // Ignore the image offset from the command, but extract the
                // image size.
                //
                g_ulImageOffset = 0;
                g_ulImageSize = ((g_pucUpdateData[8] << 24) |
                                 (g_pucUpdateData[9] << 16) |
                                 (g_pucUpdateData[10] << 8) |
                                 g_pucUpdateData[11]);

                //
                // Make sure that the image is not too large.
                //
                if(g_ulImageSize > 0x1f000)
                {
                    g_ulUpdateStatus = COMMAND_RET_INVALID_ADR;
                    break;
                }

                //
                // Loop through the pages used to store the image.
                //
                for(ulIdx = 0; ulIdx < (g_ulImageSize + 4); ulIdx += 0x400)
                {
                    //
                    // Erase this page of the image storage, setting the status
                    // if a failure occurs.
                    //
                    if(FlashErase(IMAGE_BASE + ulIdx) != 0)
                    {
                        g_ulUpdateStatus = COMMAND_RET_FLASH_FAIL;
                    }
                }

                //
                // Save the image size as the first word of the flash.
                //
                g_pulFirstWords[0] = g_ulImageSize;

                //
                // Indicate that a serial download command has been received.
                //
                HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) = 1;
            }
            while(0);

            //
            // If an error was encountered, reset the image size to zero to
            // indicate that there is no active download.
            //
            if(g_ulUpdateStatus != COMMAND_RET_SUCCESS)
            {
                g_ulImageSize = 0;
            }

            //
            // ACK this command.
            //
            UARTCharPut(UART0_BASE, 0x00);
            UARTCharPut(UART0_BASE, COMMAND_ACK);

            //
            // This command has been handled.
            //
            break;
        }

        //
        // The RUN command.
        //
        case COMMAND_RUN:
        {
            //
            // This command is ignored, so treat it as an invalid command.
            //
            g_ulUpdateStatus = COMMAND_RET_INVALID_CMD;

            //
            // ACK this command.
            //
            UARTCharPut(UART0_BASE, 0x00);
            UARTCharPut(UART0_BASE, COMMAND_ACK);

            //
            // This command has been handled.
            //
            break;
        }

        //
        // The GET_STATUS command.
        //
        case COMMAND_GET_STATUS:
        {
            //
            // ACK this command.
            //
            UARTCharPut(UART0_BASE, 0x00);
            UARTCharPut(UART0_BASE, COMMAND_ACK);

            //
            // Send a status packet.
            //
            UARTCharPut(UART0_BASE, 0x03);
            UARTCharPut(UART0_BASE, g_ulUpdateStatus);
            UARTCharPut(UART0_BASE, g_ulUpdateStatus);

            //
            // Set the UART state to ACK/NAK so that it will look for the ACK
            // or NAK packet that should be received in response to the status
            // packet.
            //
            g_ulUARTState = UART_STATE_ACK_NAK;

            //
            // This command has been handled.
            //
            break;
        }

        //
        // The SEND_DATA command.
        //
        case COMMAND_SEND_DATA:
        {
            //
            // Set the command status to success.  If a failure is detected
            // during the processing of this command, the status will be
            // updated.
            //
            g_ulUpdateStatus = COMMAND_RET_SUCCESS;

            //
            // Decrement the packet size by one (which is the command byte).
            //
            g_ulUpdateSize--;

            //
            // Only process this data if it is part of the download image.
            //
            if((g_ulImageOffset + g_ulUpdateSize) <= g_ulImageSize)
            {
                //
                // Get a pointer to the first word of the download data.
                //
                pulData = (unsigned long *)(g_pucUpdateData + 4);

                //
                // Round the data size up to a word-sized quantity.
                //
                g_ulUpdateSize = (g_ulUpdateSize + 3) & ~3;

                //
                // See if this is the first word of the image.
                //
                if(g_ulImageOffset == 0)
                {
                    //
                    // Save away the first word of the image so it can be
                    // programmed after the remainder of the image.
                    //
                    g_pulFirstWords[1] = *pulData++;
                    g_ulImageOffset += 4;
                    g_ulUpdateSize -= 4;
                }

                //
                // See if this is the second word of the image.
                //
                if((g_ulImageOffset == 4) && (g_ulUpdateSize != 0))
                {
                    //
                    // Save away the second word of the image so it can be
                    // programmed after the remainder of the image.
                    //
                    g_pulFirstWords[2] = *pulData++;
                    g_ulImageOffset += 4;
                    g_ulUpdateSize -= 4;
                }

                //
                // See if there is any data to be programmed.
                //
                if(g_ulUpdateSize != 0)
                {
                    //
                    // Program the data into flash, setting the status if a
                    // failure occurs.
                    //
                    if(FlashProgram(pulData, IMAGE_BASE + g_ulImageOffset + 4,
                                    g_ulUpdateSize) != 0)
                    {
                        g_ulUpdateStatus = COMMAND_RET_FLASH_FAIL;
                    }

                    //
                    // Increment the image offset by the number of words
                    // programmed.
                    //
                    g_ulImageOffset += g_ulUpdateSize;
                }

                //
                // See if the end of the image has been reached.
                //
                if(g_ulImageOffset >= g_ulImageSize)
                {
                    //
                    // Program the first three words of the image, setting the
                    // status if a failure occurs.
                    //
                    if(FlashProgram(g_pulFirstWords, IMAGE_BASE, 12) != 0)
                    {
                        g_ulUpdateStatus = COMMAND_RET_FLASH_FAIL;
                    }

                    //
                    // Set the image size to zero to indicate that the download
                    // has completed.
                    //
                    g_ulImageSize = 0;
                }

                //
                // Indicate that a serial download command has been received.
                //
                HWREGBITW(&g_ulFlags, FLAG_SERIAL_BOOTLOADER) = 1;
            }
            else
            {
                //
                // Data was received that is outside the bounds of the download
                // image, so set the status to indicate an error.
                //
                g_ulUpdateStatus = COMMAND_RET_INVALID_ADR;
            }

            //
            // ACK this command.
            //
            UARTCharPut(UART0_BASE, 0x00);
            UARTCharPut(UART0_BASE, COMMAND_ACK);

            //
            // This command has been handled.
            //
            break;
        }

        //
        // The RESET command.
        //
        case COMMAND_RESET:
        {
            //
            // This command is ignored, so treat it as an invalid command.
            //
            g_ulUpdateStatus = COMMAND_RET_INVALID_CMD;

            //
            // ACK this command.
            //
            UARTCharPut(UART0_BASE, 0x00);
            UARTCharPut(UART0_BASE, COMMAND_ACK);

            //
            // This command has been handled.
            //
            break;
        }

        //
        // An unknown command was received.
        //
        default:
        {
            //
            // Set the status to indicate an unknown command.
            //
            g_ulUpdateStatus = COMMAND_RET_UNKNOWN_CMD;

            //
            // ACK this command.
            //
            UARTCharPut(UART0_BASE, 0x00);
            UARTCharPut(UART0_BASE, COMMAND_ACK);

            //
            // This command has been handled.
            //
            break;
        }
    }
}

//*****************************************************************************
//
// This function is called when a receive interrupt is generated by the UART.
//
//*****************************************************************************
void
UARTIntHandler(void)
{
    //
    // Clear the interrupt cause.
    //
    UARTIntClear(UART0_BASE, UARTIntStatus(UART0_BASE, true));

    //
    // Loop while there are characters available in the UART receive FIFO.
    //
    while(UARTCharsAvail(UART0_BASE))
    {
        //
        // Determine the current UART handler state.
        //
        switch(g_ulUARTState)
        {
            //
            // The UART handler is waiting for the start of a packet.
            //
            case UART_STATE_IDLE:
            {
                //
                // The next character is the packet size.
                //
                g_ulUpdateSize = UARTCharGetNonBlocking(UART0_BASE) - 2;

                //
                // If the packet size is not zero, then go to the wait for
                // checksum state.
                //
                if(g_ulUpdateSize != 0)
                {
                    g_ulUARTState = UART_STATE_HAVE_SIZE;
                }

                //
                // This character has been read.
                //
                break;
            }

            //
            // The UART handler is waiting for the packet checksum.
            //
            case UART_STATE_HAVE_SIZE:
            {
                //
                // The next charcter is the packet checksum.
                //
                g_ulUpdateCheckSum = UARTCharGetNonBlocking(UART0_BASE);

                //
                // Set the index into the packet buffer to zero.
                //
                g_ulUpdateIdx = 3;

                //
                // Go to the wait for data state.
                //
                g_ulUARTState = UART_STATE_HAVE_CKSUM;

                //
                // This character has been read.
                //
                break;
            }

            //
            // The UART handler is waiting for the packet data.
            //
            case UART_STATE_HAVE_CKSUM:
            {
                //
                // The next character is part of the packet data.
                //
                g_pucUpdateData[g_ulUpdateIdx++] =
                    UARTCharGetNonBlocking(UART0_BASE);

                //
                // See if this was the last byte of the packet data.
                //
                if(g_ulUpdateIdx == (g_ulUpdateSize + 3))
                {
                    //
                    // Process this packet.
                    //
                    ProcessPacket();

                    //
                    // If the handler state hasn't changed (for example, as a
                    // result of packet being output), then go to the wait for
                    // packet state.
                    //
                    if(g_ulUARTState == UART_STATE_HAVE_CKSUM)
                    {
                        g_ulUARTState = UART_STATE_IDLE;
                    }
                }

                //
                // This character has been read.
                //
                break;
            }

            //
            // The UART handler is waiting for an ACK or NAK packet.
            //
            case UART_STATE_ACK_NAK:
            {
                //
                // Read the next byte.
                //
                g_ulUpdateIdx = UARTCharGetNonBlocking(UART0_BASE);

                //
                // Go to the wait for packet state if this is an ACK or NAK.
                //
                if((g_ulUpdateIdx == COMMAND_ACK) ||
                   (g_ulUpdateIdx == COMMAND_NAK))
                {
                    g_ulUARTState = UART_STATE_IDLE;
                }

                //
                // This character has been read.
                //
                break;
            }
        }
    }
}

//*****************************************************************************
//
// Initializes the UART update interface.
//
//*****************************************************************************
void
UpdateUARTInit(void)
{
    //
    // Set the flash programming speed based on the processor speed.
    //
    FlashUsecSet(50);

    //
    // Enable the peripherals used by the UART.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure the UART Tx and Rx pins for use by the UART.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure and enable the UART for 115,200, 8-N-1 operation.
    //
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                         UART_CONFIG_PAR_NONE));
    UARTEnable(UART0_BASE);

    //
    // Enable the UART receive interrupts.
    //
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    IntEnable(INT_UART0);
}
