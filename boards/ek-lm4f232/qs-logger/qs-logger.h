//*****************************************************************************
//
// qs-logger.h - Defines data types used in the data logger application.
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#ifndef __QSLOGGER_H__
#define __QSLOGGER_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// The following defines the order of data items to log.  It must match the
// order that they appear in the "CHANNELS" menu (see menus.c), and also
// the order they are stored in the ADC data buffer (see acquire.c).
//
//*****************************************************************************
#define LOG_ITEM_USER0      0
#define LOG_ITEM_USER1      1
#define LOG_ITEM_USER2      2
#define LOG_ITEM_USER3      3
#define LOG_ITEM_ACCELX     4
#define LOG_ITEM_ACCELY     5
#define LOG_ITEM_ACCELZ     6
#define LOG_ITEM_EXTTEMP    7
#define LOG_ITEM_INTTEMP    8
#define LOG_ITEM_CURRENT    9
#define NUM_LOG_ITEMS       10

//*****************************************************************************
//
// These are additional definitions of items that may be displayed on the
// screen that are not acquired data.  These are used for updating dynamic
// text screens on the display.
//
//*****************************************************************************
#define TEXT_ITEM_STATUS1       10
#define TEXT_ITEM_STATUS2       11
#define TEXT_ITEM_STATUS3       12
#define TEXT_ITEM_STATUS_TITLE  13
#define TEXT_ITEM_DATE          14
#define TEXT_ITEM_TIME          15
#define NUM_TEXT_ITEMS          16

//*****************************************************************************
//
// A structure that defines a data record.  This is the binary format of the
// acquired data that will be stored.
//
//*****************************************************************************
typedef struct
{
    unsigned long ulSeconds;        // 32 bits of seconds
    unsigned short usSubseconds;    // 15 bits of subseconds (1 bit unused)
    unsigned short usItemMask;      // 16 bits means maximum 16 data items
    short sItems[1];
} tLogRecord;

//*****************************************************************************
//
// This structure defines a container to hold the state of all the
// configuration items.  It is used both for passing configuration
// between modules and for persistent storage of the configuration.
//
//*****************************************************************************
typedef struct
{
    //
    // A value used to identify this structure
    //
    unsigned long ulCookie;

    //
    // A flag to indicate if the data logger is current logging data
    // using the sleep mode.  The data logger uses this to determine if
    // it needs to continue taking samples once it wakes from hibernate.
    // This is a 32-bit type just to keep the entire struct 32-bit aligned.
    //
    unsigned long ulSleepLogging;

    //
    // The period for sampling data.  It is stored as a 24.8 seconds.frac
    // format.  The lower 8 bits represent power-of-2 fractional seconds
    // with a resolution of 1/128 seconds (only lower 7 bits are used).
    //
    unsigned long ulPeriod;

    //
    // Saved location for writing records to flash.
    //
    unsigned long ulFlashStore;

    //
    // The name of the USB file currently opened for logging
    //
    char cFilename[8];

    //
    // The bit mask of the channels selected for logging.
    //
    unsigned short usSelectedMask;

    //
    // A flag indicating whether the data logger should sleep between samples.
    //
    unsigned char bSleep;

    //
    // A value that is used to select the storage medium.
    //
    unsigned char ucStorage;

    //
    // A checksum for the structure
    //
    unsigned long ulCrc16;
} tConfigState;

#define STATE_COOKIE 0x0355AAC0

//*****************************************************************************
//
// The values indicating which storage medium is to be used for logging data.
//
//*****************************************************************************
#define CONFIG_STORAGE_NONE   0
#define CONFIG_STORAGE_USB    1
#define CONFIG_STORAGE_HOSTPC 2
#define CONFIG_STORAGE_FLASH  3
#define CONFIG_STORAGE_VIEWER 4
#define CONFIG_STORAGE_CHOICES    5

//*****************************************************************************
//
// Function prototypes.
//
//*****************************************************************************
extern void SetStatusText(const char *pcTitle, const char *pcLine1,
                          const char *pcLine2, const char *pcLine3);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif
