//*****************************************************************************
//
// bluetooth.h - Definitions and prototypes for the Bluetooth interface
//               module for Stellaris Bluetooth SPP Demo.
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
// This is part of revision 8555 of the DK-LM3S9B96-EM2-CC2560-BLUETOPIA Firmware Package.
//
//*****************************************************************************

#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#include "BTPSKRNL.h"

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
// The following defines the list of error code that can be returned from any
// API calls for this module.
//
//*****************************************************************************
#define BTH_ERROR_INVALID_PARAMETER (-1)
#define BTH_ERROR_REQUEST_FAILURE   (-2)
#define BTH_ERROR_NOT_ALLOWED       (-3)
#define BTH_ERROR_BUFFER_FULL       (-4)

//*****************************************************************************
//
// The following defines the default Pin Code for this application.
//
//*****************************************************************************
#define DEFAULT_PIN_CODE "0000"

//*****************************************************************************
//
// The following defines the default Name postfix that will be
// added to the end of the Device Name that is Discoverable by
// other Bluetooth Devices. Note, This name should be less
// than MAX_DEVICE_NAME_LENGTH - 17 bytes in length to
// accommodate the device name prefix
// 
//*****************************************************************************
#define DEFAULT_DEVICE_NAME_POSTFIX "_StellarisSPP"

//*****************************************************************************
//
// The following defines the values that are used for the Mode Bitmask.
//
//*****************************************************************************
#define CONNECTABLE_MODE_MASK   0x0001
#define NON_CONNECTABLE_MODE    0x0000
#define CONNECTABLE_MODE        0x0001

#define DISCOVERABLE_MODE_MASK  0x0002
#define NON_DISCOVERABLE_MODE   0x0000
#define DISCOVERABLE_MODE       0x0002

#define PAIRABLE_MODE_MASK      0x000C
#define NON_PAIRABLE_MODE       0x0000
#define PAIRABLE_NON_SSP_MODE   0x0004
#define PAIRABLE_SSP_MODE       0x0008

//*****************************************************************************
//
// The following defines the size limits of variable length data elements used
// in this module.
//
//*****************************************************************************
#define SIZE_OF_BD_ADDR         6
#define SIZE_OF_LINK_KEY        16
#define SIZE_OF_PIN_CODE        16
#define MAX_DEVICE_NAME_LENGTH  32

//*****************************************************************************
//
// The following macro is used to compare two Bluetooth addresses.  The macro
// will return TRUE if the two match and FALSE otherwise.
//
//*****************************************************************************
#define MATCH_BD_ADDR(sBD_ADDR1, sBD_ADDR2)                                   \
(                                                                             \
    ((sBD_ADDR1)[0] == (sBD_ADDR2)[0]) &&                                     \
    ((sBD_ADDR1)[1] == (sBD_ADDR2)[1]) &&                                     \
    ((sBD_ADDR1)[2] == (sBD_ADDR2)[2]) &&                                     \
    ((sBD_ADDR1)[3] == (sBD_ADDR2)[3]) &&                                     \
    ((sBD_ADDR1)[4] == (sBD_ADDR2)[4]) &&                                     \
    ((sBD_ADDR1)[5] == (sBD_ADDR2)[5])                                        \
)

//*****************************************************************************
//
// The following macro is used to compare two link key values.  The macro will
// return TRUE if the two match and FALSE otherwise.
//
//*****************************************************************************
#define MATCH_LINK_KEY(sLinkKey1, sLinkKey2)                                  \
(                                                                             \
    ((sLinkKey1)[0]  == (sLinkKey2)[0])  &&                                   \
    ((sLinkKey1)[1]  == (sLinkKey2)[1])  &&                                   \
    ((sLinkKey1)[2]  == (sLinkKey2)[2])  &&                                   \
    ((sLinkKey1)[3]  == (sLinkKey2)[3])  &&                                   \
    ((sLinkKey1)[4]  == (sLinkKey2)[4])  &&                                   \
    ((sLinkKey1)[5]  == (sLinkKey2)[5])  &&                                   \
    ((sLinkKey1)[6]  == (sLinkKey2)[6])  &&                                   \
    ((sLinkKey1)[7]  == (sLinkKey2)[7])  &&                                   \
    ((sLinkKey1)[8]  == (sLinkKey2)[8])  &&                                   \
    ((sLinkKey1)[9]  == (sLinkKey2)[9])  &&                                   \
    ((sLinkKey1)[10] == (sLinkKey2)[10]) &&                                   \
    ((sLinkKey1)[11] == (sLinkKey2)[11]) &&                                   \
    ((sLinkKey1)[12] == (sLinkKey2)[12]) &&                                   \
    ((sLinkKey1)[13] == (sLinkKey2)[13]) &&                                   \
    ((sLinkKey1)[14] == (sLinkKey2)[14]) &&                                   \
    ((sLinkKey1)[15] == (sLinkKey2)[15])                                      \
)

//*****************************************************************************
//
// The following structure represents the Device Info Data that contains
// information about the Local Bluetooth Device
//
//*****************************************************************************
typedef struct
{
    unsigned char ucBDAddr[SIZE_OF_BD_ADDR];
    unsigned char ucHCIVersion;
    unsigned short sMode;
    char cDeviceName[MAX_DEVICE_NAME_LENGTH + 1];
} tDeviceInfo;

//*****************************************************************************
//
// The following enumerated type is used with the callback event to identify
// the reason for the callback event.
//
//*****************************************************************************
typedef enum
{
   cePinCodeRequest,
   ceAuthenticationComplete,
   ceAuthenticationFailure,
   ceDeviceFound,
   ceDeviceRetry,
   ceDeviceConnectionFailure,
   ceDeviceConnected,
   ceDeviceDisconnected,
   ceDataReceived
} tCallbackEvent;

//*****************************************************************************
//
// The following structure represents the container structure that holds all
// callback event data.
//
//*****************************************************************************
typedef struct
{
    tCallbackEvent sEvent;
    unsigned char ucRemoteDevice[SIZE_OF_BD_ADDR];
} tCallbackEventData;

#define CALLBACK_EVENT_SIZE (sizeof(tCallbackEventData)

//*****************************************************************************
//
// The following is the callback function prototype
//
//*****************************************************************************
typedef void (*tBluetoothCallbackFn)(tCallbackEventData *pCbData,
                                    void *pvCbParameter);

//*****************************************************************************
//
// The following are the API Function Prototypes for this module
//
//*****************************************************************************
extern int InitializeBluetooth(tBluetoothCallbackFn pfnCallbackFunction,
                               void *pvCallbackParameter,
                               BTPS_Initialization_t *pBTPSInitialization);
extern int GetLocalDeviceInformation(tDeviceInfo *psDeviceInfo);
extern int SetLocalDeviceMode(unsigned short usMode);
extern int SetLocalDeviceName(char *pcDeviceName);
extern int PinCodeResponse(unsigned char *pucBDAddr, int iPinCodeLength,
                           char *pucPinCode);
extern int DeviceDiscovery(tBoolean bStartDiscovery);
extern int ReadData(unsigned int uiDataLength, unsigned char *pucData);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __BLUETOOTH_H__
