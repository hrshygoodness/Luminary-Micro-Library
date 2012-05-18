//*****************************************************************************
//
// usbhost.c - main routine for the USB MSC example.
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
// This is part of revision 8555 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/udma.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usbmsc.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"
#include "fatfs/src/ff.h"
#include "drivers/buttons.h"
#include "drivers/formike128x128x16.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Mass Storage Class Host (usb_host_msc)</h1>
//!
//! This example application demonstrates how to connect a USB mass storage
//! class device to the evaluation kit.  When a device is detected, the
//! application displays the contents of the file system and allows browsing
//! using the buttons.
//
//*****************************************************************************

//*****************************************************************************
//
// Colors used by the display.
//
//*****************************************************************************
#define FILE_COLOR              ClrWhite
#define DIR_COLOR               ClrBlue
#define BACKGROUND_COLOR        ClrBlack

//*****************************************************************************
//
// Screen and font settings, these determine some fixed memory allocations
// by the application.
//
//*****************************************************************************
#define SCREEN_HEIGHT           128
#define FONT_HEIGHT             8
#define SPLASH_HEIGHT           15
#define TOP_HEIGHT              (SPLASH_HEIGHT + 4)

//*****************************************************************************
//
// Current FAT fs state.
//
//*****************************************************************************
static FATFS g_sFatFs;

//*****************************************************************************
//
// The graphical Context state.
//
//*****************************************************************************
tContext g_sContext;

//*****************************************************************************
//
// Defines the number of times to call to check if the attached device is
// ready.
//
//*****************************************************************************
#define USBMSC_DRIVE_RETRY      4

//*****************************************************************************
//
// The size of the host controller's memory pool in bytes.
//
//*****************************************************************************
#define HCD_MEMORY_SIZE         128

//*****************************************************************************
//
// The memory pool to provide to the Host controller driver.
//
//*****************************************************************************
unsigned char g_pHCDPool[HCD_MEMORY_SIZE];

//*****************************************************************************
//
// The instance data for the MSC driver.
//
//*****************************************************************************
unsigned long g_ulMSCInstance = 0;

//*****************************************************************************
//
// Declare the USB Events driver interface.
//
//*****************************************************************************
DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the MSC class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_USBHostMSCClassDriver
    ,&g_sUSBEventDriver
};

//*****************************************************************************
//
// This global holds the number of class drivers in the g_ppHostClassDrivers
// list.
//
//*****************************************************************************
static const unsigned long g_ulNumHostClassDrivers =
    sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

//*****************************************************************************
//
// The defines used with the g_ulButtons variable.
//
//*****************************************************************************
#define BUTTON_UP_CLICK         0x00000001
#define BUTTON_DOWN_CLICK       0x00000002
#define BUTTON_LEFT_CLICK       0x00000004
#define BUTTON_RIGHT_CLICK      0x00000008
#define BUTTON_SELECT_CLICK     0x00000010

//*****************************************************************************
//
// This global holds the current button events.
//
//*****************************************************************************
volatile unsigned long g_ulButtons = 0;

#define FLAGS_DEVICE_PRESENT    0x00000001

//*****************************************************************************
//
// Holds global flags for the system.
//
//*****************************************************************************
int g_ulFlags = 0;

//*****************************************************************************
//
// The parameters to control the USB mux on the LM3S3748 board.
//
//*****************************************************************************
#define USB_MUX_GPIO_PERIPH     SYSCTL_PERIPH_GPIOH
#define USB_MUX_GPIO_BASE       GPIO_PORTH_BASE
#define USB_MUX_GPIO_PIN        GPIO_PIN_2
#define USB_MUX_SEL_DEVICE      USB_MUX_GPIO_PIN
#define USB_MUX_SEL_HOST        0

//*****************************************************************************
//
// The directory display parameters.
//
//*****************************************************************************
#define NUM_DIR_ENTRIES         ((SCREEN_HEIGHT - TOP_HEIGHT) / FONT_HEIGHT)
#define MAX_DIR_DEPTH           4
#define MAX_FILE_NAME_LEN       (8 + 1 + 3 + 1)  // 8.1 + 1

//*****************************************************************************
//
// The directory state structure.
//
//*****************************************************************************
struct
{
    //
    // Each of the directory entries that are currently valid.
    //
    FILINFO FileInfo[NUM_DIR_ENTRIES];

    //
    // The current position in the directory structure.
    //
    unsigned long ulIndex;

