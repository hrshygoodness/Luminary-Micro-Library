//*****************************************************************************
//
// usb_host_msc.c - Example program for reading files from a USB flash drive.
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

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/udma.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "utils/ustdlib.h"
#include "usblib/usblib.h"
#include "usblib/usbmsc.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"
#include "drivers/cfal96x64x16.h"
#include "drivers/buttons.h"
#include "drivers/slidemenuwidget.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Mass Storage Class Host Example (usb_host_msc)</h1>
//!
//! This example application demonstrates reading a file system from
//! a USB flash disk.  It makes use of FatFs, a FAT file system driver.  It
//! provides a simple widget-based display for showing and navigating the file
//! system on a USB stick.
//!
//! For additional details about FatFs, see the following site:
//! http://elm-chan.org/fsw/ff/00index_e.html
//
//*****************************************************************************

//*****************************************************************************
//
// Defines the number of times to call to check if the attached device is
// ready.
//
//*****************************************************************************
#define USBMSC_DRIVE_RETRY      4

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;

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
// it's name as a string.  This is used for looking up error codes and
// providing a human-readable string.
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
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES (sizeof(g_sFresultStrings) / sizeof(tFresultString))

//*****************************************************************************
//
// Error reasons returned by ChangeToDirectory().
//
//*****************************************************************************
#define NAME_TOO_LONG_ERROR 1
#define OPENDIR_ERROR       2

//*****************************************************************************
//
// The number of SysTick ticks per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100
#define MS_PER_SYSTICK (1000 / TICKS_PER_SECOND)

//*****************************************************************************
//
// A counter for system clock ticks, used for simple timing.
//
//*****************************************************************************
static unsigned long g_ulSysTickCount;

//*****************************************************************************
//
// Holds global flags for the system.
//
//*****************************************************************************
static int g_ulFlags = 0;

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
// Define a pair of buffers that are used for holding path information.
// The buffer size must be large enough to hold the longest expected
// full path name, including the file name, and a trailing null character.
// The initial path is set to root "/".
//
//*****************************************************************************
#define PATH_BUF_SIZE   80
static char g_cCwdBuf[PATH_BUF_SIZE] = "/";
static char g_cTmpBuf[PATH_BUF_SIZE];

//*****************************************************************************
//
// A set of string pointers that are used for showing status on the display.
// Five lines of text are accommodated, which is the reasonable limit for
// this display.
//
//*****************************************************************************
static char *g_pcStatusLines[5];

//*****************************************************************************
//
// A variable to track the current level in the directory tree.  The root
// level is level 0.
//
//*****************************************************************************
static unsigned long g_ulLevel;

//*****************************************************************************
//
// Declare a pair of off-screen buffers and associated display structures.
// These are used by the slide menu widget for animated menu effects.
//
//*****************************************************************************
#define OFFSCREEN_BUF_SIZE GrOffScreen4BPPSize(96, 64)
static unsigned char g_pucOffscreenBufA[OFFSCREEN_BUF_SIZE];
static unsigned char g_pucOffscreenBufB[OFFSCREEN_BUF_SIZE];
static tDisplay g_sOffscreenDisplayA;
static tDisplay g_sOffscreenDisplayB;

//*****************************************************************************
//
// Create a palette that is used by the on-screen menus and anything else that
// uses the (above) off-screen buffers.  This palette should contain any
// colors that are used by any widget using the offscreen buffers.  There can
// be up to 16 colors in this palette.
//
//*****************************************************************************
static unsigned long g_pulPalette[] =
{
    ClrBlack,
    ClrWhite,
    ClrDarkBlue,
    ClrLightBlue,
    ClrRed,
    ClrDarkGreen,
    ClrYellow,
    ClrBlue
};
#define NUM_PALETTE_ENTRIES (sizeof(g_pulPalette) / sizeof(unsigned long))

//*****************************************************************************
//
// Define the maximum number of files that can appear at any directory level.
// This is used for allocating space for holding the file information.
// Define the maximum depth of subdirectories, also used to allocating space
// for directory structures.
// Define the maximum number of characters allowed to be stored for a file
// name.
//
//*****************************************************************************
#define MAX_FILES_PER_MENU 64
#define MAX_SUBDIR_DEPTH 32
#define MAX_FILENAME_STRING_LEN 16

//*****************************************************************************
//
// Declare a set of menu items and matching strings that are used to hold
// file information.  There are two alternating sets.  Two are needed because
// the file information must be retained for the current directory, and the
// new directory (up or down the tree).
//
//*****************************************************************************
static char g_cFileNames[2][MAX_FILES_PER_MENU][MAX_FILENAME_STRING_LEN];
static tSlideMenuItem g_sFileMenuItems[2][MAX_FILES_PER_MENU];

