//*****************************************************************************
//
// lmi_fs.c - File System Processing for enet_io application.
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
// This is part of revision 9453 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/fs.h"
#include "httpserver_raw/fsdata.h"
#include "io.h"

//*****************************************************************************
//
// Include the file system data for this application.  This file is generated
// by the makefsfile utility, using the following command (all on one line):
//
//     makefsfile -i fs -o io_fsdata.h -r -h
//
// If any changes are made to the static content of the web pages served by the
// application, this script must be used to regenerate io_fsdata.h in order
// for those changes to be picked up by the web server.
//
//*****************************************************************************
#include "io_fsdata.h"

//*****************************************************************************
//
// Global Settings for demo page content.
//
//*****************************************************************************
static char g_cSampleTextBuffer[16] = {0};

//*****************************************************************************
//
// Open a file and return a handle to the file, if found.  Otherwise,
// return NULL.  This function also looks for special filenames used to
// provide specific status information or to control various subsystems.
// These filenames are used by the JavaScript on the "IO Control Demo 1"
// example web page.
//
//*****************************************************************************
struct fs_file *
fs_open(char *name)
{
    char *data;
    int i;
    const struct fsdata_file *ptTree;
    struct fs_file *ptFile = NULL;

    //
    // Allocate memory for the file system structure.
    //
    ptFile = mem_malloc(sizeof(struct fs_file));
    if(NULL == ptFile)
    {
        return(NULL);
    }

    //
    // Process request to toggle STATUS LED
    //
    if(strncmp(name, "/cgi-bin/toggle_led", 19) == 0)
    {
        //
        // Toggle the STATUS LED
        //
        io_set_led(!io_is_led_on());

        //
        // Setup the file structure to return whatever.
        //
        ptFile->data = NULL;
        ptFile->len = 0;
        ptFile->index = 0;
        ptFile->pextension = NULL;

        //
        // Return the file system pointer.
        //
        return(ptFile);
    }

    //
    // Process request to turn PWM ON/OFF
    //
    if(strncmp(name, "/cgi-bin/pwm_onoff", 18) == 0)
    {
        //
        // Turn PWM on/off
        //
        io_set_pwm(!io_is_pwm_on());

        //
        // Setup the file structure to return whatever.
        //
        ptFile->data = NULL;
        ptFile->len = 0;
        ptFile->index = 0;
        ptFile->pextension = NULL;

        //
        // Return the file system pointer.
        //
        return(ptFile);
    }

    //
    // Process request for PWM freq update
    //
    if(strncmp(name, "/pwm_freq?value=", 16) == 0)
    {
        //
        // Get Frequency String
        //
        data = name;
        data += 16;
        i = 0;
        do
        {
            switch(data[i])
            {
                case 0:
                case '&':
                    g_cSampleTextBuffer[i] = 0;
                    break;
                case '+':
                    g_cSampleTextBuffer[i] = ' ';
                    break;
                default:
                    g_cSampleTextBuffer[i] = data[i];
                    break;
            }
            if(g_cSampleTextBuffer[i] == 0)
            {
                break;
            }
            i++;
        }while(i < sizeof(g_cSampleTextBuffer));

        //
        // Set PWM Frequency
        //
        io_pwm_freq(ustrtoul(g_cSampleTextBuffer,NULL,10));

        //
        // Setup the file structure to return whatever.
        //
        ptFile->data = NULL;
        ptFile->len = 0;
        ptFile->index = 0;
        ptFile->pextension = NULL;

        //
        // Return the file system pointer.
        //
        return(ptFile);
    }

    //
    // Process request for PWM Duty Cycle update
    //
    if(strncmp(name, "/pwm_dutycycle?value=", 21) == 0)
    {
        //
        // Get Duty Cycle String
        //
        data = name;
        data += 21;
        i = 0;
        do
        {
            switch(data[i])
            {
                case 0:
                case '&':
                    g_cSampleTextBuffer[i] = 0;
                    break;
                case '+':
                    g_cSampleTextBuffer[i] = ' ';
                    break;
                default:
                    g_cSampleTextBuffer[i] = data[i];
                    break;
            }
            if(g_cSampleTextBuffer[i] == 0)
            {
                break;
            }
            i++;
        }while(i < sizeof(g_cSampleTextBuffer));

        //
        // Set PWM Duty Cycle
        //
        io_pwm_dutycycle(ustrtoul(g_cSampleTextBuffer,NULL,10));

        //
        // Setup the file structure to return whatever.
        //
        ptFile->data = NULL;
        ptFile->len = 0;
        ptFile->index = 0;
        ptFile->pextension = NULL;

        //
        // Return the file system pointer.
        //
        return(ptFile);
    }

    //
    // Request for LED State?
    //
    if(strncmp(name, "/ledstate?id", 12) == 0)
    {
        static char pcBuf[4];

        //
        // Get the state of the LED
        //
        io_get_ledstate(pcBuf, 4);

        ptFile->data = pcBuf;
        ptFile->len = strlen(pcBuf);
        ptFile->index = ptFile->len;
        ptFile->pextension = NULL;
        return(ptFile);
    }

    //
    // Request for PWM State?
    //
    if(strncmp(name, "/pwmstate?id", 12) == 0)
    {
        static char pcBuf[4];

        //
        // Get the state of the PWM
        //
        io_get_pwmstate(pcBuf, 4);

        ptFile->data = pcBuf;
        ptFile->len = strlen(pcBuf);
        ptFile->index = ptFile->len;
        ptFile->pextension = NULL;
        return(ptFile);
    }

    //
    // Request PWM Frequency?
    //
    if(strncmp(name, "/pwmfreqget?id", 14) == 0)
    {
        static char pcBuf[16];

        //
        // Get the frequency of the PWM
        //
        usprintf(pcBuf,"%d",io_get_pwmfreq());

        ptFile->data = pcBuf;
        ptFile->len = strlen(pcBuf);
        ptFile->index = ptFile->len;
        ptFile->pextension = NULL;
        return(ptFile);
    }

    //
    // Request PWM Duty Cycle?
    //
    if(strncmp(name, "/pwmdutycycleget?id", 19) == 0)
    {
        static char pcBuf[16];

        //
        // Get the duty cycle of te PWM
        //
        usprintf(pcBuf,"%d",io_get_pwmdutycycle());

        ptFile->data = pcBuf;
        ptFile->len = strlen(pcBuf);
        ptFile->index = ptFile->len;
        ptFile->pextension = NULL;
        return(ptFile);
    }

    //
    // If I can't find it there, look in the rest of the main file system
    //
    else
    {
        //
        // Initialize the file system tree pointer to the root of the linked list.
        //
        ptTree = FS_ROOT;

        //
        // Begin processing the linked list, looking for the requested file name.
        //
        while(NULL != ptTree)
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
    }

    //
    // If we didn't find the file, ptTee will be NULL.  Make sure we
    // return a NULL pointer if this happens.
    //
    if(NULL == ptTree)
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
    // Free the main file system object.
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
    // Check to see if a command (pextension = 1).
    //
    if(file->pextension == (void *)1)
    {
        //
        // Nothting to do for this file type.
        //
        file->pextension = NULL;
        return(-1);
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