    //
    // What is the currently highlighted selection.
    //
    unsigned long ulSelectIndex;

    //
    // The number of valid values in the directory structure.
    //
    unsigned long ulValidValues;

    //
    // The current directory context.
    //
    DIR DirState;

    //
    // The null terminated string that holds the current directory name.
    //
    char szPWD[MAX_DIR_DEPTH * MAX_FILE_NAME_LEN];
}
g_DirData;

//*****************************************************************************
//
// Hold the current state for the application.
//
//*****************************************************************************
volatile enum
{
    //
    // No device is present.
    //
    STATE_NO_DEVICE,

    //
    // Mass storage device is being enumerated.
    //
    STATE_DEVICE_ENUM,

    //
    // Mass storage device is ready.
    //
    STATE_DEVICE_READY,

    //
    // An unsupported device has been attached.
    //
    STATE_UNKNOWN_DEVICE,

    //
    // A mass storage device was connected but failed to ever report ready.
    //
    STATE_TIMEOUT_DEVICE,

    //
    // A power fault has occurred.
    //
    STATE_POWER_FAULT
}
g_eState;

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.  In this application uDMA is only used for USB,
// so only the first 6 channels are needed.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
tDMAControlTable g_sDMAControlTable[6];
#elif defined(ccs)
#pragma DATA_ALIGN(g_sDMAControlTable, 1024)
tDMAControlTable g_sDMAControlTable[6];
#else
tDMAControlTable g_sDMAControlTable[6] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// This function clears out the text area used to display the directory
// contents.
//
// \param None.
//
// This function is used to clear out the text area used to display the
// directory contents.
//
// \return None.
//
//*****************************************************************************
void
ClearTextBox(void)
{
    tRectangle TextBox;

    TextBox.sXMin = 0;
    TextBox.sYMin = TOP_HEIGHT;
    TextBox.sXMax = g_sFormike128x128x16.usWidth - 1;
    TextBox.sYMax = g_sFormike128x128x16.usHeight - 1;

    //
    // Set the fill color.
    //
    GrContextForegroundSet(&g_sContext, BACKGROUND_COLOR);

    //
    // Fill the text area with the fill color.
    //
    GrRectFill(&g_sContext, &TextBox);

    GrContextForegroundSet(&g_sContext, FILE_COLOR);
    GrContextBackgroundSet(&g_sContext, ClrBlack);
}

//*****************************************************************************
//
// This function updates the contents of the directory text window.
//
// \param None.
//
// This function is will update the state of the directory window.  This can
// be the result of a DirUpdate() call which completely changed the contents
// of the window, or a selection changed and the screen needs to be updated.
//
// \return None.
//
//*****************************************************************************
void
UpdateWindow(void)
{
    int iIdx;
    int iLine;

    //
    // Set the first line of the directory text window.
    //
    iLine = TOP_HEIGHT;

    //
    // Clear out the text area for the entries.
    //
    ClearTextBox();

    //
    // Display all valid values.
    //
    for(iIdx = 0; iIdx < g_DirData.ulValidValues; iIdx++)
    {
        //
        // Change the background for the selected item.
        //
        if(g_DirData.ulSelectIndex == iIdx)
        {
            GrContextBackgroundSet(&g_sContext, ClrGray);
        }
        else
        {
            GrContextBackgroundSet(&g_sContext, ClrBlack);
        }

        //
        // Change the color for directories.
        //
        if (g_DirData.FileInfo[iIdx].fattrib & AM_DIR)
        {
            GrContextForegroundSet(&g_sContext, DIR_COLOR);
            GrStringDraw(&g_sContext, g_DirData.FileInfo[iIdx].fname,
                         100, 0, iLine, 1);
        }
        //
        // Change the color for files.
        //
        else
        {
            GrContextForegroundSet(&g_sContext, FILE_COLOR);
            GrStringDraw(&g_sContext, g_DirData.FileInfo[iIdx].fname,
                         100, 0, iLine, 1);
        }

        //
        // Move down by the height of the characters used.
        //
        iLine += g_sFontFixed6x8.ucHeight;
    }
}

