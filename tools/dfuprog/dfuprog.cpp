//*****************************************************************************
//
// dfuprog.cpp : A simple command-line utility for programming a USB-connected
//               Stellaris board running the USB DFU boot loader.
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
#include <conio.h>
#include <windows.h>
#include "lmdfu.h"
#include "lmdfuwrap.h"

//*****************************************************************************
//
// Helpful macros for generating output depending upon verbose and quiet flags.
//
//*****************************************************************************
#define VERBOSEPRINT(...) if(g_bVerbose) { printf(__VA_ARGS__); }
#define QUIETPRINT(...) if(!g_bQuiet) { printf(__VA_ARGS__); }

//*****************************************************************************
//
// Globals whose values are set or overridden via command line parameters.
//
//*****************************************************************************
bool g_bVerbose      = false;
bool g_bQuiet        = false;
bool g_bOverwrite    = false;
bool g_bUpload       = false;
bool g_bClear        = false;
bool g_bBinary       = false;
bool g_bEnumOnly     = false;
bool g_bDisregardIDs = false;
bool g_bSkipVerify   = false;
bool g_bWaitOnExit   = false;
bool g_bReset        = false;
bool g_bSwitchMode   = false;
char *g_pszFile      = NULL;
unsigned long g_ulAddress  = 0;
unsigned long g_ulAddressOverride  = 0xFFFFFFFF;
unsigned long g_ulLength = 0;
int g_iDeviceIndex = 0;

//*****************************************************************************
//
// Exit the application, optionally pausing for a key press first.
//
//*****************************************************************************
void
ExitApp(int iRetcode)
{

    //
    // Has the caller asked us to pause before exiting?
    //
    if(g_bWaitOnExit)
    {
        printf("\nPress any key to exit...\n");
        while (!_kbhit())
        {
            //
            // Wait for a key press.
            //
        }
    }

    exit(iRetcode);
}

//*****************************************************************************
//
// Display the welcome banner when the program is started.
//
//*****************************************************************************
void
PrintWelcome(void)
{
    if(g_bQuiet)
    {
        return;
    }

    printf("\nUSB Device Firmware Upgrade Example\n");
    printf("Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.\n\n");
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

    printf("This application may be used to download images to a Texas Instruments\n");
    printf("Stellaris microcontroller running the USB Device Firmware Upgrade\n");
    printf("boot loader.  Additionally, the application can read back the\n");
    printf("existing application image or a subsection of flash and store it\n");
    printf("either as raw data or wrapped as a DFU-downloadable image.\n\n");
    printf("Supported parameters are:\n\n");
    printf("-e        - Enumerate connected devices, show info then exit.\n");
    printf("-m        - Switch into DFU mode if device is currently in runtime mode.\n");
    printf("-u        - Upload an image from the board into the target DFU file.\n");
    printf("            If absent, the file will be downloaded to the board.\n");
    printf("-c          Clear a block of flash. The address and size of the\n");
    printf("            block are may be given using -a and -l.  If these are\n");
    printf("            absent, clears the entire writable area of flash. Trumps -u.\n");
    printf("-f <file> - The file name for upload or download use.\n");
    printf("-b        - Upload binary rather than a DFU-formatted file. (used with -u)\n");
    printf("-d        - Disregard VID and PID in DFU image to be downloaded.\n");
    printf("-s        - Skip verification after a download operation.\n");
    printf("-a <num>  - Set the address the binary will be flashed to or read from.\n");
    printf("            If absent, binary files are flashed the default application\n");
    printf("            start address for the target.  Ignored for DFU files.\n");
    printf("-l <num>  - Set the upload length (use with -u). If absent, the\n");
    printf("            entire writable flash area is uploaded.\n");
    printf("-i <num>  - Sets the index of the USB DFU device to access if more\n");
    printf("            than one is found. If absent, the first device found is used.\n");
    printf("-x        - Overwrite existing file without prompting. (used with -u)\n");
    printf("-r        - Reset the target on completion of operation.\n");
    printf("-? or -h  - Show this help.\n");
    printf("-q        - Quiet mode. Disable output to stdio.\n");
    printf("-w        - Wait for a key press before exiting.\n");
    printf("-v        - Enable verbose output\n\n");
    printf("Examples:\n\n");
    printf("   dfuprog -f program.bin -a 0x1800\n\n");
    printf("Writes binary file program.bin to the device at address 0x1800\n\n");
    printf("   dfuprog -i 1 -f program.dfu\n\n");
    printf("Writes DFU-formatted file program.dfu to the second connected\n");
    printf("device (index 1) at the address found in the DFU file prefix.\n\n");
    printf("   dfuprog -u -f appimage.dfu\n\n");
    printf("Reads the current board application image into DFU-formatted file\n");
    printf("appimage.dfu\n\n");
}

