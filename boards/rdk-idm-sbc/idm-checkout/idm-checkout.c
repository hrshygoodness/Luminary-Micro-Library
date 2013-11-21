//*****************************************************************************
//
// idm-checkout.c - Example application for IDM-SBC board which exercises
//                  the various peripherals and subsystems on the board.
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/epi.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/udma.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/container.h"
#include "grlib/listbox.h"
#include "grlib/pushbutton.h"
#include "grlib/slider.h"
#include "utils/cmdline.h"
#include "utils/fswrapper.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"
#include "httpserver_raw/httpd.h"
#include "drivers/kitronix320x240x16_ssd2119_idm_sbc.h"
#include "drivers/sound.h"
#include "drivers/touch.h"
#include "drivers/ssiflash.h"
#include "drivers/sdram.h"
#include "drivers/set_pinout.h"
#include "idm-checkout.h"
#include "file.h"
#include "gui_widgets.h"
#include "grlib_demo.h"
#include "images.h"
#include "usb_mouse.h"
#include "imageview.h"
#include "audioplay.h"
#include "tftp.h"

///*****************************************************************************
//
//! \addtogroup example_list
//! <h1>IDM-SBC Board Checkout Application (idm-checkout)</h1>
//!
//! This widget-based application exercises many of the peripherals found on
//! the RDK-IDM-SBC reference board.  It offers the following features:
//!
//! - USB mouse support.  The application will show the state of up to
//!   three mouse buttons and a cursor position when a USB mouse is connected
//!   to the board.
//! - TFTP server.  This server allows the image stored in the 1MB serial flash
//!   device to be written and read from a remote Ethernet-connected system.
//!   The image in the serial flash is copied to SDRAM on startup and used as
//!   the source for the RAM-based web server file system.  Suitable images
//!   can be created using the makefsfile utility with the -b and -h switches.
//!   To upload a binary image to the serial flash, use the TFTP command line
//!   <code>tftp -i \<board IP\> PUT \<binary file\> eeprom</code>.
//!   To read the current image out of serial flash, use command line
//!   <code>tftp -i \<board IP\> GET eeprom \<binary file\></code>.
//!   When shipped, the serial flash on the board is empty. The file
//!   ramfs_data.bin which contains a web photo gallery is provided in the
//!   StellarisWare/boards/rdk-idm-sbc/idm-checkout directory and can be
//!   downloaded to the device using the TFTP command given above.
//! - Web server.  The lwIP TCP/IP stack is used to implement a web server
//!   which can serve files from an internal file system, a FAT file system
//!   on an installed microSD card or a file system image stored in serial
//!   flash and copied to SDRAM during initialization.  These file systems
//!   may be accessed from a web browser using URLs
//!   <code>http://\<board IP\></code>,
//!   <code>http://\<board IP\>/sdcard/\<filename\></code> and
//!   <code>http://\<board ID\>/ram/\<filename\></code> respectively where
//!   \<board IP\> is the dotted decimal IP address assigned to the board and
//!   \<filename\> indicates the particular file being requested.  Note that
//!   the web server does not open default filenames (such as index.htm) in
//!   any directory other than the root so the full path and filename must be
//!   specified.
//! - Touch screen.  The current touch coordinates are displayed whenever the
//!   screen is pressed.
//! - LED control.  A GUI widget allows control of one of the Ethernet LEDs on
//!   the board.
//! - Serial command line.  A simple command line is implemented via UART0.
//!   Connect a terminal emulator set to 115200/8/N/1 and enter "help" for
//!   information on supported commands.
//! - JPEG image viewer.  The QVGA display is used to display JPEG images
//!   from the ``images'' directory in the web server's SDRAM file system image.
//!   The user can scroll the image on the display using the touchscreen.  This
//!   demonstration relies upon the availability of the ramfs_data.bin file
//!   described under ``TFTP server'' above.  Ensure that this has been written
//!   to the serial flash and the board rebooted before trying this function.
//! - Audio player.  Uncompressed WAV files stored on the microSD card may be
//!   played back via the headphone jack on the I2S daughter board.  Available
//!   wave audio files are shown in a listbox on the left side of the display.
//!   Click the desired file then press "Play" to play it back.  Volume
//!   control is provided via a touchscreen slider shown on the right side of
//!   the display.
//!
//! This application supports remote software update over Ethernet using the
//! LM Flash Programmer application.  A firmware update is initiated using the
//! remote update request ``magic packet'' from LM Flash Programmer.
//
//*****************************************************************************

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// The buffer that holds the command line.
//
//*****************************************************************************
static char g_cCmdBuf[CMD_BUF_SIZE];

