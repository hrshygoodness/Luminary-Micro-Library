//*****************************************************************************
//
// vidwidget.c - Video image display and button widgets.
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

#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "vidwidget.h"
#include "camera.h"

//*****************************************************************************
//
//! \addtogroup vidwidget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Make sure we have access to an "abs-like" macro.
//
//*****************************************************************************
#define absval(a) (((a) < 0) ? -(a) : (a))

//*****************************************************************************
//
// Draws a video widget.
//
// \param pWidget is a pointer to the video widget to be drawn.
//
// This function draws a video widget on the display.  This is called in
// response to a \b WIDGET_MSG_PAINT message.
//
// \return None.
//
//*****************************************************************************
static void
VideoWidgetPaint(tWidget *pWidget)
{
    tRectangle sRect;
    tVideoWidget *pVideo;
    tContext sCtx;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Convert the generic widget pointer into a Video widget pointer.
    //
    pVideo = (tVideoWidget *)pWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, pWidget->pDisplay);

    //
    // Initialize the clipping region based on the extents of this rectangular
    // Video widget.
    //
    GrContextClipRegionSet(&sCtx, &(pWidget->sPosition));

    //
    // Take a copy of the current widget position.
    //
    sRect=pWidget->sPosition;

    //
    // See if the video widget outline style is selected.
    //
    if(pVideo->ulStyle & VW_STYLE_OUTLINE)
    {
        unsigned long ulLoop;

        GrContextForegroundSet(&sCtx, pVideo->ulOutlineColor);

        //
        // Outline the video widget with the outline color.
        //
        for(ulLoop = 0; ulLoop < (unsigned long)pVideo->ucBorderWidth; ulLoop++)
        {
            GrRectDraw(&sCtx, &sRect);
            sRect.sXMin++;
            sRect.sYMin++;
            sRect.sXMax--;
            sRect.sYMax--;
        }
    }

    //
    // Fill the video widget with the key color.
    //
    GrContextForegroundSet(&sCtx, pVideo->ulKeyColor);
    GrRectFill(&sCtx, &sRect);
}

//*****************************************************************************
//
// Set the video display buffer pointer to display the correct section of
// the captured video in the window.
//
//*****************************************************************************
static void
DisplayPositionSet(tVideoWidget *pWidget)
{
    unsigned long ulAddr;
    tVideoInst *psInst;

    //
    // Get our video instance pointer.
    //
    psInst = pWidget->psVideoInst;
    //
    // By default, we display from the top left of the captured video image.
    //
    ulAddr = psInst->ulCapAddr;

    //
    // Is the video image size larger than the display?  This will be the case
    // if we are capturing VGA resolution and not downscaling.
    //
    if(!(pWidget->ulStyle && VW_STYLE_DOWNSCALE) ||
       (pWidget->ulStyle & VW_STYLE_VGA))
    {
        //
        // The video image is larger than the display so we can reposition it.
        // We do this by moving the display start pointer within the buffer.
        // We assume that the offsets passed are already clipped such that
        // they will not cause the image to fall off any side of the screen.
        //
        ulAddr += ((psInst->sXOffset * 2) +
                   (psInst->sYOffset * psInst->usStride));
    }

    //
    // Set up the buffer from which the FPGA will display the captured video.
    //
    CameraDisplayBufferSet(ulAddr, psInst->usStride, true);
}

