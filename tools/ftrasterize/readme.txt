This program converts any font that FreeType recognizes into a compressed font
for use by the Stellaris Graphics Library.  The font generated may support
either 8 bit indexing allowing support of various ISO8859 variants or 32 bit
indexing allowing support for wide character sets such as Unicode.

If the -r option is not supplied, the tool will generate a font containing a
contiguous block of glyphs identified by the first and last character numbers
provided.  To allow encoding of some ISO8859 character sets from Unicode
fonts where these characters appear at higher codepoints (for example
Latin/Cyrillic where the ISO8859-5 mapping appears at offset 0x400 in Unicode
space), additional parameters may be supplied to translate a block of source
font codepoint numbers downwards into the 0-255 ISO8859 range during conversion.

If the -r option is supplied, the output font is relocatable (and hence
suitable for use from non-random-access memory such as an SSI EEPROM or SDCard)
and supports multiple blocks of characters from wide character codepages.  When
generating this type of font, the -c parameter may be used to provide a list
of the Unicode characters that are to be included in the output font.
If -w is specified and more than one font name is provided on the command
line, the fonts are searched in the order they appear to find characters
if the previous font in the list does not contain a required glyph.

Usage: ftrasterize.exe [options] <font> [<font>]

Supported options are:
  -b            Specifies that this is a bold font.
  -f <filename> Specifies the base name for this font [default="font"].
  -i            Specifies that this is an italic font.
  -m            Specifies that this is a monospaced font.
  -s <size>     Specifies the point size of this font unless the parameter
                starts with "F" in which case the supplied number is assumed
                to be an index into the font's fixed size table. [default=20]
  -w <num>      Forces a character to be whitespace [default=32]
  -n            Do not force whitespace (-w ignored)
  -p <num>      Specifies the first character to encode [default=32]
  -e <num>      Specifies the last character to encode [default=126]
  -t <num>      Specifies the codepoint of the first output font character
                to translate downwards [default=0].  Ignored if used with -r.
  -o <num>      Specifies the source font codepoint for the first character in
                the translated block [default=0].  Ignored if used with -r.
  -u            Use Unicode character mapping in the source font.
  -r            Generate a relocatable, wide character set font.
  -y            Write the output in binary format suitable for storage in a
                file system.  If absent, a C format source file is generated.
                Ignored unless -r is specified.
  -c <filename> Encode characters whose codepoints are provided in the
                given text file.  Each line of the file contains either a
                single hex Unicode character number or two hex Unicode
                numbers separated by a comma.  If the first non-comment
                line contains "REMAP", the output font is generated using
                a custom codepage with the glyphs identified by the characters
                listed in the file indexed sequentially. (only valid with -r).
  -a <num>      Specifies the index of the font character map to use in
                the conversion.  If absent, Unicode is assumed for relocatable,
                wide character fonts or when -u is specified, else the Adobe
                Custom map is used if present or Unicode otherwise.
  -d            Display details of the font provided. All other options are
                ignored if this switch is provided.
  -l            Show the chosen glyphs on the terminal (but don't write any other
                output).
  -z <num>      Set the output font's codepage to the supplied value.  This is
                used to specify a custom codepage identifier when performing
                glyph remapping. Values should be between CODEPAGE_CUSTOM_BASE
                (0x8000) and 0xFFFF. (only valid with -r).
  -v            Shows verbose output.
  -h            Shows this help.

Long command aliases are:

  -a           --charmap
  -b           --bold
  -c           --charfile
  -d           --display
  -e           --end
  -f           --font
  -g           --copyright
  -h           --help
  -i           --italic
  -m           --monospaced
  -n           --no-force-whitespace
  -o           --translate_output
  -p           --start
  -r           --relocatable
  -s           --size
  -l           --show
  -t           --translate_source
  -u           --unicode
  -v           --verbose
  -w           --whitespace
  -y           --binary
  -z           --codepage

Examples:

  ftrasterize.exe -f foobar -s 24 foobar.ttf

Produces fontfoobar24.c with a 24 point version of the font in foobar.ttf.

  ftrasterize.exe -f cyrillic -s 12 -u -p 32 -e 255 -t 160 -o 1024 unicode.ttf

Produces fontcyrillic12.c with a 12 point version of the font in unicode.ttf
with characters numbered 160-255 in the output (ISO8859-5Cyrillic glyphs) taken
from the codepoints starting at 1024 in the source, unicode font.

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

This is part of revision 9453 of the Stellaris Firmware Development Package.
