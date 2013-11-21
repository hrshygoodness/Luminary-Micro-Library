//*****************************************************************************
//
// jpgwidget.c - JPEG image display and button widgets.
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

#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "third_party/jpeg/jinclude.h"
#include "third_party/jpeg/jpeglib.h"
#include "third_party/jpeg/jerror.h"
#include "third_party/jpeg/jramdatasrc.h"
#include "extram.h"
#include "jpgwidget.h"

//*****************************************************************************
//
//! \addtogroup jpgwidget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Decompress the JPEG image whose compressed data is linked to the supplied
// widget.
//
//*****************************************************************************
static long
JPEGDecompressImage(tJPEGWidget *pJPEG)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    boolean bRetcode;
    JDIMENSION iNumLines;
    JSAMPROW pScanRows[4];
    unsigned char *pSrcPixel;
    unsigned short *pusPixel;
    unsigned long ulImageSize, ulScanBufferHeight, ulColor;
    int iPixel, iTotalPixels;

    //
    // Initialize the deccompression object.
    //
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    //
    // Set the data source.
    //
    bRetcode = jpeg_ram_src(&cinfo, (JOCTET *)pJPEG->pucImage,
                            pJPEG->ulImageLen);
    if(!bRetcode)
    {
        return(1);
    }

    //
    // Read the image header.
    //
    jpeg_read_header(&cinfo, TRUE);

    //
    // Tell the JPEG decoder to use the fast integer DCT algorithm.  This is
    // required since the default is the slow integer version but we have
    // disabled this in the current version of third_party/jpeg/jmorecfg.h to
    // reduce the image size.
    //
    cinfo.dct_method = JDCT_IFAST;

    //
    // Calculate the output image dimensions so that we can allocate
    // appropriate buffers.
    //
    jpeg_calc_output_dimensions(&cinfo);

    //
    // Allocate a buffer large enough to hold the output image stored at 16
    // bits per pixel (the native color format for the display)
    //
    ulImageSize = (cinfo.output_width * cinfo.output_height * 2);
    pJPEG->psJPEGInst->pusImage = (unsigned short *)ExtRAMAlloc(ulImageSize);
    pusPixel = pJPEG->psJPEGInst->pusImage;

    if(!pJPEG->psJPEGInst->pusImage)
    {
        return(2);
    }

    //
    // Allocate a buffer that can hold cinfo.rec_outbuf_height lines of pixels
    // output from the decompressor.  These pixels are described as multiple
    // components (typically 3) so we need to take this into account.
    //
    ulScanBufferHeight = ((cinfo.rec_outbuf_height < 4) ?
                           cinfo.rec_outbuf_height : 4);
    pScanRows[0] = (JSAMPROW)ExtRAMAlloc(cinfo.output_width *
                                         ulScanBufferHeight *
                                         cinfo.output_components);
    if(!pScanRows[0])
    {
        ExtRAMFree(pJPEG->psJPEGInst->pusImage);
        pJPEG->psJPEGInst->pusImage = 0;
        return(3);
    }

    //
    // Remember the size of the image we are decompressing.
    //
    pJPEG->psJPEGInst->usHeight = cinfo.output_height;
    pJPEG->psJPEGInst->usWidth = cinfo.output_width;
    pJPEG->psJPEGInst->sXOffset = 0;
    pJPEG->psJPEGInst->sYOffset = 0;

    //
    // If we allocated more than 1 line, we need to fill in the row pointers
    // for each of the other rows in the scanline buffer.
    //
    for(iNumLines = 1; iNumLines < (int)ulScanBufferHeight; iNumLines++)
    {
        pScanRows[iNumLines] = pScanRows[iNumLines - 1] +
                               (cinfo.output_width * cinfo.output_components);
    }

    //
    //
    // Start decompression.
    //
    jpeg_start_decompress(&cinfo);

    //
    // Decompress the image a piece at a time.
    //
    while (cinfo.output_scanline < cinfo.output_height)
    {
      //
      // Request some decompressed pixels.
      //
      iNumLines = jpeg_read_scanlines(&cinfo, pScanRows, ulScanBufferHeight);

      //
      // How many pixels do we need to process?
      //
      iTotalPixels = iNumLines * cinfo.output_width;
      pSrcPixel = (unsigned char *)pScanRows[0];

      for(iPixel = iTotalPixels; iPixel; iPixel--)
      {
          //
          // Read the RGB pixel from the scanline.
          //
          ulColor = *pSrcPixel++;
          ulColor <<= 8;
          ulColor |= *pSrcPixel++;
          ulColor <<= 8;
          ulColor |= *pSrcPixel++;

          //
          // Convert to 16 bit and store in the output image buffer.
          //
          *pusPixel = pJPEG->sBase.pDisplay->pfnColorTranslate(
                                 pJPEG->sBase.pDisplay->pvDisplayData,
                                 ulColor);
          pusPixel++;
      }
    }

    //
    // Destroy the decompression object.
    //
    jpeg_finish_decompress(&cinfo);

    //
    // Free the scanline buffer.
    //
    ExtRAMFree(pScanRows[0]);

    //
    // Tell the caller everything went well.
    //
    return(0);
}

