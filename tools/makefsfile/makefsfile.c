//*****************************************************************************
//
// makefsfile.c - A simple command line application to generate a file system
//                image file from the contents of a directory.
//
// This application is based on the Perl script "makefsdata" shipped with
// the Light Weight Internet Protocol (lwIP) TCP/IP stack which can be found
// at http://www.sics.se/~adam/lwip/.
//
// Copyright (c) 2008-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 7611 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

typedef struct _tFileInfo_
{
    struct _tFileInfo_ *psNext;
    char *pszFileName;
    char *pszStructName;
    long lFileSize;
}
tFileInfo;

//*****************************************************************************
//
// Globals used to hold command line parameter information.
//
//*****************************************************************************
bool g_bExcludeHeaders = false;
bool g_bVerbose = false;
bool g_bQuiet = false;
bool g_bOverwrite = false;
bool g_bBinaryOutput = false;
bool g_bSingleFile = false;
char *g_pszExclude = NULL;
char *g_pszInputDir = NULL;
char *g_pszOutput = "fsdata.c";

//*****************************************************************************
//
// Globals tracking the list of files being processed.
//
//*****************************************************************************
unsigned long g_ulNumFiles = 0;
unsigned long g_ulTotalSize = 0;
long g_lLastFileHeaderOffset = 0;
long g_lFileSizeOffset = 0;
FILE *g_fhOutput = NULL;
tFileInfo g_sFileHead = { NULL, NULL, NULL, 0 };

char *g_pszDefaultExcludeList[] =
{
   ".svn",
   "CVS",
   "thumbs.db",
   "filelist.txt",
   "dirlist.txt"
};

#define NUM_EXCLUDE_STRINGS (sizeof(g_pszDefaultExcludeList) / sizeof(char *))

//*****************************************************************************
//
// Storage for strings read from the user's exclude file and the number of
// filenames it contains.
//
//*****************************************************************************
char *g_pszUserExcludeStrings = NULL;
unsigned long g_ulNumUserExcludeFiles = 0;

//*****************************************************************************
//
// Count of the number of directories processed.
//
//*****************************************************************************
int g_iDirCount = 0;

//*****************************************************************************
//
// HTTP header strings for various filename extensions.
//
//*****************************************************************************
typedef struct
{
    char *pszExtension;
    char *pszHTTPHeader;
}
tHTTPHeader;

tHTTPHeader g_sHTTPHeaders[] =
{
    { "html",  "Content-type: text/html\r\n" },
    { "htm",   "Content-type: text/html\r\n" },
    { "shtml", "Content-type: text/html\r\n"
               "Expires: Fri, 10 Apr 2008 14:00:00 GMT\r\n"
               "Pragma: no-cache\r\n"},
    { "shtm",  "Content-type: text/html\r\n"
               "Expires: Fri, 10 Apr 2008 14:00:00 GMT\r\n"
               "Pragma: no-cache\r\n"},
    { "ssi",   "Content-type: text/html\r\n"
               "Expires: Fri, 10 Apr 2008 14:00:00 GMT\r\n"
               "Pragma: no-cache\r\n"},
    { "gif",   "Content-type: image/gif\r\n" },
    { "png",   "Content-type: image/png\r\n" },
    { "jpg",   "Content-type: image/jpeg\r\n" },
    { "bmp",   "Content-type: image/bmp\r\n" },
    { "ico",   "Content-type: image/x-icon\r\n" },
    { "class", "Content-type: application/octet-stream\r\n" },
    { "js",    "Content-type: application/x-javascript\r\n" },
    { "swf",   "Content-type: application/x-shockwave-flash\r\n" },
    { "ram",   "Content-type: audio/x-pn-realaudio\r\n" },
    { "css",   "Content-type: text/css\r\n" },
    { "xml",   "Content-type: text/xml\r\n"
               "Expires: Fri, 10 Apr 2008 14:00:00 GMT\r\n"
               "Pragma: no-cache\r\n"},
    { "txt",   "Content-type: text/plain\r\n" }
};

#define NUM_HTTP_HEADERS        (sizeof(g_sHTTPHeaders) / sizeof(tHTTPHeader))

#define DEFAULT_HTTP_HEADER     "Content-type: text/plain\r\n"

#define MAX_HTTP_HEADER_LEN     512

//*****************************************************************************
//
// Characters in a path/filename that will be replaced by underscores when
// constructing structure names in the output file.
//
//*****************************************************************************
#define FILENAME_TOKENS         " .\\/~`!@#$%^&*()-+=[]{}|;:""'<>,/?"

