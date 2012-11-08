//*****************************************************************************
//
// camera.c - Low level driver for the camera functions on the FPGA/camera
//            daughter board.
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

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/i2c.h"
#include "driverlib/epi.h"
#include "driverlib/rom.h"
#include "driverlib/cpu.h"
#include "grlib/grlib.h"
#include "set_pinout.h"
#include "camerafpga.h"
#include "camera.h"
#include "kitronix320x240x16_fpga.h"

//*****************************************************************************
//
//! \addtogroup camera_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// I2C-related parameters for the camera control interface.
//
//*****************************************************************************
#define CAMERA_I2C_ADDR         (0x42 >> 1)
#define CAMERA_I2C_MASTER_BASE  I2C0_MASTER_BASE
#define CAMERA_I2C_PERIPH       SYSCTL_PERIPH_I2C0
#define CAMERA_I2CSCL_GPIO_PORT GPIO_PORTB_BASE
#define CAMERA_I2CSDA_PIN       GPIO_PIN_3
#define CAMERA_I2CSCL_PIN       GPIO_PIN_2

#define CAMERA_INT              INT_GPIOJ
#define CAMERA_INT_BASE         GPIO_PORTJ_BASE
#define CAMERA_INT_PIN          GPIO_PIN_6

//*****************************************************************************
//
// A simple structure used to hold register indices and values used to
// initialize the camera module.
//
//*****************************************************************************
typedef struct
{
    unsigned char ucReg;
    unsigned char ucVal;
}
tRegValue;

//*****************************************************************************
//
// Camera module register initialization values.  These values were supplied by
// Omnivision.
//
//*****************************************************************************
tRegValue g_psCameraRegInit[] =
{
    {0x12, 0x80},
    {0x0e, 0x08},

    {0x0c, 0x16},

    {0x48, 0x42},
    {0x41, 0x43},
    {0x4c, 0x7b},
    {0x81, 0xff},
    {0x21, 0x44},
    {0x16, 0x03},
    {0x39, 0x80},
    {0x1e, 0xb1},

    // Format
    {0x12, 0x06},
    {0x82, 0x03},
    {0xd0, 0x48},
    {0x80, 0x7f},
    {0x3e, 0x30},
    {0x22, 0x00},

    // Resolution
    {0x17, 0x69},
    {0x18, 0xa4},
    {0x19, 0x03},
    {0x1a, 0xf6},

    {0xc8, 0x02},
    {0xc9, 0x80},
    {0xca, 0x01},
    {0xcb, 0xe0},

    {0xcc, 0x02},
    {0xcd, 0x80},
    {0xce, 0x01},
    {0xcf, 0xe0},

    // Lens Correction
    {0x85, 0x90},
    {0x86, 0x00},
    {0x87, 0x00},
    {0x88, 0x10},
    {0x89, 0x30},
    {0x8a, 0x29},
    {0x8b, 0x26},

    // Color Matrix
    {0xbb, 0x80},
    {0xbc, 0x62},
    {0xbd, 0x1e},
    {0xbe, 0x26},
    {0xbf, 0x7b},
    {0xc0, 0xac},
    {0xc1, 0x1e},

    // Edge + Denoise
    {0xb7, 0x05},
    {0xb8, 0x09},
    {0xb9, 0x00},
    {0xba, 0x18},

    // UVAdjust
    {0x5a, 0x4a},
    {0x5b, 0x9f},
    {0x5c, 0x48},
    {0x5d, 0x32},

    // AEC/AGC target
    {0x24, 0x7d},
    {0x25, 0x6b},
    {0x26, 0xc3},

    // Gamma
    {0xa3, 0x0b},
    {0xa4, 0x15},
    {0xa5, 0x2a},
    {0xa6, 0x51},
    {0xa7, 0x63},
    {0xa8, 0x74},
    {0xa9, 0x83},
    {0xaa, 0x91},
    {0xab, 0x9e},
    {0xac, 0xaa},
    {0xad, 0xbe},
    {0xae, 0xce},
    {0xaf, 0xe5},
    {0xb0, 0xf3},
    {0xb1, 0xfb},
    {0xb2, 0x06},

    // AWB
    {0x8e, 0x92},
    {0x96, 0xff},
    {0x97, 0x00},

    // Advance
    {0x8c, 0x5d},
    {0x8d, 0x11},
    {0x8e, 0x12},
    {0x8f, 0x11},
    {0x90, 0x50},
    {0x91, 0x22},
    {0x92, 0xd1},
    {0x93, 0xa7},
    {0x94, 0x23},
    {0x95, 0x3b},
    {0x96, 0xff},
    {0x97, 0x00},
    {0x98, 0x4a},
    {0x99, 0x46},
    {0x9a, 0x3d},
    {0x9b, 0x3a},
    {0x9c, 0xf0},
    {0x9d, 0xf0},
    {0x9e, 0xf0},
    {0x9f, 0xff},
    {0xa0, 0x56},
    {0xa1, 0x55},
    {0xa2, 0x13},

    // General Control
    {0x50, 0x9a},
    {0x51, 0x80},
    {0x21, 0x23},

    {0x14, 0x28},
    {0x13, 0xf7},
    {0x11, 0x01},

    {0x0e, 0x00}
};

#define NUM_INIT_REGISTERS (sizeof(g_psCameraRegInit) / sizeof(tRegValue))

//*****************************************************************************
//
// Camera module register settings for VGA/RGB565 capture.
//
//*****************************************************************************
tRegValue g_psCameraSizeVGA[] =
{
    {0x16, 0x03},
    {0x17, 0x69},
    {0x18, 0xa4},
    {0x19, 0x0c},
    {0x1a, 0xf6},
    {0x22, 0x00},
    {0xc8, 0x02},
    {0xc9, 0x80},
    {0xca, 0x01},
    {0xcb, 0xe0},
    {0xcc, 0x02},
    {0xcd, 0x80},
    {0xce, 0x01},
    {0xcf, 0xe0},
    {0x12, 0x06}
};

#define NUM_VGA_REGISTERS (sizeof(g_psCameraSizeVGA) / sizeof(tRegValue))