//*****************************************************************************
//
// Parse the command line, extracting all parameters.
//
// Returns 0 on success. On failure, calls ExitApp(1).
//
//*****************************************************************************
int
ParseCommandLine(int argc, char *argv[])
{
    int iParm;
    bool bShowHelp;
    char *pcOptArg;

    //
    // By default, don't show the help screen.
    //
    bShowHelp = false;

    //
    // Walk through each of the parameters in the list, skipping the first one
    // which is the executable name itself.
    //
    for(iParm = 1; iParm < argc; iParm++)
    {
        //
        // Does this look like a valid switch?
        //
        if(!argv || ((argv[iParm][0] != '-') && (argv[iParm][0] != '/')) ||
           (argv[iParm][1] == '\0'))
        {
            //
            // We found something on the command line that didn't look like a
            // switch so bomb out.
            //
            printf("Unrecognized or invalid argument: %s\n", argv[iParm]);
            ExitApp(1);
        }
        else
        {
            //
            // For convenience, get a pointer to the next argument since this
            // is often a parameter for a given switch (and since this code was
            // converted from a previous version which used getopt which is not
            // available in the Windows SDK).
            //
            pcOptArg = ((iParm + 1) < argc) ? argv[iParm + 1] : NULL;
        }

        switch(argv[iParm][1])
        {
            case 'w':
                g_bWaitOnExit = true;
                break;

            case 'c':
                g_bClear = true;
                break;

            case 's':
                g_bSkipVerify = true;
                break;

            case 'd':
                g_bDisregardIDs = true;
                break;

            case 'e':
                g_bEnumOnly = true;
                break;

            case 'u':
                g_bUpload = true;
                break;

            case 'm':
                g_bSwitchMode = true;
                break;

            case 'f':
                g_pszFile = pcOptArg;
                iParm++;
                break;

            case 'b':
                g_bBinary = true;
                break;

            case 'a':
                g_ulAddressOverride = (unsigned long)strtol(pcOptArg, NULL, 0);
                iParm++;
                break;

            case 'l':
                g_ulLength = (unsigned long)strtol(pcOptArg, NULL, 0);
                iParm++;
                break;

            case 'i':
                g_iDeviceIndex = (int)strtol(pcOptArg, NULL, 0);
                iParm++;
                break;

            case 'r':
                g_bReset = TRUE;
                break;

            case 'v':
                g_bVerbose = TRUE;
                break;

            case 'q':
                g_bQuiet = TRUE;
                break;

            case 'x':
                g_bOverwrite = TRUE;
                break;

            case '?':
            case 'h':
                bShowHelp = TRUE;
                break;

            default:
                printf("Unrecognized argument: %s\n", argv[iParm]);
                ExitApp(1);
        }
    }

    //
    // Show the welcome banner unless we have been told to be quiet.
    //
    PrintWelcome();

    //
    // Show the help screen if requested.
    //
    if(bShowHelp)
    {
        ShowHelp();
        ExitApp(0);
    }

    //
    // Catch various invalid or pointless parameter cases.
    //
    if(g_bEnumOnly)
    {
        //
        // The -e option causes pretty much all other command line options to
        // be ignored so let the user know if they specified something that is
        // not going to do anything.
        //
        if(g_iDeviceIndex || g_ulLength || g_ulAddress || g_bBinary ||
           g_pszFile || g_bUpload || g_bClear || g_bSwitchMode)
        {
            //
            // Tell the user what's going on but don't exit since this is not
            // a fatal error condition.
            //
            QUIETPRINT("Some options ignored - irrelevant when used "
                       "with -e.\n");
        }
    }
    else
    {
        if(!g_bClear && !g_bSwitchMode)
        {
            //
            // We are performing a download or upload operation.  In this case,
            // we definitely need a file name.
            //
            if(!g_pszFile)
            {
                //
                // No file name provided.  If we haven't displayed it already,
                // show command line help then display the error information.
                //
                ShowHelp();

                QUIETPRINT("ERROR: No file name was specified. Please use -f "
                           "to provide one.\n");
                ExitApp(1);
            }
        }
    }

    //
    // Tell the caller that everything is OK.
    //
    return(0);
}

