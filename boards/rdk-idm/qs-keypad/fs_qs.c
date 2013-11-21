//*****************************************************************************
//
// fs_qs.c - File system processing for the lwIP web server.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/fs.h"
#include "httpserver_raw/fsdata.h"
#include "qs-keypad.h"

//*****************************************************************************
//
// Include the file system data for this application.  This file is generated
// by the makefsdata script from lwIP, using the following command (all on one
// line):
//
//     perl ../../../third_party/lwip-1.3.2/apps/httpserver_raw/makefsdata
//          html fsdata-qs.h
//
// If any changes are made to the static content of the web pages served by the
// game, this script must be used to regenerate fsdata-qs.h in order for those
// changes to be picked up by the web server in the game.
//
//*****************************************************************************
#include "fsdata-qs.h"

//*****************************************************************************
//
// Open a file and return a handle to the file, if found.  Otherwise, return
// NULL.
//
//*****************************************************************************
struct fs_file *
fs_open(char *name)
{
    const struct fsdata_file *ptTree;
    struct fs_file *ptFile = NULL;
    unsigned long ulIdx, ulValue;

    //
    // Allocate memory for the file system structure.
    //
    ptFile = mem_malloc(sizeof(struct fs_file));
    if(ptFile == NULL)
    {
        //
        // If we can't allocate the memory, return.
        //
        return(NULL);
    }

    //
    // See if the security code is being changed.
    //
    if(strncmp(name, "/index.html?value=", 18) == 0)
    {
        //
        // Extract the security code from the HTML request.
        //
        ulValue = 0;
        for(ulIdx = 18; ulIdx < 22; ulIdx++)
        {
            if((name[ulIdx] >= '0') && (name[ulIdx] <= '9'))
            {
                ulValue = (ulValue << 4) | (name[ulIdx] - '0');
            }
            else
            {
                break;
            }
        }

        //
        // If there was actually a security code provided, then change the
        // security code to the one provided.
        //
        if(ulIdx != 18)
        {
            SetAccessCode(ulValue);
        }

        //
        // Remove the GET data from the end of the file name.
        //
        name[11] = 0;
    }

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
        // Compare the requested file "name" to the file name in the current
        // node.
        //
        if(strncmp(name, (char *)ptTree->name, ptTree->len) == 0)
        {
            //
            // Fill in the data pointer and length values from the linked list
            // node.
            //
            ptFile->data = (char *)ptTree->data;
            ptFile->len = ptTree->len;

            //
            // For now, we setup the read index to the end of the file,
            // indicating that all data has been read.
            //
            ptFile->index = ptTree->len;

            //
            // If this is the index file, setup this file structure so that
            // fs_read() below will be called to perform the actual read of the
            // data.
            //
            if(strcmp(name, "/index.html") == 0)
            {
                ptFile->len = 0;
                ptFile->index = 0;
                ptFile->pextension = (void *)ptTree->len;
            }
            else
            {
                ptFile->pextension = 0;
            }

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
    // Check to see if we actually find a file.  If we did not, free up the
    // file system structure.
    //
    if(ptTree == NULL)
    {
        mem_free(ptFile);
        ptFile = NULL;
    }

    //
    // Return the file system pointer.
    //
    return(ptFile);
}

//*****************************************************************************
//
// Close an opened file designated by the handle.
//
//*****************************************************************************
void
fs_close(struct fs_file *file)
{
    //
    // Free the memory allocated by the fs_open function.
    //
    mem_free(file);
}

//*****************************************************************************
//
// Read the next chunck of data from the file.  Return the count of data
// that was read.  Return 0 if no data is currently available.  Return
// a -1 if at the end of file.
//
//*****************************************************************************
int
fs_read(struct fs_file *file, char *buffer, int count)
{
    //
    // Check to see if more data is available.
    //
    if(file->pextension == 0)
    {
        //
        // There is no remaining data.  Return a -1 for EOF indication.
        //
        return(-1);
    }

    //
    // Determine how much data we can copy.  The minimum of the 'count'
    // parameter or the available data in the file system buffer.
    //
    if((unsigned long)file->pextension < count)
    {
        count = (unsigned long)file->pextension;
    }

    //
    // Copy the data.
    //
    memcpy(buffer, file->data, count);
    file->data += count;
    file->pextension = (void *)((unsigned long)file->pextension - count);

    //
    // See if the "ABCD" string exists in this portion of the file data.
    //
    buffer = ustrstr(buffer, "ABCD");
    if(buffer)
    {
        //
        // Replace the "ABCD" string with the current access code.
        //
        buffer[0] = '0' + ((g_ulAccessCode >> 12) & 15);
        buffer[1] = '0' + ((g_ulAccessCode >> 8) & 15);
        buffer[2] = '0' + ((g_ulAccessCode >> 4) & 15);
        buffer[3] = '0' + (g_ulAccessCode & 15);
    }

    //
    // Return the count of data that we copied.
    //
    return(count);
}
