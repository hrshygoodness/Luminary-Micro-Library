This program converts an image in NetPBM format into a C structure definition
for a tImage that can be passed to the GrImageDraw() function.  Only the binary
PPM format (indicated by format code "P6") is supported by the program.

The NetPBM (http://netpbm.sourceforge.net) or ImageMagick
(http://www.imagemagick.org) packages may be used to convert image from other
formats (such as PNG, JPEG, or BMP) to a PPM.  They may also be used to scale
the image and reduce the number of colors as required by the application.

Using NetPBM as an example, given an image foo.jpg, the following steps are
used to convert it to something usable by the GrImageDraw() function:

1) "jpegtopnm foo.jpg > foo.pnm"

   This step converts the JPEG to a PNM, preparing it for further processing by
   the NetPBM tools.  Alternatively "bmptopnm", "pngtopnm", or "giftopnm" (or
   many others) can be used to convert other image formats to a PNM.  If
   starting from a PNM that is not in the required format, "pnmtopnm" will
   convert the file to the required PPM format.

2) "pamscale -width 64 -height 48 foo.pnm > foo_sm.pnm"

   Scale the image to 64 pixels wide by 48 pixels tall.  This is an optional
   step that is not needed if the image is already the required size.

3) "pnmcolormap 16 foo_sm.pnm > foo_cm.pnm"

   Produces a colormap of the 16 colors that can be used to best represent the
   color content in the image.  This colormap is used in the next step to
   reduce the amount of color content in the image.  If the image was
   constructed with the correct number of colors from the start, then this step
   (and the next) can be skipped.

   Changing the first argument (16 in this example) will change the number of
   colors in the colormap.  More colors provides better image color quality (in
   very colorful images) at the cost of more storage.  Note that 256 is the
   maximum number of colors that can be utilized by GrImageDraw().

4) "pnmremap -mapfile=foo_cm.pnm foo_sm.pnm > foo_img.pnm"

   Use the colormap produced in step 3 and set each pixel of the image to be
   the closest color in the colormap to the original pixel.  After this
   mapping, the image will only have the number of colors selected in the
   colormap.

5) "pnmtoc -c foo_img.pnm > foo.c"

   Convert the resulting PNM to a C array in foo.c, using compression.  The
   resulting file provides a structure definition for "g_sImage", which will
   likely require hand editing to be a more indicative name (and avoid a name
   clash if more than one image is used).

6) "ppmtobmp foo_img.pnm > foo_test.bmp"

   Convert the final PNM to a BMP for local examination.  Viewing the resulting
   image (after scaling and color reduction) on a PC will usually be quicker
   and easier, making it faster to try a variety of sizes and colormap sizes
   until an image of acceptable quality is produced.  This step is optional,
   though.

Any number of open source and commercial image manipulation program can perform
the equivalent operations (and even more sophisticated operations).

-------------------------------------------------------------------------------

Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
Software License Agreement

Texas Instruments (TI) is supplying this software for use solely and
exclusively on TI's microcontroller products. The software is owned by
TI and/or its suppliers, and is protected under applicable copyright
laws. You may not combine this software with "viral" open-source
software in order to form a larger program.

THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
DAMAGES, FOR ANY REASON WHATSOEVER.

This is part of revision 10636 of the Stellaris Firmware Development Package.