//*****************************************************************************
//
// A running counter of the number of system ticks which have elapsed since
// the system booted.
//
//*****************************************************************************
volatile unsigned long g_ulSysTickCount;

//*****************************************************************************
//
// Flags indicating to the main loop that some other function or interrupt
// handlers wants it to perform some operation on its behalf.
//
//*****************************************************************************
volatile unsigned long g_ulCommandFlags = 0;

//*****************************************************************************
//
// Flags indicating to the main loop that some other function or interrupt
// handlers wants it to perform some operation on its behalf.
//
//*****************************************************************************
#define COMMAND_TOUCH_UPDATE    0x00000001
#define COMMAND_USB_MODE_UPDATE 0x00000002
#define COMMAND_MOUSE_UPDATE    0x00000004

//*****************************************************************************
//
// The current position and state of the touchscreen pointer.
//
//*****************************************************************************
volatile long g_lPtrX = 0;
volatile long g_lPtrY = 0;
volatile tBoolean g_bPtrPressed = false;
char g_pcTouchCoordinates[SIZE_TOUCH_COORD_BUFFER] = { '\0' };

//*****************************************************************************
//
// The update period and counter for touchscreen information updates on the
// display.  This timeout is expressed in terms of system ticks (set to 100 per
// second by TICKS_PER_SECOND).
//
//*****************************************************************************
#define TOUCH_UPDATE_TICKS 20
unsigned long g_ulTouchUpdateTicks = 0;

//*****************************************************************************
//
// Flag used to signal to the main loop that a remote firmware update has been
// requested.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate;

//*****************************************************************************
//
// Storage for the MAC address represented as a string.
//
//*****************************************************************************
char g_pucMACAddrString[SIZE_MAC_ADDR_BUFFER];

//*****************************************************************************
//
// Storage for the IP address represented as a string.
//
//*****************************************************************************
char g_pucIPAddrString[SIZE_IP_ADDR_BUFFER];

//*****************************************************************************
//
// The DMA control structure table.
//
//*****************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable sDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(sDMAControlTable, 1024)
tDMAControlTable sDMAControlTable[64];
#else
tDMAControlTable sDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The LED checkbox widget callback function.
//
// This function is called whenever someone clicks the "LED" checkbox.
//
//*****************************************************************************
void
OnCheckLED(tWidget *pWidget, unsigned long bSelected)
{
    //
    // Set the state of the user LED on the board to follow the checkbox
    // selection.
    //
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, bSelected ? 0 : GPIO_PIN_2);

    //
    // Play the key click sound.
    //
    SoundPlay(g_pusKeyClick, g_ulKeyClickLen);
}