//*****************************************************************************
//
// Show the startup banner.
//
//*****************************************************************************
void
PrintWelcome(void)
{
    printf("\nmakefsfile - Generate a file containing a file system image.\n");
    printf("Copyright (c) 2008-2011 Texas Instruments Incorporated.  All rights reserved.\n\n");
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

    printf("This application may be used to create file system images to\n");
    printf("embed in Stellaris applications offering web server interfaces.\n");
    printf("Two output formats are supported.  By default, an ASCII, C file\n");
    printf("containing initialized data structures is generated.  This may be\n");
    printf("built alongside the other application source to link the image\n");
    printf("into the application binary itself.  The second option, enabled\n");
    printf("using the -b command line parameter, outputs a position-independent\n");
    printf("binary file that may be flashed at a suitable address independently\n");
    printf("of the application binary.  Assuming the application knows the\n");
    printf("address at which the image has been placed, it can parse the file\n");
    printf("system image as normal.\n");
    printf("One additional mode is provided to allow easy creation of a C\n");
    printf("character array from a binary file.  If -f is specified, the file\n");
    printf("provided with -i will be dumped to the output as an array of hex\n");
    printf("bytes suitable for inclusion in a program.\n\n");
    printf("Supported parameters are:\n");
    printf("-i <dir>  - The name of the directory containing the files\n");
    printf("            to be included in the image.\n");
    printf("-o <file> - The name of the output file (default fsdata.c)\n");
    printf("-x <file> - A file containing a list of filenames and directory\n");
    printf("            names to be excluded from the generated image.\n");
    printf("-h        - Exclude HTTP headers from files.  By default, HTTP\n");
    printf("            headers are added to each file in the output.\n");
    printf("-b        - Generate a position-independent binary image.\n");
    printf("-r        - Rewrite existing output file without prompting.\n");
    printf("-f        - Dump a single file as a hex character array.\n");
    printf("            In this case, -i is a file rather than a directory\n");
    printf("            name\n");
    printf("-?        - Show this help.\n");
    printf("-q        - Quiet mode.  Disable output to stdio.\n");
    printf("-v        - Enable verbose output\n\n");
    printf("Example:\n\n");
    printf("   makefsfile -i html -o lmfsdata.c\n\n");
    printf("generates an image of all files and directories in and below\n");
    printf("\"html\", writing the result into file lmfsdata.c.\n");
}

//*****************************************************************************
//
// Read the contents of file pszFile and use it to populate the exclude list
// stored in global variables g_pszUserExcludeList and g_ulNumUserExcludeFiles.
//
//*****************************************************************************
bool
PopulateExcludeList(char *pszFile)
{
    FILE *fhExclude;
    long lSize, lRead, lLoop;
    char *pszDest;
    bool bLastWasZero;

    //
    // Open the file for reading.
    //
    fhExclude = fopen(pszFile, "rb");

    if(!fhExclude)
    {
        return(false);
    }

    //
    // Determine the size of the file.
    //
    fseek(fhExclude, 0, SEEK_END);
    lSize = ftell(fhExclude);
    fseek(fhExclude, 0, SEEK_SET);


    //
    // Allocate a block of memory to hold the strings.
    //
    g_pszUserExcludeStrings = malloc(lSize);
    if(!g_pszUserExcludeStrings)
    {
        return(false);
    }

    //
    // Read the whole file into our buffer.
    //
    lRead = (long)fread(g_pszUserExcludeStrings, 1, lSize, fhExclude);

    //
    // Close the file.
    //
    fclose(fhExclude);

    //
    // Did we read everything we expected?
    //
    if(lRead != lSize)
    {
        //
        // No - we didn't read the expected number of bytes.
        //
        free(g_pszUserExcludeStrings);
        return(false);
    }

    //
    // Remove all end-of-line characters, replacing them with zeros.  At the
    // end of this operation, the buffer contains a collection of strings
    // terminated by single NULLs.
    //
    pszDest = g_pszUserExcludeStrings;
    bLastWasZero = true;
    g_ulNumUserExcludeFiles = 0;

    for(lLoop = 0; lLoop < lSize; lLoop++)
    {
        switch(g_pszUserExcludeStrings[lLoop])
        {
            case '\r':
                //
                // Do nothing - we throw away all '\r' characters.
                //
                break;

            case '\n':
                //
                // Substitute a '\n' with '\0' unless we just inserted a 0
                // in which case we skip the character.
                //
                if(!bLastWasZero)
                {
                    g_ulNumUserExcludeFiles++;
                    *pszDest++ = '\0';
                    bLastWasZero = true;
                }
                break;

            case '\0':
                if(!bLastWasZero)
                {
                    g_ulNumUserExcludeFiles++;
                    *pszDest++ = g_pszUserExcludeStrings[lLoop];
                    bLastWasZero = true;
                }
                break;

            default:
                *pszDest++ = g_pszUserExcludeStrings[lLoop];
                bLastWasZero = false;
                break;
        }
    }

    return(true);
}