//*****************************************************************************
//
// Draws a JPEG widget.
//
// \param pWidget is a pointer to the JPEG widget to be drawn.
//
// This function draws a JPEG widget on the display.  This is called in
// response to a \b WIDGET_MSG_PAINT message.
//
// \return None.
//
//*****************************************************************************
static void
JPEGWidgetPaint(tWidget *pWidget)
{
    tRectangle sRect;
    tJPEGWidget *pJPEG;
    tContext sCtx;
    unsigned short usWidth, usHeight, usRow;
    unsigned short *pusRow;
    short sSrcX, sSrcY, sDstX, sDstY;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Convert the generic widget pointer into a JPEG widget pointer.
    //
    pJPEG = (tJPEGWidget *)pWidget;

    //
    // Initialize a drawing context.
    //
    GrContextInit(&sCtx, pWidget->pDisplay);

    //
    // Initialize the clipping region based on the extents of this rectangular
    // JPEG widget.
    //
    GrContextClipRegionSet(&sCtx, &(pWidget->sPosition));

    //
    // Take a copy of the current widget position.
    //
    sRect=pWidget->sPosition;

    //
    // See if the JPEG widget outline style is selected.
    //
    if(pJPEG->ulStyle & JW_STYLE_OUTLINE)
    {
        unsigned long ulLoop;

        GrContextForegroundSet(&sCtx, pJPEG->ulOutlineColor);

        //
        // Outline the JPEG widget with the outline color.
        //
        for(ulLoop = 0; ulLoop < (unsigned long)pJPEG->ucBorderWidth; ulLoop++)
        {
            GrRectDraw(&sCtx, &sRect);
            sRect.sXMin++;
            sRect.sYMin++;
            sRect.sXMax--;
            sRect.sYMax--;
        }
    }

    //
    // If the fill style is selected fill the widget with the appropriate color.
    //
    if(pJPEG->ulStyle & JW_STYLE_FILL)
    {
        //
        // Fill the JPEG widget with the fill color.
        //
        GrContextForegroundSet(&sCtx, pJPEG->ulFillColor);
        GrRectFill(&sCtx, &sRect);
    }

    //
    // Does the widget had a decompressed image to draw?
    //
    if(pJPEG->psJPEGInst->pusImage)
    {
        //
        // What is the size of the image area of the widget?
        //
        usWidth = (sRect.sXMax - sRect.sXMin) + 1;
        usHeight = (sRect.sYMax - sRect.sYMin) + 1;

        //
        // Is the display window wider than the image?
        //
        if(usWidth > pJPEG->psJPEGInst->usWidth)
        {
            //
            // The display window is wider so we need to center the image
            // within the window.
            //
            sDstX = sRect.sXMin +
                    (short)(usWidth - pJPEG->psJPEGInst->usWidth) / 2;
            sSrcX = 0;
            usWidth = pJPEG->psJPEGInst->usWidth;
        }
        else
        {
            //
            // The image is wider so we will fill the window (in the x
            // direction) and clip the image accordingly.
            //
            sDstX = sRect.sXMin;
            sSrcX = (pJPEG->psJPEGInst->usWidth - usWidth) / 2 -
                    pJPEG->psJPEGInst->sXOffset;

            //
            // Ensure we don't wrap the image due to an offset that is too
            // large.
            //
            sSrcX = (sSrcX < 0) ? 0 : sSrcX;
        }

        //
        // Is the image highter than the display window?
        //
        if(usHeight > pJPEG->psJPEGInst->usHeight)
        {
            //
            // The display window is higher so we need to center the image
            // within the window.
            //
            sDstY = sRect.sYMin +
                    (short)(usHeight - pJPEG->psJPEGInst->usHeight) / 2;
            sSrcY = 0;
            usHeight = pJPEG->psJPEGInst->usHeight;

        }
        else
        {
            //
            // The image is higher so we will fill the window (in the y
            // direction) and clip the image accordingly.
            //
            sDstY = sRect.sYMin;
            sSrcY = (pJPEG->psJPEGInst->usHeight - usHeight) / 2 -
                    pJPEG->psJPEGInst->sYOffset;

            //
            // Ensure we don't wrap the image due to an offset that is too
            // large.
            //
            sSrcY = (sSrcY < 0) ? 0 : sSrcY;
        }

        //
        // Start at the top left of the visible portion of the image (based on the
        // slider settings).
        //
        pusRow = pJPEG->psJPEGInst->pusImage + sSrcX +
                 (sSrcY * pJPEG->psJPEGInst->usWidth);

        //
        // Draw the rows of image data using direct calls to the display driver.
        //
        for(usRow = 0; usRow < usHeight; usRow++)
        {
            sCtx.pDisplay->pfnPixelDrawMultiple(
                                           sCtx.pDisplay->pvDisplayData,
                                           sDstX, sDstY + usRow, 0, usWidth, 16,
                                           (unsigned char *)pusRow, 0);
            pusRow += pJPEG->psJPEGInst->usWidth;
        }
    }

    //
    // See if the JPEG widget text style is selected.
    //
    if(pJPEG->ulStyle & JW_STYLE_TEXT)
    {
        //
        // Compute the center of the JPEG widget.
        //
        sDstX = (sRect.sXMin + ((sRect.sXMax - sRect.sXMin + 1) / 2));
        sDstY = (sRect.sYMin + ((sRect.sYMax - sRect.sYMin + 1) / 2));

        //
        // Draw the text centered in the middle of the JPEG widget.
        //
        GrContextFontSet(&sCtx, pJPEG->pFont);
        GrContextForegroundSet(&sCtx, pJPEG->ulTextColor);
        GrStringDrawCentered(&sCtx, pJPEG->pcText, -1, sDstX, sDstY, 0);
    }
}

