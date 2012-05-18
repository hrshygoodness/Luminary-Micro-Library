This program converts a font in a format understood by FreeType into a C
structure definition for a tFont that can be used by the GrStringDraw()
function.  FreeType can understand TrueType, OpenType, Type 1, and many other
font formats.  See http://www.freetype.org for details on the features and
capabilities of FreeType.

The command line arguments to the program are as follows:

  -b             This indicates that the font is a bold face font.  This only
                 affects the name of the structure and the name of the file
                 that is produced.

  -f <filename>  This specifies the file name fragment used to construct the
                 name of the output file name.  Typically, this will be an
                 indicator of the font style, and is placed into a comment
                 within the output file as the style of the font.

  -i             This indicates that the font is an italic face font.  This
                 only affects the name of the structure and the name of the
                 file that is produced.

  -s <size>      This specifies the size (height) of the font to be rendered in
                 points (which is equivalent to pixels).  Due to the imprecise
                 nature of rendering a font down to black and white (without
                 any aliasing), it is possible that the resulting font data
                 will be slightly larger or smaller that this size.

  -p <num>       This specifies the index of the first character in the font
                 that is to be encoded.  If the value is not provided, it
                 defaults to 32 which is typically the space character.

  -e <num>       This specifies the index of the last character in the font
                 that is to be encoded.  If the value is not provided, it
                 defaults to 126 which, in ISO8859-1 is tilde (~).

  -w <num>       Encodes the specified character index as a space regardless of
                 the character which may be present in the font at that
                 location.  This is helpful in allowing a space to be included
                 in a font which only encodes a subset of the characters
                 which would not normally include the space character (for
                 example, numeric digits only).  If absent, this value defaults
                 to 32, ensuring that character 32 is always the space.

  -n             This switch overrides -w and causes no character to be encoded
                 as a space unless the source font already contains a space.

So, for example, the following would render a Computer Modern small-caps font
at 24 point:

    ftrasterize -f cmsc -s 24 cmcsc10.pfb

The output will be written to fontcmsc24.c and contain a definition for
g_sFontCmsc24.

The following would render a Computer Modern small-caps font at 44 points and
generate an output font containing only characters 48 through 58 (the numeric
digits). Additionally, character 47 in the encoded font (which is the first
character and will be the character displayed if an attempt is made to render
a character which is not included in the font) is forced to be a space:

    ftrasterize -f cmscdigits -s 44 -w 47 -p 47 -e 58 cmcsc10.pfb

The output will be written to fontcmscdigits44.c and contain a definition for
g_sFontCmscdigits44.

-------------------------------------------------------------------------------

Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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

This is part of revision 8555 of the Stellaris Firmware Development Package.