//*****************************************************************************
//
// Dump the information contained within the passed device information structure
// to stdout assuming we are not in quiet mode.
//
//*****************************************************************************
void
DumpDeviceInformation(tLMDFUHandle hHandle, tLMDFUDeviceInfo *psDevInfo)
{
    char pcStringBuf[256];
    unsigned short usStringLen;
    tLMDFUErr eRetcode;

    //
    // Show the VID and PID for the device.
    //
    QUIETPRINT("VID: 0x%04x    PID: 0x%04x\n", psDevInfo->usVID,
               psDevInfo->usPID);

    //
    // Now query the string descriptors associated with the device and
    // display these.
    //
    // We chicken out here and get an ASCII version of the strings
    // in the default language (even though this may not be
    // something that can be represented in ASCII).  Handling the
    // real UNICODE returned by LMDFUDeviceStringGet() is left as
    // an exercise to the reader.
    //
    usStringLen = sizeof(pcStringBuf);
    eRetcode = _LMDFUDeviceASCIIStringGet(hHandle, psDevInfo->ucProductString,
                                         pcStringBuf, &usStringLen);
    QUIETPRINT("Device Name:   %s\n",
               (eRetcode == DFU_OK) ? pcStringBuf : "<<Unknown>>");


    usStringLen = sizeof(pcStringBuf);
    eRetcode = _LMDFUDeviceASCIIStringGet(hHandle,
                                         psDevInfo->ucManufacturerString,
                                         pcStringBuf, &usStringLen);
    QUIETPRINT("Manufacturer:  %s\n",
               (eRetcode == DFU_OK) ? pcStringBuf : "<<Unknown>>");

    usStringLen = sizeof(pcStringBuf);
    eRetcode = _LMDFUDeviceASCIIStringGet(hHandle,
                                         psDevInfo->ucDFUInterfaceString,
                                         pcStringBuf, &usStringLen);
    QUIETPRINT("DFU Interface: %s\n",
               (eRetcode == DFU_OK) ? pcStringBuf : "<<Unknown>>");

    usStringLen = sizeof(pcStringBuf);
    eRetcode = _LMDFUDeviceASCIIStringGet(hHandle, psDevInfo->ucSerialString,
                                         pcStringBuf, &usStringLen);
    QUIETPRINT("Serial Num:    %s\n",
               (eRetcode == DFU_OK) ? pcStringBuf : "<<Unknown>>");

    //
    // Display other relevant information.
    //
    QUIETPRINT("Max Transfer:  %d bytes\n", psDevInfo->usTransferSize);
    QUIETPRINT("Mode:          %s\n", psDevInfo->bDFUMode ? "DFU" : "Runtime");
    if(psDevInfo->bDFUMode)
    {
        QUIETPRINT("TI Extensions: %s\n", psDevInfo->bSupportsStellarisExtensions ?
                        "Supported" : "Not Supported");
        if(psDevInfo->bSupportsStellarisExtensions)
        {
            QUIETPRINT("Target:        %s revision %c%c\n",
                       psDevInfo->pcPartNumber,
                       psDevInfo->cRevisionMajor + 'A',
                       psDevInfo->cRevisionMinor + '0');
        }
    }
    QUIETPRINT("Attributes:\n");
    QUIETPRINT("   Will Detach:       %s\n",
               ((psDevInfo->ucDFUAttributes & DFU_ATTR_WILL_DETACH) ?
                "Yes" : "No"));
    QUIETPRINT("   Manifest Tolerant: %s\n",
               ((psDevInfo->ucDFUAttributes & DFU_ATTR_MANIFEST_TOLERANT) ?
                "Yes" : "No"));
    QUIETPRINT("   Upload Capable:    %s\n",
        ((psDevInfo->ucDFUAttributes & DFU_ATTR_CAN_UPLOAD) ? "Yes" : "No"));
    QUIETPRINT("   Download Capable:  %s\n",
        ((psDevInfo->ucDFUAttributes & DFU_ATTR_CAN_DOWNLOAD) ? "Yes" : "No"));
}

