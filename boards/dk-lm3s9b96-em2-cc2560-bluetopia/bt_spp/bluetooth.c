//*****************************************************************************
//
// bluetooth.c - Bluetooth interface module for Stellaris Bluetooth SPP Demo
//               application.
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

#include <string.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_i2s.h"
#include "inc/hw_flash.h"
#include "driverlib/flash.h"

#include "BTPSKRNL.h"
#include "SS1BTPS.h"
#include "DISCAPI.h"
#include "bluetooth.h"

//*****************************************************************************
//
// A macro for displaying messages to the debug console using the Bluetooth
// stack utility.
//
//*****************************************************************************
#define Display(_x) DBG_MSG(DBG_ZONE_DEVELOPMENT, _x)

#define DEVICE_NAME_TO_CONNECT  "BlueMSP-Demo"
#define SPP_PORT_NUMBER         1
#define MAX_ATTEMPT_COUNT       2

//*****************************************************************************
//
// Defines an address in flash for storing link keys.  When a key needs to be
// updated the entire sector is erased so be sure this area of flash is not
// used for storing anything else.

//*****************************************************************************
#ifndef SAVED_LINK_KEY_ADDRESS
#define SAVED_LINK_KEY_ADDRESS 0x3F000
#endif

//*****************************************************************************
//
// The following macro is used to convert a BD_ADDR_t type into a character
// array.
//
//*****************************************************************************
#define BD_ADDR_To_Array(sBD_ADDR, pucArray)                                  \
do                                                                            \
{                                                                             \
    ((pucArray)[0] = (sBD_ADDR).BD_ADDR5);                                    \
    ((pucArray)[1] = (sBD_ADDR).BD_ADDR4);                                    \
    ((pucArray)[2] = (sBD_ADDR).BD_ADDR3);                                    \
    ((pucArray)[3] = (sBD_ADDR).BD_ADDR2);                                    \
    ((pucArray)[4] = (sBD_ADDR).BD_ADDR1);                                    \
    ((pucArray)[5] = (sBD_ADDR).BD_ADDR0);                                    \
} while(0)

//*****************************************************************************
//
// Define a structure type to hold a BD_ADDR to link key mapping.
//
//*****************************************************************************
typedef struct
{
    tBoolean bEmpty;
    BD_ADDR_t BD_ADDR;
    Link_Key_t sLinkKey;
} tLinkKeyInfo;

#define NUM_SUPPORTED_LINK_KEYS 5

//*****************************************************************************
//
// Static Local Variable Definitions.
//
//*****************************************************************************
static GAP_Authentication_Information_t g_sAuthenticationInfo;
static unsigned int g_uiBluetoothStackID;
static tDeviceInfo g_sDeviceInfo;
static tBluetoothCallbackFn g_pfnCallbackFunction;
static void *g_pvCallbackParameter;
static tLinkKeyInfo g_sLinkKeyInfo[NUM_SUPPORTED_LINK_KEYS];
static Class_of_Device_t g_sClassOfDevice;
static unsigned int g_uiSerialPortID;
static BD_ADDR_t g_sRemoteBD_ADDR;
static unsigned int g_uiRetryCount;

//*****************************************************************************
//
// The following structure contains the information that will be supplied to
// the Bluetooth chip to support Extended Inquiry Result.  At minimum, the
// Device Name and Power Level should be advertised.
//
//*****************************************************************************
static const unsigned char g_ucEIR[] =
{
    0x09,
    HCI_EXTENDED_INQUIRY_RESPONSE_DATA_TYPE_LOCAL_NAME_COMPLETE,
    'S', 'P', 'P', ' ', 'D', 'e', 'm', 'o',
    0x02,
    HCI_EXTENDED_INQUIRY_RESPONSE_DATA_TYPE_TX_POWER_LEVEL,
    0,
    0x0
};

//*****************************************************************************
//
// Forward declaration
//
//*****************************************************************************
static tBoolean SPPConnect(BD_ADDR_t sBD_ADDR);

//*****************************************************************************
//
// The following function is used to read a chunk of data from the flash
// memory.  The function will transfer the data from the location allocated
// for link key storage for as many bytes as defined by the iLength parameter.
// The second parameter specifies a caller-supplied buffer where the data that
// is read from flash will be stored.
//
//*****************************************************************************
static void
ReadFlash(int iLength, unsigned char *pucDest)
{
#if (SAVED_LINK_KEY_ADDRESS != 0)
    BTPS_MemCopy(pucDest, (void *)SAVED_LINK_KEY_ADDRESS, iLength);
#endif
}

//*****************************************************************************
//
// The following function is used to write a chunk of data to the flash memory.
// The function will write the data to the location in flash allocated for
// link key storage.  The first parameter defines the number of bytes to be
// written to flash, and the second parameter points to a buffer that holds
// the data that is to be written.
//
//*****************************************************************************
static void
WriteFlash(int iLength, unsigned char *pucSrc)
{
#if (SAVED_LINK_KEY_ADDRESS != 0)
    unsigned long ulErasePages;

    //
    // Compute the number of pages that need to be erased.
    //
    ulErasePages = (iLength + (FLASH_ERASE_SIZE - 1)) / FLASH_ERASE_SIZE;

    //
    // Erase the pages needed to store the new data.
    //
    while(ulErasePages--)
    {
        FlashErase(SAVED_LINK_KEY_ADDRESS + (ulErasePages * FLASH_ERASE_SIZE));
    }

    //
    // Make sure iLength is multiple of 4
    //
    iLength = (iLength + 3) & ~0x3;;

    //
    // Program the data into the flash
    //
    FlashProgram((unsigned long *)pucSrc, SAVED_LINK_KEY_ADDRESS, iLength);
#endif
}

//*****************************************************************************
//
// The following function is used to locate a link key for the supplied
// Bluetooth address.  If located, the function will return the index of the
// link key in the LinkKeyInfo structure.  If a link key was not located the
// function returns a negative number.
//
//*****************************************************************************
static int
LocateLinkKey(BD_ADDR_t sBD_ADDR)
{
    int iIdx;

    //
    // Loop through the list for a match for the BD_ADDR.
    //
    for(iIdx = 0; iIdx < NUM_SUPPORTED_LINK_KEYS; iIdx++)
    {
        if((!g_sLinkKeyInfo[iIdx].bEmpty) &&
           (COMPARE_BD_ADDR(g_sLinkKeyInfo[iIdx].BD_ADDR, sBD_ADDR)))
        {
            return(iIdx);
        }
    }

    //
    // If we fell out of loop above, then no match was found
    //
    return(-1);
}

