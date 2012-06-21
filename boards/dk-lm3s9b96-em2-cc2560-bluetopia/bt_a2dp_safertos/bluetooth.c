//*****************************************************************************
//
// bluetooth.c - Bluetooth interface module for Stellaris Bluetooth A2DP Demo
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
#include "driverlib/i2s.h"
#include "driverlib/flash.h"

#include "BTPSKRNL.h"
#include "SS1BTPS.h"
#include "SS1BTGAV.h"
#include "SS1BTA2D.h"
#include "SS1SBC.h"
#include "SS1BTAVC.h"
#include "SS1BTAVR.h"

#include "bluetooth.h"
#include "dac32sound.h"

//*****************************************************************************
//
// A macro for displaying messages to the debug console using the Bluetooth
// stack utility.
//
//*****************************************************************************
#define Display(_x) DBG_MSG(DBG_ZONE_DEVELOPMENT, _x)

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
    BD_ADDR_t sBD_ADDR;
    Link_Key_t sLinkKey;
} tLinkKeyInfo;

#define NUM_SUPPORTED_LINK_KEYS 5

//*****************************************************************************
//
// The following defines the stack size of the SBC decoding thread
//
//*****************************************************************************
#define SBC_DECODE_STACK_SIZE  1024

//*****************************************************************************
//
// The following defines the number of audio samples that are generated for
// each SBC frame.
//
//*****************************************************************************
#define NUM_AUDIO_SAMPLES_PER_SBC_FRAME 128

//*****************************************************************************
//
// The GAVD audio frames are received, stored and decoded.  Each GAVD frame
// will normally contain 8 SBC frames and will be received every 25ms.  Each
// SBC frame will yield 128 left samples and 128 right samples.  We are setup
// to buffer the SBC frames as they arrive.  The size of each SBC frame can
// vary.
//
//*****************************************************************************
#define SBC_BUFFER_SIZE (8 * 1024)

//*****************************************************************************
//
// Each SBC frame will yield 128 left samples and 128 right samples.  Setup the
// buffer to hold 8 decoded SBC frames.
//
//*****************************************************************************
#define AUDIO_BUFFER_SIZE (128 * 8)

//*****************************************************************************
//
// The following defines a constant that is used to control the jitter of
// audio playback.  The decode routine monitors that amount of audio samples in
// the playback buffer.  If the system decodes samples faster than the playback
// then the buffer will fill.  If the playback is faster then the decoder then
// we will run out of samples to play.  Playback buffer space is continuously
// monitored and when we have BufferHighLimit of audio samples in the buffer,
// the SAMPLE_RATE_ADJUSTMENT_VALUE is added to the current sample rate in
// order to playback faster and open up some buffer space.  If the audio
// samples passes BufferLowLimit, then the playback speed is reduced back to
// normal.
//
//*****************************************************************************
#define SAMPLE_RATE_ADJUSTMENT_VALUE 100

//*****************************************************************************
//
// The following enumerates the states that the playback state machine may be
// in.
//
//*****************************************************************************
typedef enum
{
    asIdle,
    asBuffering,
    asDecoding,
    asPlaying
} tAudioState;

//*****************************************************************************
//
// The following structure represents the audio data that has been decoded and
// ready to be played.  Newly decoded data is placed in the buffer to be sent
// to the DAC using the iInIndex.  Data is removed from the buffer and sent to
// the DAC using the iOutIndex.  The buffer has been sized to hold a integral
// number of SBC samples.  In the case that an odd format is sent and N number
// of decoded SBC frames will not completely fill the buffer before requiring
// the buffer to be wrapped, the iEndIndex value indicates the index of the
// last valid audio sample.
//
//*****************************************************************************
typedef struct
{
    tAudioState sAudioState;
    int iSBCIn;
    int iSBCOut;
    int iSBCEnd;
    int iSBCFree;
    int iSBCUsed;
    int iSBCFrameLength;
    unsigned char ucSBCBuffer[SBC_BUFFER_SIZE];
    int iInIndex;
    int iOutIndex;
    int iEndIndex;
    int iNumAudioSamples;
    unsigned short usLeftChannel[AUDIO_BUFFER_SIZE];
    unsigned short usRightChannel[AUDIO_BUFFER_SIZE];
} tAudioData;

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
static A2DP_SBC_Codec_Specific_Information_Element_t g_sSpecInfo;
static GAVD_Service_Capabilities_Info_t g_sCapability[2];
static unsigned long g_ulRecordHandle;
static SDP_UUID_Entry_t g_sUUIDEntry;
static SDP_Data_Element_t g_psProfileInfo[4];
static GAVD_SDP_Service_Record_t g_sGAVDSDPRecordInfo;
static GAVD_Local_End_Point_Info_t g_sEndPointInfo;
static unsigned int g_uiAVCTPProfileID;
static unsigned long g_ulAVCTPRecordHandle;
static unsigned char g_ucTransactionID;
static Class_of_Device_t g_sClassOfDevice;
static Decoder_t g_sDecoderHandle;
static SBC_Decode_Data_t g_sDecodedData;
static SBC_Decode_Configuration_t g_sDecodeConfiguration;
static BD_ADDR_t g_sConnectedAudioDevice;
static tAudioState g_sAudioState;
static tAudioData g_sAudioData;
static int g_iFormatFlag = 0;
static unsigned long g_ulCurrentSampleRate;
static unsigned char g_ucSampleRateAdjustment;
static int g_iBufferHighLimit;
static int g_iBufferLowLimit;
static tBoolean g_bExit;
static Event_t g_sSBCDecodeEvent;

