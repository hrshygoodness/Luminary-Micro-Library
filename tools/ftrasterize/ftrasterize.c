//*****************************************************************************
//
// ftrasterize.c - Program to convert FreeType-compatible fonts to compressed
//                 bitmap fonts using FreeType.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include <ft2build.h>
#include FT_FREETYPE_H
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

typedef unsigned char tBoolean;
#include "../../grlib/grlib.h"

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

//*****************************************************************************
//
// The maximum number of font filenames that can be provided on the command
// line when working with Unicode fonts.  The first font provided is the main
// font and any following fonts are examined if the main font does not contain
// a required glyph.
//
//*****************************************************************************
#define MAX_FONTS 4

//*****************************************************************************
//
// Macros used to generate independent byte printf parameters from short and
// long values.
//
//*****************************************************************************
#define SHORT_PARAMS(x) ((x) & 0xFF), (((x) >> 8) & 0xFF)
#define LONG_PARAMS(x) ((x) & 0xFF), (((x) >> 8) & 0xFF),                      \
                       (((x) >> 16) & 0xFF), (((x) >> 24) & 0xFF)

//*****************************************************************************
//
// Parameters used in the conversion of the font.
//
//*****************************************************************************
typedef struct
{
    char *pcAppName;
    char *pcFilename;
    char *pcCharFile;
    char *pcFontInputName[MAX_FONTS];
    char *pcCopyrightFile;
    int iNumFonts;
    int iSize;
    int bFixedSize;
    int bBold;
    int bItalic;
    int bMono;
    int bVerbose;
    int bBinary;
    int bRemap;
    int bShow;
    int iFirst;
    int iLast;
    int iSpaceChar;
    int bNoForceSpace;
    int bUnicode;
    int iTranslateStart;
    int iTranslateSource;
    int iCharMap;
    int iOutputCodePage;
    int iFixedX;
    int iFixedY;
} tConversionParameters;

//*****************************************************************************
//
// Properties of the font that are calculated during conversion.
//
//*****************************************************************************
typedef struct
{
    int iWidth;
    int iYMin;
    int iYMax;
    unsigned short usCodePage;
    unsigned short usNumBlocks;
    unsigned long ulNumGlyphs;
} tFontInfo;

//*****************************************************************************
//
// A structure describing a block of characters to encode.
//
//*****************************************************************************
typedef struct _tCodepointBlock
{
    struct _tCodepointBlock *pNext;
    unsigned long ulStart;
    unsigned long ulEnd;
} tCodepointBlock;

//*****************************************************************************
//
// Descriptions of the various font character map encodings supported by
// FreeType.
//
//*****************************************************************************
typedef struct
{
    unsigned long ulValue;
    const char *pcName;
} tKeyString;

#define SELF_DESC(x) {(unsigned long)(x), #x}

const tKeyString g_pEncodingDescs[] =
{
SELF_DESC(FT_ENCODING_NONE),
SELF_DESC(FT_ENCODING_MS_SYMBOL),
SELF_DESC(FT_ENCODING_UNICODE),
SELF_DESC(FT_ENCODING_SJIS),
SELF_DESC(FT_ENCODING_GB2312),
SELF_DESC(FT_ENCODING_BIG5),
SELF_DESC(FT_ENCODING_WANSUNG),
SELF_DESC(FT_ENCODING_JOHAB),
SELF_DESC(FT_ENCODING_ADOBE_STANDARD),
SELF_DESC(FT_ENCODING_ADOBE_EXPERT),
SELF_DESC(FT_ENCODING_ADOBE_CUSTOM),
SELF_DESC(FT_ENCODING_ADOBE_LATIN_1),
SELF_DESC(FT_ENCODING_OLD_LATIN_2),
SELF_DESC(FT_ENCODING_APPLE_ROMAN) };

#define NUM_ENCODING_DESCS (sizeof(g_pEncodingDescs) / sizeof(tKeyString))

const tKeyString g_pPlatformDescs[] =
{
{ 0, "Apple Unicode" },
{ 1, "Apple Script Manager" },
{ 2, "ISO" },
{ 3, "Windows" } };

#define NUM_PLATFORM_DESCS (sizeof(g_pPlatformDescs) / sizeof(tKeyString))

//*****************************************************************************
//
// Descriptions of font and string codepages.
//
//*****************************************************************************
const tKeyString g_pCodepageDescs[] =
{
SELF_DESC(CODEPAGE_ISO8859_1),
SELF_DESC(CODEPAGE_ISO8859_2),
SELF_DESC(CODEPAGE_ISO8859_3),
SELF_DESC(CODEPAGE_ISO8859_5),
SELF_DESC(CODEPAGE_ISO8859_6),
SELF_DESC(CODEPAGE_ISO8859_7),
SELF_DESC(CODEPAGE_ISO8859_8),
SELF_DESC(CODEPAGE_ISO8859_9),
SELF_DESC(CODEPAGE_ISO8859_10),
SELF_DESC(CODEPAGE_ISO8859_11),
SELF_DESC(CODEPAGE_ISO8859_13),
SELF_DESC(CODEPAGE_ISO8859_14),
SELF_DESC(CODEPAGE_ISO8859_15),
SELF_DESC(CODEPAGE_ISO8859_16),
SELF_DESC(CODEPAGE_UNICODE),
SELF_DESC(CODEPAGE_GB2312),
SELF_DESC(CODEPAGE_GB18030),
SELF_DESC(CODEPAGE_BIG5),
SELF_DESC(CODEPAGE_SHIFT_JIS),
SELF_DESC(CODEPAGE_UTF_8),
SELF_DESC(CODEPAGE_UTF_16) };

#define NUM_CODEPAGE_DESCS (sizeof(g_pCodepageDescs) / sizeof(tKeyString))

//*****************************************************************************
//
// This array maps the character encodings defined in the source font to
// the appropriate codepage as required in the GrLib tFontWide header.
//
//*****************************************************************************
typedef struct
{
    unsigned long ulFTEncoding;
    unsigned short usCodepage;
} tCodePageMapping;

#define CODEPAGE_MAPPING(x, y) {(x), (y)}

const tCodePageMapping g_psCodePageMapping[] =
{
CODEPAGE_MAPPING(FT_ENCODING_UNICODE, CODEPAGE_UNICODE),
CODEPAGE_MAPPING(FT_ENCODING_ADOBE_LATIN_1, CODEPAGE_ISO8859_1),
CODEPAGE_MAPPING(FT_ENCODING_OLD_LATIN_2, CODEPAGE_ISO8859_2),
CODEPAGE_MAPPING(FT_ENCODING_SJIS, CODEPAGE_SHIFT_JIS),
CODEPAGE_MAPPING(FT_ENCODING_GB2312, CODEPAGE_GB2312),
CODEPAGE_MAPPING(FT_ENCODING_BIG5, CODEPAGE_BIG5) };

#define NUM_CODEPAGE_MAPPINGS (sizeof(g_psCodePageMapping) /                  \
                               sizeof(tCodePageMapping))

//*****************************************************************************
//
// The structure that describes each character glyph.
//
//*****************************************************************************
typedef struct
{
    //
    // The codepoint (character code) that this glyph represents.
    //
    unsigned long ulCodePoint;

    //
    // The width of the bitmap representation of this glyph.
    //
    int iWidth;

    //
    // The height of the bitmap representation of this glyph.
    //
    int iRows;

    //
    // The number of bytes in each row of the bitmap representation of this
    // glyph (this may be more than width * 8).
    //
    int iPitch;

    //
    // The number of rows between the baseline and the top of this glyph.
    //
    int iBitmapTop;

    //
    // The minimum X value in the glyph bitmap.
    //
    int iMinX;

    //
    // The maximum X value in the glyph bitmap.
    //
    int iMaxX;

    //
    // The bitmap representation of this glyph.
    //
    unsigned char *pucData;

    //
    // The compressed data describing this glyph.
    //
    unsigned char *pucChar;
} tGlyph;

//*****************************************************************************
//
// An array of glyphs for the first 256 ASCII characters (though normally we
// will only store 32 (space) through 126 (~)).
//
//*****************************************************************************
tGlyph g_pGlyphs[256];

//*****************************************************************************
//
// Command line options.
//
//*****************************************************************************
struct option g_pCmdLineOptions[] =
{
    { "charmap", required_argument, 0, 'a' },
    { "bold", no_argument, 0, 'b' },
    { "charfile", required_argument, 0, 'c' },
    { "display", no_argument, 0, 'd' },
    { "end", required_argument, 0, 'e' },
    { "font", no_argument, 0, 'f' },
    { "copyright", required_argument, 0, 'g' },
    { "help", no_argument, 0, 'h' },
    { "italic", no_argument, 0, 'i' },
    { "monospaced", no_argument, 0, 'm' },
    { "no-force-whitespace", no_argument, 0, 'n' },
    { "translate_output", required_argument, 0, 'o' },
    { "start", required_argument, 0, 'p' },
    { "relocatable", no_argument, 0, 'r' },
    { "size", required_argument, 0, 's' },
    { "show", no_argument, 0, 'l' },
    { "translate_source", required_argument, 0, 't' },
    { "unicode", no_argument, 0, 'u' },
    { "verbose", no_argument, 0, 'v' },
    { "whitespace", required_argument, 0, 'w' },
    { "binary", no_argument, 0, 'y' },
    { "codepage", required_argument, 0, 'z' },
    { 0, 0, 0, 0 }
};

const char * g_pcCmdLineArgs = "a:bc:de:f:g:hilmno:p:rs:t:uvw:yz:";

//*****************************************************************************
//
// Prints the usage message for this application.
//
//*****************************************************************************
void Usage(char *argv, int bError)
{
    int iLoop;
    FILE *fhOut;

    //
    // Set the output stream according to whether we encountered an error or
    // were asked for help information.
    //
    fhOut = bError ? stderr : stdout;

    fprintf(fhOut, "Usage: %s [options] <font> [<font>]\n", basename(argv));
    fprintf(
            fhOut,
            "Converts any font that FreeType recognizes into a compressed font for use by\n");
    fprintf(
            fhOut,
            "the Stellaris Graphics Library.  The font generated may support either 8 bit\n");
    fprintf(fhOut,
            "indexing allowing support of various ISO8859 variants or 32 bit indexing\n");
    fprintf(fhOut,
            "allowing support for wide character sets such as Unicode.\n");
    fprintf(fhOut, "\n");
    fprintf(
            fhOut,
            "If the -r option is not supplied, the tool will generate a font containing a\n");
    fprintf(
            fhOut,
            "contiguous block of glyphs identified by the first and last character numbers\n");
    fprintf(fhOut,
            "provided.  To allow encoding of some ISO8859 character sets from Unicode\n");
    fprintf(fhOut,
            "fonts where these characters appear at higher codepoints (for example\n");
    fprintf(
            fhOut,
            "Latin/Cyrillic where the ISO8859-5 mapping appears at offset 0x400 in Unicode\n");
    fprintf(
            fhOut,
            "space), additional parameters may be supplied to translate a block of source\n");
    fprintf(
            fhOut,
            "font codepoint numbers downwards into the 0-255 ISO8859 range during conversion.\n");
    fprintf(fhOut, "\n");
    fprintf(fhOut,
            "If the -r option is supplied, the output font is relocatable (and hence\n");
    fprintf(
            fhOut,
            "suitable for use from non-random-access memory such as an SSI EEPROM or SDCard)\n");
    fprintf(
            fhOut,
            "and supports multiple blocks of characters from wide character codepages.  When\n");
    fprintf(
            fhOut,
            "generating this type of font, the -c parameter may be used to provide a list\n");
    fprintf(fhOut,
            "of the Unicode characters that are to be included in the output font.\n");
    fprintf(fhOut,
            "If -w is specified and more than one font name is provided on the command\n"
            "line, the fonts are searched in the order they appear to find characters\n"
            "if the previous font in the list does not contain a required glyph.\n");
    fprintf(fhOut, "\n");
    fprintf(fhOut, "Supported options are:\n");
    fprintf(fhOut, "  -b            Specifies that this is a bold font.\n");
    fprintf(fhOut, "  -f <filename> Specifies the base name for this font "
        "[default=\"font\"].\n");
    fprintf(fhOut, "  -i            Specifies that this is an italic font.\n");
    fprintf(fhOut,
            "  -m            Specifies that this is a monospaced font.\n");
    fprintf(fhOut,
            "  -s <size>     Specifies the point size of this font unless the parameter\n");
    fprintf(
            fhOut,
            "                starts with \"F\" in which case the supplied number is assumed\n");
    fprintf(
            fhOut,
            "                to be an index into the font's fixed size table. [default=20]\n");
    fprintf(fhOut, "  -w <num>      Forces a character to be whitespace "
        "[default=32]\n");
    fprintf(fhOut, "  -n            Do not force whitespace (-w ignored)\n");
    fprintf(fhOut, "  -p <num>      Specifies the first character to encode "
        "[default=32]\n");
    fprintf(fhOut, "  -e <num>      Specifies the last character to encode "
        "[default=126]\n");
    fprintf(
            fhOut,
            "  -t <num>      Specifies the codepoint of the first output font character\n"
            "                to translate downwards [default=0].  Ignored if used with -r.\n");
    fprintf(
            fhOut,
            "  -o <num>      Specifies the source font codepoint for the first character in\n"
            "                the translated block [default=0].  Ignored if used with -r.\n");
    fprintf(fhOut,
            "  -u            Use Unicode character mapping in the source font.\n");
    fprintf(fhOut,
            "  -r            Generate a relocatable, wide character set font.\n");
    fprintf(
            fhOut,
            "  -y            Write the output in binary format suitable for storage in a\n"
            "                file system.  If absent, a C format source file is generated.\n"
            "                Ignored unless -r is specified.\n");
    fprintf(fhOut,
            "  -c <filename> Encode characters whose codepoints are provided in the\n");
    fprintf(
            fhOut,
            "                given text file.  Each line of the file contains either a\n");
    fprintf(fhOut,
            "                single hex Unicode character number or two hex Unicode\n");
    fprintf(fhOut,
            "                numbers separated by a comma.  If the first non-comment\n"
            "                line contains \"REMAP\", the output font is generated using\n"
            "                a custom codepage with the glyphs identified by the characters\n"
            "                listed in the file indexed sequentially. (only valid with -r).\n");
    fprintf(fhOut,
            "  -a <num>      Specifies the index of the font character map to use in\n");
    fprintf(
            fhOut,
            "                the conversion.  If absent, Unicode is assumed for relocatable,\n");
    fprintf(
            fhOut,
            "                wide character fonts or when -u is specified, else the Adobe\n");
    fprintf(fhOut,
            "                Custom map is used if present or Unicode otherwise.\n");
    fprintf(fhOut,
            "  -d            Display details of the font provided. All other options are\n"
            "                ignored if this switch is provided.\n");
    fprintf(fhOut,
            "  -l            Show the chosen glyphs on the terminal (but don't write any other\n"
            "                output).\n");
    fprintf(fhOut,
            "  -z <num>      Set the output font's codepage to the supplied value.  This is\n"
            "                used to specify a custom codepage identifier when performing\n"
            "                glyph remapping. Values should be between CODEPAGE_CUSTOM_BASE\n"
            "                (0x8000) and 0xFFFF. (only valid with -r).\n");
    fprintf(fhOut, "  -v            Shows verbose output.\n");
    fprintf(fhOut, "  -h            Shows this help.\n");
    fprintf(fhOut, "\nLong command aliases are:\n\n");
    iLoop = 0;
    while(g_pCmdLineOptions[iLoop].name != (const char *) 0)
    {
        fprintf(fhOut, "  -%c           --%s\n", g_pCmdLineOptions[iLoop].val,
                g_pCmdLineOptions[iLoop].name);
        iLoop++;
    }
    fprintf(fhOut, "\n");
    fprintf(fhOut, "Examples:\n\n");
    fprintf(fhOut, "  %s -f foobar -s 24 foobar.ttf\n\n", basename(argv));
    fprintf(fhOut, "Produces fontfoobar24.c with a 24 point version of the "
        "font in foobar.ttf.\n\n");
    fprintf(
            fhOut,
            "  %s -f cyrillic -s 12 -u -p 32 -e 255 -t 160 -o 1024 unicode.ttf\n\n",
            basename(argv));
    fprintf(fhOut, "Produces fontcyrillic12.c with a 12 point version of the "
        "font in unicode.ttf\n"
        "with characters numbered 160-255 in the output (ISO8859-5"
        "Cyrillic glyphs) taken\n"
        "from the codepoints starting at 1024 in the source, unicode font.\n");
    fprintf(fhOut, "\n");
    fprintf(fhOut, "Report bugs to <support_lmi@ti.com>\n");
}

//*****************************************************************************
//
// Find a matching value in a string/value table and return the associated
// string.
//
//*****************************************************************************
const char *
GetStringFromValue(unsigned long ulValue, const tKeyString *pTable,
        int iNumEntries)
{
    int iLoop;

    //
    // Loop through all the encodings we know about.
    //
    for(iLoop = 0; iLoop < iNumEntries; iLoop++)
    {
        //
        // Does the character map passed match this encoding?
        //
        if(pTable[iLoop].ulValue == ulValue)
        {
            //
            // Yes - return the encoding's name.
            //
            return (pTable[iLoop].pcName);
        }
    }

    //
    // If we get here, we didn't find the encoding in our list.
    //
    return ("**Unrecognized**");
}