//*****************************************************************************
//
// Upload an image from the device identified by the passed handle.  The actual
// section of flash to upload and the format to save the upload in is controlled
// by command line parameters via global variables.
//
// Returns 0 on success or a positive error return code on failure.
//
//*****************************************************************************
int
UploadImage(tLMDFUHandle hHandle, tLMDFUParams *psDFU)
{
    FILE *fh;
    tLMDFUErr eRetcode;
    unsigned char *pcFileBuf;
    size_t iLen;
    int iRetcode;
    int iResponse;
    errno_t eErr;

    QUIETPRINT("Uploading from device to %s...\n", g_pszFile);

    //
    // We only support upload on devices supporting the Stellaris DFU
    // protocol since, on other devices, we have no idea how large an
    // uploaded image to expect.
    //
    if(!psDFU)
    {
        QUIETPRINT("Target device does not support Stellaris protocol.\n");
        return(40);
    }

    //
    // Check if the address has a override value.
    //
    if(g_ulAddressOverride != 0xFFFFFFFF)
    {
        g_ulAddress = g_ulAddressOverride;
    }
    else if(g_ulAddress == 0)
    {
        //
        // Where should we start uploading from?  If the start address is not
        // specified, assume the start of the writeable area. Upload from
        // the application start address.
        //
        g_ulAddress = psDFU->ulAppStartAddr;
    }

    //
    // How much data must we read?  If the length is not specified, set the
    // length to be the whole block between the start address and the top of
    // the writable area.
    //
    if(g_ulLength == 0)
    {
        g_ulLength = psDFU->ulFlashTop - g_ulAddress;
    }

    //
    // Now allocate a buffer large enough to hold the image.  We need to add
    // space for the 8 byte prefix and 16 byte suffix if we have been asked
    // for a DFU-format image.
    //
    pcFileBuf = (unsigned char *)malloc(g_ulLength + (g_bBinary ? 0 : 24));
    if(!pcFileBuf)
    {
        //
        // We can't allocate the memory!
        //
        QUIETPRINT("Error allocating %d bytes of memory!\n",
                   g_ulLength + (g_bBinary ? 0 : 24));
        return(41);
    }

    //
    // Upload the data into our buffer.
    //
    eRetcode = _LMDFUUpload(hHandle, pcFileBuf, g_ulAddress,
                           (g_ulLength + (g_bBinary ? 0 : 24)), g_bBinary, NULL);

    if(eRetcode == DFU_OK)
    {
        //
        // We got the image successfully.  Have we been given the go-ahead to
        // overwrite the output file without a warning?
        //
        if(!g_bOverwrite)
        {
            eErr = fopen_s(&fh, g_pszFile, "rb");
            if(eErr == 0)
            {
                //
                // We don't need this handle any more so close the file.
                //
                fclose(fh);

                //
                // The file exists and we are in quiet mode so fail the
                // operation since we can't prompt the user for permission to
                // overwrite.
                //
                if(g_bQuiet)
                {
                    free(pcFileBuf);
                    return(43);
                }
                else
                {
                    //
                    // Prompt the user for permission to overwrite the output
                    // file.
                    //
                    printf("File %s exists. Overwrite? (Y/N): ", g_pszFile);
                    iResponse = getc(stdin);
                    if((iResponse != 'y') && (iResponse != 'Y'))
                    {
                        //
                        // The user didn't respond with 'y' or 'Y' so return an
                        // error and don't overwrite the file.
                        //
                        VERBOSEPRINT("User chose not to overwrite output.\n");
                        free(pcFileBuf);
                        return(44);
                    }
                    printf("Overwriting existing output file.\n");
                }
            }
        }

        //
        // At this point it is fine for us to open the output file for writing.
        //
        eErr = fopen_s(&fh, g_pszFile, "wb");

        if(eErr != 0)
        {
            QUIETPRINT("Error opening output file for writing.\n");
            iRetcode = 45;
        }
        else
        {
            //
            // Write the uploaded image to the file.
            //
            iLen = fwrite(pcFileBuf, 1, (g_ulLength + (g_bBinary ? 0 : 24)), fh);

            //
            // Did we write the expected amount of data?
            //
            if(iLen != (g_ulLength + (g_bBinary ? 0 : 24)))
            {
                //
                // No - something went wrong.
                //
                QUIETPRINT("Error writing output. Write %d bytes, "
                           "expected %d.\n", iLen,
                           (g_ulLength + (g_bBinary ? 0 : 24)));
                iRetcode = 46;
            }
            else
            {
                //
                // Life is good.  Set an appropriate exit code.
                //
                iRetcode = 0;
            }

            //
            // Close the output file.
            //
            fclose(fh);
        }
    }
    else
    {
        //
        // There was an error attempting to upload the image from the device.
        //
        QUIETPRINT("Error uploading %d bytes from 0x%08x!\n",
                   g_ulLength + (g_bBinary ? 0 : 24), g_ulAddress);
        iRetcode = 42;
    }

    //
    // Free up our image buffer.
    //
    free(pcFileBuf);

    return(iRetcode);
}