//*****************************************************************************
//
// Handles changes required as a result of pointer movement.
//
// \param pVideo is a pointer to the video widget.
// \param lX is the X coordinate of the pointer event.
// \param lY is the Y coordinate of the pointer event.
//
// This function is called whenever a new pointer message is received which
// may indicate a change in position that requires us to scroll the video image.
// If necessary, the OnScroll callback is made indicating the current offset
// of the image.  This function does not cause any repainting of the widget.
//
// \return Returns \b true if one or other of the image offsets changed as a
// result of the call or \b false if no changes were made.
//
//*****************************************************************************
static tBoolean
VideoWidgetHandlePtrPos(tVideoWidget *pVideo, long lX, long lY)
{
    short sDeltaX, sDeltaY, sXOffset, sYOffset, sMaxX, sMaxY;
    tBoolean bRetcode;

    //
    // Assume nothing changes until we know otherwise.
    //
    bRetcode = false;

    //
    // Did the pointer position change?
    //
    if((pVideo->psVideoInst->sXStart == (short)lX) &&
       (pVideo->psVideoInst->sYStart == (short)lY))
    {
        //
        // The pointer position didn't change so we have nothing to do.
        //
        return(false);
    }

    //
    // Determine the new offset and apply it to the current total offset.
    //
    sDeltaX = pVideo->psVideoInst->sXStart - (short)lX;
    sDeltaY = pVideo->psVideoInst->sYStart - (short)lY;

    sXOffset = pVideo->psVideoInst->sXOffset + sDeltaX;
    sYOffset = pVideo->psVideoInst->sYOffset + sDeltaY;

    //
    // Now check to make sure that neither offset causes the image to move
    // out of the screen and limit them as required.
    //
    sMaxX = pVideo->psVideoInst->usDispWidth -
            pVideo->sBase.pDisplay->usWidth;
    sMaxY = pVideo->psVideoInst->usDispHeight -
            pVideo->sBase.pDisplay->usHeight;
    sXOffset = (sXOffset > sMaxX) ? sMaxX : sXOffset;
    sYOffset = (sYOffset > sMaxY) ? sMaxY : sYOffset;
    sXOffset = (sXOffset < 0) ? 0 : sXOffset;
    sYOffset = (sYOffset < 0) ? 0 : sYOffset;

    //
    // Now we've calculated the new image offset.  Is it different from the
    // previous offset?
    //
    if((sXOffset != pVideo->psVideoInst->sXOffset) ||
       (sYOffset != pVideo->psVideoInst->sYOffset))
    {
        //
        // Yes - something changed.
        //
        bRetcode = true;
        pVideo->psVideoInst->sXOffset = sXOffset;
        pVideo->psVideoInst->sYOffset = sYOffset;

        //
        // Call the camera driver to set the appropriate video display
        // rectangle.
        //
        DisplayPositionSet(pVideo);

        //
        // Do we need to make a callback?
        //
        if(pVideo->pfnOnScroll)
        {
            pVideo->pfnOnScroll((tWidget *)pVideo, sXOffset, sYOffset);
        }
    }

    //
    // Remember where the pointer was the last time we looked at it.
    //
    pVideo->psVideoInst->sXStart = (short)lX;
    pVideo->psVideoInst->sYStart = (short)lY;

    //
    // Tell the caller whether anything changed or not.
    //
    return(bRetcode);
}

//*****************************************************************************
//
//! Handles pointer events for a video widget.
//!
//! \param pWidget is a pointer to the video widget.
//! \param ulMsg is the pointer event message.
//! \param lX is the X coordinate of the pointer event.
//! \param lY is the Y coordinate of the pointer event.
//!
//! This function processes pointer event messages for a Video widget.
//! This is called in response to a \b WIDGET_MSG_PTR_DOWN,
//! \b WIDGET_MSG_PTR_MOVE, and \b WIDGET_MSG_PTR_UP messages.
//!
//! If the widget has the \b #VW_STYLE_LOCKED flag set, the input is ignored
//! and this function returns immediately.
//!
//! Pointer messages are used to control scrolling of the video image within
//! the area of the widget.  In this case, any pointer movement that will cause
//! a change in the image position is reported to the client via the OnScroll
//! callback function.
//!
//! \return Returns 1 if the coordinates are within the extents of the widget
//! and 0 otherwise.
//
//*****************************************************************************
static long
VideoWidgetClick(tWidget *pWidget, unsigned long ulMsg, long lX, long lY)
{
    tVideoWidget *pVideo;
    tBoolean bWithinWidget;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Convert the generic widget pointer into a Video widget pointer.
    //
    pVideo = (tVideoWidget *)pWidget;

    //
    // Is the widget currently locked?
    //
    if(pVideo->ulStyle & VW_STYLE_LOCKED)
    {
        //
        // We ignore this message and have the widget manager pass it back
        // up the tree.
        //
        return(0);
    }

    //
    // Does this event occur within the bounds of this widget?
    //
    bWithinWidget = GrRectContainsPoint(&pWidget->sPosition, lX, lY);

    //
    // Which kind of message did we receive?  We handle pointer messages to
    // allow the user to scroll the image around within the widget.
    //
    switch(ulMsg)
    {
        //
        // The user has pressed the touchscreen.
        //
        case WIDGET_MSG_PTR_DOWN:
        {
            //
            // Did this event occur within the bounds of this particular
            // widget?
            //
            if(bWithinWidget)
            {
                //
                // Yes, it's our press so remember where it occurred and
                // remember that the touchscreen is pressed.
                //
                pVideo->psVideoInst->sXStart = (short)lX;
                pVideo->psVideoInst->sYStart = (short)lY;
            }

            break;
        }

        //
        // The touchscreen has been released.
        //
        case WIDGET_MSG_PTR_UP:
        {
            //
            // Has the pointer moved since the last time we looked?  If so
            // handle it appropriately.
            //
            VideoWidgetHandlePtrPos(pVideo, lX, lY);

            break;
        }

        //
        // The pointer position has changed.
        //
        case WIDGET_MSG_PTR_MOVE:
        {
            //
            // Calculate the new image offsets based on the new pointer
            // position.
            //
            VideoWidgetHandlePtrPos(pVideo, lX, lY);

            break;
        }

        default:
            break;
    }

    //
    // Tell the widget manager whether this event occurred within the bounds
    // of this widget.
    //
    return((long)bWithinWidget);
}

