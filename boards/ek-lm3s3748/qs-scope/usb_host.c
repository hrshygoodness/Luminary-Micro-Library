//*****************************************************************************
//
// usb_host.c - Functions relating to USB MSC host operation within the
//              Quickstart oscilloscope application.
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

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/usb.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "usblib/usbmsc.h"
#include "usblib/device/usbdevice.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"
#include "utils/uartstdio.h"
#include "fatfs/src/ff.h"
#include "data-acq.h"
#include "file.h"
#include "qs-scope.h"
#include "renderer.h"
#include "usbdescriptors.h"
#include "usbhw.h"
#include "usb_device.h"

//*****************************************************************************
//
// The instance data for the MSC driver.
//
//*****************************************************************************
unsigned long g_ulMSCInstance = 0;

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the MSC class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_USBHostMSCClassDriver
};

//*****************************************************************************
//
// This global holds the number of class drivers in the g_ppHostClassDrivers
// list.
//
//*****************************************************************************
static unsigned long g_ulNumHostClassDrivers =
    sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

//*****************************************************************************
//
// Hold the current state for the host.
//
//*****************************************************************************
volatile enum
{
    STATE_NO_DEVICE,
    STATE_DEVICE_ENUM,
    STATE_DEVICE_READY,
    STATE_ERROR
}
g_eState;

//*****************************************************************************
//
// Storage for the descriptors of the attached device when we are in host
// mode.
//
//*****************************************************************************
#define USB_HOST_HEAP_SIZE      128
unsigned char g_pcUSBHostHeap[USB_HOST_HEAP_SIZE];

//*****************************************************************************
//
// Calls the USB host stack tick function.
//
// This function is called periodically to allow the USB host stack to
// process any outstanding items.
//
// \return None.
//
//*****************************************************************************
void
ScopeUSBHostTick(void)
{
    tBoolean bRetcode;

    //
    // Call the stack to allow it to perform any processing needing done.
    //
    USBHCDMain();

    //
    // See if we need to do anything here.
    //
    if(g_eState == STATE_DEVICE_ENUM)
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
        }
        else
        {
            //
            // The device is available.  Try to open its root directory just
            // to make sure it is accessible.
            //
            bRetcode = FileIsDrivePresent(1);

            if(bRetcode)
            {
                //
                // The USB drive is accessible so dump a message and set the state
                // to indicate that the device is ready for use.
                //
                UARTprintf("Opened root directory - USB drive present.\n");
                RendererSetAlert("USB drive\ndetected.", 200);
                g_eState = STATE_DEVICE_READY;
            }
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
            // Mount the USB flash stick as logical drive 1 in the file system.
            //
            FileMountUSB(true);

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
            // Remove the USB stick from the file system.
            //
            FileMountUSB(false);

            //
            // Close our MSC drive instance.
            //
            USBHMSCDriveClose(g_ulMSCInstance);
            g_ulMSCInstance = 0;

            //
            // Go back to the "no device" state and wait for a new connection.
            //
            g_eState = STATE_NO_DEVICE;
            RendererSetAlert("USB drive\nremoved.", 200);

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
// This function is called by the USB library to inform us of the mode that
// we are operating in, USB_MODE_DEVICE or USB_MODE_HOST.
//
//*****************************************************************************
void
ScopeUSBModeCallback(unsigned long ulIndex, tUSBMode eMode)
{
    //
    // What is the new operating mode?
    //
    switch(eMode)
    {
        //
        // We are switching to USB device mode.
        //
        case USB_MODE_DEVICE:
        {
            //
            // Clear the flag used by other modules in the application to
            // indicate which mode we are in.
            //
            g_bUSBModeIsHost = false;

            break;
        }

        //
        // We are switching to USB host mode.
        //
        case USB_MODE_HOST:
        {
            //
            // Set the flag used by other modules in the application to
            // indicate which mode we are in.
            //
            g_bUSBModeIsHost = true;

            break;
        }

        default:
        {
            //
            // This callback will only ever be called to tell us we are
            // operating as a host or a device.  Any other value indicates
            // something has gone horribly wrong.
            //
            break;
        }
    }
}

//*****************************************************************************
//
// Set up for operation as a USB host allowing access to Mass Storage Class
// devices.
//
//*****************************************************************************
tBoolean
ScopeUSBHostInit(void)
{
    //
    // Set the USB power control pins to be controlled by the USB controller.
    //
    SysCtlPeripheralEnable(USB_HOST_GPIO_PERIPH);
    SysCtlPeripheralEnable(USB_PWR_GPIO_PERIPH);
    GPIOPinTypeUSBDigital(USB_HOST_GPIO_BASE, USB_HOST_GPIO_PINS);
    GPIOPinTypeUSBDigital(USB_PWR_GPIO_BASE, USB_PWR_GPIO_PINS);

    //
    // Configure the USB mux on the board to put us in host mode.  We pull
    // the relevant pin low to do this.
    //
    SysCtlPeripheralEnable(USB_MUX_GPIO_PERIPH);
    GPIOPinTypeGPIOOutput(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN);
    GPIOPinWrite(USB_MUX_GPIO_BASE, USB_MUX_GPIO_PIN, USB_MUX_SEL_HOST);


    //
    // Tell the stack that we will be operating as a host rather than a
    // device.  Note that we expect a callback to ScopeUSBModeCallback
    // before this function call returns.  This will perform additional host
    // initialization.
    //
    USBStackModeSet(0, USB_MODE_HOST, ScopeUSBModeCallback);

    //
    // Register the host class drivers.
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers,
                          g_ulNumHostClassDrivers);

    //
    // Set our Mass Storage Class driver callback.
    //
    g_ulMSCInstance = USBHMSCDriveOpen(0, MSCCallback);

    //
    // Initialize the host controller and give it the memory we have
    // set aside for it to use for descriptor storage.
    //
    USBHCDInit(0, g_pcUSBHostHeap, USB_HOST_HEAP_SIZE);

    //
    // All appears well.
    //
    return(true);
}

//*****************************************************************************
//
// Clean up and release all USB hardware resources.
//
//*****************************************************************************
void
ScopeUSBHostTerm(void)
{
    //
    // If necessary, unmount the USB flash drive.
    //
    if((g_eState == STATE_DEVICE_ENUM) || (g_eState == STATE_DEVICE_READY))
    {
        FileMountUSB(false);
    }

    //
    // Nothing is connected any more.
    //
    g_eState = STATE_NO_DEVICE;

    //
    // Close our MSC drive instance.
    //
    USBHMSCDriveClose(g_ulMSCInstance);
    g_ulMSCInstance = 0;

    //
    // Tell the USB library that we are finished using the host controller.
    //
    USBHCDTerm(0);
}