//*****************************************************************************
//
// Handles changes required as a result of pointer movement.
//
// \param pJPEG is a pointer to the JPEG widget.
// \param lX is the X coordinate of the pointer event.
// \param lY is the Y coordinate of the pointer event.
//
// This function is called whenever a new pointer message is received which
// may indicate a change in position that requires us to scroll the JPEG image.
// If necessary, the OnScroll callback is made indicating the current offset
// of the image.  This function does not cause any repainting of the widget.
//
// \return Returns \b true if one or other of the image offsets changed as a
// result of the call or \b false if no changes were made.
//
//*****************************************************************************
static tBoolean
JPEGWidgetHandlePtrPos(tJPEGWidget *pJPEG, long lX, long lY)
{
    short sDeltaX, sDeltaY, sXOffset, sYOffset, sMaxX, sMaxY;
    tBoolean bRetcode;

    //
    // Assume nothing changes until we know otherwise.
    //
    bRetcode = false;

    //
    // Do we have an image?
    //
    if(!pJPEG->psJPEGInst->pusImage)
    {
        //
        // No image so just return immediately since there's nothing we can
        // scroll.
        //
        return(false);
    }

    //
    // Did the pointer position change?
    //
    if((pJPEG->psJPEGInst->sXStart == (short)lX) &&
       (pJPEG->psJPEGInst->sYStart == (short)lY))
    {
        //
        // The pointer position didn't change so we have nothing to do.
        //
        return(false);
    }

    //
    // Determine the new offset and apply it to the current total offset.
    //
    sDeltaX = (short)lX - pJPEG->psJPEGInst->sXStart;
    sDeltaY = (short)lY - pJPEG->psJPEGInst->sYStart;

    sXOffset = pJPEG->psJPEGInst->sXOffset + sDeltaX;
    sYOffset = pJPEG->psJPEGInst->sYOffset + sDeltaY;

    //
    // Now check to make sure that neither offset causes the image to move
    // out of the display window and limit them as required.
    //
    sDeltaX = (pJPEG->sBase.sPosition.sXMax - pJPEG->sBase.sPosition.sXMin) + 1;
    sDeltaY = (pJPEG->sBase.sPosition.sYMax - pJPEG->sBase.sPosition.sYMin) + 1;
    sMaxX = abs((pJPEG->psJPEGInst->usWidth - sDeltaX) / 2);
    sMaxY = abs((pJPEG->psJPEGInst->usHeight - sDeltaY) / 2);

    //
    // If the window is wider than the image, we don't allow any scrolling.
    //
    if(sMaxX < 0)
    {
        sXOffset = 0;
    }
    else
    {
        //
        // Make sure that the total offset is not too large for the image we
        // are displaying.
        //
        if(abs(sXOffset) > sMaxX)
        {
            sXOffset = (sXOffset < 0) ? -sMaxX : sMaxX;
        }
    }

    //
    // If the window is higher than the image, we don't allow any scrolling.
    //
    if(sMaxY < 0)
    {
        sYOffset = 0;
    }
    else
    {
        //
        // Make sure that the total offset is not too large for the image we
        // are displaying.
        //
        if(abs(sYOffset) > sMaxY)
        {
            sYOffset = (sYOffset < 0) ? -sMaxY : sMaxY;
        }
    }

    //
    // Now we've calculated the new image offset.  Is it different from the
    // previous offset?
    //
    if((sXOffset != pJPEG->psJPEGInst->sXOffset) ||
       (sYOffset != pJPEG->psJPEGInst->sYOffset))
    {
        //
        // Yes - something changed.
        //
        bRetcode = true;
        pJPEG->psJPEGInst->sXOffset = sXOffset;
        pJPEG->psJPEGInst->sYOffset = sYOffset;

        //
        // Do we need to make a callback?
        //
        if(pJPEG->pfnOnScroll)
        {
            pJPEG->pfnOnScroll((tWidget *)pJPEG, sXOffset, sYOffset);
        }
    }

    //
    // Remember where the pointer was the last time we looked at it.
    //
    pJPEG->psJPEGInst->sXStart = (short)lX;
    pJPEG->psJPEGInst->sYStart = (short)lY;

    //
    // Tell the caller whether anything changed or not.
    //
    return(bRetcode);
}