//*****************************************************************************
//
// Camera module register settings for QVGA/RGB565 capture.
//
//*****************************************************************************
tRegValue g_psCameraSizeQVGA[] =
{
    {0x16, 0x03},
    {0x17, 0x69},
    {0x18, 0xa4},
    {0x19, 0x03},
    {0x1a, 0xf6},
    {0x22, 0x10},
    {0xc8, 0x02},
    {0xc9, 0x80},
    {0xca, 0x00},
    {0xcb, 0xf0},
    {0xcc, 0x01},
    {0xcd, 0x40},
    {0xce, 0x00},
    {0xcf, 0xf0},
    {0x12, 0x46}
};

#define NUM_QVGA_REGISTERS (sizeof(g_psCameraSizeQVGA) / sizeof(tRegValue))

//*****************************************************************************
//
// A structure used to store register settings for different camera brightness
// settings.
//
//*****************************************************************************
typedef struct
{
    unsigned char ucThreshold;
    unsigned char ucReg24Val;
    unsigned char ucReg25Val;
    unsigned char ucReg26Val;
}
tBrightness;

//*****************************************************************************
//
// Camera brightness settings for levels -4EV through +4EV in 1EV steps.
//
//*****************************************************************************
tBrightness g_psBrightness[] =
{
    {0x10, 0x30, 0x28, 0x61},
    {0x30, 0x40, 0x38, 0x71},
    {0x50, 0x50, 0x48, 0x92},
    {0x70, 0x60, 0x58, 0x92},
    {0x90, 0x78, 0x68, 0xB4},
    {0xB0, 0x88, 0x80, 0xC5},
    {0xD0, 0x98, 0x90, 0xD6},
    {0xF0, 0xA8, 0xA0, 0xE5},
    {0xFF, 0xB8, 0xB0, 0xF8},
};

#define NUM_BRIGHTNESS_SETTINGS (sizeof(g_psBrightness) / sizeof(tBrightness))

//*****************************************************************************
//
// A structure used to store register settings for different camera brightness
// settings.
//
//*****************************************************************************
typedef struct
{
    unsigned char ucThreshold;
    unsigned char ucRegD4Val;
    unsigned char ucRegD3Val;
}
tContrast;

//*****************************************************************************
//
// Camera register settings for 9 contrast steps.
//
//*****************************************************************************
tContrast g_psContrast[] =
{
    {0x10, 0x10, 0xD0},
    {0x30, 0x14, 0x80},
    {0x50, 0x18, 0x48},
    {0x70, 0x1C, 0x20},
    {0x90, 0x20, 0x00},
    {0xB0, 0x24, 0x00},
    {0xD0, 0x28, 0x00},
    {0xF0, 0x2C, 0x00},
    {0xFF, 0x30, 0x00}
};

#define NUM_CONTRAST_SETTINGS (sizeof(g_psContrast) / sizeof(tContrast))

//*****************************************************************************
//
// A pointer to the function that is called when asynchronous events relating
// to video capture or display occur.
//
//*****************************************************************************
tCameraCallback g_pfnCameraCallback;

//*****************************************************************************
//
// The set of events that the client has requested notification for.
//
//*****************************************************************************
unsigned long g_ulCameraEvents;

//*****************************************************************************
//
// Flags used to indicate that a given interrupt has occurred.
//
//*****************************************************************************
volatile unsigned long g_ulCameraSignals;

//*****************************************************************************
//
// The handler for interrupts from the FPGA (via EPI30.  These inform us of various
// asynchronous events related to video capture and display.
//
//*****************************************************************************
void
CameraFPGAIntHandler(void)
{
    long lStatus;
    unsigned long ulInts;

    //
    // Get the interrupt status and clear the GPIO interrupt.
    //
    lStatus = ROM_GPIOPinIntStatus(CAMERA_INT_BASE, true);
    ROM_GPIOPinIntClear(CAMERA_INT_BASE, (unsigned char)(lStatus & 0xFF));

    //
    // Get the interrupt status from the FPGA and clear all pending interrupts.
    //
    ulInts = (unsigned long)HWREGH(FPGA_IRQSTAT_REG);
    HWREGH(FPGA_IRQSTAT_REG) = ulInts;

    //
    // Clear the bits in the signal variable corresponding to the interrupts
    // we are processing.
    //
    g_ulCameraSignals &= ~ulInts;

    //
    // If this interrupt relates to an event that the client has asked to see
    // and if a callback function has been provided, call it.
    //
    if((g_ulCameraEvents & ulInts) && g_pfnCameraCallback)
    {
        //
        // Pass the event notification to the client's callback function.
        //
        g_pfnCameraCallback(g_ulCameraEvents & ulInts);
    }
}

//*****************************************************************************
//
// Waits for the relevant signal from the FPGA to indicate that all previous
// register value changes have been latched and the new values are now in use.
// Register writes take effect at the end of each frame.  The bCapture
// parameter allows the caller to specify whether to wait until the end of the
// current video frame capture or the end of the frame being displayed.
//
//*****************************************************************************
static void
WaitForFrameEnd(tBoolean bCapture)
{
    unsigned long ulIntToCheck;

    //
    // Determine which interrupt to use depending upon whether we are to wait
    // for the end of the capture or display.
    //
    ulIntToCheck = bCapture ? FPGA_ISR_VCFEI : FPGA_ISR_LTEI;

    //
    // Temporarily disable the FPGA interrupt while we are setting the signal
    // flag.
    //
    ROM_IntDisable(CAMERA_INT);

    //
    // Set the global flag indicating that we are waiting for this
    // interrupt.
    //
    g_ulCameraSignals |= ulIntToCheck;

    //
    // Ensure that the interrupt we need to look at is turned on.
    //
    HWREGH(FPGA_IRQEN_REG) = g_ulCameraEvents | ulIntToCheck;

    //
    // Enable the FPGA interrupt once again now that everything is set up.
    //
    ROM_IntEnable(CAMERA_INT);

    //
    // Now wait for the interrupt to fire.  Note that this will hang if the
    // client is silly enough to ask us to wait when capture or display is
    // not actually running.  Hopefully this comment will suffice to help
    // people realize that they have done this.
    //
    while(g_ulCameraSignals & ulIntToCheck)
    {
        //
        // Wait for another interrupt.
        //
        CPUwfi();
    }

    //
    // Reinstate the original interrupt state.
    //
    HWREGH(FPGA_IRQEN_REG) = g_ulCameraEvents;
}

