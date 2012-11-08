//*****************************************************************************
//
// dfuwrap.c - A simple command line application to generate a file system
//                image file from the contents of a directory.
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
// This is part of revision 9453 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

typedef unsigned char BOOL;
#define FALSE 0
#define TRUE  1

//*****************************************************************************
//
// Globals controlled by various command line parameters.
//
//*****************************************************************************
BOOL g_bVerbose     = FALSE;
BOOL g_bQuiet       = FALSE;
BOOL g_bOverwrite   = FALSE;
BOOL g_bAdd         = TRUE;
BOOL g_bCheck       = FALSE;
BOOL g_bForce       = FALSE;
unsigned long  g_ulAddress   = 0;
unsigned short g_usVendorID  = 0x1CBE;    // Texas Instruments (Stellaris)
unsigned short g_usProductID = 0x00FF;    // Stellaris DFU boot loader
unsigned short g_usDeviceID  = 0x0000;
char *g_pszInput    = NULL;
char *g_pszOutput = "image.dfu";

//*****************************************************************************
//
// Helpful macros for generating output depending upon verbose and quiet flags.
//
//*****************************************************************************
#define VERBOSEPRINT(...) if(g_bVerbose) { printf(__VA_ARGS__); }
#define QUIETPRINT(...) if(!g_bQuiet) { printf(__VA_ARGS__); }

//*****************************************************************************
//
// Macros for accessing multi-byte fields in the DFU suffix and prefix.
//
//*****************************************************************************
#define WRITE_LONG(num, ptr)                                                  \
{                                                                             \
    *((unsigned char *)(ptr)) = ((num) & 0xFF);                               \
    *(((unsigned char *)(ptr)) + 1) = (((num) >> 8) & 0xFF);                  \
    *(((unsigned char *)(ptr)) + 2) = (((num) >> 16) & 0xFF);                 \
    *(((unsigned char *)(ptr)) + 3) = (((num) >> 24) & 0xFF);                 \
}
#define WRITE_SHORT(num, ptr)                                                 \
{                                                                             \
    *((unsigned char *)(ptr)) = ((num) & 0xFF);                               \
    *(((unsigned char *)(ptr)) + 1) = (((num) >> 8) & 0xFF);                  \
}

#define READ_SHORT(ptr)                                                       \
    (*((unsigned char *)(ptr)) | (*(((unsigned char *)(ptr)) + 1) << 8))

#define READ_LONG(ptr)                                                       \
    (*((unsigned char *)(ptr))              |                                \
    (*(((unsigned char *)(ptr)) + 1) << 8)  |                                \
    (*(((unsigned char *)(ptr)) + 2) << 16) |                                \
    (*(((unsigned char *)(ptr)) + 3) << 24))

//*****************************************************************************
//
// Storage for the CRC32 calculation lookup table.
//
//*****************************************************************************
unsigned long g_pulCRC32Table[256];

//*****************************************************************************
//
// The standard DFU file suffix.
//
//*****************************************************************************
unsigned char g_pcDFUSuffix[] =
{
    0x00, // bcdDevice LSB
    0x00, // bcdDevice MSB
    0x00, // idProduct LSB
    0x00, // idProduct MSB
    0x00, // idVendor LSB
    0x00, // idVendor MSB
    0x00, // bcdDFU LSB
    0x01, // bcdDFU MSB
    'U',  // ucDfuSignature LSB
    'F',  // ucDfuSignature
    'D',  // ucDfuSignature MSB
    16,   // bLength
    0x00, // dwCRC LSB
    0x00, // dwCRC byte 2
    0x00, // dwCRC byte 3
    0x00  // dwCRC MSB
};

//*****************************************************************************
//
// The Stellaris-specific binary image suffix used by the DFU driver to
// determine where the image should be located in flash.
//
//*****************************************************************************
unsigned char g_pcDFUPrefix[] =
{
    0x01, // STELLARIS_DFU_PROG
    0x00, // Reserved
    0x00, // LSB start address / 1024
    0x20, // MSB start address / 1024
    0x00, // LSB file payload length (excluding prefix and suffix)
    0x00, // Byte 2 file payload length (excluding prefix and suffix)
    0x00, // Byte 3 file payload length (excluding prefix and suffix)
    0x00, // MSB file payload length (excluding prefix and suffix)
};