//*****************************************************************************
//
// The following structure contains the information that will be supplied to
// the Bluetooth chip to support Extended Inquiry Result.  At minimum, the
// device name and power level should be advertised.
//
//*****************************************************************************
static const unsigned char g_ucEIR[] =
{
    0x0A,
    HCI_EXTENDED_INQUIRY_RESPONSE_DATA_TYPE_LOCAL_NAME_COMPLETE,
    'A', '2', 'D', 'P', ' ', 'D', 'e', 'm', 'o',
    0x02,
    HCI_EXTENDED_INQUIRY_RESPONSE_DATA_TYPE_TX_POWER_LEVEL,
    0,
    0x0
};

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
// link key in the tLinkKeyInfo structure.  If a link key was not located the
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
           (COMPARE_BD_ADDR(g_sLinkKeyInfo[iIdx].sBD_ADDR, sBD_ADDR)))
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
// place.  The function returns the index of the saved key.
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
        g_sLinkKeyInfo[iIdx].sBD_ADDR = sBD_ADDR;
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
// The following function provides a means to respond to a Pin code request.
// The function takes as its parameters a pointer to the Bluetooth address of
// the device that requested the Pin code and a pointer to the Pin Code data as
// well as the number length of the data pointed to by PinCode.  The
// function return zero on success and a negative number on failure.
// * NOTE * The PinCodeLength must be more than zero and less SIZE_OF_PIN_CODE
//          Bytes.
//
//*****************************************************************************
int
PinCodeResponse(unsigned char *pucBD_ADDR, int iPinCodeLength,
                char *pcPinCode)
{
    int iRetVal;
    BD_ADDR_t sRemoteBD_ADDR;

    //
    // Verify that the parameters passed in appear valid.
    //
    if(pucBD_ADDR && iPinCodeLength && (iPinCodeLength <= SIZE_OF_PIN_CODE) &&
       pcPinCode)
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
                     pcPinCode, iPinCodeLength);

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
// GAP Event Callback Function.  When this function is called it is
// operating in the context of another thread, so thread safety should
// be considered.
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
            // If we are in a Pairable mode, then register the authentication
            // callback.
            //
            if(sPairableMode != pmNonPairableMode)
            {
                //
                // Register the authentication callback.
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
// Interrupt handler for I2S peripheral, which is used for passing data to the
// audio DAC.
//
//*****************************************************************************
void
DACIntHandler(void)
{
    unsigned long ulStatus;

    //
    // Get the interrupt status and clear any pending interrupts.
    //
    ulStatus = I2SIntStatus(I2S0_BASE, true);
    I2SIntClear(I2S0_BASE, ulStatus);

    //
    // Pack the left and right channel into a single 32 bit value and load it
    // into the transmit register.  Continue this until the transmit FIFO is
    // full.
    //
    while(g_sAudioData.iNumAudioSamples &&
          I2STxDataPutNonBlocking(I2S0_BASE,
                ((g_sAudioData.usLeftChannel[g_sAudioData.iOutIndex] << 16) |
                  g_sAudioData.usRightChannel[g_sAudioData.iOutIndex])))
    {
        //
        // Increment the output index and handle any buffer wrap.
        //
        g_sAudioData.iOutIndex++;
        g_sAudioData.iNumAudioSamples--;
        if(g_sAudioData.iEndIndex &&
           (g_sAudioData.iOutIndex >= g_sAudioData.iEndIndex))
        {
            g_sAudioData.iOutIndex = 0;
        }
    }

    //
    // If we run out of audio samples, disable the DAC interrupts and set the
    // state to waiting.  We will start playing again when we buffer up enough
    // audio samples.
    //
    if(!g_sAudioData.iNumAudioSamples)
    {
        I2SIntDisable(I2S0_BASE, I2S_INT_TXREQ);
        g_sAudioState = asDecoding;
    }
}

//*****************************************************************************
//
// The following function issued to decode a block of SBC data.  When a full
// buffer of data is decoded, the data is delivered to the upper layer for
// playback.
//
//*****************************************************************************
static int
Decode(unsigned int uiDataLength, unsigned char *pucDataPtr)
{
    int iRetVal;
    unsigned int uiUnusedDataLength;

    //
    // Note the amount of unprocessed data.
    //
    uiUnusedDataLength = uiDataLength;

    //
    // Verify that the parameters passed in appear valid.
    //
    if(uiDataLength && pucDataPtr)
    {
        //
        // Process while there is unused data.
        //
        while(uiUnusedDataLength)
        {
            //
            // Initialize the decode data structure for this iteration.
            //
            g_sDecodedData.LeftChannelDataLength  = 0;
            g_sDecodedData.RightChannelDataLength = 0;
            g_sDecodedData.LeftChannelDataPtr =
                    &g_sAudioData.usLeftChannel[g_sAudioData.iInIndex];
            g_sDecodedData.RightChannelDataPtr =
                    &g_sAudioData.usRightChannel[g_sAudioData.iInIndex];

            //
            // Lock out I2S interrupts while updating buffer info
            //
            if(g_sAudioState == asPlaying)
            {
                I2SIntDisable(I2S0_BASE, I2S_INT_TXREQ);
            }

            //
            // Calculate the amount of space available for audio samples.
            //
            if(g_sAudioData.iOutIndex > g_sAudioData.iInIndex)
            {
               g_sDecodedData.ChannelDataSize =
                       (unsigned int)(g_sAudioData.iOutIndex -
                                      g_sAudioData.iInIndex);
            }
            else
            {
               g_sDecodedData.ChannelDataSize =
                       (unsigned int)(AUDIO_BUFFER_SIZE -
                                      g_sAudioData.iInIndex);
            }

            //
            // Restore I2S processing
            //
            if(g_sAudioState == asPlaying)
            {
                I2SIntEnable(I2S0_BASE, I2S_INT_TXREQ);
            }

            //
            // Make sure that there is enough room to contain the samples from
            // a single SBC frame.
            //
            if(g_sDecodedData.ChannelDataSize < NUM_AUDIO_SAMPLES_PER_SBC_FRAME)
            {
               break;
            }

            //
            // Pass the SBC data into the decoder.  If a complete frame was
            // decoded then we need to write the decoded data to the output
            // buffer.
            //
            iRetVal = SBC_Decode_Data(
                            g_sDecoderHandle, uiUnusedDataLength,
                            &pucDataPtr[uiDataLength - uiUnusedDataLength],
                            &g_sDecodeConfiguration, &g_sDecodedData,
                            &uiUnusedDataLength);
            if(iRetVal == SBC_PROCESSING_COMPLETE)
            {
                //
                // If format was changed then recompute buffer limits
                //
                if(g_iFormatFlag == 1)
                {
                    g_iFormatFlag = 0;

                    //
                    // Show configuration on debug console
                    //
                    Display(("Frame Length     : %d\r\n",
                             g_sDecodeConfiguration.FrameLength));
                    Display(("Bit Pool         : %d\r\n",
                             g_sDecodeConfiguration.BitPool));
                    Display(("Bit Rate         : %d\r\n",
                             g_sDecodeConfiguration.BitRate));
                    Display(("Buffer Length    : %d\r\n",
                             g_sDecodedData.LeftChannelDataLength));
                    Display(("Frames/GAVD      : %d\r\n",
                            (uiDataLength/g_sDecodeConfiguration.FrameLength)));
                }

                //
                // Adjust the index for the audio samples that were just added.
                //
                g_sAudioData.iInIndex += g_sDecodedData.LeftChannelDataLength;

                //
                // Check to see if we are waiting to build up samples before we
                // start playing the audio.
                //
                if(g_sAudioState == asPlaying)
                {
                    //
                    // Protect data structures from int handler corruption
                    //
                    I2SIntDisable(I2S0_BASE, I2S_INT_TXREQ);
                    //
                    // Update the number of audio samples that are available to
                    // be played.
                    //
                    g_sAudioData.iNumAudioSamples +=
                            g_sDecodedData.LeftChannelDataLength;

                    //
                    // Reenable I2S audio int handler
                    //
                    I2SIntEnable(I2S0_BASE, I2S_INT_TXREQ);
                }
                else
                {
                    //
                    // Check to see if we are waiting to build up samples
                    // before we start playing the audio.
                    //
                    if(g_sAudioData.sAudioState == asDecoding)
                    {
                        g_sAudioData.iNumAudioSamples +=
                                g_sDecodedData.LeftChannelDataLength;

                        //
                        // Check to see if we have enough samples to start.
                        //
                        if(g_sAudioData.iNumAudioSamples >=
                                (AUDIO_BUFFER_SIZE >> 1))
                        {
                            //
                            // It is time to start the audio, so enable the
                            // audio interrupt.
                            //
                            g_sAudioState = asPlaying;
                            I2SIntEnable(I2S0_BASE, I2S_INT_TXREQ);
                        }
                    }
                }

                //
                // Check to see if another packet would overflow the buffer.
                //
                if((g_sAudioData.iInIndex +
                    g_sDecodedData.LeftChannelDataLength) > AUDIO_BUFFER_SIZE)
                {
                    //
                    // Set a marker to the last available audio sample and wrap
                    // the index to the beginning of the buffer.
                    //
                    g_sAudioData.iEndIndex = g_sAudioData.iInIndex;
                    g_sAudioData.iInIndex  = 0;
                }
            }
            else
            {
                Display(("Incomplete %d %d\n", iRetVal, uiUnusedDataLength));
            }
        }
    }

    //
    // Return the number of bytes that were processed.
    //
    return(uiDataLength - uiUnusedDataLength);
}

//*****************************************************************************
//
// The following function is used to process the Set Configuration Request.
//
//*****************************************************************************
static int
GAVD_SetConfiguration(unsigned int uiNumberServiceCapabilities,
                      GAVD_Service_Capabilities_Info_t *psCapab)
{
    int iRetVal;
    unsigned char sValue;
    unsigned char *sSpecInfoPtr;
    GAVD_Media_Codec_Info_Element_Data_t *pCodecInfo;

    //
    // Initialize the return value.
    //
    iRetVal = -A2DP_GAVD_ERROR_CODE_NOT_SUPPORTED_CODEC_TYPE;

    //
    // Check for valid parameters passed to this function
    //
    while(uiNumberServiceCapabilities && psCapab)
    {
        //
        // We need to make sure that the Code is SBC audio and that we support
        // the bit rate.
        //
        if(psCapab->ServiceCategory == scMediaCodec)
        {
            //
            // Check for expected codec info
            //
            pCodecInfo =
                    &psCapab->InfoElement.GAVD_Media_Codec_Info_Element_Data;
            if((pCodecInfo->MediaType == mtAudio) &&
               (pCodecInfo->MediaCodecType == A2DP_MEDIA_CODEC_TYPE_SBC))
            {
                if(pCodecInfo->MediaCodecSpecificInfoLength ==
                        A2DP_SBC_CODEC_SPECIFIC_INFORMATION_ELEMENT_SIZE)
                {
                    //
                    // Get a pointer to the configuration information.
                    //
                    sSpecInfoPtr = pCodecInfo->MediaCodecSpecificInfo;

                    //
                    // Check to see what frequency is being requested.
                    // *MUST* be 44.1 KHz, or 48 KHz.
                    //
                    sValue = A2DP_SBC_READ_SAMPLING_FREQUENCY(sSpecInfoPtr);
                    if((sValue == A2DP_SBC_SAMPLING_FREQUENCY_44_1_KHZ_VALUE) ||
                       (sValue == A2DP_SBC_SAMPLING_FREQUENCY_48_KHZ_VALUE))
                    {
                        //
                        // Display the sampling frequency and return the
                        // proper Value.
                        //
                        Display(("Sampling Frequency: %s\n",
                                (sValue ==
                                    A2DP_SBC_SAMPLING_FREQUENCY_44_1_KHZ_VALUE)
                                    ? "44.1 KHz" : "48KHz"));
                        iRetVal = (sValue ==
                                    A2DP_SBC_SAMPLING_FREQUENCY_44_1_KHZ_VALUE)
                                    ? 44100 : 48000;
                    }
                    else
                    {
                        //
                        // Unsupported sample rate requested so return error
                        //
                        iRetVal = -A2DP_GAVD_ERROR_CODE_NOT_SUPPORTED_SAMPLING_FREQUENCY;
                    }

                    //
                    // Read the requested channel modes.
                    //
                    Display(("Channel Mode      : \f"));
                    sValue = A2DP_SBC_READ_CHANNEL_MODE(sSpecInfoPtr);

                    //
                    // Show the value of the channel mode on the debug console
                    //
                    switch(sValue)
                    {
                        case A2DP_SBC_CHANNEL_MODE_JOINT_STEREO_VALUE:
                        {
                            Display(("Joint Stereo\n"));
                            break;
                        }
                        case A2DP_SBC_CHANNEL_MODE_STEREO_VALUE:
                        {
                            Display(("Stereo\n"));
                            break;
                        }
                        case A2DP_SBC_CHANNEL_MODE_DUAL_CHANNEL_VALUE:
                        {
                            Display(("Dual Channel\n"));
                            break;
                        }
                        default:
                        {
                            //
                            // This case should not happen, so return an
                            // error
                            //
                            Display(("Unsupported\n"));
                            iRetVal = -A2DP_GAVD_ERROR_CODE_NOT_SUPPORTED_CHANNEL_MODE;
                            break;
                        }
                    }

                    //
                    // Read the block length.
                    //
                    Display(("Block Length      : \f"));
                    sValue = A2DP_SBC_READ_BLOCK_LENGTH(sSpecInfoPtr);
                    switch(sValue)
                    {
                        case A2DP_SBC_BLOCK_LENGTH_FOUR_VALUE:
                        {
                            Display(("4\n"));
                            break;
                        }
                        case A2DP_SBC_BLOCK_LENGTH_EIGHT_VALUE:
                        {
                            Display(("8\n"));
                            break;
                        }
                        case A2DP_SBC_BLOCK_LENGTH_TWELVE_VALUE:
                        {
                            Display(("12\n"));
                            break;
                        }
                        case A2DP_SBC_BLOCK_LENGTH_SIXTEEN_VALUE:
                        {
                            Display(("16\n"));
                            break;
                        }
                        default:
                        {
                            Display(("Invalid\n"));
                            iRetVal = -A2DP_GAVD_ERROR_CODE_INVALID_BLOCK_LENGTH;
                            break;
                        }
                    }

                    //
                    // Read the number of SBC subbands.
                    //
                    Display(("Number Sub Bands  : \f"));
                    sValue = A2DP_SBC_READ_SUBBANDS(sSpecInfoPtr);
                    switch(sValue)
                    {
                        case A2DP_SBC_SUBBANDS_FOUR_VALUE:
                        {
                            Display(("4\n"));
                            break;
                        }
                        case A2DP_SBC_SUBBANDS_EIGHT_VALUE:
                        {
                            Display(("8\n"));
                            break;
                        }
                        default:
                        {
                            Display(("Unsupported\n"));
                            iRetVal = -A2DP_GAVD_ERROR_CODE_NOT_SUPPORTED_SUBBANDS;
                            break;
                        }
                    }

                    //
                    // Read the SBC allocation methods.
                    //
                    sValue = A2DP_SBC_READ_ALLOCATION_METHOD(sSpecInfoPtr);
                    Display(("Allocation Method : \f"));
                    switch(sValue)
                    {
                        case A2DP_SBC_ALLOCATION_METHOD_SNR_VALUE:
                        {
                            Display(("SNR\n"));
                            break;
                        }
                        case A2DP_SBC_ALLOCATION_METHOD_LOUDNESS_VALUE:
                        {
                            Display(("Loudness\n"));
                            break;
                        }
                        default:
                        {
                            Display(("Unsupported\n"));
                            iRetVal = -A2DP_GAVD_ERROR_CODE_INVALID_ALLOCATION_METHOD;
                            break;
                        }
                    }

                    //
                    // Assign minimum and maximum supported SBC bit pool
                    // values.
                    //
                    if(A2DP_SBC_READ_MINIMUM_BIT_POOL_VALUE(sSpecInfoPtr) <
                            0x0A)
                    {
                        iRetVal = -A2DP_GAVD_ERROR_CODE_NOT_SUPPORTED_MINIMUM_BIT_POOL_VALUE;
                    }

                    if(A2DP_SBC_READ_MAXIMUM_BIT_POOL_VALUE(sSpecInfoPtr) >
                            0x90)
                    {
                        iRetVal = -A2DP_GAVD_ERROR_CODE_NOT_SUPPORTED_MAXIMUM_BIT_POOL_VALUE;
                    }

                    Display(("Min/Max Bit Pool  : (%d)/(%d)\n",
                            A2DP_SBC_READ_MINIMUM_BIT_POOL_VALUE(sSpecInfoPtr),
                            A2DP_SBC_READ_MAXIMUM_BIT_POOL_VALUE(sSpecInfoPtr)));
                }

                //
                // MediaCodecSpecificInfoLength is not as expected
                //
                else
                {
                    iRetVal = -A2DP_GAVD_ERROR_CODE_INVALID_VERSION;
                }
            }
        }

        //
        // Move to the next capability in the list.
        //
        uiNumberServiceCapabilities--;
        psCapab++;
    }

    //
    // Return value to caller.
    //
    return(iRetVal);
}

//*****************************************************************************
//
// GAVD Event Callback Function.  When this function is called it is
// operating in the context of another thread, so thread safety should
// be considered.
//
//*****************************************************************************
static void BTPSAPI
GAVD_EventCallback(unsigned int uiBluetoothStackID,
                   GAVD_Event_Data_t *psGAVDEventData,
                   unsigned long ulCallbackParameter)
{
    int iRetVal;
    int iDataLength;
    int iNumSBCFrames;
    int iSBCFrameLength;
    Byte_t *psDataPtr;
    tCallbackEventData sCallbackEventData;

    //
    // Verify that the parameters passed in appear valid.
    //
    if(uiBluetoothStackID && psGAVDEventData)
    {
        switch(psGAVDEventData->Event_Data_Type)
        {
            //
            // Set configuration requests has been received
            //
            case etGAVD_Set_Configuration_Indication:
            {
                GAVD_Set_Configuration_Indication_Data_t *pData;
                pData = psGAVDEventData->
                            Event_Data.GAVD_Set_Configuration_Indication_Data;

                //
                // Show a message on the debug console
                //
                Display(("GAVD Set Configuration Indication.\n"));

                //
                // Set format flag
                //
                g_iFormatFlag = 1;

                //
                // Note the device address of the connecting device.
                //
                g_sConnectedAudioDevice = pData->BD_ADDR;

                //
                // Verify that the configuration is supported
                //
                iRetVal = GAVD_SetConfiguration(
                                        pData->NumberServiceCapabilities,
                                        pData->ServiceCapabilities);
                if(iRetVal > 0)
                {
                    //
                    // Set the selected sample rate.
                    //
                    g_ulCurrentSampleRate = iRetVal;
                    g_ucSampleRateAdjustment = 0;
                    SoundSetFormat(g_ulCurrentSampleRate);
                    iRetVal = 0;
                }

                //
                // Adjust negative error code
                //
                else
                {
                    iRetVal = -iRetVal;
                }

                //
                // Send a response to the request.
                //
                GAVD_Set_Configuration_Response(uiBluetoothStackID,
                                                pData->LSEID, scNone, iRetVal);
                break;
            }

            //
            // Server endpoint has been opened
            //
            case etGAVD_Open_End_Point_Indication:
            {
                //
                // Show a message on the debug console
                //
                Display(("GAVD Open End Point Indication\n"));

                //
                // Verify that a callback function has been registered
                //
                if(g_pfnCallbackFunction)
                {
                    //
                    // Set the event type.
                    //
                    sCallbackEventData.sEvent = ceAudioEndpointOpen;

                    //
                    // Attempt to call the user with the event information.
                    //
                    g_pfnCallbackFunction(&sCallbackEventData,
                                          g_pvCallbackParameter);
                }
                break;
            }

            //
            // End point has been closed
            //
            case etGAVD_Close_End_Point_Indication:
            {
                //
                // Show a message on the debug console
                //
                Display(("GAVD Close End Point Indication\n"));

                //
                // Verify that a callback function has been registered.
                //
                if(g_pfnCallbackFunction)
                {
                    //
                    // Set the event type.
                    //
                    sCallbackEventData.sEvent = ceAudioEndpointClose;

                    //
                    // Attempt to call the user with the event information.
                    //
                    g_pfnCallbackFunction(&sCallbackEventData,
                                          g_pvCallbackParameter);
                }

                //
                // Stop sending any audio data and set then state to idle.
                //
                g_sAudioState = asIdle;
                I2SIntDisable(I2S0_BASE, I2S_INT_TXREQ);

                //
                // Reset the event to denote that there is no data to
                // process.
                //
                if(g_sSBCDecodeEvent)
                {
                   BTPS_ResetEvent(g_sSBCDecodeEvent);
                }
                break;
            }

            //
            // Start indication was received
            //
            case etGAVD_Start_Indication:
            {
                //
                // Show a message on the debug console
                //
                Display(("GAVD Start Indication\n"));

                //
                // Verify that a callback function has been registered.
                //
                if(g_pfnCallbackFunction)
                {
                    //
                    // Set the event type.
                    //
                    sCallbackEventData.sEvent = ceAudioStreamStart;

                    //
                    // Attempt to call the user with the event information.
                    //
                    g_pfnCallbackFunction(&sCallbackEventData,
                                          g_pvCallbackParameter);
                }

                //
                // Reset all of the audio data information.
                //
                BTPS_MemInitialize(&g_sAudioData, 0, sizeof(g_sAudioData));

                //
                // Reset any sample rate adjustment that might be active.
                //
                g_ucSampleRateAdjustment = 0;
                SoundSetFormat(g_ulCurrentSampleRate);

                //
                // Respond to the start and always indicate success.
                //
                GAVD_Start_Stream_Response(
                    uiBluetoothStackID,
                    psGAVDEventData->
                        Event_Data.GAVD_Start_Indication_Data->LSEID, 0);
                break;
            }

            //
            // Suspend indication was received
            //
            case etGAVD_Suspend_Indication:
            {
                //
                // Show a message on the debug console
                //
                Display(("GAVD Suspend Indication\n"));

                //
                // Verify that a callback function has been registered.
                //
                if(g_pfnCallbackFunction)
                {
                    //
                    // Set the event type.
                    //
                    sCallbackEventData.sEvent = ceAudioStreamSuspend;

                    //
                    // Attempt to call the user with the event information.
                    //
                    g_pfnCallbackFunction(&sCallbackEventData,
                                          g_pvCallbackParameter);
                }

                //
                // Suspend Indication was received, so we need to stop playing
                // any audio.  Place the state to Idle and wait for the audio
                // to be resumed.
                //
                g_sAudioState = asIdle;
                I2SIntDisable(I2S0_BASE, I2S_INT_TXREQ);

                //
                // Reset the event to denote that there is no data to
                // process.
                //
                if(g_sSBCDecodeEvent)
                {
                    BTPS_ResetEvent(g_sSBCDecodeEvent);
                }

                //
                // Respond to the suspend and always indicate success.
                //
                GAVD_Suspend_Stream_Response(
                    uiBluetoothStackID,
                    psGAVDEventData->
                        Event_Data.GAVD_Suspend_Indication_Data->LSEID, 0);
                break;
            }

            //
            // Abort indication was received
            //
            case etGAVD_Abort_Indication:
            {
                //
                // Show a message on the debug console
                //
                Display(("GAVD Abort Indication: "));
                Display(("LSEID: %dn",
                         psGAVDEventData->
                             Event_Data.GAVD_Abort_Indication_Data->LSEID));

                //
                // Abort Indication was received, so we need to stop playing
                // any audio.  Place the state to Idle and wait for the audio
                // to be resumed.
                //
                g_sAudioState = asIdle;
                I2SIntDisable(I2S0_BASE, I2S_INT_TXREQ);

                //
                // Reset the event to denote that there is no data to
                // process.
                //
                if(g_sSBCDecodeEvent)
                {
                    BTPS_ResetEvent(g_sSBCDecodeEvent);
                }
                break;
            }

            //
            // Data was received
            //
            case etGAVD_Data_Indication:
            {
                iDataLength =
                    psGAVDEventData->
                        Event_Data.GAVD_Data_Indication_Data->DataLength - 1;
                psDataPtr =
                    (Byte_t *)&(psGAVDEventData->
                        Event_Data.GAVD_Data_Indication_Data->DataBuffer[1]);

                iNumSBCFrames =
                    (psGAVDEventData->
                        Event_Data.GAVD_Data_Indication_Data->DataBuffer[0] &
                        A2DP_SBC_HEADER_NUMBER_FRAMES_MASK);

                //
                // Determine the Number and Size of each SBC frames that are in
                // the GAVD packet.
                //
                iSBCFrameLength = (iDataLength / iNumSBCFrames);
                if(!g_sAudioData.iSBCEnd)
                {
                    //
                    // This is the first SBC frame received since the stream was
                    // started.  Initialize information about the data being
                    // received.
                    //
                    g_sAudioData.iSBCEnd = ((SBC_BUFFER_SIZE / iSBCFrameLength)
                                            * iSBCFrameLength);
                    g_sAudioData.iSBCFree = g_sAudioData.iSBCEnd;
                    g_sAudioData.iSBCUsed = 0;
                    g_sAudioData.iSBCFrameLength = iSBCFrameLength;
                    g_sAudioData.sAudioState = asBuffering;
                    g_iBufferLowLimit = g_sAudioData.iSBCEnd >> 1;
                    g_iBufferHighLimit = g_sAudioData.iSBCEnd -
                                        (g_sAudioData.iSBCEnd >> 2);
                    Display(("Buffer High Limit: %d\r\n", g_iBufferHighLimit));
                    Display(("Buffer Low Limit : %d\r\n", g_iBufferLowLimit));

                    //
                    // Go ahead and inform the decoder that there is going to
                    // be data to process.
                    //
                    if(g_sSBCDecodeEvent)
                    {
                        BTPS_SetEvent(g_sSBCDecodeEvent);
                    }
                }

                //
                // Move the SBC frames to the SBC buffer as long as there is
                // room for the data.
                // * NOTE * There are two choices here.  We can either:
                //             - drop the frames that we don't have
                //               room for (i.e. these newest frames)
                //             - drop the oldest frames to make room.
                //          Currently we are going to drop the newest
                //          (incoming) frames.  Either way frames will
                //          have to be dropped.
                //
                while(iNumSBCFrames-- && g_sAudioData.iSBCFree)
                {
                    //
                    // Copy the SBC frames to the SBC buffer.
                    //
                    BTPS_MemCopy(&(g_sAudioData.ucSBCBuffer[g_sAudioData.iSBCIn]),
                                 psDataPtr, iSBCFrameLength);

                    psDataPtr += iSBCFrameLength;
                    g_sAudioData.iSBCFree -= iSBCFrameLength;
                    g_sAudioData.iSBCIn += iSBCFrameLength;
                    g_sAudioData.iSBCUsed += iSBCFrameLength;
                    if(g_sAudioData.iSBCIn >= g_sAudioData.iSBCEnd)
                    {
                        //
                        // Wrap the buffer pointer.
                        //
                        g_sAudioData.iSBCIn = 0;
                    }
                }

                //
                // Check to state of the SBC buffer to see if we need to adjust
                // the playback speed.
                //
                if(!g_ucSampleRateAdjustment &&
                   (g_sAudioData.iSBCUsed >= g_iBufferHighLimit))
                {
                    //
                    // We have too many samples so we will increase the
                    // playback speed to free up storage.
                    //
                    g_ucSampleRateAdjustment = SAMPLE_RATE_ADJUSTMENT_VALUE;
                    SoundSetFormat(g_ulCurrentSampleRate +
                                   g_ucSampleRateAdjustment);
                    Display(("Up %d %d\r\n",
                            (unsigned long)g_ulCurrentSampleRate,
                            g_sAudioData.iSBCUsed));
                }

                if(g_ucSampleRateAdjustment &&
                   (g_sAudioData.iSBCUsed <= g_iBufferLowLimit))
                {
                    //
                    // We have too many samples so we will increase the
                    // playback speed to reduce the storage.
                    //
                    g_ucSampleRateAdjustment = 0;
                    SoundSetFormat(g_ulCurrentSampleRate);
                    Display(("Down %d %d\r\n",
                            (unsigned long)g_ulCurrentSampleRate,
                            g_sAudioData.iSBCUsed));
                }
                break;
            }

            //
            // Reconfigure indication
            //
            case etGAVD_Reconfigure_Indication:
            {
                GAVD_Reconfigure_Indication_Data_t *pReCfg;
                GAVD_Set_Configuration_Indication_Data_t *pSetCfg;
                pReCfg = psGAVDEventData->
                            Event_Data.GAVD_Reconfigure_Indication_Data;
                pSetCfg = psGAVDEventData->
                            Event_Data.GAVD_Set_Configuration_Indication_Data;

                //
                // Show a message on the debug console
                //
                Display(("GAVD Reconfigure Indication: "));
                Display(("LSEID: %d.\n", pReCfg->LSEID));

                //
                // Set the format flag
                //
                g_iFormatFlag = 1;

                //
                // Verify that the configuration is supported.
                //
                iRetVal = GAVD_SetConfiguration(
                                        pSetCfg->NumberServiceCapabilities,
                                        pSetCfg->ServiceCapabilities);

                //
                // Check return code from above and adjust if negative (error)
                //
                if(iRetVal > 0)
                {
                    iRetVal = 0;
                }
                else
                {
                    iRetVal = -iRetVal;
                }

                //
                // Respond to the reconfiguration request
                //
                GAVD_Reconfigure_Response(uiBluetoothStackID, pReCfg->LSEID,
                                          scNone, iRetVal);
                break;
            }

            //
            // Unknown event
            //
            default:
            {
                //
                // Show a message on the debug console
                //
                Display(("Unknown GAVD Event.\n"));
                break;
            }
        }
    }
}

//*****************************************************************************
//
// The following function is used to register a GAVD endpoint that can be
// connected to by remote devices.
//
//*****************************************************************************
static int
GAVDRegisterEndPoint(void)
{
    int iRetVal;
    GAVD_Media_Codec_Info_Element_Data_t *pMediaCodecInfo;

    //
    // First check to see if a valid Bluetooth stack ID exists.
    //
    if(g_uiBluetoothStackID)
    {
        //
        // Create the capabilities for the end point.
        // * NOTE * The capabilities that we are creating are for the A2DP
        //          profile.  We are hardcoding the media codec information for
        //          demonstration purposes.
        //
        g_sCapability[0].ServiceCategory = scMediaTransport;
        g_sCapability[1].ServiceCategory = scMediaCodec;
        pMediaCodecInfo =
            &g_sCapability[1].InfoElement.GAVD_Media_Codec_Info_Element_Data;
        pMediaCodecInfo->MediaType = mtAudio;
        pMediaCodecInfo->MediaCodecType = A2DP_MEDIA_CODEC_TYPE_SBC;
        pMediaCodecInfo->MediaCodecSpecificInfoLength =
                        A2DP_SBC_CODEC_SPECIFIC_INFORMATION_ELEMENT_SIZE;
        pMediaCodecInfo->MediaCodecSpecificInfo = (Byte_t *)&g_sSpecInfo;

        //
        // Initialize the SBC codec specific information.
        // * NOTE * This could also be hardcoded as a simple array of bytes,
        //          but for completeness, it will be shown here.
        // * NOTE * See A2DP specification for the specific meaning and what
        //          minimum feature set needs to be supported.
        // * NOTE * See A2DPAPI.h for the definition of these values.
        //
        BTPS_MemInitialize(&g_sSpecInfo, 0,
                           A2DP_SBC_CODEC_SPECIFIC_INFORMATION_ELEMENT_SIZE);

        //
        // Assign supported sampling frequencies - SNK *MUST* support both 44.1
        // KHz, and 48 KHz.
        //
        A2DP_SBC_ASSIGN_SAMPLING_FREQUENCY(&g_sSpecInfo,
                              (A2DP_SBC_SAMPLING_FREQUENCY_44_1_KHZ_VALUE |
                               A2DP_SBC_SAMPLING_FREQUENCY_48_KHZ_VALUE));

        //
        // Assign supported channel modes.
        //
        A2DP_SBC_ASSIGN_CHANNEL_MODE(&g_sSpecInfo,
                              (A2DP_SBC_CHANNEL_MODE_JOINT_STEREO_VALUE |
                               A2DP_SBC_CHANNEL_MODE_STEREO_VALUE       |
                               A2DP_SBC_CHANNEL_MODE_DUAL_CHANNEL_VALUE));

        //
        // Assign supported SBC block lengths.
        //
        A2DP_SBC_ASSIGN_BLOCK_LENGTH(&g_sSpecInfo,
                              (A2DP_SBC_BLOCK_LENGTH_FOUR_VALUE   |
                               A2DP_SBC_BLOCK_LENGTH_EIGHT_VALUE  |
                               A2DP_SBC_BLOCK_LENGTH_TWELVE_VALUE |
                               A2DP_SBC_BLOCK_LENGTH_SIXTEEN_VALUE));

        //
        // Assign supported SBC subbands.
        //
        A2DP_SBC_ASSIGN_SUBBANDS(&g_sSpecInfo,
                              (A2DP_SBC_SUBBANDS_FOUR_VALUE |
                               A2DP_SBC_SUBBANDS_EIGHT_VALUE));

        //
        // Assign supported SBC allocation methods.
        //
        A2DP_SBC_ASSIGN_ALLOCATION_METHOD(&g_sSpecInfo,
                              (A2DP_SBC_ALLOCATION_METHOD_SNR_VALUE |
                               A2DP_SBC_ALLOCATION_METHOD_LOUDNESS_VALUE));

        //
        // Assign minimum and maximum supported SBC bit pool values.
        //
        A2DP_SBC_ASSIGN_MINIMUM_BIT_POOL_VALUE(&g_sSpecInfo, 0x0A);
        A2DP_SBC_ASSIGN_MAXIMUM_BIT_POOL_VALUE(&g_sSpecInfo, 0x35);

        //
        // Create the end point info structure.
        //
        g_sEndPointInfo.NumberCapabilities = 2;
        g_sEndPointInfo.CapabilitiesInfo = g_sCapability;
        g_sEndPointInfo.MediaType = mtAudio;
        g_sEndPointInfo.TSEP = tspSNK;
        g_sEndPointInfo.MediaInMTU = 1000;      // value will fit in a 3-DH5
        g_sEndPointInfo.ReportingInMTU = 1000;  // value will fit in a 3-DH5
        g_sEndPointInfo.RecoveryInMTU = 1000;   // value will fit in a 3-DH5

        //
        // Try to register the end point.
        //
        iRetVal = GAVD_Register_End_Point(g_uiBluetoothStackID,
                                          &g_sEndPointInfo,
                                          GAVD_EventCallback, 0);

        //
        // Now check to see if the function call was successfully made.
        //
        if(iRetVal > 0)
        {
            //
            // Register a SDP service record.
            // * NOTE * We will simply register an Advanced Audio Distribution
            //          Profile (A2DP) SDP record for demonstration purposes.
            //          First format the service class as an audio sink.
            //
            g_sGAVDSDPRecordInfo.NumberServiceClassUUID = 1;
            g_sGAVDSDPRecordInfo.SDPUUIDEntries = &g_sUUIDEntry;

            g_sUUIDEntry.SDP_Data_Element_Type = deUUID_16;
            ASSIGN_UUID_16(g_sUUIDEntry.UUID_Value.UUID_16, 0x11, 0x0B);

            //
            // Next flag that there are no additional protocols that need to be
            // added.
            //
            g_sGAVDSDPRecordInfo.ProtocolList = NULL;

            //
            // Next add the profile list information to the record.
            //
            g_sGAVDSDPRecordInfo.ProfileList = g_psProfileInfo;

            //
            // Now we need to actually build the Bluetooth profile descriptor
            // list which is data element sequence of data element sequences.
            //
            g_psProfileInfo[0].SDP_Data_Element_Type = deSequence;
            g_psProfileInfo[0].SDP_Data_Element_Length = 1;
            g_psProfileInfo[0].SDP_Data_Element.SDP_Data_Element_Sequence =
                    &(g_psProfileInfo[1]);

            g_psProfileInfo[1].SDP_Data_Element_Type = deSequence;
            g_psProfileInfo[1].SDP_Data_Element_Length = 2;
            g_psProfileInfo[1].SDP_Data_Element.SDP_Data_Element_Sequence =
                    &(g_psProfileInfo[2]);

            g_psProfileInfo[2].SDP_Data_Element_Type = deUUID_16;
            g_psProfileInfo[2].SDP_Data_Element_Length = UUID_16_SIZE;
            ASSIGN_UUID_16(g_psProfileInfo[2].SDP_Data_Element.UUID_16, 0x11,
                           0x0D);

            g_psProfileInfo[3].SDP_Data_Element_Type = deUnsignedInteger2Bytes;
            g_psProfileInfo[3].SDP_Data_Element_Length = WORD_SIZE;
            g_psProfileInfo[3].SDP_Data_Element.UnsignedInteger2Bytes = 0x0100;

            //
            // Now attempt to add the GAVD SDP record to the SDP database.
            //
            if(!GAVD_Register_SDP_Record(g_uiBluetoothStackID,
                                         &g_sGAVDSDPRecordInfo,
                                         "GAVD Audio Sink Sample",
                                         &g_ulRecordHandle))
            {
                //
                // The end point was successfully registered.  The return value
                // of the call is the LSEID and is required for future GAVD
                // calls to this end point.
                //
                Display(("GAVD_Register_End_Point: Function Successful "
                         "(LSEID = 0x%X).\n", iRetVal));
            }
            else
            {
                //
                // Error adding the SDP record to the database, so go ahead
                // unregister the end point and notify the user.
                //
                GAVD_Un_Register_End_Point(g_uiBluetoothStackID, iRetVal);

                Display(("Unable to register SDP Record.  "
                         "Endpoint not registered.\n"));

                //
                // Flag an error to the caller.
                //
                iRetVal = -1;
            }
        }
        else
        {
            //
            // There was an error attempting to register the end point.
            //
            Display(("GAVD_Register_End_Point: Function Failure: %d.\n",
                    iRetVal));
        }
    }
    else
    {
        //
        // No valid Bluetooth stack ID exists.
        //
        Display(("Stack ID Invalid.\n"));
        iRetVal = -1;
    }

    return(iRetVal);
}

//*****************************************************************************
//
// AVCTP Event Callback Function. When this function is called it is
// operating in the context of another thread, so thread safety should be
// considered.
//
//*****************************************************************************
static void BTPSAPI
AVCTP_EventCallback(unsigned int uiBluetoothStackID,
                   AVCTP_Event_Data_t *psAVCTPEventData,
                   unsigned long ulCallbackParameter)
{
    tCallbackEventData sCallbackEventData;

    //
    // Verify that the parameters passed in appear valid.
    //
    if(uiBluetoothStackID && psAVCTPEventData)
    {
        switch(psAVCTPEventData->Event_Data_Type)
        {
            case etAVCTP_Connect_Indication:
            {
                //
                // Connect indication was received, so inform the user.
                // Verify that a callback function has been registered.
                //
                if(g_pfnCallbackFunction)
                {
                    //
                    // Set the event type.
                    //
                    sCallbackEventData.sEvent = ceRemoteControlConnectionOpen;

                    //
                    // Attempt to call the user with the event information.
                    //
                    g_pfnCallbackFunction(&sCallbackEventData,
                                          g_pvCallbackParameter);
                }
                break;
            }
            case etAVCTP_Disconnect_Indication:
            {
                //
                // Disconnect indication was received, so inform the user.
                // Verify that a callback function has been registered.
                //
                if(g_pfnCallbackFunction)
                {
                    //
                    // Set the event type.
                    //
                    sCallbackEventData.sEvent = ceRemoteControlConnectionClose;

                    //
                    // Attempt to call the user with the event information.
                    //
                    g_pfnCallbackFunction(&sCallbackEventData,
                                          g_pvCallbackParameter);
                }
                break;
            }
            default:
            {
                //
                // We are not interested in these events.
                //
                break;
            }
        }
    }
}

//*****************************************************************************
//
// The following function is used to register an AVRCP remote control
// controller with AVCTP that can be connected to by remote devices.
//
//*****************************************************************************
static int
RegisterAVRCPController(void)
{
   int iRetVal;
   UUID_16_t ProfileUUID;

    //
    // First check to see if a valid Bluetooth stack ID exists.
    //
    if(g_uiBluetoothStackID)
    {
        //
        // Attempt to register the Audio/Video Remote Control profile (AVRCP)
        // with AVCTP.
        //
        SDP_ASSIGN_AUDIO_VIDEO_REMOTE_CONTROL_PROFILE_UUID_16(ProfileUUID);

        iRetVal = AVCTP_Register_Profile(g_uiBluetoothStackID,
                                         ProfileUUID,
                                         AVCTP_EventCallback,
                                         0);
        if(iRetVal > 0)
        {
            g_uiAVCTPProfileID = (unsigned int)iRetVal;

            //
            // Finally, attempt to register an AVRCP controller service
            // record.
            //
            iRetVal = AVRCP_Register_SDP_Record_Version(g_uiBluetoothStackID,
                        TRUE, "AVRCP Controller", "TI/Stonestreet One",
                        (Word_t)SDP_AVRCP_SUPPORTED_FEATURES_CONTROLLER_CATEGORY_1,
                        apvVersion1_0, &g_ulAVCTPRecordHandle);
            if(!iRetVal)
            {
               Display(("AVRCP Controller Registered.\n"));

               //
               // Initialize the transaction ID.
               //
               g_ucTransactionID = 0;
            }
            else
            {
                //
                // Unable to register profile with AVCTP.
                //
                Display(("Unable to register AVRCP profile SDP Record.\n"));

                //
                // Un-Register the profile that was registered with AVCTP.
                //
                AVCTP_UnRegister_Profile(g_uiBluetoothStackID,
                                         g_uiAVCTPProfileID);
                iRetVal = -3;
            }
        }
        else
        {
            //
            // Unable to register profile with AVCTP.
            //
            Display(("Unable to register AVRCP profile with AVCTP.\n"));
            iRetVal = -2;
        }
    }
    else
    {
        //
        // No valid Bluetooth stack ID exists.
        //
        Display(("Stack ID Invalid.\n"));
        iRetVal = -1;
    }

    return(iRetVal);
}

//*****************************************************************************
//
// The following function provides a means to send the specified remote
// control command to the remote device.  This function accepts the
// remote control command to send.  This function will return zero on
// success and a negative number on failure.
//
//*****************************************************************************
int
SendRemoteControlCommand(tRemoteControlCommand sCommand)
{
    int iRetVal;
    Byte_t Buffer[16];
    AVRCP_Pass_Through_Command_Data_t PassThroughCommandData;

    //
    // Verify that the module is initialized correctly.
    //
    if(g_uiBluetoothStackID && g_uiAVCTPProfileID)
    {
        iRetVal = 0;

        //
        // Initialize common members of the pass through command.
        // * NOTE * State flag of FALSE specifies button down.
        //
        PassThroughCommandData.CommandType = AVRCP_CTYPE_CONTROL;
        PassThroughCommandData.SubunitType = AVRCP_SUBUNIT_TYPE_PANEL;
        PassThroughCommandData.SubunitID = AVRCP_SUBUNIT_ID_INSTANCE_0;
        PassThroughCommandData.OperationID = (Byte_t)0;
        PassThroughCommandData.StateFlag = (Boolean_t)FALSE;
        PassThroughCommandData.OperationDataLength = 0;
        PassThroughCommandData.OperationData = NULL;

        //
        // Determine the command and go ahead and build it.
        //
        switch(sCommand)
        {
            case rcPlay:
            {
                PassThroughCommandData.OperationID =
                                   (Byte_t)AVRCP_PASS_THROUGH_ID_PLAY;
                break;
            }
            case rcPause:
            {
                PassThroughCommandData.OperationID =
                                   (Byte_t)AVRCP_PASS_THROUGH_ID_PAUSE;
                break;
            }
            case rcNext:
            {
                PassThroughCommandData.OperationID =
                                   (Byte_t)AVRCP_PASS_THROUGH_ID_FORWARD;
                break;
            }
            case rcBack:
            {
                PassThroughCommandData.OperationID =
                                   (Byte_t)AVRCP_PASS_THROUGH_ID_BACKWARD;
                break;
            }
            case rcVolumeUp:
            {
                PassThroughCommandData.OperationID =
                                   (Byte_t)AVRCP_PASS_THROUGH_ID_VOLUME_UP;
                break;
            }
            case rcVolumeDown:
            {
                PassThroughCommandData.OperationID =
                                   (Byte_t)AVRCP_PASS_THROUGH_ID_VOLUME_DOWN;
                break;
            }
            default:
            {
                iRetVal = -2;
                break;
            }
        }

        //
        // Proceed only if the AVRCP command is supported.
        //
        if(!iRetVal)
        {
            //
            // Everything parsed correct, go ahead and try to build the
            // command.
            //
            iRetVal = AVRCP_Format_Pass_Through_Command(g_uiBluetoothStackID,
                                                        &PassThroughCommandData,
                                                        sizeof(Buffer),
                                                        Buffer);
            if(iRetVal > 0)
            {
                //
                // Command built, attempt to send it.
                //
                iRetVal = AVCTP_Send_Message(g_uiBluetoothStackID,
                                             g_uiAVCTPProfileID,
                                             g_sConnectedAudioDevice,
                                             (Byte_t)((g_ucTransactionID++) &
                                                      AVCTP_TRANSACTION_ID_MASK),
                                             FALSE,
                                             iRetVal,
                                             Buffer);

                //
                // If the command was sent, send the button release.
                //
                if(!iRetVal)
                {
                    PassThroughCommandData.StateFlag = (Boolean_t)TRUE;

                    iRetVal = AVRCP_Format_Pass_Through_Command(
                                g_uiBluetoothStackID, &PassThroughCommandData,
                                sizeof(Buffer), Buffer);
                    if(iRetVal > 0)
                    {
                        //
                        // Command built, attempt to send it.
                        //
                        iRetVal = AVCTP_Send_Message(
                                        g_uiBluetoothStackID,
                                        g_uiAVCTPProfileID,
                                        g_sConnectedAudioDevice,
                                        (Byte_t)((g_ucTransactionID++) &
                                        AVCTP_TRANSACTION_ID_MASK),
                                        FALSE, iRetVal, Buffer);
                    }
                }
            }
        }
    }
    else
    {
        iRetVal = -1;
    }

    return(iRetVal);
}

//*****************************************************************************
//
// The following function represents the SBC callback thread.
//
//*****************************************************************************
static void *BTPSAPI
SBCDecodeThread(void *UserData)
{
    int iBytesUsed;

    //
    // Verify that the parameters we require are valid.
    //
    if(!g_bExit && g_sSBCDecodeEvent)
    {
        while(1)
        {
            //
            // Wait until we are informed that there is something to do.
            //
            if(BTPS_WaitEvent(g_sSBCDecodeEvent, BTPS_INFINITE_WAIT))
            {
                //
                // Event signalled, make sure we have not been instructed
                // to exit.
                //
                if(!g_bExit)
                {
                    if(!BSC_LockBluetoothStack(g_uiBluetoothStackID))
                    {
                        //
                        // Verify that we have audio data to process.
                        //
                        if(g_sAudioData.sAudioState != asIdle)
                        {
                            //
                            // Check to see if we have started to receive
                            // SBC packets and are in the process of
                            // bufferiing the SBC packets.
                            //
                            if(g_sAudioData.sAudioState == asBuffering)
                            {
                                //
                                // Buffer SBC packets until the SBC packet
                                // buffer is 1/4 full (small jitter).
                                //
                                if(g_sAudioData.iSBCUsed >=
                                   (g_sAudioData.iSBCEnd >> 2))
                                {
                                    g_sAudioData.sAudioState = asDecoding;
                                }
                            }

                            //
                            //  Check to see if we are decoding or playing
                            //
                            if(g_sAudioData.sAudioState != asBuffering)
                            {
                                //
                                // While there are SBC packets in the buffer
                                // then we will attempt to decode the packet.
                                //
                                while(g_sAudioData.iSBCUsed)
                                {
                                    //
                                    // If there is not enough space left to
                                    // decode an SBC frame then we need to
                                    // break out of the loop.
                                    //
                                    if((AUDIO_BUFFER_SIZE -
                                        g_sAudioData.iNumAudioSamples) <
                                       NUM_AUDIO_SAMPLES_PER_SBC_FRAME)
                                    {
                                        break;
                                    }

                                    //
                                    // Release the lock on the Bluetooth
                                    // stack so that we can continue to
                                    // process Bluetooth events while we
                                    // are decoding.
                                    //
                                    BSC_UnLockBluetoothStack(g_uiBluetoothStackID);

                                    //
                                    // Attempt to decode an SBC frame.
                                    //
                                    iBytesUsed =
                                        Decode(g_sAudioData.iSBCFrameLength,
                                               &(g_sAudioData.ucSBCBuffer[g_sAudioData.iSBCOut]));

                                    //
                                    // Re-acquire the lock because we
                                    // need to operate on some variables
                                    // that need to be protected.
                                    //
                                    BSC_LockBluetoothStack(g_uiBluetoothStackID);

                                    if(iBytesUsed > 0)
                                    {
                                        //
                                        // Adjust the index and free counts
                                        // based on the number of bytes
                                        // processed.
                                        //
                                        g_sAudioData.iSBCOut += iBytesUsed;
                                        g_sAudioData.iSBCFree += iBytesUsed;
                                        g_sAudioData.iSBCUsed -= iBytesUsed;

                                        //
                                        // Check to see if the buffer needs
                                        // to be wrapped.
                                        //
                                        if(g_sAudioData.iSBCOut >=
                                           g_sAudioData.iSBCEnd)
                                        {
                                           g_sAudioData.iSBCOut = 0;
                                        }
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                            }
                        }

                        //
                        // If we got here then it means we still hold
                        // the Lock, so we need to go ahead and release
                        // the lock.
                        //
                        BSC_UnLockBluetoothStack(g_uiBluetoothStackID);
                    }
                    else
                    {
                       break;
                    }

                    //
                    // Delay to allow time for either more data to
                    // arrive or the audio buffer to play out.
                    //
                    BTPS_Delay(1);
                }
                else
                {
                   break;
                }
            }
            else
            {
                break;
            }
        }
    }

    return(NULL);
}

//*****************************************************************************
//
// The following function is responsible for initializing the Bluetooth stack
// as well as the A2DP server.  The function takes as its parameters a pointer
// to a callback function that is to called when Bluetooth events occur.  The
// second parameter is an application defined value that will be passed when
// the callback function is called.  The final parameter is MANDATORY and
// specifies (at a minimum) the function callback that will be called by the
// Bluetooth sub-system when the Bluetooth sub-system requires the optional
// function that is called by the Bluetooth module whenever there is a
// character of debug output data to be output.  The function returns zero on
// success and a negative value if an error occurs.
//
//*****************************************************************************
int
InitializeBluetooth(tBluetoothCallbackFn pfnCallbackFunction,
                    void *pvCallbackParameter,
                    BTPS_Initialization_t *pBTPSInitialization)
{
    int iRetVal;
    int iCount;
    Byte_t sStatus;
    BD_ADDR_t sBD_ADDR;
    HCI_Version_t sHCIVersion;
    ThreadHandle_t sSBCDecodeThread;
    HCI_DriverInformation_t sDriverInformation;
    L2CA_Link_Connect_Params_t sConnectParams;
    Extended_Inquiry_Response_Data_t *psEIRData;

    //
    // Verify that the parameters passed in appear valid.
    //
    if(pfnCallbackFunction && pBTPSInitialization)
    {
        //
        // Initialize the OS abstraction layer.
        //
        BTPS_Init((void *)pBTPSInitialization);

        //
        // Configure the UART parameters and initialize the Bluetooth stack.
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
            // Save the Bluetooth stack ID.
            //
            g_uiBluetoothStackID = (unsigned int)iRetVal;
            g_pfnCallbackFunction = pfnCallbackFunction;
            g_pvCallbackParameter = pvCallbackParameter;

            //
            // Read and display the Bluetooth version.
            //
            HCI_Version_Supported(g_uiBluetoothStackID, &sHCIVersion);
            g_sDeviceInfo.ucHCIVersion = (unsigned char)sHCIVersion;

            //
            // Read the local Bluetooth device address.
            //
            GAP_Query_Local_BD_ADDR(g_uiBluetoothStackID, &sBD_ADDR);
            BD_ADDR_To_Array(sBD_ADDR, g_sDeviceInfo.ucBDAddr);

            //
            // Go ahead and allow master/slave role switch.
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
            // In order to allow bonding we are making all devices:
            // discoverable, connectable, and [airable.  We are also
            // registering an authentication callback.  Pairing is not required
            // to use this application however some devices require it.
            //
            GAP_Set_Connectability_Mode(g_uiBluetoothStackID,
                                        cmNonConnectableMode);
            GAP_Set_Discoverability_Mode(g_uiBluetoothStackID,
                                        dmNonDiscoverableMode, 0);
            GAP_Set_Pairability_Mode(g_uiBluetoothStackID,
                                        pmPairableMode);

            //
            // Set the current state in the device info structure.
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
            // Set our local name.
            //
            BTPS_SprintF(g_sDeviceInfo.cDeviceName, DEFAULT_DEVICE_NAME);
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
                // Initialize the Extended Inquiry Data with predefined data
                //
                BTPS_MemCopy(psEIRData->Extended_Inquiry_Response_Data,
                             g_ucEIR,
                             sizeof(g_ucEIR));

                //
                // Write the Extended Inquiry Data to the controller.  This
                // will be used to respond to an Extended Inquiry request
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

            if(GAVD_Initialize(g_uiBluetoothStackID))
            {
                Display(("GAVD failed to Initialize\n"));
            }
            else
            {
                Display(("GAVD Initialized\n"));
            }

            //
            // Register an end point.
            //
            GAVDRegisterEndPoint();

            //
            // Initialize the SBC decoder.
            //
            g_sDecoderHandle = SBC_Initialize_Decoder();

            //
            // Initialize AVCTP (required for AVRCP).
            //
            if(AVCTP_Initialize(g_uiBluetoothStackID))
            {
                Display(("AVCP failed to Initialize\n"));
            }
            else
            {
                Display(("AVCTP Initialized\n"));
            }

            //
            // Register remote control controller
            //
            RegisterAVRCPController();

            //
            // Read the stored link key information from flash.
            //
            ReadFlash((NUM_SUPPORTED_LINK_KEYS * sizeof(tLinkKeyInfo)),
                      (unsigned char *)&g_sLinkKeyInfo);

            //
            // Count the number of link keys that are stored.
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

            DAC32SoundInit();

            //
            // Set the audio state to idle.
            //
            g_sAudioState = asIdle;

            //
            // First create an event to signal when the task should process
            // new SBC data.
            //
            if((g_sSBCDecodeEvent = BTPS_CreateEvent(FALSE)) != NULL)
            {
                //
                // Add a task that that will manage decoding the SBC data and
                // supplying the audio data buffer to the DAC.
                //
                g_bExit = FALSE;

                sSBCDecodeThread = BTPS_CreateThread(SBCDecodeThread,
                                                     SBC_DECODE_STACK_SIZE,
                                                     NULL);
                if(sSBCDecodeThread != NULL)
                {
                    //
                    // Everything initialized, go ahead and flag success
                    // to the caller.
                    //
                    iRetVal = 0;
                }
                else
                {
                    //
                    // Unable to create the SBC decode thread, so go ahead and
                    // free the event that we allocated and flag an error.
                    //
                    BTPS_CloseEvent(g_sSBCDecodeEvent);

                    iRetVal = BTH_ERROR_RESOURCE_FAILURE;
                }
            }
            else
            {
                iRetVal = BTH_ERROR_RESOURCE_FAILURE;
            }
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

    return(iRetVal);
}