//*****************************************************************************
//
// Download an image to the the device identified by the passed handle.  The
// image to be downloaded and other parameters related to the operation are
// controlled by command line parameters via global variables.
//
// Returns 0 on success or a positive error return code on failure.
//
//*****************************************************************************
int
DownloadImage(tLMDFUHandle hHandle, tLMDFUParams *psDFU)
{
    FILE *fh;
    tLMDFUErr eRetcode;
    unsigned char *pcFileBuf;
    size_t iLen;
    size_t iRead;
    bool bStellarisFormat;

    QUIETPRINT("Downloading %s to device...\n", g_pszFile);

    //
    // Check if the address has a override value.
    //
    if(g_ulAddressOverride != 0xFFFFFFFF)
    {
        g_ulAddress = g_ulAddressOverride;
    }

    //
    // Does the input file exist?
    //
    fopen_s(&fh, g_pszFile, "rb");
    if(!fh)
    {
        QUIETPRINT("Unable to open file %s. Does it exist?\n", g_pszFile);
        return(10);
    }

    //
    // How big is the file?
    //
    fseek(fh, 0, SEEK_END);
    iLen = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    //
    // Allocate a buffer large enough for the file.
    //
    pcFileBuf = (unsigned char *)malloc(iLen);
    if(!pcFileBuf)
    {
        QUIETPRINT("Error allocating %d bytes of memory!\n", iLen);
        fclose(fh);
        return(11);
    }

    //
    // Read the file into our buffer and check that it was correctly read.
    //
    iRead = fread(pcFileBuf, 1, iLen, fh);
    fclose(fh);

    if(iRead != iLen)
    {
        QUIETPRINT("Error reading input file!\n");
        free(pcFileBuf);
        return(12);
    }

    //
    // Check to see whether this is a binary or a DFU-wrapped file.
    //
    eRetcode = _LMDFUIsValidImage(hHandle, pcFileBuf, (unsigned long)iLen, &bStellarisFormat);

    //
    // Is the image for the target we are currently talking to?
    //
    if((eRetcode == DFU_ERR_UNSUPPORTED) && !g_bDisregardIDs)
    {
        QUIETPRINT("This image does not appear to be valid for the target "
                   "device.\nUse -d to disregard embedded IDs\n");
        free(pcFileBuf);
        return(14);
    }

    if(((eRetcode == DFU_OK) ||
       ((eRetcode == DFU_ERR_UNSUPPORTED) && g_bDisregardIDs)) &&
       bStellarisFormat)
    {
        //
        // This is a DFU formatted image and either it is intended for the
        // target device or the user wants us to ignore the IDs in the file.
        // Since it also contains a Stellaris prefix, we can send it to the
        // main download function.
        //
        VERBOSEPRINT("Image contains valid DFU suffix and Stellaris prefix.\n");
        VERBOSEPRINT("Downloading image to flash.... ");
        eRetcode = _LMDFUDownload(hHandle, pcFileBuf, (unsigned long)iLen, g_bSkipVerify,
                                 g_bDisregardIDs, NULL);
    }
    else
    {
        //
        // This is not a DFU-formatted file so we download it as a binary.  In
        // this case, we need to pass the flash address passed by the user or,
        // if this was not provided, the application start address that the
        // device told us to use when we queried its capabilities earlier.
        //

        //
        // Did the file contain a DFU suffix but no Stellaris prefix? If so,
        // we remove the DFU suffix since this doesn't get downloaded.  The
        // length of the suffix is in the 5th last byte.
        //
        if((eRetcode == DFU_OK) || (eRetcode == DFU_ERR_UNSUPPORTED))
        {
            iLen -= pcFileBuf[iLen - 5];
        }

        VERBOSEPRINT("Image is not fully DFU-wrapped. Downloading as binary\n");
        VERBOSEPRINT("Downloading image to flash.... ");

        eRetcode = _LMDFUDownloadBin(hHandle, pcFileBuf, (unsigned long)iLen,
                                    g_ulAddress, g_bSkipVerify, NULL);
    }

    VERBOSEPRINT("Completed.\n");

    //
    // Free the file buffer memory.
    //
    free(pcFileBuf);

    if(eRetcode != DFU_OK)
    {
        QUIETPRINT("Error %s (%d) reported during file download\n",
                   _LMDFUErrorStringGet(eRetcode), eRetcode);
        return(13);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
// Erase a section of the writeable region of flash.  If the "-a" parameter
// was not specified or was specified with address 0, the entire writeable
// region of the target device flash will be erased.  If a non-zero address
// is provided, the -l parameter must also specified to set the start address
// and length of the region that will be erased.  The start address and length
// must both be integer multiples of 1024.
//
// Returns 0 on success or a positive error return code on failure.
//
//*****************************************************************************
int
ClearFlash(tLMDFUHandle hHandle, tLMDFUParams *psDFU)
{
    tLMDFUErr eRetcode;

    if(g_ulAddress)
    {
        QUIETPRINT("Clearing %dKB flash block from address 0x%08x\n",
                   g_ulLength / 1024, g_ulAddress);
    }
    else
    {
        QUIETPRINT("Clearing entire writable region of flash.\n");
    }

    //
    // Erase the required block and verify that the erase completed
    // successfully.
    //
    eRetcode = _LMDFUErase(hHandle, g_ulAddress, g_ulLength, true, NULL);

    if(eRetcode != DFU_OK)
    {
        QUIETPRINT("Error %s (%d) erasing flash!\n",
                   _LMDFUErrorStringGet(eRetcode), eRetcode);
        return(20);
    }
    else
    {
        QUIETPRINT("Flash erased successfully.\n");
        return(0);
    }
}

//*****************************************************************************
//
// The main entry point of the DFU programmer example application.
//
//*****************************************************************************
int
main(int argc,char* argv[])
{
    tLMDFUDeviceInfo sDevInfo;
    tLMDFUHandle hHandle;
    tLMDFUErr eRetcode;
    tLMDFUParams sDFU;
    tLMDFUParams *psDFU;
    int iExitCode;
    int iDevIndex;
    bool bCompleted;
    bool bDeviceFound;

    //
    // Parse the command line parameters, print the welcome banner and
    // tell the user about any errors they made.
    //
    ParseCommandLine(argc, argv);

    //
    // Initialize the DFU library.
    //
    eRetcode = _LMDFUInit();

    //
    // Could we load the DLL and query all its entry points successfully?
    //
    if(eRetcode != DFU_OK)
    {
        //
        // Oops - something is wrong with the DFU DLL.
        //
        if(eRetcode == DFU_ERR_NOT_FOUND)
        {
            printf("The driver for the USB Device Firmware Upgrade device cannot be found.\n");
            printf("Before running this program, please connect the DFU device to this system\n");
            printf("and install the device driver when prompted by Windows.  The device driver\n");
            printf("can be found on the evaluation kit CD or can be found in the windows_drivers\n");
            printf("directory of your StellarisWare installation.\n\n");
            iExitCode = 10;
        }
        else if(eRetcode == DFU_ERR_INVALID_ADDR)
        {
            printf("The driver for the USB Device Firmware Upgrade device was found but appears\n");
            printf("to be a version which this program does not support. Please download and\n");
            printf("install the latest device driver and example applications from the TI\n");
            printf("web site to ensure that you are using compatible versions. The drivers\n");
            printf("can be found in the windows_drivers directory of your StellarisWare\n");
            printf("installation and the applications can be found in package \"Windows-side\n");
            printf("examples for USB kits\" which may be downloaded from the web site at\n");
            printf("http://www.ti.com/software_updates.\n\n");
            iExitCode = 11;
        }
        else
        {
            printf("An error was reported while attempting to load the device driver for the \n");
            printf("USB Device Firmware Upgrade device.  If this error persists, please download\n");
            printf("and reinstall the latest device driver and example applications from the TI\n");
            printf("web site. The drivers can be found in the windows_drivers directory of your\n");
            printf("StellarisWare installation and the applications can be found in package\n");
            printf("\"Windows-side examples for USB kits\" which may be downloaded from\n");
            printf("http://www.ti.com/software_updates.\n\n");
            iExitCode = 12;
        }

        //
        // Exit now with the appropriate error code.
        //
        ExitApp(iExitCode);
    }

    //
    // Enumerate the available DFU devices until we find the one we have been
    // asked to access.
    //
    QUIETPRINT("Scanning USB buses for supported DFU devices...\n\n");

    iDevIndex = 0;
    bCompleted = false;
    iExitCode = 0;
    bDeviceFound = false;

    do
    {
        //
        // Try to open a device.
        //
        eRetcode = _LMDFUDeviceOpen(iDevIndex, &sDevInfo, &hHandle);

        //
        // Were we successful?
        //
        if(eRetcode == DFU_OK)
        {
            //
            // Yes - if we are enumerating, dump information on the device.
            //
            if(g_bEnumOnly)
            {
                QUIETPRINT("\n<<<< Device %d >>>>\n\n", iDevIndex);
                DumpDeviceInformation(hHandle, &sDevInfo);
            }
            else
            {
                //
                // Have we found the device we are looking for?
                //
                if(iDevIndex == g_iDeviceIndex)
                {
                    //
                    // We have found the required device
                    //
                    bDeviceFound = true;

                    //
                    // Is the device currently in runtime mode?
                    //
                    if(sDevInfo.bDFUMode == FALSE)
                    {
                        //
                        // Have we been asked to switch it into DFU mode?
                        //
                        if(g_bSwitchMode)
                        {
                            QUIETPRINT("\n<<<< Device %d >>>>\n\n", iDevIndex);
                            DumpDeviceInformation(hHandle, &sDevInfo);
                            QUIETPRINT("Switching device into DFU mode.\n");

                            //
                            // Yes - switch the device into DFU mode.
                            //
                            eRetcode = _LMDFUModeSwitch(hHandle);

                            //
                            // Set an appropriate return code.
                            //
                            iExitCode = (eRetcode != DFU_OK) ? 0 : 100;

                            //
                            // Drop out of the loop.
                            //
                            break;
                        }
                        else
                        {
                            QUIETPRINT("Device is in runtime mode. Switch to "
                                       "DFU mode using '-m' before\n"
                                       "attempting any other operation\n");
                        }
                    }
                    else
                    {
                        //
                        // If we were asked to switch modes and the device is already in
                        // DFU mode, check that we were passed some other operation to
                        // perform.
                        //
                        if(g_bSwitchMode)
                        {
                            QUIETPRINT("Device is already in DFU mode. No "
                                        "switch necessary.\n");
                        }

                        //
                        // If it supports the Stellaris DFU protocol, read back
                        // some important parameters.
                        //
                        if(sDevInfo.bSupportsStellarisExtensions)
                        {
                            //
                            // Read flash size parameters from the device.
                            //
                            eRetcode = _LMDFUParamsGet(hHandle, &sDFU);
                            if(eRetcode != DFU_OK)
                            {
                                QUIETPRINT("Error %s (%d) reading flash "
                                           "parameters.\n",
                                           _LMDFUErrorStringGet(eRetcode),
                                           eRetcode);
                            }
                            psDFU = &sDFU;
                        }
                        else
                        {
                            //
                            // The device doesn't support the Stellaris
                            // protocol so set the DFU parameter structure
                            // pointer to NULL.
                            //
                            psDFU = (tLMDFUParams *)0;
                        }

                        //
                        // No errors so far?
                        //
                        if(eRetcode == DFU_OK)
                        {
                            //
                            // Have we been asked to clear the flash?
                            //
                            if(g_bClear)
                            {
                                //
                                // Yes - clear the required area of flash.
                                //
                                iExitCode = ClearFlash(hHandle, psDFU);
                            }
                            //
                            // Yes - have we been asked to upload the current
                            // image to a file on the host?
                            //
                            else if(g_bUpload)
                            {
                                //
                                // Yes - go ahead and perform the upload.
                                //
                                iExitCode = UploadImage(hHandle, psDFU);
                            }
                            else
                            {
                                //
                                // No - download an image to the device if a
                                // filename has been provided.
                                //
                                if(g_pszFile)
                                {
                                    iExitCode = DownloadImage(hHandle, psDFU);
                                }
                            }
                        }

                        //
                        // At this point, we've done whatever we have been asked
                        // to do so make sure we exit the loop next time we get
                        // to the end.
                        //
                        bCompleted = true;
                    }
                }
            }

            //
            // We are finished with this device for now.
            //
            eRetcode = _LMDFUDeviceClose(hHandle, g_bReset);

            //
            // Move on to look for another device.
            //
            iDevIndex++;
        }
    }
    while((eRetcode == DFU_OK) && !bCompleted);

    //
    // Print a final summary of what we found if we are merely enumerating the
    // devices.
    //
    if(g_bEnumOnly)
    {
        QUIETPRINT("\nFound %d device%s.\n", iDevIndex, (iDevIndex == 1) ? "" : "s");
    }
    else
    {
        //
        // Did we find the device that the user wanted to talk to?
        //
        if(!bDeviceFound)
        {
            //
            // No - we couldn't find the requested device.
            //
            QUIETPRINT("The requested device was not found on the bus.\n");
            iExitCode = 1;
        }
    }

    ExitApp(iExitCode);
}