//*****************************************************************************
//
// This function seeks the directory entries in the positive or negative
// direction.
//
// \param iDirSeek is the number of entries to seek by in the positive or
// negative direction.
//
// This function is used to seek up(negative) or down(positive) the list of
// directory entries by the value given in the \e iDirSeek parameter.  If the
// call would cause the seek to go too far at either end the result will
// either be at the first entry in the directory or at the end of the
// directory entry list.
//
// \return None.
//
//*****************************************************************************
int
DirSeek(int iDirSeek)
{
    int iSeek;
    FILINFO FileInfo;

    //
    // If this is a positive seek then just seek by that amount.
    //
    if(iDirSeek >= 0)
    {
        //
        // Simply seek this many forward.
        //
        iSeek = iDirSeek;
    }
    //
    // Seeking in the negative direction is more difficult as FAT has no
    // backward linking.
    //
    else
    {
        //
        // Set the position to seek from 0.
        //
        iSeek = g_DirData.ulIndex + iDirSeek;

        //
        // Rail the value at 0.
        //
        if(iSeek >= 0)
        {
            //
            // Seek from the start of the current directory.
            //
            if (f_opendir(&g_DirData.DirState, g_DirData.szPWD) == FR_OK)
            {
                // what to do?
            }

            //
            // Reset the index to 0.
            //
            g_DirData.ulIndex = 0;
        }
    }

    //
    // Don't do anything else if the seek would go negative.
    //
    if(iSeek < 0)
    {
        return(0);
    }

    //
    // All values are now invalid.
    //
    g_DirData.ulValidValues = 0;

    //
    // Now perform the needed seek which is now, in all cases, a positive seek.
    //
    while(iSeek > 0)
    {
        //
        // Keep reading entries until the correct seek has been completed or
        // there are no more entries.
        //
        if((f_readdir(&g_DirData.DirState, &FileInfo) == FR_OK) &&
            FileInfo.fname[0])
        {
            //
            // Found a valid entry so decrement the remaining number to search
            // for.
            //
            iSeek--;

            //
            // Update the current position in the directory.
            //
            g_DirData.ulIndex++;
        }
        else
        {
            return(0);
        }
    }
    return(1);
}

//*****************************************************************************
//
// This updates the directory window state but not the screen.
//
// \param None.
//
// This function is used to update the names of the directory entries and the
// current state of the directory contents.  If it is called and there are no
// more directory entries the contents of the directory state is invalidated
// but the screen is not updated.
//
// \return None.
//
//*****************************************************************************
int
DirUpdate(void)
{
    int iIdx;

    iIdx = 0;

    //
    // Move the index value down by the number of entries that were previously
    // on the screen.
    //
    g_DirData.ulIndex += g_DirData.ulValidValues;

    //
    // If at the top and in a directory display the ".." entry.
    //
    if((g_DirData.szPWD[1] != 0) && (g_DirData.ulIndex) == 0)
    {
        //
        // Skip the first entry as we are going to use it for ".." entry.
        //
        iIdx = 1;
        g_DirData.FileInfo[0].fname[0] = '.';
        g_DirData.FileInfo[0].fname[1] = '.';
        g_DirData.FileInfo[0].fname[2] = 0;

        //
        // Force this to a directory entry so that it can be changed to.
        //
        g_DirData.FileInfo[0].fattrib = AM_DIR;
    }

    //
    // There are directory entries remaining, read them out until the buffer
    // is full.
    //
    while((f_readdir(&g_DirData.DirState, &g_DirData.FileInfo[iIdx]) ==
           FR_OK) && (g_DirData.FileInfo[iIdx].fname[0] != 0))
    {
        iIdx++;

        if(iIdx >= NUM_DIR_ENTRIES)
        {
            break;
        }
    }

    //
    // Save the number of valid entries read in.
    //
    g_DirData.ulValidValues = iIdx;

    return(iIdx);
}

//*****************************************************************************
//
// This handles the event when the selection must move up.
//
// \param None.
//
// This function is called when the application wants to move the selection
// point in the directory list up by one element.  This will handle updating
// the directory contents if necessary or just updating the screen.
//
// \return None.
//
//*****************************************************************************
void
MoveDown(void)
{
    //
    // If the selection is moving to the next screen, then update the contents
    // and the display.
    //
    if(g_DirData.ulValidValues <= (g_DirData.ulSelectIndex + 1))
    {
        //
        // If the contents of the directory update successfully, then update
        // the screen.
        //
        if(DirUpdate() != 0)
        {
            //
            // Move the selection back to the first entry.
            //
            g_DirData.ulSelectIndex = 0;

            //
            // Update the display.
            //
            UpdateWindow();
        }
    }
    else
    {
        //
        // Just update the selection and update the display.
        //
        g_DirData.ulSelectIndex++;
        UpdateWindow();
    }
}