//*****************************************************************************
//
//! Handles messages for a Video widget.
//!
//! \param pWidget is a pointer to the Video widget.
//! \param ulMsg is the message.
//! \param ulParam1 is the first parameter to the message.
//! \param ulParam2 is the second parameter to the message.
//!
//! This function receives messages intended for this Video widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! This function is called by the Stellaris Graphics Library Widget Manager.
//! An application should not call this function but should interact with the
//! video widget using the other functions and macros offered by the widget API.
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
long
VideoWidgetMsgProc(tWidget *pWidget, unsigned long ulMsg,
                  unsigned long ulParam1, unsigned long ulParam2)
{
    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Determine which message is being sent.
    //
    switch(ulMsg)
    {
        //
        // The widget paint request has been sent.
        //
        case WIDGET_MSG_PAINT:
        {
            //
            // Handle the widget paint request.
            //
            VideoWidgetPaint(pWidget);

            //
            // Return one to indicate that the message was successfully
            // processed.
            //
            return(1);
        }

        //
        // One of the pointer requests has been sent.
        //
        case WIDGET_MSG_PTR_DOWN:
        case WIDGET_MSG_PTR_MOVE:
        case WIDGET_MSG_PTR_UP:
        {
            //
            // Handle the pointer request, returning the appropriate value.
            //
            return(VideoWidgetClick(pWidget, ulMsg, ulParam1, ulParam2));
        }

        //
        // An unknown request has been sent.
        //
        default:
        {
            //
            // Let the default message handler process this message.
            //
            return(WidgetDefaultMsgProc(pWidget, ulMsg, ulParam1, ulParam2));
        }
    }
}