//*****************************************************************************
//
// Return the name of a character map
//
//*****************************************************************************
const char *
GetCharmapName(struct FT_CharMapRec_ *pCharMap)
{
    return (GetStringFromValue(pCharMap->encoding, g_pEncodingDescs,
            NUM_ENCODING_DESCS));
}

//*****************************************************************************
//
// Return the name associated with a given platform ID.
//
//*****************************************************************************
const char *
GetPlatformName(FT_UShort usPlatform)
{
    return (GetStringFromValue((unsigned long) usPlatform, g_pPlatformDescs,
            NUM_PLATFORM_DESCS));
}

//*****************************************************************************
//
// Get a string describing a given font codepage identifier.
//
//*****************************************************************************
const char *
GetCodepageName(unsigned short usCodepage)
{
    //
    // Is this a custom codepage identifier?
    //
    if(usCodepage >= CODEPAGE_CUSTOM_BASE)
    {
        //
        // Yes - return a generic string.
        //
        return ("CUSTOM");
    }
    else
    {
        //
        // No - look the codepage up in our string/value array.
        //
        return (GetStringFromValue(usCodepage, g_pCodepageDescs,
                NUM_CODEPAGE_DESCS));
    }
}

//*****************************************************************************
//
// Display information about the font whose name is passed.
//
// Returns 0 on success, 1 on failure.
//
//*****************************************************************************
int DisplayFontInfo(tConversionParameters *pParams)
{
    FT_Library pLibrary;
    FT_Face pFace;
    FT_UInt uiGlyphIndex;
    FT_ULong ulCharCode, ulLastCharCode, ulStart;
    int iLoop, iRunning, iBlock, iGlyph;

    //
    // Initialize the FreeType library.
    //
    if(FT_Init_FreeType(&pLibrary) != 0)
    {
        printf("%s: Unable to initialize the FreeType library.\n",
                pParams->pcAppName);
        return (1);
    }

    //
    // Load the specified font file into the FreeType library.
    //
    if(FT_New_Face(pLibrary, pParams->pcFontInputName[0], 0, &pFace) != 0)
    {
        printf("%s: Unable to open font file '%s'\n", pParams->pcAppName,
                pParams->pcFontInputName[0]);
        return (1);
    }

    //
    // We loaded the font so now display information we glean from the pFace
    // structure.
    //
    printf("\nInformation for font %s:\n\n", pParams->pcFontInputName[0]);
    printf("Family:       %s\n", pFace->family_name);
    printf("Style:        %s\n", pFace->style_name);
    printf("Num glyphs:   %d\n", pFace->num_glyphs);
    printf("Style:        0x%x\n", pFace->style_flags);
    if(pFace->style_flags & FT_STYLE_FLAG_ITALIC)
    {
        printf("    ITALIC\n");
    }
    if(pFace->style_flags & FT_STYLE_FLAG_BOLD)
    {
        printf("    BOLD\n");
    }

    printf("Flags:        0x%x\n", pFace->face_flags);

    if(pFace->face_flags & FT_FACE_FLAG_SCALABLE)
    {
        printf("    FT_FACE_FLAG_SCALABLE\n");
    }
    if(pFace->face_flags & FT_FACE_FLAG_FIXED_SIZES)
    {
        printf("    FT_FACE_FLAG_FIXED_SIZES\n");
    }
    if(pFace->face_flags & FT_FACE_FLAG_FIXED_WIDTH)
    {
        printf("    FT_FACE_FLAG_FIXED_WIDTH\n");
    }
    if(pFace->face_flags & FT_FACE_FLAG_SFNT)
    {
        printf("    FT_FACE_FLAG_SFNT\n");
    }
    if(pFace->face_flags & FT_FACE_FLAG_HORIZONTAL)
    {
        printf("    FT_FACE_FLAG_HORIZONTAL\n");
    }
    if(pFace->face_flags & FT_FACE_FLAG_VERTICAL)
    {
        printf("    FT_FACE_FLAG_VERTICAL\n");
    }
    if(pFace->face_flags & FT_FACE_FLAG_KERNING)
    {
        printf("    FT_FACE_FLAG_KERNING\n");
    }
    if(pFace->face_flags & FT_FACE_FLAG_FAST_GLYPHS)
    {
        printf("    FT_FACE_FLAG_FAST_GLYPHS\n");
    }
    if(pFace->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS)
    {
        printf("    FT_FACE_FLAG_MULTIPLE_MASTERS\n");
    }
    if(pFace->face_flags & FT_FACE_FLAG_GLYPH_NAMES)
    {
        printf("    FT_FACE_FLAG_GLYPH_NAMES\n");
    }
    if(pFace->face_flags & FT_FACE_FLAG_EXTERNAL_STREAM)
    {
        printf("    FT_FACE_FLAG_EXTERNAL_STREAM\n");
    }
    if(pFace->face_flags & FT_FACE_FLAG_HINTER)
    {
        printf("    FT_FACE_FLAG_HINTER\n");
    }

    if(FT_HAS_FIXED_SIZES(pFace))
    {
        printf("Fixed sizes:  %d\n", pFace->num_fixed_sizes);

        if(pFace->available_sizes)
        {
            for(iLoop = 0; iLoop < pFace->num_fixed_sizes; iLoop++)
            {
                printf("    %2d: %2d x %2d\n", iLoop,
                        pFace->available_sizes[iLoop].width,
                        pFace->available_sizes[iLoop].height);
            }
        }
    }
    printf("Num charmaps: %d\n", pFace->num_charmaps);
    for(iLoop = 0; iLoop < pFace->num_charmaps; iLoop++)
    {
        printf("    %d. %-26s (%08x), %-20s (%d), %d\n", iLoop, GetCharmapName(
                pFace->charmaps[iLoop]), pFace->charmaps[iLoop]->encoding,
                GetPlatformName(pFace->charmaps[iLoop]->platform_id),
                pFace->charmaps[iLoop]->platform_id,
                pFace->charmaps[iLoop]->encoding_id);
    }

    printf("\nUnicode characters encoded:\n");

    FT_Select_Charmap(pFace, FT_ENCODING_UNICODE);

    ulLastCharCode = FT_Get_First_Char(pFace, &uiGlyphIndex);

    iRunning = 0;
    iGlyph = 0;
    iBlock = 0;
    ulStart = ulLastCharCode;

    while(uiGlyphIndex != 0)
    {
        iGlyph++;

        //
        // Get the next character number in the font.
        //
        ulCharCode = FT_Get_Next_Char(pFace, ulLastCharCode, &uiGlyphIndex);

        //
        // Does this character code immediately follow the last one we read?
        //
        if(ulCharCode == (ulLastCharCode + 1))
        {
            //
            // Yes - continue the same run.
            //
            iRunning = 1;
        }
        else
        {
            //
            // This is the end of a run or the last character we read was
            // a single on its own.
            //

            iBlock++;

            //
            // Is this the end of a run or just a single character?
            //
            if(pParams->bVerbose)
            {
                if(iRunning)
                {
                    //
                    // We are at the end of a run of characters so display the
                    // range found.
                    //
                    printf("0x%06x-0x%06x ", ulStart, ulLastCharCode);
                }
                else
                {
                    //
                    // We found a character on its own so display its code.
                    //
                    printf("0x%06x          ", ulStart);
                }

                if(!(iBlock % 4))
                {
                    printf("\n");
                }
            }

            //
            // Set up to detect another run of characters.
            //
            ulStart = ulCharCode;
            iRunning = 0;
        }

        //
        // Get ready for the next character.
        //
        ulLastCharCode = ulCharCode;
    }

    printf("\n%d encoded characters in %d blocks.\n", iGlyph, iBlock);

    //
    // All is well.
    //
    return (0);
}

//*****************************************************************************
//
// Free the character map linked list.  It is assumed that the head element
// of the list is fixed and must not be freed.
//
//*****************************************************************************
void FreeCharMapList(tCodepointBlock *pHead)
{
    tCodepointBlock *pBlock, *pNext;

    //
    // Start at the beginning of the list we've been passed. Note that we do
    // not free the head (anchor) element of the list.
    //
    pBlock = pHead->pNext;

    //
    // Walk the list, freeing all the blocks it contains.
    //
    while(pBlock)
    {
        pNext = pBlock->pNext;
        free(pBlock);
        pBlock = pNext;
    }
}

//*****************************************************************************
//
// Build a linked list of characters to encode from the supplied character map
// text file.
//
// The list returned by this function will start in the head structure passed
// and end with an empty block (ulStart = pNext = 0, ulEnd = 0xFFFFFFFF).
//
// \return Returns the number of valid blocks read from the charmap file or
// 0 on error.
//
//*****************************************************************************
int ParseCharMapFile(tConversionParameters *pParams, tCodepointBlock *pHead)
{
    FILE *pFile;
    tCodepointBlock *pBlock, *pNext;
    int iNumBlocks, iNumVals, iLineNum, bError;
    unsigned long ulStart, ulEnd;
    char pcBuffer[80];

    //
    // Open the character map file for text reading.
    //
    pFile = fopen(pParams->pcCharFile, "r");

    //
    // Did we open the file successfully?
    //
    if(!pFile)
    {
        fprintf(stderr, "%s: Character map file %s doesn't exist or cannot be "
            "opened!\n", pParams->pcAppName, pParams->pcCharFile);
        return (0);
    }

    //
    // Set things up to read from the file.
    //
    iLineNum = 0;
    iNumBlocks = 0;
    bError = false;
    pHead->pNext = NULL;
    pHead->ulStart = 0xFFFFFFFF;
    pHead->ulEnd = 0xFFFFFFFF;
    pBlock = pHead;

    //
    // Continue reading from the file until we are finished.
    //
    while(fgets(pcBuffer, 80, pFile))
    {
        //
        // Increment our line counter.
        //
        iLineNum++;

        //
        // Skip this line if it's a comment or a blank.
        //
        if((pcBuffer[0] == '#') || (pcBuffer[0] == '\n'))
        {
            continue;
        }

        //
        // Check to see if this is a line telling us to remap codepoints.
        //
        if(!strncmp(pcBuffer, "REMAP", 5))
        {
            //
            // Yes - set a flag that we will use later when writing the font
            // file.
            //
            pParams->bRemap = 1;
            continue;
        }

        //
        // Tell the user which block we are reading.
        //
        if(pParams->bVerbose)
        {
            printf("Block %s", pcBuffer);
        }

        //
        // Parse one or two values from the line
        //
        iNumVals = sscanf(pcBuffer, "%x, %x", &ulStart, &ulEnd);

        //
        // Did we read any values?
        //
        if(iNumVals)
        {
            //
            // If the line contained a single value, set the end to be the same
            // as the start.
            //
            if(iNumVals == 1)
            {
                ulEnd = ulStart;
            }

            //
            // Make sure the start and end values appear valid.
            //
            if(ulStart > ulEnd)
            {
                fprintf(stderr, "%s: Error - start value greater than end in "
                    "charmap file at line %d.\n", pParams->pcAppName, iLineNum);
                bError = true;
                break;
            }

            //
            // If necessary, allocate a new block.
            //
            if((pBlock->ulStart != 0xFFFFFFFF) && (pBlock->ulEnd != 0xFFFFFFFF))
            {
                //
                // Allocate the next block and link it to the list.
                //
                pNext = malloc(sizeof(tCodepointBlock));
                if(pNext)
                {
                    //
                    // Clean out the next block and link it to the end of the list.
                    // We do this (rather than linking it to the head) since we
                    // want to maintain the order of blocks as defined in the
                    // charmap file.
                    //
                    pBlock->pNext = pNext;
                    pBlock = pNext;
                    pNext->pNext = NULL;
                }
                else
                {
                    //
                    // We can't allocate memory for our new block!
                    //
                    fprintf(stderr,
                            "%s: Error - can't allocate memory for charmap "
                                "block!\n", pParams->pcAppName);
                    bError = true;
                    break;
                }
            }

            //
            // Add this block to the list.  We do not currently try to
            // consolidate blocks but this is something we could do later.
            //
            pBlock->ulStart = ulStart;
            pBlock->ulEnd = ulEnd;
            iNumBlocks++;
        }
        else
        {
            //
            // The line didn't parse correctly. Print a warning but continue.
            //
            fprintf(stderr, "Syntax error in charmap file at line %d. "
                "Ignoring.\n", iLineNum);
        }
    }

    //
    // Did we experience any error when reading the file?
    //
    if(bError)
    {
        //
        // Yes - tidy up before we leave.
        //
        FreeCharMapList(pHead);
        iNumBlocks = 0;
    }

    //
    // Close the character map file.
    //
    fclose(pFile);

    if(pParams->bVerbose)
    {
        printf("Character map file parsed. %d blocks found.\n", iNumBlocks);
    }

    //
    // Return the number of blocks in the character block list.
    //
    return (iNumBlocks);
}

//*****************************************************************************
//
// Set the font character size that we are to render.  Returns true on success
// false on failure.
//
//*****************************************************************************
int SetFontCharSize(FT_Face pFace, tConversionParameters *pParams)
{
    //
    // Have we been asked to choose one of the font's fixed sizes?
    //
    if(pParams->bFixedSize)
    {
        //
        // Yes - make sure the font support fixed sizes.
        //
        if(!FT_HAS_FIXED_SIZES(pFace))
        {
            fprintf(stderr, "%s: This font does not contain fixed sizes!\n",
                    pParams->pcAppName);
            return (false);
        }

        //
        // Make sure that the fixed size index passed is valid.
        //
        if(pParams->iSize >= pFace->num_fixed_sizes)
        {
            fprintf(stderr, "%s: Invalid size index (%d) passed. Valid values "
                "are < %d!\n", pParams->pcAppName, pParams->iSize,
                    pFace->num_fixed_sizes);
            return (false);
        }

        //
        // Remember the font cell dimensions.
        //
        pParams->iFixedX = pFace->available_sizes[pParams->iSize].width;
        pParams->iFixedY = pFace->available_sizes[pParams->iSize].height;

        //
        // Select the chosen fixed size.
        //
        FT_Select_Size(pFace, pParams->iSize);

        //
        // Tell the user what we've done if they want verbose output.
        //
        if(pParams->bVerbose)
        {
            printf("Selected size index %d (%d x %d).\n", pParams->iSize,
                    pFace->available_sizes[pParams->iSize].width,
                    pFace->available_sizes[pParams->iSize].height);
        }
    }
    else
    {
        //
        // Not a fixed size so check to see if the size is reasonable.
        //
        if((pParams->iSize <= 0) || (pParams->iSize > 100))
        {
            fprintf(stderr, "%s: The font size must be from 1 to 100, "
                "inclusive.\n", pParams->pcAppName);
            return (false);
        }

        //
        // Set the size based on the point size provided.  Are we
        // generating a monospaced font?
        //
        if(pParams->bMono)
        {
            if(pParams->bVerbose)
            {
                printf("Selected monospaced %dpt size.\n", pParams->iSize);
            }

            //
            // Yes - set independent width and height sizes to give us a
            // nicely proportioned character cell.
            //
            FT_Set_Char_Size(pFace, pParams->iSize * 56, pParams->iSize * 64,
                    0, 72);
        }
        else
        {
            if(pParams->bVerbose)
            {
                printf("Selected variable width %dpt size.\n", pParams->iSize);
            }

            //
            // No - set only the height since we'll be using the native
            // character width in this case.
            //
            FT_Set_Char_Size(pFace, 0, pParams->iSize * 64, 0, 72);
        }

        //
        // We are not using one of the font's fixed sizes.
        //
        pParams->iFixedX = 0;
        pParams->iFixedY = 0;

    }

    return (true);
}

//*****************************************************************************
//
// Return the output font codepage identifier mapping to the FreeType character
// map encoding value passed.
//
// Returns 0xFFFF on error or a valid codepage identifier if a mapping is
// found.
//
//*****************************************************************************
unsigned short CodePageFromEncoding(FT_Encoding eEncoding)
{
    int iLoop;

    //
    // Loop through all the known encoding/codepage mappings.
    //
    for(iLoop = 0; iLoop < NUM_CODEPAGE_MAPPINGS; iLoop++)
    {
        //
        // Does this table entry refer to the passed encoding?
        //
        if((unsigned long) eEncoding == g_psCodePageMapping[iLoop].ulFTEncoding)
        {
            //
            // Yes - return the matching codepage identifier.
            //
            return (g_psCodePageMapping[iLoop].usCodepage);
        }
    }

    //
    // If we get here, there is no mapping between the font encoding and a
    // GrLib font codepage so report an error.
    //
    return (0xFFFF);
}