//*****************************************************************************
//
// The following function is used to locate a link key for the supplied
// Bluetooth address.  If located, the function will remove the entry from the
// LinkKeyInfo structure and repack the structure.  If a link key was not
// located the function returns a negative number.
//
//*****************************************************************************
static int
DeleteLinkKey(BD_ADDR_t sBD_ADDR)
{
    int iIdx;

    //
    // Find the specified key in the link key storage
    //
    iIdx = LocateLinkKey(sBD_ADDR);

    //
    // If a valid index is returned, then remove it from the storage by
    // overwriting it while repacking the remaining keys in the structure.
    //
    if(iIdx >= 0)
    {
        //
        // Loop through remaining keys and slide all the remaining keys down
        // by one slot in the structure.
        //
        for(; iIdx < (NUM_SUPPORTED_LINK_KEYS - 1); iIdx++)
        {
            g_sLinkKeyInfo[iIdx] = g_sLinkKeyInfo[iIdx + 1];
        }

        //
        // Flag the last entry as free.
        //
        BTPS_MemInitialize(&g_sLinkKeyInfo[NUM_SUPPORTED_LINK_KEYS - 1],
                           0xFF, sizeof(Link_Key_t));

        //
        // Rewrite the new key info structure to flash.
        //
        WriteFlash(sizeof(g_sLinkKeyInfo), (unsigned char *)&g_sLinkKeyInfo);

        //
        // Return a success indication
        //
        return(1);
    }

    //
    // Else the key was not found so return an error.
    //
    else
    {
        return(-1);
    }
}

//*****************************************************************************
//
// The following function is used to save a new link key in the link key
// storage structure.  If the remote device address is already present in
// storage, then its link key will be updated and no new slots are used.
// If an empty slot is available then that will be used for the new device
// address and link key.  Otherwise, if there are no empty slots, then the
// oldest address/key pair will be deleted and the new one added in its
// place.
// The function returns the index of the saved key.
//
//*****************************************************************************
static int
SaveLinkKeyInfo(BD_ADDR_t sBD_ADDR, Link_Key_t sLinkKey)
{
    int iIdx;

    //
    // Check to see if this remote device address is already present in the
    // key info structure
    //
    iIdx = LocateLinkKey(sBD_ADDR);

    //
    // The device address is already in storage
    //
    if(iIdx >= 0)
    {
        //
        // Check the stored key for this device address.  If it is not the
        // same then update the entry with the new link key and rewrite
        // the structure to flash.
        //
        if(!COMPARE_LINK_KEY(g_sLinkKeyInfo[iIdx].sLinkKey, sLinkKey))
        {
            g_sLinkKeyInfo[iIdx].sLinkKey = sLinkKey;
            WriteFlash(sizeof(g_sLinkKeyInfo),
                       (unsigned char *)&g_sLinkKeyInfo);
        }
    }

    //
    // The device address is not already in storage
    //
    else
    {
        //
        // Loop through the list looking for an empty slot.
        //
        for(iIdx = 0; iIdx < NUM_SUPPORTED_LINK_KEYS; iIdx++)
        {
            if(g_sLinkKeyInfo[iIdx].bEmpty)
            {
                //
                // Empty slot was found so stop looking
                //
                break;
            }
        }

        //
        // Check to see if we found an empty slot.  If not, then we will
        // replace the oldest entry.
        //
        if(iIdx == NUM_SUPPORTED_LINK_KEYS)
        {
            //
            // Move all of the link key information down 1 slot.  The will
            // delete the key in the first slot, which is the oldest.
            //
            for(iIdx = 1; iIdx < NUM_SUPPORTED_LINK_KEYS; iIdx++)
            {
                g_sLinkKeyInfo[iIdx - 1] = g_sLinkKeyInfo[iIdx];
            }

            //
            // Set the index of the free slot to be the last entry, which is
            // now available.
            //
            iIdx = NUM_SUPPORTED_LINK_KEYS - 1;
        }

        //
        // Save the link key information in the available slot (found above)
        //
        g_sLinkKeyInfo[iIdx].bEmpty = 0;
        g_sLinkKeyInfo[iIdx].BD_ADDR = sBD_ADDR;
        g_sLinkKeyInfo[iIdx].sLinkKey = sLinkKey;

        //
        // Save the updated link key structure to flash
        //
        WriteFlash(sizeof(g_sLinkKeyInfo),
                   (unsigned char *)&g_sLinkKeyInfo);
    }

    //
    // Return index of the saved key
    //
    return(iIdx);
}

//*****************************************************************************
//
// The following function is used to issue a callback to the registered
// callback function.
//
//*****************************************************************************
static void
IssueCallback(BD_ADDR_t *psBD_ADDR, tCallbackEvent eEvent)
{
    tCallbackEventData sCallbackEventData;

    //
    // Verify that there is a valid callback function.
    //
    if(g_pfnCallbackFunction)
    {
        //
        // Load the callback event type
        //
        sCallbackEventData.sEvent = eEvent;

        //
        // Load the remote device address, if available
        //
        if(psBD_ADDR)
        {
            BD_ADDR_To_Array(*psBD_ADDR, sCallbackEventData.ucRemoteDevice);
        }

        //
        // Call the user callback function
        //
        g_pfnCallbackFunction(&sCallbackEventData, g_pvCallbackParameter);
    }
}