//*****************************************************************************
//
//! Initializes a Video widget.
//!
//! \param pWidget is a pointer to the video widget to initialize.
//! \param pDisplay is a pointer to the display on which to draw the widget.
//! \param ulBufAddr is the FPGA SRAM address of the start of the video capture
//! buffer. Valid values are from 0 through (1MB - (size of 1 video frame)).
//! \param bVGA is \b true if the capture buffer is sized to accept a 640x480
//! (VGA) video frame or \b false if sized for 320x240 (QVGA).
//! \param lX is the X coordinate of the upper left corner of the video widget.
//! \param lY is the Y coordinate of the upper left corner of the video widget.
//! \param lWidth is the width of the video widget.
//! \param lHeight is the height of the video widget.
//! \param psInst is a pointer to a structure in RAM that the widget can use
//! to hold working variables.
//!
//! This function initializes the provided video widget.  The widget position
//! is set based on the parameters passed. All other widget parameters including
//! style flags are set to 0.  The caller must make use of the various widget
//! functions to set any required parameters or styles after making this call.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetInit(tVideoWidget *pWidget, const tDisplay *pDisplay,
                unsigned long ulBufAddr, tBoolean bVGA, long lX,
                long lY, long lWidth, long lHeight, tVideoInst *psInst)
{
    unsigned long ulIdx;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);
    ASSERT(pDisplay);
    ASSERT(psInst);

    //
    // Clear out the widget structure.
    //
    for(ulIdx = 0; ulIdx < sizeof(tVideoWidget); ulIdx ++)
    {
        ((unsigned char *)pWidget)[ulIdx] = 0;
    }

    //
    // Set the size of the Video widget structure.
    //
    pWidget->sBase.lSize = sizeof(tVideoWidget);

    //
    // Mark this widget as fully disconnected.
    //
    pWidget->sBase.pParent = 0;
    pWidget->sBase.pNext = 0;
    pWidget->sBase.pChild = 0;

    //
    // Save the display pointer.
    //
    pWidget->sBase.pDisplay = pDisplay;

    //
    // Set the extents of this rectangular Video widget.
    //
    pWidget->sBase.sPosition.sXMin = lX;
    pWidget->sBase.sPosition.sYMin = lY;
    pWidget->sBase.sPosition.sXMax = lX + lWidth - 1;
    pWidget->sBase.sPosition.sYMax = lY + lHeight - 1;

    //
    // Use the Video widget message handler to process messages to
    // this Video widget.
    //
    pWidget->sBase.pfnMsgProc = VideoWidgetMsgProc;

    //
    // Clear the video instance structure and attach it to the widget.
    //
    for(ulIdx = 0; ulIdx < sizeof(tVideoInst); ulIdx ++)
    {
        ((unsigned char *)psInst)[ulIdx] = 0;
    }
    pWidget->psVideoInst = psInst;

    //
    // Initialize the camera module and configure the video capture buffer.
    //
    CameraInit(((bVGA ? CAMERA_SIZE_VGA : CAMERA_SIZE_QVGA) |
               CAMERA_FORMAT_RGB565), ulBufAddr, 0);

    pWidget->psVideoInst->ulCapAddr = ulBufAddr;
    if(bVGA)
    {
        pWidget->psVideoInst->usStride = VIDEO_BUFF_STRIDE;
        pWidget->psVideoInst->usCapWidth = 640;
        pWidget->psVideoInst->usCapHeight = 480;
    }
    else
    {
        pWidget->psVideoInst->usStride = VIDEO_BUFF_STRIDE / 2;
        pWidget->psVideoInst->usCapWidth = 320;
        pWidget->psVideoInst->usCapHeight = 240;
    }
}