//*****************************************************************************
//
// This function is called from the main loop to update the displayed
// information relating to the touchscreen.  The display is only updated if
// something significant happened since last time the function was called.
//
//*****************************************************************************
void
ProcessCommandTouchUpdate(void)
{
    static tBoolean bLastPressed = true;

    //
    // Is the screen being pressed?
    //
    if(g_bPtrPressed)
    {
        long lX, lY;

        //
        // Copy the volatiles to keep the compiler from generating a
        // warning.
        //
        lX = g_lPtrX;
        lY = g_lPtrY;

        //
        // Format the string containing the current screen touch
        // coordinates.
        //
        usnprintf(g_pcTouchCoordinates, SIZE_TOUCH_COORD_BUFFER,
                  "(%3d,%3d)", lX, lY);
        bLastPressed = true;

        //
        // Make sure the string gets repainted if visible
        //
        if(g_ulCurrentScreen == IO_SCREEN)
        {
            WidgetPaint((tWidget *)&g_sTouchPos);
        }
    }
    else
    {
        //
        // The screen is not being pressed so remove the coordinate
        // information from the display if we have not done so already.
        //
        if(bLastPressed)
        {
            //
            // The screen is no longer being pressed.
            //
            usnprintf(g_pcTouchCoordinates, SIZE_TOUCH_COORD_BUFFER,
                      "None    ");
            bLastPressed = false;

            //
            // Make sure the screen gets repainted.
            //
            if(g_ulCurrentScreen == IO_SCREEN)
            {
                WidgetPaint((tWidget *)&g_sTouchPos);
            }
        }
    }
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
    // Update our system timer counter.
    //
    g_ulSysTickCount++;

    //
    // Call the lwIP timer.
    //
    lwIPTimer(1000 / TICKS_PER_SECOND);

    //
    // Call the file system tick timer.
    //
    fs_tick(1000 / TICKS_PER_SECOND);

    //
    // Update the touchscreen information on the display if necessary.
    //
    if(g_ulTouchUpdateTicks == 0)
    {
        //
        // Ask the main loop to update the touch count.
        //
        g_ulCommandFlags |= COMMAND_TOUCH_UPDATE;

        //
        // Reload the tick counter for the next update.
        //
        g_ulTouchUpdateTicks = TOUCH_UPDATE_TICKS;
    }
    else
    {
        //
        // Decrement the timer we use to determine when to update the touch
        // info on the display.
        //
        g_ulTouchUpdateTicks --;
    }
}

//*****************************************************************************
//
// This function prints the current IP and MAC addresses to the UART.
//
//*****************************************************************************
static void
ShowEthernetAddresses(void)
{
    UARTprintf("MAC: %s\n", g_pucMACAddrString);
    UARTprintf("IP:  %s\n", g_pucIPAddrString);
}

//*****************************************************************************
//
// This function is called by the software update module whenever a remote
// host requests to update the firmware on this board.  We set a flag that
// will cause the bootloader to be entered the next time the user enters a
// command on the console.
//
//*****************************************************************************
void SoftwareUpdateRequestCallback(void)
{
    g_bFirmwareUpdate = true;
}

//*****************************************************************************
//
// This function implements the "addr" command.  It shows the current IP
// and Ethernet MAC addresses.
//
//*****************************************************************************
int
Cmd_addr(int argc, char *argv[])
{
    //
    // Print the addresses to the UART terminal.
    //
    ShowEthernetAddresses();

    //
    // Return success (not that we will ever get here).
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
    { "addr",   Cmd_addr,      " : Show ethernet and IP addresses" },
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
    while(1)
    {
    }
}
#endif

//*****************************************************************************
//
// Intercept messages in flow from the touchscreen driver to the widget manager.
// This function allows the application to track the current pointer position
// and state before passing this on to the widget manager.
//
//*****************************************************************************
long
CheckoutPointerMessage(unsigned long ulMessage, long lX, long lY)
{

    //
    // Save the current touch position.
    //
    g_lPtrX = lX;
    g_lPtrY = lY;

    //
    // Determine whether the screen has been pressed or released.
    //
    g_bPtrPressed = (ulMessage == WIDGET_MSG_PTR_UP) ? false : true;

    //
    // Pass the message on to the widget library.
    //
    return(WidgetPointerMessage(ulMessage, lX, lY));
}

