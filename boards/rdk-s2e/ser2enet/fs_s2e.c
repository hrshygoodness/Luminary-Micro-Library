//*****************************************************************************
//
// fs_s2e.c - File System Processing for lwIP Web Server Apps.
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
// This is part of revision 8555 of the RDK-S2E Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/ethernet.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/fs.h"
#include "httpserver_raw/fsdata.h"
#include "telnet.h"

//*****************************************************************************
//
//! \addtogroup fs_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Include the file system data for this application.  This file is generated
// using the following command:
//
//     makefsfile -i fs -o fsdata-s2e.h -r -h
//
// If any changes are made to the static content of the web pages served by the
// application, this script must be used to regenerate fsdata-s2e.h in order
// for those changes to be picked up by the web server.
//
//*****************************************************************************
#include "fsdata-s2e.h"

//*****************************************************************************
//
// Define the number of open files that we can support.
//
//*****************************************************************************
#ifndef LWIP_MAX_OPEN_FILES
#define LWIP_MAX_OPEN_FILES     30
#endif

//*****************************************************************************
//
// Define various strings and buffers used to generate the diagnostic web
// pages.
//
//*****************************************************************************
#ifdef ENABLE_WEB_DIAGNOSTICS
#define SIZE_DIAG_BUFFER 512
char g_pcDiagBuffer[SIZE_DIAG_BUFFER];
#endif

//*****************************************************************************
//
// Define the file system memory allocation structure.
//
//*****************************************************************************
struct sFileSystemTable
{
    struct fs_file sHandle;
    tBoolean bInUse;
};

//*****************************************************************************
//
// Define the file system memory allocation structure.
//
//*****************************************************************************
struct sFileSystemTable sFileMemory[LWIP_MAX_OPEN_FILES];

//*****************************************************************************
//
//! Allocate memory from the file system table for a file pointer.
//!
//! This function will loop though the file system table to see if there is
//! an available file handle to open.  If there is, a pointer to that file
//! handle will be returned and the handle will be marked as in use.
//!
//! \return Returns the pointer to a file handle if available, otherwise NULL.
//
//*****************************************************************************
static struct fs_file *
fs_malloc(void)
{
    int iIndex;

    //
    // Loop through the file handle table.
    //
    for(iIndex = 0; iIndex < LWIP_MAX_OPEN_FILES; iIndex++)
    {
        //
        // If a file handle is found that is NOT in use, return it.
        //
        if(sFileMemory[iIndex].bInUse == false)
        {
            sFileMemory[iIndex].bInUse = true;
            return(&sFileMemory[iIndex].sHandle);
        }
    }

    //
    // If we get here, no file handle was found.
    //
    return(NULL);
}

//*****************************************************************************
//
//! Allocate memory from the file system table for a file pointer.
//!
//! \param file is the pointer to the file handle that is to be freed.
//!
//! \return None.
//
//*****************************************************************************
static void
fs_free(struct fs_file *file)
{
    int iIndex;

    //
    // Loop through the file handle table.
    //
    for(iIndex = 0; iIndex < LWIP_MAX_OPEN_FILES; iIndex++)
    {
        //
        // If a file handle is found that is NOT in use, return it.
        //
        if(&sFileMemory[iIndex].sHandle == file)
        {
            sFileMemory[iIndex].bInUse = false;
            break;
        }
    }

    //
    // We're done.
    //
    return;
}

//*****************************************************************************
//
//! Open a file and return a handle to the file.
//!
//! \param name is the pointer to the string that contains the file name.
//!
//! This function will check the file name against a list of files that require
//! ``special'' handling.  If the file name matches this list, then the
//! file extensions will be enabled for dynamic file content generation.
//! Otherwise, the file name will be compared against the list of names in
//! in the built-in flash file system.  If the file is not found, a NULL
//! handle will be returned.
//!
//! \return Returns the pointer to the file handle if found, otherwise NULL.
//
//*****************************************************************************
struct fs_file *
fs_open(char *name)
{
    const struct fsdata_file *ptTree;
    struct fs_file *ptFile = NULL;

    //
    // Allocate memory for the file system structure.
    //
    ptFile = fs_malloc();
    if(NULL == ptFile)
    {
        return(NULL);
    }

#ifdef ENABLE_WEB_DIAGNOSTICS
    //
    // Have we been asked for one of the web diagnostic pages?
    //
    if(strncmp(name, "/diag.html?port=", 16) == 0)
    {
        //
        // Yes - have we been passed a valid port number?
        //
        if((name[16] == '0') || (name[16] == '1'))
        {
            //
            // Yes - we have a valid port number so write the diag info into
            // our global buffer.
            //
            TelnetWriteDiagInfo(g_pcDiagBuffer, SIZE_DIAG_BUFFER,
                                name[16] - '0');

            //
            // Setup the file structure to return whatever.
            //
            ptFile->data = g_pcDiagBuffer;
            ptFile->len = strlen(g_pcDiagBuffer);
            ptFile->index = ptFile->len;
            ptFile->pextension = NULL;

            //
            // Return the file system pointer.
            //
            return(ptFile);
        }
    }
#endif

    //
    // Initialize the file system tree pointer to the root of the linked list.
    //
    ptTree = FS_ROOT;

    //
    // Begin processing the linked list, looking for the requested file name.
    //
    while(ptTree != NULL)
    {
        //
        // Compare the requested file "name" to the file name in the
        // current node.
        //
        if(strncmp(name, (char *)ptTree->name, ptTree->len) == 0)
        {
            //
            // Fill in the data pointer and length values from the
            // linked list node.
            //
            ptFile->data = (char *)ptTree->data;
            ptFile->len = ptTree->len;

            //
            // For now, we setup the read index to the end of the file,
            // indicating that all data has been read.
            //
            ptFile->index = ptTree->len;

            //
            // We are not using any file system extensions in this
            // application, so set the pointer to NULL.
            //
            ptFile->pextension = NULL;

            //
            // Exit the loop and return the file system pointer.
            //
            break;
        }

        //
        // If we get here, we did not find the file at this node of the linked
        // list.  Get the next element in the list.
        //
        ptTree = ptTree->next;
    }

    //
    // If we didn't find the file, ptTee will be NULL.  Make sure we
    // return a NULL pointer if this happens.
    //
    if(ptTree == NULL)
    {
        fs_free(ptFile);
        ptFile = NULL;
    }

    //
    // Return the file system pointer.
    //
    return(ptFile);
}

//*****************************************************************************
//
//! Close an opened file designated by the handle.
//!
//! \param file is the pointer to the file handle to be closed.
//!
//! This function will free the memory associated with the file handle, and
//! perform any additional actions that are required for closing this handle.
//!
//! \return None.
//
//*****************************************************************************
void
fs_close(struct fs_file *file)
{
    //
    // Free the main file system object.
    //
    fs_free(file);
}

//*****************************************************************************
//
//! Read data from the opened file.
//!
//! \param file is the pointer to the file handle to be read from.
//! \param buffer is the pointer to data buffer to be filled.
//! \param count is the maximum number of data bytes to be read.
//!
//! This function will fill in the buffer with up to ``count'' bytes of data.
//! If there is ``special'' processing required for dynamic content, this
//! function will also handle that processing as needed.
//!
//! \return the number of data bytes read, or -1 if the end of the file has
//! been reached.
//
//*****************************************************************************
int
fs_read(struct fs_file *file, char *buffer, int count)
{
    //
    // For the S2E web server (as currently coded), there is no dynamic or
    // extended content, so we should always return EOF (-1).
    //
    return(-1);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