//*****************************************************************************
//
//! Initializes the camera associated with a statically allocated video widget.
//!
//! \param pWidget is a pointer to the video widget to initialize.
//! \param ulBufAddr is the FPGA SRAM address of the start of the video capture
//! buffer. Valid values are from 0 through (1MB - (size of 1 video frame)).
//!
//! This function must be called to correctly initialize a statically allocated
//! video widget.  In ensures that all the widget parameters which are relevant
//! are passed to the low level camera driver and that the widget is ready to
//! start capturing and displaying motion video.
//!
//! Parameter \e ulBufAddr must point to an area of FPGA SRAM large enough to
//! hold the largest video image that the application may ever capture.  If only
//! QVGA images are being used, the mimumum buffer size is (320 * 240 * 2) bytes
//! but if VGA resolution is ever selected using VideoWidgetResolutionSet(),
//! the buffer must be at least (640 * 480 * 2) bytes in size.
//!
//! A dynamically created video widget which makes calls to the various
//! functions in the widget API to initialize each field of the widget structure
//! need not call this function.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetCameraInit(tVideoWidget *pWidget, unsigned long ulBufAddr)
{
    //
    // Initialize the camera driver itself.
    //
    CameraInit(((pWidget->ulStyle & VW_STYLE_VGA) ?
                 CAMERA_SIZE_VGA : CAMERA_SIZE_QVGA) | CAMERA_FORMAT_RGB565,
                 ulBufAddr, 0);

    pWidget->psVideoInst->ulCapAddr = ulBufAddr;
    pWidget->psVideoInst->sXStart = 0;
    pWidget->psVideoInst->sYStart = 0;
    pWidget->psVideoInst->sXOffset = 0;
    pWidget->psVideoInst->sYOffset = 0;
    if(pWidget->ulStyle & VW_STYLE_VGA)
    {
        pWidget->psVideoInst->usStride = VIDEO_BUFF_STRIDE;
        pWidget->psVideoInst->usCapWidth = 640;
        pWidget->psVideoInst->usCapHeight = 480;
    }
    else
    {
        pWidget->psVideoInst->usStride = VIDEO_BUFF_STRIDE / 2;
        pWidget->psVideoInst->usCapWidth = 320;
        pWidget->psVideoInst->usCapHeight = 240;
    }

    //
    // Set the maximum displayable image size depending upon whether or not
    // downscaling is selected.
    //
    if(pWidget->ulStyle & VW_STYLE_DOWNSCALE)
    {
        pWidget->psVideoInst->usDispWidth =
                                    pWidget->psVideoInst->usCapWidth / 2;
        pWidget->psVideoInst->usDispHeight =
                                    pWidget->psVideoInst->usCapHeight / 2;
    }
    else
    {
        pWidget->psVideoInst->usDispWidth = pWidget->psVideoInst->usCapWidth;
        pWidget->psVideoInst->usDispHeight = pWidget->psVideoInst->usCapHeight;
    }

    //
    // Set the chromakey color.
    //
    CameraDisplayChromaKeySet(pWidget->ulKeyColor);

    //
    // Enable chromakey.
    //
    CameraDisplayChromaKeyEnable(true);

    //
    // Set the initial display buffer.
    //
    DisplayPositionSet(pWidget);

    //
    // Start capturing motion video.
    //
    CameraCaptureStart();
}

//*****************************************************************************
//
//! Sets the graphics plane color on top of which video pixels will be
//! displayed.
//!
//! \param pWidget is a pointer to the video widget.
//! \param ulColor is the 24 bit RGB color identifier representing the chromakey
//! color to be used.
//!
//! This function sets or changes the video chromakey color.  This color is
//! used to indicate where on the display video pixels are to be shown.  It is
//! used to fill the video widget allowing the video to ``shine through'' the
//! graphics from the layer underneath.  If your application statically
//! allocates the video widget and calls VideoWidgetCameraInit(), it is not
//! necessary to also call VideoWidgetKeyColorSet() since the chromakey is also
//! set during the widget initialization function.
//!
//! Note that video will be displayed in all areas of the screen where the
//! chromakey color is drawn in the graphics plane regardless of whether those
//! areas are within the bounds of the vidwidget object itself.  Care should,
//! therefore, be taken to ensure that the chromakey color chosen is not used
//! elsewhere in the user interface unless, of course, you intend video to show
//! at those positions too.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetKeyColorSet(tWidget *pWidget, unsigned long ulColor)
{
    tVideoWidget *pVideo;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Convert the generic widget pointer into a Video widget pointer.
    //
    pVideo = (tVideoWidget *)pWidget;

    //
    // Remember the key color we have been passed.
    //
    pVideo->ulKeyColor = ulColor;

    //
    // Set the supplied color as the chromakey in the camera driver.
    //
    CameraDisplayChromaKeySet(ulColor);

    //
    // Enable display chromakeying if currently disabled.
    //
    CameraDisplayChromaKeyEnable(true);
}

//*****************************************************************************
//
//! Enables or disables display of the video plane.
//!
//! \param pWidget is a pointer to the video widget.
//! \param bBlank is \b true if the video plane is to be enabled or \b false
//! if it is to be disabled.
//!
//! This function enables or disables display of the video plane.  When enabled,
//! the current contents of the video display buffer, either motion video or
//! the last captured frame if video capture is disabled, will be shown on the
//! screen above any pixels painted with the chromakey color.  When disabled,
//! the chromakey pixels will be shown on the screen rather than video.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetBlankSet(tWidget *pWidget, tBoolean bBlank)
{
    if(bBlank)
    {
        //
        // Tell the camera driver to stop displaying the video layer.
        //
        CameraDisplayStop(true);
    }
    else
    {
        //
        // Tell the camera driver to stop displaying the video layer.
        //
        CameraDisplayStart();
    }
}