//*****************************************************************************
//
// Declare a set of menus, one for each level of directory.
//
//*****************************************************************************
static tSlideMenu g_sFileMenus[MAX_SUBDIR_DEPTH];

//*****************************************************************************
//
// Define the slide menu widget.  This is the wigdet that is used for
// displaying the file information.
//
//*****************************************************************************
SlideMenu(g_sFileMenuWidget, WIDGET_ROOT, 0, 0, &g_sCFAL96x64x16, 0, 0, 96, 64,
          &g_sOffscreenDisplayA, &g_sOffscreenDisplayB, 16,
          ClrWhite, ClrDarkGreen, ClrBlack, &g_sFontFixed6x8, &g_sFileMenus[0],
          0);

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
// This is the handler for this SysTick interrupt.  It simply increments a
// counter that is used for timing.
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
    return("UNKNOWN ERR");
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
static tBoolean
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
// \return None
//
//*****************************************************************************
static void
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
// pvData is actually a pointer to a tEventInfo structure.
//
// This function will be called to inform the application when a USB event has
// occurred that is outside those related to the mass storage device.  At this
// point this is used to detect unsupported devices being inserted and removed.
// It is also used to inform the application when a power fault has occurred.
// This function is required when the g_USBGenericEventDriver is included in
// the host controller driver array that is passed in to the
// USBHCDRegisterDrivers() function.
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

    //
    // Process each kind of event
    //
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
// This function shows a status screen.  It draws a banner at the top of the
// screen with the name of the application, and then up to 5 lines of text
// in the remaining screen area.  The caller specifies a pointer to a set of
// strings, and the count of number of lines of text (up to 5).  This
// function attempts to vertically center the strings on the display.
//
//*****************************************************************************
static void
ShowStatusScreen(char *pcStatus[], unsigned long ulCount)
{
    tContext sContext;
    tRectangle sRect;
    unsigned int uIdx;
    long lY;

    //
    // Initialize the graphics context.
    //
    GrContextInit(&sContext, &g_sCFAL96x64x16);

    //
    // Fill the top part of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.sYMax = 9;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    //
    // Fill the rest of the display with black, to clear whatever was there
    // before.
    //
    sRect.sYMin = 10;
    sRect.sYMax = 63;
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &sRect);

    //
    // Change foreground for white text.
    //
    GrContextForegroundSet(&sContext, ClrWhite);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&sContext, g_pFontFixed6x8);
    GrStringDrawCentered(&sContext, "usb-host-msc", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 4, 0);

    //
    // Cap the max number of status lines to 5
    //
    ulCount = (ulCount > 5) ? 5 : ulCount;

    //
    // Compute the starting Y coordinate based on the number of lines.
    //
    lY = 40 - (ulCount * 5);

    //
    // Display the status lines
    //
    for(uIdx = 0; uIdx < ulCount; uIdx++)
    {
        GrStringDrawCentered(&sContext, pcStatus[uIdx], -1,
                             GrContextDpyWidthGet(&sContext) / 2, lY, 0);
        lY += 10;
    }
}

//*****************************************************************************
//
// This function is called to read the contents of the current directory from
// the USB stick and populate a set of menu items, one for each file in the
// directory.  A subdirectory within the directory counts as a file item.
//
// This function returns the number of file items that were found, or 0 if
// there is any error detected.
//
//*****************************************************************************
static unsigned long
PopulateFileList(unsigned long ulLevel)
{
    unsigned long ulItemCount;
    FRESULT fresult;

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
        g_pcStatusLines[0] = "Error from";
        g_pcStatusLines[1] = "USB disk";
        g_pcStatusLines[2] = (char *)StringFromFresult(fresult);
        ShowStatusScreen(g_pcStatusLines, 3);
        return(0);
    }

    //
    // Initialize the count of files in this directory
    //
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
            g_pcStatusLines[0] = "Error from";
            g_pcStatusLines[1] = "USB disk";
            g_pcStatusLines[2] = (char *)StringFromFresult(fresult);
            ShowStatusScreen(g_pcStatusLines, 3);
            return(0);
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
        // Add the information to a menu item
        //
        if(ulItemCount < MAX_FILES_PER_MENU)
        {
            tSlideMenuItem *pMenuItem;

            //
            // Get a pointer to the current menu item.  Use the directory
            // level to determine which of the two sets of menu items to use.
            // (ulLevel & 1].  This lets us alternate between the current
            // set of menu items and the new set (up or down the tree).
            //
            pMenuItem = &g_sFileMenuItems[ulLevel & 1][ulItemCount];

            //
            // Add the file name to the menu item
            //
            usnprintf(g_cFileNames[ulLevel & 1][ulItemCount],
                      MAX_FILENAME_STRING_LEN, "%s", g_sFileInfo.fname);
            pMenuItem->pcText = g_cFileNames[ulLevel & 1][ulItemCount];

            //
            // If this is a directory, then add the next level menu so that
            // when displayed it will be showed with a submenu option
            // (next level down in directory tree).  Otherwise it is a file
            // so clear the child menu so that there is no submenu option
            // shown.
            //
            pMenuItem->pChildMenu = (g_sFileInfo.fattrib & AM_DIR) ?
                                    &g_sFileMenus[ulLevel + 1] : 0;

            //
            // Move to the next entry in the item array we use to populate the
            // list box.
            //
            ulItemCount++;
        }
    }

    //
    // Made it to here, return the count of files that were populated.
    //
    return(ulItemCount);
}

