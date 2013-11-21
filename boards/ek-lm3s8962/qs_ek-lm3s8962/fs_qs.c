//*****************************************************************************
//
// fs_qs.c - File system processing for the lwIP web server.
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "utils/lwiplib.h"
#include "httpserver_raw/httpd.h"
#include "httpserver_raw/fs.h"
#include "httpserver_raw/fsdata.h"
#include "audio.h"
#include "game.h"

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
// A local copy of the player position.  This ensures that the player does not
// move mid-transfer, possibly resulting in a wildly inaccurate position.
//
//*****************************************************************************
static unsigned char g_pucPlayer[4];

//*****************************************************************************
//
// A local copy of the monster positions.  This ensures that the monsters do
// not move mid-transfer, possibly resulting in wildly inaccurate positions.
//
//*****************************************************************************
static unsigned char g_pucMonsters[400];

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
    // See if the maze data file has been requested.
    //
    if(strncmp(name, "/maze.dat", 9) == 0)
    {
        //
        // Setup the file structure to return the maze data.
        //
        ptFile->data = (char *)g_ppcMaze;
        ptFile->len = sizeof(g_ppcMaze);
        ptFile->index = 0;
        ptFile->pextension = 0;

        //
        // Return the file system pointer.
        //
        return(ptFile);
    }

    //
    // See if the player position file has been requested.
    //
    if(strncmp(name, "/player.dat", 11) == 0)
    {
        //
        // Fill in the buffer containing the player position.
        //
        g_pucPlayer[0] = g_usPlayerX % 256;
        g_pucPlayer[1] = g_usPlayerX / 256;
        g_pucPlayer[2] = g_usPlayerY % 256;
        g_pucPlayer[3] = g_usPlayerY / 256;

        //
        // Setup the file structure to return the player position data.
        //
        ptFile->data = (char *)g_pucPlayer;
        ptFile->len = 4;
        ptFile->index = 0;
        ptFile->pextension = 0;

        //
        // Return the file system pointer.
        //
        return(ptFile);
    }

    //
    // See if the monster position file has been requested.
    //
    if(strncmp(name, "/monster.dat", 12) == 0)
    {
        unsigned long ulIdx;

        //
        // Loop through the monsters, filling in the buffer containing the
        // monster positions.
        //
        for(ulIdx = 0; ulIdx < 100; ulIdx++)
        {
            g_pucMonsters[(ulIdx * 4) + 0] = g_pusMonsterX[ulIdx] % 256;
            g_pucMonsters[(ulIdx * 4) + 1] = g_pusMonsterX[ulIdx] / 256;
            g_pucMonsters[(ulIdx * 4) + 2] = g_pusMonsterY[ulIdx] % 256;
            g_pucMonsters[(ulIdx * 4) + 3] = g_pusMonsterY[ulIdx] / 256;
        }

        //
        // Setup the file structure to return the monster position data.
        //
        ptFile->data = (char *)g_pucMonsters;
        ptFile->len = 400;
        ptFile->index = 0;
        ptFile->pextension = 0;

        //
        // Return the file system pointer.
        //
        return(ptFile);
    }

    //
    // See if the volume up file has been requested.
    //
    if(strncmp(name, "/volume_up.html", 15) == 0)
    {
        //
        // Turn up the volume.
        //
        AudioVolumeUp(10);

        //
        // Setup the file structure to return the volume page.
        //
        ptFile->data = NULL;
        ptFile->len = 0;
        ptFile->index = 0;
        ptFile->pextension = (void *)1;

        //
        // Return the file system pointer.
        //
        return(ptFile);
    }

    //
    // See if the volume down file has been requested.
    //
    if(strncmp(name, "/volume_down.html", 17) == 0)
    {
        //
        // Turn down the volume.
        //
        AudioVolumeDown(10);

        //
        // Setup the file structure to return the volume page.
        //
        ptFile->data = NULL;
        ptFile->len = 0;
        ptFile->index = 0;
        ptFile->pextension = (void *)1;

        //
        // Return the file system pointer.
        //
        return(ptFile);
    }

    //
    // See if the volume file has been requested.
    //
    if(strncmp(name, "/volume_get.html", 16) == 0)
    {
        //
        // Setup the file structure to return the volume page.
        //
        ptFile->data = NULL;
        ptFile->len = 0;
        ptFile->index = 0;
        ptFile->pextension = (void *)1;

        //
        // Return the file system pointer.
        //
        return(ptFile);
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
    int iAvailable;

    //
    // Check to see if an audio update was requested (pextension = 1).
    //
    if(file->pextension == (void *)1)
    {
        static const char pucTemp[] = "<html><body>   </body></html>";
        unsigned char ucTemp;

        //
        // Check to see if the buffer is large enough.
        //
        if(count < sizeof(pucTemp))
        {
            file->pextension = NULL;
            return(-1);
        }

        //
        // Copy the template into the buffer.
        //
        memcpy(buffer, pucTemp, sizeof(pucTemp));

        //
        // Get the current audio volume level.
        //
        ucTemp = AudioVolumeGet();

        //
        // Convert the audio volume into an ascii number.
        //
        if(ucTemp > 99)
        {
            buffer[12] = '0' + ucTemp / 100;
            buffer[13] = '0';
            buffer[14] = '0';
            ucTemp %= 100;
        }
        if(ucTemp > 9)
        {
            buffer[13] = '0' + ucTemp / 10;
            buffer[14] = '0';
            ucTemp %= 10;
        }
        buffer[14] = '0' + ucTemp;

        //
        // Clear the extension data and return the count.
        //
        file->pextension = NULL;
        return(sizeof(pucTemp));
    }

    //
    // Check to see if more data is available.
    //
    if(file->len == file->index)
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
    iAvailable = file->len - file->index;
    if(iAvailable > count)
    {
        iAvailable = count;
    }

    //
    // Copy the data.
    //
    memcpy(buffer, file->data + iAvailable, iAvailable);
    file->index += iAvailable;

    //
    // Return the count of data that we copied.
    //
    return(iAvailable);
}