//*****************************************************************************
//
// Render a single glyph into the structure passed and determine the minimum
// and maximum X pixel coordinate for the glyph.
//
// Returns true on success or false if the font did not contain the requested
// character.
//
//*****************************************************************************
tBoolean RenderGlyph(tConversionParameters *pParams, FT_Face pFace,
        unsigned long ulCodePoint, tGlyph *pGlyph, tBoolean bQuiet)
{
    int iXMin, iXMax, iX, iY;
    unsigned int uiIndex;
    tBoolean bRendered;
    FT_GlyphSlot pSlot;
    FT_Bitmap *ppMap;
    FT_Error iError;

    //
    // Get some pointers to make things easier.
    //
    pSlot = pFace->glyph;
    ppMap = &(pSlot->bitmap);

    //
    // Get the character index within the font from the original codepoint.
    //
    uiIndex = FT_Get_Char_Index(pFace, ulCodePoint);

    //
    // For now, remember that we have not yet rendered the glyph.
    //
    bRendered = false;

    //
    // Does the font contain this character?
    //
    if(uiIndex)
    {
        //
        // The font contains the character so load the glyph.
        //
        iError = FT_Load_Glyph(pFace, uiIndex, FT_LOAD_DEFAULT
                | FT_LOAD_TARGET_MONO);
        if(!iError)
        {
            //
            // If this is an outline glyph, then render it.
            //
            if(pSlot->format == FT_GLYPH_FORMAT_OUTLINE)
            {
                iError = FT_Render_Glyph(pSlot, FT_RENDER_MODE_MONO);
                if(iError & !bQuiet)
                {
                    fprintf(stderr,
                            "%s: Error %d: Can't render glyph for 0x%x!\n",
                            pParams->pcAppName, iError, ulCodePoint);
                }
            }

            //
            // Save the relevant information about this glyph.
            //
            if(ppMap->width == 0)
            {
                //
                // This glyph is zero width so it's likely a space.  Dump a
                // warning.
                //
                if(!bQuiet)
                {
                    fprintf(stderr, "%s: Warning: Zero width glyph for 0x%x.\n",
                            pParams->pcAppName, ulCodePoint);
                }

                if(pParams->bVerbose)
                {
                    printf("Zero width character 0x%x:\n", ulCodePoint);
                    printf("  width    %d\n", pSlot->metrics.width);
                    printf("  height   %d\n", pSlot->metrics.height);
                    printf("  AdvanceX %d.%02d\n", pSlot->metrics.horiAdvance
                            >> 6, pSlot->metrics.horiAdvance & 0x3F);
                    printf("  AdvanceY %d.%02d\n", pSlot->metrics.vertAdvance
                            >> 6, pSlot->metrics.vertAdvance & 0x3F);
                }

                //
                // Fill in blank information on the glyph except for the width
                // which we read from the glyph metrics.
                //
                pGlyph->ulCodePoint = ulCodePoint;
                pGlyph->iWidth = 0;
                pGlyph->iRows = 0;
                pGlyph->iPitch = 0;
                pGlyph->iBitmapTop = 0;
                pGlyph->iMaxX = pSlot->metrics.horiAdvance >> 6;
                pGlyph->iMinX = 0;
                pGlyph->pucData = NULL;
                pGlyph->pucChar = NULL;
                bRendered = true;
            }
            else
            {
                //
                // We rendered the glyph so now save all the relevant
                // information.
                //
                pGlyph->ulCodePoint = ulCodePoint;
                pGlyph->iWidth = ppMap->width;
                pGlyph->iRows = ppMap->rows;
                pGlyph->iPitch = ppMap->pitch;
                pGlyph->iBitmapTop = pSlot->bitmap_top;
                pGlyph->pucData = malloc(ppMap->rows * ppMap->pitch);

                if(pParams->bVerbose)
                {
                    printf("Character 0x%x: %d x %d, pitch %d, top %d\n",
                            ulCodePoint, pGlyph->iWidth, pGlyph->iRows,
                            pGlyph->iPitch, pGlyph->iBitmapTop);
                }

                //
                // Did we manage to allocate a buffer for the glyph bitmap?
                //
                if(pGlyph->pucData)
                {
                    //
                    // We allocated memory to store the rendered glyph so now
                    // copy the data into our new buffer.
                    //
                    memcpy(pGlyph->pucData, ppMap->buffer, ppMap->rows
                            * ppMap->pitch);
                    bRendered = true;
                }
                else
                {
                    if(!bQuiet || pParams->bVerbose)
                    {
                        fprintf(stderr, "%s: Error! Can't allocate buffer for "
                                "glyph %x bitmap!\n", pParams->pcAppName,
                                ulCodePoint);
                    }
                }
            }
        }
        else
        {
            if(!bQuiet || pParams->bVerbose)
            {
                fprintf(stderr,
                    "%s: Warning %d: Could not load glyph for '%c' (%d)\n",
                    pParams->pcAppName, iError, ulCodePoint);
            }
        }
    }
    else
    {
        if(pParams->bVerbose)
        {
            printf("No glyph found for character 0x%x.\n", ulCodePoint);
        }
    }

    if(!bRendered)
    {
        //
        // The font does not contain a glyph for this character so mark the
        // output structure to indicate that this is an undefined character.
        //
        pGlyph->ulCodePoint = ulCodePoint;
        pGlyph->iWidth = 0;
        pGlyph->iRows = 0;
        pGlyph->iPitch = 0;
        pGlyph->iBitmapTop = 0;
        pGlyph->iMaxX = 0;
        pGlyph->iMinX = 0;
        pGlyph->pucData = NULL;
        pGlyph->pucChar = NULL;
    }
    else
    {
        //
        // Now determine the minimum and maximum X pixel value in the
        // rendered glyph.
        //

        //
        // Loop through the rows of the bitmap for this glyph.
        //
        for(iY = 0, iXMin = 1000000, iXMax = 0; iY < pGlyph->iRows; iY++)
        {
            //
            // Find the minimum X for this row of the glyph.
            //
            for(iX = 0; iX < pGlyph->iWidth; iX++)
            {
                if(pGlyph->pucData[(iY * pGlyph->iPitch) + (iX / 8)] & (1 << (7
                        - (iX & 7))))
                {
                    if(iX < iXMin)
                    {
                        iXMin = iX;
                    }
                    break;
                }
            }

            //
            // Find the maximum X for this row of the glyph.
            //
            for(iX = pGlyph->iWidth - 1; iX >= 0; iX--)
            {
                if(pGlyph->pucData[(iY * pGlyph->iPitch) + (iX / 8)] & (1 << (7
                        - (iX & 7))))
                {
                    if(iX > iXMax)
                    {
                        iXMax = iX;
                    }
                    break;
                }
            }
        }

        if(pParams->bVerbose)
        {
            printf("Character 0x%x: xMin %d, xMax %d\n", ulCodePoint, iXMin,
                    iXMax);
        }

        //
        // If this glyph has no bitmap data (typically just the space
        // character), then provide a default minimum and maximum X value.
        //
        if(iXMin == 1000000)
        {
            iXMin = 0;
            if(pParams->bFixedSize)
            {
                iXMax = pFace->available_sizes[pParams->iSize].width;
            }
            else
            {
                if(pGlyph->iMaxX)
                {
                    iXMax = pGlyph->iMaxX;
                }
                else
                {
                    iXMax = (3 * pParams->iSize) / 10;
                }
            }

            if(pParams->bVerbose)
            {
                printf("Set char 0x%x width to %d pixels.\n", ulCodePoint,
                        iXMax);
            }
        }

        //
        // Save the minimum and maximum X value for this glyph.
        //
        pGlyph->iMinX = iXMin;
        pGlyph->iMaxX = iXMax;
    }

    return (bRendered);
}

//*****************************************************************************
//
// Display the glyph bitmap using ASCII "X" and " " characters on the terminal.
//
//*****************************************************************************
tBoolean
DisplayGlyph(tConversionParameters *pParams, tGlyph *pGlyph,
             int iWidth, int iYMin, int iYMax)
{
    int iX, iY, iPixel, iBytesPerLine, iMaxPix;
    unsigned char ucPixel;
    unsigned char *pucRow;

    printf("Character 0x%x, %dx%d, pitch %d bytes:\n", pGlyph->ulCodePoint,
            pGlyph->iWidth, pGlyph->iRows, pGlyph->iPitch);

    //
    // If this glyph has no bitmap attached to it and no width defined, bomb
    // out now since it's obviously an undefined character that we can skip.
    // If it has a width but no data, it's a space so continue.
    //
    if(!pGlyph->pucData && !pGlyph->iMaxX)
    {
        printf("No data - glyph absent.\n");
        return (false);
    }

    //
    // Run through the rows and columns of the glyph bitmap outputing pixels
    // one by one.
    //
    for(iY = 0; iY < pGlyph->iRows; iY++)
    {
        //
        // Print the row number.
        //
        printf("\n %3d - ", iY);

        //
        // Get a pointer to the start of the row's data.
        //
        pucRow = &pGlyph->pucData[iY * pGlyph->iPitch];

        //
        // Loop through each byte in this row.
        //
        for(iX = 0; iX < pGlyph->iPitch; iX++)
        {
            //
            // Get this group of 8 pixels.
            //
            ucPixel = pucRow[iX];

            //
            // How many pixels do we still have to draw?
            //
            iMaxPix = pGlyph->iWidth - (iX * 8);
            if(iMaxPix >= 8)
            {
                iMaxPix = 8;
            }

            //
            // Dump each pixel as an ASCII character.
            //
            for(iPixel = 0; iPixel < iMaxPix; iPixel++)
            {
                printf("%c", ucPixel & (1 << (7 - iPixel)) ? 'X' : '.');
            }
        }
    }

    //
    // Leave a blank line after this glyph's pattern.
    //
    printf("\n\n");

    return(true);
}

//*****************************************************************************
//
// Compress the glyph bitmap data and attach it to the passed glyph structure.
//
//*****************************************************************************
tBoolean CompressGlyph(tConversionParameters *pParams, tGlyph *pGlyph,
        int iWidth, int iYMin, int iYMax)
{
    int iOpt, iX, iY, iXMin, iXMax;
    int iPrevBit, iBit, iZero, iOne;
    unsigned char pucChar[512];

    //
    // If this glyph has no bitmap attached to it and no width defined, bomb
    // out now since it's obviously an undefined character that we can skip.
    // If it has a width but no data, it's a space so continue.
    //
    if(!pGlyph->pucData && !pGlyph->iMaxX)
    {
        if(pParams->bVerbose)
        {
            printf("Error compressing glyph. pucData 0x%x, iMaxX %d\n",
                    pGlyph->pucData, pGlyph->iMaxX);
        }
        return (false);
    }

    //
    // Determine the width and starting position for this glyph based on
    // if this is a monospaced or proportional font.
    //
    if(pParams->bMono)
    {
        //
        // For monospaced fonts, horizontally center the glyph in the
        // character cell, providing uniform padding on either side of the
        // glyph.
        //
        iXMin = (pGlyph->iMinX - ((iWidth + 1 + (pParams->iSize / 10)
                - pGlyph->iMaxX + pGlyph->iMinX) / 2));
        iXMax = iXMin + iWidth + 1 + (pParams->iSize / 10);
    }
    else
    {
        //
        // For proportionally-spaced fonts, left-align the glyph in the
        // character cell and provide uniform inter-character padding on
        // the right side.
        //
        iXMin = pGlyph->iMinX;
        iXMax = pGlyph->iMaxX + 1 + (pParams->iSize / 10);
    }

    //
    // Loop through the rows of this glyph, extended the glyph as necessary
    // to be the size of the maximal bounding box for this font.
    //
    for(iY = 0, iZero = 0, iOne = 0, iBit = 0, iOpt = 0; iY < (iYMin - iYMax
            + 1); iY++)
    {
        //
        // Loop through the columns of this row, extending the glyph as
        // necessary.
        //
        for(iX = iXMin; iX < iXMax; iX++)
        {
            //
            // Save the value of the previous bit.
            //
            iPrevBit = iBit;

            //
            // See if this bit is within the bitmap data for this glyph.
            //
            if((iY >= (iYMin - pGlyph->iBitmapTop)) && (iY < (iYMin
                    - pGlyph->iBitmapTop + pGlyph->iRows)) && (iX
                    >= pGlyph->iMinX) && (iX <= pGlyph->iMaxX))
            {
                //
                // Extract this bit from the bitmap data for this glyph.
                //
                if(pGlyph->pucData[((iY - iYMin + pGlyph->iBitmapTop)
                        * pGlyph->iPitch) + (iX / 8)] & (1 << (7 - (iX & 7))))
                {
                    iBit = 1;
                }
                else
                {
                    iBit = 0;
                }
            }

            //
            // Otherwise, bits not within the bitmap data for the glyph are
            // filled with zero.
            //
            else
            {
                iBit = 0;
            }

            //
            // See if this is the very first bit in the glyph.
            //
            if((iX == iXMin) && (iY == 0))
            {
                //
                // Set the number of zero and one bits based on the value
                // of the first bit.
                //
                iZero = 1 - iBit;
                iOne = iBit;

                //
                // Go to the next bit.
                //
                continue;
            }

            //
            // See if this pixel value matches the previous pixel value.
            //
            if(iBit == iPrevBit)
            {
                //
                // Increment the appropriate pixel count.
                //
                if(iBit == 0)
                {
                    iZero++;
                }
                else
                {
                    iOne++;
                }

                //
                // Keep counting if the end of the pixel array has not been
                // reached.
                //
                if((iY != (iYMin - iYMax)) || (iX != (iXMax - 1)))
                {
                    continue;
                }
            }

            //
            // See if the pixel value is one (which means that the previous
            // pixel value would be zero).
            //
            else if(iBit == 1)
            {
                //
                // Increment the count of one pixels.
                //
                iOne++;

                //
                // Keep counting if the end of the pixel array has not been
                // reached.
                //
                if((iY != (iYMin - iYMax)) || (iX != (iXMax - 1)))
                {
                    continue;
                }
            }

            //
            // See if there are more than 45 zero pixels, which will be
            // encoded as a repeated byte.
            //
            while(iZero > 45)
            {
                //
                // Encode the zero bits as a repeated zero byte.
                //
                pucChar[iOpt++] = 0x00;
                pucChar[iOpt++] = (iZero > 1016) ? 127 : (iZero / 8);

                //
                // Remove the zero bits that were just output.
                //
                iZero -= (iZero > 1016) ? 1016 : (iZero & ~7);
            }

            //
            // Loop while there are more than 15 zero pixels.
            //
            while(iZero > 15)
            {
                //
                // Output a byte that produces 15 zero pixels and no one
                // pixels.
                //
                pucChar[iOpt++] = 0xf0;

                //
                // Decrement the count of zero pixels by the number just
                // output.
                //
                iZero -= 15;
            }

            //
            // See if there are more than 15 one pixels.
            //
            if(iOne > 15)
            {
                //
                // Output a byte that produces the remaining zero pixels
                // and 15 one pixels.
                //
                pucChar[iOpt++] = (iZero << 4) | 15;

                //
                // Decrement the count of one pixels by the number just
                // output.
                //
                iOne -= 15;

                //
                // See if there are more than 45 one pixels, which will be
                // encoded as a repeated byte.
                //
                while(iOne > 45)
                {
                    //
                    // Encode the one bits as a repeated one byte.
                    //
                    pucChar[iOpt++] = 0x00;
                    pucChar[iOpt++] = (0x80
                            | ((iOne > 1016) ? 127 : (iOne / 8)));

                    //
                    // Remove the one bits that were just output.
                    //
                    iOne -= (iOne > 1016) ? 1016 : (iOne & ~7);
                }

                //
                // Loop while there are more one pixels.
                //
                while(iOne)
                {
                    //
                    // See if ther are than 15 one pixels left.
                    //
                    if(iOne > 15)
                    {
                        //
                        // Output a byte that produces no zero pixels and
                        // 15 one pixels.
                        //
                        pucChar[iOpt++] = 0x0f;

                        //
                        // Decrement the count of one pixels by the number
                        // just copied.
                        //
                        iOne -= 15;
                    }
                    else
                    {
                        //
                        // Output a byte that produces the remaining one
                        // pixels.
                        //
                        pucChar[iOpt++] = iOne;

                        //
                        // There are no more one pixels to output.
                        //
                        break;
                    }
                }
            }

            //
            // Otherwise, see if there are any zero or one pixels that need
            // to be output.
            //
            else if(iZero || iOne)
            {
                //
                // Output a byte that produces the remaining zero and one
                // pixels.  There is guaranteed to be less than 16 of each
                // at this point.
                //
                pucChar[iOpt++] = (iZero << 4) | iOne;
            }

            //
            // Bytes are output when a zero pixel following a one pixel is
            // encountered, so the loop should be restarted with the zero
            // pixel count set to 1 (for the pixel that caused the output)
            // and the one pixel count set to 0 (since no one pixels have
            // been found yet in the new span).
            //
            iZero = 1;
            iOne = 0;

            //
            // Print an error and return if the size of the compressed
            // character is too large.
            //
            if(iOpt > 254)
            {
                fprintf(stderr, "%s: Character '%c' was larger than 256 "
                    "bytes!\n", pParams->pcAppName, pGlyph->ulCodePoint);
                return (false);
            }
        }
    }

    //
    // Save the compressed data for this glyph.
    //
    pGlyph->pucChar = malloc(iOpt + 2);
    pGlyph->pucChar[0] = iOpt + 2;
    pGlyph->pucChar[1] = iXMax - iXMin;
    memcpy(pGlyph->pucChar + 2, pucChar, iOpt);

    return(true);
}