//*****************************************************************************
//
// Write a value to a particular camera control register.
//
//*****************************************************************************
tBoolean
CameraRegWrite(unsigned char ucReg, unsigned char ucVal)
{
    //
    // Set the slave address.
    //
    ROM_I2CMasterSlaveAddrSet(CAMERA_I2C_MASTER_BASE, CAMERA_I2C_ADDR, false);

    //
    // Write the next byte to the controller.
    //
    ROM_I2CMasterDataPut(CAMERA_I2C_MASTER_BASE, ucReg);

    //
    // Continue the transfer.
    //
    ROM_I2CMasterControl(CAMERA_I2C_MASTER_BASE,
                         I2C_MASTER_CMD_BURST_SEND_START);

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(CAMERA_I2C_MASTER_BASE, false) == 0)
    {
    }

    //
    // Clear the interrupt status.  Note that we DO NOT check for ACK here
    // since the SCCB protocol declares the bit we would normally associate
    // with ACK to be a "don't care."
    //
    ROM_I2CMasterIntClear(CAMERA_I2C_MASTER_BASE);

    //
    // Write the next byte to the controller.
    //
    ROM_I2CMasterDataPut(CAMERA_I2C_MASTER_BASE, ucVal);

    //
    // End the transfer.
    //
    ROM_I2CMasterControl(CAMERA_I2C_MASTER_BASE,
                         I2C_MASTER_CMD_BURST_SEND_FINISH);

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(CAMERA_I2C_MASTER_BASE, false) == 0)
    {
    }

    //
    // Clear the interrupt status.  Once again, don't check for ACK.
    //
    ROM_I2CMasterIntClear(CAMERA_I2C_MASTER_BASE);

    SysCtlDelay(ROM_SysCtlClockGet() / 1000);

    return(true);
}

//*****************************************************************************
//
// Write a list of camera registers with particular values.
//
//*****************************************************************************
static tBoolean
CameraRegSequenceWrite(tRegValue *psRegs, unsigned long ulCount)
{
    unsigned long ulLoop;
    tBoolean bRetcode;

    //
    // Initialize our return code.
    //
    bRetcode = true;

    //
    // Loop through each of the register/value entries supplied.
    //
    for(ulLoop = 0; ulLoop < ulCount; ulLoop++)
    {
        //
        // Write the register with the given value.
        //
        bRetcode = CameraRegWrite(psRegs[ulLoop].ucReg,
                                  psRegs[ulLoop].ucVal);

        //
        // Was the write successful?
        //
        if(!bRetcode)
        {
            //
            // No - drop out and return the error.
            //
            break;
        }
    }

    //
    // Tell the caller how we got on.
    //
    return(bRetcode);
}