//*****************************************************************************
//
// Dump the list of file and directory names that are being excluded from
// the output.
//
//*****************************************************************************
void
DumpExcludeList(void)
{
    char *pszDest;
    long lLoop;

    printf("Excluding the following files and directory names:\n");

    //
    // Dump the exclude list contents.
    //
    if(g_pszExclude)
    {
        //
        // We are using a user-supplied exclude list.  Dump this.
        //
        pszDest = g_pszUserExcludeStrings;

        for(lLoop = 0; lLoop < g_ulNumUserExcludeFiles; lLoop++)
        {
            printf("  %s\n", pszDest);
            pszDest += (strlen(pszDest) + 1);
        }
    }
    else
    {
        //
        // We are using the default internal exclude list. Dump this.
        //
        for(lLoop = 0; lLoop < NUM_EXCLUDE_STRINGS; lLoop++)
        {
            printf("  %s\n", g_pszDefaultExcludeList[lLoop]);
        }
    }

    printf("\n");
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
    bool bRetcode;
    bool bShowHelp;
    bool bExcludeOK;

    //
    // By default, don't show the help screen and assume the exclude file
    // was fine.
    //
    bShowHelp = false;
    bExcludeOK = true;

    while(1)
    {
        //
        // Get the next command line parameter.
        //
        iRetcode = getopt(argc, argv, "i:o:x:hv?qrbf");

        if(iRetcode == -1)
        {
            break;
        }

        switch(iRetcode)
        {
            case 'i':
            {
                g_pszInputDir = optarg;
                break;
            }

            case 'o':
            {
                g_pszOutput = optarg;
                break;
            }

            case 'x':
            {
                g_pszExclude = optarg;

                //
                // Populate the exclude list with the contents of the given
                // file.
                //
                bExcludeOK = PopulateExcludeList(g_pszExclude);
                if(!bExcludeOK)
                {
                    bShowHelp = true;
                }
                break;
            }

            case 'f':
            {
                g_bSingleFile = true;
                break;
            }

            case 'h':
            {
                g_bExcludeHeaders = true;
                break;
            }

            case 'b':
            {
                g_bBinaryOutput = true;
                break;
            }

            case 'v':
            {
                g_bVerbose = true;
                break;
            }

            case 'q':
            {
                g_bQuiet = true;
                break;
            }

            case 'r':
            {
                g_bOverwrite = true;
                break;
            }

            case '?':
            {
                bShowHelp = true;
                break;
            }
        }
    }

    //
    // Show the welcome banner unless we have been told to be quiet.
    //
    if(!g_bQuiet)
    {
        PrintWelcome();
    }

    if(bShowHelp || (g_pszInputDir == NULL))
    {
        ShowHelp();

        //
        // Was there a problem with the exclude file?
        //
        if(bExcludeOK == false)
        {
            printf("\nThere was a problem reading exclude file %s.\n",
                   g_pszExclude);
        }

        //
        // Make sure we were given an input file.
        //
        if(g_pszInputDir == NULL)
        {
            printf("\nAn input directory must be specified using the -i "
                   "parameter.\n");
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
        printf("Input %s   %s\n", g_bSingleFile ? "file:     " : "directory:",
               g_pszInputDir);
        printf("Output file:       %s\n", g_pszOutput);
        printf("Output format:     %s\n",
               ((g_bBinaryOutput && !g_bSingleFile)? "Binary" : "ASCII C"));
        printf("Overwrite output?: %s\n", g_bOverwrite ? "Yes" : "No");

        if(!g_bSingleFile)
        {
            printf("Exclude headers?:  %s\n", g_bExcludeHeaders ? "Yes" : "No");
            printf("Exclude file:      %s\n\n", (g_pszExclude == NULL) ? "None" :
                                              g_pszExclude);
            DumpExcludeList();
        }
        else
        {
            printf("\n");
        }
    }
}

//*****************************************************************************
//
// Open the user's chosen output file, taking into account whether or not
// we have been asked to overwrite it automatically.
//
//*****************************************************************************
FILE *
OpenOutputFile(char *szFile)
{
    FILE *fhOut;
    int iResponse;
    unsigned long ulSize;

    //
    // First check to see if the file already exists unless we are to overwrite
    // it automatically.
    //
    if(!g_bOverwrite)
    {
        fhOut = fopen(szFile, "r");

        if(fhOut)
        {
            //
            // Close the file.
            //
            fclose(fhOut);

            //
            // If we are not running quietly, prompt the user as to what they
            // want to do.
            //
            if(!g_bQuiet)
            {
                printf("File %s exists. Overwrite? ", szFile);
                iResponse = getc(stdin);
                if((iResponse != 'y') && (iResponse != 'Y'))
                {
                    //
                    // The user didn't respond with 'y' or 'Y' so return an
                    // error and don't overwrite the file.
                    //
                    return(NULL);
                }
            }
            else
            {
                //
                // In quiet mode but -r has not been specified so don't
                // overwrite.
                //
                return(NULL);
            }
        }
    }

    //
    // If we get here, it is safe to open and, possibly, overwrite the output
    // file.
    //
    if(g_bVerbose)
    {
        printf("Opening output file %s\n", szFile);
    }

    //
    // Open the file in either binary or ASCII mode depending upon the
    // output type we are to use.
    //
    fhOut = fopen(szFile, g_bBinaryOutput ? "wb" : "w");

    if(fhOut)
    {
        if(g_bBinaryOutput)
        {
            //
            // Write the marker word "FIMG" at the beginning of the output
            // file so that the parser knows this is a position-independent
            // binary file system image.
            //
            fwrite("FIMG", 4, 1, fhOut);

            //
            // Remember where we are so that we can patch the file system
            // image size once we are done.
            //
            g_lFileSizeOffset = ftell(fhOut);

            //
            // Now write 4 bytes of 0 just to reserve space for the file
            // system image size.
            //
            ulSize = 0;
            fwrite(&ulSize, 4, 1, fhOut);

            //
            // Initialize the size of the image with the number of bytes
            // we have already written.
            //
            g_ulTotalSize = 8;
        }
        else
        {
            //
            // We are writing an ASCII C output file so add a header to
            // describe what the file contains.
            //
            fprintf(fhOut, "//***************************************************************************\n");
            fprintf(fhOut, "//\n");
            fprintf(fhOut, "// File System Image.\n");
            fprintf(fhOut, "//\n");
            fprintf(fhOut, "// This file was automatically generated using the makefsfile utility.\n");
            fprintf(fhOut, "//\n");
            fprintf(fhOut, "//***************************************************************************\n\n");

            //
            // This variable tracks the number of bytes in the file system
            // image not the number of bytes in the ASCII output file so
            // initialize it to 0 here (since we have not written anything that
            // will contribute to the binary size yet).
            //
            g_ulTotalSize = 0;
        }
    }
    return(fhOut);
}

//*****************************************************************************
//
// This function checks the supplied file or directory name against all those
// in the current exclude list and returns true to indicate that this file is
// to be included in the output or false to indicate that it is to be excluded.
//
//*****************************************************************************
bool
IncludeThisFile(char *pszFile)
{
    int iLoop;
    char *pszString;

    //
    // If the file exclude list has not been overridden, check the filename
    // against our default list.
    //
    if(g_pszExclude == NULL)
    {
        //
        // Look at each of the default exclude strings in turn.
        //
        for(iLoop = 0; iLoop < NUM_EXCLUDE_STRINGS; iLoop++)
        {
            //
            // Does this file name match the exclude list entry?
            //
            if(!strcmp(pszFile, g_pszDefaultExcludeList[iLoop]))
            {
                //
                // Yes - this file is to be excluded.
                //
                return(false);
            }
        }
    }
    else
    {
        //
        // Search the user exclude list rather than the default internal one.
        //
        pszString = g_pszUserExcludeStrings;

        for(iLoop = 0; iLoop < g_ulNumUserExcludeFiles; iLoop++)
        {
            //
            // Does this file name match the exclude list entry?
            //
            if(!strcmp(pszFile, pszString))
            {
                //
                // Yes - this file is to be excluded.
                //
                return(false);
            }

            //
            // Move on to the next string.
            //
            pszString += strlen(pszString) + 1;
        }
    }

    return(true);
}

//*****************************************************************************
//
// Generate the relevant HTTP headers for the given filename and write
// them into the supplied buffer.
//
//*****************************************************************************
bool
GetHTTPHeaders(char *pszDest, int iDestLen, char *pszFile)
{
    int iLen, iLoop;
    char *pszStart;
    char *pszExt;
    char *pszWork;

    //
    // Set up for writing the output.
    //
    iLen = iDestLen;
    pszStart = pszDest;

    //
    // Is this a normal file or our special-case 404 file?
    //
    if(strstr(pszFile, "404"))
    {
        snprintf(pszDest, iLen, "HTTP/1.0 404 File not found\r\n");
    }
    else
    {
        snprintf(pszDest, iLen, "HTTP/1.0 200 OK\r\n");
    }

    //
    // Move to the end of the added string.
    //
    iLen -= strlen(pszDest);
    pszStart += strlen(pszDest);

    //
    // Was the buffer too small?
    //
    if(iLen <= 0)
    {
        //
        // Yes - fail the call.
        //
        if(!g_bQuiet)
        {
            printf("%d byte HTTP header buffer exhausted after line 1!\n",
                   iDestLen);
            printf("Current header is\n%s\n", pszDest);
        }

        return(false);
    }

    //
    // Add the server ID string.
    //
    snprintf(pszStart, iLen,
             "Server: lwIP/1.3.2 (http://www.sics.se/~adam/lwip/)\r\n");

    //
    // Move to the end of the added string.
    //
    iLen = iDestLen - strlen(pszDest);
    pszStart = pszDest + strlen(pszDest);

    //
    // Was the buffer too small?
    //
    if(iLen <= 0)
    {
        //
        // Yes - fail the call.
        //
        if(!g_bQuiet)
        {
            printf("HTTP header buffer exhausted after server ID!\n");
        }

        return(false);
    }

    //
    // Get a pointer to the file extension.  We find this by looking for the
    // last occurrence of "." in the filename passed.
    //
    pszExt = NULL;
    pszWork = strchr(pszFile, '.');
    while(pszWork)
    {
        pszExt = pszWork + 1;
        pszWork = strchr(pszExt, '.');
    }

    if(g_bVerbose)
    {
        printf("File extension is %s.\n", pszExt);
    }

    //
    // Clear pszWork since we are about to use it for something different.
    //
    pszWork = NULL;

    //
    // Now determine the content type and any content-specific headers
    //
    for(iLoop = 0; (iLoop < NUM_HTTP_HEADERS) && pszExt; iLoop++)
    {
        //
        // Have we found a matching extension?
        //
        if(!strcmp(g_sHTTPHeaders[iLoop].pszExtension, pszExt))
        {
            pszWork = g_sHTTPHeaders[iLoop].pszHTTPHeader;
            if(g_bVerbose)
            {
                printf("File extension found. Header is:\n%s\n", pszWork);
            }
            break;
        }
    }

    //
    // Did we find a matching extension?
    //
    if(!pszWork)
    {
        //
        // No - use the default, plain text file type.
        //
        pszWork = DEFAULT_HTTP_HEADER;

        if(g_bVerbose)
        {
            printf("Using default HTTP header for extension %s\n", pszExt);
        }

    }

    //
    // Output the file extension-specific headers.
    //
    snprintf(pszStart, iLen, "%s\r\n", pszWork);

    //
    // Move to the end of the added string.
    //
    iLen = iDestLen - strlen(pszDest);
    pszStart = pszDest + strlen(pszDest);

    //
    // Was the buffer too small?
    //
    if(iLen <= 0)
    {
        //
        // Yes - fail the call.
        //
        return(false);
    }

    //
    // If we get here, all the required headers were correctly written into
    // the supplied buffer.
    //
    return(true);
}

//*****************************************************************************
//
// Dump a block of bytes to the output file in the correct format for ASCII or
// binary output.
//
//*****************************************************************************
bool
DumpHexToOutput(FILE *fhOutput, char *pcData, long lSize)
{
    long lLoop;
    int iRetcode;

    //
    // Are we outputing in ASCII format?
    //
    if(!g_bBinaryOutput)
    {
        //
        // Yes - ASCII output so indent the start of the first line.
        //
        iRetcode = fprintf(fhOutput, "   ");

        //
        // Dump the bytes one at a time.
        //
        for(lLoop = 0; lLoop < lSize; lLoop++)
        {
            //
            // Dump a character in 0xZZ format.
            //
            iRetcode = fprintf(fhOutput, " 0x%02x,",
                               (unsigned char)pcData[lLoop]);

            //
            // Exit the loop if we failed to write to the output.
            //
            if(!iRetcode)
            {
                break;
            }

            //
            // Take a new line after every 8 characters.
            //
            if((lLoop % 8) == 7)
            {
                fprintf(fhOutput, "\n   ");
            }
        }

        //
        // End the final line.
        //
        fprintf(fhOutput, "\n");
    }
    else
    {
        //
        // We are outputing binary so merely write the input data directly
        // to the output file.
        //
        iRetcode = fwrite(pcData, lSize, 1, fhOutput);
    }

    //
    // Update the binary size.
    //
    g_ulTotalSize += lSize;

    //
    // Was everything OK?
    //
    return(iRetcode ? true : false);
}

//*****************************************************************************
//
// Write a 32 bit value to the output file such that it appears in little
// endian byte order.
//
//*****************************************************************************
bool
WriteLittleEndianDWORD(FILE *fhOutput, unsigned long ulValue)
{
    unsigned char ucBuffer[4];
    int iRetcode;

    //
    // Be absolutely sure of the byte order we are going to write out.
    //
    ucBuffer[0] = (unsigned char)(ulValue & 0xFF);
    ucBuffer[1] = (unsigned char)((ulValue >> 8) & 0xFF);
    ucBuffer[2] = (unsigned char)((ulValue >> 16) & 0xFF);
    ucBuffer[3] = (unsigned char)((ulValue >> 24) & 0xFF);

    //
    // Write the 4 bytes comprising the little-endian value.
    //
    iRetcode = fwrite(ucBuffer, 1, 4, fhOutput);

    //
    // Update the binary size.
    //
    g_ulTotalSize += 4;

    //
    // Let the caller know whether or not we wrote the value successfully.
    //
    return((iRetcode == 4) ? true : false);
}

//*****************************************************************************
//
// Dump the contents of the supplied file as a file system data structure,
// including HTTP headers if required.
//
//*****************************************************************************
bool
DumpFileContents(FILE *fhOutput, char *pszFile, tFileInfo *pFileInfo,
                 bool bNoHeaders)
{
    bool bRetcode;
    char pcHTTPHeaders[MAX_HTTP_HEADER_LEN];
    char *pcFileBuffer;
    long lRead;
    FILE *fhFile;

    //
    // Get the HTTP headers if necessary.
    //
    if(!bNoHeaders)
    {
        //
        // Get the HTTP headers appropriate for this file type.
        //
        bRetcode = GetHTTPHeaders(pcHTTPHeaders, MAX_HTTP_HEADER_LEN,
                                  pFileInfo->pszFileName);

        //
        // Did we get the headers successfully?
        //
        if(!bRetcode)
        {
            if(g_bVerbose)
            {
                printf("Failure determining HTTP headers!\n");
            }
            return(false);
        }
    }
    else
    {
        //
        // No HTTP headers.
        //
        pcHTTPHeaders[0] = '\0';
    }

    //
    // Open the file for binary read operations.
    fhFile = fopen(pszFile, "rb");
    if(!fhFile)
    {
        if(g_bVerbose)
        {
            printf("Can't open file %s!\n", pszFile);
        }
        return(false);
    }

    //
    // Determine the file's size.
    //
    fseek(fhFile, 0, SEEK_END);
    pFileInfo->lFileSize = ftell(fhFile);
    fseek(fhFile, 0, SEEK_SET);

    //
    // Zero length files are a problem.
    //
    if(pFileInfo->lFileSize == 0)
    {
        if(g_bVerbose)
        {
            printf("Zero length file!\n");
        }

        return(false);
    }

    //
    // Allocate a buffer for the file data.
    //
    pcFileBuffer = malloc(pFileInfo->lFileSize);
    if(!pcFileBuffer)
    {
        if(g_bVerbose)
        {
            printf("Can't allocate file read buffer!\n");
        }
        return(false);
    }

    //
    // Read the file contents into the buffer.
    //
    lRead = (long)fread(pcFileBuffer, 1, pFileInfo->lFileSize, fhFile);

    //
    // Close the file.
    //
    fclose(fhFile);

    //
    // Did we read everything we expected?
    //
    if(lRead != pFileInfo->lFileSize)
    {
        //
        // There was a problem reading the file.
        //
        if(g_bVerbose)
        {
            printf("Read %ld bytes but expected %ld!\n", lRead,
                   pFileInfo->lFileSize);
        }

        free(pcFileBuffer);
        return(false);
    }

    //
    // Output the structure definition and header comment if we are
    // generating ASCII output.
    //
    if(!g_bBinaryOutput || g_bSingleFile)
    {
        fprintf(fhOutput, "static const unsigned char data%s[] =\n{\n",
                pFileInfo->pszStructName);
        fprintf(fhOutput, "\t/* %s */\n", pFileInfo->pszFileName);
    }
    else
    {
        //
        // We remember the current file pointer so that we can go back
        // and fix up the last offset once we have finished processing all
        // the files.
        //
        g_lLastFileHeaderOffset = ftell(fhOutput);

        //
        // Output the offset to the next file in the output.  We fill this in
        // even before we know whether or not there is another file in the
        // output.  This will be fixed up once we finish parsing the whole
        // directory tree.
        //
        WriteLittleEndianDWORD(fhOutput, (17 +
                               strlen(pFileInfo->pszFileName) +
                               strlen(pcHTTPHeaders) +
                               pFileInfo->lFileSize));

        //
        // Write the offset to the filename string.  The filename starts
        // immediately following this 16 byte header so the offset, from the
        // start of the header, is always 16 bytes.
        //
        WriteLittleEndianDWORD(fhOutput, 16);

        //
        // Write the offset of the data.  Again, this is relative to the start
        // of the header so it is 16 + the length of the filename including
        // its terminating 0.
        //
        WriteLittleEndianDWORD(fhOutput, (17 +
                               strlen(pFileInfo->pszFileName)));

        //
        // Write the size of the data.  This does not include the filename,
        // merely the HTTP headers + the file data itself.
        //
        WriteLittleEndianDWORD(fhOutput, (pFileInfo->lFileSize +
                                          strlen(pcHTTPHeaders)));
    }

    //
    // Dump the filename to the output.  Note that the "+ 1" here ensures
    // that we also dump the terminating zero character.
    //
    if(!g_bSingleFile)
    {
        bRetcode = DumpHexToOutput(fhOutput, pFileInfo->pszFileName,
                                   strlen(pFileInfo->pszFileName) + 1);

        if(!bRetcode)
        {
            if(g_bVerbose)
            {
                printf("Failed to write filename to output!\n");
            }
            free(pcFileBuffer);
            return(false);
        }

        //
        // If required, dump the HTTP headers to the output.  In this case, we don't
        // want the terminating zero character.
        //
        if(!bNoHeaders)
        {
            bRetcode = DumpHexToOutput(fhOutput, pcHTTPHeaders,
                                       strlen(pcHTTPHeaders));

            if(!bRetcode)
            {
                if(g_bVerbose)
                {
                    printf("Failed to write HTTP headers to output!\n");
                }
                free(pcFileBuffer);
                return(false);
            }
        }
    }

    //
    // Dump the contents of the file to the output.
    //
    bRetcode = DumpHexToOutput(fhOutput, pcFileBuffer, pFileInfo->lFileSize);

    //
    // Update the file size to include the headers that we may have appended
    // at the beginning.
    //
    if(!bNoHeaders)
    {
        pFileInfo->lFileSize += strlen(pcHTTPHeaders);
    }

    //
    // Free the file buffer.
    //
    free(pcFileBuffer);

    //
    // Close the file data structure if outputing ASCII.
    //
    if(!g_bBinaryOutput || g_bSingleFile)
    {
        fprintf(fhOutput, "};\n\n");
    }

    return(bRetcode);
}

//*****************************************************************************
//
// Generate a filtered version of the supplied filename with all "/", " " and
// "." characters replaced with underscores.
//
//*****************************************************************************
void
FilterFilename(char *pszDest, char *pszSrc)
{
    int iLoop, iToken, iNumTokens;
    char *pcToken;

    //
    // Get a pointer to our list of token characters.
    //
    pcToken = FILENAME_TOKENS;
    iNumTokens = strlen(pcToken);

    //
    // Loop through each character in the filename.  Make sure we include the
    // terminating zero.
    //
    for(iLoop = 0; iLoop < (strlen(pszSrc) + 1); iLoop++)
    {
        //
        // Check this character against each of the character in our token
        // string.
        //
        for(iToken = 0; iToken < iNumTokens; iToken++)
        {
            //
            // Does this source character match the token character?
            //
            if(pszSrc[iLoop] == pcToken[iToken])
            {
                //
                // Yes - replace it with an underscore and exit the loop.
                //
                pszDest[iLoop] = '_';
                break;
            }
        }

        //
        // If we drop out at the end of the inner loop, we didn't find a token
        // so copy the character from the source to the destination unmodified.
        //
        if(iToken == iNumTokens)
        {
            pszDest[iLoop] = pszSrc[iLoop];
        }
    }
}

//*****************************************************************************
//
// Process a single file, copying it into the output and updating the file
// list to include information necessary to build the file information list at
// the end of the output file.
//
//*****************************************************************************
bool
ProcessSingleFile(char *pszFile, char *pszRootDir)
{
    tFileInfo *pFileInfo;
    char *pszFSFileName;
    bool bRetcode;

    //
    // Tell the user what we are up to.
    //
    if(g_bVerbose)
    {
        printf("Processing %s...\n", pszFile);
    }

    //
    // Get a pointer to the start of the filename as it will be in the
    // new file system. This means removing the pszRootDir substring from the
    // start of pszFile.
    //
    if(!strncmp(pszFile, pszRootDir, strlen(pszRootDir)))
    {
        pszFSFileName = pszFile + strlen(pszRootDir);
        if(g_bVerbose)
        {
            printf("Final filename is %s\n", pszFSFileName);
        }
    }
    else
    {
        //
        // Weird - the root directory doesn't appear in the passed filename.
        //
        return(false);
    }

    //
    // Create a new entry for the file list.
    //
    pFileInfo = malloc(sizeof(tFileInfo));

    if(!pFileInfo)
    {
        if(g_bVerbose)
        {
            printf("Can't allocate memory to store file information!\n");
        }
        return(false);
    }

    //
    // Allocate memory to hold the file name in both its original and
    // filtered forms.
    //
    pFileInfo->pszFileName = malloc(strlen(pszFSFileName) + 1);
    pFileInfo->pszStructName = malloc(strlen(pszFSFileName) + 1);

    //
    // Make sure we got the required memory.
    //
    if(!pFileInfo->pszFileName || !pFileInfo->pszStructName)
    {
        if(g_bVerbose)
        {
            free(pFileInfo);
            printf("Can't allocate memory to store filename information!\n");
        }
        return(false);
    }

    //
    // Copy the filename into the file info structure.
    //
    strcpy(pFileInfo->pszFileName, pszFSFileName);

    //
    // Generate the output structure name by replacing all dots and slashes in
    // the filename with underscores.
    //
    FilterFilename(pFileInfo->pszStructName, pszFSFileName);

    if(g_bVerbose)
    {
        printf("Filtered filename is %s\n", pFileInfo->pszStructName);
    }

    //
    // Dump the contents of the file to the output.
    //
    bRetcode = DumpFileContents(g_fhOutput, pszFile, pFileInfo,
                                g_bExcludeHeaders);

    //
    // Was there a problem?
    //
    if(!bRetcode)
    {
        //
        // Yes - free the memory we allocated.
        //
        free(pFileInfo->pszFileName);
        free(pFileInfo->pszStructName);
        free(pFileInfo);
    }
    else
    {
        //
        // The file was dumped successfully so link its information into the
        // list for later use in building the directory.
        //
        pFileInfo->psNext = g_sFileHead.psNext;
        g_sFileHead.psNext = pFileInfo;
    }

    //
    // Tell the caller how things went.
    //
    return(bRetcode);
}

//*****************************************************************************
//
// This recursive function processes all the files in the directory tree below
// the directory whose name is passed.
//
// pszDir is the path to the directory to process relative to the working
// directory of the application.
//
//*****************************************************************************
bool
ProcessFilesInDirectory(char *pszDir)
{
    int iRetcode;
    DIR *hDir;
    struct stat sFileInfo;
    struct dirent *psDirEntry;
    char szPath[FILENAME_MAX];
    bool bRetcode;

    //
    // Increment the number of directories we have processed.
    //
    g_iDirCount++;

    if(g_bVerbose)
    {
        printf("Changing to directory %s\n", pszDir);
    }

    //
    // Open the directory we have been passed.
    //
    hDir = opendir(pszDir);

    //
    // Read and process each entry in the directory
    //
    while((psDirEntry = readdir(hDir)) != NULL)
    {
        //
        // Ignore "." and ".." directories and any file which is listed
        // in the exclude list.
        //
        if(!strcmp(psDirEntry->d_name, ".")  ||
           !strcmp(psDirEntry->d_name, "..") ||
           !IncludeThisFile(psDirEntry->d_name))
        {
            if(g_bVerbose)
            {
                printf("Excluding %s\n", psDirEntry->d_name);
            }
            continue;
        }

        //
        // Get information on this file (remembering to add the path from
        // the current working directory first).
        //
        snprintf(szPath, FILENAME_MAX, "%s/%s",pszDir, psDirEntry->d_name);
        iRetcode = stat(szPath, &sFileInfo);

        if(iRetcode == 0)
        {
            //
            // Is this a directory?
            //
            if(S_ISDIR(sFileInfo.st_mode))
            {
                //
                // Yes - make a recursive call to this function to process the
                // new directory.
                //
                bRetcode = ProcessFilesInDirectory(szPath);
            }
            else
            {
                //
                // It's a file so go ahead and process it.
                //
                bRetcode = ProcessSingleFile(szPath, g_pszInputDir);
            }

            //
            // If something went wrong, return an error.
            //
            if(!bRetcode)
            {
                if(g_bVerbose)
                {
                    printf("Error reported processing %s\n", szPath);
                }
                break;
            }
        }
        else
        {
            printf("Can't get file info for %s! Returned %d. Errno %d\n",
                   szPath, iRetcode, errno);
            bRetcode = false;
            break;
        }
    }

    //
    // Close the directory and return
    //
    closedir(hDir);
    return(bRetcode);
}

//*****************************************************************************
//
// The main entry point of the application.
//
//*****************************************************************************
int
main(int argc, char *argv[])
{
    int iRetcode;
    int iFileCount;
    long lSize;
    tFileInfo *pFileInfo;
    char *pszPrevious;
    bool bRetcode;

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
    // Open the output file for writing in ASCII mode.
    //
    g_fhOutput = OpenOutputFile(g_pszOutput);
    if(!g_fhOutput)
    {
        if(g_pszUserExcludeStrings)
        {
            free(g_pszUserExcludeStrings);
        }
        return(2);
    }

    //
    // Was -f specified?  If so, we just dump one file to the output as a
    // C-style array of unsigned chars in hex format.  This is here to save me
    // spending any more time mucking with "od" and regular expressions to
    // generate a C array containing a single binary file.
    //
    if(g_bSingleFile)
    {
        tFileInfo FileInfo;

        FileInfo.pszStructName = "File";
        FileInfo.pszFileName = g_pszInputDir;
        bRetcode = DumpFileContents(g_fhOutput, g_pszInputDir, &FileInfo, true);

        iFileCount = 1;
        g_iDirCount = 1;
        g_ulTotalSize = FileInfo.lFileSize;
        iRetcode = (bRetcode ? 0 : 1);
    }
    else
    {
        //
        // Start the (recursive) process of processing the files.
        //
        bRetcode = ProcessFilesInDirectory(g_pszInputDir);

        //
        // If we processed all the files successfully and are outputing ASCII,
        // we have some more to do.
        //
        if(bRetcode)
        {
            //
            // If we are outputing binary, the whole output is now complete aside
            // from patching the final "pNext" offset in the file. If generating
            // ASCII, however, we still need to generate the file descriptor list.
            // This could have been done during the processing of the files as it
            // is for binary mode but by keeping the table to the end, it makes
            // the structure of the file system easier to understand when reading
            // the file so we do this instead.
            //

            //
            // Clear our file counter and previous file marker.
            //
            iFileCount = 0;
            pszPrevious = NULL;

            //
            // Walk through the information we gathered on each file and output
            // the relevant structure definition.
            //
            while(g_sFileHead.psNext)
            {
                //
                // Increment our file counter.
                //
                iFileCount++;

                //
                // Which file are we dealing with just now?
                //
                pFileInfo = g_sFileHead.psNext;

                //
                // If outputing ASCII, dump the file descriptor for this file.
                //
                if(!g_bBinaryOutput)
                {
                    //
                    // What is the size of the filename string embedded in the file
                    // data?
                    //
                    lSize = strlen(pFileInfo->pszFileName) + 1;

                    fprintf(g_fhOutput, "const struct fsdata_file file%s[] =\n",
                            pFileInfo->pszStructName);
                    fprintf(g_fhOutput,
                            "{\n\t{\n\t\t%s%s,\n\t\tdata%s,\n\t\tdata%s + %ld,\n\t\tsizeof(data%s) - %ld\n\t}\n};\n\n",
                            (pszPrevious ? "file" : ""),
                            (pszPrevious ? pszPrevious : "NULL"),
                             pFileInfo->pszStructName,
                             pFileInfo->pszStructName, lSize,
                             pFileInfo->pszStructName, lSize);

                    //
                    // Update the binary size.
                    //
                    g_ulTotalSize += 16;
                }

                //
                // Free the previous filename string if one exists.
                //
                if(pszPrevious)
                {
                    free(pszPrevious);
                }

                //
                // Remember this filename for next time round.
                //
                pszPrevious = pFileInfo->pszStructName;

                //
                // Unlink this node from the file list.
                //
                g_sFileHead.psNext = pFileInfo->psNext;

                //
                // If this is the very last file, we need to output
                // one additional line to finish things off before we
                // free the file info structure.
                //
                if((pFileInfo->psNext == NULL) && !g_bBinaryOutput)
                {
                    fprintf(g_fhOutput, "#define FS_ROOT file%s\n\n",
                            pFileInfo->pszStructName);
                }

                //
                // Free the memory associated with this node in the list.  We free
                // the pszStructName field next time round since it is saved in
                // pszPrevious;
                //
                free(pFileInfo->pszFileName);
                free(pFileInfo);
            }

            //
            // Free the final previous filename string if one exists.
            //
            if(pszPrevious)
            {
                free(pszPrevious);
            }

            //
            // All files have been processed so now finish things off with the
            // #define which indicates the number of files.
            //
            if(!g_bBinaryOutput)
            {
                iRetcode = fprintf(g_fhOutput, "#define FS_NUMFILES %d\n\n",
                                   iFileCount);
            }
            else
            {
                //
                // In binary mode, we need to patch the offset of the last file
                // descriptor written to the output to mark it as the last file
                // in the list.
                //
                fseek(g_fhOutput, g_lLastFileHeaderOffset, SEEK_SET);
                iRetcode = (int)WriteLittleEndianDWORD(g_fhOutput, 0);

                //
                // Adjust the total size since the previous write doesn't
                // contribute to the overall binary size.
                //
                g_ulTotalSize -= 4;

                //
                // We also need to add the final binary size to the file system
                // header.
                //
                if(iRetcode)
                {
                    fseek(g_fhOutput, g_lFileSizeOffset, SEEK_SET);
                    iRetcode = (int)WriteLittleEndianDWORD(g_fhOutput,
                                                           g_ulTotalSize);
                }

                //
                // Once again, adjust the total size to ignore the last write.
                //
                g_ulTotalSize -= 4;
            }

            //
            // Make sure we wrote the last string successfully.  The assumption
            // here is that if any failure occured in earlier fprintf calls, this
            // last one will also fail so we don't need to check on every call.
            // Valid? Let's see...
            //
            if(!iRetcode)
            {
                //
                // Set a non-zero application return code to indicate a failure.
                //
                iRetcode = 4;
            }
            else
            {
                //
                // Set the final application return code to 0 to indicate success.
                //
                iRetcode = 0;
            }
        }
        else
        {
            //
            // Set an error return code since we didn't complete the file
            // processing successfully.
            //
            iRetcode = 3;
        }
    }
    //
    // Close the output file.
    //
    fclose(g_fhOutput);

    //
    // Free the user exclude list if one exists.
    //
    if(g_pszUserExcludeStrings)
    {
        free(g_pszUserExcludeStrings);
    }

    //
    // Tell the user what happened.
    //
    if(!g_bQuiet)
    {
        if(iRetcode == 0)
        {
            printf("Completed successfully. "
                   "%d files from %d director%s processed.\n"
                   "Binary size %ld (0x%08lx) bytes\n",
                   iFileCount, g_iDirCount,
                   ((g_iDirCount == 1) ? "y" : "ies"), g_ulTotalSize,
                   g_ulTotalSize);
        }
        else
        {
            printf("An error (%d) occured while processing files!\n",
                   iRetcode);
        }
    }

    //
    // Send the required exit code.
    //
    return(iRetcode);
}