//*****************************************************************************
//
// Initialize the CRC32 calculation table for the polynomial required by the
// DFU specification.  This code is based on an example found at
//
// http://www.createwindow.com/programming/crc32/index.htm.
//
//*****************************************************************************
unsigned long
Reflect(unsigned long ulRef, char ucCh)
{
      unsigned long ulValue;
      int iLoop;

      ulValue = 0;

      //
      // Swap bit 0 for bit 7, bit 1 for bit 6, etc.
      //
      for(iLoop = 1; iLoop < (ucCh + 1); iLoop++)
      {
            if(ulRef & 1)
                  ulValue |= 1 << (ucCh - iLoop);
            ulRef >>= 1;
      }
      return ulValue;
}

void
InitCRC32Table()
{
    unsigned long ulPolynomial;
    int i, j;

    // This is the ANSI X 3.66 polynomial as required by the DFU
    // specification.
    //
    ulPolynomial = 0x04c11db7;

    for(i = 0; i <= 0xFF; i++)
    {
        g_pulCRC32Table[i]=Reflect(i, 8) << 24;
          for (j = 0; j < 8; j++)
          {
              g_pulCRC32Table[i] = (g_pulCRC32Table[i] << 1) ^
                                     (g_pulCRC32Table[i] & (1 << 31) ?
                                      ulPolynomial : 0);
          }
          g_pulCRC32Table[i] = Reflect(g_pulCRC32Table[i], 32);
    }
}

//*****************************************************************************
//
// Calculate the CRC for the supplied block of data.
//
//*****************************************************************************
unsigned long
CalculateCRC32(unsigned char *pcData, unsigned long ulLength)
{
    unsigned long ulCRC;
    unsigned long ulCount;
    unsigned char* pcBuffer;
    unsigned char ucChar;

    //
    // Initialize the CRC to all 1s.
    //
    ulCRC = 0xFFFFFFFF;

    //
    // Get a pointer to the start of the data and the number of bytes to
    // process.
    //
    pcBuffer = pcData;
    ulCount = ulLength;

    //
    // Perform the algorithm on each byte in the supplied buffer using the
    // lookup table values calculated in InitCRC32Table().
    //
    while(ulCount--)
    {
        ucChar = *pcBuffer++;
        ulCRC = (ulCRC >> 8) ^ g_pulCRC32Table[(ulCRC & 0xFF) ^ ucChar];
    }

    // Return the result.
    return (ulCRC);
}

//*****************************************************************************
//
// Show the startup banner.
//
//*****************************************************************************
void
PrintWelcome(void)
{
    QUIETPRINT("\ndfuwrap - Wrap a binary file for use in USB DFU download.\n");
    QUIETPRINT("Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.\n\n");
}

//*****************************************************************************
//
// Show help on the application command line parameters.
//
//*****************************************************************************
void
ShowHelp(void)
{
    //
    // Only print help if we are not in quiet mode.
    //
    if(g_bQuiet)
    {
        return;
    }

    printf("This application may be used to wrap binary files which are\n");
    printf("to be flashed to a Stellaris device using the USB boot loader.\n");
    printf("Additionally, the application can check the validity of an\n");
    printf("existing Device Firmware Upgrade (DFU) wrapper or remove the\n");
    printf("wrapper to retrieve the original binary payload.\n\n");
    printf("Supported parameters are:\n\n");
    printf("-i <file> - The name of the input file.\n");
    printf("-o <file> - The name of the output file (default image.dfu)\n");
    printf("-r        - Remove an existing DFU wrapper from the input file.\n");
    printf("-c        - Check validity of the input file's existing DFU wrapper.\n");
    printf("-v <num>  - Set the DFU wrapper's USB vendor ID (default 0x1CBE).\n");
    printf("-p <num>  - Set the DFU wrapper's USB product ID (default 0x00FF).\n");
    printf("-d <num>  - Set the DFU wrapper's USB device ID (default 0x0000).\n");
    printf("-a <num>  - Set the address the binary will be flashed to.\n");
    printf("-x        - Overwrite existing output file without prompting.\n");
    printf("-f        - Force wrapper writing even if a wrapper already exists.\n");
    printf("-? or -h  - Show this help.\n");
    printf("-q        - Quiet mode. Disable output to stdio.\n");
    printf("-e        - Enable verbose output\n\n");
    printf("Example:\n\n");
    printf("   dfuwrap -i program.bin -o program.dfu -a 0x1800\n\n");
    printf("wraps program.bin in a DFU wrapper which will cause the image to\n");
    printf("address 0x1800 in Stellaris flash.\n\n");
}

