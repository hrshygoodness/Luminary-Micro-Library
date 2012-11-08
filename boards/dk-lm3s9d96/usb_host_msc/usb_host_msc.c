//*****************************************************************************
//
// usb_host_msc.c - Example program for reading files from a USB flash drive.
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
// This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/udma.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/listbox.h"
#include "grlib/pushbutton.h"
#include "utils/cmdline.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "usblib/usblib.h"
#include "usblib/usbmsc.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
#include "drivers/touch.h"
#include "drivers/set_pinout.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Mass Storage Class Host Example (usb_host_msc)</h1>
//!
//! This example application demonstrates reading a file system from
//! a USB flash disk.  It makes use of FatFs, a FAT file system driver.  It
//! provides a simple widget-based console on the display and also a UART-based
//! command line for viewing and navigating the file system on the flash disk.
//!
//! For additional details about FatFs, see the following site:
//! http://elm-chan.org/fsw/ff/00index_e.html
//!
//! The application may also be operated via a serial terminal attached to
//! UART0. The RS232 communication parameters should be set to 115,200 bits
//! per second, and 8-n-1 mode.  When the program is started a message will be
//! printed to the terminal.  Type ``help'' for command help.
//
//*****************************************************************************

//*****************************************************************************
//
// Debug-related definitions and declarations.
//
// Debug output is available via UART0 if DEBUG is defined during build.
//
//*****************************************************************************
#ifdef DEBUG
//*****************************************************************************
//
// Map all debug print calls to UARTprintf in debug builds.
//
//*****************************************************************************
#define DEBUG_PRINT UARTprintf

#else

//*****************************************************************************
//
// Compile out all debug print calls in release builds.
//
//*****************************************************************************
#define DEBUG_PRINT while(0) ((int (*)(char *, ...))0)
#endif

//*****************************************************************************
//
// Defines the size of the buffers that hold the path, or temporary
// data from the USB disk.  There are two buffers allocated of this size.
// The buffer size must be large enough to hold the longest expected
// full path name, including the file name, and a trailing null character.
//
//*****************************************************************************
#define PATH_BUF_SIZE   80

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// Defines the number of times to call to check if the attached device is
// ready.
//
//*****************************************************************************
#define USBMSC_DRIVE_RETRY      4

//*****************************************************************************
//
// This buffer holds the full path to the current working directory.
// Initially it is root ("/").
//
//*****************************************************************************
static char g_cCwdBuf[PATH_BUF_SIZE] = "/";

//*****************************************************************************
//
// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card.
//
//*****************************************************************************
static char g_cTmpBuf[PATH_BUF_SIZE];

//*****************************************************************************
//
// The buffer that holds the command line.
//
//*****************************************************************************
static char g_cCmdBuf[CMD_BUF_SIZE];

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;
static FIL g_sFileObject;

//*****************************************************************************
//
// A structure that holds a mapping between an FRESULT numerical code,
// and a string representation.  FRESULT codes are returned from the FatFs
// FAT file system driver.
//
//*****************************************************************************
typedef struct
{
    FRESULT fresult;
    char *pcResultStr;
}
tFresultString;

//*****************************************************************************
//
// A macro to make it easy to add result codes to the table.
//
//*****************************************************************************
#define FRESULT_ENTRY(f)        { (f), (#f) }

//*****************************************************************************
//
// A table that holds a mapping between the numerical FRESULT code and
// it's name as a string.  This is used for looking up error codes for
// printing to the console.
//
//*****************************************************************************
tFresultString g_sFresultStrings[] =
{
    FRESULT_ENTRY(FR_OK),
    FRESULT_ENTRY(FR_NOT_READY),
    FRESULT_ENTRY(FR_NO_FILE),
    FRESULT_ENTRY(FR_NO_PATH),
    FRESULT_ENTRY(FR_INVALID_NAME),
    FRESULT_ENTRY(FR_INVALID_DRIVE),
    FRESULT_ENTRY(FR_DENIED),
    FRESULT_ENTRY(FR_EXIST),
    FRESULT_ENTRY(FR_RW_ERROR),
    FRESULT_ENTRY(FR_WRITE_PROTECTED),
    FRESULT_ENTRY(FR_NOT_ENABLED),
    FRESULT_ENTRY(FR_NO_FILESYSTEM),
    FRESULT_ENTRY(FR_INVALID_OBJECT),
    FRESULT_ENTRY(FR_MKFS_ABORTED)
};

//*****************************************************************************
//
// Error reasons returned by ChangeDirectory().
//
//*****************************************************************************
#define NAME_TOO_LONG_ERROR 1
#define OPENDIR_ERROR       2

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES (sizeof(g_sFresultStrings) / sizeof(tFresultString))

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// Our running system tick counter and a global used to determine the time
// elapsed since last call to GetTickms().
//
//*****************************************************************************
unsigned long g_ulSysTickCount;
unsigned long g_ulLastTick;

//*****************************************************************************
//
// Storage for the filename listbox widget string table.
//
//*****************************************************************************
#define NUM_LIST_STRINGS 48
const char *g_ppcDirListStrings[NUM_LIST_STRINGS];

//*****************************************************************************
//
// Storage for the names of the files in the current directory.  Filenames
// are stored in format "(D) filename.ext" for directories or "(F) filename.ext"
// for files.
//
//*****************************************************************************
#define MAX_FILENAME_STRING_LEN (4 + 8 + 1 + 3 + 1)
char g_pcFilenames[NUM_LIST_STRINGS][MAX_FILENAME_STRING_LEN];