//*****************************************************************************
//
// Free all memory buffers attached to an array of glyphs.  Note that this
// function does not free the memory used by the glyph array itself.
//
//*****************************************************************************
void FreeGlyphs(tGlyph *pGlyph, int iCount)
{
    //
    // Loop through the whole array we have been passed.
    //
    while(iCount)
    {
        //
        // Does this glyph had a bitmap attached to it?
        //
        if(pGlyph->pucData)
        {
            //
            // Yes - free the bitmap memory.
            //
            free(pGlyph->pucData);
            pGlyph->pucData = NULL;
        }

        //
        // Does this glyph have a compressed character attached to it?
        //
        if(pGlyph->pucChar)
        {
            //
            // Yes - free the compressed image.
            //
            free(pGlyph->pucChar);
            pGlyph->pucChar = NULL;
        }

        //
        // Move on to the next glyph in the list.
        //
        pGlyph++;
        iCount--;
    }
}

//*****************************************************************************
//
// Write the block tables and glyph data for a binary font whose glyphs are not
// being remapped.
//
//*****************************************************************************
tBoolean WriteBinaryBlocks(FILE *pFile, tFontInfo *pFont,
        tGlyph *pGlyphStart, tCodepointBlock *pBlockList)
{
    tCodepointBlock *pBlock;
    tFontBlock sBlock;
    tGlyph *pGlyph, *pGlyphBlockStart;
    unsigned long ulOffset, ulTemp;
    int iGlyph, iCount, iLoop, iX;

    //
    // The first glyph offset table will appear immediately after the block
    // table.
    //
    ulOffset = sizeof(tFontWide) + (pFont->usNumBlocks * sizeof(tFontBlock));

    //
    // Loop through each of the blocks.
    //
    for(iX = 0, pBlock = pBlockList, pGlyph = pGlyphStart; iX
            < pFont->usNumBlocks; iX++)
    {
        //
        // Generate the block table entry for this block.
        //
        sBlock.ulStartCodepoint = pBlock->ulStart;
        sBlock.ulNumCodepoints = (pBlock->ulEnd - pBlock->ulStart) + 1;
        sBlock.ulGlyphTableOffset = ulOffset;
        fwrite(&sBlock, sizeof(tFontBlock), 1, pFile);

        //
        // Update the offset to the next block's glyph table.  First adjust
        // for the size of this block's glyph offset table.
        //
        ulOffset += ((pBlock->ulEnd - pBlock->ulStart) + 1) * 4;

        //
        // Now walk through the characters in this block adding up the
        // compressed size of each.
        //
        for(iGlyph = 0; iGlyph < ((pBlock->ulEnd - pBlock->ulStart) + 1); iGlyph++)
        {
            //
            // Check that the glyph has the expected codepoint number.  If this
            // fails, this indicates a bug in this application.
            //
            if(pGlyph->ulCodePoint != pBlock->ulStart + iGlyph)
            {
                fprintf(stderr, "Error: Expected codepoint 0x%x but glyph is "
                    "for 0x%x!\n", pBlock->ulStart + iGlyph,
                        pGlyph->ulCodePoint);
            }

            //
            // If this glyph has data, add its size to the tally.
            //
            if(pGlyph->pucChar)
            {
                ulOffset += pGlyph->pucChar[0];
            }

            //
            // Move on to the next glyph.
            //
            pGlyph++;
        }

        //
        // Round up to the next multiple of 4 since we always start a new
        // glyph offset table on a 4 byte boundary.
        //
        ulOffset = (ulOffset + 3) & ~3;

        //
        // Move to the next block.
        //
        pBlock = pBlock->pNext;
    }

    //
    // At this point, we have written all the font block headers. Now we move
    // on to the glyph data for each block.  This comprises a glyph offset
    // table followed by the compressed data for each glyph in the block.  In
    // these sections, the offsets are relative to the first entry in the
    // glyph offset table (since this makes it a lot easier to copy blocks of
    // glyph data between fonts).
    //

    //
    // Loop through each of the blocks again.
    //
    for(iX = 0, pBlock = pBlockList, pGlyph = pGlyphStart; iX
            < pFont->usNumBlocks; iX++)
    {
        //
        // Remember the pointer to the first glyph for this block.
        //
        pGlyphBlockStart = pGlyph;

        //
        // Generate the glyph offset table for this block.
        //

        //
        // The offset to the first glyph data is just the size of the offset
        // table.
        //
        ulOffset = ((pBlock->ulEnd - pBlock->ulStart) + 1) * 4;

        //
        // Loop through each glyph in the block and write the glyph offset
        // table.
        //
        for(iGlyph = 0; iGlyph < ((pBlock->ulEnd - pBlock->ulStart) + 1); iGlyph++)
        {
            //
            // Check that the glyph has the expected codepoint number.  If this
            // fails, this indicates a bug in this application.
            //
            if(pGlyph->ulCodePoint != pBlock->ulStart + iGlyph)
            {
                fprintf(stderr, "Error: Exected codepoint 0x%x but glyph is "
                    "for 0x%x!\n", pBlock->ulStart + iGlyph,
                        pGlyph->ulCodePoint);
            }

            //
            // Does this glyph have data?
            //
            if(pGlyph->pucChar)
            {
                //
                // This glyph has data so write the offset to the output.
                //
                fwrite(&ulOffset, 4, 1, pFile);

                //
                // Update the offset to leave space for this glyph's data.
                //
                ulOffset += pGlyph->pucChar[0];
            }
            else
            {
                //
                // This glyph has no data so write a marker 0 indicating that
                // the glyph is absent from the font.
                //
                ulTemp = 0;
                fwrite(&ulTemp, 4, 1, pFile);
            }

            //
            // Move on to the next glyph.
            //
            pGlyph++;
        }

        //
        // Here, the glyph offset table is written.  Now write the glyph data
        // itself.
        //

        //
        // Loop through each glyph in the block and write the glyph offset
        // table.
        //
        iCount = 0;
        for(iGlyph = 0, pGlyph = pGlyphBlockStart, ulOffset = 0; iGlyph
                < ((pBlock->ulEnd - pBlock->ulStart) + 1); iGlyph++)
        {
            //
            // Does this glyph have any data?
            //
            if(pGlyph->pucChar)
            {
                //
                // Yes- write the glyph data and update the offset.
                //
                ulOffset += pGlyph->pucChar[0];
                fwrite(pGlyph->pucChar, pGlyph->pucChar[0], 1, pFile);
            }

            //
            // Move on to the next glyph.
            //
            pGlyph++;
        }

        //
        // At this point, the block is complete but we may need to add some
        // padding bytes to get us to the next 4 byte boundary in the file.
        //
        if(ulOffset % 4)
        {
            ulTemp = 0;
            fwrite(&ulTemp, 4 - (ulOffset % 4), 1, pFile);
        }

        //
        // Move to the next block.
        //
        pBlock = pBlock->pNext;
    }

    return(true);
}

//*****************************************************************************
//
// Write the block tables and glyph data for a binary font whose glyphs are
// being remapped.
//
//*****************************************************************************
tBoolean WriteRemappedBinaryBlocks(FILE *pFile, tFontInfo *pFont,
        tGlyph *pGlyphStart, tCodepointBlock *pBlockList)
{
    tCodepointBlock *pBlock;
    unsigned long ulOffset, ulTemp;
    tFontBlock sBlock;
    int iGlyph, iCount, iLoop;

    //
    // The first glyph offset table will appear immediately after the block
    // table. Since we're remapping, there is only a single block in the output
    // font.
    //
    ulOffset = sizeof(tFontWide) + sizeof(tFontBlock);

    //
    // Generate the block table entry for the single remapped block.
    //
    sBlock.ulGlyphTableOffset = ulOffset;
    sBlock.ulStartCodepoint = 1;
    sBlock.ulNumCodepoints = pFont->ulNumGlyphs;
    fwrite(&sBlock, sizeof(tFontBlock), 1, pFile);

    //
    // The offset to the first glyph data is just the size of the offset
    // table.
    //
    ulOffset = pFont->ulNumGlyphs * 4;

    //
    // Loop through each glyph in the block and write the glyph offset
    // table.
    //
    for(iGlyph = 0; iGlyph < pFont->ulNumGlyphs; iGlyph++)
    {
        //
        // Does this glyph have data?
        //
        if(pGlyphStart[iGlyph].pucChar)
        {
            //
            // Yes - write the offset to the table.
            //
            fwrite(&ulOffset, 4, 1, pFile);

            //
            // Update the offset to leave space for this glyph's data.
            //
            ulOffset += pGlyphStart[iGlyph].pucChar[0];
        }
        else
        {
            //
            // The glyph has no data so write a 0 to indicate this.
            //
            ulTemp = 0;
            fwrite(&ulTemp, 4, 1, pFile);
        }
    }

    //
    // Here, the glyph offset table is written.  Now write the glyph data
    // itself.  Loop through each glyph and write the glyph data.
    //
    iCount = 0;
    for(iGlyph = 0, ulOffset = 0; iGlyph < pFont->ulNumGlyphs; iGlyph++)
    {
        //
        // Does this glyph have any data?
        //
        if(pGlyphStart[iGlyph].pucChar)
        {
            //
            // Yes - write it to the file.
            //
            fwrite(pGlyphStart[iGlyph].pucChar, 1,
                   pGlyphStart[iGlyph].pucChar[0], pFile);

            //
            // Update our offset.
            //
            ulOffset += pGlyphStart[iGlyph].pucChar[0];
        }
    }

    //
    // At this point, the output is complete.
    //
    return(true);
}

//*****************************************************************************
//
// Write a binary file containing the font whose glyph data is passed.
//
//*****************************************************************************
tBoolean WriteBinaryWideFont(tConversionParameters *pParams, tFontInfo *pFont,
        tGlyph *pGlyphStart, tCodepointBlock *pBlockList)
{
    tCodepointBlock *pBlock;
    tGlyph *pGlyph, *pGlyphBlockStart;
    int iX, iOpt, iFontSize, iGlyph, iLoop, iCount;
    unsigned long ulOffset, ulTemp;
    char pucChar[512];
    char pucSize[8];
    char *pcCapFilename;
    FILE *pFile;
    tFontWide sFont;
    tFontBlock sBlock;

    if(pParams->bVerbose)
    {
        printf("Writing binary format output file.\n");
    }

    //
    // Convert the filename to all lower case letters.
    //
    for(iX = 0; iX < strlen(pParams->pcFilename); iX++)
    {
        if((pParams->pcFilename[iX] >= 'A') && (pParams->pcFilename[iX] <= 'Z'))
        {
            pParams->pcFilename[iX] += 0x20;
        }
    }

    //
    // Copy the filename and capitalize the first character.
    //
    pcCapFilename = malloc(strlen(pParams->pcFilename) + 1);
    strcpy(pcCapFilename, pParams->pcFilename);
    pcCapFilename[0] -= 0x20;

    //
    // Format the size string.
    //
    if(pParams->bFixedSize)
    {
        //
        // This font is defined by a fixed pixel size.
        //
        sprintf(pucSize, "%dx%d", pParams->iFixedX, pParams->iFixedY);
    }
    else
    {
        //
        // This font is defined by point size.
        //
        sprintf(pucSize, "%dpt", pParams->iSize);
    }

    //
    // Open the file to which the compressed font will be written.
    //
    sprintf(pucChar, "font%s%s%s%s.bin", pParams->pcFilename, pucSize,
            pParams->bBold ? "b" : "", pParams->bItalic ? "i" : "");

    if(pParams->bVerbose)
    {
        printf("Output file name is %s\n", pucChar);
    }

    //
    // Open the output file for binary writing.
    //
    pFile = fopen(pucChar, "wb");

    //
    // Get the total size of the compressed font data.
    //
    for(iX = 0, iOpt = 0; iX < pFont->ulNumGlyphs; iX++)
    {
        //
        // Does this glyph have any encoded data?
        //
        if(pGlyphStart[iX].pucChar)
        {
            //
            // Yes - update our size.
            //
            iOpt += pGlyphStart[iX].pucChar[0];
        }
    }

    //
    // The total size of the font includes the headers, block table and
    // glyph offset tables.
    //
    iFontSize = iOpt + sizeof(tFontWide) + (sizeof(tFontBlock)
            * pFont->usNumBlocks) + (sizeof(tFontOffsetTable)
            * pFont->ulNumGlyphs);

    if(pParams->bVerbose)
    {
        printf("Font contains %d blocks and %d glyphs.\n", pFont->usNumBlocks,
                pFont->ulNumGlyphs);
        printf("%d bytes of glyph data, %d bytes total size.\n", iOpt,
                iFontSize);
    }

    //
    // Write the tFontWide header fields.
    //
    sFont.ucFormat = FONT_FMT_WIDE_PIXEL_RLE;
    sFont.ucMaxWidth = pFont->iWidth;
    sFont.ucHeight = pFont->iYMin - pFont->iYMax + 1;
    sFont.ucBaseline = pFont->iYMin;
    sFont.usCodepage = pFont->usCodePage;
       sFont.usNumBlocks = pParams->bRemap ? 1 : pFont->usNumBlocks;
    fwrite(&sFont, sizeof(tFontWide), 1, pFile);

    //
    // Write the font block table next.  Are we remapping the font glyphs?
    //
    if(pParams->bRemap)
    {
        //
        // We are remapping so write the appropriate header for a single
        // block of characters.
        //
        WriteRemappedBinaryBlocks(pFile, pFont, pGlyphStart, pBlockList);
    }
    else
    {
        //
        // We are not remapping so write the normal header containing all the
        // blocks described in the block list.
        //
        WriteBinaryBlocks(pFile, pFont, pGlyphStart, pBlockList);
    }

    //
    // Close the output file.
    //
    fclose(pFile);

    //
    // Free assorted buffers.
    //
    free(pcCapFilename);

    //
    // Tell the caller we all is fine.
    //
    return (true);
}

//*****************************************************************************
//
// Insert the copyright information at the top of the text output file.
//
//*****************************************************************************
tBoolean WriteCopyrightBlock(tConversionParameters *pParams, char *pcSize,
                             char *pcCapFilename, FILE *pFile)
{
    FILE *pCopyright;
    char pcBuffer[256];

    //
    // Has a copyright file been provided?
    //
    if(!pParams->pcCopyrightFile)
    {
        //
        // No - just write the default TI copyright block.
        //
        fprintf(pFile, "//********************************************************"
            "*********************\n");
        fprintf(pFile, "//\n");
        fprintf(pFile, "// This file is generated by ftrasterize; DO NOT EDIT BY "
            "HAND!\n");
        fprintf(pFile, "//\n");
        fprintf(pFile, "//********************************************************"
            "*********************\n");
    }
    else
    {
        //
        // Yes - copy the text from the copyright file into the output file as
        // a C comment block.
        //

        //
        // Open the copyright file.
        //
        pCopyright = fopen(pParams->pcCopyrightFile, "r");
        if(!pCopyright)
        {
            printf("ERROR: Can't open copyright header %s!\n",
                   pParams->pcCopyrightFile);
            return(false);
        }

        //
        // Output the basic header.
        //
        fprintf(pFile, "//********************************************************"
            "*********************\n");
        fprintf(pFile, "//\n");

        //
        // Output the copyright information from the file.
        //
        while(fgets(pcBuffer, 256, pCopyright))
        {
            fprintf(pFile, "// %s", pcBuffer);
        }

        fprintf(pFile, "//********************************************************"
            "*********************\n\n");

        fprintf(pFile, "//********************************************************"
            "*********************\n");
        fprintf(pFile, "//\n");
        fprintf(pFile, "// This file is generated by ftrasterize; DO NOT EDIT BY "
            "HAND!\n");
        fprintf(pFile, "//\n");
        fprintf(pFile, "//********************************************************"
            "*********************\n");
    }
}

//*****************************************************************************
//
// Write the block table and glyph data for an ASCII font whose glyphs are
// being remapped.
//
//*****************************************************************************
tBoolean WriteRemappedASCIIBlocks(FILE *pFile, tFontInfo *pFont,
        tGlyph *pGlyphStart, tCodepointBlock *pBlockList)
{
    tCodepointBlock *pBlock;
    unsigned long ulOffset;
    int iGlyph, iCount, iLoop;

    //
    // The first glyph offset table will appear immediately after the block
    // table. Since we're remapping, there is only a single block in the output
    // font.
    //
    ulOffset = sizeof(tFontWide) + sizeof(tFontBlock);

    //
    // Generate the block table entry for the single remapped block.
    //
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // Block header 1: Codepoints 0x0001 - 0x%0x\n",
            pFont->ulNumGlyphs);
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
            LONG_PARAMS(1));
    fprintf(pFile, "    0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
            LONG_PARAMS(pFont->ulNumGlyphs));
    fprintf(pFile, "    0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
            LONG_PARAMS(ulOffset));
    fprintf(pFile, "\n");

    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // Block 1 Offsets: Codepoints 0x0001 - 0x%0x\n",
            pFont->ulNumGlyphs);
    fprintf(pFile, "    //\n");

    //
    // The offset to the first glyph data is just the size of the offset
    // table.
    //
    ulOffset = pFont->ulNumGlyphs * 4;

    //
    // Loop through each glyph in the block and write the glyph offset
    // table.
    //
    for(iGlyph = 0; iGlyph < pFont->ulNumGlyphs; iGlyph++)
    {
        //
        // If this glyph has data, write the offset to the table.
        //
        if(pGlyphStart[iGlyph].pucChar)
        {
            fprintf(pFile, "    0x%02x, 0x%02x, 0x%02x, 0x%02x,   "
                "// Offset %d (0x%x)\n", LONG_PARAMS(ulOffset),ulOffset,
                    ulOffset) ;

            //
            // Update the offset to leave space for this glyph's data.
            //
            ulOffset += pGlyphStart[iGlyph].pucChar[0];
        }
        else
        {
            fprintf(pFile, "    0x%02x, 0x%02x, 0x%02x, 0x%02x,   "
                "// Glyph Absent\n", LONG_PARAMS(0));
        }
    }

    //
    // Here, the glyph offset table is written.  Now write the glyph data
    // itself.
    //
    fprintf(pFile, "\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // Block 1 Data: Codepoints 1 - 0x%0x\n",
            pFont->ulNumGlyphs);
    fprintf(pFile, "    //");

    //
    // Loop through each glyph and write the glyph data.
    //
    iCount = 0;
    for(iGlyph = 0, ulOffset = 0; iGlyph < pFont->ulNumGlyphs; iGlyph++)
    {
        //
        // Does this glyph have any data?
        //
        if(pGlyphStart[iGlyph].pucChar)
        {
            ulOffset += pGlyphStart[iGlyph].pucChar[0];
            for(iLoop = 0; iLoop < pGlyphStart[iGlyph].pucChar[0]; iLoop++)
            {
                if(!(iCount % 12))
                {
                    fprintf(pFile, "\n    ");
                }
                fprintf(pFile, "%3d, ", pGlyphStart[iGlyph].pucChar[iLoop]);
                iCount++;
            }
        }
    }

    //
    // At this point, the output is complete.
    //
    fprintf(pFile, "\n");
}