//*****************************************************************************
//
// Parse the command line, extracting all parameters.
//
// Returns 0 on failure, 1 on success.
//
//*****************************************************************************
int
ParseCommandLine(int argc, char *argv[])
{
    int iRetcode;
    BOOL bRetcode;
    BOOL bShowHelp;

    //
    // By default, don't show the help screen.
    //
    bShowHelp = FALSE;

    while(1)
    {
        //
        // Get the next command line parameter.
        //
        iRetcode = getopt(argc, argv, "a:i:o:v:d:p:eh?qcrfx");

        if(iRetcode == -1)
        {
            break;
        }

        switch(iRetcode)
        {
            case 'i':
                g_pszInput = optarg;
                break;

            case 'o':
                g_pszOutput = optarg;
                break;

            case 'v':
                g_usVendorID = (unsigned short)strtol(optarg, NULL, 0);
                break;

            case 'd':
                g_usDeviceID = (unsigned short)strtol(optarg, NULL, 0);
                break;

            case 'p':
                g_usProductID = (unsigned short)strtol(optarg, NULL, 0);
                break;

            case 'a':
                g_ulAddress = (unsigned long)strtol(optarg, NULL, 0);
                break;

            case 'e':
                g_bVerbose = TRUE;
                break;

            case 'f':
                g_bForce = TRUE;
                break;

            case 'q':
                g_bQuiet = TRUE;
                break;

            case 'x':
                g_bOverwrite = TRUE;
                break;

            case 'c':
                g_bCheck = TRUE;
                break;

            case 'r':
                g_bAdd = FALSE;
                break;

            case '?':
            case 'h':
                bShowHelp = TRUE;
                break;
        }
    }

    //
    // Show the welcome banner unless we have been told to be quiet.
    //
    PrintWelcome();

    //
    // Catch various invalid parameter cases.
    //
    if(bShowHelp || (g_pszInput == NULL) ||
       (((g_ulAddress == 0) || (g_ulAddress & 1023)) && g_bAdd && !g_bCheck))
    {
        ShowHelp();

        //
        // Make sure we were given an input file.
        //
        if(g_pszInput == NULL)
        {
            QUIETPRINT("ERROR: An input file must be specified using the -i "
                       "parameter.\n");
        }

        //
        // Make sure we were given a start address.
        //
        if(g_bAdd && !g_bCheck)
        {
            if(g_ulAddress == 0)
            {
                QUIETPRINT("ERROR: The flash address of the image must be "
                           "provided using the -a parameter.\n");
            }
            else
            {
                QUIETPRINT("ERROR: The supplied flash address must be a "
                           "multiple of 1024.\n");
            }
        }

        //
        // If we get here, we exit immediately.
        //
        exit(1);
    }

    //
    // Tell the caller that everything is OK.
    //
    return(1);
}

//*****************************************************************************
//
// Dump the command line parameters to stdout if we are in verbose mode.
//
//*****************************************************************************
void
DumpCommandLineParameters(void)
{
    if(!g_bQuiet && g_bVerbose)
    {
        printf("Input file:        %s\n", g_pszInput);
        printf("Output file:       %s\n", g_pszOutput);
        printf("Operation:         %s\n",
               g_bCheck ? "Check" : (g_bAdd ? "Add" : "Remove"));
        printf("Vendor ID:         0x%04x\n", g_usVendorID);
        printf("Product ID:        0x%04x\n", g_usProductID);
        printf("Device ID:         0x%04x\n", g_usDeviceID);
        printf("Flash Address:     0x%08lx\n", g_ulAddress);
        printf("Overwrite output?: %s\n", g_bOverwrite ? "Yes" : "No");
        printf("Force wrapper?:    %s\n", g_bForce ? "Yes" : "No");
    }
}