//*****************************************************************************
//
// Storage for the strings which appear in the status box at the bottom of the
// display.
//
//****************************************************************************
#define NUM_STATUS_STRINGS 6
#define MAX_STATUS_STRING_LEN (36 + 1)
char g_pcStatus[NUM_STATUS_STRINGS][MAX_STATUS_STRING_LEN];

//*****************************************************************************
//
// Holds global flags for the system.
//
//*****************************************************************************
int g_ulFlags = 0;

//*****************************************************************************
//
// Flag indicating that some USB device is connected.
//
//*****************************************************************************
#define FLAGS_DEVICE_PRESENT    0x00000001

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
// The current USB operating mode - Host, Device or unknown.
//
//*****************************************************************************
tUSBMode g_eCurrentUSBMode;

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
    &g_USBHostMSCClassDriver,
    &g_sUSBEventDriver
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
// Storage for the status listbox widget string table.
//
//*****************************************************************************
const char *g_ppcStatusStrings[NUM_STATUS_STRINGS] =
{
    g_pcStatus[0],
    g_pcStatus[1],
    g_pcStatus[2],
    g_pcStatus[3],
    g_pcStatus[4],
    g_pcStatus[5]
};
unsigned long g_ulStatusStringIndex = 0;

//*****************************************************************************
//
// Forward declarations for functions called by the widgets used in the user
// interface.
//
//*****************************************************************************
void OnListBoxChange(tWidget *pWidget, short usSelected);
void OnBtnCD(tWidget *pWidget);
void OnBtnUp(tWidget *pWidget);
static int PopulateFileListBox(tBoolean bRedraw);
static FRESULT ChangeToDirectory(char *pcDirectory, unsigned long *pulReason);
static int PrintfStatus(char *pcFormat, ...);
static const char *StringFromFresult(FRESULT fresult);

//*****************************************************************************
//
// Widget definitions
//
//*****************************************************************************

//*****************************************************************************
//
// The listbox used to display directory contents.
//
//*****************************************************************************
extern tCanvasWidget g_sBackground;
extern tCanvasWidget g_sUpBackground;
extern tCanvasWidget g_sCDBackground;

ListBox(g_sStatusList, &g_sBackground, 0, 0, &g_sKitronix320x240x16_SSD2119,
        40, 170, 220, 52, (LISTBOX_STYLE_OUTLINE | LISTBOX_STYLE_LOCKED |
        LISTBOX_STYLE_WRAP), ClrBlack, ClrBlack, ClrSilver, ClrSilver, ClrWhite,
        g_pFontFixed6x8, g_ppcStatusStrings,  NUM_STATUS_STRINGS,
        NUM_STATUS_STRINGS, 0);

ListBox(g_sDirList, &g_sBackground, &g_sStatusList, 0,
        &g_sKitronix320x240x16_SSD2119,
        40, 60, 120, 100, LISTBOX_STYLE_OUTLINE, ClrBlack, ClrDarkBlue,
        ClrSilver, ClrWhite, ClrWhite, g_pFontCmss12, g_ppcDirListStrings,
        NUM_LIST_STRINGS, 0, OnListBoxChange);

//*****************************************************************************
//
// The canvas widget used to show the current directory prompt.
//
//*****************************************************************************
Canvas(g_sPWDTitle, &g_sBackground, &g_sDirList, 0,
       &g_sKitronix320x240x16_SSD2119, 10, 35, 40, 20, CANVAS_STYLE_TEXT,
       ClrBlack, 0, ClrWhite, g_pFontCmss12, "PWD:", 0, 0);

//*****************************************************************************
//
// The canvas widget used to show the current directory.
//
//*****************************************************************************
Canvas(g_sPWD, &g_sBackground, &g_sPWDTitle, 0, &g_sKitronix320x240x16_SSD2119,
       50, 35, 260, 20, (CANVAS_STYLE_TEXT | CANVAS_STYLE_FILL |
       CANVAS_STYLE_TEXT_LEFT), ClrBlack, 0, ClrWhite, g_pFontCmss12,
       g_cCwdBuf, 0, 0);

//*****************************************************************************
//
// The button used to change to the selected directory directory.
//
//*****************************************************************************
RectangularButton(g_sCDBtn, &g_sCDBackground, 0, 0,
                  &g_sKitronix320x240x16_SSD2119, 170, 75, 90, 30,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrBlack, ClrBlue, ClrWhite, ClrWhite,
                   g_pFontCm20, "CD", 0, 0, 0, 0, OnBtnCD);