//*****************************************************************************
//
// Write the block tables and glyph data for an ASCII font whose glyphs are not
// being remapped.
//
//*****************************************************************************
tBoolean WriteASCIIBlocks(FILE *pFile, tFontInfo *pFont,
        tGlyph *pGlyphStart, tCodepointBlock *pBlockList)
{
    tCodepointBlock *pBlock;
    tGlyph *pGlyph, *pGlyphBlockStart;
    unsigned long ulOffset;
    int iX, iLoop, iGlyph, iCount;

    //
    // The first glyph offset table will appear immediately after the block
    // table.
    //
    ulOffset = sizeof(tFontWide) + (pFont->usNumBlocks * sizeof(tFontBlock));

    //
    // Loop through each of the blocks.
    //
    for(iX = 0, pBlock = pBlockList, pGlyph = pGlyphStart; iX
            < pFont->usNumBlocks; iX++)
    {
        //
        // Generate the block table entry for this block.
        //
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // Block header %d: Codepoints 0x%0x - 0x%0x\n",
                iX, pBlock->ulStart, pBlock->ulEnd);
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
                LONG_PARAMS(pBlock->ulStart));
        fprintf(pFile, "    0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
                LONG_PARAMS((pBlock->ulEnd - pBlock->ulStart) + 1));
        fprintf(pFile, "    0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
                LONG_PARAMS(ulOffset));
        fprintf(pFile, "\n");

        //
        // Update the offset to the next block's glyph table.  First adjust
        // for the size of this block's glyph offset table.
        //
        ulOffset += ((pBlock->ulEnd - pBlock->ulStart) + 1) * 4;

        //
        // Now walk through the characters in this block adding up the
        // compressed size of each.
        //
        for(iGlyph = 0; iGlyph < ((pBlock->ulEnd - pBlock->ulStart) + 1); iGlyph++)
        {
            //
            // Check that the glyph has the expected codepoint number.  If this
            // fails, this indicates a bug in this application.
            //
            if(pGlyph->ulCodePoint != pBlock->ulStart + iGlyph)
            {
                fprintf(stderr, "Error: Expected codepoint 0x%x but glyph is "
                    "for 0x%x!\n", pBlock->ulStart + iGlyph,
                        pGlyph->ulCodePoint);
            }

            //
            // If this glyph has data, add its size to the tally.
            //
            if(pGlyph->pucChar)
            {
                ulOffset += pGlyph->pucChar[0];
            }

            //
            // Move on to the next glyph.
            //
            pGlyph++;
        }

        //
        // Round up to the next multiple of 4 since we always start a new
        // glyph offset table on a 4 byte boundary.
        //
        ulOffset = (ulOffset + 3) & ~3;

        //
        // Move to the next block.
        //
        pBlock = pBlock->pNext;
    }


    //
    // At this point, we have written all the font block headers. Now we move
    // on to the glyph data for each block.  This comprises a glyph offset
    // table followed by the compressed data for each glyph in the block.  In
    // these sections, the offsets are relative to the first entry in the
    // glyph offset table (since this makes it a lot easier to copy blocks of
    // glyph data between fonts).
    //

    //
    // Loop through each of the blocks again.
    //
    for(iX = 0, pBlock = pBlockList, pGlyph = pGlyphStart; iX
            < pFont->usNumBlocks; iX++)
    {
        //
        // Remember the pointer to the first glyph for this block.
        //
        pGlyphBlockStart = pGlyph;

        //
        // Generate the glyph offset table for this block.
        //
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // Block %d Offsets: Codepoints 0x%0x - 0x%0x\n",
                iX, pBlock->ulStart, pBlock->ulEnd);
        fprintf(pFile, "    //\n");

        //
        // The offset to the first glyph data is just the size of the offset
        // table.
        //
        ulOffset = ((pBlock->ulEnd - pBlock->ulStart) + 1) * 4;

        //
        // Loop through each glyph in the block and write the glyph offset
        // table.
        //
        for(iGlyph = 0; iGlyph < ((pBlock->ulEnd - pBlock->ulStart) + 1); iGlyph++)
        {
            //
            // Check that the glyph has the expected codepoint number.  If this
            // fails, this indicates a bug in this application.
            //
            if(pGlyph->ulCodePoint != pBlock->ulStart + iGlyph)
            {
                fprintf(stderr, "Error: Exected codepoint 0x%x but glyph is "
                    "for 0x%x!\n", pBlock->ulStart + iGlyph,
                        pGlyph->ulCodePoint);
            }

            //
            // If this glyph has data, write the offset to the table.
            //
            if(pGlyph->pucChar)
            {
                fprintf(pFile, "    0x%02x, 0x%02x, 0x%02x, 0x%02x,   "
                    "// Offset %d (0x%x)\n", LONG_PARAMS(ulOffset),ulOffset,
                        ulOffset) ;

                //
                // Update the offset to leave space for this glyph's data.
                //
                ulOffset += pGlyph->pucChar[0];
            }
            else
            {
                fprintf(pFile, "    0x%02x, 0x%02x, 0x%02x, 0x%02x,   "
                    "// Glyph Absent\n", LONG_PARAMS(0));
            }

            //
            // Move on to the next glyph.
            //
            pGlyph++;
        }

        //
        // Here, the glyph offset table is written.  Now write the glyph data
        // itself.
        //
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // Block %d Data: Codepoints 0x%0x - 0x%0x\n", iX,
                pBlock->ulStart, pBlock->ulEnd);
        fprintf(pFile, "    //");

        //
        // Loop through each glyph in the block and write the glyph offset
        // table.
        //
        iCount = 0;
        for(iGlyph = 0, pGlyph = pGlyphBlockStart, ulOffset = 0; iGlyph
                < ((pBlock->ulEnd - pBlock->ulStart) + 1); iGlyph++)
        {
            //
            // Does this glyph have any data?
            //
            if(pGlyph->pucChar)
            {
                ulOffset += pGlyph->pucChar[0];
                for(iLoop = 0; iLoop < pGlyph->pucChar[0]; iLoop++)
                {
                    if(!(iCount % 12))
                    {
                        fprintf(pFile, "\n    ");
                    }
                    fprintf(pFile, "%3d, ", pGlyph->pucChar[iLoop]);
                    iCount++;
                }
            }

            //
            // Move on to the next glyph.
            //
            pGlyph++;
        }

        //
        // At this point, the block is complete but we may need to add some
        // padding bytes to get us to the next 4 byte boundary in the file.
        //
        if(ulOffset % 4)
        {
            fprintf(pFile, "\n    ");
            while(ulOffset % 4)
            {
                fprintf(pFile, "  0, ");
                ulOffset++;
            }
            fprintf(pFile, "  // Padding\n\n");
        }
        else
        {
            fprintf(pFile, "\n");
        }

        //
        // Move to the next block.
        //
        pBlock = pBlock->pNext;
    }
}

//*****************************************************************************
//
// Write an ASCII (C) file containing the font whose glyph data is passed.
//
//*****************************************************************************
tBoolean WriteASCIIWideFont(tConversionParameters *pParams, tFontInfo *pFont,
        tGlyph *pGlyphStart, tCodepointBlock *pBlockList)
{
    tCodepointBlock *pBlock;
    tGlyph *pGlyph, *pGlyphBlockStart;
    int iX, iOpt, iFontSize, iGlyph, iLoop, iCount;
    unsigned long ulOffset;
    char pucChar[512];
    char pucSize[8];
    char *pcCapFilename;
    FILE *pFile;
    tBoolean bRetcode;

    if(pParams->bVerbose)
    {
        printf("Writing ASCII format output file.\n");
    }

    //
    // Convert the filename to all lower case letters.
    //
    for(iX = 0; iX < strlen(pParams->pcFilename); iX++)
    {
        if((pParams->pcFilename[iX] >= 'A') && (pParams->pcFilename[iX] <= 'Z'))
        {
            pParams->pcFilename[iX] += 0x20;
        }
    }

    //
    // Copy the filename and capitalize the first character.
    //
    pcCapFilename = malloc(strlen(pParams->pcFilename) + 1);
    strcpy(pcCapFilename, pParams->pcFilename);
    pcCapFilename[0] -= 0x20;

    //
    // Format the size string.
    //
    if(pParams->bFixedSize)
    {
        //
        // This font is defined by a fixed pixel size.
        //
        sprintf(pucSize, "%dx%d", pParams->iFixedX, pParams->iFixedY);
    }
    else
    {
        //
        // This font is defined by point size.
        //
        sprintf(pucSize, "%dpt", pParams->iSize);
    }

    //
    // Open the file to which the compressed font will be written.
    //
    sprintf(pucChar, "font%s%s%s%s.c", pParams->pcFilename, pucSize,
            pParams->bBold ? "b" : "", pParams->bItalic ? "i" : "");

    if(pParams->bVerbose)
    {
        printf("Output file name is %s\n", pucChar);
    }

    pFile = fopen(pucChar, "w");
    if(!pFile)
    {
        printf("Unable to open output file %s!\n", pucChar);
        return(false);
    }

    //
    // Write the header to the output file.
    //
    bRetcode = WriteCopyrightBlock(pParams, pucSize, pcCapFilename, pFile);
    if(!bRetcode)
    {
        printf("Unable to write file header information to %s!\n", pucChar);
        return(false);
    }
    fprintf(pFile, "\n");
    fprintf(pFile, "#include \"grlib/grlib.h\"\n");
    fprintf(pFile, "\n");

    //
    // Get the total size of the compressed font data.
    //
    for(iX = 0, iOpt = 0; iX < pFont->ulNumGlyphs; iX++)
    {
        //
        // Does this glyph have any encoded data?
        //
        if(pGlyphStart[iX].pucChar)
        {
            //
            // Yes - update our size.
            //
            iOpt += pGlyphStart[iX].pucChar[0];
        }
    }

    //
    // The total size of the font includes the headers, block table and
    // glyph offset tables.
    //
    if(!pParams->bRemap)
    {
        //
        // We are not remapping the font codepage so calculate the size taking
        // into account the number of blocks of glyphs from the source font that
        // we are encoding.
        //
        iFontSize = iOpt + sizeof(tFontWide) +
                    (sizeof(tFontBlock) * pFont->usNumBlocks) +
                    (sizeof(tFontOffsetTable) * pFont->ulNumGlyphs);
    }
    else
    {
        //
        // We are remapping the source font glyphs to a new codepage in a
        // single block so calculate the size accordingly.
        //
        iFontSize = iOpt + sizeof(tFontWide) + sizeof(tFontBlock) +
                    (sizeof(tFontOffsetTable) * pFont->ulNumGlyphs);
    }

    if(pParams->bVerbose)
    {
        printf("Font contains %d blocks and %d glyphs.\n", pFont->usNumBlocks,
                pFont->ulNumGlyphs);
        printf("%d bytes of glyph data, %d bytes total size.\n", iOpt,
                iFontSize);
        if(pParams->bRemap)
        {
            printf("Remapping font to a single block with codepage 0x%04x.\n",
                    pFont->usCodePage);
        }
    }

    //
    // Write the font details to the output file.  The memory usage of the font
    // is a close approximation that may vary based on the compiler used and
    // the compiler options.
    //
    fprintf(pFile, "//********************************************************"
        "*********************\n");
    fprintf(pFile, "//\n");
    fprintf(pFile, "// Details of this font:\n");
    fprintf(pFile, "//     Characters: %d in %d blocks\n", pFont->ulNumGlyphs,
            pFont->usNumBlocks);
    fprintf(pFile, "//     Style: %s\n", pParams->pcFilename);
    fprintf(pFile, "//     Size: %s\n", pucSize);
    fprintf(pFile, "//     Bold: %s\n", pParams->bBold ? "yes" : "no");
    fprintf(pFile, "//     Italic: %s\n", pParams->bItalic ? "yes" : "no");
    fprintf(pFile, "//     Memory usage: %d bytes\n", iFontSize);
    if(pParams->bRemap)
    {
        fprintf(pFile, "//     Glyphs remapped. Custom codepage 0x%04x.\n",
                pFont->usCodePage);
    }
    fprintf(pFile, "//\n");
    fprintf(pFile, "//********************************************************"
        "*********************\n");
    fprintf(pFile, "\n");

    //
    // Write the font information.  Although we define various types in grlib.h
    // to describe the various blocks within the font, the data as a whole needs
    // to be contiguous so we define it in terms of a single character array.
    // All multi-byte fields are, of course, little endian.
    //

    fprintf(pFile, "const unsigned char g_puc%s%s%s%s[] =\n{\n", pcCapFilename,
            pucSize, pParams->bBold ? "b" : "", pParams->bItalic ? "i" : "");

    //
    // Write the tFontWide header fields.
    //
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // The format of the font.\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    FONT_FMT_WIDE_PIXEL_RLE,\n");
    fprintf(pFile, "\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // The maximum width of the font.\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    %d,\n", pFont->iWidth);
    fprintf(pFile, "\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // The height of the font.\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    %d,\n", pFont->iYMin - pFont->iYMax + 1);
    fprintf(pFile, "\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // The baseline of the font.\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    %d,\n", pFont->iYMin);
    fprintf(pFile, "\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // The font codepage (%s).\n", GetCodepageName(
            pFont->usCodePage));
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    %d, %d,\n", SHORT_PARAMS(pFont->usCodePage));
    fprintf(pFile, "\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // The number of blocks of characters (%d).\n",
            pParams->bRemap ? 1 : pFont->usNumBlocks);
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    %d, %d,\n",
            SHORT_PARAMS(pParams->bRemap ? 1 : pFont->usNumBlocks));
    fprintf(pFile, "\n");

    //
    // Write the font block table next.  Are we remapping the font glyphs?
    //
    if(pParams->bRemap)
    {
        //
        // We are remapping so write the appropriate header for a single
        // block of characters.
        //
        WriteRemappedASCIIBlocks(pFile, pFont, pGlyphStart, pBlockList);
    }
    else
    {
        //
        // We are not remapping so write the normal header containing all the
        // blocks described in the block list.
        //
        WriteASCIIBlocks(pFile, pFont, pGlyphStart, pBlockList);
    }

    //
    // Close the array definition and include a variable with an appropriate
    // type cast to allow easy use of GrStringDraw.
    //
    fprintf(pFile, "};\n\n");
    fprintf(pFile, "tFont *g_psFontW%s%s%s%s = (tFont *)g_puc%s%s%s%s;\n",
            pcCapFilename, pucSize, pParams->bBold ? "b" : "",
            pParams->bItalic ? "i" : "", pcCapFilename, pucSize,
            pParams->bBold ? "b" : "", pParams->bItalic ? "i" : "");

    //
    // Close the output file.
    //
    fclose(pFile);

    //
    // Free assorted buffers.
    //
    free(pcCapFilename);

    //
    // Tell the caller we all is fine.
    //
    return (true);
}