//*****************************************************************************
//
// Read the input file into memory, optionally leaving space before and after
// it for the DFU suffix and Stellaris prefix.
//
// On success, *pulLength is written with the length of the buffer allocated.
// This will be the file size plus the prefix and suffix lengths if bHdrs is
// TRUE.
//
// Returns a pointer to the start of the allocated buffer if successful or
// NULL if there was a problem. If headers have been allocated, the file data
// starts sizeof(g_pcDFUprefix) bytes into the allocated block.
//
//*****************************************************************************
unsigned char *
ReadInputFile(char *pcFilename, BOOL bHdrs, unsigned long *pulLength)
{
    char *pcFileBuffer;
    int iRead;
    int iSize;
    int iSizeAlloc;
    FILE *fhFile;

    QUIETPRINT("Reading input file %s\n", pcFilename);

    //
    // Try to open the input file.
    //
    fhFile = fopen(pcFilename, "rb");
    if(!fhFile)
    {
        //
        // File not found or cannot be opened for some reason.
        //
        QUIETPRINT("Can't open file!\n");
        return(NULL);
    }

    //
    // Determine the file length.
    //
    fseek(fhFile, 0, SEEK_END);
    iSize = ftell(fhFile);
    fseek(fhFile, 0, SEEK_SET);

    //
    // Allocate a buffer to hold the file contents and, if requested, the
    // prefix and suffix structures.
    //
    iSizeAlloc = iSize + (bHdrs ?
                    (sizeof(g_pcDFUPrefix) + sizeof(g_pcDFUSuffix)) : 0);
    pcFileBuffer = malloc(iSizeAlloc);
    if(pcFileBuffer == NULL)
    {
        QUIETPRINT("Can't allocate %d bytes of memory!\n", iSizeAlloc);
        return(NULL);
    }

    //
    // Read the file contents into the buffer at the correct position.
    //
    iRead = fread(pcFileBuffer + (bHdrs ? sizeof(g_pcDFUPrefix) : 0),
                     1, iSize, fhFile);

    //
    // Close the file.
    //
    fclose(fhFile);

    //
    // Did we get the whole file?
    //
    if(iSize != iRead)
    {
        //
        // Nope - free the buffer and return an error.
        //
        QUIETPRINT("Error reading file. Expected %d bytes, got %d!\n",
                   iSize, iRead);
        free(pcFileBuffer);
        return(NULL);
    }

    //
    // If we are adding headers, copy the template structures into the buffer
    // before we return it to the caller.
    //
    if(bHdrs)
    {
        //
        // Copy the prefix.
        //
        memcpy(pcFileBuffer, g_pcDFUPrefix, sizeof(g_pcDFUPrefix));

        //
        // Copy the suffix.
        //
        memcpy(&pcFileBuffer[iSizeAlloc] - sizeof(g_pcDFUSuffix), g_pcDFUSuffix,
               sizeof(g_pcDFUSuffix));
    }

    //
    // Return the new buffer to the caller along with its size.
    //
    *pulLength = (unsigned long)iSizeAlloc;
    return(pcFileBuffer);
}


