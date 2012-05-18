//*****************************************************************************
//
// os.cxx - The OS specific functions.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#ifdef __WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif
#include "os.h"

//*****************************************************************************
//
// This function is used to create a thread for the application.
//
//*****************************************************************************
void
OSThreadCreate(void *(WorkerThread)(void *pvData))
{
#ifdef __WIN32
    //
    // Create the requested thread.
    //
    _beginthread((void (*)(void *))WorkerThread, 0, 0);
#else
    pthread_t thread;

    //
    // Create the requested thread.
    //
    pthread_create(&thread, 0, WorkerThread, 0);
#endif
}

//*****************************************************************************
//
// This function is used to kill a thread for the application.
//
//*****************************************************************************
void
OSThreadExit(void)
{
#ifdef __WIN32
    _endthread();
#else
    pthread_exit(0);
#endif
}

//*****************************************************************************
//
// This function is used to sleep for a given number of seconds.
//
//*****************************************************************************
void
OSSleep(unsigned long ulSeconds)
{
#ifdef __WIN32
    Sleep(ulSeconds * 1000);
#else
    sleep(ulSeconds);
#endif
}