//*****************************************************************************
//
// The following function provides a means to respond to a Pin Code request.
// The function takes as its parameters a pointer to the Bluetooth Address of
// the device that requested the Pin Code and a pointer to the Pin Code data as
// well as the number length of the data pointed to by the PinCode.  The
// function return zero on success and a negative number on failure.
// * NOTE * The PinCodeLength must be more than zero and less SIZE_OF_PIN_CODE
//          Bytes.
//
//*****************************************************************************
int
PinCodeResponse(unsigned char *pucBD_ADDR, int iPinCodeLength,
                char *pucPinCode)
{
    int iRetVal;
    BD_ADDR_t sRemoteBD_ADDR;

    //
    // Verify that the parameters passed in appear valid.
    //
    if(pucBD_ADDR && iPinCodeLength && (iPinCodeLength <= SIZE_OF_PIN_CODE) &&
       pucPinCode)
    {
        //
        // Assign the Bluetooth Address information.
        //
        ASSIGN_BD_ADDR(sRemoteBD_ADDR, pucBD_ADDR[0], pucBD_ADDR[1],
                                       pucBD_ADDR[2], pucBD_ADDR[3],
                                       pucBD_ADDR[4], pucBD_ADDR[5]);

        //
        // Setup the Authentication Information Response structure.
        //
        g_sAuthenticationInfo.GAP_Authentication_Type = atPINCode;
        g_sAuthenticationInfo.Authentication_Data_Length = iPinCodeLength;
        BTPS_MemCopy(&(g_sAuthenticationInfo.Authentication_Data.PIN_Code),
                     pucPinCode, iPinCodeLength);

        //
        // Submit the Authentication Response.
        //
        iRetVal = GAP_Authentication_Response(g_uiBluetoothStackID,
                                              sRemoteBD_ADDR,
                                              &g_sAuthenticationInfo);

        //
        // Check the result of the submitted command.
        //
        if(!iRetVal)
        {
            Display(("GAP_Authentication_Response() Success.\n"));
        }
        else
        {
            Display(("GAP_Authentication_Response() Failure: %d.\n", iRetVal));
            iRetVal = BTH_ERROR_REQUEST_FAILURE;
        }
    }
    else
    {
        iRetVal = BTH_ERROR_INVALID_PARAMETER;
    }

    return(iRetVal);
}

//*****************************************************************************
//
// GAP Event Callback Function.  When this function is called it may be
// operating in the context of another thread, so thread safety should be
// considered.
//
//*****************************************************************************
static void BTPSAPI
GAP_EventCallback(unsigned int uiBluetoothStackID,
                  GAP_Event_Data_t *psGAPEventData,
                  unsigned long ulCallbackParameter)
{
    int iIndex;
    BD_ADDR_t sBD_ADDR;

    //
    // Verify that the parameters passed in appear valid.
    //
    if(uiBluetoothStackID && psGAPEventData)
    {
        //
        // Process each event type
        //
        switch(psGAPEventData->Event_Data_Type)
        {
            //
            // Authentication event
            //
            case etAuthentication:
            {
                GAP_Authentication_Event_Data_t *pData;
                pData = psGAPEventData->
                            Event_Data.GAP_Authentication_Event_Data;

                //
                // Determine which kind of authentication event occurred.
                //
                switch(pData->GAP_Authentication_Event_Type)
                {
                    //
                    // Link key request from remote device
                    //
                    case atLinkKeyRequest:
                    {
                        //
                        // Show a message on the debug console
                        //
                        Display(("GAP_LinkKeyRequest\n"));

                        //
                        // Get the address of the Device that is requesting the
                        // link key.
                        //
                        sBD_ADDR = pData->Remote_Device;

                        //
                        // Search for a link Key for the remote device.  If
                        // located send the link key, else indicate that we do
                        // not have one.
                        //
                        iIndex = LocateLinkKey(sBD_ADDR);
                        if(iIndex >= 0)
                        {
                            Display(("Located Link Key at Index %d\n", iIndex));
                            g_sAuthenticationInfo.Authentication_Data_Length =
                                    sizeof(Link_Key_t);
                            g_sAuthenticationInfo.Authentication_Data.Link_Key =
                                    g_sLinkKeyInfo[iIndex].sLinkKey;
                        }
                        else
                        {
                            Display(("No Link Key Found\n"));
                            g_sAuthenticationInfo.Authentication_Data_Length =
                                    0;
                        }

                        //
                        // Submit the authentication response.
                        //
                        g_sAuthenticationInfo.GAP_Authentication_Type =
                                atLinkKey;
                        GAP_Authentication_Response(uiBluetoothStackID,
                                                    sBD_ADDR,
                                                    &g_sAuthenticationInfo);
                        break;
                    }

                    //
                    // PIN code request
                    //
                    case atPINCodeRequest:
                    {
                        //
                        // Show a message on the debug console
                        //
                        Display(("GAP_PINCodeRequest\n"));

                        //
                        // Get the address of the device for the PIN code
                        // request
                        //
                        sBD_ADDR = psGAPEventData->Event_Data.
                                GAP_Authentication_Event_Data->
                                Remote_Device;

                        //
                        // Issue the call back to the user function
                        //
                        IssueCallback(&sBD_ADDR, cePinCodeRequest);

                        break;
                    }

                    //
                    // Link key creation
                    //
                    case atLinkKeyCreation:
                    {
                        //
                        // Show a message on the debug console
                        //
                        Display(("GAP_LinkKeyCreation\n"));

                        //
                        // Save the Link Key that was created.
                        //
                        iIndex = SaveLinkKeyInfo(psGAPEventData->Event_Data.
                                                 GAP_Authentication_Event_Data->
                                                 Remote_Device,
                                                psGAPEventData->Event_Data.
                                                GAP_Authentication_Event_Data->
                                                Authentication_Event_Data.
                                                Link_Key_Info.Link_Key);
                        Display(("SaveLinkKeyInfo returned %d\n", iIndex));
                        break;
                    }

                    //
                    // IO capability request
                    //
                    case atIOCapabilityRequest:
                    {
                        GAP_IO_Capabilities_t *pCap;
                        GAP_Authentication_Event_Data_t *pData;
                        pCap = &g_sAuthenticationInfo.Authentication_Data.
                                    IO_Capabilities;
                        pData = psGAPEventData->
                                    Event_Data.GAP_Authentication_Event_Data;

                        //
                        // Show a message on the debug console
                        //
                        Display(("atIOCapabilityRequest\n"));

                        //
                        // Setup the Authentication Information Response
                        // structure.
                        //
                        g_sAuthenticationInfo.GAP_Authentication_Type =
                                atIOCapabilities;
                        g_sAuthenticationInfo.Authentication_Data_Length =
                                sizeof(GAP_IO_Capabilities_t);
                        pCap->IO_Capability = icNoInputNoOutput;
                        pCap->MITM_Protection_Required = FALSE;
                        pCap->OOB_Data_Present = FALSE;

                        //
                        // Submit the Authentication Response.
                        //
                        GAP_Authentication_Response(uiBluetoothStackID,
                                                    pData->Remote_Device,
                                                    &g_sAuthenticationInfo);
                        break;
                    }

                    //
                    // User confirmation request
                    //
                    case atUserConfirmationRequest:
                    {
                        GAP_Authentication_Event_Data_t *pData;
                        pData = psGAPEventData->
                                    Event_Data.GAP_Authentication_Event_Data;

                        //
                        // Show a message on the debug console
                        //
                        Display(("atUserConfirmationRequest\n"));

                        //
                        // Invoke JUST Works Process...
                        //
                        g_sAuthenticationInfo.GAP_Authentication_Type =
                                atUserConfirmation;
                        g_sAuthenticationInfo.Authentication_Data_Length =
                                (Byte_t)sizeof(Byte_t);
                        g_sAuthenticationInfo.Authentication_Data.Confirmation =
                                TRUE;

                        //
                        // Submit the Authentication Response.
                        //
                        Display(("Autoaccept: %d\n",
                                pData->Authentication_Event_Data.Numeric_Value));
                        GAP_Authentication_Response(uiBluetoothStackID,
                                                    pData->Remote_Device,
                                                    &g_sAuthenticationInfo);
                        break;
                    }

                    //
                    // Authentication status
                    //
                    case atAuthenticationStatus:
                    {
                        GAP_Authentication_Event_Data_t *pData;
                        pData = psGAPEventData->
                                    Event_Data.GAP_Authentication_Event_Data;

                        //
                        // Show a message on the debug console
                        //
                        Display(("atAuthenticationStatus\n"));
                        //
                        // Check to see if we were successful.  If not, then
                        // any saved link key is now invalid and we need to
                        // delete any link key that is associated with the
                        // remote device.
                        //
                        if(pData->
                               Authentication_Event_Data.Authentication_Status)
                        {
                            DeleteLinkKey(pData->Remote_Device);
                            Display(("Authentication Failure,  "
                                     "Deleting Link Key\n"));
                        }
                        break;
                    }

                    //
                    // Unknown authentication event
                    //
                    default:
                    {
                        break;
                    }
                }
                break;
            }

            //
            // Unknown GAP event
            //
            default:
            {
                break;
            }
        }
    }
}