//*****************************************************************************
//
//! Handles pointer events for a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget.
//! \param ulMsg is the pointer event message.
//! \param lX is the X coordinate of the pointer event.
//! \param lY is the Y coordinate of the pointer event.
//!
//! This function processes pointer event messages for a JPEG widget.
//! This is called in response to a \b WIDGET_MSG_PTR_DOWN,
//! \b WIDGET_MSG_PTR_MOVE, and \b WIDGET_MSG_PTR_UP messages.
//!
//! If the widget has the \b #JW_STYLE_LOCKED flag set, the input is ignored
//! and this function returns immediately.
//!
//! If the widget is a button type (having style flag \b #JW_STYLE_BUTTON set),
//! and the mouse message is within the extent of the widget, the OnClick
//! callback function will be called on \b WIDGET_MSG_PTR_DOWN if style flag
//! \b #JW_STYLE_RELEASE_NOTIFY is not set or on \b WIDGET_MSG_PTR_UP if
//! \b #JW_STYLE_RELEASE_NOTIFY is set.
//!
//! If the widget is a canvas type (style flag \b #JW_STYLE_BUTTON not set),
//! pointer messages are used to control scrolling of the JPEG image within
//! the area of the widget.  In this case, any pointer movement that will cause
//! a change in the image position is reported to the client via the OnScroll
//! callback function.
//!
//! \return Returns 1 if the coordinates are within the extents of the widget
//! and 0 otherwise.
//
//*****************************************************************************
static long
JPEGWidgetClick(tWidget *pWidget, unsigned long ulMsg, long lX, long lY)
{
    tJPEGWidget *pJPEG;
    tBoolean bWithinWidget, bChanged;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Convert the generic widget pointer into a JPEG widget pointer.
    //
    pJPEG = (tJPEGWidget *)pWidget;

    //
    // Is the widget currently locked?
    //
    if(pJPEG->ulStyle & JW_STYLE_LOCKED)
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
    // Is this widget a button type?
    //
    if(pJPEG->ulStyle & JW_STYLE_BUTTON)
    {
        //
        // Yes - it's a button.  In this case, we only look for PTR_UP and
        // PTR_DOWN messages so that we can trigger the OnClick callback.
        //
        if(pJPEG->pfnOnClick)
        {
            //
            // Call the click callback if the screen has been pressed and we
            // are notifying on press or if the screen has been released and
            // we are notifying on release.
            //
            if(((ulMsg == WIDGET_MSG_PTR_UP) &&
               (pJPEG->ulStyle & JW_STYLE_RELEASE_NOTIFY)) ||
               ((ulMsg == WIDGET_MSG_PTR_DOWN) &&
               !(pJPEG->ulStyle & JW_STYLE_RELEASE_NOTIFY)))
            {
                pJPEG->pfnOnClick(pWidget);
            }
        }
    }
    else
    {
        //
        // This is a canvas style JPEG widget so we track mouse movement to
        // allow us to scroll the image based on touchscreen gestures.
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
                    pJPEG->ulStyle |= JW_STYLE_PRESSED;
                    pJPEG->psJPEGInst->sXStart = (short)lX;
                    pJPEG->psJPEGInst->sYStart = (short)lY;
                }

                break;
            }

            //
            // The touchscreen has been released.
            //
            case WIDGET_MSG_PTR_UP:
            {
                //
                // Remember that the touchscreen is no longer pressed.
                //
                pJPEG->ulStyle &= ~ JW_STYLE_PRESSED;

                //
                // Has the pointer moved since the last time we looked?  If so
                // handle it appropriately.
                //
                bChanged = JPEGWidgetHandlePtrPos(pJPEG, lX, lY);

                //
                // Repaint the widget.
                //
                WidgetPaint(pWidget);
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
                bChanged = JPEGWidgetHandlePtrPos(pJPEG, lX, lY);

                //
                // If something changed and we were asked to redraw
                // automatically on scrolling, do so here.
                //
                if(bChanged && (pJPEG->ulStyle & JW_STYLE_SCROLL))
                {
                    WidgetPaint(pWidget);
                }
                break;
            }

            default:
                break;
        }
    }

    //
    // Tell the widget manager whether this event occurred within the bounds
    // of this widget.
    //
    return((long)bWithinWidget);
}

