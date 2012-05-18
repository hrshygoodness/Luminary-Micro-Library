//*****************************************************************************
//
// qs-logger.c - Data logger Quickstart application for EK-LM4F232.
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
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/adc.h"
#include "driverlib/hibernate.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "usblib/usblib.h"
#include "drivers/cfal96x64x16.h"
#include "drivers/buttons.h"
#include "utils/ustdlib.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "drivers/slidemenuwidget.h"
#include "stripchartwidget.h"
#include "clocksetwidget.h"
#include "qs-logger.h"
#include "images.h"
#include "menus.h"
#include "acquire.h"
#include "inc/hw_gpio.h"
#include "inc/hw_hibernate.h"
#include "usbstick.h"
#include "usbserial.h"
#include "flashstore.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Data Logger (qs-logger)</h1>
//!
//! This example application is a data logger.  It can be configured to collect
//! data from up to 10 data sources.  The possible data sources are:
//! - 4 analog inputs, 0-20V
//! - 3-axis accelerometer
//! - internal and external temperature sensors
//! - processor current consumption
//!
//! The data logger provides a menu navigation that is operated by the buttons
//! on the EK-LM4F232 board (up, down, left, right, select).  The data logger
//! can be configured by using the menus.  The following items can be
//! configured:
//! - data sources to be logged
//! - sample rate
//! - storage location
//! - sleep modes
//! - clock
//!
//! Using the data logger:
//!
//! Use the CONFIG menu to configure the data logger.  The following choices
//! are provided:
//!
//! - CHANNELS - enable specific channels of data that will be logged
//! - PERIOD - select the sample period
//! - STORAGE - select where the collected data will be stored:
//!     - FLASH - stored in the internal flash memory
//!     - USB - stored on a connected USB memory stick
//!     - HOST PC - transmitted to a host PC via USB OTG virtual serial port
//!     - NONE - the data will only be displayed and not stored
//! - SLEEP - select whether or not the board sleeps between samples.  Sleep
//! mode is allowed when storing to flash at with a period of 1 second or
//! longer.
//! - CLOCK - allows setting of internal time-of-day clock that is used for
//! time stamping of the sampled data
//!
//! Use the START menu to start the data logger running.  It will begin
//! collecting and storing the data.  It will continue to collect data until
//! stopped by pressing the left button or select button.
//!
//! While the data logger is collecting data and it is not configured to
//! sleep, a simple strip chart showing the collected data will appear on the
//! display.  If the data logger is configured to sleep, then no strip chart
//! will be shown.
//!
//! If the data logger is storing to internal flash memory, it will overwrite
//! the oldest data.  If storing to a USB memory device it will store data
//! until the device is full.
//!
//! The VIEW menu allows viewing the values of the data sources in numerical
//! format.  When viewed this way the data is not stored.
//!
//! The SAVE menu allows saving data that was stored in internal flash memory
//! to a USB stick.  The data will be saved in a text file in CSV format.
//!
//! The ERASE menu is used to erase the internal memory so more data can be
//! saved.
//!
//! When the EK-LM4F232 board running qs-logger is connected to a host PC via
//! the USB OTG connection for the first time, Windows will prompt for a device
//! driver for the board.  This can be found in C:/StellarisWare/windows_drivers
//! assuming you installed the software in the default folder.
//!
//! A companion Windows application, logger, can be found in the
//! StellarisWare/tools/bin directory.  When the data logger's STORAGE option
//! is set to "HOST PC" and the board is connected to a PC via the USB OTG
//! connection, captured data will be transfered back to the PC using the
//! virtual serial port that the EK board offers.  When the logger application
//! is run, it will search for the first connected EK-LM4F232 board and display
//! any sample data received.  The application also offers the option to log
//! the data to a file on the PC.
//
//*****************************************************************************

//*****************************************************************************
//
// The clock rate for the SysTick interrupt and a counter of system clock
// ticks.  The SysTick interrupt is used for basic timing in the application.
//
//*****************************************************************************
#define CLOCK_RATE              100
#define MS_PER_SYSTICK (1000 / CLOCK_RATE)
static volatile unsigned long g_ulTickCount;

//*****************************************************************************
//
// A widget handle of the widget that should receive the focus of any button
// events.
//
//*****************************************************************************
static unsigned long g_ulKeyFocusWidgetHandle;