//*****************************************************************************
//
// The following function is used to retrieve information about the local
// Bluetooth device.  The function takes as its parameter a pointer to a Device
// Info Structure that will be filled in by this function with information
// about the local device.  If the function fails to return the device
// information, the return value will be negative, otherwise the return value
// will be zero.
//
//*****************************************************************************
int
GetLocalDeviceInformation(tDeviceInfo *psDeviceInfo)
{
    int iRetVal;

    //
    // Verify that the parameters passed in appear valid.
    //
    if(psDeviceInfo)
    {
        //
        // Copy the local device info to the user.
        //
        BTPS_MemCopy(psDeviceInfo, &g_sDeviceInfo, sizeof(tDeviceInfo));
        iRetVal = 0;
    }
    else
    {
        iRetVal = BTH_ERROR_INVALID_PARAMETER;
    }

    return(iRetVal);
}

//*****************************************************************************
//
// The following function is used to set the local device mode.  The function
// takes as its parameter a bit-mask that indicates the state of the various
// modes.  It should be noted that if the device supports Secured Simple
// Pairing, then once it has been enabled then it can't be disabled.
//
//*****************************************************************************
int
SetLocalDeviceMode(unsigned short usMode)
{
    int iRetVal;
    GAP_Pairability_Mode_t sPairableMode;

    //
    // Make sure that 2 modes are not being set.
    //
    if((usMode & PAIRABLE_MODE_MASK) != PAIRABLE_MODE_MASK)
    {
        //
        // Determine the mode that is being enabled.
        //
        sPairableMode = pmNonPairableMode;
        if(usMode & PAIRABLE_NON_SSP_MODE)
        {
            sPairableMode = pmPairableMode;
        }
        if(usMode & PAIRABLE_SSP_MODE)
        {
            sPairableMode = pmPairableMode_EnableSecureSimplePairing;
        }

        //
        // Attempt to set the mode.
        //
        iRetVal = GAP_Set_Pairability_Mode(g_uiBluetoothStackID,
                                           sPairableMode);

        //
        // If there was no error and pairable mode is being enabled then we
        // need to register for remote authentication.
        //
        if(!iRetVal)
        {
            //
            // If we are in a Pairable mode, then register the Authentication
            // callback.
            //
            if(sPairableMode != pmNonPairableMode)
            {
                //
                // Register and Authentication Callback.
                //
                GAP_Register_Remote_Authentication(g_uiBluetoothStackID,
                                                   GAP_EventCallback, 0);
            }

            //
            // Check the Connectability Mode.
            //
            if(usMode & CONNECTABLE_MODE)
            {
                GAP_Set_Connectability_Mode(g_uiBluetoothStackID,
                                            cmConnectableMode);
            }
            else
            {
                GAP_Set_Connectability_Mode(g_uiBluetoothStackID,
                                            cmNonConnectableMode);
            }

            //
            // Check the Discoverability Mode.
            //
            if(usMode & DISCOVERABLE_MODE)
            {
                GAP_Set_Discoverability_Mode(g_uiBluetoothStackID,
                                             dmGeneralDiscoverableMode, 0);
            }
            else
            {
                GAP_Set_Discoverability_Mode(g_uiBluetoothStackID,
                                             dmNonDiscoverableMode, 0);
            }

            //
            // Save the current Mode settings.
            //
            g_sDeviceInfo.sMode = usMode;
        }

        //
        // There was an error setting pairability, set error return code
        //
        else
        {
            iRetVal = BTH_ERROR_REQUEST_FAILURE;
        }
    }

    //
    // There was an error in the parameters passed to this function, set
    // error return code.
    //
    else
    {
        iRetVal = BTH_ERROR_INVALID_PARAMETER;
    }

    return(iRetVal);
}