//*****************************************************************************
//
// This function is used to change to a new directory in the file system.
// It takes a parameter that specifies the directory to make the current
// working directory.
// Path separators must use a forward slash "/".  The directory parameter
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
//  NAME_TOO_LONG_ERROR - combination of paths are too long for the buffer
//  OPENDIR_ERROR - there is some problem opening the new directory
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
    // At this point, a candidate new directory path is in cTmpBuf.
    // Try to open it to make sure it is valid.
    //
    fresult = f_opendir(&g_sDirObject, g_cTmpBuf);

    //
    // If it can't be opened, then it is a bad path.  Return an error.
    //
    if(fresult != FR_OK)
    {
        *pulReason = OPENDIR_ERROR;
        return(fresult);
    }

    //
    // Otherwise, it is a valid new path, so copy it into the CWD.
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
// Sends a button/key press message to the slide menu widget that is showing
// files.
//
//*****************************************************************************
static void
SendWidgetKeyMessage(unsigned long  ulMsg)
{
    WidgetMessageQueueAdd(WIDGET_ROOT,
                          ulMsg, (unsigned long)&g_sFileMenuWidget, 0, 1, 1);
}

//*****************************************************************************
//
// This function performs actions that are common whenever the directory
// level is changed up or down.  It populates the correct menu structure with
// the list of files in the directory.
//
//*****************************************************************************
static tBoolean
ProcessDirChange(char *pcDir, unsigned long ulLevel)
{
    FRESULT fresult;
    unsigned long ulReason;
    unsigned long ulFileCount;

    //
    // Attempt to change to the new directory.
    //
    fresult = ChangeToDirectory(pcDir, &ulReason);

    //
    // If the directory change was successful, populate the
    // list of files for the new subdirectory.
    //
    if((fresult == FR_OK) && (ulLevel < MAX_SUBDIR_DEPTH))
    {
        //
        // Get a pointer to the current menu for this CWD.
        //
        tSlideMenu *pMenu = &g_sFileMenus[ulLevel];

        //
        // Populate the menu items with the file list for the new CWD.
        //
        ulFileCount = PopulateFileList(ulLevel);

        //
        // Initialize the file menu with the list of menu items,
        // which are just files and dirs in the root directory
        //
        pMenu->pSlideMenuItems = g_sFileMenuItems[ulLevel & 1];
        pMenu->ulItems = ulFileCount;

        //
        // Set the parent directory, if there is one.  If at level 0
        // (CWD is root), then there is no parent directory.
        //
        if(ulLevel)
        {
            pMenu->pParent = &g_sFileMenus[ulLevel - 1];
        }
        else
        {
            pMenu->pParent = 0;
        }

        //
        // If we are descending into a new subdir, then initialize the other
        // menu item fields to default values.
        //
        if(ulLevel > g_ulLevel)
        {
            pMenu->ulCenterIndex = 0;
            pMenu->ulFocusIndex = 0;
            pMenu->bMultiSelectable = 0;
        }

        //
        // Return a success indication
        //
        return(true);
    }

    //
    // Directory change was not successful
    //
    else
    {
        //
        // Return failure indication
        //
        return(false);
    }
}