//*****************************************************************************
//
// Tracks the data logging state.
//
//*****************************************************************************
typedef enum
{
    STATE_IDLE,
    STATE_LOGGING,
    STATE_VIEWING,
    STATE_SAVING,
    STATE_ERASING,
    STATE_FREEFLASH,
    STATE_CLOCKSET,
    STATE_CLOCKEXIT,
} tLoggerState;
static tLoggerState g_sLoggerState = STATE_IDLE;

//*****************************************************************************
//
// The configuration of the application.  This holds the information that
// will need to be saved if sleeping is used.
//
//*****************************************************************************
static tConfigState g_sConfigState;

//*****************************************************************************
//
// The current state of USB OTG in the system based on the detected mode.
//
//*****************************************************************************
volatile tUSBMode g_eCurrentUSBMode = USB_MODE_NONE;

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
// Forward declaration of callback function used for clock setter widget.
//
//*****************************************************************************
static void ClockSetOkCallback(tWidget *pWidget, tBoolean bOk);

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
// Provide a simple function so other parts of the application can update
// a status display.
//
//*****************************************************************************
void
SetStatusText(const char *pcTitle, const char *pcLine1, const char *pcLine2,
              const char *pcLine3)
{
    static const char cBlankLine[] = "                ";

    //
    // Check to see if each parameter was passed, and if so then update its
    // text field on the status dislay.
    //
    pcTitle = pcTitle ? pcTitle : cBlankLine;
    MenuUpdateText(TEXT_ITEM_STATUS_TITLE, pcTitle);
    pcLine1 = pcLine1 ? pcLine1 : cBlankLine;
    MenuUpdateText(TEXT_ITEM_STATUS1, pcLine1);
    pcLine2 = pcLine2 ? pcLine2 : cBlankLine;
    MenuUpdateText(TEXT_ITEM_STATUS2, pcLine2);
    pcLine3 = pcLine3 ? pcLine3 : cBlankLine;
    MenuUpdateText(TEXT_ITEM_STATUS3, pcLine3);

    //
    // Force a repaint after all the status text fields have been updated.
    //
    WidgetPaint(WIDGET_ROOT);
    WidgetMessageQueueProcess();
}
//*****************************************************************************
//
// Handles the SysTick timeout interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Increment the tick count.
    //
    g_ulTickCount++;
}

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
    static unsigned long ulLastTick = 0;

    ulRetVal = g_ulTickCount;
    ulSaved = ulRetVal;

    if(ulSaved > ulLastTick)
    {
        ulRetVal = ulSaved - ulLastTick;
    }
    else
    {
        ulRetVal = ulLastTick - ulSaved;
    }

    //
    // This could miss a few milliseconds but the timings here are on a
    // much larger scale.
    //
    ulLastTick = ulSaved;

    //
    // Return the number of milliseconds since the last time this was called.
    //
    return(ulRetVal * MS_PER_SYSTICK);
}