//*****************************************************************************
//
// The following function is used to set the local device name.  The function
// takes as its parameter a pointer to the new device name.  The function will
// truncate the name if the name exceeds MAX_DEVICE_NAME_LENGTH characters.
// The function returns zero on success and a negative number on failure.
//
//*****************************************************************************
int
SetLocalDeviceName(char *pcDeviceName)
{
    int iRetVal;

    //
    // Verify that the parameters passed in appear valid.
    //
    if(pcDeviceName)
    {
        //
        // Check to see that the length of the name is within the limits.
        //
        iRetVal = BTPS_StringLength(pcDeviceName);
        if(iRetVal > MAX_DEVICE_NAME_LENGTH)
        {
            //
            // Truncate the name to the Maximum length.
            //
            pcDeviceName[MAX_DEVICE_NAME_LENGTH] = 0;
            iRetVal = MAX_DEVICE_NAME_LENGTH;
        }

        //
        // Copy the device name plus the terminator.
        //
        BTPS_MemCopy(g_sDeviceInfo.cDeviceName, pcDeviceName, iRetVal + 1);

        //
        // Set the new device name.
        //
        if(GAP_Set_Local_Device_Name(g_uiBluetoothStackID,
                                     g_sDeviceInfo.cDeviceName))
        {
            iRetVal = BTH_ERROR_REQUEST_FAILURE;
        }
        else
        {
            iRetVal = 0;
        }
    }

    //
    // Function parameter was bad, set error return code
    //
    else
    {
        iRetVal = BTH_ERROR_INVALID_PARAMETER;
    }

    return(iRetVal);
}

//*****************************************************************************
//
// The following function is for an SPP Event callback.  This function will be
// called whenever a SPP Event occurs that is associated with the Bluetooth
// stack.  This function passes to the caller the SPP event data that occurred
// and the SPP event callback parameter that was specified when this callback
// was installed.  The caller is free to use the contents of the SPP event data
// ONLY in the context of this callback.  If the caller requires the data for
// a longer period of time, then the callback function MUST copy the data into
// another buffer.  This function is guaranteed NOT to be invoked more than
// once simultaneously for the specified installed callback (that is, this
// function DOES NOT have to be reentrant).  It needs to be noted however, that
// if the same callback is installed more than once, then the callbacks will be
// called serially.  Because of this, the processing in this function should
// be as efficient as possible.  It should also be noted that this function is
// called in the thread context of a thread that the user does NOT own.
// Therefore, processing in this function should be as efficient as possible
// (this argument holds anyway because another SPP Event will not be processed
// while this function call is outstanding).
// * NOTE * This function MUST NOT Block and wait for events that can only be
//          satisfied by Receiving SPP Event Packets.  A deadlock WILL occur
//          because NO SPP Event callbacks will be issued while this function
//          is outstanding.
//
//*****************************************************************************
static void
SPP_EventCallback(unsigned int uiBluetoothStackID,
                  SPP_Event_Data_t *psSPPEventData,
                  unsigned long ulCallbackParameter)
{
    int iReturnValue = 0;

    //
    // SEE SPPAPI.H for a list of all possible event types. This
    // program only services its required events.
    //

    //
    // Check parameter validity
    //
    if(psSPPEventData != NULL && uiBluetoothStackID)
    {
        //
        // Process callback event type
        //
        switch(psSPPEventData->Event_Data_Type)
        {
            //
            // Open port confirmation event
            //
            case etPort_Open_Confirmation:
            {
                SPP_Open_Port_Confirmation_Data_t *pData;
                pData = psSPPEventData->
                            Event_Data.SPP_Open_Port_Confirmation_Data;

                //
                // Show status message to console
                //
                Display(("\r\nSPP Open Port Confirmation, ID: 0x%X, "
                         "Status 0x%X.\r\n", pData->SerialPortID,
                         pData->PortOpenStatus));

                //
                // Check the status to make sure that an error did not occur.
                //
                if(pData->PortOpenStatus)
                {
                    //
                    // Determine if we can attempt to connect again.
                    //
                    if(g_uiRetryCount)
                    {
                        //
                        // Attempt to connect again.
                        //
                        if(SPPConnect(g_sRemoteBD_ADDR))
                        {
                            //
                            // Issue a reconnecting callback.
                            //
                            IssueCallback(&g_sRemoteBD_ADDR, ceDeviceRetry);
                        }
                    }

                    //
                    // Otherwise no more retries
                    //
                    else
                    {
                        //
                        // Issue a connection failed callback.
                        //
                        IssueCallback(&g_sRemoteBD_ADDR,
                                      ceDeviceConnectionFailure);

                        //
                        // An error occurred while opening the Serial Port
                        // so invalidate the Serial Port ID.
                        //
                        g_uiSerialPortID = 0;
                    }
                }
                else
                {
                    //
                    // If serial port corresponds to opened serial port, issue
                    // callback to user app
                    //
                    if(pData->SerialPortID == g_uiSerialPortID)
                    {
                        IssueCallback(&g_sRemoteBD_ADDR, ceDeviceConnected);
                    }
                }
                break;
            }

            //
            // Port close indication event
            //
            case etPort_Close_Port_Indication:
            {
                SPP_Close_Port_Indication_Data_t *pData;
                pData = psSPPEventData->
                            Event_Data.SPP_Close_Port_Indication_Data;

                //
                // Show status message to console.
                //
                Display(("\r\nSPP Close Port Indication, ID: 0x%X\r\n",
                         pData->SerialPortID));

                //
                // Verify that the closed serial port corresponds to the
                // client serial port that we opened and callback to user app.
                //
                if(pData->SerialPortID == g_uiSerialPortID)
                {
                    IssueCallback(&g_sRemoteBD_ADDR, ceDeviceDisconnected);
                }

                //
                // Invalidate the Serial Port ID.
                //
                g_uiSerialPortID = 0;
                break;
            }

            //
            // Port status event
            //
            case etPort_Status_Indication:
            {
#ifdef DEBUG_ENABLED

                SPP_Port_Status_Indication_Data_t *pData;
                pData = psSPPEventData->
                            Event_Data.SPP_Port_Status_Indication_Data;

                //
                // Display port status information to the console
                //
                Display(("\r\nSPP Port Status Indication, ID: 0x%X, Status: "
                         "0x%X,Break Status: 0x%X, Length: 0x%X.\r\n",
                         pData->SerialPortID, pData->PortStatus,
                         pData->BreakStatus, pData->BreakTimeout));

#endif
                break;
            }

            //
            // Port data indication event
            //
            case etPort_Data_Indication:
            {
                SPP_Data_Indication_Data_t *pData;
                pData = psSPPEventData->
                            Event_Data.SPP_Data_Indication_Data;

                //
                // Show information to console
                //
                Display(("SPP Data Indication: 0x%X,Length: 0x%X.\r\n",
                         pData->SerialPortID, pData->DataLength));

                //
                // Verify that the closed serial port corresponds to the
                // client serial port that we opened, and callback to user app
                //
                if(pData->SerialPortID == g_uiSerialPortID)
                {
                    IssueCallback(&g_sRemoteBD_ADDR, ceDataReceived);
                }
                break;
            }

            //
            // Port send information indication event
            //
            case etPort_Send_Port_Information_Indication:
            {
                SPP_Send_Port_Information_Indication_Data_t *pData;
                pData = psSPPEventData->
                        Event_Data.SPP_Send_Port_Information_Indication_Data;

                //
                // Simply respond with the information that was sent to us.
                //
                iReturnValue = SPP_Respond_Port_Information(uiBluetoothStackID,
                                  pData->SerialPortID,
                                  &pData->SPPPortInformation);
                break;
            }

            //
            // Handle unknown event
            //
            default:
            {
                Display(("\r\nUnknown SPP Event.\r\n"));
                break;
            }
        }

        //
        // Check the return value of any function that might have been
        // executed in the callback.
        //
        if(iReturnValue)
        {
            //
            // An error occurred, so output an error message.
            //
            Display(("Error %d.\r\n", iReturnValue));
        }
    }
    else
    {
        //
        // There was an error with one or more of the input parameters.
        //
        Display(("SPP callback data: Event_Data = NULL.\r\n"));
    }
}

