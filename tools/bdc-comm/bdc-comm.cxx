//*****************************************************************************
//
// bdc-comm.cxx - The main control loop for the bdc-comm application.
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

#include <libgen.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "can_proto.h"
#include "cmdline.h"
#include "os.h"
#include "uart_handler.h"
#include "gui.h"
#include "gui_handlers.h"

#ifdef __WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

#ifdef __WIN32
#define MUTEX HANDLE
#else
#define MUTEX pthread_mutex_t
#endif

//*****************************************************************************
//
// The global mutex used to protect the UART controller use by threads.
//
//*****************************************************************************
MUTEX mMutex;

#define MAX_CAN_ID              64


//*****************************************************************************
//
// This array holds the strings used for the currently defined Manufacturers.
//
//*****************************************************************************
static const char *g_ppcManufacturers[] =
{
    "none",
    "National Instruments",
    "Texas Instruments",
    "DEKA"
};

//*****************************************************************************
//
// This array holds the strings used for the currently defined device types.
//
//*****************************************************************************
static const char *g_ppcTypes[] =
{
    "none",
    "robot",
    "motor controller",
    "relay",
    "gyro",
    "accelerometer",
    "ultrasonic",
    "gear tooth"
};

//*****************************************************************************
//
// This variable is modified by a command line parameter to match the COM
// port that has been requested.
//
//*****************************************************************************
char g_szCOMName[32] =
{
#ifdef __WIN32
    "\\\\.\\COM1"
#else
    "/dev/ttyS0"
#endif
};

//*****************************************************************************
//
// The UART message buffer.
//
//*****************************************************************************
static unsigned char g_pucUARTTxMessage[516];
static unsigned long g_ulUARTMsgLen;

//*****************************************************************************
//
// These variables are used to hold the messages as they come in from the UART.
//
//*****************************************************************************
static unsigned char g_pucUARTMessage[12];
static unsigned long g_ulUARTSize;
static unsigned long g_ulUARTLength;

//*****************************************************************************
//
// The current UART state and its global variable.
//
//*****************************************************************************
#define UART_STATE_IDLE         0
#define UART_STATE_LENGTH       1
#define UART_STATE_DATA         2
#define UART_STATE_ESCAPE       3
static unsigned long g_ulUARTState = UART_STATE_IDLE;

//*****************************************************************************
//
// The current Device ID in use.
//
//*****************************************************************************
unsigned long g_ulID;

//*****************************************************************************
//
// Holds if the heart beat is enabled or not.
//
//*****************************************************************************
unsigned long g_ulHeartbeat = 1;

//*****************************************************************************
//
// Indicates if the device is currently active.
//
//*****************************************************************************
unsigned long g_ulBoardStatus = 0;
bool g_bBoardStatusActive = false;

//*****************************************************************************
//
// This value is true if GUI is in use and false if the command line interaface
// is being used.
//
//*****************************************************************************
bool g_bUseGUI = true;

//*****************************************************************************
//
// This value is true if the application is currently connected to the serial
// port.
//
//*****************************************************************************
bool g_bConnected = false;

//*****************************************************************************
//
// This value is true if there is currently a synchronous update pending.
//
//*****************************************************************************
bool g_bSynchronousUpdate = false;

//*****************************************************************************
//
// The current Maximum output voltage.
//
//*****************************************************************************
double g_dMaxVout;

//*****************************************************************************
//
// The current Vbus output voltage.
//
//*****************************************************************************
double g_dVbus;

//*****************************************************************************
//
// These values are used to hold the "faked" command line arguments that are
// passed into the command line hander functions.
//
//*****************************************************************************
static char g_pArg1[256];
static char g_pArg2[256];
static char g_pArg3[256];
static char g_pArg4[256];
static char g_pArg5[256];
static char g_pArg6[256];
static char g_pArg7[256];
static char g_pArg8[256];
static char g_pArg9[256];
static char g_pArg10[256];
static char g_pArg11[256];
char *g_argv[11] =
{
    g_pArg1, g_pArg2, g_pArg3, g_pArg4, g_pArg5, g_pArg6, g_pArg7, g_pArg8,
        g_pArg9, g_pArg10, g_pArg11
};

//*****************************************************************************
//
// The current fault status string.
//
//*****************************************************************************
char g_pcFaultTxt[16];

//*****************************************************************************
//
// A flag used to avoid reporting COMM faults under startup conditions.
//
//*****************************************************************************
static bool g_bIgnoreCOMM = false;

//*****************************************************************************
//
// This is used to range check values from the console. It should be set to the
// last valid PSTAT message byte ID (in can_proto.h).
//
//*****************************************************************************
#define PSTATUS_MAX_ID  LM_PSTAT_CANERR_B1

//*****************************************************************************
//
// The usage function for the application.
//
//*****************************************************************************
void
Usage(char *pcFilename)
{
    fprintf(stdout, "Usage: %s [OPTION]\n", basename(pcFilename));
    fprintf(stdout, "A simple command-line interface to a Jaguar.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Options are:\n");
#ifdef __WIN32
    fprintf(stderr, "  -c NUM   The number of the COM port to use"
                    " (default: COM1)\n");
#else
    fprintf(stderr, "  -c TTY   The name of the TTY device to use"
                    " (default: /dev/ttyS0)\n");
#endif
    fprintf(stderr, "\n");
    fprintf(stderr, "Report bugs to <support_lmi@ti.com>.\n");
}

//*****************************************************************************
//
// This function is used to create the MUTEX used by the application.
//
//*****************************************************************************
int
MutexInit(MUTEX *mutex)
{
#ifdef __WIN32
    //
    // Use the Windows call to create a mutex.
    //
    *mutex = CreateMutex(0, FALSE, 0);

    return(*mutex == 0);
#else
    return(pthread_mutex_init(mutex, NULL));
#endif
}

//*****************************************************************************
//
// Take the mutex and wait forever until it is released.
//
//*****************************************************************************
int
MutexLock(MUTEX *mutex)
{
    //
    // Wait for the mutex to be released.
    //
#ifdef __WIN32
    if(WaitForSingleObject(*mutex, INFINITE) == WAIT_FAILED)
    {
        return(1);
    }
    else
    {
        return(0);
    }
#else
    return(pthread_mutex_lock(mutex));
#endif
}

//*****************************************************************************
//
// Release the mutex.
//
//*****************************************************************************
int
MutexUnlock(MUTEX *mutex)
{
    //
    // Release the mutex.
    //
#ifdef __WIN32
    return(ReleaseMutex(*mutex) == 0);
#else
    return(pthread_mutex_unlock(mutex));
#endif
}

//*****************************************************************************
//
// Sends a character to the UART.
//
//*****************************************************************************
static void
UARTPutChar(unsigned long ulChar)
{
    //
    // See if the character being sent is 0xff.
    //
    if(ulChar == 0xff)
    {
        //
        // Send 0xfe 0xfe, the escaped version of 0xff.  A sign extended
        // version of 0xfe is used to avoid the check below for 0xfe, thereby
        // avoiding an infinite loop.  Only the lower 8 bits are actually sent,
        // so 0xfe is what is actually transmitted.
        //
        g_pucUARTTxMessage[g_ulUARTMsgLen++] = 0xfe;
        g_pucUARTTxMessage[g_ulUARTMsgLen++] = 0xfe;
    }

    //
    // Otherwise, see if the character being sent is 0xfe.
    //
    else if(ulChar == 0xfe)
    {
        //
        // Send 0xfe 0xfd, the escaped version of 0xfe.  A sign extended
        // version of 0xfe is used to avoid the check above for 0xfe, thereby
        // avoiding an infinite loop.  Only the lower 8 bits are actually sent,
        // so 0xfe is what is actually transmitted.
        //
        g_pucUARTTxMessage[g_ulUARTMsgLen++] = 0xfe;
        g_pucUARTTxMessage[g_ulUARTMsgLen++] = 0xfd;
    }

    //
    // Otherwise, simply send this character.
    //
    else
    {
        g_pucUARTTxMessage[g_ulUARTMsgLen++] = ulChar;
    }
}

//*****************************************************************************
//
// Sends a message to the UART.
//
//*****************************************************************************
static void
UARTSendMessage(unsigned long ulID, unsigned char *pucData,
                unsigned long ulDataLength)
{
    //
    // Lock the resource.
    //
    MutexLock(&mMutex);

    //
    // Send the start of packet indicator.  A sign extended version of 0xff is
    // used to avoid having it escaped.
    //
    g_ulUARTMsgLen = 0;
    UARTPutChar(0xffffffff);

    //
    // Send the length of the data packet.
    //
    UARTPutChar(ulDataLength + 4);

    //
    // Send the message ID.
    //
    UARTPutChar(ulID & 0xff);
    UARTPutChar((ulID >> 8) & 0xff);
    UARTPutChar((ulID >> 16) & 0xff);
    UARTPutChar((ulID >> 24) & 0xff);

    //
    // Send the associated data, if any.
    //
    while(ulDataLength--)
    {
        UARTPutChar(*pucData++);
    }

    //
    // Send the constructed message.
    //
    UARTSendData(g_pucUARTTxMessage, g_ulUARTMsgLen);

    //
    // Release the mutex so that other threads can access the UART TX path.
    //
    MutexUnlock(&mMutex);
}