//*****************************************************************************
//
// The program main function.  It performs initialization, then runs a loop to
// process USB activities and operate the user interface.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulDriveTimeout;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the system clock to run at 50MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Configure the required pins for USB operation.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    ROM_GPIOPinConfigure(GPIO_PG4_USB0EPEN);
    ROM_GPIOPinTypeUSBDigital(GPIO_PORTG_BASE, GPIO_PIN_4);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable the uDMA controller and set up the control table base.
    // The uDMA controller is used by the USB library.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    ROM_uDMAEnable();
    ROM_uDMAControlBaseSet(g_sDMAControlTable);

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize the buttons driver.
    //
    ButtonsInit();

    //
    // Initialize two offscreen displays and assign the palette.  These
    // buffers are used by the slide menu widget to allow animation effects.
    //
    GrOffScreen4BPPInit(&g_sOffscreenDisplayA, g_pucOffscreenBufA, 96, 64);
    GrOffScreen4BPPPaletteSet(&g_sOffscreenDisplayA, g_pulPalette, 0,
                              NUM_PALETTE_ENTRIES);
    GrOffScreen4BPPInit(&g_sOffscreenDisplayB, g_pucOffscreenBufB, 96, 64);
    GrOffScreen4BPPPaletteSet(&g_sOffscreenDisplayB, g_pulPalette, 0,
                              NUM_PALETTE_ENTRIES);

    //
    // Show an initial status screen
    //
    g_pcStatusLines[0] = "Waiting";
    g_pcStatusLines[1] = "for device";
    ShowStatusScreen(g_pcStatusLines, 2);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sFileMenuWidget);

    //
    // Initially wait for device connection.
    //
    g_eState = STATE_NO_DEVICE;

    //
    // Initialize the USB stack for host mode.
    //
    USBStackModeSet(0, USB_MODE_HOST, 0);

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
    // Initialize the USB controller for host operation.
    //
    USBHCDInit(0, g_pHCDPool, HCD_MEMORY_SIZE);

    //
    // Initialize the file system.
    //
    FileInit();

    //
    // Enter an infinite loop to run the user interface and process USB
    // events.
    //
    while(1)
    {
        unsigned long ulLastTickCount = 0;

        //
        // Call the USB stack to keep it running.
        //
        USBHCDMain();

        //
        // Process any messages in the widget message queue.  This keeps the
        // display UI running.
        //
        WidgetMessageQueueProcess();

        //
        // Take action based on the application state.
        //
        switch(g_eState)
        {
            //
            // A device has enumerated.
            //
            case STATE_DEVICE_ENUM:
            {
                //
                // Check to see if the device is ready.  If not then stay
                // in this state and we will check it again on the next pass.
                //
                if(USBHMSCDriveReady(g_ulMSCInstance) != 0)
                {
                    //
                    // Wait about 500ms before attempting to check if the
                    // device is ready again.
                    //
                    ROM_SysCtlDelay(ROM_SysCtlClockGet()/(3));

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
                // Getting here means the device is ready.
                // Reset the CWD to the root directory.
                //
                g_cCwdBuf[0] = '/';
                g_cCwdBuf[1] = 0;

                //
                // Set the initial directory level to the root
                //
                g_ulLevel = 0;

                //
                // We need to reset the indexes of the root menu to 0, so that
                // it will start at the top of the file list, and reset the
                // slide menu widget to start with the root menu.
                //
                g_sFileMenus[g_ulLevel].ulCenterIndex = 0;
                g_sFileMenus[g_ulLevel].ulFocusIndex = 0;
                SlideMenuMenuSet(&g_sFileMenuWidget, &g_sFileMenus[g_ulLevel]);

                //
                // Initiate a directory change to the root.  This will
                // populate a menu structure representing the root directory.
                //
                if(ProcessDirChange("/", g_ulLevel))
                {
                    //
                    // If there were no errors reported, we are ready for
                    // MSC operation.
                    //
                    g_eState = STATE_DEVICE_READY;

                    //
                    // Set the Device Present flag.
                    //
                    g_ulFlags = FLAGS_DEVICE_PRESENT;

                    //
                    // Request a repaint so the file menu will be shown
                    //
                    WidgetPaint(WIDGET_ROOT);
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
                    // Show waiting message on screen
                    //
                    g_pcStatusLines[0] = "Waiting";
                    g_pcStatusLines[1] = "for device";
                    ShowStatusScreen(g_pcStatusLines, 2);

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
                    g_pcStatusLines[0] = "Unknown";
                    g_pcStatusLines[1] = "device";
                    ShowStatusScreen(g_pcStatusLines, 2);
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
                    //
                    // Clear the screen and indicate that an unknown device
                    // is present.
                    //
                    g_pcStatusLines[0] = "Device";
                    g_pcStatusLines[1] = "Timeout";
                    ShowStatusScreen(g_pcStatusLines, 2);
                }

                //
                // Set the Device Present flag.
                //
                g_ulFlags = FLAGS_DEVICE_PRESENT;

                break;
            }

            //
            // The device is ready and in use.
            //
            case STATE_DEVICE_READY:
            {
                //
                // Process occurrence of timer tick.  Check for user input
                // once each tick.
                //
                if(g_ulSysTickCount != ulLastTickCount)
                {
                    unsigned char ucButtonState;
                    unsigned char ucButtonChanged;

                    ulLastTickCount = g_ulSysTickCount;

                    //
                    // Get the current debounced state of the buttons.
                    //
                    ucButtonState = ButtonsPoll(&ucButtonChanged, 0);

                    //
                    // If select button or right button is pressed, then we
                    // are trying to descend into another directory
                    //
                    if(BUTTON_PRESSED(SELECT_BUTTON,
                                      ucButtonState, ucButtonChanged) ||
                       BUTTON_PRESSED(RIGHT_BUTTON,
                                      ucButtonState, ucButtonChanged))
                    {
                        unsigned long ulNewLevel;
                        unsigned long ulItemIdx;
                        char *pcItemName;

                        //
                        // Get a pointer to the current menu for this CWD.
                        //
                        tSlideMenu *pMenu = &g_sFileMenus[g_ulLevel];

                        //
                        // Get the highlighted index in the current file list.
                        // This is the currently highlighted file or dir
                        // on the display.  Then get the name of the file at
                        // this index.
                        //
                        ulItemIdx = SlideMenuFocusItemGet(pMenu);
                        pcItemName = pMenu->pSlideMenuItems[ulItemIdx].pcText;

                        //
                        // Make sure we are not yet past the maximum tree
                        // depth.
                        //
                        if(g_ulLevel < MAX_SUBDIR_DEPTH)
                        {
                            //
                            // Potential new level is one greater than the
                            // current level.
                            //
                            ulNewLevel = g_ulLevel + 1;

                            //
                            // Process the directory change to the new
                            // directory.  This function will populate a menu
                            // structure with the files and subdirs in the new
                            // directory.
                            //
                            if(ProcessDirChange(pcItemName, ulNewLevel))
                            {
                                //
                                // If the change was successful, then update
                                // the level.
                                //
                                g_ulLevel = ulNewLevel;

                                //
                                // Now that all the prep is done, send the
                                // KEY_RIGHT message to the widget and it will
                                // "slide" from the previous file list to the
                                // new file list of the CWD.
                                //
                                SendWidgetKeyMessage(WIDGET_MSG_KEY_RIGHT);

                            }
                        }
                    }

                    //
                    // If the UP button is pressed, just pass it to the widget
                    // which will handle scrolling the list of files.
                    //
                    if(BUTTON_PRESSED(UP_BUTTON, ucButtonState, ucButtonChanged))
                    {
                        SendWidgetKeyMessage(WIDGET_MSG_KEY_UP);
                    }

                    //
                    // If the DOWN button is pressed, just pass it to the widget
                    // which will handle scrolling the list of files.
                    //
                    if(BUTTON_PRESSED(DOWN_BUTTON, ucButtonState, ucButtonChanged))
                    {
                        SendWidgetKeyMessage(WIDGET_MSG_KEY_DOWN);
                    }

                    //
                    // If the LEFT button is pressed, then we are attempting
                    // to go up a level in the file system.
                    //
                    if(BUTTON_PRESSED(LEFT_BUTTON, ucButtonState, ucButtonChanged))
                    {
                        unsigned long ulNewLevel;

                        //
                        // Make sure we are not already at the top of the
                        // directory tree (at root).
                        //
                        if(g_ulLevel)
                        {
                            //
                            // Potential new level is one less than the
                            // current level.
                            //
                            ulNewLevel = g_ulLevel - 1;

                            //
                            // Process the directory change to the new
                            // directory.  This function will populate a menu
                            // structure with the files and subdirs in the new
                            // directory.
                            //
                            if(ProcessDirChange("..", ulNewLevel))
                            {
                                //
                                // If the change was successful, then update
                                // the level.
                                //
                                g_ulLevel = ulNewLevel;

                                //
                                // Now that all the prep is done, send the
                                // KEY_LEFT message to the widget and it will
                                // "slide" from the previous file list to the
                                // new file list of the CWD.
                                //
                                SendWidgetKeyMessage(WIDGET_MSG_KEY_LEFT);
                            }
                        }
                    }
                }
                break;
            }
            //
            // Something has caused a power fault.
            //
            case STATE_POWER_FAULT:
            {
                //
                // Clear the screen and show a power fault indication.
                //
                g_pcStatusLines[0] = "Power";
                g_pcStatusLines[1] = "fault";
                ShowStatusScreen(g_pcStatusLines, 2);
                break;
            }

            default:
            {
                break;
            }
        }
    }
}