//*****************************************************************************
//
// This handles the event when the selection must move down.
//
// \param None.
//
// This function is called when the application wants to move the selection
// point in the directory list down by one element.  This will handle updating
// the directory contents if necessary or just updating the screen.
//
// \return None.
//
//*****************************************************************************
void
MoveUp(void)
{
    int iSeek;

    //
    // If the selection is at the top, then there will need to be a directory
    // update.
    //
    if(g_DirData.ulSelectIndex == 0)
    {
        //
        // Seek by at least one page of entries.
        //
        iSeek = -NUM_DIR_ENTRIES;

        //
        // Need to go up one extra if we were right at the top and seeked
        // up to skip the first entry on the old page.
        //
        if(g_DirData.ulValidValues == 0)
        {
            iSeek--;
        }

        //
        // Seek back one page in the directory structure.
        //
        if(DirSeek(iSeek))
        {
            //
            // Set the selection to the last element on the screen and update
            // the directory contents.
            //
            g_DirData.ulSelectIndex = NUM_DIR_ENTRIES - 1;
            DirUpdate();
        }
    }
    else
    {
        //
        // Not at the top of the screen so if there are no valid values, read
        // just enough to refresh the current values that may have been
        // erased by seeking past the end of the directory.
        //
        if(g_DirData.ulValidValues == 0)
        {
            DirSeek(-(g_DirData.ulSelectIndex + 1));
            DirUpdate();
        }

        //
        // Move the selection up by one.
        //
        g_DirData.ulSelectIndex--;
    }

    //
    // Update the display.
    //
    UpdateWindow();
}

//*****************************************************************************
//
// This handles the event when the selection is pressed.
//
// \param None.
//
// This function is called when the application wants to check if the selection
// should perform any action.  At this point it will only act on directories
// by changing into that directory and displaying its contents.
//
// \return None.
//
//*****************************************************************************
void
SelectDir(void)
{
    int iIdx;

    //
    // Only operate on directories.
    //
    if(g_DirData.FileInfo[g_DirData.ulSelectIndex].fattrib & AM_DIR)
    {
        //
        // Look for the special ".." entry.
        //
        if((g_DirData.FileInfo[g_DirData.ulSelectIndex].fname[0] == '.') &&
           (g_DirData.FileInfo[g_DirData.ulSelectIndex].fname[1] == '.'))
        {
            //
            // Find the end of the current directory.
            //
            iIdx = strlen(g_DirData.szPWD);

            //
            // Look backwards through the string for the last "/"
            //
            while(iIdx != 0)
            {
                if(g_DirData.szPWD[iIdx] == '/')
                {
                    g_DirData.szPWD[iIdx] = 0;
                    break;
                }

                iIdx--;
            }

            //
            // If none was found then go back to the root directory.
            //
            if(iIdx == 0)
            {
                g_DirData.szPWD[1] = 0;
            }

            //
            // If the directory open fails, then just return for now
            // this will result in no update.
            //
            if (f_opendir(&g_DirData.DirState, g_DirData.szPWD) != FR_OK)
            {
                return;
            }
        }
        else
        {
            //
            // Find the end of the directory.
            //
            iIdx = strlen(g_DirData.szPWD);

            //
            // Don't append if at the root.
            //
            if(iIdx != 1)
            {
                g_DirData.szPWD[iIdx] = '/';
                g_DirData.szPWD[iIdx+1] = 0;
            }

            //
            // Append the new directory to the current directory.
            //
            strcat(g_DirData.szPWD,
                   g_DirData.FileInfo[g_DirData.ulSelectIndex].fname);

            //
            // Read from the start of the current directory.
            //
            if (f_opendir(&g_DirData.DirState, g_DirData.szPWD) != FR_OK)
            {
                //
                // If the directory open fails, then just return for now
                // this will result in no update.
                //
                return;
            }
        }

        //
        // Reset the state of the directory structure and update the
        // contents.
        //
        g_DirData.ulIndex = 0;
        g_DirData.ulSelectIndex = 0;
        g_DirData.ulValidValues = 0;
        DirUpdate();

        //
        // Update the display.
        //
        UpdateWindow();
    }
}