//*****************************************************************************
//
// Open a single font, select the required character size and codepage.
//
// Returns 0 on failure of a font face handle on success.
//
//*****************************************************************************
FT_Face
InitializeFont(tConversionParameters *pParams, const char *pcFontName,
        FT_Library pLibrary, unsigned short *pusCodepage)
{
    FT_Face pFace;
    int iRetcode;

    //
    // Tell the user what we're doing.
    //
    if(pParams->bVerbose)
    {
        fprintf(stdout, "Opening font %s...\n", pcFontName);
    }

    //
    // Load the font file into the FreeType library.
    //
    if(FT_New_Face(pLibrary, pcFontName, 0, &pFace) != 0)
    {
        fprintf(stderr, "%s: Unable to open font file '%s'\n",
                pParams->pcAppName, pcFontName);
        return (0);
    }

    //
    // Set the character size we are to render.  This will be set either by
    // point size or as an index into the font's fixed size list.
    //
    iRetcode = SetFontCharSize(pFace, pParams);
    if(!iRetcode)
    {
        //
        // An error was reported so clean up and return.
        //
        FT_Done_Face(pFace);
        return (0);
    }

    //
    // Did the user specify a specific character map on the command line?
    //
    if(pParams->iCharMap != -1)
    {
        if(pParams->bVerbose)
        {
            fprintf(stdout, "User-provided character map.\n");
        }

        //
        // Yes - check that we've been passed a valid charmap index.
        //
        if(pParams->iCharMap >= pFace->num_charmaps)
        {
            //
            // The character map index is invalid.
            //
            fprintf(stderr, "%s: Error - invalid character map index (%d). "
                "Valid values are < %d.\n", pParams->pcAppName,
                    pParams->iCharMap, pFace->num_charmaps);

            //
            // Clean up and return.
            //
            FT_Done_Face(pFace);
            return (0);
        }
        else
        {
            //
            // The character map index is valid so select it.
            //
            FT_Set_Charmap(pFace, pFace->charmaps[pParams->iCharMap]);

            //
            // Set the codepage that we will use to identify the font.  Note
            // that this will be overridden later if we've been asked to remap
            // the codepoints in the generated font.
            //
            if(pusCodepage)
            {
                *pusCodepage = CodePageFromEncoding(
                        pFace->charmaps[pParams->iCharMap]->encoding);
            }
        }
    }
    else
    {
        if(pParams->bVerbose && !pParams->bUnicode)
        {
            printf("No character map specified. Defaulting to Unicode.\n");
        }

        //
        // The user didn't specify a specific character map so pick Unicode if
        // it's available.
        //
        if(FT_Select_Charmap(pFace, FT_ENCODING_UNICODE) != 0)
        {
            //
            // Unicode isn't available so pick the first encoding that the
            // font offers.
            //
            FT_Set_Charmap(pFace, pFace->charmaps[0]);
            fprintf(stderr, "%s: Warning - Font has no Unicode charmap! "
                "Using first mapping (%s) instead.\n", GetCharmapName(
                    pFace->charmaps[0]));
            if(pusCodepage)
            {
                *pusCodepage = CodePageFromEncoding(
                            pFace->charmaps[0]->encoding);
            }
        }
        else
        {
            //
            // Get the output font codepage identifier for Unicode.
            //
            if(pusCodepage)
            {
                *pusCodepage = CodePageFromEncoding(FT_ENCODING_UNICODE);
            }
        }
    }

    //
    // If we get here, all is well so return the handle.
    //
    return(pFace);
}

//*****************************************************************************
//
// Generate a font file containing wide character set (typically Unicode)
// characters.
//
//*****************************************************************************
int ConvertWideFont(tConversionParameters *pParams)
{
    tCodepointBlock sBlockListHead;
    tCodepointBlock *pBlock;
    unsigned long ulGlyphIndex, ulCodePoint;
    int iRetcode, iXMin, iXMax, iLoop;
    tBoolean bRendered, bRetcode;
    FT_Library pLibrary;
    FT_GlyphSlot pSlot;
    FT_Bitmap *ppMap;
    FT_Encoding eEncoding;
    tGlyph *pGlyph;
    FT_Face ppFace[MAX_FONTS];
    tFontInfo sFontInfo;

    if(pParams->bVerbose)
    {
        printf("Generating wide format output.\n");
    }

    sFontInfo.iWidth = 0;
    sFontInfo.iYMin = 0;
    sFontInfo.iYMax = 0;
    sFontInfo.ulNumGlyphs = 0;
    sFontInfo.usCodePage = CODEPAGE_UNICODE;
    sFontInfo.usNumBlocks = 0;

    //
    // If we were passed a character map file, parse it and set up the
    // block list.
    //
    if(pParams->pcCharFile)
    {
        //
        // Build the linked list of blocks and determine the number of
        // codepoint blocks that we will need to read and encode.
        //
        if(pParams->bVerbose)
        {
            printf("Parsing character map from %s\n", pParams->pcCharFile);
        }

        sFontInfo.usNumBlocks = ParseCharMapFile(pParams, &sBlockListHead);

        if(pParams->bVerbose)
        {
            tCodepointBlock *pBlock;

            pBlock = &sBlockListHead;
            while(pBlock)
            {
                printf("    0x%06x - 0x%06x\n", pBlock->ulStart, pBlock->ulEnd);
                pBlock = pBlock->pNext;
            }
        }
    }
    else
    {
        //
        // We've not been passed a character map file so initialize the
        // codepoint list with the first and last codepoints as provided on
        // the command line (or the defaults).
        //
        sBlockListHead.pNext = (tCodepointBlock *) 0;
        sBlockListHead.ulStart = (unsigned long) pParams->iFirst;
        sBlockListHead.ulEnd = (unsigned long) pParams->iLast;

        //
        // Make sure we've been passed sensible values.
        //
        if((pParams->iFirst < 0) || (pParams->iFirst > pParams->iLast))
        {
            //
            // Someone passed a bogus start or end codepoint number.
            //
            fprintf(stderr, "%s: Start and end character numbers are invalid!");
            sFontInfo.usNumBlocks = 0;
        }
        else
        {
            //
            // All is well.
            //
            sFontInfo.usNumBlocks = 1;
        }
    }

    //
    // If something was wrong with the character map we were asked to encode,
    // return at this point.
    //
    if(!sFontInfo.usNumBlocks)
    {
        return (1);
    }

    //
    // Provide some information if we've been asked for verbose output.
    //
    if(pParams->bVerbose)
    {
        printf("Processing %d blocks of characters from font.\n",
                sFontInfo.usNumBlocks);
    }

    //
    // We have the character map read.  Now calculate the number of glyphs we
    // need to read.
    //
    pBlock = &sBlockListHead;
    while(pBlock)
    {
        //
        // Add the number of glyphs in this block to our running total.
        //
        sFontInfo.ulNumGlyphs += (pBlock->ulEnd - pBlock->ulStart) + 1;
        pBlock = pBlock->pNext;
    }

    if(pParams->bVerbose)
    {
        fprintf(stdout, "Encoding %d characters.\n", sFontInfo.ulNumGlyphs);
    }

    //
    // Initialize the FreeType library.
    //
    if(FT_Init_FreeType(&pLibrary) != 0)
    {
        fprintf(stderr, "%s: Unable to initialize the FreeType library.\n",
                pParams->pcAppName);
        return (1);
    }

    //
    // Load the font files we've been asked use into the FreeType library.
    //
    for(iLoop = 0; iLoop < pParams->iNumFonts; iLoop++)
    {
        //
        // Load the font, set the character size and codepage.  Note that we
        // record the codepage only if this is the first font opened (the main
        // one).
        //
        ppFace[iLoop] = InitializeFont(pParams,
                                       pParams->pcFontInputName[iLoop],
                                       pLibrary,
                                       (iLoop == 0 ? &sFontInfo.usCodePage : 0));
    }

    //
    // Make sure the main font opened.  If any of the fallback fonts didn't
    // open for some reason, just dump a warning.
    //
    if(!ppFace[0])
    {
        fprintf(stderr, "%s: Unable to open main font file '%s'\n",
                pParams->pcAppName, pParams->pcFontInputName[0]);
        FT_Done_FreeType(pLibrary);
        FreeCharMapList(&sBlockListHead);
        return(1);
    }

    //
    // Check and warn if any of the fallback fonts couldn't be opened.
    //
    for(iLoop = 1; iLoop < pParams->iNumFonts; iLoop++)
    {
        if(!ppFace[iLoop])
        {
            fprintf(stderr, "%s: Warning - fallback font '%s' could not be "
                    "initialized.!\n", pParams->pcFontInputName[iLoop]);
        }
    }

    //
    // If the user has provided a custom output codepage identifier, set it
    // here.
    //
    if(pParams->iOutputCodePage != -1)
    {
        sFontInfo.usCodePage = (unsigned short)pParams->iOutputCodePage;
    }

    //
    // Did we find a supported codepage for the font?
    //
    if(sFontInfo.usCodePage == 0xFFFF)
    {
        //
        // We couldn't find a matching output codepage for the character map
        // chosen.  This is considered a fatal error since we have no way to
        // tie up the
        //
        fprintf(stderr, "%s: Error - the chosen font character map doesn't "
            "match any supported\nGrLib font codepage!\n", pParams->pcAppName);

        //
        // Clean up and return.
        //
        FT_Done_Face(ppFace[0]);
        FT_Done_FreeType(pLibrary);
        FreeCharMapList(&sBlockListHead);
        return (1);
    }

    if(pParams->bVerbose)
    {
        printf("Rendering individual character glyphs...\n");
    }

    //
    // Initialize our minimum and maximum Y values for our font.
    //
    sFontInfo.iYMin = 0;
    sFontInfo.iYMax = 0;

    //
    // Allocate storage for all the glyphs we need to render.
    //
    pGlyph = malloc(sizeof(tGlyph) * sFontInfo.ulNumGlyphs);
    if(!pGlyph)
    {
        fprintf(stderr, "%s: Error - can't allocate storage for %d glyphs!\n",
                pParams->pcAppName, sFontInfo.ulNumGlyphs);
    }
    else
    {
        //
        // We allocated storage for all the glyphs we need to render so start
        // rendering them.
        //
        pBlock = &sBlockListHead;
        ulGlyphIndex = 0;

        //
        // Loop through each block of contiguous characters.
        //
        while(pBlock)
        {
            //
            // Loop through each character in the block.
            //
            for(ulCodePoint = pBlock->ulStart; ulCodePoint <= pBlock->ulEnd;
                ulCodePoint++)
            {
                //
                // Render the glyph.  We start by looking in the main font
                // then fall back on the other fonts until we find one which
                // contains the character (assuming any do).
                //
                for(iLoop = 0; iLoop < pParams->iNumFonts; iLoop++)
                {
                    //
                    // If we opened this font successfully...
                    //
                    if(ppFace[iLoop])
                    {
                        //
                        // Try to render the character.
                        //
                        bRendered = RenderGlyph(pParams, ppFace[iLoop],
                                                ulCodePoint,
                                                &pGlyph[ulGlyphIndex], true);

                        //
                        // If we found and rendered the character...
                        //
                        if(bRendered)
                        {
                            //
                            // Exit the loop.
                            //
                            break;
                        }
                    }
                }

                //
                // Did we manage to find and render the glyph?
                //
                if(!bRendered)
                {
                    //
                    // No. Tell the user we failed to find the glyph.
                    //
                    fprintf(stderr, "%s: Warning - can't find a glyph for "
                            "codepoint 0x%x!\n", pParams->pcAppName,
                            ulCodePoint);
                }

                //
                // If this glyph is wider than any previously encountered
                // glyph, then remember its width as the maximum width in
                // this font.
                //
                if((pGlyph[ulGlyphIndex].iMaxX - pGlyph[ulGlyphIndex].iMinX)
                        > sFontInfo.iWidth)
                {
                    sFontInfo.iWidth = (pGlyph[ulGlyphIndex].iMaxX
                            - pGlyph[ulGlyphIndex].iMinX);
                }

                //
                // Update our rolling minimum and maximum Y values for the
                // rendered glyphs.
                //
                //
                // Adjust the minimum Y value if necessary.
                //
                if(pGlyph[ulGlyphIndex].iBitmapTop > sFontInfo.iYMin)
                {
                    sFontInfo.iYMin = pGlyph[ulGlyphIndex].iBitmapTop;
                }

                //
                // Adjust the maximum Y value if necessary.
                //
                if((pGlyph[ulGlyphIndex].iBitmapTop
                        - pGlyph[ulGlyphIndex].iRows + 1) < sFontInfo.iYMax)
                {
                    sFontInfo.iYMax = pGlyph[ulGlyphIndex].iBitmapTop
                            - pGlyph[ulGlyphIndex].iRows + 1;
                }

                //
                // Move on to the next glyph.
                //
                ulGlyphIndex++;
            }

            //
            // Move on to the next block of characters.
            //
            pBlock = pBlock->pNext;
        }
    }

    //
    // We're finished with our fonts and FreeType now so clean up.
    //
    for(iLoop = 0; iLoop < pParams->iNumFonts; iLoop++)
    {
        //
        // If this font slot is occupied...
        //
        if(ppFace[iLoop])
        {
            //
            // Free the font.
            //
            FT_Done_Face(ppFace[iLoop]);
        }
    }
    FT_Done_FreeType(pLibrary);

    if(pParams->bVerbose)
    {
        printf("Compressing glyphs...\n");
    }

    //
    // Loop through each glyph and compress it.  The compressed image is
    // attached to the glyph whose pointer we pass to this function.
    //
    for(ulCodePoint = 0; ulCodePoint < sFontInfo.ulNumGlyphs; ulCodePoint++)
    {
        CompressGlyph(pParams, &pGlyph[ulCodePoint], sFontInfo.iWidth,
                sFontInfo.iYMin, sFontInfo.iYMax);
    }

    if(pParams->bVerbose)
    {
        printf("Finished compressing glyphs.\n");
    }

    //
    // At this point, we have all the character data ready to go so let's write
    // our output file.  Are we to write a binary or ASCII file?
    //
    if(pParams->bBinary)
    {
        //
        // Write a binary font file suitable for storing in a file system.
        //
        bRetcode = WriteBinaryWideFont(pParams, &sFontInfo, pGlyph,
                &sBlockListHead);
    }
    else
    {
        //
        // Write an ASCII (C) font file suitable for building into an
        // application.
        //
        bRetcode = WriteASCIIWideFont(pParams, &sFontInfo, pGlyph,
                &sBlockListHead);
    }

    //
    // Did we write the file successfully?
    //
    if(!bRetcode)
    {
        //
        // No - report the error.
        //
        fprintf(stderr, "%s: Error - failed to write output file!\n",
                pParams->pcAppName);
    }
    else
    {
        //
        // The file was written successfully so, if we're showing verbose
        // output, let the user know.
        //
        if(pParams->bVerbose)
        {
            printf("Output file written successfully.\n");
        }
    }

    //
    // Free our glyph memory.
    //
    FreeGlyphs(pGlyph, sFontInfo.ulNumGlyphs);
    free(pGlyph);

    //
    // Free the character map list.
    //
    FreeCharMapList(&sBlockListHead);

    //
    // If we get here, all is well so report this to the caller.
    //
    return (bRetcode ? 0 : 1);
}