//*****************************************************************************
//
// The canvas widget acting as the background to the CD button.
//
//*****************************************************************************
Canvas(g_sCDBackground, WIDGET_ROOT, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 170, 75, 90, 30,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The button used to change to the next higher directory.
//
//*****************************************************************************
RectangularButton(g_sUpBtn, &g_sUpBackground, 0, 0,
                  &g_sKitronix320x240x16_SSD2119, 170, 115, 90, 30,
                  (PB_STYLE_OUTLINE | PB_STYLE_TEXT_OPAQUE | PB_STYLE_TEXT |
                   PB_STYLE_FILL | PB_STYLE_RELEASE_NOTIFY),
                   ClrBlack, ClrBlue, ClrWhite, ClrWhite,
                   g_pFontCm20, "Up", 0, 0, 0, 0, OnBtnUp);

//*****************************************************************************
//
// The canvas widget acting as the background to the UP button.
//
//*****************************************************************************
Canvas(g_sUpBackground, WIDGET_ROOT, &g_sCDBackground, 0,
       &g_sKitronix320x240x16_SSD2119, 170, 115, 90, 30,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The canvas widget acting as the background to the left portion of the
// display.
//
//*****************************************************************************
Canvas(g_sBackground, WIDGET_ROOT, &g_sUpBackground, &g_sPWD,
       &g_sKitronix320x240x16_SSD2119, 10, 60, 120, 230,
       CANVAS_STYLE_FILL, ClrBlack, 0, 0, 0, 0, 0, 0);

//*****************************************************************************
//
// The heading containing the application title.
//
//*****************************************************************************
Canvas(g_sHeading, WIDGET_ROOT, &g_sBackground, 0,
       &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 23,
       (CANVAS_STYLE_FILL | CANVAS_STYLE_OUTLINE | CANVAS_STYLE_TEXT),
       ClrDarkBlue, ClrWhite, ClrWhite, g_pFontCm20, "usb-host-msc", 0, 0);

//*****************************************************************************
//
// The listbox widget callback function.
//
// This function is called whenever someone changes the selected entry in the
// listbox containing the files and directories found in the current directory.
//
//*****************************************************************************
void OnListBoxChange(tWidget *pWidget, short usSelected)
{
    short sSelected;

    //
    // If no USB drive is present, just ignore this.
    //
    if(g_eState != STATE_DEVICE_READY)
    {
        return;
    }

    //
    // Get the current selection from the list box.
    //
    sSelected = ListBoxSelectionGet(&g_sDirList);

    //
    // Is there any selection?
    //
    if(sSelected == -1)
    {
        return;
    }
    else
    {
        //
        // Is the selection a directory name?
        //
        if(g_pcFilenames[sSelected][1] == 'D')
        {
            //
            // Enable the "CD" button.
            //
            WidgetAdd((tWidget *)&g_sCDBackground, (tWidget *)&g_sCDBtn);
        }
        else
        {
            //
            // Hide the "CD" button.
            //
            WidgetRemove((tWidget *)&g_sCDBtn);
        }

        //
        // Make sure the CD button (or its background) is drawn correctly).
        //
        WidgetPaint((tWidget *)&g_sCDBackground);
    }

    //
    // Update the status display to say what we've done.
    //
    PrintfStatus("Selected %s %s\n",
                 (g_pcFilenames[sSelected][1] == 'D') ? "dir" : "file",
                 &g_pcFilenames[sSelected][4]);
}

//*****************************************************************************
//
// The "CD" button widget callback function.
//
// This function is called whenever someone presses the "CD" button.
//
//*****************************************************************************
void OnBtnCD(tWidget *pWidget)
{
    short sSelected;
    unsigned long ulReason;
    FRESULT fresult;
    //
    // Get the current selection from the list box.
    //
    sSelected = ListBoxSelectionGet(&g_sDirList);

    //
    // Is there any selection?
    //
    if(sSelected == -1)
    {
        return;
    }
    else
    {
        //
        // Is the selection a directory name?
        //
        if(g_pcFilenames[sSelected][1] == 'D')
        {
            //
            // Yes - change to the new directory.
            //
            fresult = ChangeToDirectory(&g_pcFilenames[sSelected][4],
                                        &ulReason);

            if(fresult != FR_OK)
            {
                //
                // Update the status display to show the error.
                //
                PrintfStatus("Error changing directory.");
                PrintfStatus((char *)StringFromFresult(fresult));
            }
            else
            {
                //
                // Tell the user what happened.
                //
                PrintfStatus("Changed to %s", g_cCwdBuf);

                //
                // Update the directory name and the list box contents.
                //
                PopulateFileListBox(true);
                WidgetPaint((tWidget *)&g_sPWD);

                //
                // Enable the "Up" button and disable the "CD" button.
                //
                WidgetAdd((tWidget *)&g_sUpBackground, (tWidget *)&g_sUpBtn);
                WidgetRemove((tWidget *)&g_sCDBtn);

                //
                // Make sure the buttons are repainted correctly.
                //
                WidgetPaint((tWidget *)&g_sUpBtn);
                WidgetPaint((tWidget *)&g_sCDBackground);
            }
        }
    }
}

//*****************************************************************************
//
// The "Up" button widget callback function.
//
// This function is called whenever someone presses the "Up" button.
//
//*****************************************************************************
void OnBtnUp(tWidget *pWidget)
{
    unsigned long ulReason;
    FRESULT fresult;

    //
    // Change up one directory.
    //
    fresult = ChangeToDirectory("..", &ulReason);

    if(fresult != FR_OK)
    {
        //
        // Update the status display to show the error.
        //
        PrintfStatus("Error changing directory.");
        PrintfStatus((char *)StringFromFresult(fresult));
    }
    else
    {
        //
        // Update the directory name and the list box contents.
        //
        WidgetPaint((tWidget *)&g_sPWD);
        PopulateFileListBox(true);

        //
        // If we are now in the root directory, hide the "Up" button.
        //
        if((strlen(g_cCwdBuf) == 1) && (g_cCwdBuf[0] == '/'))
        {
            WidgetRemove((tWidget *)&g_sUpBtn);
        }
        else
        {
            WidgetAdd((tWidget *)&g_sUpBackground, (tWidget *)&g_sUpBtn);
        }

        //
        // Disable the CD button since re-populating the list removes the
        // selection.
        //
        WidgetRemove((tWidget *)&g_sCDBtn);

        //
        // Tell the user what happened.
        //
        PrintfStatus("Changed to %s", g_cCwdBuf);

        //
        // Repaint the buttons.
        //
        WidgetPaint((tWidget *)&g_sUpBackground);
        WidgetPaint((tWidget *)&g_sCDBackground);
    }
}

//*****************************************************************************
//
// This function is used to add a new string to the status list box at the
// bottom of the display.  This shows errors and echos user commands entered
// via the UART.
//
//*****************************************************************************
static int
PrintfStatus(char *pcFormat, ...)
{
    int iRet;
    va_list vaArgP;

    //
    // Start the varargs processing.
    //
    va_start(vaArgP, pcFormat);

    //
    // Call vsnprintf to perform the conversion.
    //
    iRet = uvsnprintf(g_pcStatus[g_ulStatusStringIndex], MAX_STATUS_STRING_LEN,
                      pcFormat, vaArgP);

    //
    // End the varargs processing.
    //
    va_end(vaArgP);

    //
    // Add the new string to the status listbox.
    //
    ListBoxTextAdd(&g_sStatusList, g_pcStatus[g_ulStatusStringIndex]);

    //
    // Update our string index.
    //
    g_ulStatusStringIndex++;
    if(g_ulStatusStringIndex == NUM_STATUS_STRINGS)
    {
        g_ulStatusStringIndex = 0;
    }

    //
    // Repaint the status listbox.
    //
    WidgetPaint((tWidget *)&g_sStatusList);

    //
    // Return the conversion count.
    //
    return(iRet);

}

//*****************************************************************************
//
// This function returns a string representation of an error code
// that was returned from a function call to FatFs.  It can be used
// for printing human readable error messages.
//
//*****************************************************************************
static const char *
StringFromFresult(FRESULT fresult)
{
    unsigned int uIdx;

    //
    // Enter a loop to search the error code table for a matching
    // error code.
    //
    for(uIdx = 0; uIdx < NUM_FRESULT_CODES; uIdx++)
    {
        //
        // If a match is found, then return the string name of the
        // error code.
        //
        if(g_sFresultStrings[uIdx].fresult == fresult)
        {
            return(g_sFresultStrings[uIdx].pcResultStr);
        }
    }

    //
    // At this point no matching code was found, so return a
    // string indicating unknown error.
    //
    return("UNKNOWN ERROR CODE");
}

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.  FatFs requires a
// timer tick every 10 ms for internal timing purposes.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Update our tick counter.
    //
    g_ulSysTickCount++;
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
// USB Mode callback
//
// \param ulIndex is the zero-based index of the USB controller making the
//        callback.
// \param eMode indicates the new operating mode.
//
// This function is called by the USB library whenever an OTG mode change
// occurs and, if a connection has been made, informs us of whether we are to
// operate as a host or device.
//
// \return None.
//
//*****************************************************************************
void
ModeCallback(unsigned long ulIndex, tUSBMode eMode)
{
    //
    // Save the new mode.
    //

    g_eCurrentUSBMode = eMode;

    switch(eMode)
    {
        case USB_MODE_HOST:
        {
            DEBUG_PRINT("\nHost Mode.\n");
            break;
        }
        case USB_MODE_DEVICE:
        {
            DEBUG_PRINT("\nDevice Mode.\n");
            break;
        }
        case USB_MODE_NONE:
        {
            DEBUG_PRINT("\nIdle Mode.\n");
            break;
        }
        default:
        {
            DEBUG_PRINT("ERROR: Bad Mode!\n");
            break;
        }
    }
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
// \return None
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
        // An unknown device has been connected.
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
        // The unknown device has been been unplugged.
        //
        case USB_EVENT_DISCONNECTED:
        {
            //
            // Unknown device has been removed.
            //
            g_eState = STATE_NO_DEVICE;

            break;
        }

        //
        // A bus power fault was detected.
        //
        case USB_EVENT_POWER_FAULT:
        {
            //
            // No power means no device is present.
            //
            g_eState = STATE_POWER_FAULT;
            DEBUG_PRINT("Power fault");

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
// This function implements the "ls" command.  It opens the current
// directory and enumerates through the contents, and prints a line for
// each item it finds.  It shows details such as file attributes, time and
// date, and the file size, along with the name.  It shows a summary of
// file sizes at the end along with free space.
//
//*****************************************************************************
int
Cmd_ls(int argc, char *argv[])
{
    unsigned long ulTotalSize, ulItemCount, ulFileCount, ulDirCount;
    FRESULT fresult;
    FATFS *pFatFs;

    //
    // Empty the list box on the display.
    //
    ListBoxClear(&g_sDirList);

    //
    // Make sure the list box will be redrawn next time the message queue
    // is processed.
    //
    WidgetPaint((tWidget *)&g_sDirList);

    //
    // Open the current directory for access.
    //
    fresult = f_opendir(&g_sDirObject, g_cCwdBuf);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        //
        // Ensure that the error is reported.
        //
        ListBoxTextAdd(&g_sDirList, "Error from SD Card:");
        ListBoxTextAdd(&g_sDirList, (char *)StringFromFresult(fresult));
        return(fresult);
    }

    ulTotalSize = 0;
    ulFileCount = 0;
    ulDirCount = 0;
    ulItemCount = 0;

    //
    // Give an extra blank line before the listing.
    //
    UARTprintf("\n");

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
    {
        //
        // Read an entry from the directory.
        //
        fresult = f_readdir(&g_sDirObject, &g_sFileInfo);

        //
        // Check for error and return if there is a problem.
        //
        if(fresult != FR_OK)
        {
            return(fresult);
        }

        //
        // If the file name is blank, then this is the end of the
        // listing.
        //
        if(!g_sFileInfo.fname[0])
        {
            break;
        }

        //
        // Print the entry information on a single line with formatting
        // to show the attributes, date, time, size, and name.
        //
        UARTprintf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s\n",
                 (g_sFileInfo.fattrib & AM_DIR) ? 'D' : '-',
                 (g_sFileInfo.fattrib & AM_RDO) ? 'R' : '-',
                 (g_sFileInfo.fattrib & AM_HID) ? 'H' : '-',
                 (g_sFileInfo.fattrib & AM_SYS) ? 'S' : '-',
                 (g_sFileInfo.fattrib & AM_ARC) ? 'A' : '-',
                 (g_sFileInfo.fdate >> 9) + 1980,
                 (g_sFileInfo.fdate >> 5) & 15,
                 g_sFileInfo.fdate & 31,
                 (g_sFileInfo.ftime >> 11),
                 (g_sFileInfo.ftime >> 5) & 63,
                 g_sFileInfo.fsize,
                 g_sFileInfo.fname);

        //
        // Add the information as a line in the listbox widget.
        //
        if(ulItemCount < NUM_LIST_STRINGS)
        {
            usprintf(g_pcFilenames[ulItemCount], "(%c) %12s",
                     (g_sFileInfo.fattrib & AM_DIR) ? 'D' : 'F',
                      g_sFileInfo.fname);
            ListBoxTextAdd(&g_sDirList, g_pcFilenames[ulItemCount]);
        }

        //
        // If the attribute is directory, then increment the directory count.
        //
        if(g_sFileInfo.fattrib & AM_DIR)
        {
            ulDirCount++;
        }

        //
        // Otherwise, it is a file.  Increment the file count, and
        // add in the file size to the total.
        //
        else
        {
            ulFileCount++;
            ulTotalSize += g_sFileInfo.fsize;
        }

        //
        // Move to the next entry in the item array we use to populate the
        // list box.
        //
        ulItemCount++;

        //
        // Wait for the UART transmit buffer to empty.
        //
        UARTFlushTx(false);

    }

    //
    // Print summary lines showing the file, dir, and size totals.
    //
    UARTprintf("\n%4u File(s),%10u bytes total\n%4u Dir(s)",
                ulFileCount, ulTotalSize, ulDirCount);

    //
    // Get the free space.
    //
    fresult = f_getfree("/", &ulTotalSize, &pFatFs);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        return(fresult);
    }

    //
    // Display the amount of free space that was calculated.
    //
    UARTprintf(", %10uK bytes free\n", ulTotalSize * pFatFs->sects_clust / 2);

    //
    // Wait for the UART transmit buffer to empty.
    //
    UARTFlushTx(false);

    //
    // Made it to here, return with no errors.
    //
    return(0);
}

//*****************************************************************************
//
// This function is called to read the contents of the current directory on
// the SD card and fill the listbox containing the names of all files and
// directories.
//
//*****************************************************************************
static int
PopulateFileListBox(tBoolean bRepaint)
{
    unsigned long ulItemCount;
    FRESULT fresult;

    //
    // Empty the list box on the display.
    //
    ListBoxClear(&g_sDirList);

    //
    // Make sure the list box will be redrawn next time the message queue
    // is processed.
    //
    if(bRepaint)
    {
        WidgetPaint((tWidget *)&g_sDirList);
    }

    //
    // Open the current directory for access.
    //
    fresult = f_opendir(&g_sDirObject, g_cCwdBuf);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        //
        // Ensure that the error is reported.
        //
        PrintfStatus("Error from USB disk:");
        PrintfStatus((char *)StringFromFresult(fresult));
        return(fresult);
    }

    ulItemCount = 0;

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
    {
        //
        // Read an entry from the directory.
        //
        fresult = f_readdir(&g_sDirObject, &g_sFileInfo);

        //
        // Check for error and return if there is a problem.
        //
        if(fresult != FR_OK)
        {
            PrintfStatus("Error from USB disk:");
            PrintfStatus((char *)StringFromFresult(fresult));
            return(fresult);
        }

        //
        // If the file name is blank, then this is the end of the
        // listing.
        //
        if(!g_sFileInfo.fname[0])
        {
            break;
        }

        //
        // Add the information as a line in the listbox widget.
        //
        if(ulItemCount < NUM_LIST_STRINGS)
        {
            usnprintf(g_pcFilenames[ulItemCount], MAX_FILENAME_STRING_LEN,
                      "(%c) %s", (g_sFileInfo.fattrib & AM_DIR) ? 'D' : 'F',
                      g_sFileInfo.fname);
            ListBoxTextAdd(&g_sDirList, g_pcFilenames[ulItemCount]);
        }

        //
        // Move to the next entry in the item array we use to populate the
        // list box.
        //
        ulItemCount++;
    }

    //
    // Made it to here, return with no errors.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "cd" command.  It takes an argument
// that specifies the directory to make the current working directory.
// Path separators must use a forward slash "/".  The argument to cd
// can be one of the following:
// * root ("/")
// * a fully specified path ("/my/path/to/mydir")
// * a single directory name that is in the current directory ("mydir")
// * parent directory ("..")
//
// It does not understand relative paths, so dont try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory
// to make sure it exists.  If the new path is opened successfully, then
// the current working directory (cwd) is changed to the new path.
//
// In cases of error, the pulReason parameter will be written with one of
// the following values:
//
//*****************************************************************************
static FRESULT
ChangeToDirectory(char *pcDirectory, unsigned long *pulReason)
{
    unsigned int uIdx;
    FRESULT fresult;

    //
    // Copy the current working path into a temporary buffer so
    // it can be manipulated.
    //
    strcpy(g_cTmpBuf, g_cCwdBuf);

    //
    // If the first character is /, then this is a fully specified
    // path, and it should just be used as-is.
    //
    if(pcDirectory[0] == '/')
    {
        //
        // Make sure the new path is not bigger than the cwd buffer.
        //
        if(strlen(pcDirectory) + 1 > sizeof(g_cCwdBuf))
        {
            *pulReason = NAME_TOO_LONG_ERROR;
            return(FR_OK);
        }

        //
        // If the new path name (in argv[1])  is not too long, then
        // copy it into the temporary buffer so it can be checked.
        //
        else
        {
            strncpy(g_cTmpBuf, pcDirectory, sizeof(g_cTmpBuf));
        }
    }

    //
    // If the argument is .. then attempt to remove the lowest level
    // on the CWD.
    //
    else if(!strcmp(pcDirectory, ".."))
    {
        //
        // Get the index to the last character in the current path.
        //
        uIdx = strlen(g_cTmpBuf) - 1;

        //
        // Back up from the end of the path name until a separator (/)
        // is found, or until we bump up to the start of the path.
        //
        while((g_cTmpBuf[uIdx] != '/') && (uIdx > 1))
        {
            //
            // Back up one character.
            //
            uIdx--;
        }

        //
        // Now we are either at the lowest level separator in the
        // current path, or at the beginning of the string (root).
        // So set the new end of string here, effectively removing
        // that last part of the path.
        //
        g_cTmpBuf[uIdx] = 0;
    }

    //
    // Otherwise this is just a normal path name from the current
    // directory, and it needs to be appended to the current path.
    //
    else
    {
        //
        // Test to make sure that when the new additional path is
        // added on to the current path, there is room in the buffer
        // for the full new path.  It needs to include a new separator,
        // and a trailing null character.
        //
        if(strlen(g_cTmpBuf) + strlen(pcDirectory) + 1 + 1 > sizeof(g_cCwdBuf))
        {
            *pulReason = NAME_TOO_LONG_ERROR;
            return(FR_INVALID_OBJECT);
        }

        //
        // The new path is okay, so add the separator and then append
        // the new directory to the path.
        //
        else
        {
            //
            // If not already at the root level, then append a /
            //
            if(strcmp(g_cTmpBuf, "/"))
            {
                strcat(g_cTmpBuf, "/");
            }

            //
            // Append the new directory to the path.
            //
            strcat(g_cTmpBuf, pcDirectory);
        }
    }

    //
    // At this point, a candidate new directory path is in chTmpBuf.
    // Try to open it to make sure it is valid.
    //
    fresult = f_opendir(&g_sDirObject, g_cTmpBuf);

    //
    // If it cant be opened, then it is a bad path.  Inform
    // user and return.
    //
    if(fresult != FR_OK)
    {
        *pulReason = OPENDIR_ERROR;
        return(fresult);
    }

    //
    // Otherwise, it is a valid new path, so copy it into the CWD and update
    // the screen.
    //
    else
    {
        strncpy(g_cCwdBuf, g_cTmpBuf, sizeof(g_cCwdBuf));
    }

    //
    // Return success.
    //
    return(FR_OK);
}

//*****************************************************************************
//
// This function implements the "cd" command.  It takes an argument
// that specifies the directory to make the current working directory.
// Path separators must use a forward slash "/".  The argument to cd
// can be one of the following:
// * root ("/")
// * a fully specified path ("/my/path/to/mydir")
// * a single directory name that is in the current directory ("mydir")
// * parent directory ("..")
//
// It does not understand relative paths, so don't try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory
// to make sure it exists.  If the new path is opened successfully, then
// the current working directory (cwd) is changed to the new path.
//
//*****************************************************************************
int
Cmd_cd(int argc, char *argv[])
{
    unsigned long ulReason;
    FRESULT fresult;

    //
    // Try to change to the directory provided on the command line.
    //
    fresult = ChangeToDirectory(argv[1], &ulReason);

    //
    // If an error was reported, try to offer some helpful information.
    //
    if(fresult != FR_OK)
    {
        switch(ulReason)
        {
            case OPENDIR_ERROR:
                UARTprintf("Error opening new directory.\n");
                PrintfStatus("Error opening new directory.\n");
                break;

            case NAME_TOO_LONG_ERROR:
                UARTprintf("Resulting path name is too long.\n");
                PrintfStatus("Resulting path name is too long.\n");
                break;

            default:
                UARTprintf("An unrecognized error was reported.\n");
                PrintfStatus("An unrecognized error was reported.\n");
                break;
        }
    }
    else
    {
        //
        // Tell the user what happened.
        //
        PrintfStatus("Changed to %s", g_cCwdBuf);

        //
        // Update the contents of the file list.
        //
        PopulateFileListBox(true);

        //
        // Update the current directory on the display.
        //
        WidgetPaint((tWidget *)&g_sPWD);

        //
        // Enable the "Up" button if we are no longer in the root directory.
        //
        if((g_cCwdBuf[0] == '/') && (g_cCwdBuf[1] == '\0'))
        {
            //
            // We are in the root directory so disable the "Up" button.
            //
            WidgetRemove((tWidget *)&g_sUpBtn);
        }
        else
        {
            //
            // Enable the "Up" button.
            //
            WidgetAdd((tWidget *)&g_sUpBackground, (tWidget *)&g_sUpBtn);
        }

        //
        // Make sure the "Up" button is repainted correctly.
        //
        WidgetPaint((tWidget *)&g_sUpBackground);
    }

    //
    // Return the appropriate error code.
    //
    return(fresult);
}

//*****************************************************************************
//
// This function implements the "pwd" command.  It simply prints the
// current working directory.
//
//*****************************************************************************
int
Cmd_pwd(int argc, char *argv[])
{
    //
    // Print the CWD to the console.
    //
    UARTprintf("%s\n", g_cCwdBuf);

    //
    // Wait for the UART transmit buffer to empty.
    //
    UARTFlushTx(false);

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "cat" command.  It reads the contents of
// a file and prints it to the console.  This should only be used on
// text files.  If it is used on a binary file, then a bunch of garbage
// is likely to printed on the console.
//
//*****************************************************************************
int
Cmd_cat(int argc, char *argv[])
{
    FRESULT fresult;
    unsigned short usBytesRead;

    //
    // First, check to make sure that the current path (CWD), plus
    // the file name, plus a separator and trailing null, will all
    // fit in the temporary buffer that will be used to hold the
    // file name.  The file name must be fully specified, with path,
    // to FatFs.
    //
    if(strlen(g_cCwdBuf) + strlen(argv[1]) + 1 + 1 > sizeof(g_cTmpBuf))
    {
        UARTprintf("Resulting path name is too long\n");
        return(0);
    }

    //
    // Copy the current path to the temporary buffer so it can be manipulated.
    //
    strcpy(g_cTmpBuf, g_cCwdBuf);

    //
    // If not already at the root level, then append a separator.
    //
    if(strcmp("/", g_cCwdBuf))
    {
        strcat(g_cTmpBuf, "/");
    }

    //
    // Now finally, append the file name to result in a fully specified file.
    //
    strcat(g_cTmpBuf, argv[1]);

    //
    // Open the file for reading.
    //
    fresult = f_open(&g_sFileObject, g_cTmpBuf, FA_READ);

    //
    // If there was some problem opening the file, then return
    // an error.
    //
    if(fresult != FR_OK)
    {
        return(fresult);
    }

    //
    // Enter a loop to repeatedly read data from the file and display it,
    // until the end of the file is reached.
    //
    do
    {
        //
        // Read a block of data from the file.  Read as much as can fit
        // in the temporary buffer, including a space for the trailing null.
        //
        fresult = f_read(&g_sFileObject, g_cTmpBuf, sizeof(g_cTmpBuf) - 1,
                         &usBytesRead);

        //
        // If there was an error reading, then print a newline and
        // return the error to the user.
        //
        if(fresult != FR_OK)
        {
            UARTprintf("\n");
            return(fresult);
        }

        //
        // Null terminate the last block that was read to make it a
        // null terminated string that can be used with printf.
        //
        g_cTmpBuf[usBytesRead] = 0;

        //
        // Print the last chunk of the file that was received.
        //
        UARTprintf("%s", g_cTmpBuf);

        //
        // Wait for the UART transmit buffer to empty.
        //
        UARTFlushTx(false);

    //
    // Continue reading until less than the full number of bytes are
    // read.  That means the end of the buffer was reached.
    //
    }
    while(usBytesRead == sizeof(g_cTmpBuf) - 1);

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "help" command.  It prints a simple list
// of the available commands with a brief description.
//
//*****************************************************************************
int
Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    //
    // Print some header text.
    //
    UARTprintf("\nAvailable commands\n");
    UARTprintf("------------------\n");

    //
    // Point at the beginning of the command table.
    //
    pEntry = &g_sCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The
    // end of the table has been reached when the command name is NULL.
    //
    while(pEntry->pcCmd)
    {
        //
        // Print the command name and the brief description.
        //
        UARTprintf("%s%s\n", pEntry->pcCmd, pEntry->pcHelp);

        //
        // Advance to the next entry in the table.
        //
        pEntry++;

        //
        // Wiat for the UART to catch up.
        //
        UARTFlushTx(false);
    }

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This is the table that holds the command names, implementing functions,
// and brief description.
//
//*****************************************************************************
tCmdLineEntry g_sCmdTable[] =
{
    { "help",   Cmd_help,      " : Display list of commands" },
    { "h",      Cmd_help,   "    : alias for help" },
    { "?",      Cmd_help,   "    : alias for help" },
    { "ls",     Cmd_ls,      "   : Display list of files" },
    { "chdir",  Cmd_cd,         ": Change directory" },
    { "cd",     Cmd_cd,      "   : alias for chdir" },
    { "pwd",    Cmd_pwd,      "  : Show current working directory" },
    { "cat",    Cmd_cat,      "  : Show contents of a text file" },
    { 0, 0, 0 }
};

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
// This function returns the number of ticks since the last time this function
// was called.
//
//*****************************************************************************
unsigned long
GetTickms(void)
{
    unsigned long ulRetVal;
    unsigned long ulSaved;

    ulRetVal = g_ulSysTickCount;
    ulSaved = ulRetVal;

    if(ulSaved > g_ulLastTick)
    {
        ulRetVal = ulSaved - g_ulLastTick;
    }
    else
    {
        ulRetVal = g_ulLastTick - ulSaved;
    }

    //
    // This could miss a few milliseconds but the timings here are on a
    // much larger scale.
    //
    g_ulLastTick = ulSaved;

    //
    // Return the number of milliseconds since the last time this was called.
    //
    return(ulRetVal * MS_PER_SYSTICK);
}

//*****************************************************************************
//
// The program main function.  It performs initialization, then runs
// a command processing loop to read commands from the console.
//
//*****************************************************************************
int
main(void)
{
    int nStatus;
    tUSBMode eLastMode;
    char *pcString;
    unsigned long ulDriveTimeout;

    //
    // Set the system clock to run at 50MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Initially wait for device connection.
    //
    g_eState = STATE_NO_DEVICE;
    eLastMode = USB_MODE_OTG;

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable the uDMA controller and set up the control table base.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    ROM_uDMAEnable();
    ROM_uDMAControlBaseSet(g_sDMAControlTable);

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Set GPIO A0 and A1 as UART.
    //
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART as a console for text I/O.
    //
    UARTStdioInit(0);

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sHeading);

    //
    // Set some initial strings.
    //
    ListBoxTextAdd(&g_sDirList, "Waiting for device...");

    //
    // Issue the initial paint request to the widgets then immediately call
    // the widget manager to process the paint message.  This ensures that the
    // display is drawn as quickly as possible and saves the delay we would
    // otherwise experience if we processed the paint message after mounting
    // and reading the SD card.
    //
    WidgetPaint(WIDGET_ROOT);
    WidgetMessageQueueProcess();

    //
    // Print hello message to user.
    //
    UARTprintf("\n\nUSB Mass Storage Class Host Example Program\n");
    UARTprintf("Type \'help\' for help.\n");

    //
    // Configure the required pins for USB operation.
    //
    ROM_GPIOPinTypeUSBDigital(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    ROM_GPIOPinTypeUSBDigital(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the USB stack mode and pass in a mode callback.
    //
    USBStackModeSet(0, USB_MODE_OTG, ModeCallback);

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
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Initialize the USB controller for OTG operation with a 2ms polling
    // rate.
    //
    USBOTGModeInit(0, 2000, g_pHCDPool, HCD_MEMORY_SIZE);

    //
    // Initialize the file system.
    //
    FileInit();

    //
    // Enter an (almost) infinite loop for reading and processing commands from
    // the user.
    //
    while(1)
    {
        //
        // Print a prompt to the console.  Show the CWD.
        //
        UARTprintf("\n%s> ", g_cCwdBuf);

        //
        // Is there a command waiting to be processed?
        //
        while(UARTPeek('\r') < 0)
        {
            //
            // No command is waiting yet - go ahead and do housekeeping.
            //

            //
            // Tell the OTG library code how much time has passed in
            // milliseconds since the last call.
            //
            USBOTGMain(GetTickms());

            //
            // Has the USB mode changed since last time we checked?
            //
            if(g_eCurrentUSBMode != eLastMode)
            {
                //
                // Remember the new mode.
                //
                eLastMode = g_eCurrentUSBMode;

                switch(eLastMode)
                {
                    case USB_MODE_HOST:
                        pcString = "HOST";
                        break;

                    case USB_MODE_DEVICE:
                        pcString = "DEVICE";
                        break;

                    case USB_MODE_NONE:
                        pcString = "NONE";
                        break;

                    default:
                        pcString = "UNKNOWN";
                        break;
                }

                UARTprintf("USB mode changed to %s\n", pcString);
            }

            //
            // Process any messages in the widget message queue.
            //
            WidgetMessageQueueProcess();

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
                        SysCtlDelay(SysCtlClockGet()/(3*2));

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
                    g_cCwdBuf[0] = '/';
                    g_cCwdBuf[1] = 0;

                    //
                    // Fill the list box with the files and directories found.
                    //
                    if(!PopulateFileListBox(true))
                    {
                        //
                        // If there were no errors reported, we are ready for
                        // MSC operation.
                        //
                        g_eState = STATE_DEVICE_READY;
                    }

                    //
                    // Set the Device Present flag.
                    //
                    g_ulFlags = FLAGS_DEVICE_PRESENT;

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
                        // Empty the list box on the display.
                        //
                        ListBoxClear(&g_sDirList);
                        ListBoxTextAdd(&g_sDirList, "Waiting for device...");
                        WidgetPaint((tWidget *)&g_sDirList);

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
                        // Clear the screen and indicate that an unknown device
                        // is present.
                        //
                        ListBoxClear(&g_sDirList);
                        ListBoxTextAdd(&g_sDirList, "Unknown device.");
                        WidgetPaint((tWidget *)&g_sDirList);
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
                        ListBoxClear(&g_sDirList);
                        ListBoxTextAdd(&g_sDirList, "Device Timeout.");
                        WidgetPaint((tWidget *)&g_sDirList);
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

        //
        // Get a line of text from the user.
        //
        UARTgets(g_cCmdBuf, sizeof(g_cCmdBuf));

        //
        // Pass the line from the user to the command processor.
        // It will be parsed and valid commands executed.
        //
        nStatus = CmdLineProcess(g_cCmdBuf);

        //
        // Handle the case of bad command.
        //
        if(nStatus == CMDLINE_BAD_CMD)
        {
            UARTprintf("Bad command!\n");
        }

        //
        // Handle the case of too many arguments.
        //
        else if(nStatus == CMDLINE_TOO_MANY_ARGS)
        {
            UARTprintf("Too many arguments for command processor!\n");
        }

        //
        // Otherwise the command was executed.  Print the error
        // code if one was returned.
        //
        else if(nStatus != 0)
        {
            UARTprintf("Command returned error code %s\n",
                        StringFromFresult((FRESULT)nStatus));
        }
    }
}