//*****************************************************************************
//
// Read the value from a particular camera control register.
//
//*****************************************************************************
unsigned char
CameraRegRead(unsigned char ucReg)
{
    unsigned char ucVal;

    //
    // Clear any previously signalled interrupts.
    //
    ROM_I2CMasterIntClear(CAMERA_I2C_MASTER_BASE);

    //
    // Start with a dummy write to get the address set in the EEPROM.
    //
    ROM_I2CMasterSlaveAddrSet(CAMERA_I2C_MASTER_BASE, CAMERA_I2C_ADDR, false);

    //
    // Place the register to be read in the data register.
    //
    ROM_I2CMasterDataPut(CAMERA_I2C_MASTER_BASE, ucReg);

    //
    // Perform a single send, writing the register address as the only byte.
    //
    ROM_I2CMasterControl(CAMERA_I2C_MASTER_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    //
    // Wait until the current byte has been transferred.
    //
    while(ROM_I2CMasterIntStatus(CAMERA_I2C_MASTER_BASE, false) == 0)
    {
    }

    //
    // Clear the interrupt status.  Note that we DO NOT check for ACK here
    // since the SCCB protocol declares the bit we would normally associate
    // with ACK to be a "don't care."
    //
    ROM_I2CMasterIntClear(CAMERA_I2C_MASTER_BASE);

    //
    // Put the I2C master into receive mode.
    //
    ROM_I2CMasterSlaveAddrSet(CAMERA_I2C_MASTER_BASE, CAMERA_I2C_ADDR, true);

    //
    // Start the receive.
    //
    ROM_I2CMasterControl(CAMERA_I2C_MASTER_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);

    //
    // Wait until the current byte has been read.
    //
    while(ROM_I2CMasterIntStatus(CAMERA_I2C_MASTER_BASE, false) == 0)
    {
    }

    //
    // Read the received character.
    //
    ucVal =  ROM_I2CMasterDataGet(CAMERA_I2C_MASTER_BASE);

    //
    // Clear pending interrupt notifications.
    //
    ROM_I2CMasterIntClear(CAMERA_I2C_MASTER_BASE);

    //
    // Tell the caller we read the required data.
    //
    return(ucVal);
}

//*****************************************************************************
//
//! Initializes the camera and prepares for motion video capture.
//!
//! \param ulFlags defines the initial video size and pixel format.
//! \param ulCaptureAddr is the address of the video capture buffer in
//! daughter board SRAM where 0 indicates the bottom of the memory.
//! \param pfnCallback is a pointer to a function that will be called to
//! inform the application of any requested asynchronous events related to
//! video capture and display.  If no callbacks are required, this parameter
//! may be set to 0 to disable all notifications.
//!
//! This function must be called after PinoutSet() and before any other camera
//! API to initialize the camera and set the initial capture buffer location.
//! On exit, the camera is ready to start capturing motion video but capture has
//! not been enabled.
//!
//! Parameter \e ulFlags is comprised of ORed flags indicating the desired
//! initial video capture size and pixel format.  It must contain two flags,
//! one each from the following groups:
//!
//! Video dimensions: \b CAMERA_SIZE_VGA for 640x480 capture or \b
//! CAMERA_SIZE_QVGA for 320x240 capture.
//!
//! Pixel format: \b CAMERA_FORMAT_RGB565 to capture 16bpp RGB data with the
//! red component in the most significant 5 bits of the half word pixel, \b
//! CAMERA_FORMAT_BGR565 to capture RGB data with the blue component in the
//! most significant 5 bits of the pixel, \b CAMERA_FORMAT_YUYV to capture
//! video data in YUV422 format with YUYV component ordering or \b
//! CAMERA_FORMAT_YVYU to capture YUV422 data with YVYU component ordering.
//! Note that direct display of the video data is only possible when using
//! \b CAMERA_FORMAT_RGB565.
//!
//! The \e ulCaptureAddr parameter defines the start of the video capture buffer
//! relative to the start of the daughter board SRAM.  It is assumed that the
//! buffer is sized to hold a single frame of video at the size requested in
//! \e ulFlags.  The caller is responsible for managing the SRAM on the daughter
//! board and ensuring that this buffer is large enough to hold the captured
//! video and that it does not overlap with the graphics display buffer, if
//! used.
//!
//! \return Returns \b true if the camera was initialized successfully or
//! \b false if the required hardware was not present or some other error
//! occured during initialization.
//
//*****************************************************************************
tBoolean CameraInit(unsigned long ulFlags, unsigned long ulCaptureAddr,
                    tCameraCallback pfnCallback)
{
    tBoolean bRetcode;

    //
    // Make sure that the FPGA/camera daughter board is present.  This will
    // have been detected during a previous call to PinoutSet().
    //
    if(g_eDaughterType != DAUGHTER_FPGA)
    {
        return(false);
    }

    //
    // Enable the I2C controller used to interface to the camera controller.
    //
    ROM_SysCtlPeripheralEnable(CAMERA_I2C_PERIPH);

    //
    // Configure the I2C SCL and SDA pins for I2C operation.
    //
    ROM_GPIOPinTypeI2C(CAMERA_I2CSCL_GPIO_PORT,
                       CAMERA_I2CSCL_PIN | CAMERA_I2CSDA_PIN);

    //
    // Initialize the I2C master.
    //
    ROM_I2CMasterInitExpClk(CAMERA_I2C_MASTER_BASE, SysCtlClockGet(), 0);

    //
    // Initialize the camera.
    //
    bRetcode = CameraRegSequenceWrite(g_psCameraRegInit, NUM_INIT_REGISTERS);
    if(!bRetcode)
    {
        //
        // There was an error during camera initialization!
        //
        return(bRetcode);
    }

    //
    // Remember the callback function.
    //
    g_pfnCameraCallback = pfnCallback;

    //
    // Set the initial capture format and size.
    //
    CameraCaptureTypeSet(ulFlags);

    //
    // Set the initial buffer address and assume that the buffer dimensions
    // match the video size requested.  If this is not the case, the application
    // can call CameraCaptureBufferSet() to set different dimensions later.
    //
    CameraCaptureBufferSet(ulCaptureAddr,
                    ((ulFlags & CAMERA_SIZE_VGA) ? (640 * 2) : (320 * 2)),
                    true);

    //
    // Configure the interrupt pin from the FPGA but disable all interrupt
    // sources for now.
    //
    HWREGH(FPGA_IRQEN_REG) = 0;
    ROM_GPIODirModeSet(CAMERA_INT_BASE, CAMERA_INT_PIN, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(CAMERA_INT_BASE, CAMERA_INT_PIN, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);
    ROM_GPIOIntTypeSet(CAMERA_INT_BASE, CAMERA_INT_PIN, GPIO_LOW_LEVEL);
    ROM_GPIOPinIntEnable(CAMERA_INT_BASE, CAMERA_INT_PIN);
    ROM_IntEnable(CAMERA_INT);

    //
    // If we get here, all is well so tell the caller everything is fine.
    //
    return(true);
}

//*****************************************************************************
//
//! Sets or clears notification of various video events.
//!
//! \param ulEvents defines whether notification is enabled for a given event.
//! Valid values are logical OR combinations of \b CAMERA_EVENT_CAPTURE_START,
//! \b CAMERA_EVENT_CAPTURE_END, \b CAMERA_EVENT_CAPTURE_MATCH, \b
//! CAMERA_EVENT_DISPLAY_START, \b CAMERA_EVENT_DISPLAY_END and \b
//! CAMERA_EVENT_DISPLAY_MATCH.
//! \param ulEventMask defines which events are to be affected by this call.
//! Valid values are as for \e ulEvents.  A 1 in a given position in this
//! parameter causes the respective event's notification state to be taken
//! from \e ulEvents. A zero causes the associated bit in \e ulEvents to be
//! ignored.
//!
//! This function enables or disables client notification for one or more
//! asynchronous video events.  The use of a mask parameter, \e ulEventMask,
//! allows notifications for groups of events to be configured without affecting
//! the current states of other events.
//!
//! When event notification is enabled, the callback function supplied on
//! CameraInit() will be called whenever that video event occurs.
//!
//! \return None.
//
//*****************************************************************************
void
CameraEventsSet(unsigned long ulEvents, unsigned long ulEventMask)
{
    //
    // Update the global event notification set based on the client's request.
    //
    g_ulCameraEvents = (g_ulCameraEvents & ~ulEventMask) | ulEvents;

    //
    // Enable the FPGA interrupts required to produce the desired event
    // notifications.  We write to the status register here to clear any
    // unhandled interrupts of the type we are interested in before we enable
    // the interrupt.  This prevents the event from firing immediately based on
    // some stale, past occurrence.
    //
    HWREGH(FPGA_IRQSTAT_REG) |= g_ulCameraEvents;
    HWREGH(FPGA_IRQEN_REG) = g_ulCameraEvents;
}

//*****************************************************************************
//
//! Sets the video capture size and pixel format.
//!
//! \param ulFlags defines the video size and pixel format.
//!
//! This function sets the size of video captured from the camera and the
//! pixel format that is stored in the video buffer.
//!
//! Parameter \e ulFlags is comprised of ORed flags indicating the desired
//! video capture size and pixel format.  It must contain two flags, one each
//! from the following groups:
//!
//! Video dimensions: \b CAMERA_SIZE_VGA for 640x480 capture or \b
//! CAMERA_SIZE_QVGA for 320x240 capture.
//!
//! Pixel format: \b CAMERA_FORMAT_RGB565 to capture 16bpp RGB data with the
//! red component in the most significant 5 bits of the half word pixel, \b
//! CAMERA_FORMAT_BGR565 to capture RGB data with the blue component in the
//! most significant 5 bits of the pixel, \b CAMERA_FORMAT_YUYV to capture
//! video data in YUV422 format with YUYV component ordering or \b
//! CAMERA_FORMAT_YVYU to capture YUV422 data with YVYU component ordering.
//! Note that direct display of the video data is only possible when using
//! \b CAMERA_FORMAT_RGB565.
//!
//! The caller is responsible for ensuring that the currently-defined video
//! capture buffer is large enough to hold the video frame size defined here.
//!
//! \return None.
//
//*****************************************************************************
void
CameraCaptureTypeSet(unsigned long ulFlags)
{
    unsigned char ucValue;

    //
    // First set the capture resolution.
    //
    if(ulFlags & CAMERA_SIZE_VGA)
    {
        //
        // Set up for VGA (640x480) resolution capture.
        //
        HWREGH(FPGA_SYSCTRL_REG) &= ~FPGA_SYSCTRL_QVGA;
        CameraRegSequenceWrite(g_psCameraSizeVGA, NUM_VGA_REGISTERS);
    }
    else
    {
        //
        // Set up for QVGA (320x240) resolution capture.
        //
        CameraRegSequenceWrite(g_psCameraSizeQVGA, NUM_QVGA_REGISTERS);
        HWREGH(FPGA_SYSCTRL_REG) |= FPGA_SYSCTRL_QVGA;
    }

    //
    // Now set the appropriate pixel format.  The size setting code sets things
    // up correctly for RGB565 so we don't need to do anything if asked for this
    // format.
    //
    if(ulFlags & CAMERA_FORMAT_BGR565)
    {
        //
        // Swap the R and B component positions in the pixel.
        //
        ucValue = CameraRegRead(0x0C);
        CameraRegWrite(0x0C, ucValue | 0x20);
    }
    else if(ulFlags & CAMERA_FORMAT_YUYV)
    {
        //
        // Set YUV output format.
        //
        ucValue = CameraRegRead(0x12);
        CameraRegWrite(0x12, ucValue & 0xF0);
    }
    else if(ulFlags & CAMERA_FORMAT_YVYU)
    {
        //
        // Set YUV output format.
        //
        ucValue = CameraRegRead(0x12);
        CameraRegWrite(0x12, ucValue & 0xF0);

        //
        // Swap the YU/YV order.
        //
        ucValue = CameraRegRead(0x0C);
        CameraRegWrite(0x0C, ucValue | 0x10);
    }
}

//*****************************************************************************
//
//! Sets the video capture buffer.
//!
//! \param ulAddr is the address of the video capture buffer in the FPGA SRAM.
//! This address must be between 0x0 and (0x100000 - video frame size).
//! \param usStride is the width of the video capture buffer in bytes.  This
//! must be at least as wide as a single line of the configured video capture
//! frame size (640 * 2 for VGA or 320 * 2 for QVGA) and must be even.
//! \param bAsync indicates whether to wait for the capture change to take
//! effect before returning or not.  If \b true, the call will return
//! immediately.  If \b false, the call will block until the new capture
//! buffer parameters have been read and applied.
//!
//! This function may be called to move or resize the video capture buffer.
//! The initial buffer is configured during a call to CaptureInit() with stride
//! 1280 for VGA resolution or 640 for QVGA at the location provided in the
//! \b ulCaptureAddr parameter to that function.
//!
//! The caller is responsible for ensuring that the video capture buffer is
//! large enough to hold the currently configured video frame size and that the
//! buffer does not overlap with any other buffer in FPGA SRAM (specifically
//! the graphics buffer).
//!
//! \return None.
//
//*****************************************************************************
void
CameraCaptureBufferSet(unsigned long ulAddr, unsigned short usStride,
                       tBoolean bAsync)
{
    ASSERT(ulAddr < 0x100000);
    ASSERT((usStride & 1) == 0);

    //
    // Write the buffer address.  We use a single 32bit write to write both
    // the VML and VMH registers since this will be broken into 2 half word
    // writes for us by the hardware.
    //
    HWREG(FPGA_VML_REG) = ulAddr;

    //
    // Write the buffer stride
    //
    HWREGH(FPGA_VMS_REG) = usStride;

    //
    // The current FPGA implementation doesn't have a register for the
    // buffer height so we merely ignore the parameter for now.  The parameter
    // is left in the API, however, since it may be useful later.
    //

    //
    // Were we asked to perform this change synchronously?
    //
    if(!bAsync)
    {
        //
        // The caller wants us to wait for the change to take effect.
        //
        WaitForFrameEnd(true);
    }
}

//*****************************************************************************
//
//! Enables or disables color bar output from the camera.
//!
//! \param bEnable is set to \b true to enable color bar output from the camera
//! or \b false to disable it and return to normal video capture.
//! \param bMix is set to \b true to mix the color bars with the image captured
//! by the camera or \b false to replace the camera image completely with the
//! color bars.
//!
//! This function may be used to enable or disable color bars in the output
//! from the camera.  When \e bMix is \b false, the output from the camera is
//! replaced with a standard pattern of vertical color bars.  When \e bMix is
//! \b true, the normal video output is mixed with the color bar image.
//!
//! \return None.
//
//*****************************************************************************
void
CameraColorBarsEnable(tBoolean bEnable, tBoolean bMix)
{
    unsigned char ucVal;

    //
    // Have we been asked to control the opaque or mixed color bars?
    //
    if(bMix)
    {
        //
        // Enable or disable the color bars which are mixed with the camera
        // output.
        //
        ucVal = CameraRegRead(0x0C);
        ucVal = bEnable ? (ucVal | 0x01) : (ucVal & ~0x01);
        CameraRegWrite(0x0C, ucVal);
    }
    else
    {
        //
        // Enable or disable opaque color bars.
        //
        ucVal = CameraRegRead(0x82);
        ucVal = bEnable ? (ucVal | 0x0C) : (ucVal & ~0x0C);
        CameraRegWrite(0x82, ucVal);
    }
}

//*****************************************************************************
//
//! Starts video capture.
//!
//! This call starts the process of capturing video frames.  Frames from the
//! camera are written to the buffer configured by a call to the CameraInit()
//! or CameraCaptureBufferSet() functions.  Following this call, events \b
//! CAMERA_EVENT_CAPTURE_START and \b CAMERA_EVENT_CAPTURE_END will be notified
//! to the application callback if they have been enabled via a call to the
//! CameraEventsSet() function.
//!
//! \return None.
//
//*****************************************************************************
void
CameraCaptureStart(void)
{
    //
    // Set the control bit telling the FPGA to start capturing video.
    //
    HWREGH(FPGA_SYSCTRL_REG) |= FPGA_SYSCTRL_VCEN;
}

//*****************************************************************************
//
//! Stops video capture.
//!
//! \param bAsync indicates whether the function call should return immediately
//! (\b true) or wait until the current frame capture has completed (\b false)
//! before returning.
//!
//! This call stops the capture of video frames from the camera.  Capture will
//! stop at the end of the frame which is currently being read and the caller
//! may choose to return immediately or wait until the end of the frame by
//! setting the \e bAsync parameter appropriately.  In cases where \e bAsync is
//! set to \b true, capture will end following the next \b
//! CAMERA_EVENT_CAPTURE_END event.
//!
//! \return None.
//
//*****************************************************************************
void
CameraCaptureStop(tBoolean bAsync)
{
    //
    // Were we asked to perform this change synchronously?
    //
    if(!bAsync)
    {
        //
        // The caller wants us to wait for the change to take effect.  We wait
        // for the end of the current frame then stop in this case.
        //
        WaitForFrameEnd(true);
    }

    //
    // Clear the control bit telling the FPGA to stop capturing video.
    //
    HWREGH(FPGA_SYSCTRL_REG) &= ~FPGA_SYSCTRL_VCEN;
}

//*****************************************************************************
//
//! Sets the capture line on which \b CAMERA_EVENT_CAPTURE_MATCH is generated.
//!
//! \param usLine is the video line number at which \b
//! CAMERA_EVENT_CAPTURE_MATCH will be generated.  Valid values are 0 to 479 if
//! VGA resolution is being captured or 0 to 239 if QVGA is being captured.
//!
//! This call sets the video line at which the \b CAMERA_EVENT_CAPTURE_MATCH
//! event will be generated assuming this event has previously been enabled
//! using a call to CameraEventsSet().
//!
//! \return None.
//
//*****************************************************************************
void
CameraCaptureMatchSet(unsigned short usLine)
{
    //
    // Set the capture match register with the line number provided.
    //
    HWREGH(FPGA_VCRM_REG) = usLine;
}

//*****************************************************************************
//
//! Sets the address and size of the video display buffer.
//!
//! \param ulAddr is the address of the video display buffer in the FPGA SRAM.
//! This address must be between 0x0 and (0x100000 - video frame size).
//! \param usStride is the stride of the video display buffer in bytes.  The
//! stride is defined as the number of bytes between vertically adjacent pixels.
//! \param bAsync indicates whether the function call should return immediately
//! (\b true) or wait until the new buffer parameters have taken effect (/b
//! false) before returning.
//!
//! This call sets the address and stride of the buffer from which video should
//! be displayed.  It is assumed that the image contained in this buffer is the
//! size of the video currently being captured, VGA or QVGA.
//!
//! When displaying live video, the buffer set here will generally be the same
//! buffer as was passed to CameraCaptureBufferSet() or CameraInit().
//! Decoupling the capture and display buffers, however, offers applications
//! the option of capturing into one buffer while displaying from a different
//! region of memory.
//!
//! \return None.
//
//*****************************************************************************
void
CameraDisplayBufferSet(unsigned long ulAddr, unsigned short usStride,
                       tBoolean bAsync)
{
    ASSERT(ulAddr < 0x100000);
    ASSERT((usStride & 1) == 0);

    //
    // Write the buffer address.  We use a single 32bit write to write both
    // the LVML and LVMH registers since this will be broken into 2 half word
    // writes for us by the hardware.
    //
    HWREG(FPGA_LVML_REG) = ulAddr;

    //
    // Write the buffer stride.
    //
    HWREGH(FPGA_LVMS_REG) = usStride;

    //
    // Were we asked to perform this change synchronously?
    //
    if(!bAsync)
    {
        //
        // The caller wants us to wait for the change to take effect.
        //
        WaitForFrameEnd(false);
    }
}

//*****************************************************************************
//
//! Sets the chromakey color controlling graphics transparency.
//!
//! \param ulRGB is the RGB888 color to use as the graphics chromakey.
//!
//! This call sets the color which will be considered the chromakey.  If this
//! color appears in the graphics buffer and both graphics and video planes are
//! enabled, the video pixel will be shown instead of the graphics pixel at
//! this position.
//!
//! Chromakeying offers an easy way to allow graphics to be displayed on top
//! of the video with the video "shining through" the graphics in selected
//! areas.  To achieve this, paint areas of the graphics plane at which the
//! video is to be visible using the chromakey color.  Areas in which the
//! graphics are to be seen should use colors other than the chromakey.
//!
//! Note that only a simple compare is provided to select between graphics and
//! video using the key - the video pixel is shown if the graphics color is
//! exactly equal to the chromakey color. If you are preparing graphics which
//! contain areas of chromakey, make sure that you do not anti-alias the edges
//! between the visible portion of your graphic and the chromakey areas since
//! these areas containing a blend of the chromakey and graphics colors will
//! show as graphics pixels and result in a chromakey-colored fringe around
//! your graphic.
//!
//! \return None.
//
//*****************************************************************************
void
CameraDisplayChromaKeySet(unsigned long ulRGB)
{
    //
    // Set the chromakey color.
    //
    HWREGH(FPGA_CHRMKEY_REG) = Kitronix320x240x16_FPGAColorMap(ulRGB);
}

//*****************************************************************************
//
//! Enables or disables chromakey mixing of graphics and video.
//!
//! \param bEnable is \b true to enable graphics/video chromakeying and \b
//! false to disable it.
//!
//! This call enables or disables chromakeying.  When disabled, the graphics
//! plane will be displayed in preference to video whenever it is enabled
//! by a call to CameraDisplayStart(), regardless of whether the video plane
//! is also enabled.
//!
//! With chromakey enabled and both graphics and video planes enabled, the
//! graphics and video will be mixed on the display.  At each pixel location,
//! if the graphics plane contains the current chromakey color (as set using a
//! call to CameraDisplayChromakeySet()), the corresponding video pixel will be
//! shown instead of the graphics pixel.
//!
//! Another way to think of this is that graphics plane sits above the video
//! plane with the chromakey color transparent, allowing the video to ``shine
//! through'' the graphics in areas where it exists.
//!
//! \return None.
//
//*****************************************************************************
void
CameraDisplayChromaKeyEnable(tBoolean bEnable)
{
    //
    // Enable or disable chromakeying.
    //
    if(bEnable)
    {
        //
        // Enable chromakey.
        //
        HWREGH(FPGA_SYSCTRL_REG) |= FPGA_SYSCTRL_CMKEN;
    }
    else
    {
        //
        // Disable chromakey.
        //
        HWREGH(FPGA_SYSCTRL_REG) &= ~FPGA_SYSCTRL_CMKEN;
    }
}

//*****************************************************************************
//
//! Enables or disables video display downscaling.
//!
//! \param bDownscale is \b true if the display should show half size video or
//! \b false if full size video is to be displayed.
//!
//! This call allows a client to display video either at the size it was
//! captured or at quarter the capture size (skipping every other pixel and
//! line).  This may be used to show full screen video on the 320x240 display
//! while capturing 640x480 VGA images into the video capture buffer.
//!
//! Note that the downscaling uses a simple pixel skipping approach so video
//! display quality will likely be somewhat lower when using this downscale
//! option than would be achieved by capturing at the required resolution and
//! displaying the native size without downscaling on display.
//!
//! \return None.
//
//*****************************************************************************
void
CameraDisplayDownscaleSet(tBoolean bDownscale)
{
    //
    // Enable or disable video downscaling.
    //
    if(bDownscale)
    {
        //
        // Enable downscaling.
        //
        HWREGH(FPGA_SYSCTRL_REG) |= FPGA_SYSCTRL_VSCALE;
    }
    else
    {
        //
        // Disable downscaling.
        //
        HWREGH(FPGA_SYSCTRL_REG) &= ~FPGA_SYSCTRL_VSCALE;
    }
}

//*****************************************************************************
//
//! Enables the video display plane.
//!
//! This call instructs the FPGA to enable the video display plane using the
//! current size, position and buffer pointer settings.
//!
//! \return None.
//
//*****************************************************************************
void
CameraDisplayStart(void)
{
    //
    // Enable/disable the video display plane.
    //
    HWREGH(FPGA_SYSCTRL_REG) |= FPGA_SYSCTRL_VDEN;
}

//*****************************************************************************
//
//! Disables the video display plane.
//!
//! \param bAsync indicates whether the function call should return immediately
//! (\b true) or wait until the current display frame has completed (/b false)
//! before returning.
//!
//! This call disables the video display plane on the LCD.  The caller may
//! choose to return immediately or wait until the end of the frame by setting
//! the \e bAsync parameter appropriately.  In cases where \e bAsync is set to
//! \b true, display processing will end following the next
//! \b CAMERA_EVENT_DISPLAY_END event.
//!
//! \return None.
//
//*****************************************************************************
void
CameraDisplayStop(tBoolean bAsync)
{
    //
    // Stop automatic display processing
    //
    HWREGH(FPGA_SYSCTRL_REG) &= ~FPGA_SYSCTRL_VDEN;

    //
    // Were we asked to perform this change synchronously?
    //
    if(!bAsync)
    {
        //
        // The caller wants us to wait for the change to take effect.
        //
        WaitForFrameEnd(false);
    }
}

//*****************************************************************************
//
//! Sets the display line on which \b CAMERA_EVENT_DISPLAY_MATCH is generated.
//!
//! \param usLine is the display line number at which \b
//! CAMERA_EVENT_DISPLAY_MATCH will be generated.  Valid values are 0 to 239.
//!
//! This call sets the display line at which the \b CAMERA_EVENT_DISPLAY_MATCH
//! event will be generated assuming this event has previously been enabled
//! using a call to CameraEventsSet().  The event is generated as the FPGA
//! scans out the data for the requested display row.  Note, however, that the
//! FPGA is merely writing data into the display controller's frame buffer so
//! this event cannot be used to synchronize display updates to prevent
//! ``tearing.''
//!
//! \return None.
//
//*****************************************************************************
void
CameraDisplayMatchSet(unsigned short usLine)
{
    //
    // Set the LCD row match register with the line number provided.
    //
    HWREGH(FPGA_LRM_REG) = usLine;
}

//*****************************************************************************
//
//! Sets the brightness of the video image.
//!
//! \param ucBrightness is an unsigned value representing the required
//! brightness for the video image. Values in the range [0, 255] are scaled to
//! represent exposure values between -4EV and +4EV with \b BRIGHTNESS_NORMAL
//! (128) representing ``normal'' brightness or 0EV adjustment.
//!
//! This function sets the brightness (exposure) of the captured video image.
//! Values of \e ucBrightness above \b BRIGHTNESS_NORMAL cause the image to be
//! brighter while values below this darken the image.
//!
//! \return None.
//
//*****************************************************************************
void
CameraBrightnessSet(unsigned char ucBrightness)
{
    unsigned long ulLoop;

    //
    // Loop through the available brightness settings.
    //
    for(ulLoop = 0; ulLoop < NUM_BRIGHTNESS_SETTINGS; ulLoop++)
    {
        //
        // Does the supplied brightness fall into the current band?
        //
        if(ucBrightness <= g_psBrightness[ulLoop].ucThreshold)
        {
            //
            // Yes - write the registers to set the appropriate brightness
            // level.
            //
            CameraRegWrite(0x24, g_psBrightness[ulLoop].ucReg24Val);
            CameraRegWrite(0x25, g_psBrightness[ulLoop].ucReg25Val);
            CameraRegWrite(0x26, g_psBrightness[ulLoop].ucReg26Val);
            return;
        }
    }
}

//*****************************************************************************
//
//! Sets the color saturation of the video image.
//!
//! \param ucSaturation is a value representing the required saturation
//! for the video image.  Normal saturation is represented by value \b
//! SATURATION_NORMAL (128) with values above this increasing saturation and
//! values below decreasing it.
//!
//! This function sets the color saturation of the captured video image. Higher
//! values result in more vivid images and lower values produce a muted or even
//! monochrome effect.
//!
//! \return None.
//
//*****************************************************************************
void
CameraSaturationSet(unsigned char ucSaturation)
{
    unsigned char ucVal;

    //
    // Enable color adjustments.
    //
    ucVal = CameraRegRead(0x81);
    ucVal |= 0x33;
    CameraRegWrite(0x81, ucVal);

    //
    // Write the new saturation value.
    //
    CameraRegWrite(0xD8, ucSaturation >> 1);
    CameraRegWrite(0xD9, ucSaturation >> 1);

    //
    // Enable saturation adjustment.
    //
    ucVal = CameraRegRead(0xD2);
    ucVal |= 0x02;
    CameraRegWrite(0xD2, ucVal);
}

//*****************************************************************************
//
//! Sets the contrast of the video image.
//!
//! \param ucContrast is a value representing the required contrast for the
//! video image.  Normal contrast is represented by value \b CONTRAST_NORMAL
//! (128) with values above this increasing contrast and values below
//! decreasing it.
//!
//! This function sets the contrast of the captured video image. Higher
//! values result in higher contrast images and lower values result in lower
//! contrast images.
//!
//! \return None.
//
//*****************************************************************************
void
CameraContrastSet(unsigned char ucContrast)
{
    unsigned char ucVal;
    unsigned long ulLoop;

    //
    // Enable color adjustments.
    //
    ucVal = CameraRegRead(0x81);
    ucVal |= 0x33;
    CameraRegWrite(0x81, ucVal);

    //
    // Write the new contrast value.  Register values for 0xD5 and 0xD2 are
    // the same for each setting but 0xD4 and 0xD3 vary.
    //
    CameraRegWrite(0xD5, 0x20);

    //
    // Loop through the available contrast settings.
    //
    for(ulLoop = 0; ulLoop < NUM_CONTRAST_SETTINGS; ulLoop++)
    {
        //
        // Does the supplied contrast fall into the current band?
        //
        if(ucContrast <= g_psContrast[ulLoop].ucThreshold)
        {
            //
            // Yes - write the registers to set the appropriate contrast.
            //
            CameraRegWrite(0xD4, g_psContrast[ulLoop].ucRegD4Val);
            CameraRegWrite(0xD3, g_psContrast[ulLoop].ucRegD3Val);
            break;
        }
    }

    //
    // Enable contrast adjustments.
    //
    ucVal = CameraRegRead(0xD2);
    ucVal |= 0x04;
    CameraRegWrite(0xD2, ucVal);

    //
    // Set the sense of the contrast adjustment, increase or decrease.
    //
    ucVal = CameraRegRead(0xDC);
    if(ucContrast < CONTRAST_NORMAL)
    {
        ucVal |= 0x04;
    }
    else
    {
        ucVal &= ~0x04;
    }
    CameraRegWrite(0xDC, ucVal);
}

//*****************************************************************************
//
//! Sets the flip (vertical reflection) state of the video.
//!
//! \param bFlip is \b true if the incoming motion video is to be flipped in
//! the vertical direction or \b false to leave the image oriented normally.
//!
//! \return None.
//
//*****************************************************************************
void
CameraFlipSet(tBoolean bFlip)
{
    unsigned char ucVal;

    //
    // Get the existing register value, mask in the appropriately set flip
    // bit and write it back.
    //
    ucVal = CameraRegRead(0x0C);
    ucVal = bFlip ? (ucVal | 0x80) : (ucVal & ~0x80);
    CameraRegWrite(0x0C, ucVal);
}

//*****************************************************************************
//
//! Sets the mirror (horizontal reflection) state of the video.
//!
//! \param bMirror is \b true if the incoming motion video is to be mirrored in
//! the horizontal direction or \b false to leave the image oriented normally.
//!
//! This function would typically be used for applications where a user is
//! viewing their own video.  Since people are more used to seeing a mirror
//! image of themselves, this is often more comfortable than viewing the non-
//! inverted image.
//!
//! \return None.
//
//*****************************************************************************
void
CameraMirrorSet(tBoolean bMirror)
{
    unsigned char ucVal;

    //
    // Get the existing register value, mask in the appropriately set mirror
    // bit and write it back.
    //
    ucVal = CameraRegRead(0x0C);
    ucVal = bMirror ? (ucVal | 0x40) : (ucVal & ~0x40);
    CameraRegWrite(0x0C, ucVal);
}

//*****************************************************************************
//
//! Reads pixel data from a given position in the video image.
//!
//! \param bCapBuffer is \b true to read data from the image described by
//! the current video capture buffer settings or \b false to read from the
//! current video display buffer.
//! \param usX is the X coordinate of the start of the data to be read.
//! \param usY is the Y coordinate of the start of the data to be read.
//! \param ulNumPixels is the number of pixels to be read. Pixels are read
//! starting at \e usX, \e usY and progressing to the right then downwards.
//! \param b24bit indicates whether to return raw (RGB565) data or extracted
//! colors in RGB24 format.
//! \param pusBuffer points to the buffer into which the pixel data should be
//! copied.
//!
//! This function allows an application to read back a section of a captured
//! image into a buffer in Stellaris internal SRAM.  The data may be returned
//! either in the native 16bpp format supported by the camera and LCD or in
//! 24bpp RGB format.  The 24bpp format returned places the blue component in
//! the lowest memory location followed by the green component then the red
//! component.
//!
//! \return None.
//
//*****************************************************************************
void
CameraImageDataGet(tBoolean bCapBuffer, unsigned short usX, unsigned short usY,
                   unsigned long ulNumPixels, tBoolean b24bit,
                   unsigned short *pusBuffer)
{
    unsigned long ulBase, ulWidth, ulStride;
    unsigned short usPixel;
    unsigned char *pucBuffer;

    //
    // Get the width of the video in the buffer.  We assume the capture and
    // display images are the same dimensions.
    //
    ulWidth = (HWREGH(FPGA_SYSCTRL_REG) & FPGA_SYSCTRL_QVGA) ? 320 : 640;

    //
    // Which buffer are we to read from?
    //
    if(bCapBuffer)
    {
        //
        // Reading from the capture buffer.  Retrieve the pointer and
        // dimensions from the capture registers.
        //
        ulBase = HWREG(FPGA_VML_REG);
        ulStride = (unsigned long)HWREGH(FPGA_VMS_REG);
    }
    else
    {
        //
        // Reading from the display buffer.  Retrieve the pointer and
        // dimensions from the display registers.
        //
        ulBase = HWREG(FPGA_LVML_REG);
        ulStride = (unsigned long)HWREGH(FPGA_LVMS_REG);
    }

    //
    // Set up for the copy loops.
    //
    ulBase += (usY * ulStride);
    pucBuffer = (unsigned char *)pusBuffer;

    //
    // Read the required number of pixels.
    //
    while(ulNumPixels)
    {
        //
        // Get a pixel.
        //
        FPGAREADH((ulBase + (usX * 2)), usPixel);

        //
        // Write the pixel to the output.
        //
        if(b24bit)
        {
            *(unsigned char *)pucBuffer++ = BLUEFROM565(usPixel);
            *(unsigned char *)pucBuffer++ = GREENFROM565(usPixel);
            *(unsigned char *)pucBuffer++ = REDFROM565(usPixel);
        }
        else
        {
            *pusBuffer++ = usPixel;
        }

        //
        // Move to the next pixel
        //
        usX++;
        if(usX >= ulWidth)
        {
            usX = 0;
            ulBase += ulStride;
        }

        //
        // Decrement our pixel count.
        //
        ulNumPixels--;
    }
}


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