//*****************************************************************************
//
//! Sets the video capture resolution.
//!
//! \param pWidget is a pointer to the video widget.
//! \param bVGA is \b true if VGA resolution (640x480) video capture is to be
//! set or \b false if QVGA (320x240) is to be used.
//!
//! This function allows the video capture dimensions to be set or changed.  An
//! application using a statically allocated widget which calls
//! VideoWidgetCameraInit() need not call this function unless it wishes to
//! change the capture resolution since the initialization function already
//! configures the capture resolution.
//!
//! If the resolution requested is already in use, this function will return
//! without doing anything.
//!
//! As a side effect of this function, the video display scroll position is
//! reset if a resolution change takes effect.
//!
//! An application using this function must be careful to ensure that the
//! video capture buffer configured via VideoWidgetCameraInit() or
//! VideoWidgetInit() is large enough to hold the video image size requested
//! here.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetResolutionSet(tWidget *pWidget, tBoolean bVGA)
{
    tVideoWidget *pVideo;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Convert the generic widget pointer into a Video widget pointer.
    //
    pVideo = (tVideoWidget *)pWidget;

    //
    // Do we need to actually change anything?
    //
    if(((pVideo->ulStyle & VW_STYLE_VGA) && bVGA) ||
        (!(pVideo->ulStyle & VW_STYLE_VGA) && !bVGA))
    {
        //
        // No - we are not changing resolution so just return.
        //
        return;
    }

    //
    // Set the camera to capture the resolution of video requested.
    //
    CameraCaptureTypeSet((bVGA ? CAMERA_SIZE_VGA : CAMERA_SIZE_QVGA) |
                         CAMERA_FORMAT_RGB565);

    //
    // Reset our scroll position and set the new stride and frame dimensions.
    //
    pVideo->psVideoInst->sXStart = 0;
    pVideo->psVideoInst->sYStart = 0;
    pVideo->psVideoInst->sXOffset = 0;
    pVideo->psVideoInst->sYOffset = 0;
    if(bVGA)
    {
        pVideo->psVideoInst->usStride = VIDEO_BUFF_STRIDE;
        pVideo->psVideoInst->usCapWidth = 640;
        pVideo->psVideoInst->usCapHeight = 480;
        pVideo->ulStyle |= VW_STYLE_VGA;
    }
    else
    {
        pVideo->psVideoInst->usStride = VIDEO_BUFF_STRIDE / 2;
        pVideo->psVideoInst->usCapWidth = 320;
        pVideo->psVideoInst->usCapHeight = 240;
        pVideo->ulStyle &= ~VW_STYLE_VGA;
    }

    //
    // Reset the capture and display buffer strides to match the new resolution.
    //
    CameraCaptureBufferSet(pVideo->psVideoInst->ulCapAddr,
                           pVideo->psVideoInst->usStride, true);
    CameraDisplayBufferSet(pVideo->psVideoInst->ulCapAddr,
                           pVideo->psVideoInst->usStride, true);

    //
    // Set the maximum displayable image size depending upon whether or not
    // downscaling is selected.
    //
    if(pVideo->ulStyle & VW_STYLE_DOWNSCALE)
    {
        pVideo->psVideoInst->usDispWidth =
            pVideo->psVideoInst->usCapWidth / 2;
        pVideo->psVideoInst->usDispHeight =
            pVideo->psVideoInst->usCapHeight / 2;
    }
    else
    {
        pVideo->psVideoInst->usDispWidth = pVideo->psVideoInst->usCapWidth;
        pVideo->psVideoInst->usDispHeight = pVideo->psVideoInst->usCapHeight;
    }

    //
    // Reposition the video to make sure we're not showing garbage.
    //
    DisplayPositionSet(pVideo);
}

//*****************************************************************************
//
//! Freezes or unfreezes motion video.
//!
//! \param pWidget is a pointer to the video widget.
//! \param bFreeze is \b true if video capture is to be frozen or \b false
//! if it is to be started.
//!
//! This function freezes or unfreezes motion video capture.  When capture is
//! frozen, a static image will be displayed in the video plane assuming that
//! video display has not been disabled using a call to VideoWidgetBlankSet().
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetFreezeSet(tWidget *pWidget, tBoolean bFreeze)
{
    //
    // Are we stopping or starting the video?
    //
    if(bFreeze)
    {
        //
        // Tell the camera driver to stop capturing motion video.
        //
        CameraCaptureStop(true);
    }
    else
    {
        //
        // Tell the camera driver to start capturing motion video.
        //
        CameraCaptureStart();
    }
}