//*****************************************************************************
//
// The following function is used to attempt to connect to a remote
// SPP device.  This function returns TRUE if successful or FALSE
// it if fails.
//
//*****************************************************************************
static tBoolean
SPPConnect(BD_ADDR_t sBD_ADDR)
{
    int iResult;
    tBoolean bRetVal;

    //
    // Now let's attempt to open the Remote Serial Port
    // Server.
    //
    iResult = SPP_Open_Remote_Port(g_uiBluetoothStackID, sBD_ADDR,
                                   SPP_PORT_NUMBER, SPP_EventCallback, 0);
    if(iResult > 0)
    {
       //
       // Note the serial port client ID.
       //
       g_uiSerialPortID = (unsigned int)iResult;
       g_sRemoteBD_ADDR = sBD_ADDR;
       bRetVal = true;
    }
    else
    {
       //
       // Inform the user that the call to open the remote serial port server
       // failed.
       //
       Display(("Error calling SPP_Open_Remote_Port(): %d.\r\n", iResult));
       bRetVal = false;
    }

    //
    // Decrement the retry count variable.
    //
    if(g_uiRetryCount)
    {
        g_uiRetryCount--;
    }

    //
    // Return success or failure indication to caller
    //
    return(bRetVal);
}

//*****************************************************************************
//
//  DISC Event Callback Function.
//
// * NOTE * This function MUST NOT Block and wait for Events that
//          can only be satisfied by another Bluetooth Callback.
//
//*****************************************************************************
static void BTPSAPI
DISC_Event_Callback(unsigned int uiBluetoothStackID,
                    DISC_Event_Data_t *psData,
                    unsigned long ulCallbackParameter)
{
    char                  *pcName;
    BD_ADDR_t             *psDeviceBD_ADDR;

    //
    // First, check to see if the required parameters appear to be
    // semi-valid.
    //
    if((uiBluetoothStackID) && (psData))
    {
        //
        // Make sure this is a device discovery callback.
        //
        if(((psData->Event_Data_Type == etDISC_Device_Information_Indication) &&
            (psData->Event_Data.DISC_Device_Information_Indication_Data) &&
            (psData->Event_Data.DISC_Device_Information_Indication_Data->DeviceInfo.NameValid)))
        {
            //
            // Issue a callback for this device that we have discovered.
            //
            IssueCallback(&psData->Event_Data.DISC_Device_Information_Indication_Data->DeviceInfo.BD_ADDR,
                          ceDeviceFound);

            pcName =
                psData->Event_Data.DISC_Device_Information_Indication_Data->DeviceInfo.DeviceName;

            //
            // Try to match to the correct device name.
            //
            if((!g_uiSerialPortID) &&
               (BTPS_StringLength(pcName) >=
                BTPS_StringLength(DEVICE_NAME_TO_CONNECT)))
            {
                //
                // Determine if the device name is the correct to Connect
                // to.
                //
                if(
                  !BTPS_MemCompare(pcName,DEVICE_NAME_TO_CONNECT,
                                   BTPS_StringLength(DEVICE_NAME_TO_CONNECT))
                  )
                {
                    Display(("Attempting to connect to %s.\r\n",pcName));

                    psDeviceBD_ADDR =
                        &(psData->Event_Data.DISC_Device_Information_Indication_Data->DeviceInfo.BD_ADDR);

                    //
                    // Setup the retry count;
                    //
                    g_uiRetryCount = MAX_ATTEMPT_COUNT;

                    //
                    // Attempt to connect to this device
                    //
                    SPPConnect(*psDeviceBD_ADDR);
                }
                else
                {
                    Display(("Device name doesn't match %s.\r\n",
                             DEVICE_NAME_TO_CONNECT));
                }
            }
        }
    }
}

//*****************************************************************************
//
// The following function provides a means of starting a new device discovery
// operation OR stopping a previously started device discovery operation.  The
// first parameter specifies whether to start a discovery operation (true) or
// to stop a previously started device discovery operation (false).  The
// function returns zero on success and a negative number on failure.
//
//*****************************************************************************
int
DeviceDiscovery(tBoolean bStartDiscovery)
{
    int iReturnValue;
    Device_Filter_t sFilter;

    //
    // If parameter is true, then start the device discovery
    //
    if(bStartDiscovery)
    {
        //
        // Set up the device filter for the device
        // discovery procedure. Use the Limited
        // Discovery LAP and also filter on the class
        // of device (EZ430 uses 0x004228).
        //
        ASSIGN_CLASS_OF_DEVICE(sFilter.ClassOfDeviceMask,0x00,0x28,0x14);
        ASSIGN_LAP(sFilter.LAP,0x9E,0x8B,0x00);

        //
        // Start the device discovery
        //
        iReturnValue = DISC_Device_Discovery_Start(g_uiBluetoothStackID,
                                                   &sFilter,
                                                   DISC_Event_Callback,
                                                   0);

        //
        // Check for success in starting device discovery
        //
        if(!iReturnValue)
        {
            Display(("Device Discovery Started.\r\n"));
        }

        //
        // indicate error starting discovery
        //
        else
        {
            Display(("DISC_Device_Discovery_Start Failed %d.\r\n",
                     iReturnValue));
            iReturnValue = BTH_ERROR_REQUEST_FAILURE;
        }
    }

    //
    // Otherwise parameter was false, so stop device discovery
    //
    else
    {
        iReturnValue = DISC_Device_Discovery_Stop(g_uiBluetoothStackID);

        //
        // Success in stopping disovery
        //
        if(!iReturnValue)
        {
            Display(("Device Discovery Stopped.\r\n"));
        }

        //
        // Discovery not stopped successfully
        //
        else
        {
            Display(("DISC_Device_Discovery_Stop Failed %d.\r\n",
                     iReturnValue));
            iReturnValue = BTH_ERROR_REQUEST_FAILURE;
        }
    }

    //
    // Return indication to user
    //
    return(iReturnValue);
}