//*****************************************************************************
//
//! Handles messages for a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget.
//! \param ulMsg is the message.
//! \param ulParam1 is the first parameter to the message.
//! \param ulParam2 is the second parameter to the message.
//!
//! This function receives messages intended for this JPEG widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
long
JPEGWidgetMsgProc(tWidget *pWidget, unsigned long ulMsg,
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
            JPEGWidgetPaint(pWidget);

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
            return(JPEGWidgetClick(pWidget, ulMsg, ulParam1, ulParam2));
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
//! Initializes a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget to initialize.
//! \param pDisplay is a pointer to the display on which to draw the push
//! button.
//! \param lX is the X coordinate of the upper left corner of the JPEG widget.
//! \param lY is the Y coordinate of the upper left corner of the JPEG widget.
//! \param lWidth is the width of the JPEG widget.
//! \param lHeight is the height of the JPEG widget.
//!
//! This function initializes the provided JPEG widget.  The widget position
//! is set and all styles and parameters set to 0.  The caller must make use
//! of the various widget functions to set any required parameters after
//! making this call.
//!
//! \return None.
//
//*****************************************************************************
void
JPEGWidgetInit(tJPEGWidget *pWidget, const tDisplay *pDisplay,
               long lX, long lY, long lWidth, long lHeight)
{
    unsigned long ulIdx;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);
    ASSERT(pDisplay);

    //
    // Clear out the widget structure.
    //
    for(ulIdx = 0; ulIdx < sizeof(tJPEGWidget); ulIdx += 4)
    {
        ((unsigned long *)pWidget)[ulIdx / 4] = 0;
    }

    //
    // Set the size of the JPEG widget structure.
    //
    pWidget->sBase.lSize = sizeof(tJPEGWidget);

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
    // Set the extents of this rectangular JPEG widget.
    //
    pWidget->sBase.sPosition.sXMin = lX;
    pWidget->sBase.sPosition.sYMin = lY;
    pWidget->sBase.sPosition.sXMax = lX + lWidth - 1;
    pWidget->sBase.sPosition.sYMax = lY + lHeight - 1;

    //
    // Use the JPEG widget message handler to process messages to
    // this JPEG widget.
    //
    pWidget->sBase.pfnMsgProc = JPEGWidgetMsgProc;
}