//*****************************************************************************
//
// Determine whether the supplied block of data appears to start with a valid
// Stellaris-specific DFU download prefix structure.
//
//*****************************************************************************
BOOL
IsPrefixValid(unsigned char *pcPrefix, unsigned char *pcEnd)
{
    unsigned short usStartAddr;
    unsigned long ulLength;

    VERBOSEPRINT("Looking for valid prefix...\n");

    //
    // Is the data block large enough to contain a whole prefix structure?
    //
    if((pcEnd - pcPrefix) < sizeof(g_pcDFUPrefix))
    {
        //
        // Nope - prefix can't be valid.
        //
        VERBOSEPRINT("File is too short to contain a prefix.\n");
        return(FALSE);
    }

    //
    // Check the first 2 bytes of the prefix since their values are fixed.
    //
    if((pcPrefix[0] != 0x01) || (pcPrefix[1] != 0x00))
    {
        //
        // First two values are not as expected so the prefix is invalid.
        //
        VERBOSEPRINT("Prefix fixed values are incorrect.\n");
        return(FALSE);
    }

    //
    // Read the start address and length from the supposed prefix.
    //
    ulLength = READ_LONG(pcPrefix + 4);
    usStartAddr = READ_SHORT(pcPrefix + 2);

    //
    // Is the length as expected? Note that we allow two cases so that we can
    // catch files which may or may not contain the DFU suffix in addition to
    // this prefix.
    //
    if((ulLength != (pcEnd - pcPrefix) - sizeof(g_pcDFUPrefix)) &&
       (ulLength != (pcEnd - pcPrefix) -
           (sizeof(g_pcDFUPrefix) + sizeof(g_pcDFUSuffix))))
    {
        //
        // Nope. Prefix is invalid.
        //
        VERBOSEPRINT("Length is not valid for supplied data.\n", ulLength);
        return(FALSE);
    }

    //
    // If we get here, the prefix is valid.
    //
    VERBOSEPRINT("Prefix appears valid.\n");
    return(TRUE);
}

//*****************************************************************************
//
// Determine whether the supplied block of data appears to end with a valid
// DFU-standard suffix structure.
//
//*****************************************************************************
BOOL
IsSuffixValid(unsigned char *pcData, unsigned char *pcEnd)
{
    unsigned char ucSuffixLen;
    unsigned long ulCRCRead, ulCRCCalc;

    VERBOSEPRINT("Looking for valid suffix...\n");

    //
    // Assuming there is a valid suffix, what length is it reported as being?
    //
    ucSuffixLen = *(pcEnd - 5);
    VERBOSEPRINT("Length reported as %d bytes\n", ucSuffixLen);

    //
    // Is the data long enough to contain this suffix and is the suffix at
    // at least as long as the ones we already know about?
    //
    if((ucSuffixLen < sizeof(g_pcDFUSuffix)) ||
       ((pcEnd - pcData) < ucSuffixLen))
    {
        //
        // The reported length cannot indicate a valid suffix.
        //
        VERBOSEPRINT("Suffix length is not valid.\n");
        return(FALSE);
    }

    //
    // Now check that the "DFU" marker is in place.
    //
    if( (*(pcEnd - 6) != 'D') ||
        (*(pcEnd - 7) != 'F') ||
        (*(pcEnd - 8) != 'U'))
    {
        //
        // The DFU marker is not in place so the suffix is invalid.
        //
        VERBOSEPRINT("Suffix 'DFU' marker is not present.\n");
        return(FALSE);
    }

    //
    // Now check that the CRC of the data matches the CRC in the supposed
    // suffix.
    //
    ulCRCRead = READ_LONG(pcEnd - 4);
    ulCRCCalc = CalculateCRC32(pcData, ((pcEnd - 4) - pcData));

    //
    // If the CRCs match, we have a good suffix, else there is a problem.
    //
    if(ulCRCRead == ulCRCCalc)
    {
        VERBOSEPRINT("DFU suffix is valid.\n");
        return(TRUE);
    }
    else
    {
        VERBOSEPRINT("Read CRC 0x%08lx, calculated 0x%08lx.\n", ulCRCRead, ulCRCCalc);
        VERBOSEPRINT("DFU suffix is invalid.\n");
        return(FALSE);
    }
}

//*****************************************************************************
//
// Dump the contents of the Stellaris-specfic DFU file prefix.
//
//*****************************************************************************
void
DumpPrefix(unsigned char *pcPrefix)
{
    unsigned long ulLength;

    ulLength = READ_LONG(pcPrefix + 4);

    QUIETPRINT("\nStellaris DFU Prefix\n");
    QUIETPRINT("--------------------\n\n");
    QUIETPRINT("Flash address:  0x%08x\n", (READ_SHORT(pcPrefix + 2) * 1024));
    QUIETPRINT("Payload length: %ld (0x%lx) bytes, %ldKB\n", ulLength,
               ulLength, ulLength / 1024);
}