//*****************************************************************************
//
// The following function provides a means of reading SPP Data that was
// received from a remote device.  The first parameter to this function
// specifies the length of the data buffer and the second parameter is the
// data buffer to store the read data.  This function returns the number of
// bytes read OR a negative error code.
//
//*****************************************************************************
int
ReadData(unsigned int uiDataLength, unsigned char *pucData)
{
    int iReturnValue;
    tBoolean bDone;
    unsigned int uiBytesRead;
    unsigned int uiTotalBytesRead;

    //
    // Verify that the parameters passed in appear valid.
    //
    if(uiDataLength && pucData)
    {
        //
        // Loop while we have buffer space and we have data to read.
        //
        bDone = false;
        uiBytesRead = 0;
        iReturnValue = 0;
        uiTotalBytesRead = 0;
        while(!bDone && uiDataLength)
        {
            iReturnValue = SPP_Data_Read(g_uiBluetoothStackID,
                                         g_uiSerialPortID,
                                         (Word_t)uiDataLength, pucData);
            if(iReturnValue > 0)
            {
                uiBytesRead = (unsigned int)iReturnValue;
                pucData += uiBytesRead;
                uiTotalBytesRead += uiBytesRead;
                uiDataLength -= uiBytesRead;

                //
                // Set the return value to the number of bytes read to handle
                // the case where the DataLength has been decremented to 0.
                //
                iReturnValue  = (int)(uiTotalBytesRead);
            }
            else
            {
                //
                // We have finished reading SPP Data.
                //
                bDone = true;

                if(!iReturnValue)
                {
                    //
                    // A value of 0 indicates that their was no more data to
                    // read.
                    //
                    iReturnValue = (int)(uiTotalBytesRead);
                }
                else
                {
                    //
                    // An error occured.
                    //
                    Display(("SPP_Data_Read returned %d.\r\n", iReturnValue));
                    iReturnValue = BTH_ERROR_REQUEST_FAILURE;
                }
            }
        }
    }
    else
    {
        iReturnValue = BTH_ERROR_INVALID_PARAMETER;
    }

    return(iReturnValue);
}