//*****************************************************************************
//
// Callback function for USB OTG mode changes.
//
//*****************************************************************************
static void
ModeCallback(unsigned long ulIndex, tUSBMode eMode)
{
    //
    // Save the new mode.
    //
    g_eCurrentUSBMode = eMode;

    //
    // Mode-specific handling code could go here.
    //
    switch(eMode)
    {
        case USB_MODE_HOST:
        {
            break;
        }
        case USB_MODE_DEVICE:
        {
            break;
        }
        case USB_MODE_NONE:
        {
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
// Gets the logger configuration from battery backed memory.
// The configuration is read from the memory in the Hibernate module.
// It is checked for validity.  If found to be valid the function returns a
// 0.  If not valid, then it returns non-zero.
//
//*****************************************************************************
static int
GetSavedState(tConfigState *pState)
{
    unsigned long ulStateLen = sizeof(tConfigState) / 4;
    unsigned short usCrc16;

    //
    // Check the arguments
    //
    ASSERT(pState);
    if(!pState)
    {
        return(1);
    }

    //
    // Read a block from hibernation memory into the application state
    // structure.
    //
    HibernateDataGet((unsigned long *)pState, ulStateLen);

    //
    // Check first to see if the "cookie" value is correct.
    //
    if(pState->ulCookie != STATE_COOKIE)
    {
        return(1);
    }

    //
    // Find the 16-bit CRC of the block.  The CRC is stored in the last
    // location, so subtract 1 word from the count.
    //
    usCrc16 = ROM_Crc16Array(ulStateLen - 1, (const unsigned long *)pState);

    //
    // If the CRC does not match, then the block is not good.
    //
    if(pState->ulCrc16 != (unsigned long)usCrc16)
    {
        return(1);
    }

    //
    // At this point the state structure that was retrieved from the
    // battery backed memory has been validated, so return it as a valid
    // logger state configuration.
    //
    return(0);
}

//*****************************************************************************
//
// Stores the logger configuration to battery backed memory in the
// Hibernation module.  The configuration is saved with a cookie value and
// a CRC in order to ensure validity.
//
//*****************************************************************************
static void
SetSavedState(tConfigState *pState)
{
    unsigned long ulStateLen = sizeof(tConfigState) / 4;
    unsigned short usCrc16;

    //
    // Check the arguments.
    //
    ASSERT(pState);
    if(pState)
    {
        //
        // Write the cookie value to the block
        //
        pState->ulCookie = STATE_COOKIE;

        //
        // Find the 16-bit CRC of the block.  The CRC is stored in the last
        // location, so subtract 1 word from the count.
        //
        usCrc16 = ROM_Crc16Array(ulStateLen - 1,
                                 (const unsigned long *)pState);

        //
        // Save the computed CRC into the structure.
        //
        pState->ulCrc16 = (unsigned long)usCrc16;

        //
        // Now write the entire block to the Hibernate memory.
        //
        HibernateDataSet((unsigned long *)pState, ulStateLen);
    }
}

//*****************************************************************************
//
// Populate the application configuration with default values.
//
//*****************************************************************************
static void
GetDefaultState(tConfigState *pState)
{
    //
    // Check the arguments
    //
    ASSERT(pState);
    if(pState)
    {
        //
        // get the default values from the menu system
        //
        MenuGetDefaultState(pState);

        //
        // Set the filename to a null string
        //
        pState->cFilename[0] = 0;

        //
        // Set bogus address for flash storage
        //
        pState->ulFlashStore = 0;

        //
        // Turn off sleep logging
        //
        pState->ulSleepLogging = 0;
    }
}

//*****************************************************************************
//
// Sends a button press message to whichever widget has the  button focus.
//
//*****************************************************************************
static void
SendWidgetKeyMessage(unsigned long  ulMsg)
{
    WidgetMessageQueueAdd(WIDGET_ROOT, ulMsg, g_ulKeyFocusWidgetHandle,
                          0, 1, 1);
}

//*****************************************************************************
//
// Callback function from the menu widget.  This function is called whenever
// the menu is used to activate a child widget that is associated with the
// menu.  It is also called when the widget is deactivated and control is
// returned to the menu widget.  It can be used to trigger different actions
// depending on which menus are chosen, and to track the state of the
// application and control focus for the user interface.
//
// This function is called in the context of widget tree message processing
// so care should be taken if doing any operation that affects the display
// or widget tree.
//
//*****************************************************************************
static void
WidgetActivated(tWidget *pWidget, tSlideMenuItem *pMenuItem,
                tBoolean bActivated)
{
    char *pcMenuText;

    //
    // Handle the activation or deactivation of the strip chart.  The
    // strip chart widget is activated when the user selects the START menu.
    //
    if(pWidget == &g_sStripChart.sBase)
    {
        //
        // If the strip chart is activated, start the logger running.
        //
        if(bActivated)
        {
            //
            // Get the current state of the menus
            //
            MenuGetState(&g_sConfigState);

            //
            // Save the state in battery backed memory
            //
            SetSavedState(&g_sConfigState);

            //
            // Start logger and update the logger state
            //
            AcquireStart(&g_sConfigState);
            g_sLoggerState = STATE_LOGGING;
        }

        //
        // If the strip chart is deactivated, stop the logger.
        //
        else
        {
            AcquireStop();
            g_sLoggerState = STATE_IDLE;
        }
    }

    //
    // Handle the activation or deactivation of any of the container
    // canvas that is used for showing the acquired data as a numerical
    // display.  This happens when the VIEW menu is used.
    //
    else if((pWidget == &g_sAINContainerCanvas.sBase) ||
            (pWidget == &g_sAccelContainerCanvas.sBase) ||
            (pWidget == &g_sCurrentContainerCanvas.sBase) ||
            (pWidget == &g_sClockContainerCanvas.sBase) ||
            (pWidget == &g_sTempContainerCanvas.sBase))
    {
        //
        // A viewer has been activated.
        //
        if(bActivated)
        {
            static tConfigState sLocalState;

            //
            // Get the current menu configuration state and save it in a local
            // storage.
            //
            MenuGetState(&sLocalState);

            //
            // Modify the state to set values that are suitable for using
            // with the viewer.  The acquisition rate is set to 1/2 second
            // and all channels are selected.  The storage medium is set to
            // "viewer" so the acquistion module will write the value of
            // acquired data to the appropriate viewing canvas.
            //
            sLocalState.ucStorage = CONFIG_STORAGE_VIEWER;
            sLocalState.ulPeriod = 0x00000040;
            sLocalState.usSelectedMask = 0x3ff;

            //
            // Start the acquisition module running.
            //
            AcquireStart(&sLocalState);
            g_sLoggerState = STATE_VIEWING;
        }

        //
        // The viewer has been deactivated so turn off the acquisition module.
        //
        else
        {
            AcquireStop();
            g_sLoggerState = STATE_IDLE;
        }
    }

    //
    // Handle the case when a status display has been activated.  This can
    // occur when any of several menu items are selected.
    //
    else if(pWidget == &g_sStatusContainerCanvas.sBase)
    {
        //
        // Get pointer to the text of the current menu item.
        //
        if(pMenuItem)
        {
            pcMenuText = pMenuItem->pcText;
        }
        else
        {
            return;
        }

        //
        // If activated from the SAVE menu, then the flash data needs to be
        // saved to USB stick.  Enter the saving state.
        //
        if(!strcmp(pcMenuText, "SAVE"))
        {
            if(bActivated)
            {
                g_sLoggerState = STATE_SAVING;
            }
            else
            {
                g_sLoggerState = STATE_IDLE;
            }
        }

        //
        // If activated from the ERASE menu, then the flash data needs to be
        // erased.  Enter the erasing state.
        //
        else if(!strcmp(pcMenuText, "ERASE DATA?"))
        {
            if(bActivated)
            {
                g_sLoggerState = STATE_ERASING;
            }
            else
            {
                g_sLoggerState = STATE_IDLE;
            }
        }

        //
        // If activated from the FLASH SPACE menu, then the user will be shown
        // a report on the amount of free space in flash.  Enter the reporting
        // state.
        //
        else if(!strcmp(pcMenuText, "FLASH SPACE"))
        {
            if(bActivated)
            {
                g_sLoggerState = STATE_FREEFLASH;
            }
            else
            {
                g_sLoggerState = STATE_IDLE;
            }
        }
    }

    //
    // Handle the activation of the clock setting widget.  Deactivation is
    // handled through a separate callback.
    //
    else if(pWidget == &g_sClockSetter.sBase)
    {
        unsigned long ulRTC;

        //
        // If the clock setter is activated, load the time structure fields.
        //
        if(bActivated)
        {
            //
            // Get the current time in seconds from the RTC.
            //
            ulRTC = HibernateRTCGet();

            //
            // Convert the RTC time to a time structure.
            //
            ulocaltime(ulRTC, &g_sTimeClock);

            //
            // Set the callback that will be called when the clock setting
            // widget is deactivated.  Since the clock setting widget needs
            // to take over the focus for button events, it uses a separate
            // callback when it is finsihed.
            //
            ClockSetCallbackSet((tClockSetWidget *)pWidget,
                                ClockSetOkCallback);

            //
            // Give the clock setter widget focus for the button events
            //
            g_ulKeyFocusWidgetHandle = (unsigned long)pWidget;
            g_sLoggerState = STATE_CLOCKSET;
        }
    }
}

//*****************************************************************************
//
// This function is called when the user clicks OK or CANCEL in the clock
// setting widget.
//
//*****************************************************************************
static void
ClockSetOkCallback(tWidget *pWidget, tBoolean bOk)
{
    unsigned long ulRTC;

    //
    // Only update the RTC if the OK button was selected.
    //
    if(bOk)
    {
        //
        // Convert the time structure that was altered by the clock setting
        // widget into seconds.
        //
        ulRTC = umktime(&g_sTimeClock);

        //
        // If the conversion was valid, then write the updated clock to the
        // Hibernate RTC.
        //
        if(ulRTC != (unsigned long)(-1))
        {
            HibernateRTCSet(ulRTC);
        }
    }

    //
    // Set the state to clock exit so some cleanup can be done from the
    // main loop.
    //
    g_sLoggerState = STATE_CLOCKEXIT;
}

//*****************************************************************************
//
// Initialize and operate the data logger.
//
//*****************************************************************************
int
main(void)
{
    tContext sDisplayContext;
    tContext sBufferContext;
    unsigned long uX;
    unsigned long uY;
    unsigned long ulHibIntStatus;
    tBoolean bSkipSplash = false;
    unsigned long ulSysClock;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();

    //
    // Set the clocking to run at 50 MHz.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);
    ulSysClock = ROM_SysCtlClockGet();

    //
    // Initialize the data acquisition module.  This initializes the ADC
    // hardware.
    //
    AcquireInit();

    //
    // Enable access to  the hibernate peripheral.  If the hibernate peripheral
    // was already running then this will have no effect.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);

    //
    // Check to see if the hiberate module is already active and if so then
    // read the saved configuration state.  If both are okay, then proceed
    // to check and see if we are logging data using sleep mode.
    //
    if(HibernateIsActive() && !GetSavedState(&g_sConfigState))
    {
        //
        // Read the status of the hibernate module.
        //
        ulHibIntStatus = HibernateIntStatus(1);

        //
        // If this is a pin wake, that means the user pressed the select
        // button and we should terminate the sleep logging.  In this case
        // we will fall out of this conditional section, and go through the
        // normal startup below, but skipping the splash screen so the user
        // gets immediate response.
        //
        if(ulHibIntStatus & HIBERNATE_INT_PIN_WAKE)
        {
            //
            // Clear the interrupt flag so it is not seen again until another
            // wake.
            //
            HibernateIntClear(HIBERNATE_INT_PIN_WAKE);
            bSkipSplash = 1;
        }

        //
        // Otherwise if we are waking from hibernate and it was not a pin
        // wake, then it must be from RTC match.  Check to see if we are
        // sleep logging and if so then go through an abbreviated startup
        // in order to collect the data and go back to sleep.
        //
        else if(g_sConfigState.ulSleepLogging &&
                (ulHibIntStatus & HIBERNATE_INT_RTC_MATCH_0))
        {
            //
            // Start logger and pass the configuration.  The logger should
            // configure itself to take one sample.
            //
            AcquireStart(&g_sConfigState);
            g_sLoggerState = STATE_LOGGING;

            //
            // Enter a forever loop to run the acquisition.  This will run
            // until a new sample has been taken and stored.
            //
            while(!AcquireRun())
            {
            }

            //
            // Getting here means that a data acquisition was performed and we
            // can now go back to sleep.  Save the configuration and then
            // activate the hibernate.
            //
            SetSavedState(&g_sConfigState);

            //
            // Set wake condition on pin-wake or RTC match.  Then put the
            // processor in hibernation.
            //
            HibernateWakeSet(HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RTC);
            HibernateRequest();

            //
            // Hibernating takes a finite amount of time to occur, so wait
            // here forever until hibernate activates and the processor
            // power is removed.
            //
            for(;;)
            {
            }
        }

        //
        // Otherwise, this was not a pin wake, and we were not sleep logging,
        // so just fall out of this conditional and go through the normal
        // startup below.
        //
    }

    //
    // In this case, either the hibernate module was not already active, or
    // the saved configuration was not valid.  Initialize the configuration
    // to the default state and then go through the normal startup below.
    //
    else
    {
        GetDefaultState(&g_sConfigState);
    }

    //
    // Enable the Hibernate module to run.
    //
    HibernateEnableExpClk(SysCtlClockGet());

    //
    // The hibernate peripheral trim register must be set per silicon
    // erratum 2.1
    //
    HibernateRTCTrimSet(0x7FFF);

    //
    // Start the RTC running.  If it was already running then this will have
    // no effect.
    //
    HibernateRTCEnable();

    //
    // In case we were sleep logging and are now finished (due to user
    // pressing select button), then disable sleep logging so it doesnt
    // try to start up again.
    //
    g_sConfigState.ulSleepLogging = 0;
    SetSavedState(&g_sConfigState);

    //
    // Initialize the display driver.
    //
    CFAL96x64x16Init();

    //
    // Initialize the buttons driver.
    //
    ButtonsInit();

    //
    // Pass the restored state to the menu system.
    //
    MenuSetState(&g_sConfigState);

    //
    // Enable the USB peripheral
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);

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
    // Erratum workaround for silicon revision A1.  VBUS must have pull-down.
    //
    if(CLASS_IS_BLIZZARD && REVISION_IS_A1)
    {
        HWREG(GPIO_PORTB_BASE + GPIO_O_PDR) |= GPIO_PIN_1;
    }

    //
    // Initialize the USB stack mode and pass in a mode callback.
    //
    USBStackModeSet(0, USB_MODE_OTG, ModeCallback);

    //
    // Initialize the stack to be used with USB stick.
    //
    USBStickInit();

    //
    // Initialize the stack to be used as a serial device.
    //
    USBSerialInit();

    //
    // Initialize the USB controller for dual mode operation with a 2ms polling
    // rate.
    //
    USBOTGModeInit(0, 2000, g_pHCDPool, HCD_MEMORY_SIZE);

    //
    // Initialize the menus module.  This module will control the user
    // interface menuing system.
    //
    MenuInit(WidgetActivated);

    //
    // Configure SysTick to periodically interrupt.
    //
    g_ulTickCount = 0;
    ROM_SysTickPeriodSet(ulSysClock / CLOCK_RATE);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Initialize the display context and another context that is used
    // as an offscreen drawing buffer for display animation effect
    //
    GrContextInit(&sDisplayContext, &g_sCFAL96x64x16);
    GrContextInit(&sBufferContext, &g_sOffscreenDisplayA);

    //
    // Show the splash screen if we are not skipping it.  The only reason to
    // skip it is if the application was in sleep-logging mode and the user
    // just waked it up with the select button.
    //
    if(!bSkipSplash)
    {
        const unsigned char *pucSplashLogo = g_pucImage_TI_Black;

        //
        // Draw the TI logo on the display.  Use an animation effect where the
        // logo will "slide" onto the screen.  Allow select button to break
        // out of animation.
        //
        for(uX = 0; uX < 96; uX++)
        {
            if(ButtonsPoll(0, 0) & SELECT_BUTTON)
            {
                break;
            }
            GrImageDraw(&sDisplayContext, pucSplashLogo, 95 - uX, 0);
        }

        //
        // Leave the logo on the screen for a short duration.  Monitor the
        // buttons so that if the user presses the select button, the logo
        // display is terminated and the application starts immediately.
        //
        while(g_ulTickCount < 400)
        {
            if(ButtonsPoll(0, 0) & SELECT_BUTTON)
            {
                break;
            }
        }

        //
        // Extended splash sequence
        //
        if(ButtonsPoll(0, 0) & UP_BUTTON)
        {
            for(uX = 0; uX < 96; uX += 4)
            {
                GrImageDraw(&sDisplayContext, g_ppucImage_Splash[(uX / 4) & 3],
                            (long)uX - 96L, 0);
                GrImageDraw(&sDisplayContext, pucSplashLogo, uX, 0);
                ROM_SysCtlDelay(ulSysClock / 12);
            }
            ROM_SysCtlDelay(ulSysClock / 3);
            pucSplashLogo = g_ppucImage_Splash[4];
            GrImageDraw(&sDisplayContext, pucSplashLogo, 0, 0);
            ROM_SysCtlDelay(ulSysClock / 12);
        }

        //
        // Draw the initial menu into the offscreen buffer.
        //
        SlideMenuDraw(&g_sMenuWidget, &sBufferContext, 0);

        //
        // Now, draw both the TI logo splash screen (from above) and the initial
        // menu on the screen at the same time, moving the coordinates so that
        // the logo "slides" off the display and the menu "slides" onto the
        // display.
        //
        for(uY = 0; uY < 64; uY++)
        {
            GrImageDraw(&sDisplayContext, pucSplashLogo, 0, -uY);
            GrImageDraw(&sDisplayContext, g_pucOffscreenBufA, 0, 63 - uY);
        }
    }

    //
    // Add the menu widget to the widget tree and send an initial paint
    // request.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sMenuWidget);
    WidgetPaint(WIDGET_ROOT);

    //
    // Set the focus handle to the menu widget.  Any button events will be
    // sent to this widget
    //
    g_ulKeyFocusWidgetHandle = (unsigned long)&g_sMenuWidget;

    //
    // Forever loop to run the application
    //
    while(1)
    {
        static unsigned long ulLastTickCount = 0;

        //
        // Each time the timer tick occurs, process any button events.
        //
        if(g_ulTickCount != ulLastTickCount)
        {
            unsigned char ucButtonState;
            unsigned char ucButtonChanged;

            //
            // Remember last tick count
            //
            ulLastTickCount = g_ulTickCount;

            //
            // Read the debounced state of the buttons.
            //
            ucButtonState = ButtonsPoll(&ucButtonChanged, 0);

            //
            // Pass any button presses through to the widget message
            // processing mechanism.  The widget that has the button event
            // focus (probably the menu widget) will catch these button events.
            //
            if(BUTTON_PRESSED(SELECT_BUTTON, ucButtonState, ucButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_SELECT);
            }
            if(BUTTON_PRESSED(UP_BUTTON, ucButtonState, ucButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_UP);
            }
            if(BUTTON_PRESSED(DOWN_BUTTON, ucButtonState, ucButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_DOWN);
            }
            if(BUTTON_PRESSED(LEFT_BUTTON, ucButtonState, ucButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_LEFT);
            }
            if(BUTTON_PRESSED(RIGHT_BUTTON, ucButtonState, ucButtonChanged))
            {
                SendWidgetKeyMessage(WIDGET_MSG_KEY_RIGHT);
            }
        }

        //
        // Tell the OTG library code how much time has passed in milliseconds
        // since the last call.
        //
        USBOTGMain(GetTickms());

        //
        // Call functions as needed to keep the host or device mode running.
        //
        if(g_eCurrentUSBMode == USB_MODE_DEVICE)
        {
            USBSerialRun();
        }
        else if(g_eCurrentUSBMode == USB_MODE_HOST)
        {
            USBStickRun();
        }

        //
        // If in the logging state, then call the logger run function.  This
        // keeps the data acquisition running.
        //
        if((g_sLoggerState == STATE_LOGGING) ||
           (g_sLoggerState == STATE_VIEWING))
        {
            if(AcquireRun() && g_sConfigState.ulSleepLogging)
            {
                //
                // If sleep logging is enabled, then at this point we have
                // stored the first data item, now save the state and start
                // hibernation.  Wait for the power to be cut.
                //
                SetSavedState(&g_sConfigState);
                HibernateWakeSet(HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RTC);
                HibernateRequest();
                for(;;)
                {
                }
            }

            //
            // If viewing instead of logging then request a repaint to keep
            // the viewing window updated.
            //
            if(g_sLoggerState == STATE_VIEWING)
            {
                WidgetPaint(WIDGET_ROOT);
            }
        }

        //
        // If in the saving state, then save data from flash storage to
        // USB stick.
        //
        if(g_sLoggerState == STATE_SAVING)
        {
            //
            // Save data from flash to USB
            //
            FlashStoreSave();

            //
            // Return to idle state
            //
            g_sLoggerState = STATE_IDLE;
        }

        //
        // If in the erasing state, then erase the data stored in flash.
        //
        if(g_sLoggerState == STATE_ERASING)
        {
            //
            // Save data from flash to USB
            //
            FlashStoreErase();

            //
            // Return to idle state
            //
            g_sLoggerState = STATE_IDLE;
        }

        //
        // If in the flash reporting state, then show the report of the amount
        // of used and free flash memory.
        //
        if(g_sLoggerState == STATE_FREEFLASH)
        {
            //
            // Report free flash space
            //
            FlashStoreReport();

            //
            // Return to idle state
            //
            g_sLoggerState = STATE_IDLE;
        }

        //
        // If we are exiting the clock setting widget, that means that control
        // needs to be given back to the menu system.
        //
        if(g_sLoggerState == STATE_CLOCKEXIT)
        {
            //
            // Give the button event focus back to the menu system
            //
            g_ulKeyFocusWidgetHandle = (unsigned long)&g_sMenuWidget;

            //
            // Send a button event to the menu widget that means the left
            // key was pressed.  This signals the menu widget to deactivate
            // the current child widget (which was the clock setting wigdet).
            // This will cause the menu widget to slide the clock set widget
            // off the screen and resume control of the display.
            //
            SendWidgetKeyMessage(WIDGET_MSG_KEY_LEFT);
            g_sLoggerState = STATE_IDLE;
        }

        //
        // Process any new messages that are in the widget queue.  This keeps
        // the user interface running.
        //
        WidgetMessageQueueProcess();
    }
}