//*****************************************************************************
//
// Dump the contents of the DFU-standard file suffix.  Note that the pointer
// passed here points to the byte one past the end of the suffix such that the
// last suffix byte can be found at *(pcEnd - 1).
//
//*****************************************************************************
void
DumpSuffix(unsigned char *pcEnd)
{
    QUIETPRINT("\nDFU File Suffix\n");
    QUIETPRINT("---------------\n\n");
    QUIETPRINT("Suffix Length:  %d bytes\n", *(pcEnd - 5));
    QUIETPRINT("Suffix Version: 0x%4x\n", READ_SHORT(pcEnd - 10));
    QUIETPRINT("Device ID:      0x%04x\n", READ_SHORT(pcEnd - 16));
    QUIETPRINT("Product ID:     0x%04x\n", READ_SHORT(pcEnd - 14));
    QUIETPRINT("Vendor ID:      0x%04x\n", READ_SHORT(pcEnd - 12));
    QUIETPRINT("CRC:            0x%08x\n", READ_LONG(pcEnd - 4));
}

//*****************************************************************************
//
// Open the output file after checking whether it exists and getting user
// permission for an overwrite (if required) then write the supplied data to
// it.
//
// Returns 0 on success or a positive value on error.
//
//*****************************************************************************
int
WriteOutputFile(char *pszFile, unsigned char *pcData, unsigned long ulLength)
{
    FILE *fh;
    int iResponse;
    unsigned long ulWritten;

    //
    // Have we been asked to overwrite an existing output file without
    // prompting?
    //
    if(!g_bOverwrite)
    {
        //
        // No - we need to check to see if the file exists before proceeding.
        //
        fh = fopen(pszFile, "rb");
        if(fh)
        {
            VERBOSEPRINT("Output file already exists.\n");

            //
            // The file already exists. Close it them prompt the user about
            // whether they want to overwrite or not.
            //
            fclose(fh);

            if(!g_bQuiet)
            {
                printf("File %s exists. Overwrite? ", pszFile);
                iResponse = getc(stdin);
                if((iResponse != 'y') && (iResponse != 'Y'))
                {
                    //
                    // The user didn't respond with 'y' or 'Y' so return an
                    // error and don't overwrite the file.
                    //
                    VERBOSEPRINT("User chose not to overwrite output.\n");
                    return(6);
                }
                printf("Overwriting existing output file.\n");
            }
            else
            {
                //
                // In quiet mode but -x has not been specified so don't
                // overwrite.
                //
                return(7);
            }
        }
    }

    //
    // If we reach here, it is fine to overwrite the file (or the file doesn't
    // already exist) so go ahead and open it.
    //
    fh = fopen(pszFile, "wb");
    if(!fh)
    {
        QUIETPRINT("Error opening output file for writing\n");
        return(8);
    }

    //
    // Write the supplied data to the file.
    //
    VERBOSEPRINT("Writing %ld (0x%lx) bytes to output file.\n", ulLength,
                 ulLength);
    ulWritten = fwrite(pcData, 1, ulLength, fh);

    //
    // Close the file.
    //
    fclose(fh);

    //
    // Did we write all the data?
    //
    if(ulWritten != ulLength)
    {
        QUIETPRINT("Error writing data to output file! Wrote %ld, "
                   "requested %ld\n", ulWritten, ulLength);
        return(9);
    }
    else
    {
        QUIETPRINT("Output file written successfully.\n");
    }

    return(0);
}