//*****************************************************************************
//
// Parse the UART response message from the network.
//
//*****************************************************************************
void
ParseResponse(void)
{
    unsigned long ulID;
    long lValue, lValueFractional, lValueOriginal;
    char pcTempString[100];
    double dValue;
    int iDevice, iTemp;
    int iIdx;
    bool bFound = false;

    //
    // Get the device number out of the first byte of the message.
    //
    ulID = *(unsigned long *)g_pucUARTMessage;
    iDevice = ulID & CAN_MSGID_DEVNO_M;

    //
    // Read the actual command out of the message.
    //
    switch(ulID & ~(CAN_MSGID_DEVNO_M))
    {
        //
        // Handle the device enumeration command.
        //
        case CAN_MSGID_API_ENUMERATE:
        {
            if(g_bUseGUI)
            {
                sprintf(pcTempString, "%ld", ulID & CAN_MSGID_DEVNO_M);

                //
                // Add the board to the drop down list.
                //
                g_pSelectBoard->add(pcTempString);
            }
            else
            {
                printf("system enum = %ld\n", ulID & CAN_MSGID_DEVNO_M);
            }

            break;
        }

        //
        // Handle the firmware version request.
        //
        case CAN_MSGID_API_FIRMVER:
        {
            if(g_bUseGUI)
            {
                g_pSystemFirmwareVer->value(
                    *(unsigned long *)(g_pucUARTMessage + 4));
            }
            else
            {
                printf("firmware version (%ld) = %ld\n",
                       ulID & CAN_MSGID_DEVNO_M,
                       *(unsigned long *)(g_pucUARTMessage + 4));
            }

            break;
        }

        //
        // Handle the hardware version request.
        //
        case LM_API_HWVER:
        {
            if(g_bUseGUI)
            {
                g_pSystemHardwareVer->value(
                    *(unsigned char *)(g_pucUARTMessage + 5));
            }
            else
            {
                printf("hardware version (%ld) = %2d\n",
                       ulID & CAN_MSGID_DEVNO_M,
                       *(unsigned char *)(g_pucUARTMessage + 5));
            }

            break;
        }

        //
        // Handle the device query request.
        //
        case CAN_MSGID_API_DEVQUERY:
        {
            if(g_bUseGUI)
            {
                sprintf(pcTempString, "%s, %s",
                g_ppcManufacturers[g_pucUARTMessage[5]],
                g_ppcTypes[g_pucUARTMessage[4]]);

                g_pSystemBoardInformation->value(pcTempString);
            }
            else
            {
                printf("system query (%ld) = %s, %s\n",
                       ulID & CAN_MSGID_DEVNO_M,
                       g_ppcManufacturers[g_pucUARTMessage[5]],
                       g_ppcTypes[g_pucUARTMessage[4]]);
            }

            break;
        }

        //
        // Handle the Voltage mode set request.
        //
        case LM_API_VOLT_SET:
        {
            lValue = *(short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                //
                // Apply the scaling factor so that the Vout is within range.
                //
                if(lValue < 0)
                {
                    lValue = ((lValue * 100) - 16384) / 32767;
                }
                else
                {
                    lValue = ((lValue * 100) + 16384) / 32767;
                }

                //
                // Update the GUI with the values.
                //
                g_pModeSetBox->value((double)lValue);
                g_pModeSetSlider->value(lValue);
            }
            else
            {
                printf("volt set (%ld) = %ld\n", ulID & CAN_MSGID_DEVNO_M,
                       lValue);
            }

            break;
        }

        //
        // Handle the Voltage mode set ramp rate request.
        //
        case LM_API_VOLT_SET_RAMP:
        {
            lValue = *(short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                g_pModeRamp->value(lValue);
            }
            else
            {
                printf("volt ramp (%ld) = %ld\n", ulID & CAN_MSGID_DEVNO_M,
                       lValue);
            }

            break;
        }

        //
        // Handle the voltage compensation mode set request.
        //
        case LM_API_VCOMP_SET:
        {
            lValue = *(short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 128 / 100;
                }
                else
                {
                    lValue += 128 / 100;
                }

                sprintf(pcTempString, "%s%ld.%02ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 256) : (lValue / 256),
                        (lValue < 0) ?
                        ((((0 - lValue) % 256) * 100) / 256) :
                        (((lValue % 256) * 100) / 256));

                //
                // Convert the string to a float.
                //
                dValue = strtod(pcTempString, NULL);

                //
                // Update the GUI with the values.
                //
                g_pModeSetBox->value(dValue);
                g_pModeSetSlider->value(dValue);
            }
            else
            {
                printf("vcomp set (%ld) = %ld (%s%ld.%02ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 256) : (lValue / 256),
                       (lValue < 0) ?
                       ((((0 - lValue) % 256) * 100) / 256) :
                       (((lValue % 256) * 100) / 256));
            }

            break;
        }

        //
        // Handle the voltage compensation mode target voltage ramp request.
        //
        case LM_API_VCOMP_IN_RAMP:
        {
            lValue = *(short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                g_pModeRamp->value(lValue);
            }
            else
            {
                printf("vcomp ramp (%ld) = %ld\n", ulID & CAN_MSGID_DEVNO_M,
                       lValue);
            }

            break;
        }

        //
        // Handle the voltage compensation mode compensation ramp request.
        //
        case LM_API_VCOMP_COMP_RAMP:
        {
            lValue = *(short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                g_pModeCompRamp->value(lValue);
            }
            else
            {
                printf("vcomp comp (%ld) = %ld\n", ulID & CAN_MSGID_DEVNO_M,
                       lValue);
            }

            break;
        }

        //
        // Handle the Current control mode enable request.
        //
        case LM_API_ICTRL_SET:
        {
            lValue = *(short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 128 / 10;
                }
                else
                {
                    lValue += 128 / 10;
                }

                sprintf(pcTempString, "%s%ld.%01ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 256) : (lValue / 256),
                        (lValue < 0) ?
                        ((((0 - lValue) % 256) * 10) / 256) :
                        (((lValue % 256) * 10) / 256));

                //
                // Convert the string to a float.
                //
                dValue = strtod(pcTempString, NULL);

                //
                // Update the GUI with the values.
                //
                g_pModeSetBox->value(dValue);
                g_pModeSetSlider->value(dValue);
            }
            else
            {
                printf("cur set (%ld) = %ld (%s%ld.%02ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 256) : (lValue / 256),
                       (lValue < 0) ?
                       ((((0 - lValue) % 256) * 100) / 256) :
                       (((lValue % 256) * 100) / 256));
            }

            break;
        }

        //
        // Handle the Current control mode P parameter set request.
        //
        case LM_API_ICTRL_PC:
        {
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                dValue = strtod(pcTempString, NULL);
                g_pModeP->value(dValue);
            }
            else
            {
                printf("cur p (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) : (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the Current control mode I parameter set request.
        //
        case LM_API_ICTRL_IC:
        {
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                dValue = strtod(pcTempString, NULL);
                g_pModeI->value(dValue);
            }
            else
            {
                printf("cur i (%ld) = %ld (%s%ld.%03ld)\n",
                        ulID & CAN_MSGID_DEVNO_M, lValue,
                        (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the Current control mode D parameter set request.
        //
        case LM_API_ICTRL_DC:
        {
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                dValue = strtod(pcTempString, NULL);
                g_pModeD->value(dValue);
            }
            else
            {
                printf("cur d (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) : (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the Speed mode enable request.
        //
        case LM_API_SPD_SET:
        {
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                sprintf(pcTempString, "%ld", lValue / 65536);

                //
                // Convert the string to a float.
                //
                dValue = strtod(pcTempString, NULL);

                //
                // Update the GUI with the values.
                //
                g_pModeSetBox->value(dValue);
                g_pModeSetSlider->value(dValue);
            }
            else
            {
                printf("speed set (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) : (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the Speed control mode P parameter set request.
        //
        case LM_API_SPD_PC:
        {
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                dValue = strtod(pcTempString, NULL);
                g_pModeP->value(dValue);
            }
            else
            {
                printf("speed p (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) : (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the Speed control mode I parameter set request.
        //
        case LM_API_SPD_IC:
        {
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                dValue = strtod(pcTempString, NULL);

                g_pModeI->value(dValue);
            }
            else
            {
                printf("speed i (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) :
                                       (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the Speed control mode D parameter set request.
        //
        case LM_API_SPD_DC:
        {
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                dValue = strtod(pcTempString, NULL);
                g_pModeD->value(dValue);
            }
            else
            {
                printf("speed d (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) : (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the Speed control mode speed reference set request.
        //
        case LM_API_SPD_REF:
        {
            if(g_bUseGUI)
            {
                if((g_pucUARTMessage[4] < 4) && (g_pucUARTMessage[4] != 1))
                {
                    g_pReference->value(g_pucUARTMessage[4]);
                }
                else
                {
                    g_pReference->value(-1);
                }
            }
            else
            {
                printf("speed ref (%ld) = %d\n", ulID & CAN_MSGID_DEVNO_M,
                       g_pucUARTMessage[4]);
            }

            break;
        }

        //
        // Handle the Position control mode position set request.
        //
        case LM_API_POS_SET:
        {
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                //
                // Convert the string to a float.
                //
                dValue = strtod(pcTempString, NULL);

                //
                // Update the GUI with the values.
                //
                g_pModeSetBox->value(dValue);
                g_pModeSetSlider->value(dValue);
            }
            else
            {
                printf("pos set (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) : (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the Position control mode P parameter set request.
        //
        case LM_API_POS_PC:
        {
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                dValue = strtod(pcTempString, NULL);

                g_pModeP->value(dValue);
            }
            else
            {
                printf("pos p (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) : (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the Position control mode I parameter set request.
        //
        case LM_API_POS_IC:
        {
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                dValue = strtod(pcTempString, NULL);

                g_pModeI->value(dValue);
            }
            else
            {
                printf("pos i (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) : (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the Position control mode D parameter set request.
        //
        case LM_API_POS_DC:
        {
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                dValue = strtod(pcTempString, NULL);

                g_pModeD->value(dValue);
            }
            else
            {
                printf("pos d (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) : (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the Position control mode position reference set request.
        //
        case LM_API_POS_REF:
        {
            if(g_bUseGUI)
            {
                if(g_pucUARTMessage[4] < 2)
                {
                    g_pReference->value(g_pucUARTMessage[4]);
                }
                else if(g_pucUARTMessage[4] < 4)
                {
                    g_pReference->value(0);
                }
                else
                {
                    g_pReference->value(-1);
                }
            }
            else
            {
                printf("pos ref (%ld) = %d\n", ulID & CAN_MSGID_DEVNO_M,
                       g_pucUARTMessage[4]);
            }

            break;
        }

        //
        // Handle the get Output Voltage request.
        //
        case LM_API_STATUS_VOLTOUT:
        {
            lValue = *(short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                //
                // Convert the output voltage percentage into the "actual"
                // output voltage.
                //
                dValue = (((float)lValue * g_dVbus * g_dMaxVout) /
                          (32767 * 100));

                //
                // Update the status structure with the new value.
                //
                g_sBoardStatus.fVout = dValue;
            }
            else
            {
                printf("stat vout (%ld) = %ld\n", ulID & CAN_MSGID_DEVNO_M,
                       lValue);
            }

            break;
        }

        //
        // Handle the get Bus Voltage request.
        //
        case LM_API_STATUS_VOLTBUS:
        {
            //
            // Grab the response data and store it into a single variable.
            //
            lValue = *(unsigned short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                //
                // Get the number into the xx.xx format for display.
                //
                sprintf(pcTempString, "%ld.%02ld", lValue / 256,
                        ((lValue % 256) * 100) / 256);

                //
                // Convert the string to a float.
                //
                dValue = strtod(pcTempString, NULL);

                //
                // Update the global value.
                //
                g_dVbus = dValue;

                //
                // Update the status structure with the new value.
                //
                g_sBoardStatus.fVbus = dValue;
            }
            else
            {
                //
                // Update the window with the new value.
                //
                printf("stat vbus (%ld) = %ld (%ld.%02ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue, lValue / 256,
                       ((lValue % 256) * 100) / 256);
            }

            break;
        }

        //
        // Handle the get Fault status request.
        //
        case LM_API_STATUS_FAULT:
        {
            //
            // Grab the response data and store it into a single variable.
            //
            lValue = *(unsigned short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                g_sBoardStatus.lFault = lValue;
            }
            else
            {
                printf("stat fault (%ld) = %ld\n", ulID & CAN_MSGID_DEVNO_M,
                       lValue);
            }

            break;
        }

        //
        // Handle the get Sticky Fault status request.
        //
        case LM_API_STATUS_STKY_FLT:
        {
            //
            // Grab the response data and store it into a single variable.
            //
            lValue = *(unsigned short *)(g_pucUARTMessage + 4);

            //
            // Check for the IgnoreComm flag.
            //
            if(g_bIgnoreCOMM == true)
            {
                //
                // Mask out the COMM flag and reset the condition.
                //
                lValue &= ~(LM_FAULT_COMM);
                g_bIgnoreCOMM = false;
            }

            if(g_bUseGUI)
            {
                g_sBoardState[g_ulID].ulStkyFault |= lValue;
            }
            else
            {
                printf("stat stkyfault (%ld) = %ld\n", ulID & CAN_MSGID_DEVNO_M,
                       lValue);
            }

            break;
        }

        //
        // Handle the get Current request.
        //
        case LM_API_STATUS_CURRENT:
        {
            //
            // Grab the response data and store it into a single variable.
            //
            lValue = *(short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                //
                // Get the number into the xx.xx format for display.
                //
                sprintf(pcTempString, "%ld.%01ld", lValue / 256,
                        ((lValue % 256) * 100) / 256);

                //
                // Convert the string to a float.
                //
                dValue = strtod(pcTempString, NULL);

                //
                // Update the status structure with the new value.
                //
                g_sBoardStatus.fCurrent = dValue;
            }
            else
            {
                //
                // Update the window with the new value.
                //
                printf("stat cur (%ld) = %ld (%ld.%01ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue, lValue / 256,
                       ((lValue % 256) * 100) / 256);
            }

            break;
        }

        //
        // Handle the get Temperature request.
        //
        case LM_API_STATUS_TEMP:
        {
            //
            // Grab the response data and store it into a single variable.
            //
            lValue = *(short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                //
                // Update the status structure with the new value.
                //
                g_sBoardStatus.fTemperature = (float)lValue / 256.0;
            }
            else
            {
                //
                // Update the window with the new value.
                //
                printf("stat temp (%ld) = %ld (%ld.%02ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue, lValue / 256,
                       ((lValue % 256) * 100) / 256);
            }

            break;
        }

        //
        // Handle the get Position request.
        //
        case LM_API_STATUS_POS:
        {
            //
            // Grab the response data and store it into a single variable.
            //
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                //
                // Get the number into the xx.xx format for display.
                //
                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                        (lValue / 65536), (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                //
                // Convert the string to a float.
                //
                dValue = strtod(pcTempString, NULL);

                //
                // Update the status structure with the new value.
                //
                g_sBoardStatus.fPosition = dValue;
            }
            else
            {
                //
                // Update the window with the new value.
                //
                printf("stat pos (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) :
                                       (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the get Speed request.
        //
        case LM_API_STATUS_SPD:
        {
            //
            // Grab the response data and store it into a single variable.
            //
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                //
                // Get the number into the xx.xx format for display.
                //
                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                        (lValue / 65536), (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                //
                // Convert the string to a float.
                //
                dValue = strtod(pcTempString, NULL);

                //
                // Update the status structure with the new value.
                //
                g_sBoardStatus.fSpeed = dValue;
            }
            else
            {
                //
                // Update the window with the new value.
                //
                printf("stat speed (%ld) = %ld (%s%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 65536) : (lValue / 65536),
                       (lValue < 0) ?
                       ((((0 - lValue) % 65536) * 1000) / 65536) :
                       (((lValue % 65536) * 1000) / 65536));
            }

            break;
        }

        //
        // Handle the get limit values request.
        //
        case LM_API_STATUS_LIMIT:
        {
            if(g_bUseGUI)
            {
                //
                // Clear the non-sticky status bits (reverse logic).
                //
                g_sBoardState[g_ulID].ucLimits |= (LM_STATUS_LIMIT_FWD |
                    LM_STATUS_LIMIT_REV | LM_STATUS_LIMIT_SFWD |
                    LM_STATUS_LIMIT_SREV);

                //
                // AND in Sticky (additive) and non-sticky status updates.
                //
                g_sBoardState[g_ulID].ucLimits &= (unsigned char)g_pucUARTMessage[4];
            }
            else
            {
                //
                // Update the window with the new values.
                //
                printf("stat limit (%ld) = %c%c\n",
                       ulID & CAN_MSGID_DEVNO_M,
                       (g_pucUARTMessage[4] & LM_STATUS_LIMIT_FWD) ? '.' : 'F',
                       (g_pucUARTMessage[4] & LM_STATUS_LIMIT_REV) ? '.' : 'R');

                //
                // If set, show soft limits.
                //
                if(!(g_pucUARTMessage[4] & LM_STATUS_LIMIT_SFWD) ||
                        !(g_pucUARTMessage[4] & LM_STATUS_LIMIT_SREV))
                {
                    printf("stat softlimit (%ld) = %c%c\n",
                           ulID & CAN_MSGID_DEVNO_M,
                           (g_pucUARTMessage[4] & LM_STATUS_LIMIT_SFWD) ?
                                    '.' : 'F',
                           (g_pucUARTMessage[4] & LM_STATUS_LIMIT_SREV) ?
                                    '.' : 'R');
                }

                //
                // If set, show sticky limits.
                //
                if(!(g_pucUARTMessage[4] & LM_STATUS_LIMIT_STKY_FWD) ||
                        !(g_pucUARTMessage[4] & LM_STATUS_LIMIT_STKY_REV))
                {
                    printf("stat sticky-limit (%ld) = %c%c\n",
                           ulID & CAN_MSGID_DEVNO_M,
                           (g_pucUARTMessage[4] & LM_STATUS_LIMIT_STKY_FWD) ?
                                    '.' : 'F',
                           (g_pucUARTMessage[4] & LM_STATUS_LIMIT_STKY_REV) ?
                                    '.' : 'R');
                }

                //
                // If set, show sticky soft limits.
                //
                if(!(g_pucUARTMessage[4] & LM_STATUS_LIMIT_STKY_SFWD) ||
                        !(g_pucUARTMessage[4] & LM_STATUS_LIMIT_STKY_SREV))
                {
                    printf("stat sticky-softlimit (%ld) = %c%c\n",
                           ulID & CAN_MSGID_DEVNO_M,
                           (g_pucUARTMessage[4] &
                                LM_STATUS_LIMIT_STKY_SFWD) ? '.' : 'F',
                           (g_pucUARTMessage[4] &
                                LM_STATUS_LIMIT_STKY_SREV) ? '.' : 'R');
                }
            }

            break;
        }

        //
        // Handle the get Power status request.
        //
        case LM_API_STATUS_POWER:
        {
            if(g_bUseGUI)
            {
                //
                // Update the status structure with the new value.
                //
                g_sBoardStatus.lPower = (long)g_pucUARTMessage[4];
            }
            else
            {
                //
                // Update the window with the new value.
                //
                printf("stat power (%ld) = %d\n", ulID & CAN_MSGID_DEVNO_M,
                       g_pucUARTMessage[4]);
            }

            break;
        }

        //
        // Handle the get status for various the various control modes.
        //
        case LM_API_STATUS_CMODE:
        {
            if(g_bUseGUI)
            {
                //
                // Update the status structure with the new value.
                //
                g_sBoardState[g_ulID].ulControlMode = g_pucUARTMessage[4];

                //
                // Set the state of the mode select drop-down.
                //
                g_pSelectMode->value(g_pucUARTMessage[4]);
            }
            else
            {
                switch(g_pucUARTMessage[4])
                {
                    //
                    // Indicate Voltage control mode.
                    //
                    case LM_STATUS_CMODE_VOLT:
                    {
                        printf("Control Mode (%ld) = Voltage\n",
                               ulID & CAN_MSGID_DEVNO_M);

                        break;
                    }

                    //
                    // Indicate voltage compensation mode.
                    //
                    case LM_STATUS_CMODE_VCOMP:
                    {
                        printf("Control Mode (%ld) = Voltage Compensation\n",
                               ulID & CAN_MSGID_DEVNO_M);

                        break;
                    }

                    //
                    // Indicate Current control mode.
                    //
                    case LM_STATUS_CMODE_CURRENT:
                    {
                        printf("Control Mode (%ld) = Current\n",
                               ulID & CAN_MSGID_DEVNO_M);

                        break;
                    }

                    //
                    // Indicate Speed control mode.
                    //
                    case LM_STATUS_CMODE_SPEED:
                    {
                        printf("Control Mode (%ld) = Speed\n",
                               ulID & CAN_MSGID_DEVNO_M);

                        break;
                    }

                    //
                    // Indicate Position control mode.
                    //
                    case LM_STATUS_CMODE_POS:
                    {
                        printf("Control Mode (%ld) = Position\n",
                               ulID & CAN_MSGID_DEVNO_M);

                        break;
                    }

                    //
                    // Indicate unknown control mode.
                    //
                    default:
                    {
                        printf("Control Mode (%ld) = Unknown\n",
                               ulID & CAN_MSGID_DEVNO_M);

                        break;
                    }
                }
            }

            break;
        }

        //
        // Handle the get Output Voltage request.
        //
        case LM_API_STATUS_VOUT:
        {
            lValue = *(short *)(g_pucUARTMessage + 4);

            if(!g_bUseGUI)
            {
                printf("stat vout2 (%ld) = %ld (%s%ld.%02ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (lValue < 0) ? "-" : "",
                       (lValue < 0) ? ((0 - lValue) / 256) : (lValue / 256),
                       (lValue < 0) ?
                       ((((0 - lValue) % 256) * 100) / 256) :
                       (((lValue % 256) * 100) / 256));
            }

            break;
        }

        //
        // Handle the get Encoder Number of Lines request.
        //
        case LM_API_CFG_ENC_LINES:
        {
            if(g_bUseGUI)
            {
                g_pConfigEncoderLines->value(
                    (double)(*(unsigned short *)(g_pucUARTMessage + 4)));
            }
            else
            {
                printf("config lines (%ld) = %d\n", ulID & CAN_MSGID_DEVNO_M,
                       *(unsigned short *)(g_pucUARTMessage + 4));
            }

            break;
        }

        //
        // Handle the get Number of Pot Turns request.
        //
        case LM_API_CFG_POT_TURNS:
        {
            if(g_bUseGUI)
            {
                g_pConfigPOTTurns->value(
                    (double)(*(unsigned short *)(g_pucUARTMessage + 4)));
            }
            else
            {
                printf("config turns (%ld) = %d\n", ulID & CAN_MSGID_DEVNO_M,
                       *(unsigned short *)(g_pucUARTMessage + 4));
            }

            break;
        }

        //
        // Handle the Coast Break response.
        //
        case LM_API_CFG_BRAKE_COAST:
        {
            if(!g_bUseGUI)
            {
                printf("config brake (%ld) = ", ulID & CAN_MSGID_DEVNO_M);
            }

            if(g_pucUARTMessage[4] == 0)
            {
                if(g_bUseGUI)
                {
                    g_pConfigStopJumper->value(1);
                    g_pConfigStopBrake->value(0);
                    g_pConfigStopCoast->value(0);
                }
                else
                {
                    printf("jumper\n");
                }
            }
            else if(g_pucUARTMessage[4] == 1)
            {
                if(g_bUseGUI)
                {
                    g_pConfigStopJumper->value(0);
                    g_pConfigStopBrake->value(1);
                    g_pConfigStopCoast->value(0);
                }
                else
                {
                    printf("brake\n");
                }
            }
            else if(g_pucUARTMessage[4] == 2)
            {
                if(g_bUseGUI)
                {
                    g_pConfigStopJumper->value(0);
                    g_pConfigStopBrake->value(0);
                    g_pConfigStopCoast->value(1);
                }
                else
                {
                    printf("coast\n");
                }
            }
            else if(!g_bUseGUI)
            {
                printf("???\n");
            }

            break;
        }

        //
        // Handle the Limit switch mode response.
        //
        case LM_API_CFG_LIMIT_MODE:
        {
            if(!g_bUseGUI)
            {
                printf("config limit (%ld) = ", ulID & CAN_MSGID_DEVNO_M);
            }

            if(g_pucUARTMessage[4] == 0)
            {
                if(g_bUseGUI)
                {
                    g_pConfigLimitSwitches->value(0);
                    g_pConfigFwdLimitLt->deactivate();
                    g_pConfigFwdLimitGt->deactivate();
                    g_pConfigFwdLimitValue->deactivate();
                    g_pConfigRevLimitLt->deactivate();
                    g_pConfigRevLimitGt->deactivate();
                    g_pConfigRevLimitValue->deactivate();
                }
                else
                {
                    printf("off\n");
                }
            }
            else if(g_pucUARTMessage[4] == 1)
            {
                if(g_bUseGUI)
                {
                    g_pConfigLimitSwitches->value(1);
                    g_pConfigFwdLimitLt->activate();
                    g_pConfigFwdLimitGt->activate();
                    g_pConfigFwdLimitValue->activate();
                    g_pConfigRevLimitLt->activate();
                    g_pConfigRevLimitGt->activate();
                    g_pConfigRevLimitValue->activate();
                }
                else
                {
                    printf("on\n");
                }
            }
            else if(!g_bUseGUI)
            {
                printf("???\n");
            }

            break;
        }

        //
        // Handle the get Forward Limit response.
        //
        case LM_API_CFG_LIMIT_FWD:
        {
            //
            // Grab the response data and store it into a single variable.
            //
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                //
                // Convert the string to a float.
                //
                dValue = strtod(pcTempString, NULL);

                g_pConfigFwdLimitValue->value(dValue);

                if(g_pucUARTMessage[8] == 0)
                {
                    g_pConfigFwdLimitLt->value(1);
                    g_pConfigFwdLimitGt->value(0);
                }
                else
                {
                    g_pConfigFwdLimitLt->value(0);
                    g_pConfigFwdLimitGt->value(1);
                }
            }
            else
            {
                printf("config limit fwd (%ld) = %ld %s\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (g_pucUARTMessage[8] == 0) ? "lt" : "gt");
            }

            break;
        }

        //
        // Handle the get Reverse Limit response.
        //
        case LM_API_CFG_LIMIT_REV:
        {
            //
            // Grab the response data and store it into a single variable.
            //
            lValue = *(long *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue -= 32768 / 1000;
                }
                else
                {
                    lValue += 32768 / 1000;
                }

                sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 65536) :
                                        (lValue / 65536),
                        (lValue < 0) ?
                        ((((0 - lValue) % 65536) * 1000) / 65536) :
                        (((lValue % 65536) * 1000) / 65536));

                //
                // Convert the string to a float.
                //
                dValue = strtod(pcTempString, NULL);

                g_pConfigRevLimitValue->value(dValue);

                if(g_pucUARTMessage[8] == 0)
                {
                    g_pConfigRevLimitLt->value(1);
                    g_pConfigRevLimitGt->value(0);
                }
                else
                {
                    g_pConfigRevLimitLt->value(0);
                    g_pConfigRevLimitGt->value(1);
                }
            }
            else
            {
                printf("config limit rev (%ld) = %ld %s\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue,
                       (g_pucUARTMessage[8] == 0) ? "lt" : "gt");
            }

            break;
        }

        //
        // Handle the get Maximum Voltage out response.
        //
        case LM_API_CFG_MAX_VOUT:
        {
            lValue = *(unsigned short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                if(lValue < 0)
                {
                    lValue = ((lValue - 1) * 1000) - (0xc00 / 2);
                }
                else
                {
                    lValue = ((lValue + 1) * 1000) + (0xc00 / 2);
                }

                sprintf(pcTempString, "%s%ld.%01ld", (lValue < 0) ? "-" : "",
                        (lValue < 0) ? ((0 - lValue) / 30720) :
                        (lValue / 30720),
                        (lValue < 0) ? (((0 - lValue) % 30720) / 3072) :
                        ((lValue % 30720) / 3072));

                g_dMaxVout = strtod(pcTempString, NULL);

                g_pConfigMaxVout->value(g_dMaxVout);
            }
            else
            {
                printf("config maxvout (%ld) = %ld\n", ulID & CAN_MSGID_DEVNO_M,
                       lValue);
            }

            break;
        }

        //
        // Handle the get Fault Time Configuration response.
        //
        case LM_API_CFG_FAULT_TIME:
        {
            lValue = *(unsigned short *)(g_pucUARTMessage + 4);

            if(g_bUseGUI)
            {
                g_pConfigFaultTime->value(lValue);
            }
            else
            {
                printf("config faulttime (%ld) = %ld (%ld.%03ld)\n",
                       ulID & CAN_MSGID_DEVNO_M, lValue, lValue / 1000,
                       lValue % 1000);
            }

            break;
        }

        //
        // Handle the get Fault Count request
        //
        case LM_API_STATUS_FLT_COUNT:
        {
            if(g_bUseGUI)
            {
                //
                // Grab the response data and store it in the board
                // status structure.
                //
                g_sBoardStatus.ucCurrentFaults = g_pucUARTMessage[4];
                g_sBoardStatus.ucTemperatureFaults = g_pucUARTMessage[5];
                g_sBoardStatus.ucVoltageFaults = g_pucUARTMessage[6];
                g_sBoardStatus.ucGateFaults = g_pucUARTMessage[7];
                g_sBoardStatus.ucCommFaults = g_pucUARTMessage[8];
                g_sBoardStatus.ucCANStatus = g_pucUARTMessage[9];
                g_sBoardStatus.ucCANRxErrors = g_pucUARTMessage[10];
                g_sBoardStatus.ucCANTxErrors = g_pucUARTMessage[11];
            }
            else
            {
                printf("stat fault-counts (%ld):\n", ulID & CAN_MSGID_DEVNO_M);
                printf("\tcurr faults: %d\n", g_pucUARTMessage[4]);
                printf("\ttemp faults: %d\n", g_pucUARTMessage[5]);
                printf("\tvolt faults: %d\n", g_pucUARTMessage[6]);
                printf("\tgate faults: %d\n", g_pucUARTMessage[7]);
                printf("\tcomm faults: %d\n", g_pucUARTMessage[8]);
                printf("\tcansts[7:0]: 0x%x\n", g_pucUARTMessage[9]);
                printf("\tcanerr[15:8]: 0x%x\n", g_pucUARTMessage[10]);
                printf("\tcanerr[7:0]: 0x%x\n", g_pucUARTMessage[11]);
            }

            break;
        }

        //
        // Handle Periodic Status intervals (expects numerical order of API#)
        //
        case LM_API_PSTAT_PER_EN_S0:
        case LM_API_PSTAT_PER_EN_S1:
        case LM_API_PSTAT_PER_EN_S2:
        case LM_API_PSTAT_PER_EN_S3:
        {
            //
            // If this is not for the active board, ignore it.
            //
            if(iDevice != g_ulID)
            {
                break;
            }

            //
            // Get the status message number.
            //
            iIdx = ((ulID & ~(CAN_MSGID_DEVNO_M)) - LM_API_PSTAT_PER_EN_S0) >>
                CAN_MSGID_API_S;

            //
            // Load the data into a temp variable.
            //
            lValue = (g_pucUARTMessage[5] << 8) | g_pucUARTMessage[4];

            if(g_bUseGUI)
            {
                //
                // Store the response data in the board
                // status structure.
                //
                g_sBoardStatus.fPStatusMsgIntervals[iIdx] = lValue;


                //
                // if msg interval is not 0, flag the enable.
                //
                if(g_sBoardStatus.fPStatusMsgIntervals[iIdx] > 0)
                {
                    g_sBoardStatus.lBoardFlags |=
                        (PSTAT_STATEF_EN << iIdx);
                }
                else
                {
                    g_sBoardStatus.lBoardFlags &=
                        ~(PSTAT_STATEF_EN << iIdx);
                }
            }
            else
            {
                printf("pstat msg%d int (%ld): %ldms\n", iIdx,
                    ulID & CAN_MSGID_DEVNO_M, lValue);
            }
            break;
        }

        //
        // Handle Periodic Status config (expects numerical order of API#)
        //
        case LM_API_PSTAT_CFG_S0:
        case LM_API_PSTAT_CFG_S1:
        case LM_API_PSTAT_CFG_S2:
        case LM_API_PSTAT_CFG_S3:
        {
            //
            // If this is not for the active board, ignore it.
            //
            if(iDevice != g_ulID)
            {
                break;
            }

            //
            // Get the status message number.
            //
            iTemp = ((ulID & ~(CAN_MSGID_DEVNO_M)) - LM_API_PSTAT_CFG_S0) >>
                CAN_MSGID_API_S;

            if(g_bUseGUI)
            {
                //
                // Grab the response data and store it in the board
                // status structure.
                //
                for(iIdx = 0; iIdx < PSTATUS_PAYLOAD_SZ; iIdx++)
                {
                    g_sBoardStatus.pulPStatusMsgCfgs[iTemp][iIdx] =
                        g_pucUARTMessage[iIdx + 4];
                }

                //
                // Setup the history buffer.
                //
                GUIPeriodicStatusHistorySetup((char)iTemp);
            }
            else
            {

                printf("pstat msg%d cfg (%ld):\n", iTemp,
                    ulID & CAN_MSGID_DEVNO_M);
                for(iIdx = 0; iIdx < PSTATUS_PAYLOAD_SZ; iIdx++)
                {
                    printf("\tbyte%d ID: %s (%d)\n", iIdx,
                        g_sPStatMsgs[g_pucUARTMessage[iIdx + 4]].pcMsgMnemonic,
                        g_pucUARTMessage[iIdx + 4]);
                }
            }
            break;
         }

        //
        // Handle Periodic Status data for status msgs
        //
        case LM_API_PSTAT_DATA_S0:
        case LM_API_PSTAT_DATA_S1:
        case LM_API_PSTAT_DATA_S2:
        case LM_API_PSTAT_DATA_S3:
        {
            //
            // If this is not for the active board, ignore it.
            //
            if(iDevice != g_ulID)
            {
                break;
            }

            //
            // Get the status message number.
            //
            iTemp = ((ulID & ~(CAN_MSGID_DEVNO_M)) - LM_API_PSTAT_DATA_S0) >>
                CAN_MSGID_API_S;

            if(g_bUseGUI)
            {
                //
                // Grab the response data and store it in the board
                // status structure (if a valid config).
                //
                for(iIdx = 0; iIdx < PSTATUS_PAYLOAD_SZ; iIdx++)
                {
                    if(g_sBoardStatus.pulPStatusMsgCfgs[iTemp][iIdx])
                    {
                        g_sBoardStatus.pucPStatusMsgPayload[iTemp][iIdx] =
                            g_pucUARTMessage[4 + iIdx];
                    }
                }

                //
                // Flag the GUI to parse the payload data.
                //
                g_sBoardStatus.lBoardFlags |=
                    (PSTAT_STATEF_UPD << iTemp);
            }
            else
            {
                printf("pstat msg%d data (%ld):\n", iTemp,
                    ulID & CAN_MSGID_DEVNO_M);
                for(iIdx = 0; iIdx < PSTATUS_PAYLOAD_SZ; iIdx++)
                {
                    if(g_sBoardStatus.pulPStatusMsgCfgs[iTemp][iIdx])
                    {
                        printf("\tbyte%d : %d\n", iIdx,
                            g_pucUARTMessage[4 + iIdx]);
                    }
                }
            }
            break;
        }
    }
}

//*****************************************************************************
//
// This function handles waiting for an ACK from the device, and includes a
// timeout.
//
//*****************************************************************************
int
WaitForAck(unsigned long ulID, unsigned long ulTimeout)
{
    unsigned char ucChar;

    while(1)
    {
        //
        // If the UART timed out or failed to read for some reason then just
        // return.
        //
        if(UARTReceiveData(&ucChar, 1) == -1)
        {
            if(--ulTimeout == 0)
            {
                return(-1);
            }
            continue;
        }

        //
        // See if this is a start of packet byte.
        //
        if(ucChar == 0xff)
        {
            //
            // Reset the length of the UART message.
            //
            g_ulUARTLength = 0;

            //
            // Set the state such that the next byte received is the size
            // of the message.
            //
            g_ulUARTState = UART_STATE_LENGTH;
        }

        //
        // See if this byte is the size of the message.
        //
        else if(g_ulUARTState == UART_STATE_LENGTH)
        {
            //
            // Save the size of the message.
            //
            g_ulUARTSize = ucChar;

            //
            // Subsequent bytes received are the message data.
            //
            g_ulUARTState = UART_STATE_DATA;
        }

        //
        // See if the previous character was an escape character.
        //
        else if(g_ulUARTState == UART_STATE_ESCAPE)
        {
            //
            // See if this 0xfe, the escaped version of 0xff.
            //
            if(ucChar == 0xfe)
            {
                //
                // Store a 0xff in the message buffer.
                //
                g_pucUARTMessage[g_ulUARTLength++] = 0xff;

                //
                // Subsequent bytes received are the message data.
                //
                g_ulUARTState = UART_STATE_DATA;
            }

            //
            // Otherwise, see if this is 0xfd, the escaped version of 0xfe.
            //
            else if(ucChar == 0xfd)
            {
                //
                // Store a 0xfe in the message buffer.
                //
                g_pucUARTMessage[g_ulUARTLength++] = 0xfe;

                //
                // Subsequent bytes received are the message data.
                //
                g_ulUARTState = UART_STATE_DATA;
            }

            //
            // Otherwise, this is a corrupted sequence.  Set the receiver
            // to idle so this message is dropped, and subsequent data is
            // ignored until another start of packet is received.
            //
            else
            {
                g_ulUARTState = UART_STATE_IDLE;
            }
        }

        //
        // See if this is a part of the message data.
        //
        else if(g_ulUARTState == UART_STATE_DATA)
        {
            //
            // See if this character is an escape character.
            //
            if(ucChar == 0xfe)
            {
                //
                // The next byte is an escaped byte.
                //
                g_ulUARTState = UART_STATE_ESCAPE;
            }
            else
            {
                //
                // Store this byte in the message buffer.
                //
                g_pucUARTMessage[g_ulUARTLength++] = ucChar;
            }
        }

        //
        // See if the entire message has been received but has not been
        // processed (i.e. the most recent byte received was the end of the
        // message).
        //
        if((g_ulUARTLength == g_ulUARTSize) &&
           (g_ulUARTState == UART_STATE_DATA))
        {
            //
            // The UART interface is idle, meaning all bytes will be
            // dropped until the next start of packet byte.
            //
            g_ulUARTState = UART_STATE_IDLE;

            //
            // Parse out the data that was received.
            //
            ParseResponse();

            if(*(unsigned long *)g_pucUARTMessage == ulID)
            {
                return(0);
            }
        }
    }
    return(0);
}

//*****************************************************************************
//
// This function is used to set the currently active device ID that is being
// communicated with.
//
//*****************************************************************************
int
CmdID(int argc, char *argv[])
{
    unsigned long ulValue;

    //
    // If there is a value then this is a set request otherwise it is a get
    // request.
    //
    if(argc > 1)
    {
        //
        // Convert the ID text value to a  number.
        //
        ulValue = strtoul(argv[1], 0, 0);

        //
        // Check that the value is valid.
        //
        if((ulValue == 0) || (ulValue > 63))
        {
            printf("%s: the ID must be between 1 and 63.\n", argv[0]);
        }
        else
        {
            g_ulID = ulValue;
        }
    }
    else
    {
        printf("id = %ld\n", g_ulID);
    }

    return(0);
}

//*****************************************************************************
//
// This command is used to toggle if the heart beat messages are being send
// out.
//
//*****************************************************************************
int
CmdHeartbeat(int argc, char *argv[])
{
    //
    // Just toggle the heart beat mode.
    //
    g_ulHeartbeat ^= 1;

    printf("heart beat is %s\n", g_ulHeartbeat ? "on" : "off");

    return(0);
}

//*****************************************************************************
//
// This command controls the setting when running in voltage control mode.
//
//*****************************************************************************
int
CmdVoltage(int argc, char *argv[])
{
    long plValue[2], lTemp;
    unsigned short *pusData;
    unsigned char *pucData;
    int iIdx;

    pusData = (unsigned short *)plValue;
    pucData = (unsigned char *)plValue;

    //
    // Check if this was a request to enable Voltage control mode.
    //
    if((argc > 1) && (strcmp(argv[1], "en") == 0))
    {
        UARTSendMessage(LM_API_VOLT_EN | g_ulID, 0, 0);
        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    //
    // Check if this was a request to disable Voltage control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "dis") == 0))
    {
        UARTSendMessage(LM_API_VOLT_DIS | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    //
    // Check if this was a request to set the current output Voltage.
    //
    else if((argc > 1) && (strcmp(argv[1], "set") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            //
            // Get the setting from the argument.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < -32768)
            {
                plValue[0] = -32768;
            }
            if(plValue[0] > 32767)
            {
                plValue[0] = 32767;
            }

            //
            // If there is a third argument then this is a synchronous set
            // command.
            //
            if(argc > 3)
            {
                //
                // Get the synchronous group number.
                //
                plValue[1] = strtol(argv[3], 0, 0);

                //
                // Limit the value to valid values.
                //
                if(plValue[1] < 0)
                {
                    plValue[1] = 0;
                }
                if(plValue[1] > 255)
                {
                    plValue[1] = 0;
                }

                plValue[0] = (plValue[0] & 0x0000ffff) | (plValue[1] << 16);

                UARTSendMessage(LM_API_VOLT_SET | g_ulID,
                                (unsigned char *)plValue, 3);
            }
            else
            {
                UARTSendMessage(LM_API_VOLT_SET | g_ulID,
                                (unsigned char *)plValue, 2);
            }

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A Set with no data is a get request.
            //
            UARTSendMessage(LM_API_VOLT_SET | g_ulID, 0, 0);

            WaitForAck(LM_API_VOLT_SET | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the ramp rate in Voltage control
    // mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "ramp") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            //
            // Get the ramp rate value.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < 0)
            {
                plValue[0] = 0;
            }
            if(plValue[0] > 65535)
            {
                plValue[0] = 65535;
            }

            //
            // Send the ramp rate to the device.
            //
            UARTSendMessage(LM_API_VOLT_SET_RAMP | g_ulID,
                            (unsigned char *)plValue, 2);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A set command without data is a request for data.
            //
            UARTSendMessage(LM_API_VOLT_SET_RAMP | g_ulID, 0, 0);

            WaitForAck(LM_API_VOLT_SET_RAMP | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the current output Voltage without an
    // ACK.
    //
    else if((argc > 1) && (strcmp(argv[1], "nset") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            //
            // Get the setting from the argument.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < -32768)
            {
                plValue[0] = -32768;
            }
            if(plValue[0] > 32767)
            {
                plValue[0] = 32767;
            }

            //
            // If there is a third argument then this is a synchronous set
            // command.
            //
            if(argc > 3)
            {
                //
                // Get the synchronous group number.
                //
                plValue[1] = strtol(argv[3], 0, 0);

                //
                // Limit the value to valid values.
                //
                if(plValue[1] < 0)
                {
                    plValue[1] = 0;
                }
                if(plValue[1] > 255)
                {
                    plValue[1] = 0;
                }

                plValue[0] = (plValue[0] & 0x0000ffff) | (plValue[1] << 16);

                UARTSendMessage(LM_API_VOLT_SET_NO_ACK | g_ulID,
                                (unsigned char *)plValue, 3);
            }
            else
            {
                UARTSendMessage(LM_API_VOLT_SET_NO_ACK | g_ulID,
                                (unsigned char *)plValue, 2);
            }
        }
        else
        {
            //
            // A Set with no data is a get request.
            //
            UARTSendMessage(LM_API_VOLT_SET_NO_ACK | g_ulID, 0, 0);

            WaitForAck(LM_API_VOLT_SET_NO_ACK | g_ulID, 10);
        }
    }

    else
    {
        //
        // If this was an unknown request then print out the valid options.
        //
        printf("%s [en|dis|set|ramp|nset]\n", argv[0]);
    }

    return(0);
}

//*****************************************************************************
//
// This command controls the setting when running in voltage compensation mode.
//
//*****************************************************************************
int
CmdVComp(int argc, char *argv[])
{
    long plValue[2], lTemp;
    unsigned short *pusData;
    unsigned char *pucData;
    int iIdx;

    pusData = (unsigned short *)plValue;
    pucData = (unsigned char *)plValue;

    //
    // Check if this was a request to enable voltage compensation mode.
    //
    if((argc > 1) && (strcmp(argv[1], "en") == 0))
    {
        UARTSendMessage(LM_API_VCOMP_EN | g_ulID, 0, 0);
        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    //
    // Check if this was a request to disable voltage compensation mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "dis") == 0))
    {
        UARTSendMessage(LM_API_VCOMP_DIS | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    //
    // Check if this was a request to set the current target voltage.
    //
    else if((argc > 1) && (strcmp(argv[1], "set") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            //
            // Get the setting from the argument.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < (-24 * 256))
            {
                plValue[0] = -24 * 256;
            }
            if(plValue[0] > (24 * 256))
            {
                plValue[0] = 24 * 256;
            }

            //
            // If there is a third argument then this is a synchronous set
            // command.
            //
            if(argc > 3)
            {
                //
                // Get the synchronous group number.
                //
                plValue[1] = strtol(argv[3], 0, 0);

                //
                // Limit the value to valid values.
                //
                if(plValue[1] < 0)
                {
                    plValue[1] = 0;
                }
                if(plValue[1] > 255)
                {
                    plValue[1] = 0;
                }

                plValue[0] = (plValue[0] & 0x0000ffff) | (plValue[1] << 16);

                UARTSendMessage(LM_API_VCOMP_SET | g_ulID,
                                (unsigned char *)plValue, 3);
            }
            else
            {
                UARTSendMessage(LM_API_VCOMP_SET | g_ulID,
                                (unsigned char *)plValue, 2);
            }

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A Set with no data is a get request.
            //
            UARTSendMessage(LM_API_VCOMP_SET | g_ulID, 0, 0);

            WaitForAck(LM_API_VCOMP_SET | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the set point ramp rate in voltage
    // compensation mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "ramp") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            //
            // Get the ramp rate value.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < 0)
            {
                plValue[0] = 0;
            }
            if(plValue[0] > (24 * 256))
            {
                plValue[0] = 24 * 256;
            }

            //
            // Send the ramp rate to the device.
            //
            UARTSendMessage(LM_API_VCOMP_IN_RAMP | g_ulID,
                            (unsigned char *)plValue, 2);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A set command without data is a request for data.
            //
            UARTSendMessage(LM_API_VCOMP_IN_RAMP | g_ulID, 0, 0);

            WaitForAck(LM_API_VCOMP_IN_RAMP | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the compensation ramp rate in voltage
    // compensation mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "comp") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            //
            // Get the ramp rate value.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < 0)
            {
                plValue[0] = 0;
            }
            if(plValue[0] > 65535)
            {
                plValue[0] = 65535;
            }

            //
            // Send the ramp rate to the device.
            //
            UARTSendMessage(LM_API_VCOMP_COMP_RAMP | g_ulID,
                            (unsigned char *)plValue, 2);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A set command without data is a request for data.
            //
            UARTSendMessage(LM_API_VCOMP_COMP_RAMP | g_ulID, 0, 0);

            WaitForAck(LM_API_VCOMP_COMP_RAMP | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the current target voltage without an
    // ACK.
    //
    else if((argc > 1) && (strcmp(argv[1], "nset") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            //
            // Get the setting from the argument.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < (-24 * 256))
            {
                plValue[0] = -24 * 256;
            }
            if(plValue[0] > (24 * 256))
            {
                plValue[0] = 24 * 256;
            }

            //
            // If there is a third argument then this is a synchronous set
            // command.
            //
            if(argc > 3)
            {
                //
                // Get the synchronous group number.
                //
                plValue[1] = strtol(argv[3], 0, 0);

                //
                // Limit the value to valid values.
                //
                if(plValue[1] < 0)
                {
                    plValue[1] = 0;
                }
                if(plValue[1] > 255)
                {
                    plValue[1] = 0;
                }

                plValue[0] = (plValue[0] & 0x0000ffff) | (plValue[1] << 16);

                UARTSendMessage(LM_API_VCOMP_SET_NO_ACK | g_ulID,
                                (unsigned char *)plValue, 3);
            }
            else
            {
                UARTSendMessage(LM_API_VCOMP_SET_NO_ACK | g_ulID,
                                (unsigned char *)plValue, 2);
            }
        }
        else
        {
            //
            // A Set with no data is a get request.
            //
            UARTSendMessage(LM_API_VCOMP_SET_NO_ACK | g_ulID, 0, 0);

            WaitForAck(LM_API_VCOMP_SET_NO_ACK | g_ulID, 10);
        }
    }

    else
    {
        //
        // If this was an unknown request then print out the valid options.
        //
        printf("%s [en|dis|set|ramp|comp|nset]\n", argv[0]);
    }

    return(0);
}

//*****************************************************************************
//
// This command controls the setting when running in current control mode.
//
//*****************************************************************************
int
CmdCurrent(int argc, char *argv[])
{
    long plValue[2], lTemp;
    unsigned short *pusData;
    unsigned char *pucData;
    int iIdx;

    pusData = (unsigned short *)plValue;
    pucData = (unsigned char *)plValue;

    //
    // Check if this was a request to enable Current control mode.
    //
    if((argc > 1) && (strcmp(argv[1], "en") == 0))
    {
        UARTSendMessage(LM_API_ICTRL_EN | g_ulID, 0, 0);
        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    //
    // Check if this was a request to enable Current control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "dis") == 0))
    {
        UARTSendMessage(LM_API_ICTRL_DIS | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    //
    // Check if this was a request to set the Current.
    //
    else if((argc > 1) && (strcmp(argv[1], "set") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            //
            // Get the Current value.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < -32768)
            {
                plValue[0] = -32768;
            }
            if(plValue[0] > 32767)
            {
                plValue[0] = 32767;
            }

            //
            // If there is a third argument then this is the synchronization
            // group number.
            //
            if(argc > 3)
            {
                //
                // Get the synchronization group.
                //
                plValue[1] = strtol(argv[3], 0, 0);

                //
                // Limit the value to valid values.
                //
                if(plValue[1] < 0)
                {
                    plValue[1] = 0;
                }
                if(plValue[1] > 255)
                {
                    plValue[1] = 0;
                }
                plValue[0] = (plValue[0] & 0x0000ffff) | (plValue[1] << 16);

                UARTSendMessage(LM_API_ICTRL_SET | g_ulID,
                                (unsigned char *)plValue, 3);
            }
            else
            {
                UARTSendMessage(LM_API_ICTRL_SET | g_ulID,
                                (unsigned char *)plValue, 2);
            }

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            UARTSendMessage(LM_API_ICTRL_SET | g_ulID, 0, 0);

            WaitForAck(LM_API_ICTRL_SET | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the P value in Current control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "p") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            UARTSendMessage(LM_API_ICTRL_PC | g_ulID,
                            (unsigned char *)plValue, 4);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_ICTRL_PC | g_ulID, 0, 0);

            WaitForAck(LM_API_ICTRL_PC | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the I value in Current control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "i") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            UARTSendMessage(LM_API_ICTRL_IC | g_ulID,
                            (unsigned char *)plValue, 4);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_ICTRL_IC | g_ulID, 0, 0);

            WaitForAck(LM_API_ICTRL_IC | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the D value in Current control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "d") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            UARTSendMessage(LM_API_ICTRL_DC | g_ulID,
                            (unsigned char *)plValue, 4);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_ICTRL_DC | g_ulID, 0, 0);

            WaitForAck(LM_API_ICTRL_DC | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the Current without an ACK.
    //
    else if((argc > 1) && (strcmp(argv[1], "nset") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            //
            // Get the Current value.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < -32768)
            {
                plValue[0] = -32768;
            }
            if(plValue[0] > 32767)
            {
                plValue[0] = 32767;
            }

            //
            // If there is a third argument then this is the synchronization
            // group number.
            //
            if(argc > 3)
            {
                //
                // Get the synchronization group.
                //
                plValue[1] = strtol(argv[3], 0, 0);

                //
                // Limit the value to valid values.
                //
                if(plValue[1] < 0)
                {
                    plValue[1] = 0;
                }
                if(plValue[1] > 255)
                {
                    plValue[1] = 0;
                }
                plValue[0] = (plValue[0] & 0x0000ffff) | (plValue[1] << 16);

                UARTSendMessage(LM_API_ICTRL_SET_NO_ACK | g_ulID,
                                (unsigned char *)plValue, 3);
            }
            else
            {
                UARTSendMessage(LM_API_ICTRL_SET_NO_ACK | g_ulID,
                                (unsigned char *)plValue, 2);
            }
        }
        else
        {
            UARTSendMessage(LM_API_ICTRL_SET_NO_ACK | g_ulID, 0, 0);

            WaitForAck(LM_API_ICTRL_SET_NO_ACK | g_ulID, 10);
        }
    }

    else
    {
        //
        // If this was an unknown request then print out the valid options.
        //
        printf("%s [en|dis|set|p|i|d|nset]\n", argv[0]);
    }

    return(0);
}

//*****************************************************************************
//
// This command controls the setting when running in Speed control mode.
//
//*****************************************************************************
int
CmdSpeed(int argc, char *argv[])
{
    long plValue[2], lTemp;
    unsigned short *pusData;
    unsigned char *pucData;
    int iIdx;

    pusData = (unsigned short *)plValue;
    pucData = (unsigned char *)plValue;

    //
    // Check if this was a request to enable Speed control mode.
    //
    if((argc > 1) && (strcmp(argv[1], "en") == 0))
    {
        UARTSendMessage(LM_API_SPD_EN | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    //
    // Check if this was a request to disable Speed control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "dis") == 0))
    {
        UARTSendMessage(LM_API_SPD_DIS | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    //
    // Check if this was a request to set the speed in Speed control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "set") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);
            if(argc > 3)
            {
                plValue[1] = strtol(argv[3], 0, 0);

                //
                // Limit the value to valid values.
                //
                if(plValue[1] < 0)
                {
                    plValue[1] = 0;
                }
                if(plValue[1] > 255)
                {
                    plValue[1] = 0;
                }

                UARTSendMessage(LM_API_SPD_SET | g_ulID,
                                (unsigned char *)plValue, 5);
            }
            else
            {
                UARTSendMessage(LM_API_SPD_SET | g_ulID,
                                (unsigned char *)plValue, 4);
            }
            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_SPD_SET | g_ulID, 0, 0);

            WaitForAck(LM_API_SPD_SET | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the P value in Speed control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "p") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            UARTSendMessage(LM_API_SPD_PC | g_ulID,
                            (unsigned char *)plValue, 4);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_SPD_PC | g_ulID, 0, 0);

            WaitForAck(LM_API_SPD_PC | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the I value in Speed control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "i") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            UARTSendMessage(LM_API_SPD_IC | g_ulID,
                            (unsigned char *)plValue, 4);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_SPD_IC | g_ulID, 0, 0);

            WaitForAck(LM_API_SPD_IC | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the D value in Speed control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "d") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            UARTSendMessage(LM_API_SPD_DC | g_ulID,
                            (unsigned char *)plValue, 4);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_SPD_DC | g_ulID, 0, 0);

            WaitForAck(LM_API_SPD_DC | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the speed reference for Speed control
    // mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "ref") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < 0)
            {
                plValue[0] = 0;
            }
            if(plValue[0] > 255)
            {
                plValue[0] = 255;
            }

            UARTSendMessage(LM_API_SPD_REF | g_ulID,
                            (unsigned char *)plValue, 1);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_SPD_REF | g_ulID, 0, 0);

            WaitForAck(LM_API_SPD_REF | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the speed without an ACK in Speed
    // control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "nset") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);
            if(argc > 3)
            {
                plValue[1] = strtol(argv[3], 0, 0);

                //
                // Limit the value to valid values.
                //
                if(plValue[1] < 0)
                {
                    plValue[1] = 0;
                }
                if(plValue[1] > 255)
                {
                    plValue[1] = 0;
                }

                UARTSendMessage(LM_API_SPD_SET_NO_ACK | g_ulID,
                                (unsigned char *)plValue, 5);
            }
            else
            {
                UARTSendMessage(LM_API_SPD_SET_NO_ACK | g_ulID,
                                (unsigned char *)plValue, 4);
            }
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_SPD_SET_NO_ACK | g_ulID, 0, 0);

            WaitForAck(LM_API_SPD_SET_NO_ACK | g_ulID, 10);
        }
    }

    else
    {
        //
        // If this was an unknown request then print out the valid options.
        //
        printf("%s [en|dis|set|p|i|d|ref]\n", argv[0]);
    }

    return(0);
}

//*****************************************************************************
//
// This command controls the setting when running in Position control mode.
//
//*****************************************************************************
int
CmdPosition(int argc, char *argv[])
{
    long plValue[2], lTemp;
    unsigned short *pusData;
    unsigned char *pucData;
    int iIdx;

    pusData = (unsigned short *)plValue;
    pucData = (unsigned char *)plValue;

    //
    // Check if this was a request to enable Position control mode.
    //
    if((argc > 1) && (strcmp(argv[1], "en") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            UARTSendMessage(LM_API_POS_EN | g_ulID,
                            (unsigned char *)plValue, 4);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            printf("%s %s <value>\n", argv[0], argv[1]);
        }
    }

    //
    // Check if this was a request to disable Position control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "dis") == 0))
    {
        UARTSendMessage(LM_API_POS_DIS | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    //
    // Check if this was a request to set the position target.
    //
    else if((argc > 1) && (strcmp(argv[1], "set") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);
            if(argc > 3)
            {
                plValue[1] = strtol(argv[3], 0, 0);

                //
                // Limit the value to valid values.
                //
                if(plValue[1] < 0)
                {
                    plValue[1] = 0;
                }
                if(plValue[1] > 255)
                {
                    plValue[1] = 0;
                }
                UARTSendMessage(LM_API_POS_SET | g_ulID,
                                (unsigned char *)plValue, 5);
            }
            else
            {
                UARTSendMessage(LM_API_POS_SET | g_ulID,
                                (unsigned char *)plValue, 4);
            }
            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_POS_SET | g_ulID, 0, 0);

            WaitForAck(LM_API_POS_SET | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the P value in Position control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "p") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);
            UARTSendMessage(LM_API_POS_PC | g_ulID,
                            (unsigned char *)plValue, 4);
            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_POS_PC | g_ulID, 0, 0);

            WaitForAck(LM_API_POS_PC | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the I value in Position control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "i") == 0))
    {
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            UARTSendMessage(LM_API_POS_IC | g_ulID,
                            (unsigned char *)plValue, 4);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_POS_IC | g_ulID, 0, 0);

            WaitForAck(LM_API_POS_IC | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the D value in Position control mode.
    //
    else if((argc > 1) && (strcmp(argv[1], "d") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            UARTSendMessage(LM_API_POS_DC | g_ulID,
                            (unsigned char *)plValue, 4);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_POS_DC | g_ulID, 0, 0);

            WaitForAck(LM_API_POS_DC | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the position reference.
    //
    else if((argc > 1) && (strcmp(argv[1], "ref") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < 0)
            {
                plValue[0] = 0;
            }
            if(plValue[0] > 255)
            {
                plValue[0] = 255;
            }

            UARTSendMessage(LM_API_POS_REF | g_ulID,
                            (unsigned char *)plValue, 1);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_POS_REF | g_ulID, 0, 0);

            WaitForAck(LM_API_POS_REF | g_ulID, 10);
        }
    }

    //
    // Check if this was a request to set the position target without an ACK.
    //
    else if((argc > 1) && (strcmp(argv[1], "nset") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);
            if(argc > 3)
            {
                plValue[1] = strtol(argv[3], 0, 0);

                //
                // Limit the value to valid values.
                //
                if(plValue[1] < 0)
                {
                    plValue[1] = 0;
                }
                if(plValue[1] > 255)
                {
                    plValue[1] = 0;
                }
                UARTSendMessage(LM_API_POS_SET_NO_ACK | g_ulID,
                                (unsigned char *)plValue, 5);
            }
            else
            {
                UARTSendMessage(LM_API_POS_SET_NO_ACK | g_ulID,
                                (unsigned char *)plValue, 4);
            }
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_POS_SET_NO_ACK | g_ulID, 0, 0);

            WaitForAck(LM_API_POS_SET_NO_ACK | g_ulID, 10);
        }
    }

    else
    {
        //
        // If this was an unknown request then print out the valid options.
        //
        printf("%s [en|dis|set|p|i|d|ref|nset]\n", argv[0]);
    }

    return(0);
}

//*****************************************************************************
//
// This command handles status requests for devices.
//
//*****************************************************************************
int
CmdStatus(int argc, char *argv[])
{
    unsigned char ucData;

    if((argc > 1) && (strcmp(argv[1], "vout") == 0))
    {
        //
        // Request the current Voltage output setting.
        //
        UARTSendMessage(LM_API_STATUS_VOLTOUT | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else if((argc > 1) && (strcmp(argv[1], "vbus") == 0))
    {
        //
        // Request the current Vbus value.
        //
        UARTSendMessage(LM_API_STATUS_VOLTBUS | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else if((argc > 1) && (strcmp(argv[1], "fault") == 0))
    {
        if(argc > 2)
        {
            ucData = 1;

            //
            // Request a clear of the current Fault value.
            //
            UARTSendMessage(LM_API_STATUS_FAULT | g_ulID, &ucData, 1);
        }
        else
        {
            //
            // Request the current Fault value.
            //
            UARTSendMessage(LM_API_STATUS_FAULT | g_ulID, 0, 0);
        }

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else if((argc > 1) && (strcmp(argv[1], "cur") == 0))
    {
        //
        // Request the current Current value.
        //
        UARTSendMessage(LM_API_STATUS_CURRENT | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else if((argc > 1) && (strcmp(argv[1], "temp") == 0))
    {
        //
        // Request the current temperature.
        //
        UARTSendMessage(LM_API_STATUS_TEMP | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else if((argc > 1) && (strcmp(argv[1], "pos") == 0))
    {
        //
        // Request the current position.
        //
        UARTSendMessage(LM_API_STATUS_POS | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else if((argc > 1) && (strcmp(argv[1], "speed") == 0))
    {
        //
        // Request the current speed.
        //
        UARTSendMessage(LM_API_STATUS_SPD | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else if((argc > 1) && (strcmp(argv[1], "limit") == 0))
    {
        //
        // Request the limit switch settings.
        //
        UARTSendMessage(LM_API_STATUS_LIMIT | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else if((argc > 1) && (strcmp(argv[1], "power") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            ucData = 1;
            UARTSendMessage(LM_API_STATUS_POWER | g_ulID, &ucData, 1);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // A command with no data is a get request.
            //
            UARTSendMessage(LM_API_STATUS_POWER | g_ulID, 0, 0);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
    }

    else if((argc > 1) && (strcmp(argv[1], "cmode") == 0))
    {
        //
        // Get the current control mode.
        //
        UARTSendMessage(LM_API_STATUS_CMODE | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else if((argc > 1) && (strcmp(argv[1], "vout2") == 0))
    {
        //
        // Request the current voltage output setting.
        //
        UARTSendMessage(LM_API_STATUS_VOUT | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else if((argc > 1) && (strcmp(argv[1], "stkyfault") == 0))
    {
        //
        // Request the current Sticky Fault value.
        //
        UARTSendMessage(LM_API_STATUS_STKY_FLT | g_ulID, 0, 0);

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else if((argc > 1) && (strcmp(argv[1], "faultcnts") == 0))
    {
        if(argc > 2)
        {
            //
            // Convert the string to unsigned char value.
            //
            ucData = (char)strtol(argv[2], 0, 0);

            //
            // Request the targeted Fault Counter reset.
            //
            UARTSendMessage(LM_API_STATUS_FLT_COUNT | g_ulID, &ucData, 1);
        }
        else
        {

            //
            // Request the current Fault Counters.
            //
            UARTSendMessage(LM_API_STATUS_FLT_COUNT | g_ulID, 0, 0);
        }

        WaitForAck(LM_API_ACK | g_ulID, 10);
    }

    else
    {
        printf("%s [vout|vbus|fault|cur|temp|pos|speed|limit|power|cmode|\n"
               "\tvout2|stkyfault|faultcnts]\n", argv[0]);
    }

    return(0);
}

//*****************************************************************************
//
// This command is used to set configuration parameters used by the devices.
//
//*****************************************************************************
int
CmdConfig(int argc, char *argv[])
{
    long plValue[2];

    //
    // Get or set the number of encoder lines based on the parameters.
    //
    if((argc > 1) && (strcmp(argv[1], "lines") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid values.
            //
            if(plValue[0] < 0)
            {
                plValue[0] = 0;
            }
            if(plValue[0] > 65535)
            {
                plValue[0] = 65535;
            }

            UARTSendMessage(LM_API_CFG_ENC_LINES | g_ulID,
                            (unsigned char *)plValue, 2);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // This is a request to retrieve the current number of encode lines.
            //
            UARTSendMessage(LM_API_CFG_ENC_LINES | g_ulID, 0, 0);

            WaitForAck(LM_API_CFG_ENC_LINES | g_ulID, 10);
        }
    }

    //
    // Get or set the number of turns in a potentiometer based on the
    // parameters.
    //
    else if((argc > 1) && (strcmp(argv[1], "turns") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid settings.
            //
            if(plValue[0] < 0)
            {
                plValue[0] = 0;
            }

            if(plValue[0] > 65535)
            {
                plValue[0] = 65535;
            }

            UARTSendMessage(LM_API_CFG_POT_TURNS | g_ulID,
                            (unsigned char *)plValue, 2);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // This is a request to retrieve the value.
            //
            UARTSendMessage(LM_API_CFG_POT_TURNS | g_ulID, 0, 0);

            WaitForAck(LM_API_CFG_POT_TURNS | g_ulID, 10);
        }
    }

    //
    // Get or set the brake/coast setting based on the parameters.
    //
    else if((argc > 1) && (strcmp(argv[1], "brake") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            if(strcmp(argv[2], "jumper") == 0)
            {
                //
                // Allow the jumper to control this setting.
                //
                plValue[0] = 0;
            }
            else if(strcmp(argv[2], "brake") == 0)
            {
                //
                // Override the jumper and set the mode to active braking.
                //
                plValue[0] = 1;
            }
            else if(strcmp(argv[2], "coast") == 0)
            {
                //
                // Override the jumper and set the mode to coast braking.
                //
                plValue[0] = 2;
            }
            else
            {
                printf("%s %s [jumper|brake|coast]\n", argv[0], argv[1]);
                return(0);
            }

            UARTSendMessage(LM_API_CFG_BRAKE_COAST | g_ulID,
                            (unsigned char *)plValue, 1);
            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // This is a request to retrieve the value.
            //
            UARTSendMessage(LM_API_CFG_BRAKE_COAST | g_ulID, 0, 0);

            WaitForAck(LM_API_CFG_BRAKE_COAST | g_ulID, 10);
        }
    }

    else if((argc > 1) && (strcmp(argv[1], "limit") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            if(strcmp(argv[2], "off") == 0)
            {
                //
                // Disable the limit switches.
                //
                plValue[0] = 0;
            }
            else if(strcmp(argv[2], "on") == 0)
            {
                //
                // Enable the limit switches.
                //
                plValue[0] = 1;
            }
            else
            {
                printf("%s %s [on|off]\n", argv[0], argv[1]);
                return(0);
            }

            UARTSendMessage(LM_API_CFG_LIMIT_MODE | g_ulID,
                            (unsigned char *)plValue, 1);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // This is a request to retrieve the value.
            //
            UARTSendMessage(LM_API_CFG_LIMIT_MODE | g_ulID, 0, 0);

            WaitForAck(LM_API_CFG_LIMIT_MODE | g_ulID, 10);
        }
    }

    else if((argc > 1) && (strcmp(argv[1], "fwd") == 0))
    {
        //
        // If there is enough data then this is a request to set the value.
        //
        if(argc > 3)
        {
            //
            // Get the position limit.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Check if the value is a less than or greater than some position.
            //
            if(strcmp(argv[3], "lt") == 0)
            {
                plValue[1] = 0;
            }
            else if(strcmp(argv[3], "gt") == 0)
            {
                plValue[1] = 1;
            }
            else
            {
                printf("%s %s <pos> [lt|gt]\n", argv[0], argv[1]);
                return(0);
            }

            UARTSendMessage(LM_API_CFG_LIMIT_FWD | g_ulID,
                            (unsigned char *)plValue, 5);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // This is a request to retrieve the value.
            //
            UARTSendMessage(LM_API_CFG_LIMIT_FWD | g_ulID, 0, 0);

            WaitForAck(LM_API_CFG_LIMIT_FWD | g_ulID, 10);
        }
    }

    else if((argc > 1) && (strcmp(argv[1], "rev") == 0))
    {
        //
        // If there is enough data then this is a request to set the value.
        //
        if(argc > 3)
        {
            //
            // Get the position limit.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Check if the value is a less than or greater than some position.
            //
            if(strcmp(argv[3], "lt") == 0)
            {
                plValue[1] = 0;
            }
            else if(strcmp(argv[3], "gt") == 0)
            {
                plValue[1] = 1;
            }
            else
            {
                printf("%s %s <pos> [lt|gt]\n", argv[0], argv[1]);
                return(0);
            }

            UARTSendMessage(LM_API_CFG_LIMIT_REV | g_ulID,
                            (unsigned char *)plValue, 5);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // This is a request to retrieve the value.
            //
            UARTSendMessage(LM_API_CFG_LIMIT_REV | g_ulID, 0, 0);

            WaitForAck(LM_API_CFG_LIMIT_REV | g_ulID, 10);
        }
    }

    else if((argc > 1) && (strcmp(argv[1], "maxvout") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            //
            // Get the Max voltage out setting.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid settings.
            //
            if(plValue[0] < 0)
            {
                plValue[0] = 0;
            }

            if(plValue[0] > (12 * 256))
            {
                plValue[0] = 12 * 256;
            }

            UARTSendMessage(LM_API_CFG_MAX_VOUT | g_ulID,
                            (unsigned char *)plValue, 2);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // This is a request to retrieve the value.
            //
            UARTSendMessage(LM_API_CFG_MAX_VOUT | g_ulID, 0, 0);

            WaitForAck(LM_API_CFG_MAX_VOUT | g_ulID, 10);
        }
    }

    else if((argc > 1) && (strcmp(argv[1], "faulttime") == 0))
    {
        //
        // If there is a second argument then this is a set request.
        //
        if(argc > 2)
        {
            //
            // Get the fault timeout value to set.
            //
            plValue[0] = strtol(argv[2], 0, 0);

            //
            // Limit the value to valid settings.
            //
            if(plValue[0] < 0)
            {
                plValue[0] = 0;
            }
            if(plValue[0] > 65535)
            {
                plValue[0] = 65535;
            }

            UARTSendMessage(LM_API_CFG_FAULT_TIME | g_ulID,
                            (unsigned char *)plValue, 2);

            WaitForAck(LM_API_ACK | g_ulID, 10);
        }
        else
        {
            //
            // This is a request to retrieve the value.
            //
            UARTSendMessage(LM_API_CFG_FAULT_TIME | g_ulID, 0, 0);

            WaitForAck(LM_API_CFG_FAULT_TIME | g_ulID, 10);
        }
    }

    else
    {
        printf("%s [lines|turns|brake|limit|fwd|rev|maxvout|faulttime]\n",
               argv[0]);
    }

    return(0);
}

//*****************************************************************************
//
// This command handles periodic status requests for devices.
//
//*****************************************************************************
int
CmdPStatus(int argc, char *argv[])
{
    int iByteIdx, iIdx;
    unsigned long ulValue, ulPMsg;
    unsigned char pucBytes[8];

    //
    // All of these commands require a Periodic Message number.
    //
    if(argc > 2)
    {
        //
        // Get the message number from the parameter.
        //
        ulPMsg = strtoul(argv[2], 0, 0);

        //
        // Limit the number to a valid value.
        //
        if(ulPMsg > 3)
        {
            ulPMsg = 3;
        }

        //
        // Shift it to the appropriate location.
        //
        ulPMsg <<= CAN_MSGID_API_S;

        //
        // Handle the commands.
        //
        if(strcmp(argv[1], "int") == 0)
        {
            //
            // If there's a third parameter, this is a set request.
            //
            if(argc > 3)
            {
                //
                // Get the requested interval value from the parameter.
                //
                ulValue = strtoul(argv[3], 0, 0);

                //
                // Limit the data to valid values.
                //
                if(ulValue > 65535)
                {
                    ulValue = 65535;
                }

                //
                // Set the interval for target Periodic Message.
                //
                UARTSendMessage(LM_API_PSTAT_PER_EN_S0 | ulPMsg | g_ulID,
                    (unsigned char *)&ulValue, 2);
            }
            else
            {
                //
                // Request the current interval for requested periodic message.
                //
                UARTSendMessage(LM_API_PSTAT_PER_EN_S0 | ulPMsg | g_ulID, 0, 0);
            }
            //
            // Wait for a periodic status int response.
            //
            WaitForAck(LM_API_PSTAT_PER_EN_S0 | ulPMsg | g_ulID, 50);

        }
        else if(strcmp(argv[1], "cfg") == 0)
        {
            //
            // If there's a third parameter, this is a set request.
            //
            if(argc > 3)
            {
                //
                // Init the temp array to LM_PSTAT_END
                //
                memset(&pucBytes[0], LM_PSTAT_END, sizeof(unsigned char) * 8);

                //
                // There can only be up to eight IDs in a configuration
                // set request.
                //
                if((argc - 3) > PSTATUS_PAYLOAD_SZ)
                {
                    //
                    // Max of 8 IDs + command syntax
                    //
                    argc = PSTATUS_PAYLOAD_SZ + 3;
                }

                for(iByteIdx = 0; iByteIdx < (argc - 3); iByteIdx++)
                {
                    ulValue = 0;

                    //
                    // Identify if the string is a numerical representation
                    // (based on ASCII code of first character) if not assume
                    // it's a mnemonic.
                    //
                    if((argv[3 + iByteIdx][0] >= '0') &&
                        (argv[3 + iByteIdx][0] <= '9'))
                    {
                        //
                        // Get the requested interval value from the parameter.
                        //
                        ulValue = strtoul(argv[3 + iByteIdx], 0, 0);
                    }
                    else
                    {
                        //
                        // Find the mnemonic match.
                        //
                        for(iIdx = 1; g_sPStatMsgs[iIdx].pcMsgString != NULL;
                            iIdx++)
                        {
                            if(strcmp(argv[3 + iByteIdx],
                                g_sPStatMsgs[iIdx].pcMsgMnemonic) == 0)
                            {
                                ulValue = g_sPStatMsgs[iIdx].ulMsgID;
                            }
                        }

                    }

                    //
                    // Limit the ID to a valid value.
                    //
                    if(ulValue > PSTATUS_MAX_ID)
                    {
                        ulValue = PSTATUS_MAX_ID;
                    }

                    //
                    // Save the value to the array.
                    //
                    pucBytes[iByteIdx] = ulValue;
                }

                //
                // Set the ID config for target Periodic Message.
                //
                UARTSendMessage((LM_API_PSTAT_CFG_S0 + ulPMsg) | g_ulID,
                    (unsigned char *)&pucBytes[0], 8);
            }
            else
            {
                //
                // Request the current configuration for Periodic Message.
                //
                UARTSendMessage((LM_API_PSTAT_CFG_S0 + ulPMsg) | g_ulID, 0, 0);
            }

            //
            // Wait for a periodic status config response.
            //
            WaitForAck((LM_API_PSTAT_CFG_S0 + ulPMsg) | g_ulID, 50);
        }
    }
    else
    {
        printf("%s [int|cfg] <pmsg#>\n", argv[0]);
    }

    return(0);
}

//*****************************************************************************
//
// This command handler takes care of the system level commands.
//
//*****************************************************************************
int
CmdSystem(int argc, char *argv[])
{
    unsigned long ulValue;
    char pcBuffer[8];

    if((argc > 1) && (strcmp(argv[1], "halt") == 0))
    {
        //
        // Broadcast a Halt command.
        //
        UARTSendMessage(CAN_MSGID_API_SYSHALT, 0, 0);
    }
    else if((argc > 1) && (strcmp(argv[1], "resume") == 0))
    {
        //
        // Broadcast a Resume command.
        //
        UARTSendMessage(CAN_MSGID_API_SYSRESUME, 0, 0);
    }
    else if((argc > 1) && (strcmp(argv[1], "reset") == 0))
    {
        //
        // Broadcast a system reset command.
        //
        UARTSendMessage(CAN_MSGID_API_SYSRST, 0, 0);
    }
    else if((argc > 1) && (strcmp(argv[1], "enum") == 0))
    {
        //
        // Broadcast a system enumeration command.
        //
        UARTSendMessage(CAN_MSGID_API_ENUMERATE, 0, 0);

        //
        // Wait for a device query response.
        //
        WaitForAck(CAN_MSGID_API_DEVQUERY | g_ulID, 100);
    }
    else if((argc > 1) && (strcmp(argv[1], "assign") == 0))
    {
        //
        // There must be a value to set the device ID to before continuing.
        //
        if(argc > 2)
        {
            //
            // Get the requested ID from the parameter.
            //
            ulValue = strtoul(argv[2], 0, 0);

            //
            // Check if this request for a non-zero ID.
            //
            if(ulValue == 0)
            {
                UARTSendMessage(CAN_MSGID_API_DEVASSIGN,
                                (unsigned char *)&ulValue, 1);
            }
            else if(ulValue < MAX_CAN_ID)
            {
                //
                // Send out the device assignment ID and wait for a response
                // from a device.
                //
                UARTSendMessage(CAN_MSGID_API_DEVASSIGN,
                                (unsigned char *)&ulValue, 1);

                if(g_bUseGUI)
                {

                  //
                  // Give it some time to take effect.
                  //
                  for(ulValue = 5; ulValue > 0; ulValue--)
                  {
                    sprintf(pcBuffer, "...%ld...", ulValue);
                    g_pSystemAssign->copy_label(pcBuffer);

                    Fl::check();

                    OSSleep(1);
                  }

                  g_pSystemAssign->copy_label("Assign");
                  Fl::check();
                }
                else
                {
                    for(ulValue = 5; ulValue > 0; ulValue--)
                    {
                        printf("\r%ld", ulValue);
                        OSSleep(1);
                    }
                    printf("\r");
                }
            }
            else
            {
                printf("%s %s: the ID must be between 0 and 63.\n", argv[0],
                       argv[1]);
            }
        }
        else
        {
            printf("%s %s <id>\n", argv[0], argv[1]);
        }
    }
    else if((argc > 1) && (strcmp(argv[1], "query") == 0))
    {
        //
        // Handle the device query command that will return information about
        // the device.
        //
        UARTSendMessage(CAN_MSGID_API_DEVQUERY | g_ulID, 0, 0);

        WaitForAck(CAN_MSGID_API_DEVQUERY | g_ulID, 10);
    }
    else if((argc > 1) && (strcmp(argv[1], "sync") == 0))
    {
        //
        // Send out a synchronous update command.
        //
        if(argc > 2)
        {
            //
            // Get the synchronous update ID from the parameter.
            //
            ulValue = strtoul(argv[2], 0, 0);

            UARTSendMessage(CAN_MSGID_API_SYNC,
                            (unsigned char *)&ulValue, 1);
        }
        else
        {
            printf("%s %s <group>\n", argv[0], argv[1]);
        }
    }
    else if((argc > 1) && (strcmp(argv[1], "version") == 0))
    {
        //
        // Send out a firmware version number request.
        //
        UARTSendMessage(CAN_MSGID_API_FIRMVER | g_ulID, 0, 0);

        WaitForAck(CAN_MSGID_API_FIRMVER | g_ulID, 10);
    }
    else if((argc > 1) && (strcmp(argv[1], "hwver") == 0))
    {
        //
        // Send out a hardware version number request.
        //
        UARTSendMessage(LM_API_HWVER | g_ulID, 0, 0);

        WaitForAck(LM_API_HWVER | g_ulID, 10);
    }
    else
    {
        printf("%s [halt|resume|reset|enum|assign|query|sync|version|hwver]\n",
               argv[0]);
    }

    return(0);
}

//*****************************************************************************
//
// This function handles the firmware update command.
//
//*****************************************************************************
int
CmdUpdate(int argc, char *argv[])
{
    unsigned char *pucBuffer, pucData[8];
    unsigned long ulLength, ulIdx, ulHeartbeatSave;
    FILE *pFile;

    if(argc > 1)
    {
        //
        // Attempt to open the requested file.
        //
        pFile = fopen(argv[1], "rb");
        if(!pFile)
        {
            if(g_bUseGUI)
            {
                fl_alert("Unable to open specified file!");
            }
            else
            {
                printf("%s: Unable to open '%s'.\n", argv[0], argv[1]);
            }

            return(-1);
        }

        //
        // Find out the size of the file.
        //
        fseek(pFile, 0, SEEK_END);
        ulLength = ftell(pFile);
        fseek(pFile, 0, SEEK_SET);

        //
        // Allocate a buffer for the file.
        //
        pucBuffer = (unsigned char *)malloc(ulLength);

        if(!pucBuffer)
        {
            if(g_bUseGUI)
            {
                fl_alert("Unable to allocate memory for update!");
            }
            else
            {
                printf("%s: Unable to allocate memory for '%s'.\n", argv[0],
                       argv[1]);
            }

            //
            // Free the buffer.
            //
            free(pucBuffer);

            fclose(pFile);
            return(0);
        }

        //
        // Read the full file into the buffer and close out the file.
        //
        fread(pucBuffer, 1, ulLength, pFile);
        fclose(pFile);

        //
        // Remember the current heart beat setting and then disable the system
        // heart beats during the update.
        //
        ulHeartbeatSave = g_ulHeartbeat;
        g_ulHeartbeat = 0;

        //
        // If the ID is non-zero then send out a request to the specific ID to
        // force the update on.
        //
        if(g_ulID != 0)
        {
            pucData[0] = g_ulID;

            UARTSendMessage(CAN_MSGID_API_UPDATE, pucData, 1);

            usleep(50000);
        }

        //
        // Attempt to ping the CAN boot loader.
        //
        UARTSendMessage(LM_API_UPD_PING, 0, 0);

        //
        // Wait for an acknowledgment from the device from the boot loader.
        //
        if(WaitForAck(LM_API_UPD_ACK, 250) == -1)
        {
            if(g_bUseGUI)
            {
                fl_alert("Unable to contact the boot loader!");
            }
            else
            {
                printf("%s: Unable to contact the boot loader.\n", argv[0]);
            }

            free(pucBuffer);

            return(0);
        }

        if(!g_bUseGUI)
        {
            printf("  0%%");
        }

        //
        // Create and send the download request to the boot loader.
        //
        *(unsigned long *)pucData = 0x800;
        *(unsigned long *)(pucData + 4) = ulLength;
        UARTSendMessage(LM_API_UPD_DOWNLOAD, pucData, 8);

        if(WaitForAck(LM_API_UPD_ACK, 4000) == -1)
        {
            if(g_bUseGUI)
            {
                fl_alert("Failed to erase the device's flash!");
            }
            else
            {
                printf("%s: Failed to erase the device's flash.\n", argv[0]);
            }

            free(pucBuffer);

            return(0);
        }

        //
        // Send out the new firmware to the device.
        //
        for(ulIdx = 0; ulIdx < ulLength; ulIdx += 8)
        {
            if(g_bUseGUI)
            {
                if(g_ulID == 0)
                {
                    g_pRecoverProgress->value(((ulIdx + 8) * 100) / ulLength);
                }
                else
                {
                    g_pUpdateProgress->value(((ulIdx + 8) * 100) / ulLength);
                }
                Fl::check();
            }
            else
            {
                printf("\r%3ld%%", ((ulIdx + 8) * 100) / ulLength);
            }

            if((ulIdx + 8) > ulLength)
            {
                UARTSendMessage(LM_API_UPD_SEND_DATA,
                                pucBuffer + ulIdx, ulLength - ulIdx);
            }
            else
            {
                UARTSendMessage(LM_API_UPD_SEND_DATA,
                                pucBuffer + ulIdx, 8);
            }
            if(WaitForAck(LM_API_UPD_ACK, 250) == -1)
            {
                if(g_bUseGUI)
                {
                    fl_alert("Failed to program the device's flash!");
                }
                else
                {
                    printf("%s: Failed to program the device's flash.\n",
                           argv[0]);
                }

                free(pucBuffer);

                return(0);
            }
        }

        if(g_bUseGUI)
        {
            if(g_ulID == 0)
            {
                g_pRecoverProgress->value(100);
            }
            else
            {
                g_pUpdateProgress->value(100);
            }
            Fl::check();
        }
        else
        {
            printf("\r    \r");
        }

        UARTSendMessage(LM_API_UPD_RESET, 0, 0);

        free(pucBuffer);

        g_ulHeartbeat = ulHeartbeatSave;
    }
    else
    {
        printf("%s <filename>\n", argv[0]);
    }

    return(0);
}

//*****************************************************************************
//
// Handle the boot loader forced button update.
//
//*****************************************************************************
int
CmdBoot(int argc, char *argv[])
{
    int iRet;
    unsigned long g_ulSavedID;

    //
    // Save the global ID.
    //
    g_ulSavedID = g_ulID;

    //
    // Set the global ID to 0 so that we only update devices that are in the
    // boot loader already.
    //
    g_ulID = 0;

    if(argc < 2)
    {
        printf("%s <filename>\n", argv[0]);
        return(0);
    }

    //
    // Just do a reset to allow updating without losing power.
    //
    UARTSendMessage(CAN_MSGID_API_SYSRST, 0, 0);

    printf("Waiting on a boot request\n");

    //
    // Send a generic updater ping to keep the state of the application ok.
    //
    UARTSendMessage(LM_API_UPD_PING, 0, 0);

    //
    // Now wait for a request to boot.
    //
    do
    {
        iRet = WaitForAck(LM_API_UPD_REQUEST, 10);
        printf(".");
    } while(iRet == -1);

    //
    // Got the request so respond and start updating.
    //
    UARTSendMessage(LM_API_UPD_REQUEST, 0, 0);

    if(WaitForAck(LM_API_UPD_ACK, 10) >= 0)
    {
        printf("\nUpdating\n");

        if(CmdUpdate(argc, argv) < 0)
        {
            UARTSendMessage(LM_API_UPD_RESET, 0, 0);
        }
    }
    else
    {
        printf("\nFailed to detect boot loader\n");
    }

    //
    // Restore the global ID.
    //
    g_ulID = g_ulSavedID;

    return(0);
}

//*****************************************************************************
//
// Handle shutting down the applcation.
//
//*****************************************************************************
int
CmdExit(int argc, char *argv[])
{
    CloseUART();
    exit(0);
}

//*****************************************************************************
//
// This function implements the "help" command.  It prints a simple list
// of the available commands with a brief description.
//
//*****************************************************************************
int
CmdHelp(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    //
    // Point at the beginning of the command table.
    //
    pEntry = &g_sCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The
    // end of the table has been reached when the command name is NULL.
    //
    while(pEntry->pcCmd)
    {
        //
        // Print the command name and the brief description.
        //
        printf("%s%s\n", pEntry->pcCmd, pEntry->pcHelp);

        //
        // Advance to the next entry in the table.
        //
        pEntry++;
    }

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// The table of the commands supported by the application.
//
//*****************************************************************************
tCmdLineEntry g_sCmdTable[] =
{
    { "help",       CmdHelp, "      - display a list of commands" },
    { "h",          CmdHelp, "         - alias for help" },
    { "?",          CmdHelp, "         - alias for help" },
    { "id",         CmdID, "        - set the target ID" },
    { "heartbeat",  CmdHeartbeat, " - start/stop the heartbeat" },
    { "volt",       CmdVoltage, "      - voltage control mode commands" },
    { "vcomp",      CmdVComp, "     - voltage compensation mode commands" },
    { "cur",        CmdCurrent, "       - current control mode commands" },
    { "speed",      CmdSpeed, "     - speed control mode commands" },
    { "pos",        CmdPosition, "       - position control mode commands" },
    { "stat",       CmdStatus, "      - status commands" },
    { "config",     CmdConfig, "    - configuration commands" },
    { "system",     CmdSystem, "    - system commands" },
    { "pstat",      CmdPStatus, "     - periodic status commands" },
    { "update",     CmdUpdate, "    - update the firmware" },
    { "boot",       CmdBoot, "      - wait for boot loader to request update" },
    { "exit",       CmdExit, "      - exit the program" },
    { "quit",       CmdExit, "      - alias for exit" },
    { "q",          CmdExit, "         - alias for exit" },
    { 0, 0, 0 }
};

//*****************************************************************************
//
// PStatus helper function to centralize the byte replacement of a
// Fixed 16:16 value. Reference float is provided as a double, converted
// to 16:16, then the byte is replaced and it is converted back to a double
// which is returned.
//
//*****************************************************************************
static double
PeriodicStatusModifyFixed16(double dRefVal, unsigned char ucData, int iByteIdx)
{
    char pcTempString[100];
    long lValue;

    //
    // Convert the reference value to 16:16.
    //
    lValue = (long)(dRefVal * 65536);

    //
    // Mask out the replacement position.
    //
    lValue &= ~(0xff << iByteIdx);

    //
    // OR in the replacement byte.
    //
    lValue |= (ucData << iByteIdx);

    //
    // Convert back to a float via strtod().
    //
    sprintf(pcTempString, "%s%ld.%03ld", (lValue < 0) ?
        "-" : "", (lValue < 0) ? ((0 - lValue) / 65536) :
        (lValue / 65536), (lValue < 0) ?
        ((((0 - lValue) % 65536) * 1000) / 65536) :
        (((lValue % 65536) * 1000) / 65536));
    return(strtod(pcTempString, NULL));
}

//*****************************************************************************
//
// PStatus helper function to centralize the byte replacement of a
// Fixed 8:8 value. Reference float is provided as a double, converted
// to 8:8, them the byte is replaced and it is converted back to a double
// which is returned.
//
//*****************************************************************************
static double
PeriodicStatusModifyFixed8(double dRefVal, unsigned char ucData, int iByteIdx)
{
    char pcTempString[100];
    long lValue;

    //
    // Convert the reference value from double back to 8:8 fixed
    //
    lValue = (short)(dRefVal * 256);

    //
    // Mask out the replacement position.
    //
    lValue &= ~(0xff << iByteIdx);

    //
    // OR in the replacement byte.
    //
    lValue |= (ucData << iByteIdx);

    //
    // Convert back to a float via strtod().
    //
    sprintf(pcTempString, "%ld.%02ld", (lValue / 256),
        (((lValue % 256) * 100) / 256));
    return(strtod(pcTempString, NULL));
}

//*****************************************************************************
//
// This function handles updating the current status display.
//
//*****************************************************************************
void
UpdateStatus(void *pvData)
{
    unsigned char *pucPayload, ucData;
    char pcTempString[100];
    int iUpdIdx, iCfgIdx, iIdx;
    long lValue;
    double dValue;
    unsigned long ulHistoryFlags;
    double pdHistData[6];
    char pcLimits[20], pcFaults[20];
    time_t tRawTime;
    struct tm * tLocalTime;
    Fl_Text_Buffer *pBuffer;

    if(!g_bConnected)
    {
        return;
    }

    //
    // Update the status items on the GUI.
    //
    g_pStatusVout->value(g_sBoardStatus.fVout);
    g_pStatusVbus->value(g_sBoardStatus.fVbus);
    g_pStatusCurrent->value(g_sBoardStatus.fCurrent);
    g_pStatusTemperature->value(g_sBoardStatus.fTemperature);
    g_pStatusPosition->value(g_sBoardStatus.fPosition);
    g_pStatusSpeed->value(g_sBoardStatus.fSpeed);
    if(g_sBoardStatus.lPower > 0)
    {
        GUI_ENABLE_INDICATOR(g_pStickyFaultIndicatorPowr);
    }
    sprintf(pcTempString, "0x%02x", g_sBoardStatus.ucCANRxErrors);
    g_pStatusCanRxErr->value(pcTempString);
    sprintf(pcTempString, "0x%02x", g_sBoardStatus.ucCANTxErrors);
    g_pStatusCanTxErr->value(pcTempString);
    sprintf(pcTempString, "0x%02x", g_sBoardStatus.ucCANStatus);
    g_pStatusCanSts->value(pcTempString);
    sprintf(pcTempString, "%d", g_sBoardStatus.ucVoltageFaults);
    g_pStatusVoltageFaults->value(pcTempString);
    sprintf(pcTempString, "%d", g_sBoardStatus.ucTemperatureFaults);
    g_pStatusTemperatureFaults->value(pcTempString);
    sprintf(pcTempString, "%d", g_sBoardStatus.ucCurrentFaults);
    g_pStatusCurrentFaults->value(pcTempString);
    sprintf(pcTempString, "%d", g_sBoardStatus.ucCommFaults);
    g_pStatusCommFaults->value(pcTempString);
    sprintf(pcTempString, "%d", g_sBoardStatus.ucGateFaults);
    g_pStatusGateFaults->value(pcTempString);

    //
    // Init the character string array.
    //
    memset(&pcTempString[0], 0, sizeof(char) * 100);

    //
    // Update the limit widgets. Create a string with status of the
    // limit switches.
    //
    sprintf(pcTempString, "%c%c",
            (g_sBoardState[g_ulID].ucLimits & LM_STATUS_LIMIT_FWD) ? '.' :
                                                          'F',
            (g_sBoardState[g_ulID].ucLimits & LM_STATUS_LIMIT_REV) ? '.' :
                                                          'R');

    //
    // Update the GUI with the new value.
    //
    g_pStatusLimit->value(pcTempString);

    //
    // Create a string with the sticky status of the limit switches.
    //
    sprintf(pcTempString, "%c%c",
            (g_sBoardState[g_ulID].ucLimits & LM_STATUS_LIMIT_STKY_FWD) ?
                                                        '.' : 'F',
            (g_sBoardState[g_ulID].ucLimits & LM_STATUS_LIMIT_STKY_REV) ?
                                                        '.' : 'R');

    //
    // Update the GUI with the new value.
    //
    g_pStatusStickyLimit->value(pcTempString);

    //
    // Create a string with the Soft limit status.
    //
    sprintf(pcTempString, "%c%c",
            (g_sBoardState[g_ulID].ucLimits & LM_STATUS_LIMIT_SFWD) ?
                                                        '.' : 'F',
            (g_sBoardState[g_ulID].ucLimits & LM_STATUS_LIMIT_SREV) ?
                                                        '.' : 'R');

    //
    // Update the GUI with the new value.
    //
    g_pStatusSoftLimit->value(pcTempString);

    //
    // Create string with the stcky soft stat of the limit switches.
    //
    sprintf(pcTempString, "%c%c",
            (g_sBoardState[g_ulID].ucLimits & LM_STATUS_LIMIT_STKY_SFWD) ?
                                                        '.' : 'F',
            (g_sBoardState[g_ulID].ucLimits & LM_STATUS_LIMIT_STKY_SREV) ?
                                                        '.' : 'R');

    //
    // Update the GUI with the new value.
    //
    g_pStatusSoftStickyLimit->value(pcTempString);

    if(g_sBoardStatus.lFault)
    {
        switch(g_sBoardStatus.lFault)
        {
            case LM_FAULT_CURRENT:
            {
                strcpy(g_pcFaultTxt, "CUR FAULT");
                break;
            }

            case LM_FAULT_TEMP:
            {
                strcpy(g_pcFaultTxt, "TEMP FAULT");
                break;
            }

            case LM_FAULT_VBUS:
            {
                strcpy(g_pcFaultTxt, "VBUS FAULT");
                break;
            }

            case LM_FAULT_GATE_DRIVE:
            {
                strcpy(g_pcFaultTxt, "GATE FAULT");
                break;
            }

            case LM_FAULT_COMM:
            {
                //
                // By definition this should never happen.
                //
                strcpy(g_pcFaultTxt, "COMM FAULT");
                break;
            }
        }

        g_pStatusFault->value(g_pcFaultTxt);
        g_pStatusFault->show();
    }
    else
    {
        g_pStatusFault->hide();
    }

    //
    // Check if updated PStatus information was received.
    //
    for(iUpdIdx = 0; iUpdIdx < PSTATUS_MSGS_NUM; iUpdIdx++)
    {
        //
        // Did parser flag an update?
        //
        if(!(g_sBoardStatus.lBoardFlags & (PSTAT_STATEF_UPD << iUpdIdx)))
        {
            //
            // Not for this PStatus message, skip.
            //
            continue;
        }

        //
        // Clear the update flag.
        //
        g_sBoardStatus.lBoardFlags &= ~(PSTAT_STATEF_UPD << iUpdIdx);

        //
        // Reset the PStat History vars
        //
        memset(&pdHistData[0], 0, sizeof(double) * 6);

        //
        // Get the Buffer pointer for this PStatus Message.
        //
        if(g_sBoardStatus.ppvPStatusMsgHistory[iUpdIdx] == NULL)
        {
            GUIPeriodicStatusHistorySetup(iUpdIdx);
        }
        pBuffer =
            (Fl_Text_Buffer *)g_sBoardStatus.ppvPStatusMsgHistory[iUpdIdx];

        for(iCfgIdx = 0, ulHistoryFlags = 0; iCfgIdx < PSTATUS_PAYLOAD_SZ;
            iCfgIdx++)
        {
            //
            // Based on the Config ID for the message, save the status
            // data to the appropriate board status variable.
            //
            pucPayload = &g_sBoardStatus.pucPStatusMsgPayload[iUpdIdx][iCfgIdx];
            switch(g_sBoardStatus.pulPStatusMsgCfgs[iUpdIdx][iCfgIdx])
            {
                case LM_PSTAT_END:
                case LM_PSTAT_VOLTOUT_B0:
                case LM_PSTAT_VOLTOUT_B1:
                {
                    //
                    // Not processed.
                    //
                    break;
                }

                case LM_PSTAT_VOLTBUS_B0:
                case LM_PSTAT_VOLTBUS_B1:
                {
                    //
                    // Get the byte position based on message ID (0-1)
                    //
                    iIdx = (g_sBoardStatus.pulPStatusMsgCfgs[iUpdIdx][iCfgIdx] -
                        LM_PSTAT_VOLTBUS_B0) * 8;

                    //
                    // Call the helper function to manage the byte replacement.
                    //
                    g_sBoardStatus.fVbus =
                        PeriodicStatusModifyFixed8(g_sBoardStatus.fVbus,
                        *pucPayload, iIdx);

                    //
                    // Pstatus history data collection.
                    //
                    ulHistoryFlags |= PSTAT_LEGEND_F_VBUS;
                    pdHistData[1] = PeriodicStatusModifyFixed8(pdHistData[1],
                        *pucPayload, iIdx);
                    break;
                }

                case LM_PSTAT_CURRENT_B0:
                case LM_PSTAT_CURRENT_B1:
                {
                    //
                    // Get the byte position based on message ID (0-1)
                    //
                    iIdx = (g_sBoardStatus.pulPStatusMsgCfgs[iUpdIdx][iCfgIdx] -
                        LM_PSTAT_CURRENT_B0) * 8;

                    //
                    // Call the helper function to manage the byte replacement.
                    //
                    g_sBoardStatus.fCurrent =
                        PeriodicStatusModifyFixed8(g_sBoardStatus.fCurrent,
                        *pucPayload, iIdx);
                    ulHistoryFlags |= PSTAT_LEGEND_F_CURR;
                    pdHistData[2] = PeriodicStatusModifyFixed8(pdHistData[2],
                        *pucPayload, iIdx);
                    break;
                }

                case LM_PSTAT_TEMP_B0:
                case LM_PSTAT_TEMP_B1:
                {
                    //
                    // Get the byte position based on message ID (0-1)
                    //
                    iIdx = (g_sBoardStatus.pulPStatusMsgCfgs[iUpdIdx][iCfgIdx] -
                        LM_PSTAT_TEMP_B0) * 8;

                    //
                    // Call the helper function to manage the byte replacement.
                    //
                    g_sBoardStatus.fTemperature =
                        PeriodicStatusModifyFixed8(g_sBoardStatus.fTemperature,
                        *pucPayload, iIdx);
                    ulHistoryFlags |= PSTAT_LEGEND_F_TEMP;
                    pdHistData[3] = PeriodicStatusModifyFixed8(pdHistData[3],
                        *pucPayload, iIdx);
                    break;
                }

                case LM_PSTAT_POS_B0:
                case LM_PSTAT_POS_B1:
                case LM_PSTAT_POS_B2:
                case LM_PSTAT_POS_B3:
                {
                    //
                    // Get the byte position based on message ID (0-3)
                    //
                    iIdx = (g_sBoardStatus.pulPStatusMsgCfgs[iUpdIdx][iCfgIdx] -
                        LM_PSTAT_POS_B0) * 8;

                    //
                    // Insert in the replacement byte to our value.
                    //
                    g_sBoardStatus.fPosition =
                        PeriodicStatusModifyFixed16(g_sBoardStatus.fPosition,
                        *pucPayload, iIdx);
                    ulHistoryFlags |= PSTAT_LEGEND_F_POS;
                    pdHistData[4] = PeriodicStatusModifyFixed16(pdHistData[4],
                        *pucPayload, iIdx);
                    break;
                }

                case LM_PSTAT_SPD_B0:
                case LM_PSTAT_SPD_B1:
                case LM_PSTAT_SPD_B2:
                case LM_PSTAT_SPD_B3:
                {
                    //
                    // Get the byte position based on message ID (0-3)
                    //
                    iIdx = (g_sBoardStatus.pulPStatusMsgCfgs[iUpdIdx][iCfgIdx] -
                        LM_PSTAT_SPD_B0) * 8;
                    //
                    // Insert in the replacement byte to our value.
                    //
                    g_sBoardStatus.fSpeed =
                        PeriodicStatusModifyFixed16(g_sBoardStatus.fSpeed,
                        *pucPayload, iIdx);
                    ulHistoryFlags |= PSTAT_LEGEND_F_SPD;
                    pdHistData[5] = PeriodicStatusModifyFixed16(pdHistData[5],
                        *pucPayload, iIdx);
                    break;
                }

                case LM_PSTAT_LIMIT_NCLR:
                case LM_PSTAT_LIMIT_CLR:
                {
                    //
                    // Clear the non-sticky status bits.
                    //
                    g_sBoardState[g_ulID].ucLimits &= ~(LM_STATUS_LIMIT_FWD |
                        LM_STATUS_LIMIT_REV | LM_STATUS_LIMIT_SFWD |
                        LM_STATUS_LIMIT_SREV);

                    //
                    // Or in Sticky (additive) and non-sticky status updates.
                    //
                    g_sBoardState[g_ulID].ucLimits |= *pucPayload;
                    ulHistoryFlags |= PSTAT_LEGEND_F_LIMIT;

                    //
                    // Build all-Limit string for History display.
                    //
                    ucData = g_sBoardState[g_ulID].ucLimits;
                    sprintf(pcLimits, " %c%c%c%c%c%c%c%c  ",
                        (ucData & LM_STATUS_LIMIT_FWD) ? '.' : 'F',
                        (ucData & LM_STATUS_LIMIT_REV) ? '.' : 'R',
                        (ucData & LM_STATUS_LIMIT_STKY_FWD) ? '.' : 'F',
                        (ucData & LM_STATUS_LIMIT_STKY_REV) ? '.' : 'R',
                        (ucData & LM_STATUS_LIMIT_SFWD) ? '.' : 'F',
                        (ucData & LM_STATUS_LIMIT_SREV) ? '.' : 'R',
                        (ucData & LM_STATUS_LIMIT_STKY_SFWD) ? '.' : 'F',
                        (ucData & LM_STATUS_LIMIT_STKY_SREV) ? '.' : 'R');
                    break;
                }

                case LM_PSTAT_FAULT:
                {
                    lValue = *(unsigned short *)(pucPayload);
                    ulHistoryFlags |= PSTAT_LEGEND_F_FAULTS;
                    //
                    // Build Limit string for History display.
                    //
                    g_sBoardStatus.lFault = lValue;
                    sprintf(pcFaults, "   %c%c%c%c%c   ",
                        (lValue & LM_FAULT_COMM) ? 'C' : '.',
                        (lValue & LM_FAULT_CURRENT) ? 'I' : '.',
                        (lValue & LM_FAULT_TEMP) ? 'T' : '.',
                        (lValue & LM_FAULT_GATE_DRIVE) ? 'G' : '.',
                        (lValue & LM_FAULT_VBUS) ? 'V' : '.');
                    break;
                }

                case LM_PSTAT_STKY_FLT_NCLR:
                case LM_PSTAT_STKY_FLT_CLR:
                {
                    g_sBoardState[g_ulID].ulStkyFault |= *pucPayload;
                    ulHistoryFlags |= PSTAT_LEGEND_F_FAULTS;
                    //
                    // Build Limit string for History display.
                    //
                    break;
                }

                case LM_PSTAT_VOUT_B0:
                case LM_PSTAT_VOUT_B1:
                {
                    //
                    // Get the byte position based on message ID (0-1)
                    //
                    iIdx = (g_sBoardStatus.pulPStatusMsgCfgs[iUpdIdx][iCfgIdx] -
                        LM_PSTAT_VOUT_B0) * 8;

                    //
                    // Call the helper function to manage the byte replacement.
                    //
                    g_sBoardStatus.fVout =
                        PeriodicStatusModifyFixed8(g_sBoardStatus.fVout,
                        *pucPayload, iIdx);
                    ulHistoryFlags |= PSTAT_LEGEND_F_VOUT;
                    pdHistData[0] = PeriodicStatusModifyFixed8(pdHistData[0],
                        *pucPayload, iIdx);

                    break;
                }

                case LM_PSTAT_FLT_COUNT_CURRENT:
                {
                    g_sBoardStatus.ucCurrentFaults = *pucPayload;
                    ulHistoryFlags |= PSTAT_LEGEND_F_CURR_FLT;
                    break;
                }

                case LM_PSTAT_FLT_COUNT_TEMP:
                {
                    g_sBoardStatus.ucTemperatureFaults = *pucPayload;
                    ulHistoryFlags |= PSTAT_LEGEND_F_TEMP_FLT;
                    break;
                }

                case LM_PSTAT_FLT_COUNT_VOLTBUS:
                {
                    g_sBoardStatus.ucVoltageFaults = *pucPayload;
                    ulHistoryFlags |= PSTAT_LEGEND_F_VBUS_FLT;
                    break;
                }

                case LM_PSTAT_FLT_COUNT_GATE:
                {
                    g_sBoardStatus.ucGateFaults = *pucPayload;
                    ulHistoryFlags |= PSTAT_LEGEND_F_GATE_FLT;
                    break;
                }

                case LM_PSTAT_FLT_COUNT_COMM:
                {
                    g_sBoardStatus.ucCommFaults = *pucPayload;
                    ulHistoryFlags |= PSTAT_LEGEND_F_COMM_FLT;
                    break;
                }

                case LM_PSTAT_CANSTS:
                {
                    g_sBoardStatus.ucCANStatus = *pucPayload;
                    ulHistoryFlags |= PSTAT_LEGEND_F_CAN_STS;
                    break;
                }

                case LM_PSTAT_CANERR_B0:
                {
                    g_sBoardStatus.ucCANRxErrors = *pucPayload;
                    ulHistoryFlags |= PSTAT_LEGEND_F_CAN_RX_ERR;
                    break;
                }

                case LM_PSTAT_CANERR_B1:
                {
                    g_sBoardStatus.ucCANTxErrors = *pucPayload;
                    ulHistoryFlags |= PSTAT_LEGEND_F_CAN_TX_ERR;
                    break;
                }
            }
        }

        if((ulHistoryFlags > 0) && (pBuffer != NULL))
        {
            //
            // Add timestamp to PStat History buffer
            //
            time(&tRawTime);
            tLocalTime = localtime(&tRawTime);
            sprintf(pcTempString, " %02d:%02d:%02d |", tLocalTime->tm_hour,
                tLocalTime->tm_min, tLocalTime->tm_sec);
            pBuffer->append(pcTempString);

            //
            // Add data to PStat History buffer
            //
            for(iIdx = 0; ulHistoryFlags > 0; iIdx++, ulHistoryFlags >>= 1)
            {
                if(ulHistoryFlags & 0x01)
                {
                    switch(1 << iIdx)
                    {
                        case PSTAT_LEGEND_F_VOUT:
                        case PSTAT_LEGEND_F_VBUS:
                        case PSTAT_LEGEND_F_CURR:
                        case PSTAT_LEGEND_F_TEMP:
                        case PSTAT_LEGEND_F_POS:
                        case PSTAT_LEGEND_F_SPD:
                        {
                            sprintf(pcTempString, "   %02.2f  ",
                                    pdHistData[iIdx]);
                            break;
                        }

                        case PSTAT_LEGEND_F_CURR_FLT:
                        {
                            sprintf(pcTempString, "    %03d   ",
                                    g_sBoardStatus.ucCurrentFaults);
                            break;
                        }

                        case PSTAT_LEGEND_F_TEMP_FLT:
                        {
                            sprintf(pcTempString, "    %03d   ",
                                    g_sBoardStatus.ucTemperatureFaults);
                            break;
                        }

                        case PSTAT_LEGEND_F_VBUS_FLT:
                        {
                            sprintf(pcTempString, "    %03d   ",
                                    g_sBoardStatus.ucVoltageFaults);
                            break;
                        }

                        case PSTAT_LEGEND_F_GATE_FLT:
                        {
                            sprintf(pcTempString, "    %03d   ",
                                    g_sBoardStatus.ucGateFaults);
                            break;
                        }

                        case PSTAT_LEGEND_F_COMM_FLT:
                        {
                            sprintf(pcTempString, "    %03d   ",
                                    g_sBoardStatus.ucCommFaults);
                            break;
                        }

                        case PSTAT_LEGEND_F_CAN_STS:
                        {
                            sprintf(pcTempString, "  0x%03x   ",
                                    g_sBoardStatus.ucCANStatus);
                            break;
                        }

                        case PSTAT_LEGEND_F_CAN_RX_ERR:
                        {
                            sprintf(pcTempString, "  0x%03x   ",
                                    g_sBoardStatus.ucCANRxErrors);
                            break;
                        }

                        case PSTAT_LEGEND_F_CAN_TX_ERR:
                        {
                            sprintf(pcTempString, "  0x%03x   ",
                                    g_sBoardStatus.ucCANTxErrors);
                            break;
                        }

                        case PSTAT_LEGEND_F_LIMIT:
                        {
                            strcpy(pcTempString, pcLimits);
                            break;
                        }

                        case PSTAT_LEGEND_F_FAULTS:
                        {
                            strcpy(pcTempString, pcFaults);
                            break;
                        }
                    }

                    //
                    // String setup, add it.
                    //
                    pBuffer->append(pcTempString);

                    //
                    // If more flags are set, add the spacer.
                    //
                    if((ulHistoryFlags >> 1) > 0)
                    {
                        lValue = (11 - strlen(pcTempString));

                        //
                        // Pad out the column to account
                        // for expected worst case (11).
                        //
                        while(lValue-- > 0)
                        {
                            pBuffer->append(" ");
                        }
                        pBuffer->append("|");
                    }
                }
            }

            //
            // Some data was added, so append a newline.
            //
            pBuffer->append("\n");
        }

        //
        // Trigger a PStat history widget update.
        //

    }

    if(g_sBoardState[g_ulID].ulStkyFault)
    {
        if(g_sBoardState[g_ulID].ulStkyFault & LM_FAULT_CURRENT)
        {
            GUI_ENABLE_INDICATOR(g_pStickyFaultIndicatorCurr);
        }

        if(g_sBoardState[g_ulID].ulStkyFault & LM_FAULT_TEMP)
        {
            GUI_ENABLE_INDICATOR(g_pStickyFaultIndicatorTemp);
        }

        if(g_sBoardState[g_ulID].ulStkyFault & LM_FAULT_VBUS)
        {
            GUI_ENABLE_INDICATOR(g_pStickyFaultIndicatorVbus);
        }

        if(g_sBoardState[g_ulID].ulStkyFault & LM_FAULT_GATE_DRIVE)
        {
            GUI_ENABLE_INDICATOR(g_pStickyFaultIndicatorGate);
        }

        if(g_sBoardState[g_ulID].ulStkyFault & LM_FAULT_COMM)
        {
            GUI_ENABLE_INDICATOR(g_pStickyFaultIndicatorComm);
        }
    }
}

//*****************************************************************************
//
// This function handles sending out heart beats to the devices.
//
//*****************************************************************************
void *
HeartbeatThread(void *pvData)
{
    unsigned short usToken;
    int iIndex;

    //
    // Set this thread to the highest priority.
    //
#ifdef __WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#else
    // TODO: Linux equivalent.
#endif

    while(1)
    {
        usleep(50000);

        //
        // If there is no COM port, or no board, then do nothing.
        //
        if(!g_bConnected)
        {
            continue;
        }

        if(g_ulHeartbeat)
        {
            UARTSendMessage(CAN_MSGID_API_HEARTBEAT, 0, 0);
        }
    }
}

//*****************************************************************************
//
// The thread used to update the status while the application is running.
//
//*****************************************************************************
void *
BoardStatusThread(void *pvData)
{
    while(1)
    {
        //
        // Sleep for 500ms.
        //
        usleep(500000);

        //
        // If there is no COM port, or no board, then do nothing.
        //
        if(!g_bConnected)
        {
            continue;
        }

        //
        // If the board status is active, get the data.
        //
        if(g_ulBoardStatus)
        {
            //
            // Set the global active flag for this thread.  This blocks the GUI
            // from accessing to the receive functionality of the COM port
            // until the status has been updated.
            //
            g_bBoardStatusActive = true;

            //
            // The first "argument" will always be "stat".
            //
            strcpy(g_argv[0], "stat");

            //
            // Get the power.
            //
            strcpy(g_argv[1], "power");
            CmdStatus(2, g_argv);
            usleep(1000);

            //
            // Only get data for these items if there is not a Periodic
            // Status message providing it.
            //
            if(!PeriodicStatusIsMessageOn(LM_PSTAT_SPD_B0) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_SPD_B1) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_SPD_B2) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_SPD_B3))
            {
                //
                // Get the speed.
                //
                strcpy(g_argv[1], "speed");
                CmdStatus(2, g_argv);
                usleep(1000);
            }

            if(!PeriodicStatusIsMessageOn(LM_PSTAT_CURRENT_B0) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_CURRENT_B1))
            {
                //
                // Get the current.
                //
                strcpy(g_argv[1], "cur");
                CmdStatus(2, g_argv);
                usleep(1000);
            }

            if(!PeriodicStatusIsMessageOn(LM_PSTAT_VOLTBUS_B0) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_VOLTBUS_B1))
            {
                //
                // Get the bus voltage.
                //
                strcpy(g_argv[1], "vbus");
                CmdStatus(2, g_argv);
                usleep(1000);
            }

            if(!PeriodicStatusIsMessageOn(LM_PSTAT_VOLTOUT_B0) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_VOLTOUT_B1))
            {
                //
                // Get the output voltage.
                //
                strcpy(g_argv[1], "vout");
                CmdStatus(2, g_argv);
                usleep(1000);
            }

            if(!PeriodicStatusIsMessageOn(LM_PSTAT_TEMP_B0) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_TEMP_B1))
            {
                //
                // Get the temperature.
                //
                strcpy(g_argv[1], "temp");
                CmdStatus(2, g_argv);
                usleep(1000);
            }

            if(!PeriodicStatusIsMessageOn(LM_PSTAT_POS_B0) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_POS_B1) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_POS_B2) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_POS_B3))
            {
                //
                // Get the motor position.
                //
                strcpy(g_argv[1], "pos");
                CmdStatus(2, g_argv);
                usleep(1000);
            }

            if(!PeriodicStatusIsMessageOn(LM_PSTAT_LIMIT_CLR))
            {
                //
                // Get the limit switch position.
                //
                strcpy(g_argv[1], "limit");
                CmdStatus(2, g_argv);
                usleep(1000);
            }

            if(!PeriodicStatusIsMessageOn(LM_PSTAT_FAULT))
            {
                //
                // Get the fault status.
                //
                strcpy(g_argv[1] , "fault");
                CmdStatus(2, g_argv);
                usleep(1000);
            }

            if(!PeriodicStatusIsMessageOn(LM_PSTAT_FLT_COUNT_CURRENT) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_FLT_COUNT_TEMP) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_FLT_COUNT_VOLTBUS) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_FLT_COUNT_GATE) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_FLT_COUNT_COMM) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_CANSTS) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_CANERR_B0) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_CANERR_B1))
            {
                //
                // Get the fault counters.
                //
                strcpy(g_argv[1] , "faultcnts");
                CmdStatus(2, g_argv);
            }

            if(!PeriodicStatusIsMessageOn(LM_PSTAT_STKY_FLT_NCLR) &&
               !PeriodicStatusIsMessageOn(LM_PSTAT_STKY_FLT_CLR))
            {
                //
                // Get the sticky fault status.
                //
                strcpy(g_argv[1] , "stkyfault");
                CmdStatus(2, g_argv);
            }

            //
            // Clear the global active flag for this thread.  This grants the
            // GUI access to the receive functionality of the COM port while
            // status is not being updated.
            //
            g_bBoardStatusActive = false;

            //
            // Tell the main thread to update the status values in the GUI.
            //
            if(g_bConnected)
            {
                Fl::awake(UpdateStatus, 0);
            }
        }
    }
}

//*****************************************************************************
//
// Finds the Jaguars on the network.
//
//*****************************************************************************
void
FindJaguars(void)
{
    int iIdx;

    //
    // Initialize the status structure list to all 0's.
    //
    for(iIdx = 0; iIdx < MAX_CAN_ID; iIdx++)
    {
        g_sBoardState[iIdx].ulControlMode = LM_STATUS_CMODE_VOLT;
    }

    //
    // Send the enumerate command.
    //
    strcpy(g_argv[0], "system");
    strcpy(g_argv[1], "enum");
    CmdSystem(2, g_argv);
}

//*****************************************************************************
//
// The main control loop.
//
//*****************************************************************************
int
main(int argc, char *argv[])
{
    int iIdx;
    char pcBuffer[256];
    long lCode;

#ifndef __WIN32
    pthread_t thread;
#endif
#ifdef __WIN32
    WSADATA wsaData;
#endif

    //
    // Initialize the board structure
    //
    memset(&g_sBoardStatus, 0, sizeof(tBoardStatus));

    //
    // If running on Windows, initialize the COM library (required for multi-
    // threading).
    //
#ifdef __WIN32
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif

    setbuf(stdout, 0);

    //
    // Decide whether or not to start the GUI based on input arguments.
    //
    if(argc == 1)
    {
        //
        // Hide the console window.
        //
#ifdef __WIN32
        FreeConsole();
#endif

        //
        // Create and show the main window.
        //
        CreateMainAppWindow()->show(argc, argv);

        //
        // Set the flag to use the GUI.
        //
        g_bUseGUI = true;
    }
    else
    {
        while((lCode = getopt(argc, argv, "?c:h")) != -1)
        {
            switch(lCode)
            {
                case 'c':
                {
#ifdef __WIN32
                    sprintf(g_szCOMName, "\\\\.\\COM%s", optarg);
#else
                    strncpy(g_szCOMName, optarg, sizeof(g_szCOMName));
#endif
                    break;
                }

                case 'h':
                case '?':
                {
                    Usage(argv[0]);
                    return(1);
                }

                default:
                {
                    fprintf(stderr, "Try `%s -h' for more information.\n",
                            basename(argv[0]));
                    return(1);
                }
            }
        }

        //
        // Set the flag to not use GUI.
        //
        g_bUseGUI = false;
    }

    //
    // If using the GUI, populate the COM port drop down menu.
    //
    if(g_bUseGUI && (GUIFillCOMPortDropDown() == 0))
    {
        fl_alert("There are no COM ports on your computer...exiting.");
        exit(1);
    }

    //
    // Open the COM port.
    //
    if(g_bUseGUI)
    {
        GUIConnect();
    }
    else
    {
        if(OpenUART(g_szCOMName, 115200))
        {
            printf("Failed to configure Host UART\n");
            return(-1);
        }
        g_bConnected = true;
    }

    if(g_bUseGUI)
    {
        //
        // Init Sticky related elements.
        //
        for(iIdx = 0; iIdx < MAX_CAN_ID; iIdx++)
        {
            g_sBoardState[iIdx].ulStkyFault = 0;
            g_sBoardState[iIdx].ucLimits = 0xff;
        }

        //
        // Prepare FLTK for multi-threaded operation.
        //
        Fl::lock();
    }


    //
    // Initialize the mutex that restricts access to the COM port.
    //
    MutexInit(&mMutex);

    //
    // Create the heart beat thread.
    //
    OSThreadCreate(HeartbeatThread);

    //
    // Decide what to do based on the g_bUseGUI flag.
    //
    if(g_bUseGUI)
    {
        //
        // When the GUI is active, start the board status thread.
        //
        OSThreadCreate(BoardStatusThread);

        //
        // Signal a clear of the COMM related errors on startup.
        // This is a meaningless error to a newly connected device.
        //
        GUIExtendedStatusFaultCountSelect(LM_FAULT_COMM);
        g_bIgnoreCOMM = true;

        //
        // Handle the FLTK events in the main thread.
        //
        iIdx = Fl::run();

        //
        // Deallocate FLTK objects for PStat History
        //
        for(iIdx = 0; iIdx < PSTATUS_MSGS_NUM; iIdx++)
        {
            delete (Fl_Text_Buffer *)(g_sBoardStatus.ppvPStatusMsgHistory[iIdx]);
        }

        return(iIdx);
    }
    else
    {

        //
        // Begin the main loop for the command line version of the tool.
        //
        while(1)
        {
            printf("\n# ");
            if(fgets(pcBuffer, sizeof(pcBuffer), stdin) == 0)
            {
                printf("\n");
                CmdExit(0, 0);
            }

            while((pcBuffer[strlen(pcBuffer) - 1] == '\r') ||
            (pcBuffer[strlen(pcBuffer) - 1] == '\n'))
            {
                pcBuffer[strlen(pcBuffer) - 1] = '\0';
            }

            if(CmdLineProcess(pcBuffer) != 0)
            {
                printf("heartbeat|id|volt|vcomp|cur|speed|pos|stat|config|"
                       "pstat|system|update|help|exit\n");
            }
        }
    }
}