//*****************************************************************************
//
// Update the various widgets on the screen which indicate the state of the
// USB mouse.
//
//*****************************************************************************
static void
UpdateMouseWidgets(unsigned long ulFlags)
{
    tBoolean bConnected;

    //
    // Was a mouse connected or disconnected?
    //
    if(ulFlags & MOUSE_FLAG_CONNECTION)
    {
        //
        // The connection state changed.  Was this a connection or a
        // disconnection?
        //
        bConnected = USBMouseIsConnected();

        //
        // Are we connected to something?
        //
        if(bConnected)
        {
            //
            // Update the display with the current mode.
            //
            CanvasTextSet(&g_sModeString, g_pcMouseModes[MOUSE_MODE_STR_HOST]);

            //
            // Update the mode string if it is visible.
            //
            if(g_ulCurrentScreen == IO_SCREEN)
            {
                WidgetPaint((tWidget *)&g_sModeString);
            }

            //
            // Force ourselves to update the position and button states.
            //
            ulFlags |= (MOUSE_FLAG_POSITION | MOUSE_FLAG_BUTTONS);

            //
            // Update the status.
            //
            PrintfStatus("Mouse connected.");
        }
        else
        {
            //
            // Update the status.
            //
            PrintfStatus("Mouse disconnected.");

            //
            // No mouse is connected.
            //
            CanvasTextSet(&g_sModeString, g_pcMouseModes[MOUSE_MODE_STR_NONE]);
            CanvasImageSet(&g_sMouseBtn1, g_pucGreyLED14x14Image);
            CanvasImageSet(&g_sMouseBtn2, g_pucGreyLED14x14Image);
            CanvasImageSet(&g_sMouseBtn3, g_pucGreyLED14x14Image);

            //
            // Disable the mouse position and button indicators.
            //
            g_pcMousePos[0] = (char)0;
            if(g_ulCurrentScreen == IO_SCREEN)
            {
                WidgetPaint((tWidget *)&g_sMousePos);
                WidgetPaint((tWidget *)&g_sMouseBtn1);
                WidgetPaint((tWidget *)&g_sMouseBtn2);
                WidgetPaint((tWidget *)&g_sMouseBtn3);
                WidgetPaint((tWidget *)&g_sModeString);
            }
            //
            // Return here since we obviously can't have pointer movement or
            // button presses if there's no mouse connected.
            //
            return;
        }
    }

    //
    // Has the position changed?
    //
    if(ulFlags & MOUSE_FLAG_POSITION)
    {
        short sX, sY;

        //
        // Yes - redraw the position string.
        //
        USBMouseHostPositionGet(&sX, &sY);
        usnprintf(g_pcMousePos, MAX_MOUSE_POS_LEN, "(%d, %d)  ", sX, sY);
        if(g_ulCurrentScreen == IO_SCREEN)
        {
            WidgetPaint((tWidget *)&g_sMousePos);
        }
    }

    //
    // Did the state of any button change?
    //
    if(ulFlags & MOUSE_FLAG_BUTTONS)
    {
        unsigned long ulButtons;

        //
        // Yes - redraw the button state indicators.
        //
        ulButtons = USBMouseHostButtonsGet();

        //
        // Set the state of the button 1 indicator.
        //
        CanvasImageSet(&g_sMouseBtn1, ((ulButtons & MOUSE_BTN_1) ?
                        g_pucGreenLED14x14Image : g_pucRedLED14x14Image));
        if(g_ulCurrentScreen == IO_SCREEN)
        {
            WidgetPaint((tWidget *)&g_sMouseBtn1);
        }

        //
        // Set the state of the button 2 indicator.
        //
        CanvasImageSet(&g_sMouseBtn2, ((ulButtons & MOUSE_BTN_2) ?
                        g_pucGreenLED14x14Image : g_pucRedLED14x14Image));
        if(g_ulCurrentScreen == IO_SCREEN)
        {
            WidgetPaint((tWidget *)&g_sMouseBtn2);
        }

        //
        // Set the state of the button 3 indicator.
        //
        CanvasImageSet(&g_sMouseBtn3, ((ulButtons & MOUSE_BTN_3) ?
                        g_pucGreenLED14x14Image : g_pucRedLED14x14Image));
        if(g_ulCurrentScreen == IO_SCREEN)
        {
            WidgetPaint((tWidget *)&g_sMouseBtn3);
        }
    }
}

//*****************************************************************************
//
// Process any commands that the main function has been sent from other
// functions or interrupts.
//
//*****************************************************************************
void
ProcessMainFunctionCommands(void)
{
    if(g_ulCommandFlags & COMMAND_TOUCH_UPDATE)
    {
        ProcessCommandTouchUpdate();
        g_ulCommandFlags &= ~COMMAND_TOUCH_UPDATE;
    }
}

//*****************************************************************************
//
// The program main function.
//
//*****************************************************************************
int
main(void)
{
    int nStatus;
    unsigned long ulUser0, ulUser1, ulMouseFlags, ulIPAddr;
    unsigned char pucMACAddr[6];
    tBoolean bRetcode;
    tContext sContext;

    //
    // Set the system clock to run at 50MHz from the PLL
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout correctly for the IDM-SBC board.
    //
    PinoutSet();

    //
    // Enable one of the Ethernet LEDs on PF3. We leave the other one under
    // software control.
    //
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_3);

    //
    // Configure SysTick for a 100Hz interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

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
    // Turn on the display backlight at full brightness.
    //
    Kitronix320x240x16_SSD2119BacklightOn(255);

    //
    // Initialize the SDRAM.
    //
    SDRAMInit(1, (EPI_SDRAM_CORE_FREQ_50_100 |
              EPI_SDRAM_FULL_POWER | EPI_SDRAM_SIZE_64MBIT), 1024);

    //
    // Initialize the SSI flash driver.
    //
    SSIFlashInit();

    //
    // Configure PF2 a GPIO output for the user LED.
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,GPIO_PIN_2);

    //
    // Initialize LED to OFF (set the GPIO to 1)
    //
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);

    //
    // Get the MAC address from the UART0 and UART1 registers in NV ram.
    //
    ROM_FlashUserGet(&ulUser0, &ulUser1);

    //
    // Convert the 24/24 split MAC address from NV ram into a MAC address
    // array.
    //
    pucMACAddr[0] = ulUser0 & 0xff;
    pucMACAddr[1] = (ulUser0 >> 8) & 0xff;
    pucMACAddr[2] = (ulUser0 >> 16) & 0xff;
    pucMACAddr[3] = ulUser1 & 0xff;
    pucMACAddr[4] = (ulUser1 >> 8) & 0xff;
    pucMACAddr[5] = (ulUser1 >> 16) & 0xff;

    //
    // Format this address into the string used by the relevant widget.
    //
    usnprintf(g_pucMACAddrString, SIZE_MAC_ADDR_BUFFER,
              "%02X-%02X-%02X-%02X-%02X-%02X",
              pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
              pucMACAddr[4], pucMACAddr[5]);

    //
    // Remember that we don't have an IP address yet.
    //
    ulIPAddr = 0;
    usnprintf(g_pucIPAddrString, SIZE_IP_ADDR_BUFFER, "Not assigned");

    //
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACAddr);
    LocatorAppTitleSet("RDK-IDM-SBC idm-checkout");

    //
    // Start the remote software update module.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Initialize the FAT file system.
    //
    bRetcode = FileInit();
    if(!bRetcode)
    {
        UARTprintf("Error initializing FAT file system.\n");
        PrintfStatus("Error on FATfs init!\n");
    }
    else
    {
        PrintfStatus("File systems OK.\n");

        //
        // See if there is an SD card present.
        //
        bRetcode = FileIsDrivePresent(0);

        //
        // Tell the user what we found.
        //
        if(bRetcode)
        {
            //
            // The SD card is present so update the display accordingly.
            //
            CanvasTextColorSet(&g_sSDCard1, CLR_PRESENT);
            CanvasTextColorSet(&g_sSDCard2, CLR_PRESENT);
            CanvasTextSet(&g_sSDCard2, "Present");
            PrintfStatus("MicroSD card present.");

        }
        else
        {
            //
            // The SD card is not present.
            //
            CanvasTextColorSet(&g_sSDCard1, CLR_ABSENT);
            CanvasTextColorSet(&g_sSDCard2, CLR_ABSENT);
            CanvasTextSet(&g_sSDCard2, "Absent");
            PrintfStatus("MicroSD card absent.");
        }

        //
        // Was an SDRAM file system image found and mounted?
        //
        bRetcode = FileIsSDRAMImagePresent();
        PrintfStatus("SDRAM fs %s.", bRetcode ? "present" : "absent");
    }

    //
    // Initialize the HTTP server
    //
    httpd_init();

    //
    // Initialize the TFTP server (used to update the file system image in
    // the serial flash device).
    //
    TFTPInit();

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.  Rather than piping the touch
    // messages directly to the widget manager, we intercept them so that we
    // can display the information on the screen.
    //
    TouchScreenCallbackSet(CheckoutPointerMessage);

    //
    // Configure and enable uDMA
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    ROM_uDMAControlBaseSet(&sDMAControlTable[0]);
    ROM_uDMAEnable();

    //
    // Initialize the sound driver.
    //
    SoundInit();

    //
    // Initialize the graphics demo widgets.
    //
    GraphicsDemoInit();

    //
    // Initialize the image viewer.  This must be done after the SDRAM file
    // system image has been copied.
    //
    ImageViewerInit();

    //
    // Initialize the audio file player.
    //
    AudioPlayerInit();

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sHeading);

    //
    // Process all messages in the widget queue.  This flushes out all the
    // paint messages posted during initialization above and ensures that
    // we repaint the entire tree as a result of the next WidgetPaint() call.
    // Without this, the message queue may have overflowed and we lose the
    // next paint message.
    //
    WidgetMessageQueueProcess();
    WidgetPaint(WIDGET_ROOT);

    //
    // Initialize the USB mouse support.
    //
    USBMouseInit(g_sKitronix320x240x16_SSD2119.usWidth,
                 g_sKitronix320x240x16_SSD2119.usHeight);

    //
    // Print a hello message for the UART command line user.
    //
    UARTprintf("\n\nRDK-IDM-SBC Checkout Example Program\n");
    UARTprintf("Type \'help\' for help.\n");

    //
    // Enter an (almost) infinite loop for reading and processing commands from
    // the user.
    //
    while(1)
    {
        //
        // Print a prompt to the console.
        //
        UARTprintf("\n> ");

        //
        // Is there a command waiting to be processed or has a remote firmware
        // update been requested?
        //
        while(!g_bFirmwareUpdate && (UARTPeek('\r') < 0))
        {
            //
            // Do we have an IP address yet? If not, check to see if we've been
            // assigned one since the last time we checked.
            //
            if(ulIPAddr == 0)
            {
                //
                // What is our current IP address?
                //
                ulIPAddr = lwIPLocalIPAddrGet();

                //
                // If it's non zero, update the display.
                //
                if(ulIPAddr)
                {
                    usprintf(g_pucIPAddrString, "%d.%d.%d.%d",
                             ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                             (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);
                    if(g_ulCurrentScreen == IO_SCREEN)
                    {
                        WidgetPaint((tWidget *)&g_sIPAddr);
                    }
                }
            }

            //
            // See if we have been sent any commands from other functions or
            // interrupts.
            //
            if(g_ulCommandFlags)
            {
                ProcessMainFunctionCommands();
            }

            //
            // Perform any required regular I2S audio processing.
            //
            AudioProcess();

            //
            // Call the USB mouse module to see if anything changed.
            //
            ulMouseFlags = USBMouseProcess();

            if(ulMouseFlags)
            {
                //
                // Something changed so update the widgets related to the
                // mouse.
                //
                UpdateMouseWidgets(ulMouseFlags);
            }

            //
            // Process any messages in the widget message queue.
            //
            WidgetMessageQueueProcess();
        }

        //
        // Drop out of the loop if a remote firmware update is being requested.
        //
        if(g_bFirmwareUpdate)
        {
            break;
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
            UARTprintf("Command returned error code %d\n", nStatus);
        }
    }

    //
    // If we drop out of the loop, someone requested a software update so
    // tell the user what's going on.
    //
    GrContextInit(&sContext, &g_sKitronix320x240x16_SSD2119);
    GrContextForegroundSet(&sContext, ClrWhite);
    GrContextBackgroundSet(&sContext, ClrBlack);
    GrContextFontSet(&sContext, g_pFontCmss22b);
    GrStringDrawCentered(&sContext, "  Updating Firmware...  ", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         (GrContextDpyHeightGet(&sContext) / 2),
                         true);

    //
    // Transfer control to the bootloader.
    //
    SoftwareUpdateBegin();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }

}