//*****************************************************************************
//
// Main entry function for the application.
//
//*****************************************************************************
int
main(int argc, char *argv[])
{
    int iRetcode;
    unsigned char *pcInput;
    unsigned char *pcPrefix;
    unsigned char *pcSuffix;
    unsigned long ulFileLen;
    unsigned long ulCRC;
    BOOL bSuffixValid;
    BOOL bPrefixValid;

    //
    // Initialize the CRC32 lookup table.
    //
    InitCRC32Table();

    //
    // Parse the command line arguments
    //
    iRetcode = ParseCommandLine(argc, argv);
    if(!iRetcode)
    {
        return(1);
    }

    //
    // Echo the command line settings to stdout in verbose mode.
    //
    DumpCommandLineParameters();

    //
    // Read the input file into memory.
    //
    pcInput = ReadInputFile(g_pszInput, g_bAdd, &ulFileLen);
    if(!pcInput)
    {
        VERBOSEPRINT("Error reading input file.\n");
        exit(1);
    }

    //
    // Does the file we just read have a DFU suffix and prefix already?
    //
    pcPrefix = pcInput + (g_bAdd ? sizeof(g_pcDFUPrefix) : 0);
    pcSuffix = pcInput + (ulFileLen - (g_bAdd ? sizeof(g_pcDFUSuffix) : 0));

    bPrefixValid = IsPrefixValid(pcPrefix, pcSuffix);
    bSuffixValid = IsSuffixValid(pcPrefix, pcSuffix);

    //
    // Are we being asked to check the validity of a DFU image?
    //
    if(g_bCheck)
    {
        //
        // Yes - was the DFU prefix valid?
        //
        if(!bPrefixValid)
        {
            QUIETPRINT("File prefix appears to be invalid or absent.\n");
        }
        else
        {
            DumpPrefix(pcPrefix);
        }

        //
        // Was the suffix valid?
        //
        if(!bSuffixValid)
        {
            QUIETPRINT("DFU suffix appears to be invalid or absent.\n");
        }
        else
        {
            DumpSuffix(pcSuffix);
        }

        //
        // Set the return code to indicate whether or not the file appears
        // to be a valid DFU-formatted image.
        //
        iRetcode = (!bPrefixValid || !bSuffixValid) ? 2 : 0;
    }
    else
    {
        //
        // Were we asked to remove the existing DFU wrapper from the file?
        //
        if(!g_bAdd)
        {
            //
            // We can only remove the wrapper if one actually exists.
            //
            if(!bPrefixValid)
            {
                //
                // Without the prefix, we can't tell how much of the file to
                // write to the output.
                //
                QUIETPRINT("This does not appear to be a "
                           "valid DFU-formatted file.\n");
                iRetcode = 3;
            }
            else
            {
                //
                // Write the input file payload to the output file, this removing
                // the wrapper.
                //
                iRetcode = WriteOutputFile(g_pszOutput,
                                           pcPrefix + sizeof(g_pcDFUPrefix),
                                           READ_LONG(pcPrefix + 4));
            }
        }
        else
        {
            //
            // We are adding a new DFU wrapper to the input file. First check to
            // see if it already appears to have a wrapper.
            //
            if(bPrefixValid && bSuffixValid && !g_bForce)
            {
                QUIETPRINT("This file already contains a valid DFU wrapper.\n");
                QUIETPRINT("Use -f if you want to force the writing of "
                           "another wrapper.\n");
                iRetcode = 5;
            }
            else
            {
                //
                // The file is not wrapped or we have been asked to force a
                // new wrapper over the existing one.  First fill in the
                // header fields.
                //
                WRITE_SHORT(g_ulAddress / 1024, pcInput + 2);
                WRITE_LONG(ulFileLen -
                           (sizeof(g_pcDFUPrefix) + sizeof(g_pcDFUSuffix)),
                           pcInput + 4);

                //
                // Now fill in the DFU suffix fields that may have been
                // overridden by the user.
                //
                pcSuffix = pcInput + ulFileLen;
                WRITE_SHORT(g_usDeviceID, pcSuffix - 16);
                WRITE_SHORT(g_usProductID, pcSuffix - 14);
                WRITE_SHORT(g_usVendorID, pcSuffix - 12);

                //
                // Calculate the new file CRC.  This is calculated from all
                // but the last 4 bytes of the file (which will contain the
                // CRC itself).
                //
                ulCRC = CalculateCRC32(pcInput, ulFileLen - 4);
                WRITE_LONG(ulCRC, pcSuffix - 4);

                //
                // Now write the wrapped file to the output.
                //
                iRetcode = WriteOutputFile(g_pszOutput, pcInput, ulFileLen);
            }
        }
    }

    //
    // Free our file buffer.
    //
    free(pcInput);

    //
    // Exit the program and tell the OS that all is well.
    //
    exit(iRetcode);
}