//*****************************************************************************
//
// Show each of the requested glyphs by dumping them using ASCII "X" and " "
// characters on the terminal.
//
//*****************************************************************************
int ShowFontCharacters(tConversionParameters *pParams)
{
    tCodepointBlock sBlockListHead;
    tCodepointBlock *pBlock;
    unsigned long ulGlyphIndex, ulCodePoint;
    int iRetcode, iXMin, iXMax, iLoop;
    tBoolean bRendered, bRetcode;
    FT_Library pLibrary;
    FT_GlyphSlot pSlot;
    FT_Bitmap *ppMap;
    FT_Encoding eEncoding;
    tGlyph *pGlyph;
    FT_Face ppFace[MAX_FONTS];
    tFontInfo sFontInfo;

    if(pParams->bVerbose)
    {
        printf("Generating wide format output.\n");
    }

    sFontInfo.iWidth = 0;
    sFontInfo.iYMin = 0;
    sFontInfo.iYMax = 0;
    sFontInfo.ulNumGlyphs = 0;
    sFontInfo.usCodePage = CODEPAGE_UNICODE;
    sFontInfo.usNumBlocks = 0;

    //
    // If we were passed a character map file, parse it and set up the
    // block list.
    //
    if(pParams->pcCharFile)
    {
        //
        // Build the linked list of blocks and determine the number of
        // codepoint blocks that we will need to read and encode.
        //
        if(pParams->bVerbose)
        {
            printf("Parsing character map from %s\n", pParams->pcCharFile);
        }

        sFontInfo.usNumBlocks = ParseCharMapFile(pParams, &sBlockListHead);

        if(pParams->bVerbose)
        {
            tCodepointBlock *pBlock;

            pBlock = &sBlockListHead;
            while(pBlock)
            {
                printf("    0x%06x - 0x%06x\n", pBlock->ulStart, pBlock->ulEnd);
                pBlock = pBlock->pNext;
            }
        }
    }
    else
    {
        //
        // We've not been passed a character map file so initialize the
        // codepoint list with the first and last codepoints as provided on
        // the command line (or the defaults).
        //
        sBlockListHead.pNext = (tCodepointBlock *) 0;
        sBlockListHead.ulStart = (unsigned long) pParams->iFirst;
        sBlockListHead.ulEnd = (unsigned long) pParams->iLast;

        //
        // Make sure we've been passed sensible values.
        //
        if((pParams->iFirst < 0) || (pParams->iFirst > pParams->iLast))
        {
            //
            // Someone passed a bogus start or end codepoint number.
            //
            fprintf(stderr, "%s: Start and end character numbers are invalid!");
            sFontInfo.usNumBlocks = 0;
        }
        else
        {
            //
            // All is well.
            //
            sFontInfo.usNumBlocks = 1;
        }
    }

    //
    // If something was wrong with the character map we were asked to encode,
    // return at this point.
    //
    if(!sFontInfo.usNumBlocks)
    {
        return (1);
    }

    //
    // Provide some information if we've been asked for verbose output.
    //
    if(pParams->bVerbose)
    {
        printf("Processing %d blocks of characters from font.\n",
                sFontInfo.usNumBlocks);
    }

    //
    // We have the character map read.  Now calculate the number of glyphs we
    // need to read.
    //
    pBlock = &sBlockListHead;
    while(pBlock)
    {
        //
        // Add the number of glyphs in this block to our running total.
        //
        sFontInfo.ulNumGlyphs += (pBlock->ulEnd - pBlock->ulStart) + 1;
        pBlock = pBlock->pNext;
    }

    if(pParams->bVerbose)
    {
        fprintf(stdout, "Encoding %d characters.\n", sFontInfo.ulNumGlyphs);
    }

    //
    // Initialize the FreeType library.
    //
    if(FT_Init_FreeType(&pLibrary) != 0)
    {
        fprintf(stderr, "%s: Unable to initialize the FreeType library.\n",
                pParams->pcAppName);
        return (1);
    }

    //
    // Load the font files we've been asked use into the FreeType library.
    //
    for(iLoop = 0; iLoop < pParams->iNumFonts; iLoop++)
    {
        //
        // Load the font, set the character size and codepage.  Note that we
        // record the codepage only if this is the first font opened (the main
        // one).
        //
        ppFace[iLoop] = InitializeFont(pParams,
                                       pParams->pcFontInputName[iLoop],
                                       pLibrary,
                                       (iLoop == 0 ? &sFontInfo.usCodePage : 0));
    }

    //
    // Make sure the main font opened.  If any of the fallback fonts didn't
    // open for some reason, just dump a warning.
    //
    if(!ppFace[0])
    {
        fprintf(stderr, "%s: Unable to open main font file '%s'\n",
                pParams->pcAppName, pParams->pcFontInputName[0]);
        FT_Done_FreeType(pLibrary);
        FreeCharMapList(&sBlockListHead);
        return(1);
    }

    //
    // Check and warn if any of the fallback fonts couldn't be opened.
    //
    for(iLoop = 1; iLoop < pParams->iNumFonts; iLoop++)
    {
        if(!ppFace[iLoop])
        {
            fprintf(stderr, "%s: Warning - fallback font '%s' could not be "
                    "initialized.!\n", pParams->pcFontInputName[iLoop]);
        }
    }

    //
    // If the user has provided a custom output codepage identifier, set it
    // here.
    //
    if(pParams->iOutputCodePage != -1)
    {
        sFontInfo.usCodePage = (unsigned short)pParams->iOutputCodePage;
    }

    //
    // Did we find a supported codepage for the font?
    //
    if(sFontInfo.usCodePage == 0xFFFF)
    {
        //
        // We couldn't find a matching output codepage for the character map
        // chosen.  This is considered a fatal error since we have no way to
        // tie up the
        //
        fprintf(stderr, "%s: Error - the chosen font character map doesn't "
            "match any supported\nGrLib font codepage!\n", pParams->pcAppName);

        //
        // Clean up and return.
        //
        FT_Done_Face(ppFace[0]);
        FT_Done_FreeType(pLibrary);
        FreeCharMapList(&sBlockListHead);
        return (1);
    }

    if(pParams->bVerbose)
    {
        printf("Rendering individual character glyphs...\n");
    }

    //
    // Initialize our minimum and maximum Y values for our font.
    //
    sFontInfo.iYMin = 0;
    sFontInfo.iYMax = 0;

    //
    // Allocate storage for all the glyphs we need to render.
    //
    pGlyph = malloc(sizeof(tGlyph) * sFontInfo.ulNumGlyphs);
    if(!pGlyph)
    {
        fprintf(stderr, "%s: Error - can't allocate storage for %d glyphs!\n",
                pParams->pcAppName, sFontInfo.ulNumGlyphs);
    }
    else
    {
        //
        // We allocated storage for all the glyphs we need to render so start
        // rendering them.
        //
        pBlock = &sBlockListHead;
        ulGlyphIndex = 0;

        //
        // Loop through each block of contiguous characters.
        //
        while(pBlock)
        {
            //
            // Loop through each character in the block.
            //
            for(ulCodePoint = pBlock->ulStart; ulCodePoint <= pBlock->ulEnd;
                ulCodePoint++)
            {
                //
                // Render the glyph.  We start by looking in the main font
                // then fall back on the other fonts until we find one which
                // contains the character (assuming any do).
                //
                for(iLoop = 0; iLoop < pParams->iNumFonts; iLoop++)
                {
                    //
                    // If we opened this font successfully...
                    //
                    if(ppFace[iLoop])
                    {
                        //
                        // Try to render the character.
                        //
                        bRendered = RenderGlyph(pParams, ppFace[iLoop],
                                                ulCodePoint,
                                                &pGlyph[ulGlyphIndex], true);

                        //
                        // If we found and rendered the character...
                        //
                        if(bRendered)
                        {
                            //
                            // Exit the loop.
                            //
                            break;
                        }
                    }
                }

                //
                // Did we manage to find and render the glyph?
                //
                if(!bRendered)
                {
                    //
                    // No. Tell the user we failed to find the glyph.
                    //
                    fprintf(stderr, "%s: Warning - can't find a glyph for "
                            "codepoint 0x%x!\n", pParams->pcAppName,
                            ulCodePoint);
                }

                //
                // If this glyph is wider than any previously encountered
                // glyph, then remember its width as the maximum width in
                // this font.
                //
                if((pGlyph[ulGlyphIndex].iMaxX - pGlyph[ulGlyphIndex].iMinX)
                        > sFontInfo.iWidth)
                {
                    sFontInfo.iWidth = (pGlyph[ulGlyphIndex].iMaxX
                            - pGlyph[ulGlyphIndex].iMinX);
                }

                //
                // Update our rolling minimum and maximum Y values for the
                // rendered glyphs.
                //
                //
                // Adjust the minimum Y value if necessary.
                //
                if(pGlyph[ulGlyphIndex].iBitmapTop > sFontInfo.iYMin)
                {
                    sFontInfo.iYMin = pGlyph[ulGlyphIndex].iBitmapTop;
                }

                //
                // Adjust the maximum Y value if necessary.
                //
                if((pGlyph[ulGlyphIndex].iBitmapTop
                        - pGlyph[ulGlyphIndex].iRows + 1) < sFontInfo.iYMax)
                {
                    sFontInfo.iYMax = pGlyph[ulGlyphIndex].iBitmapTop
                            - pGlyph[ulGlyphIndex].iRows + 1;
                }

                //
                // Move on to the next glyph.
                //
                ulGlyphIndex++;
            }

            //
            // Move on to the next block of characters.
            //
            pBlock = pBlock->pNext;
        }
    }

    //
    // We're finished with our fonts and FreeType now so clean up.
    //
    for(iLoop = 0; iLoop < pParams->iNumFonts; iLoop++)
    {
        //
        // If this font slot is occupied...
        //
        if(ppFace[iLoop])
        {
            //
            // Free the font.
            //
            FT_Done_Face(ppFace[iLoop]);
        }
    }
    FT_Done_FreeType(pLibrary);

    if(pParams->bVerbose)
    {
        printf("Displaying glyphs...\n");
    }

    //
    // Loop through each glyph and display it.
    //
    for(ulCodePoint = 0; ulCodePoint < sFontInfo.ulNumGlyphs; ulCodePoint++)
    {
        DisplayGlyph(pParams, &pGlyph[ulCodePoint], sFontInfo.iWidth,
                sFontInfo.iYMin, sFontInfo.iYMax);
    }

    if(pParams->bVerbose)
    {
        printf("Finished displaying glyphs.\n");
    }

    //
    // Free our glyph memory.
    //
    FreeGlyphs(pGlyph, sFontInfo.ulNumGlyphs);
    free(pGlyph);

    //
    // Free the character map list.
    //
    FreeCharMapList(&sBlockListHead);

    //
    // If we get here, all is well so report this to the caller.
    //
    return (1);
}

//*****************************************************************************
//
// Generate a font file containing an 8 bit character set (e.g. an ISO8859
// variant).
//
//*****************************************************************************
int ConvertNarrowFont(tConversionParameters *pParams)
{
    int iOpt, iXMin, iYMin, iXMax, iYMax, iX, iY, iWidth;
    int iPrevBit, iBit, iZero, iOne;
    int iTranslateStart, iTranslateSource;
    int iDisplayFont, iWideFont, iIndex, iNewStruct;
    char *pcCapFilename;
    unsigned char pucChar[512];
    char pucSize[8];
    FT_UInt uiChar, uiSrcChar, uiIndex;
    FT_Library pLibrary;
    FT_GlyphSlot pSlot;
    FT_Bitmap *ppMap;
    tGlyph *pGlyph;
    FT_Face pFace;
    FILE *pFile;
    tBoolean bRetcode;

    if(pParams->bVerbose)
    {
        printf("Generating a narrow format font.\n");
    }

    //
    // Check to see that the indices passed are sensible.
    //
    if((pParams->iFirst < 0) || (pParams->iFirst > 254) || (pParams->iLast
            < pParams->iFirst) || (pParams->iLast > 255))
    {
        fprintf(stderr, "%s: First and last characters passed (%d, %d) are "
            "invalid. Must be [0,255]\n", pParams->pcAppName, pParams->iFirst,
                pParams->iLast);
        return (1);
    }

    //
    // Check that the whitespace character passed is valid.
    //
    if(!pParams->bNoForceSpace && ((pParams->iSpaceChar > pParams->iLast)
            || (pParams->iSpaceChar < pParams->iFirst)))
    {
        fprintf(stderr, "%s: Forced whitespace character %d is outside the "
            "encoded range.\n", pParams->pcAppName, pParams->iSpaceChar);
        return (1);
    }

    //
    // Do we need to use the new structure format?
    //
    iNewStruct = ((pParams->iFirst == 32) && (pParams->iLast == 126)) ? 0 : 1;

    if(pParams->bVerbose)
    {
        printf("Encoding characters %d to %d. Using %s format.\n",
                pParams->iFirst, pParams->iLast, iNewStruct ? "tFontEx"
                        : "tFont");
    }

    //
    // Initialize the FreeType library.
    //
    if(FT_Init_FreeType(&pLibrary) != 0)
    {
        fprintf(stderr, "%s: Unable to initialize the FreeType library.\n",
                pParams->pcAppName);
        return (1);
    }

    //
    // Load the specified font file into the FreeType library.
    //
    if(FT_New_Face(pLibrary, pParams->pcFontInputName[0], 0, &pFace) != 0)
    {
        fprintf(stderr, "%s: Unable to open font file '%s'\n",
                pParams->pcAppName, pParams->pcFontInputName[0]);
        return (1);
    }

    //
    // Select the Adobe Custom character map if it exists (it is the proper
    // character map to use for the Computer Modern fonts).  If it does not
    // exist, or we have been specifically told to use Unicode, then select the
    // Unicode character map.
    //
    if(pParams->bUnicode || (FT_Select_Charmap(pFace, FT_ENCODING_ADOBE_CUSTOM)
            != 0))
    {
        FT_Select_Charmap(pFace, FT_ENCODING_UNICODE);
    }

    //
    // Set the size of the character based on the specified size.
    //
    if(!SetFontCharSize(pFace, pParams))
    {
        fprintf(stderr, "Exiting on error. Can't set required font size.\n");
        FT_Done_Face(pFace);
        FT_Done_FreeType(pLibrary);
        return (1);
    }

    if(pParams->bVerbose)
    {
        printf("Rendering individual character glyphs...\n");
    }

    //
    // Get some pointers to make things easier.
    //
    pSlot = pFace->glyph;
    ppMap = &(pSlot->bitmap);

    //
    // Loop through the ASCII characters.
    //
    iWidth = 0;
    for(uiChar = pParams->iFirst; uiChar <= pParams->iLast; uiChar++)
    {
        //
        // Were we asked to force this character to be a space?
        //
        if((uiChar == pParams->iSpaceChar) && !pParams->bNoForceSpace)
        {
            //
            // Yes. Substitute a "blank" glyph structure to tell the later
            // code that this is a space.
            //
            if(pParams->bVerbose)
            {
                printf("Forcing character 0x%x to be a space.\n", uiChar);
            }

            g_pGlyphs[uiChar].ulCodePoint = uiChar;
            g_pGlyphs[uiChar].iWidth = 0;
            g_pGlyphs[uiChar].iRows = 0;
            g_pGlyphs[uiChar].iPitch = 0;
            g_pGlyphs[uiChar].iBitmapTop = 0;
            g_pGlyphs[uiChar].pucData = 0;
            g_pGlyphs[uiChar].iMinX = 0;
            g_pGlyphs[uiChar].iMaxX = (3 * pParams->iSize) / 10;
        }
        else
        {
            //
            // Determine the source character we are interested in.  This will
            // depend upon whether we are currently encoding the translated
            // block or the original block.
            //
            uiSrcChar = (uiChar < pParams->iTranslateStart) ? uiChar : ((uiChar
                    - pParams->iTranslateStart) + pParams->iTranslateSource);

            //
            // Get the bitmap image for the required glyph.
            //
            RenderGlyph(pParams, pFace, uiSrcChar, &g_pGlyphs[uiChar], false);

            //
            // If this glyph is wider than any previously encountered
            // glyph, then remember its width as the maximum width in
            // this font.
            //
            if((g_pGlyphs[uiChar].iMaxX - g_pGlyphs[uiChar].iMinX) > iWidth)
            {
                iWidth = (g_pGlyphs[uiChar].iMaxX - g_pGlyphs[uiChar].iMinX);
            }
        }
    }

    //
    // Free the font.
    //
    FT_Done_Face(pFace);

    //
    // Close the FreeType library.
    //
    FT_Done_FreeType(pLibrary);

    if(pParams->bVerbose)
    {
        printf("Finding maximum character dimensions...\n");
    }

    //
    // Loop through the glyphs, finding the minimum and maximum Y values
    // for the glyphs.
    //
    for(pGlyph = &g_pGlyphs[pParams->iFirst], iYMin = 0, iYMax = 0; pGlyph
            <= &g_pGlyphs[pParams->iLast]; pGlyph++)
    {
        //
        // Adjust the minimum Y value if necessary.
        //
        if(pGlyph->iBitmapTop > iYMin)
        {
            iYMin = pGlyph->iBitmapTop;
        }

        //
        // Adjust the maximum Y value if necessary.
        //
        if((pGlyph->iBitmapTop - pGlyph->iRows + 1) < iYMax)
        {
            iYMax = pGlyph->iBitmapTop - pGlyph->iRows + 1;
        }
    }

    if(pParams->bVerbose)
    {
        printf("Compressing glyphs...\n");
    }

    //
    // Loop through the glyphs, compressing each one.
    //
    for(pGlyph = &g_pGlyphs[pParams->iFirst], uiChar = pParams->iFirst; pGlyph
            <= &g_pGlyphs[pParams->iLast]; pGlyph++, uiChar++)
    {
        //
        // Compress a single glyph.
        //
        bRetcode = CompressGlyph(pParams, pGlyph, iWidth, iYMin, iYMax);
        if(pParams->bVerbose)
        {
            if(bRetcode)
            {
                if(pGlyph->pucChar)
                {
                    printf(
                            "Compressed glyph 0x%x. Width %d, %d bytes of data.\n",
                            pGlyph->ulCodePoint, pGlyph->pucChar[1],
                            pGlyph->pucChar[0]);
                }
                else
                {
                    printf("NULL returned on compressing glyph %x!\n",
                            pGlyph->ulCodePoint);
                }
            }
            else
            {
                printf("Error on compressing glyph %x!\n", pGlyph->ulCodePoint);
            }
        }
    }

    if(pParams->bVerbose)
    {
        printf("Writing output file...\n");
    }

    //
    // Convert the filename to all lower case letters.
    //
    for(iX = 0; iX < strlen(pParams->pcFilename); iX++)
    {
        if((pParams->pcFilename[iX] >= 'A') && (pParams->pcFilename[iX] <= 'Z'))
        {
            pParams->pcFilename[iX] += 0x20;
        }
    }

    //
    // Copy the filename and capitalize the first character.
    //
    pcCapFilename = malloc(strlen(pParams->pcFilename) + 1);
    strcpy(pcCapFilename, pParams->pcFilename);
    pcCapFilename[0] -= 0x20;

    //
    // Format the size string.
    //
    if(pParams->bFixedSize)
    {
        //
        // This font is defined by a fixed pixel size.
        //
        sprintf(pucSize, "%dx%d", pParams->iFixedX, pParams->iFixedY);
    }
    else
    {
        //
        // This font is defined by point size.
        //
        sprintf(pucSize, "%d", pParams->iSize);
    }

    //
    // Open the file to which the compressed font will be written.
    //
    sprintf(pucChar, "font%s%d%s%s.c", pParams->pcFilename, pParams->iSize,
            pParams->bBold ? "b" : "", pParams->bItalic ? "i" : "");
    pFile = fopen(pucChar, "w");
    if(!pFile)
    {
        printf("ERROR: Can't open output file %s!\n", pParams->pcFilename);
        return(1);
    }

    //
    // Write the header to the output file.
    //
    bRetcode = WriteCopyrightBlock(pParams, pucSize, pcCapFilename, pFile);
    if(!bRetcode)
    {
        printf("ERROR: Failed to write copyright block to output file!\n");
        fclose(pFile);
        return(1);
    }

    fprintf(pFile, "\n");
    fprintf(pFile, "#include \"grlib/grlib.h\"\n");
    fprintf(pFile, "\n");

    //
    // Get the total size of the compressed font.
    //
    for(pGlyph = &g_pGlyphs[pParams->iFirst], iOpt = 0; pGlyph
            <= &g_pGlyphs[pParams->iLast]; pGlyph++)
    {
        if(pGlyph->pucChar)
        {
            iOpt += pGlyph->pucChar[0];
        }
    }

    //
    // Write the font details to the output file.  The memory usage of the font
    // is a close approximation that may vary based on the compiler used and
    // the compiler options.
    //
    fprintf(pFile, "//********************************************************"
        "*********************\n");
    fprintf(pFile, "//\n");
    fprintf(pFile, "// Details of this font:\n");
    fprintf(pFile, "//     Characters: %d to %d inclusive\n", pParams->iFirst,
            pParams->iLast);
    fprintf(pFile, "//     Style: %s\n", pParams->pcFilename);
    fprintf(pFile, "//     Size: %d point\n", pParams->iSize);
    fprintf(pFile, "//     Bold: %s\n", pParams->bBold ? "yes" : "no");
    fprintf(pFile, "//     Italic: %s\n", pParams->bItalic ? "yes" : "no");
    fprintf(pFile, "//     Memory usage: %d bytes\n", ((iOpt + 3) & ~3) + 200);
    fprintf(pFile, "//\n");
    fprintf(pFile, "//********************************************************"
        "*********************\n");
    fprintf(pFile, "\n");

    //
    // Write the compressed font data array header to the output file.
    //
    fprintf(pFile, "//********************************************************"
        "*********************\n");
    fprintf(pFile, "//\n");
    fprintf(pFile, "// The compressed data for the %d point %s%s%s font.\n",
            pParams->iSize, pcCapFilename, pParams->bBold ? " bold" : "",
            pParams->bItalic ? " italic" : "");
    fprintf(pFile, "// Contains characters %d to %d inclusive.\n",
            pParams->iFirst, pParams->iLast);
    fprintf(pFile, "//\n");
    fprintf(pFile, "//********************************************************"
        "*********************\n");
    fprintf(pFile, "static const unsigned char g_puc%s%d%s%sData[%d] =\n",
            pcCapFilename, pParams->iSize, pParams->bBold ? "b" : "",
            pParams->bItalic ? "i" : "", iOpt);
    fprintf(pFile, "{\n");

    //
    // Loop through the glyphs.
    //
    for(pGlyph = &g_pGlyphs[pParams->iFirst], iOpt = 0; pGlyph
            <= &g_pGlyphs[pParams->iLast]; pGlyph++)
    {
        //
        // Loop through the bytes of the compressed data for this glyph.
        //
        if(pGlyph->pucChar)
        {
            for(iX = 0; iX < pGlyph->pucChar[0]; iX++)
            {
                //
                // Output this byte of the compressed glyph data, along with any
                // special formatting required to make the output file look
                // readable.
                //
                if(iOpt == 0)
                {
                    fprintf(pFile, "   ");
                }
                else if((iOpt % 12) == 0)
                {
                    fprintf(pFile, "\n   ");
                }
                fprintf(pFile, " %3d,", pGlyph->pucChar[iX]);
                iOpt++;
            }
        }
    }

    //
    // Close the compressed data array.
    //
    if((iOpt % 12) != 0)
    {
        fprintf(pFile, "\n");
    }
    fprintf(pFile, "};\n");
    fprintf(pFile, "\n");

    //
    // If we are using the new structure format, write the offset table as a
    // separate array.
    //
    if(iNewStruct)
    {
        fprintf(pFile, "//****************************************************"
            "*************************\n");
        fprintf(pFile, "//\n");
        fprintf(pFile,
                "// The glyph offset table for the %d point %s%s%s font.\n",
                pParams->iSize, pcCapFilename, pParams->bBold ? " bold" : "",
                pParams->bItalic ? " italic" : "");
        fprintf(pFile, "//\n");
        fprintf(pFile, "//****************************************************"
            "*************************\n\n");

        fprintf(pFile, "const unsigned short g_usFontOffset%s%d%s%s[] =\n",
                pcCapFilename, pParams->iSize, pParams->bBold ? "b" : "",
                pParams->bItalic ? "i" : "");

        fprintf(pFile, "{");
        for(iY = 0, iOpt = 0; iY < ((pParams->iLast - pParams->iFirst) + 1); iY++)
        {
            if(!(iY % 8))
            {
                fprintf(pFile, "\n       ");
            }
            if(g_pGlyphs[iY + pParams->iFirst].pucChar)
            {
                fprintf(pFile, " %4d,", iOpt);
                iOpt += g_pGlyphs[iY + pParams->iFirst].pucChar[0];
            }
            else
            {
                fprintf(pFile, " %4d,", 0);
            }
        }
        fprintf(pFile, "\n};\n\n");

        //
        // Write the font definition header to the output file (original tFont
        // structure).
        //
        fprintf(pFile, "//****************************************************"
            "*************************\n");
        fprintf(pFile, "//\n");
        fprintf(pFile,
                "// The font definition for the %d point %s%s%s font.\n",
                pParams->iSize, pcCapFilename, pParams->bBold ? " bold" : "",
                pParams->bItalic ? " italic" : "");
        fprintf(pFile, "//\n");
        fprintf(pFile, "//****************************************************"
            "*************************\n");

        //
        // Write the font definition to the output file (using the new tFontEx
        // structure.
        //
        fprintf(pFile, "const tFontEx g_sFontEx%s%d%s%s =\n", pcCapFilename,
                pParams->iSize, pParams->bBold ? "b" : "",
                pParams->bItalic ? "i" : "");
        fprintf(pFile, "{\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // The format of the font.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    FONT_FMT_EX_PIXEL_RLE,\n");
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // The maximum width of the font.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    %d,\n", iWidth);
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // The height of the font.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    %d,\n", iYMin - iYMax + 1);
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // The baseline of the font.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    %d,\n", iYMin);
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // The first encoded character in the font.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    %d,\n", pParams->iFirst);
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // The last encoded character in the font.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    %d,\n", pParams->iLast);
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // A pointer to the character offset table.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    g_usFontOffset%s%d%s%s,\n", pcCapFilename,
                pParams->iSize, pParams->bBold ? "b" : "",
                pParams->bItalic ? "i" : "");
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // A pointer to the actual font data\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    g_puc%s%d%s%sData\n", pcCapFilename,
                pParams->iSize, pParams->bBold ? "b" : "",
                pParams->bItalic ? "i" : "");
        fprintf(pFile, "};\n");
    }
    else
    {
        //
        // Write the font definition header to the output file (original tFont
        // structure).
        //
        fprintf(pFile, "//****************************************************"
            "*************************\n");
        fprintf(pFile, "//\n");
        fprintf(pFile,
                "// The font definition for the %d point %s%s%s font.\n",
                pParams->iSize, pcCapFilename, pParams->bBold ? " bold" : "",
                pParams->bItalic ? " italic" : "");
        fprintf(pFile, "//\n");
        fprintf(pFile, "//****************************************************"
            "*************************\n");

        //
        // Write the font definition to the output file.
        //
        fprintf(pFile, "const tFont g_sFont%s%d%s%s =\n", pcCapFilename,
                pParams->iSize, pParams->bBold ? "b" : "",
                pParams->bItalic ? "i" : "");
        fprintf(pFile, "{\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // The format of the font.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    FONT_FMT_PIXEL_RLE,\n");
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // The maximum width of the font.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    %d,\n", iWidth);
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // The height of the font.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    %d,\n", iYMin - iYMax + 1);
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // The baseline of the font.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    %d,\n", iYMin);
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // The offset to each character in the font.\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    {\n");
        for(iY = 0, iOpt = 0; iY < 12; iY++)
        {
            fprintf(pFile, "       ");
            for(iX = 0; iX < 8; iX++)
            {
                if((iY != 11) || (iX != 7))
                {
                    if(g_pGlyphs[(iY * 8) + iX + pParams->iFirst].pucChar)
                    {
                        fprintf(pFile, " %4d,", iOpt);
                        iOpt
                                += g_pGlyphs[((iY * 8) + iX + pParams->iFirst)].pucChar[0];
                    }
                    else
                    {
                        fprintf(pFile, " %4d,", 0);
                    }
                }
            }
            fprintf(pFile, "\n");
        }
        fprintf(pFile, "    },\n");
        fprintf(pFile, "\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    // A pointer to the actual font data\n");
        fprintf(pFile, "    //\n");
        fprintf(pFile, "    g_puc%s%d%s%sData\n", pcCapFilename,
                pParams->iSize, pParams->bBold ? "b" : "",
                pParams->bItalic ? "i" : "");
        fprintf(pFile, "};\n");
    }

    //
    // Close the output file.
    //
    fclose(pFile);

    if(pParams->bVerbose)
    {
        printf("Finished.\n");
    }

    //
    // Free up the resources associated with our array of glyphs.
    //
    FreeGlyphs(&g_pGlyphs[pParams->iFirst], (pParams->iLast - pParams->iFirst)
            + 1);

    //
    // Success.
    //
    return (0);
}