//*****************************************************************************
//
// The following function is responsible for initializing the Bluetooth Stack
// as well as the SPP Server.  The function takes as its parameters a pointer
// to a callback function that is to called when Bluetooth events occur.  The
// second parameter is an application defined value that will be passed when
// the callback function is called.  The final parameter is MANDATORY and
// specifies (at a minimum) the function callback that will be called by the
// Bluetooth sub-system when the Bluetooth sub-system requires the value of the
// current milli-second tick count.  This parameter can optionally specify the
// function that is called by the Bluetooth module whenever there is a
// character of debug output data to be output.  The function returns zero on
// success and a negative value if an error occurs.
//
//*****************************************************************************
int
InitializeBluetooth(tBluetoothCallbackFn pfnCallbackFunction,
                    void *pCallbackParameter,
                    BTPS_Initialization_t *pBTPSInitialization)
{
    int iRetVal;
    int iCount;
    Byte_t sStatus;
    BD_ADDR_t sBD_ADDR;
    unsigned int uiIndex;
    unsigned int uiNameLength;
    HCI_Version_t sHCIVersion;
    HCI_DriverInformation_t sDriverInformation;
    L2CA_Link_Connect_Params_t sConnectParams;
    Extended_Inquiry_Response_Data_t *psEIRData;

    //
    // Verify that the parameters passed in appear valid.
    //
    if(pfnCallbackFunction && pBTPSInitialization)
    {
        //
        // Initialize the OS Abstraction Layer.
        //
        BTPS_Init((void *)pBTPSInitialization);

        //
        // Configure the UART Parameters and Initialize the Bluetooth Stack.
        //
        HCI_DRIVER_SET_COMM_INFORMATION(&sDriverInformation, 1, 115200,
                                        cpUART);

        //
        // Set the Bluetooth serial port startup delay.  This is the amount of
        // time in ms to wait before starting to use the serial port after
        // initialization.
        //
        sDriverInformation.DriverInformation.COMMDriverInformation.
            InitializationDelay = 150;

        //
        // Initialize the Bluetooth stack.
        //
        iRetVal = BSC_Initialize(&sDriverInformation, 0);
        Display(("Bluetooth Stack ID %d\n", iRetVal));
        if(iRetVal > 0)
        {
            //
            // Save the Bluetooth Stack ID.
            //
            g_uiBluetoothStackID  = (unsigned int)iRetVal;
            g_pfnCallbackFunction = pfnCallbackFunction;
            g_pvCallbackParameter  = pCallbackParameter;

            //
            // Read and Display the Bluetooth Version.
            //
            HCI_Version_Supported(g_uiBluetoothStackID, &sHCIVersion);
            g_sDeviceInfo.ucHCIVersion = (unsigned char)sHCIVersion;

            //
            // Read the Local Bluetooth Device Address.
            //
            GAP_Query_Local_BD_ADDR(g_uiBluetoothStackID, &sBD_ADDR);
            BD_ADDR_To_Array(sBD_ADDR, g_sDeviceInfo.ucBDAddr);

            //
            // Go ahead and allow Master/Slave Role Switch.
            //
            sConnectParams.L2CA_Link_Connect_Request_Config  =
                cqAllowRoleSwitch;
            sConnectParams.L2CA_Link_Connect_Response_Config =
                csMaintainCurrentRole;
            L2CA_Set_Link_Connection_Configuration(g_uiBluetoothStackID,
                                                   &sConnectParams);

            //
            // Update the default link policy if supported.
            //
            if(HCI_Command_Supported(g_uiBluetoothStackID,
              HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
            {
                HCI_Write_Default_Link_Policy_Settings(g_uiBluetoothStackID,
                           HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH,
                            &sStatus);
            }

            //
            // In order to allow Bonding we are making all devices
            // Discoverable, Connectable, and Pairable.  We are also
            // registering an Authentication Callback.  Pairing is not required
            // to use this application however some devices require it.
            //
            GAP_Set_Connectability_Mode(g_uiBluetoothStackID,
                                        cmNonConnectableMode);
            GAP_Set_Discoverability_Mode(g_uiBluetoothStackID,
                                        dmNonDiscoverableMode, 0);
            GAP_Set_Pairability_Mode(g_uiBluetoothStackID,
                                        pmPairableMode);

            //
            // Set the current state in the Device Info structure.
            //
            g_sDeviceInfo.sMode = PAIRABLE_NON_SSP_MODE;

            //
            // Register callback to handle remote authentication requests.
            //
            if(GAP_Register_Remote_Authentication(g_uiBluetoothStackID,
                                                  GAP_EventCallback, 0))
            {
                Display(("Error Registering Remote Authentication\n"));
            }

            //
            // Set our local Name.
            //
            GAP_Query_Local_BD_ADDR(g_uiBluetoothStackID,&sBD_ADDR);


            BTPS_SprintF(g_sDeviceInfo.cDeviceName,"BlueMSP-Demo");
            uiNameLength = BTPS_StringLength(g_sDeviceInfo.cDeviceName);

            if(sBD_ADDR.BD_ADDR1 > 0x0F)
            {
               BTPS_SprintF(&g_sDeviceInfo.cDeviceName[uiNameLength],
                            "%X",sBD_ADDR.BD_ADDR1);
            }
            else
            {
               BTPS_SprintF(&g_sDeviceInfo.cDeviceName[uiNameLength],
                            "0%X",sBD_ADDR.BD_ADDR1);
            }

            if(sBD_ADDR.BD_ADDR0 > 0x0F)
            {
               BTPS_SprintF(&g_sDeviceInfo.cDeviceName[uiNameLength+2],
                            "%X",sBD_ADDR.BD_ADDR0);
            }
            else
            {
               BTPS_SprintF(&g_sDeviceInfo.cDeviceName[uiNameLength+2],
                            "0%X",sBD_ADDR.BD_ADDR0);
            }

            //
            // Make sure that the hex characters in the name are uppercase
            //
            for(uiIndex = uiNameLength; uiIndex < uiNameLength+4; uiIndex++)
            {
               if(((g_sDeviceInfo.cDeviceName[uiIndex]) >= 'a') &&
                  ((g_sDeviceInfo.cDeviceName[uiIndex]) <= 'f'))
               {
                  (g_sDeviceInfo.cDeviceName[uiIndex]) -= ' ';
               }
            }

            //
            // Add the name postfix
            //
            BTPS_SprintF(((char *)&g_sDeviceInfo.cDeviceName[uiNameLength+4]),
                         DEFAULT_DEVICE_NAME_POSTFIX);

            //
            // Set the Bluetooth Device Name
            //
            GAP_Set_Local_Device_Name(g_uiBluetoothStackID,
                                      g_sDeviceInfo.cDeviceName);

            //
            // Allocate space to store temporarily the extended response data
            //
            psEIRData = (Extended_Inquiry_Response_Data_t *)
                BTPS_AllocateMemory(sizeof(Extended_Inquiry_Response_Data_t));
            if(psEIRData)
            {
                //
                // Zero out the allocated space
                //
                BTPS_MemInitialize(psEIRData->Extended_Inquiry_Response_Data,
                                   0, sizeof(Extended_Inquiry_Response_Data_t));

                //
                // Initialize the extended inquiry data with predefined data
                //
                BTPS_MemCopy(psEIRData->Extended_Inquiry_Response_Data,
                             g_ucEIR,
                             sizeof(g_ucEIR));

                //
                // Write the extended inquiry data to the controller.  This
                // will be used to respond to an extended inquiry request
                //
                iRetVal = GAP_Write_Extended_Inquiry_Information(
                                g_uiBluetoothStackID,
                                HCI_EXTENDED_INQUIRY_RESPONSE_FEC_REQUIRED,
                                psEIRData);
                if(iRetVal)
                {
                    Display(("Failed to set Extended Inquiry Data: %d",
                            iRetVal));
                }

                //
                // Free the temporary storage
                //
                BTPS_FreeMemory(psEIRData);
            }

            //
            // Set the class of device
            //
            ASSIGN_CLASS_OF_DEVICE(g_sClassOfDevice, (Byte_t)0x24,
                                                     (Byte_t)0x04,
                                                     (Byte_t)0x04);
            GAP_Set_Class_Of_Device(g_uiBluetoothStackID, g_sClassOfDevice);

            //
            // Read the stored link key information from Flash.
            //
            ReadFlash((NUM_SUPPORTED_LINK_KEYS * sizeof(tLinkKeyInfo)),
                      (unsigned char *)&g_sLinkKeyInfo);

            //
            // Count the number of Link Keys that are stored.
            //
            iCount = 0;
            for(iRetVal = 0; iRetVal < NUM_SUPPORTED_LINK_KEYS; iRetVal++)
            {
                if(!g_sLinkKeyInfo[iRetVal].bEmpty)
                {
                    iCount++;
                }
            }
            Display(("%d Link Keys Stored\r\n", iCount));

            //
            // Initialize DISC Module.
            //
            DISC_Initialize(g_uiBluetoothStackID);

            //
            // Done initializing, set success error code
            //
            iRetVal = 0;
        }

        else
        {
            iRetVal = BTH_ERROR_REQUEST_FAILURE;
        }
    }

    //
    // Error initializing Bluetooth stack, set error return code
    //
    else
    {
        iRetVal = BTH_ERROR_INVALID_PARAMETER;
    }

    //
    // Return to caller
    //
    return(iRetVal);
}