//*****************************************************************************
//
// Initializes the file system module.
//
// \param None.
//
// This function initializes the third party FAT implementation.
//
// \return Returns \e true on success or \e false on failure.
//
//*****************************************************************************
tBoolean
FileInit(void)
{
    //
    // Mount the file system, using logical disk 0.
    //
    if(f_mount(0, &g_sFatFs) != FR_OK)
    {
        return(false);
    }
    return(true);
}

//*****************************************************************************
//
// This is the callback from the MSC driver.
//
// \param ulInstance is the driver instance which is needed when communicating
// with the driver.
// \param ulEvent is one of the events defined by the driver.
// \param pvData is a pointer to data passed into the initial call to register
// the callback.
//
// This function handles callback events from the MSC driver.  The only events
// currently handled are the MSC_EVENT_OPEN and MSC_EVENT_CLOSE.  This allows
// the main routine to know when an MSC device has been detected and
// enumerated and when an MSC device has been removed from the system.
//
// \return Returns \e true on success or \e false on failure.
//
//*****************************************************************************
void
MSCCallback(unsigned long ulInstance, unsigned long ulEvent, void *pvData)
{
    //
    // Determine the event.
    //
    switch(ulEvent)
    {
        //
        // Called when the device driver has successfully enumerated an MSC
        // device.
        //
        case MSC_EVENT_OPEN:
        {
            //
            // Proceed to the enumeration state.
            //
            g_eState = STATE_DEVICE_ENUM;
            break;
        }

        //
        // Called when the device driver has been unloaded due to error or
        // the device is no longer present.
        //
        case MSC_EVENT_CLOSE:
        {
            //
            // Go back to the "no device" state and wait for a new connection.
            //
            g_eState = STATE_NO_DEVICE;

            //
            // Re-initialize the file system.
            //
            FileInit();

            break;
        }

        default:
        {
            break;
        }
    }
}

//*****************************************************************************
//
// This is the generic callback from host stack.
//
// \param pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the mass storage device.  At this
// point this is used to detect unsupported devices being inserted and removed.
// It is also used to inform the application when a power fault has occurred.
// This function is required when the g_USBGenericEventDriver is included in
// the host controller driver array that is passed in to the
// USBHCDRegisterDrivers() function.
//
// \return None.
//
//*****************************************************************************
void
USBHCDEvents(void *pvData)
{
    tEventInfo *pEventInfo;

    //
    // Cast this pointer to its actual type.
    //
    pEventInfo = (tEventInfo *)pvData;

    switch(pEventInfo->ulEvent)
    {
        //
        // New keyboard detected.
        //
        case USB_EVENT_UNKNOWN_CONNECTED:
        {
            //
            // An unknown device was detected.
            //
            g_eState = STATE_UNKNOWN_DEVICE;

            break;
        }

        //
        // Keyboard has been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Unknown device has been removed.
            //
            g_eState = STATE_NO_DEVICE;

            break;
        }
        case USB_EVENT_POWER_FAULT:
        {
            //
            // No power means no device is present.
            //
            g_eState = STATE_POWER_FAULT;

            break;
        }
        default:
        {
            break;
        }
    }
}