//*****************************************************************************
//
//! Pass a new compressed image to the widget.
//!
//! \param pWidget is a pointer to the JPEG widget whose image is to be
//! set.
//! \param pImg is a pointer to the compressed JPEG data for the image.
//! \param ulImgLen is the number of bytes of data pointed to by pImg.
//!
//! This function is used to change the image displayed by a JPEG widget.  It is
//! safe to call it when the widget is already displaying an image since it
//! will free any existing image before decompressing the new one.  The client
//! is responsible for repainting the widget after this call is made.
//!
//! \return Returns 0 on success. Any other return code indicates failure.
//
//*****************************************************************************
long
JPEGWidgetImageSet(tWidget *pWidget, const unsigned char *pImg,
                   unsigned long ulImgLen)
{
    tJPEGWidget *pJPEG;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);
    ASSERT(pImg);
    ASSERT(ulImgLen);

    //
    // Convert the generic widget pointer into a JPEG widget pointer.
    //
    pJPEG = (tJPEGWidget *)pWidget;

    //
    // Discard any existing image.
    //
    JPEGWidgetImageDiscard(pWidget);

    //
    // Store the new image information.
    //
    pJPEG->ulImageLen = ulImgLen;
    pJPEG->pucImage = pImg;

    //
    // Decompress the new image.
    //
    return(JPEGDecompressImage(pJPEG));
}

//*****************************************************************************
//
//! Decompresses the image associated with a JPEG widget.
//!
//! \param pWidget is a pointer to the JPEG widget whose image is to be
//! decompressed.
//!
//! This function must be called by the client for any JPEG widget whose
//! compressed data pointer is initialized using the JPEGCanvas, JPEGButton
//! or JPEGWidgetStruct macros.  It decompresses the image and readies it
//! for display.
//!
//! This function must NOT be used if the widget already holds a decompressed
//! image (i.e. if this function has been called before or if a prior call
//! has been made to JPEGWidgetImageSet() without a later call to
//! JPEGWidgetImageDiscard()) since this will result in a serious memory leak.
//!
//! The client is responsible for repainting the widget after this call is
//! made.
//!
//! \return Returns 0 on success. Any other return code indicates failure.
//
//*****************************************************************************
long
JPEGWidgetImageDecompress(tWidget *pWidget)
{
    tJPEGWidget *pJPEG;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Convert the generic widget pointer into a JPEG widget pointer.
    //
    pJPEG = (tJPEGWidget *)pWidget;

    //
    // Decompress the image.
    //
    return(JPEGDecompressImage(pJPEG));
}

//*****************************************************************************
//
//! Frees any decompressed image held by the widget.
//!
//! \param pWidget is a pointer to the JPEG widget whose image is to be
//! discarded.
//!
//! This function frees any decompressed image that is currently held by the
//! widget and returns the memory it was occupying to the RAM heap.  After
//! this call, JPEGWidgetImageDecompress() may be called to re-decompress the
//! same image or JPEGWidgetImageSet() can be called to have the widget
//! decompress a new image.
//!
//! \return None.
//
//*****************************************************************************
void
JPEGWidgetImageDiscard(tWidget *pWidget)
{
    tJPEGWidget *pJPEG;

    //
    // Check the arguments.
    //
    ASSERT(pWidget);

    //
    // Convert the generic widget pointer into a JPEG widget pointer.
    //
    pJPEG = (tJPEGWidget *)pWidget;

    //
    // Does this widget currently have a decompressed image?
    //
    if(pJPEG->psJPEGInst->pusImage)
    {
        //
        // Yes - free it up and clear all the image sizes back to zero.
        //
        ExtRAMFree(pJPEG->psJPEGInst->pusImage);
        pJPEG->psJPEGInst->pusImage = 0;
        pJPEG->psJPEGInst->usHeight = 0;
        pJPEG->psJPEGInst->usWidth = 0;
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