//*****************************************************************************
//
//! Enables or disabled video display downscaling.
//!
//! \param pWidget is a pointer to the video widget.
//! \param bDownscale is \b true to downscale the video image by 50% in each
//! dimension or \b false to show video without scaling.
//!
//! This function allows an application to downscale the motion or still video
//! image by 50% in each dimension.  This allows VGA resolution video to be
//! captured and the full image to be displayed on the QVGA resolution LCD
//! display.  If QVGA video is being captured, downscaling will enable it to be
//! shown in the top left quadrant of the display.
//!
//! If VGA video is captured and downscaling is disabled, the user may select
//! the section of the video image which is displayed by dragging a stylus or
//! finger over the video widget to scroll the video.
//!
//! Video scaling is a display operation and does not affect the image stored
//! in the capture buffer.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetDownscaleSet(tWidget *pWidget, tBoolean bDownscale)
{
    tVideoWidget *psVidWidget;

    //
    // Get a pointer to our real widget.
    //
    psVidWidget = (tVideoWidget *)pWidget;

    //
    // Pass this through to the camera driver.
    //
    CameraDisplayDownscaleSet(bDownscale);

    //
    // Adjust the width and height we store to take into account whether we
    // are downscaling or not.  These fields represent the maximum image size
    // that could be displayed.
    //
    if(bDownscale)
    {
        psVidWidget->ulStyle |= VW_STYLE_DOWNSCALE;
        psVidWidget->psVideoInst->usDispWidth =
                                    psVidWidget->psVideoInst->usCapWidth / 2;
        psVidWidget->psVideoInst->usDispHeight =
                                    psVidWidget->psVideoInst->usCapHeight / 2;
    }
    else
    {
        psVidWidget->ulStyle &= ~VW_STYLE_DOWNSCALE;
        psVidWidget->psVideoInst->usDispWidth =
                                    psVidWidget->psVideoInst->usCapWidth;
        psVidWidget->psVideoInst->usDispHeight =
                                    psVidWidget->psVideoInst->usCapHeight;
    }

    //
    // Reset the display offsets.
    //
    psVidWidget->psVideoInst->sXOffset = 0;
    psVidWidget->psVideoInst->sYOffset = 0;

    //
    // Update the video on the screen.
    //
    DisplayPositionSet(psVidWidget);
}

//*****************************************************************************
//
//! Flips the video image about its horizontal axis.
//!
//! \param pWidget is a pointer to the video widget.
//! \param bFlip is \b true to flip the video image around its horizontal
//! center or \b false to leave the video unaffected.
//!
//! This function allows the captured video image to be flipped around its
//! horizontal axis.  When flipped, the bottom of the image appears at the
//! top of the display.  By using this function in conjunction with
//! VideoWidgetCameraMirrorSet(), an application can rotate the video 180
//! degrees.
//!
//! This is a camera setting and affects the image stored in the capture buffer.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetCameraFlipSet(tWidget *pWidget, tBoolean bFlip)
{
    //
    // Have the camera driver set the flip state.
    //
    CameraFlipSet(bFlip);
}

//*****************************************************************************
//
//! Mirrors the video image about its vertical axis.
//!
//! \param pWidget is a pointer to the video widget.
//! \param bMirror is \b true to mirror the video image around its vertical
//! axis or \b false to leave the video unaffected.
//!
//! This function allows the captured video image to be mirrored around its
//! vertical axis.  When mirrored, the left of the image appears at the
//! right of the display.  By using this function in conjunction with
//! VideoWidgetCameraFlipSet(), an application can rotate the video 180
//! degrees.
//!
//! This operation is also useful in video-conferencing applications where a
//! user is looking at their own image.  In these cases, people often find that
//! it feels more natural to view a mirror image of themselves.
//!
//! This is a camera setting and affects the image stored in the capture buffer.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetCameraMirrorSet(tWidget *pWidget, tBoolean bMirror)
{
    //
    // Have the camera driver set the mirror state.
    //
    CameraMirrorSet(bMirror);
}