//*****************************************************************************
//
// This is interrupt handler for the systick interrupt.
//
// This function handles the interrupts generated by the system tick.  These
// are used for button debouncing and updating the state of the buttons.
// The buttons are stored in a bitmask indicating which buttons have been
// release.  If a button is pressed twice, only one press will be seen.  There
// is no press and hold detection.
//
// \return None.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    unsigned char ucButtons;
    unsigned char ucChanged;
    unsigned char ucRepeat;

    //
    // Determine the state of the pushbuttons.
    //
    ucButtons = ButtonsPoll(&ucChanged, &ucRepeat);

    //
    // Up button has been released.
    //
    if(BUTTON_RELEASED(UP_BUTTON, ucButtons, ucChanged))
    {
        g_ulButtons |= BUTTON_UP_CLICK;
    }

    //
    // Up button has been held.
    //
    if(BUTTON_REPEAT(UP_BUTTON, ucRepeat))
    {
        g_ulButtons |= BUTTON_UP_CLICK;
    }

    //
    // Down button has been released.
    //
    if(BUTTON_RELEASED(DOWN_BUTTON, ucButtons, ucChanged))
    {
        g_ulButtons |= BUTTON_DOWN_CLICK;
    }

    //
    // Down button has been held.
    //
    if(BUTTON_REPEAT(DOWN_BUTTON, ucRepeat))
    {
        g_ulButtons |= BUTTON_DOWN_CLICK;
    }

    //
    // Left button has been released.
    //
    if(BUTTON_RELEASED(LEFT_BUTTON, ucButtons, ucChanged))
    {
        g_ulButtons |= BUTTON_LEFT_CLICK;
    }

    //
    // Right button has been released.
    //
    if(BUTTON_RELEASED(RIGHT_BUTTON, ucButtons, ucChanged))
    {
        g_ulButtons |= BUTTON_RIGHT_CLICK;
    }

    //
    // Select button has been released.
    //
    if(BUTTON_RELEASED(SELECT_BUTTON, ucButtons, ucChanged))
    {
        g_ulButtons |= BUTTON_SELECT_CLICK;
    }
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int
main(void)
{
    FRESULT FileResult;
    tRectangle sRect;
    unsigned long ulDriveTimeout;

    //
    // Initially wait for device connection.
    //
    g_eState = STATE_NO_DEVICE;

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    //
    // Enable the peripherals used by this example.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //
    // Set the USB pins to be controlled by the USB controller.
    //
    GPIOPinTypeUSBDigital(GPIO_PORTH_BASE, GPIO_PIN_3 | GPIO_PIN_4);

    //
    // The LM3S3748 board uses a USB mux that must be switched to use the
    // host connecter and not the device connecter.
    //
    GPIOPinTypeGPIOOutput(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN);
    GPIOPinWrite(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN, USB_MUX_SEL_HOST);

    //
    // Set the system tick to fire 100 times per second.
    //
    SysTickPeriodSet(SysCtlClockGet() / 100);
    SysTickIntEnable();
    SysTickEnable();

    //
    // Enable the uDMA controller and set up the control table base.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    uDMAEnable();
    uDMAControlBaseSet(g_sDMAControlTable);

    //
    // Initialize the display driver.
    //
    Formike128x128x16Init();

    //
    // Turn on the backlight.
    //
    Formike128x128x16BacklightOn();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sFormike128x128x16);

    //
    // Fill the top 15 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = SPLASH_HEIGHT - 1;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_pFontFixed6x8);

    GrStringDrawCentered(&g_sContext, "usb_host_msc", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 7, 0);

    //
    // Set the color to white and select the 20 point font.
    //
    GrContextForegroundSet(&g_sContext, FILE_COLOR);

    GrStringDraw(&g_sContext, "No Device", 100, 0, TOP_HEIGHT, 1);

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, g_ulNumHostClassDrivers);

    //
    // Open an instance of the mass storage class driver.
    //
    g_ulMSCInstance = USBHMSCDriveOpen(0, MSCCallback);

    //
    // Initialize the drive timeout.
    //
    ulDriveTimeout = USBMSC_DRIVE_RETRY;

    //
    // Initialize the power configuration.  This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH);

    //
    // Initialize the host controller.
    //
    USBHCDInit(0, g_pHCDPool, HCD_MEMORY_SIZE);

    //
    // Initialize the pushbuttons.
    //
    ButtonsInit();

    //
    // Set the auto repeat rates for the up and down buttons.
    //
    ButtonsSetAutoRepeat(UP_BUTTON, 50, 15);
    ButtonsSetAutoRepeat(DOWN_BUTTON, 50, 15);

    //
    // Current directory is "/"
    //
    g_DirData.szPWD[0] = '/';
    g_DirData.szPWD[1] = 0;

    //
    // Initialize the file system.
    //
    FileInit();

    while(1)
    {
        USBHCDMain();

        switch(g_eState)
        {
            case STATE_DEVICE_ENUM:
            {
                //
                // Take it easy on the Mass storage device if it is slow to
                // start up after connecting.
                //
                if(USBHMSCDriveReady(g_ulMSCInstance) != 0)
                {
                    //
                    // Wait about 500ms before attempting to check if the
                    // device is ready again.
                    //
                    SysCtlDelay(SysCtlClockGet() / (3 * 2));

                    //
                    // Decrement the retry count.
                    //
                    ulDriveTimeout--;

                    //
                    // If the timeout is hit then go to the
                    // STATE_TIMEOUT_DEVICE state.
                    //
                    if(ulDriveTimeout == 0)
                    {
                        g_eState = STATE_TIMEOUT_DEVICE;
                    }

                    break;
                }

                //
                // Reset the root directory.
                //
                g_DirData.szPWD[0] = '/';
                g_DirData.szPWD[1] = 0;

                //
                // Open the root directory.
                //
                FileResult = f_opendir(&g_DirData.DirState, g_DirData.szPWD);

                //
                // Wait for the root directory to open successfully.  The MSC
                // device can enumerate before being read to be accessed, so
                // there may be some delay before it is ready.
                //
                if(FileResult == FR_OK)
                {
                    //
                    // Reset the directory state.
                    //
                    g_DirData.ulIndex = 0;
                    g_DirData.ulSelectIndex = 0;
                    g_DirData.ulValidValues = 0;
                    g_eState = STATE_DEVICE_READY;

                    //
                    // Ignore buttons pressed before being ready.
                    //
                    g_ulButtons = 0;

                    //
                    // Update the screen if the root directory opened
                    // successfully.
                    //
                    DirUpdate();
                    UpdateWindow();
                }
                else if(FileResult != FR_NOT_READY)
                {
                    // some kind of error
                }

                //
                // Set the Device Present flag.
                //
                g_ulFlags = FLAGS_DEVICE_PRESENT;

                break;
            }

            //
            // This is the running state where buttons are checked and the
            // screen is updated.
            //
            case STATE_DEVICE_READY:
            {
                //
                // Down button pressed and released.
                //
                if(g_ulButtons & BUTTON_DOWN_CLICK)
                {
                    //
                    // Update the screen and directory state.
                    //
                    MoveDown();

                    //
                    // Clear the button pressed event.
                    //
                    g_ulButtons &= ~BUTTON_DOWN_CLICK;
                }

                //
                // Up button pressed and released.
                //
                if(g_ulButtons & BUTTON_UP_CLICK)
                {
                    //
                    // Update the screen and directory state.
                    //
                    MoveUp();

                    //
                    // Clear the button pressed event.
                    //
                    g_ulButtons &= ~BUTTON_UP_CLICK;
                }

                //
                // Select button pressed and released.
                //
                if(g_ulButtons & BUTTON_SELECT_CLICK)
                {
                    //
                    // If this was a directory go into it.
                    //
                    SelectDir();

                    //
                    // Clear the button pressed event.
                    //
                    g_ulButtons &= ~BUTTON_SELECT_CLICK;
                }
                break;
            }

            //
            // If there is no device then just wait for one.
            //
            case STATE_NO_DEVICE:
            {
                if(g_ulFlags == FLAGS_DEVICE_PRESENT)
                {
                    //
                    // Clear the screen and indicate that there is no longer
                    // a device present.
                    //
                    ClearTextBox();
                    GrStringDraw(&g_sContext, "No Device", 100, 0, TOP_HEIGHT,
                                 1);

                    //
                    // Clear the Device Present flag.
                    //
                    g_ulFlags &= ~FLAGS_DEVICE_PRESENT;
                }
                break;
            }

            //
            // An unknown device was connected.
            //
            case STATE_UNKNOWN_DEVICE:
            {
                //
                // If this is a new device then change the status.
                //
                if((g_ulFlags & FLAGS_DEVICE_PRESENT) == 0)
                {
                    //
                    // Clear the screen and indicate that an unknown device is
                    // present.
                    //
                    ClearTextBox();
                    GrStringDraw(&g_sContext, "Unknown Device", 100, 0,
                                 TOP_HEIGHT, 1);
                }

                //
                // Set the Device Present flag.
                //
                g_ulFlags = FLAGS_DEVICE_PRESENT;

                break;
            }

            //
            // The connected mass storage device is not reporting ready.
            //
            case STATE_TIMEOUT_DEVICE:
            {
                //
                // If this is the first time in this state then print a
                // message.
                //
                if((g_ulFlags & FLAGS_DEVICE_PRESENT) == 0)
                {
                    //
                    // Clear the screen and indicate that an unknown device
                    // is present.
                    //
                    ClearTextBox();
                    GrStringDraw(&g_sContext, "Device Timeout", 100, 0,
                                 TOP_HEIGHT, 1);
                }

                //
                // Set the Device Present flag.
                //
                g_ulFlags = FLAGS_DEVICE_PRESENT;

                break;
            }

            //
            // Something has caused a power fault.
            //
            case STATE_POWER_FAULT:
            {
                break;
            }

            default:
            {
                break;
            }
        }
    }
}
