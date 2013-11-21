//*****************************************************************************
//
// camera.h - Public definitions and prototypes for the low level driver for
//            the camera functions on the FPGA/camera daughter board.
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
// This is part of revision 10636 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#ifndef __CAMERA_H__
#define __CAMERA_H__

//*****************************************************************************
//
//! \addtogroup camera_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Flags used to set the video size and pixel format.  These may be passed in
// the ulFlags parameters to CameraInit and CameraCaptureTypeSet.
//
//*****************************************************************************
#define CAMERA_SIZE_VGA         0x01
#define CAMERA_SIZE_QVGA        0x02
#define CAMERA_FORMAT_RGB565    0x04
#define CAMERA_FORMAT_BGR565    0x08
#define CAMERA_FORMAT_YUYV      0x10
#define CAMERA_FORMAT_YVYU      0x20

//*****************************************************************************
//
// Events that may be passed to the camera callback function.  These values will
// be ORed together if multiple events are outstanding.
//
//*****************************************************************************

//
//! This event is generated as each video frame capture begins.
//
#define CAMERA_EVENT_CAPTURE_START  0x0001

//
//! This event is generated as each video frame capture ends.
//
#define CAMERA_EVENT_CAPTURE_END    0x0002

//
//! This event is generated during each captured video frame when the captured
//! line number equals the value passed to CameraCaptureMatchSet().
//
#define CAMERA_EVENT_CAPTURE_MATCH  0x0004

//
//! This event is generated as each frame sent to the LCD ends.  Note that it
//! is not possible to use this event to prevent display ``tearing'' since the
//! FPGA has no access to the display's internal frame and line sync signals.
//
#define CAMERA_EVENT_DISPLAY_START  0x0008

//
//! This event is generated as each frame sent to the LCD begins.  Note that it
//! is not possible to use this event to prevent display ``tearing'' since the
//! FPGA has no access to the display's internal frame and line sync signals.
//
#define CAMERA_EVENT_DISPLAY_END    0x0010

//
//! This event is generated during each frame sent to the LCD when the display
//! line number equals the value passed to CameraDisplayMatchSet().
//
#define CAMERA_EVENT_DISPLAY_MATCH  0x0020

//*****************************************************************************
//
// The camera driver notification callback function prototype.
//
//*****************************************************************************
typedef void (* tCameraCallback)(unsigned long ulEvents);

//*****************************************************************************
//
//! The value to pass to the CameraBrightnessSet() function to set normal
//! brightness level in the video image.
//
//*****************************************************************************
#define BRIGHTNESS_NORMAL 0x80

//*****************************************************************************
//
//! The value to pass to the CameraSaturationSet() function to set normal
//! color saturation level in the video image.
//
//*****************************************************************************
#define SATURATION_NORMAL 0x80

//*****************************************************************************
//
//! The value to pass to the CameraContrastSet() function to set normal contrast
//! in the video image.
//
//*****************************************************************************
#define CONTRAST_NORMAL   0x80

//*****************************************************************************
//
// When using a simple, single capture/single display buffer model, the
// following labels can be used to describe the video and graphics buffers
// in FPGA SRAM.  These assume a QVGA-sized, 16bpp display buffer at the
// base of SRAM and a VGA-sized, 16bpp video buffer immediately above this.
//
//*****************************************************************************
#define GRAPHICS_BUFF_BASE   0x00000000
#define GRAPHICS_BUFF_STRIDE (320 * 2)
#define GRAPHICS_BUFF_WIDTH  320
#define GRAPHICS_BUFF_HEIGHT 240

#define VIDEO_BUFF_BASE      (GRAPHICS_BUFF_BASE + (GRAPHICS_BUFF_STRIDE *    \
                              GRAPHICS_BUFF_HEIGHT))
#define VIDEO_BUFF_STRIDE    (640 * 2)
#define VIDEO_BUFF_WIDTH     640
#define VIDEO_BUFF_HEIGHT    480

#define SRAM_FREE_BASE       (VIDEO_BUFF_BASE +  (VIDEO_BUFF_STRIDE *         \
                              VIDEO_BUFF_HEIGHT))
#define SRAM_FREE_SIZE       (0x100000 - SRAM_FREE_BASE)

//*****************************************************************************
//
// Function Prototypes.
//
//*****************************************************************************
extern tBoolean CameraInit(unsigned long ulFlags, unsigned long ulCaptureAddr,
                          tCameraCallback pfnCallback);
extern void CameraEventsSet(unsigned long ulEvents, unsigned long ulEventMask);

extern void CameraCaptureTypeSet(unsigned long ulFlags);
extern void CameraCaptureBufferSet(unsigned long ulAddr,
                                   unsigned short usStride,
                                   tBoolean bAsync);
extern void CameraColorBarsEnable(tBoolean bEnable, tBoolean bMix);
extern void CameraCaptureStart(void);
extern void CameraCaptureStop(tBoolean bAsync);
extern void CameraCaptureMatchSet(unsigned short usLine);

extern void CameraDisplayBufferSet(unsigned long ulAddr,
                                   unsigned short usStride,
                                   tBoolean bAsync);
extern void CameraDisplayDownscaleSet(tBoolean bDownscale);
extern void CameraDisplayChromaKeySet(unsigned long ulRGB);
extern void CameraDisplayChromaKeyEnable(tBoolean bEnable);
extern void CameraDisplayStart(void);
extern void CameraDisplayStop(tBoolean bAsync);
extern void CameraDisplayMatchSet(unsigned short usLine);

extern void CameraBrightnessSet(unsigned char ucBrightness);
extern void CameraSaturationSet(unsigned char ucSaturation);
extern void CameraContrastSet(unsigned char ucContrast);

extern void CameraFlipSet(tBoolean bFlip);
extern void CameraMirrorSet(tBoolean bMirror);

extern void CameraImageDataGet(tBoolean bCapBuffer, unsigned short usX,
                               unsigned short usY, unsigned long ulNumPixels,
                               tBoolean b24bit, unsigned short *pusBuffer);

//*****************************************************************************
//
// Macros to extract red, green and blue components from a pixel.
//
//*****************************************************************************

//
//! This macro can be used to extract an 8 bit red color component from a 16 bit
//! RGB pixel read from the video camera.
//!
//! \param usPix is a 16 bit pixel in the format captured by the video camera.
//!
//! \return Returns the 8 bit red color component extracted from the supplied
//! pixel.
//
#define RFROMPIXEL(usPix) (((usPix) & 0xF800) >> 8)

//
//! This macro can be used to extract an 8 green red color component from a 16
//! bit RGB pixel read from the video camera.
//!
//! \param usPix is a 16 bit pixel in the format captured by the video camera.
//!
//! \return Returns the 8 bit green color component extracted from the supplied
//! pixel.
//
#define GFROMPIXEL(usPix) (((usPix) & 0x07E0) >> 3)

//
//! This macro can be used to extract an 8 green blue color component from a 16
//! bit RGB pixel read from the video camera.
//!
//! \param usPix is a 16 bit pixel in the format captured by the video camera.
//!
//! \return Returns the 8 bit blue color component extracted from the supplied
//! pixel.
//
#define BFROMPIXEL(usPix) ((usPix) & 0x001F)

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
#endif // __CAMERA_H__