//*****************************************************************************
//
//! Sets the brightness of the video image.
//!
//! \param pWidget is a pointer to the video widget.
//! \param ucBrightness is an unsigned value representing the required
//! brightness for the video image. Values in the range [0, 255] are scaled to
//! represent exposure values between -4EV and +4EV with 128 representing
//! ``normal'' brightness or 0EV adjustment.
//!
//! This function sets the brightness (exposure) of the captured video image.
//! Values of \e ucBrightness above 128 cause the image to be brighter while
//! values below this darken the image.
//!
//! This is a camera setting and affects the image stored in the capture buffer.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetBrightnessSet(tWidget *pWidget, unsigned char ucBrightness)
{
    //
    // Have the camera driver set the appropriate brightness.
    //
    CameraBrightnessSet(ucBrightness);
}

//*****************************************************************************
//
//! Sets the color saturation of the video image.
//!
//! \param pWidget is a pointer to the video widget.
//! \param ucSaturation is a value representing the required color saturation
//! for the video image.  Normal saturation is represented by value 128
//! with values above this increasing saturation and values below decreasing it.
//!
//! This function sets the color saturation of the captured video image. Higher
//! values result in more vivid images and lower values produce a muted or even
//! monochrome effect.
//!
//! This is a camera setting and affects the image stored in the capture buffer.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetSaturationSet(tWidget *pWidget, unsigned char ucSaturation)
{
    //
    // Have the camera driver set the appropriate saturation level.
    //
    CameraSaturationSet(ucSaturation);
}

//*****************************************************************************
//
//! Sets the contrast of the video image.
//!
//! \param pWidget is a pointer to the video widget.
//! \param ucContrast is a value representing the required contrast for the
//! video image.  Normal contrast is represented by value 128 with values above
//! this increasing contrast and values below decreasing it.
//!
//! This function sets the contrast of the captured video image. Higher
//! values result in higher contrast images and lower values result in lower
//! contrast images.
//!
//! This is a camera setting and affects the image stored in the capture buffer.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetContrastSet(tWidget *pWidget, unsigned char ucContrast)
{
    //
    // Have the camera driver set the appropriate contrast.
    //
    CameraContrastSet(ucContrast);
}

//*****************************************************************************
//
//! Reads pixel data from a given position in the video image.
//!
//! \param pWidget is a pointer to the video widget.
//! \param usX is the X coordinate of the start of the data to be read.  Valid
//! values are [0,639] of VGA capture is configured or [0,319] if QVGA is
//! being captured.
//! \param usY is the Y coordinate of the start of the data to be read.  Valid
//! values are [0,479] of VGA capture is configured or [0,239] if QVGA is
//! being captured.
//! \param ulPixels is the number of pixels to be read. Pixels are read
//! starting at \e usX, \e usY and progressing to the right then downwards.
//! \param pusBuffer points to the buffer into which the pixel data should be
//! copied.
//! \param b24bpp indicates whether to return raw (RGB565) data or extracted
//! colors in RGB24 format.
//!
//! This function allows an application to read back a section of a captured
//! image into a buffer in Stellaris internal SRAM.  The data may be returned
//! either in the native 16bpp format supported by the camera and LCD or in
//! 24bpp RGB format.  The 24bpp format returned places the blue component in
//! the lowest memory location followed by the green component then the red
//! component.
//!
//! It is recommended that data only be read from the capture buffer when
//! video capture is frozen.  This ensures that the data returned will comprise
//! a single video image.  Reading while capture is enabled may result in
//! pixels being returned from different images since the capture buffer content
//! may change during the time the reading is going on.
//!
//! \return None.
//
//*****************************************************************************
void
VideoWidgetImageDataGet(tWidget *pWidget, unsigned short usX,
                        unsigned short usY, unsigned long ulPixels,
                        unsigned short *pusBuffer, tBoolean b24bpp)
{
    CameraImageDataGet(true, usX, usY, ulPixels, b24bpp, pusBuffer);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