//*****************************************************************************
//
// The main application that converts a FreeType-compatible font into a
// compressed raster font for use by the Stellaris Graphics Library.
//
//*****************************************************************************
int main(int argc, char *argv[])
{
    int iOpt;
    int iDisplayFont, iWideFont, iIndex;
    int iRetcode;
    tConversionParameters sParams;

    //
    // Set the defaults for the things that can be specified from the command
    // line.
    //
    sParams.pcAppName = basename(argv[0]);
    sParams.pcFilename = "font";
    sParams.pcCharFile = (char *) 0;
    sParams.pcCopyrightFile = (char *) 0;
    sParams.iNumFonts = 0;
    sParams.iSize = 20;
    sParams.bBold = 0;
    sParams.bBinary = 0;
    sParams.bItalic = 0;
    sParams.bMono = 0;
    sParams.bRemap = 0;
    sParams.bShow = 0;
    sParams.iFirst = 32;
    sParams.iLast = 126;
    sParams.iSpaceChar = 32;
    sParams.bNoForceSpace = 0;
    sParams.bUnicode = 0;
    sParams.iCharMap = -1;
    sParams.iOutputCodePage = -1;
    sParams.bVerbose = 0;

    printf("FTRasterize: Generate a StellarisWare GrLib-compatible font.\n");
    printf("Copyright 2008-2011 Texas Instruments Incorporated.\n\n");

    //
    // Set the defaults for the translated block of character such that no
    // translation takes place and we always encode directly from the [0-255]
    // codepoint range in the source font.
    //
    sParams.iTranslateStart = 256;
    sParams.iTranslateSource = 0;

    //
    // Initialize a couple of command line option flags that are used purely
    // in this function.
    //
    iDisplayFont = 0;
    iWideFont = 0;

    //
    // Process the command line arguments.
    //
    while((iOpt = getopt_long(argc, argv, g_pcCmdLineArgs,
            g_pCmdLineOptions, &iIndex)) != -1)
    {
        //
        // See which argument was found.
        //
        switch(iOpt)
        {
            //
            // The "-a" switch was found.
            //
            case 'a':
            {
                //
                // Save the specified character map index.
                //
                sParams.iCharMap = strtoul(optarg, 0, 0);

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-b" switch was found.
            //
            case 'b':
            {
                //
                // Indicate that this is a bold font.
                //
                sParams.bBold = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-c" switch was found.
            //
            case 'c':
            {
                //
                // Save the specified character map filename.
                //
                sParams.pcCharFile = optarg;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-d" switch was found.
            //
            case 'd':
            {
                //
                // Note that we need to show font information only.
                //
                iDisplayFont = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-e" switch was found.
            //
            case 'e':
            {
                //
                // Save the last character index.
                //
                sParams.iLast = strtoul(optarg, 0, 0);

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-f" switch was found.
            //
            case 'f':
            {
                //
                // Save the specified filename.
                //
                sParams.pcFilename = optarg;

                //
                // This switch has been handled.
                //
                break;
            }


            //
            // The "-g" switch was found.
            //
            case 'g':
            {
                //
                // Save the specified copyright file name.
                //
                sParams.pcCopyrightFile = optarg;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-h" switch was found.
            //
            case 'h':
            {
                //
                // Print out the usage for this application.
                //
                Usage(sParams.pcAppName, false);

                //
                // Exit gracefully.
                //
                return (0);
            }

            //
            // The "-i" switch was found.
            //
            case 'i':
            {
                //
                // Indicate that this is an italic font.
                //
                sParams.bItalic = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-l" switch was found.
            //
            case 'l':
            {
                //
                // Remember that we need to show the glyphs.
                //
                sParams.bShow = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-m" switch was found.
            //
            case 'm':
            {
                //
                // Indicate that this is a monospaced font.
                //
                sParams.bMono = 1;

                //
                // This switch has been handled.
                //
                break;
            }

                //
                // The "-n" switch was found.
                //
            case 'n':
            {
                //
                // Set a flag to tell us not to substitute a space character
                // when we encode character 32.
                //
                sParams.bNoForceSpace = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-o" switch was found.
            //
            case 'o':
            {
                //
                // Save the offset of the translated block within the source
                // font.
                //
                sParams.iTranslateSource = strtoul(optarg, 0, 0);

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-p" switch was found.
            //
            case 'p':
            {
                //
                // Save the first character index.
                //
                sParams.iFirst = strtoul(optarg, 0, 0);

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-r" or "--relocatable" switch was found.
            //
            case 'r':
            {
                //
                // We are being asked to generate a wide (Unicode) font.
                //
                iWideFont = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-s" switch was found.
            //
            case 's':
            {
                //
                // Were we passed a point size or a fixed size index?
                //
                if(optarg[0] == 'F')
                {
                    //
                    // We were passed a fixed size index so save it and remember
                    // that this is not a point size.
                    //
                    sParams.iSize = strtoul(optarg + 1, 0, 0);
                    sParams.bFixedSize = true;
                }
                else
                {
                    //
                    // We've been passed a point size so save it.
                    //
                    sParams.iSize = strtoul(optarg, 0, 0);
                    sParams.bFixedSize = false;
                }

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-t" switch was found.
            //
            case 't':
            {
                //
                // Save the codepoint number for the start of the translated
                // block
                //
                sParams.iTranslateStart = strtoul(optarg, 0, 0);

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-u" switch was found.
            //
            case 'u':
            {
                //
                // Indicate that we are to use the Unicode character mapping.
                //
                sParams.bUnicode = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-v" switch was found.
            //
            case 'v':
            {
                //
                // Indicate that verbose output is enabled.
                //
                sParams.bVerbose = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-w" switch was found.
            //
            case 'w':
            {
                //
                // Save the character that we need to substitute a space for.
                //
                sParams.iSpaceChar = strtoul(optarg, 0, 0);

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-y" or "--binary" switch was found.
            //
            case 'y':
            {
                //
                // Remember that we are to write a binary output file.
                //
                sParams.bBinary = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            case 'z':
            {
                //
                // Remember the output codepage number to use.
                //
                sParams.iOutputCodePage = strtoul(optarg, 0, 0);

                //
                // Make sure the number is valid but don't exit unless the
                // value is too big for the field in the output structure.
                //
                if(sParams.iOutputCodePage > 0xFFFF)
                {
                    //
                    // Print out the usage for this application.
                    //
                    Usage(sParams.pcAppName, true);

                    fprintf(stderr, "Error: Custom codepage values supplied "
                            "via '-z' must be less than 0x10000!\n");
                }
                else
                {
                    if(sParams.iOutputCodePage < 0x8000)
                    {
                        fprintf(stderr, "Warning: Custom codepage values "
                                "supplied via '-z' should be above\n"
                                "CODEPAGE_CUSTOM_BASE (0x8000).\n");
                    }
                }

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // An unknown switch was found.
            //
            default:
            {
                fprintf(stderr, "%s: Unrecognized switch -%c!\n", iOpt);

                //
                // Print out the usage for this application.
                //
                Usage(sParams.pcAppName, true);

                //
                // Return a failure.
                //
                return (1);
            }
        }
    }

    //
    // There must be at least one additional argument on the command line.
    //
    if(optind >= argc)
    {
        //
        // There is not another argument, so print out the usage for this
        // application.
        //
        Usage(sParams.pcAppName, true);

        //
        // Return a failure.
        //
        return (1);
    }

    //
    // Extract the filename(s) of the source font(s) from the command line.
    //
    while((optind < argc) && (sParams.iNumFonts < MAX_FONTS))
    {
        sParams.pcFontInputName[sParams.iNumFonts++] = argv[optind++];
    }

    if(sParams.bVerbose)
    {
        printf("Command line arguments parsed.\n");
    }

    //
    // Were we asked to show information on the font or actually encode
    // something?
    //
    if(iDisplayFont)
    {
        //
        // Display information on the font whose name is passed then exit.
        //
        return (DisplayFontInfo(&sParams));
    }

    //
    // Were we asked to show the selected glyphs?
    //
    if(sParams.bShow)
    {
        return(ShowFontCharacters(&sParams));
    }

    //
    // Have we been asked to generate a multi-byte, relocatable font?
    //
    if(iWideFont)
    {
        //
        // Yes - call the relevant function.
        //
        iRetcode = ConvertWideFont(&sParams);
    }
    else
    {
        //
        // No - generate an 8 bit font.
        //
        iRetcode = ConvertNarrowFont(&sParams);
    }

    //
    // Pass the return code on to the caller.
    //
    return (iRetcode);
}
