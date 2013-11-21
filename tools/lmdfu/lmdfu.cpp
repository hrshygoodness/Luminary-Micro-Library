//
// lmdfu.cpp : A library providing Device Firmware Upgrade function for use
//             with the TI Stellaris USB boot loader.
//

#include "stdafx.h"
#include <windows.h>
#include <errno.h>
#include <initguid.h>
#include "lmdfuprot.h"
#include "lmdfu.h"
#include "dfu_guids.h"
#include "lmusbdll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//****************************************************************************
//
// DFU Interface expected class and subclass.
//
//****************************************************************************
#define USB_CLASS_APP_SPECIFIC      0xFE
#define USB_DFU_SUBCLASS            0x01

//****************************************************************************
//
// Interface protocols for DFU mode and runtime mode.
//
//****************************************************************************
#define DFU_PROTOCOL                0x02
#define RUNTIME_PROTOCOL            0x01

//****************************************************************************
//
// DFU functional descriptor type
//
//****************************************************************************
#define USB_DESC_TYPE_DFU          0x21

//****************************************************************************
//
// Some standard USB request codes.
//
//****************************************************************************
#define GET_DESCRIPTOR 6

//****************************************************************************
//
// The sizes, in bytes, of several standard USB descriptor structures.
//
//****************************************************************************
#define SIZE_DEVICE_DESCRIPTOR     18
#define SIZE_CONFIG_DESCRIPTOR     9
#define SIZE_STRING_DESC_HEADER    2

//****************************************************************************
//
// Values used with GET_DESCRIPTOR to retrieve various descriptor types.
//
//****************************************************************************
#define DEVICE_DESCRIPTOR_VALUE    0x01
#define CONFIG_DESCRIPTOR_VALUE    0x02
#define STRING_DESCRIPTOR_VALUE    0x03
#define INTERFACE_DESCRIPTOR_VALUE 0x04

//****************************************************************************
//
// Macros to extract various fields from the DFU functional descriptor.  In
// each case, parameter p is a pointer to the first byte (length byte) of the
// descriptor.
//
//****************************************************************************
#define DFU_ATTRIBUTES(p)    (*(((unsigned char *)(p)) + 2))
#define DFU_DETACHTIMEOUT(p) ((*(((unsigned char *)(p)) + 3)) +               \
                             ((*(((unsigned char *)(p)) + 4)) << 8))
#define DFU_MAXTRANSFER(p)   ((*(((unsigned char *)(p)) + 5)) +               \
                             ((*(((unsigned char *)(p)) + 6)) << 8))
#define DFU_VERSION(p)       ((*(((unsigned char *)(p)) + 7)) +               \
                             ((*(((unsigned char *)(p)) + 8)) << 8))

//****************************************************************************
//
// The instance structure hidden behind a tLMDFUHandle.
//
//****************************************************************************
typedef struct
{
    LMUSB_HANDLE   hUSB;
    bool           bSupportsLMProtocol;
    bool           bRuntimeMode;
    unsigned short usVID;
    unsigned short usPID;
    unsigned short usDevice;
    unsigned short usTransferSize;
    unsigned char  ucDFUAttributes;
    unsigned short usInterface;
    unsigned short usLastFlashBlock;
    unsigned short usFirstFlashBlock;
    unsigned short usBlockNum;
    unsigned long  ulClassInfo;
    unsigned long  ulPartInfo;
}
tLMDFUDeviceState;

//*****************************************************************************
//
// Structure mapping values from the device information ulPartInfo field to
// Stellaris LM3Sxxxx part numbers.
//
//*****************************************************************************
typedef struct
{
    unsigned char ucPartNo;
    unsigned long ulLM3SPart;
    char *pcPartString;
}
tPartNumMapping;

tPartNumMapping g_sPartNumMap[] =
{
    //
    // USB Host + Device parts
    //
    { 0x44, 0x3739, "lm3s3739" },
    { 0x49, 0x3748, "lm3s3748" },
    { 0x45, 0x3749, "lm3s3749" },
    { 0x81, 0x5632, "lm3s5632" },
    { 0x96, 0x5732, "lm3s5732" },
    { 0x97, 0x5737, "lm3s5737" },
    { 0xA0, 0x5739, "lm3s5739" },
    { 0x99, 0x5747, "lm3s5747" },
    { 0xA7, 0x5749, "lm3s5749" },

    //
    // USB Device parts
    //
    { 0x41, 0x3026, "lm3s3j26" },
    { 0x40, 0x3026, "lm3s3n26" },
    { 0x3F, 0x3026, "lm3s3w26" },
    { 0x3E, 0x3026, "lm3s3z26" },
    { 0x09, 0x5031, "lm3s5k31" },
    { 0x4A, 0x5036, "lm3s5k36" },
    { 0x0A, 0x5031, "lm3s5p31" },
    { 0x48, 0x5036, "lm3s5p36" },
    { 0x07, 0x5031, "lm3s5r31" },
    { 0x4B, 0x5036, "lm3s5r36" },
    { 0x47, 0x5036, "lm3s5t36" },
    { 0x46, 0x5036, "lm3s5y36" },
    { 0x41, 0x3826, "lm3s3826" },

    //
    // USB OTG parts
    //
    { 0x43, 0x3651, "lm3s3651" },
    { 0x46, 0x3759, "lm3s3759" },
    { 0x48, 0x3768, "lm3s3768" },
    { 0x8A, 0x5652, "lm3s5652" },
    { 0x91, 0x5662, "lm3s5662" },
    { 0x9A, 0x5752, "lm3s5752" },
    { 0x9B, 0x5757, "lm3s5757" },
    { 0x9C, 0x5762, "lm3s5762" },
    { 0x9D, 0x5767, "lm3s5767" },
    { 0xA8, 0x5769, "lm3s5769" },
    { 0xA9, 0x5768, "lm3s5768" },
    { 0x68, 0x5091, "lm3s5b91" },
    { 0x0D, 0x5051, "lm3s5p51" },
    { 0x4C, 0x5056, "lm3s5p56" },
    { 0x66, 0x9090, "lm3s9b90" },
    { 0x6A, 0x9092, "lm3s9b92" },
    { 0x6E, 0x9095, "lm3s9b95" },
    { 0x6F, 0x9096, "lm3s9b96" },
//  { 0x??  , 0x9097, "lm3s9l97" },
    { 0x4D, 0x5656, "lm3s5656" },
    { 0x69, 0x5791, "lm3s5791" },
    { 0x0B, 0x5951, "lm3s5951" },
    { 0x4E, 0x5956, "lm3s5956" }
};

#define NUM_STELLARIS_PARTS (sizeof(g_sPartNumMap) / sizeof(tPartNumMapping))

//*****************************************************************************
//
// Structure mapping DFU status values to human readable strings for debug
// purposes.
//
//*****************************************************************************
typedef struct
{
    tDFUStatus eStatus;
    char *pcStatus;
}
tStatusMapping;

tStatusMapping g_psStatusMap[] =
{
    {STATUS_OK,                 "STATUS_OK"},
    {STATUS_ERR_TARGET,         "STATUS_ERR_TARGET"},
    {STATUS_ERR_FILE,           "STATUS_ERR_FILE"},
    {STATUS_ERR_WRITE,          "STATUS_ERR_WRITE"},
    {STATUS_ERR_ERASE,          "STATUS_ERR_ERASE"},
    {STATUS_ERR_CHECK_ERASED,   "STATUS_ERR_CHECK_ERASED"},
    {STATUS_ERR_PROG,           "STATUS_ERR_PROG"},
    {STATUS_ERR_VERIFY,         "STATUS_ERR_VERIFY"},
    {STATUS_ERR_ADDRESS,        "STATUS_ERR_ADDRESS"},
    {STATUS_ERR_NOTDONE,        "STATUS_ERR_NOTDONE"},
    {STATUS_ERR_FIRMWARE,       "STATUS_ERR_FIRMWARE"},
    {STATUS_ERR_VENDOR,         "STATUS_ERR_VENDOR"},
    {STATUS_ERR_USBR,           "STATUS_ERR_USBR"},
    {STATUS_ERR_POR,            "STATUS_ERR_POR"},
    {STATUS_ERR_UNKNOWN,        "STATUS_ERR_UNKNOWN"},
    {STATUS_ERR_STALLEDPKT,     "STATUS_ERR_STALLEDPKT"}
};

#define NUM_STATUS_MAP (sizeof(g_psStatusMap) / sizeof(tStatusMapping))

//*****************************************************************************
//
// Structure mapping DFU state values to human readable strings for debug
// purposes.
//
//*****************************************************************************
typedef struct
{
    tDFUState eState;
    char *pcState;
}
tStateMapping;

tStateMapping g_psStateMap[] =
{
    {STATE_APP_IDLE,            "APP_IDLE"},
    {STATE_APP_DETACH,          "APP_DETACH"},
    {STATE_IDLE,                "IDLE"},
    {STATE_DNLOAD_SYNC,         "DNLOAD_SYNC"},
    {STATE_DNBUSY,              "DNBUSY"},
    {STATE_DNLOAD_IDLE,         "DNLOAD_IDLE"},
    {STATE_MANIFEST_SYNC,       "MANIFEST_SYNC"},
    {STATE_MANIFEST,            "MANIFEST"},
    {STATE_MANIFEST_WAIT_RESET, "MANIFEST_WAIT_RESET"},
    {STATE_UPLOAD_IDLE,         "UPLOAD_IDLE"},
    {STATE_ERROR,               "ERROR"},
};

#define NUM_STATE_MAP (sizeof(g_psStateMap) / sizeof(tStateMapping))

//*****************************************************************************
//
// Storage for the CRC32 calculation lookup table.
//
//*****************************************************************************
unsigned long g_pulCRC32Table[256];

//*****************************************************************************
//
// Macros for accessing multi-byte fields in the DFU suffix and prefix.
//
//*****************************************************************************
#define WRITE_LONG(num, ptr)                                                  \
{                                                                             \
    *((unsigned char *)(ptr)) = (unsigned char)((num) & 0xFF);                \
    *(((unsigned char *)(ptr)) + 1) = (unsigned char)(((num) >> 8) & 0xFF);   \
    *(((unsigned char *)(ptr)) + 2) = (unsigned char)(((num) >> 16) & 0xFF);  \
    *(((unsigned char *)(ptr)) + 3) = (unsigned char)(((num) >> 24) & 0xFF);  \
}
#define WRITE_SHORT(num, ptr)                                                 \
{                                                                             \
    *((unsigned char *)(ptr)) = (unsigned char)((num) & 0xFF);                \
    *(((unsigned char *)(ptr)) + 1) = (unsigned char)(((num) >> 8) & 0xFF);   \
}

#define READ_SHORT(ptr)                                                       \
    (*((unsigned char *)(ptr)) | (*(((unsigned char *)(ptr)) + 1) << 8))

#define READ_LONG(ptr)                                                       \
    (*((unsigned char *)(ptr))              |                                \
    (*(((unsigned char *)(ptr)) + 1) << 8)  |                                \
    (*(((unsigned char *)(ptr)) + 2) << 16) |                                \
    (*(((unsigned char *)(ptr)) + 3) << 24))

//*****************************************************************************
//
// Internal function prototypes.
//
//*****************************************************************************
static tLMDFUErr DFUDownloadTransfer(tLMDFUDeviceState *pState,
                                     bool bCheckStatus, unsigned char *pcData,
                                     int iLength);
static tLMDFUErr DFUUploadTransfer(tLMDFUDeviceState *pState,
                                   unsigned char *pcData,
                                   int iLength);

//*****************************************************************************
//
// Initialize the CRC32 calculation table for the polynomial required by the
// DFU specification.  This code is based on an example found at
//
// http://www.createwindow.com/programming/crc32/index.htm.
//
//*****************************************************************************
static unsigned long
Reflect(unsigned long ulRef, char ucCh)
{
      unsigned long ulValue;
      int iLoop;

      ulValue = 0;

      //
      // Swap bit 0 for bit 7, bit 1 for bit 6, etc.
      //
      for(iLoop = 1; iLoop < (ucCh + 1); iLoop++)
      {
            if(ulRef & 1)
                  ulValue |= 1 << (ucCh - iLoop);
            ulRef >>= 1;
      }
      return ulValue;
}

static void
InitCRC32Table()
{
    unsigned long ulPolynomial;
    int i, j;

    // This is the ANSI X 3.66 polynomial as required by the DFU
    // specification.
    //
    ulPolynomial = 0x04c11db7;

    for(i = 0; i <= 0xFF; i++)
    {
        g_pulCRC32Table[i]=Reflect(i, 8) << 24;
          for (j = 0; j < 8; j++)
          {
              g_pulCRC32Table[i] = (g_pulCRC32Table[i] << 1) ^
                                     (g_pulCRC32Table[i] & (1 << 31) ?
                                      ulPolynomial : 0);
          }
          g_pulCRC32Table[i] = Reflect(g_pulCRC32Table[i], 32);
    }
}

//*****************************************************************************
//
// Calculate the CRC for the supplied block of data.
//
//*****************************************************************************
static unsigned long
CalculateCRC32(unsigned char *pcData, unsigned long ulLength,
               unsigned long ulStart)
{
    unsigned long ulCRC;
    unsigned long ulCount;
    unsigned char* pcBuffer;
    unsigned char ucChar;

    //
    // Initialize the CRC to the start value specified.
    //
    ulCRC = ulStart;

    //
    // Get a pointer to the start of the data and the number of bytes to
    // process.
    //
    pcBuffer = pcData;
    ulCount = ulLength;

    //
    // Perform the algorithm on each byte in the supplied buffer using the
    // lookup table values calculated in InitCRC32Table().
    //
    while(ulCount--)
    {
        ucChar = *pcBuffer++;
        ulCRC = (ulCRC >> 8) ^ g_pulCRC32Table[(ulCRC & 0xFF) ^ ucChar];
    }

    // Return the result.
    return (ulCRC);
}

//****************************************************************************
//
// Maps a Windows system error code into the matching tLMDUErr return code.
//
// This function maps a Windows system error code into the set of values
// that function calls to this DLL are speced to return.  Callers can still
// use GetLastError() to retrieve the Windows error codes but the original
// version of this DLL (based on libusb-win32 rather than WinUSB/lmusbdll)
// returned these values so we do this for backwards compatibility.
//
//****************************************************************************
static tLMDFUErr
MapRetcode(ULONG ulWinErr)
{
    switch(ulWinErr)
    {
        //
        // There was no error.
        //
        case ERROR_SUCCESS:
            return(DFU_OK);

        //
        // TODO: Add other mappings here as we determine what they are.
        //       Windows defines about 16000 and doesn't document exactly which
        //       may be returned from particular API calls :-(
        //

        //
        // All other unspecified errors.
        //
        default:
            return(DFU_ERR_UNKNOWN);
    }
}

//****************************************************************************
//
// Gets the device descriptor for the device whose handle is passed.
//
// Returns a pointer to the descriptor if found or NULL otherwise.  The client
// must free the pointer by calling free() once it is finished using the data.
//
//****************************************************************************
static PUCHAR
GetDeviceDescriptor(LMUSB_HANDLE hUSB)
{
    PUCHAR pucDevice;
    USHORT usCount;
    BOOL bRetcode;

    //
    // Allocate the 18 bytes we need to hold the device descriptor.
    //
    pucDevice = (PUCHAR)malloc(SIZE_DEVICE_DESCRIPTOR);
    if(!pucDevice)
    {
        return(NULL);
    }

    //
    // Send a standard request to get the device descriptor.
    //
    bRetcode = Endpoint0Transfer(hUSB, (REQUEST_TRANSFER_IN |
                                REQUEST_TYPE_STANDARD |
                                REQUEST_RECIPIENT_DEVICE), GET_DESCRIPTOR,
                                (DEVICE_DESCRIPTOR_VALUE << 8), 0,
                                SIZE_DEVICE_DESCRIPTOR, pucDevice, &usCount);

    //
    // Sanity check the result.
    //
    if(bRetcode && (usCount == SIZE_DEVICE_DESCRIPTOR) &&
       (pucDevice[0] == SIZE_DEVICE_DESCRIPTOR))
    {
        //
        // We read the expected number of bytes and the descriptor tells us it
        // is the expected size so assume it's good and return it to the
        // caller.
        //
        return(pucDevice);
    }
    else
    {
        //
        // Either we got the wrong number of bytes back or the descriptor
        // seems to be indicating the wrong size.  Either way, we consider this
        // an error.
        //
        free(pucDevice);
        return(NULL);
    }
}

//****************************************************************************
//
// Gets the current configuration descriptor for the device whose handle is
// passed.
//
// Returns a pointer to the descriptor if found or NULL otherwise.  The client
// must free the pointer by calling free() once it is finished using the data.
//
//****************************************************************************
static PUCHAR
GetConfigDescriptor(LMUSB_HANDLE hUSB)
{
    PUCHAR pucConfig;
    USHORT usCount;
    UCHAR pucCfgHeader[SIZE_CONFIG_DESCRIPTOR];
    BOOL bRetcode;

    //
    // Send a standard request to get the header of the config descriptor.
    //
    bRetcode = Endpoint0Transfer(hUSB, (REQUEST_TRANSFER_IN |
                                REQUEST_TYPE_STANDARD |
                                REQUEST_RECIPIENT_DEVICE), GET_DESCRIPTOR,
                                (CONFIG_DESCRIPTOR_VALUE << 8), 0,
                                SIZE_CONFIG_DESCRIPTOR, pucCfgHeader, &usCount);

    //
    // Sanity check the result.
    //
    if(!bRetcode || (usCount != SIZE_CONFIG_DESCRIPTOR) ||
       (pucCfgHeader[0] != SIZE_CONFIG_DESCRIPTOR))
    {
        //
        // Either we got the wrong number of bytes back or the descriptor
        // seems to be indicating the wrong size.  Either way, we consider this
        // an error and bomb out.
        //
        return(NULL);
    }

    //
    // Now that we have the header, allocate enough space for the whole
    // descriptor and get it.
    //
    pucConfig = (PUCHAR)malloc(READ_SHORT(pucCfgHeader + 2));
    if(!pucConfig)
    {
        return(NULL);
    }

    //
    // Now ask for the whole configuration descriptor.
    //
    bRetcode = Endpoint0Transfer(hUSB, (REQUEST_TRANSFER_IN |
                                 REQUEST_TYPE_STANDARD |
                                 REQUEST_RECIPIENT_DEVICE), GET_DESCRIPTOR,
                                 (CONFIG_DESCRIPTOR_VALUE << 8), 0,
                                 READ_SHORT(pucCfgHeader + 2), pucConfig,
                                 &usCount);

    //
    // Sanity check the result once again.
    //
    if(!bRetcode || (usCount != READ_SHORT(pucCfgHeader + 2)) ||
       (pucConfig[0] != SIZE_CONFIG_DESCRIPTOR) ||
       (READ_SHORT(pucConfig + 2) != READ_SHORT(pucCfgHeader + 2)))
    {
        //
        // Either we got the wrong number of bytes back or the descriptor
        // seems to be indicating a wrong size.  Either way, we consider this
        // an error and bomb out.
        //
        free(pucConfig);
        return(NULL);
    }
    else
    {
        //
        // Everything seems OK so return the descriptor we read.
        //
        return(pucConfig);
    }
}

//****************************************************************************
//
// Gets a particular string descriptor for the device whose handle is passed.
//
// Returns a pointer to the descriptor if found or NULL otherwise.  The client
// must free the pointer by calling free() once it is finished using the data.
//
//****************************************************************************
static PUCHAR
GetStringDescriptor(LMUSB_HANDLE hUSB, UCHAR ucStringIndex, USHORT usLanguageID)
{
    PUCHAR pucString;
    USHORT usCount;
    UCHAR pucHeader[SIZE_STRING_DESC_HEADER];
    BOOL bRetcode;

    //
    // Send a standard request to get the header of the config descriptor.
    //
    bRetcode = Endpoint0Transfer(hUSB, (REQUEST_TRANSFER_IN |
                                REQUEST_TYPE_STANDARD |
                                REQUEST_RECIPIENT_DEVICE), GET_DESCRIPTOR,
                                ((STRING_DESCRIPTOR_VALUE << 8) |
                                ucStringIndex), usLanguageID,
                                SIZE_STRING_DESC_HEADER, pucHeader, &usCount);

    //
    // Sanity check the result.
    //
    if(!bRetcode || (usCount != SIZE_STRING_DESC_HEADER) ||
       (pucHeader[0] < (SIZE_STRING_DESC_HEADER + 2)))
    {
        //
        // We got the wrong number of bytes back or the descriptor seems to be
        // indicating the wrong size.  Either way, we consider this an error
        // and bomb out.
        //
        return(NULL);
    }

    //
    // Now that we have the header, allocate enough space for the whole
    // descriptor and get it.
    //
    pucString = (PUCHAR)malloc(*pucHeader);
    if(!pucString)
    {
        return(NULL);
    }

    //
    // Now ask for the whole string descriptor.
    //
    bRetcode = Endpoint0Transfer(hUSB, (REQUEST_TRANSFER_IN |
                                 REQUEST_TYPE_STANDARD |
                                 REQUEST_RECIPIENT_DEVICE), GET_DESCRIPTOR,
                                 ((STRING_DESCRIPTOR_VALUE << 8) |
                                 ucStringIndex), usLanguageID,
                                 (USHORT)pucHeader[0], pucString, &usCount);

    //
    // Sanity check the result once again.
    //
    if(!bRetcode || (usCount != (USHORT)pucHeader[0]) ||
       (pucString[0] != (USHORT)pucHeader[0]))
    {
        //
        // Either we got the wrong number of bytes back or the descriptor
        // seems to be indicating a wrong size.  Either way, we consider this
        // an error and bomb out.
        //
        free(pucString);
        return(NULL);
    }
    else
    {
        //
        // Everything seems OK so return the descriptor we read.
        //
        return(pucString);
    }
}

//****************************************************************************
//
// Find the first descriptor with the given type in a block of descriptors.
//
// This function must be called before any other entry point in the library.
// It initalizes global data required to access DFU devices.
//
// Returns a pointer to the descriptor if found or NULL otherwise.
//
//****************************************************************************
static unsigned char *
FindDescriptor(unsigned char ucType, unsigned char *pcStart,
               unsigned long ulLength)
{
    unsigned char *pcEnd;

    //
    // Weed out completely bogus data.  A valid descriptor must be at least 2
    // bytes long.
    //
    if(ulLength < 2)
    {
        return(NULL);
    }

    //
    // Where's the last place we could sensibly look?
    //
    pcEnd = pcStart + (ulLength - 1);

    //
    // Walk the list of descriptors looking for one of the specified type.
    //
    while(pcStart < pcEnd)
    {
        //
        // Did we find a descriptor with the requested type?
        //
        if(*(pcStart + 1) == ucType)
        {
            //
            // Yes - return its pointer.
            //
            return(pcStart);
        }
        else
        {
            //
            // No - skip over this descriptor by adding the length (byte 1) to
            // the current descriptor start pointer.
            //
            pcStart += *pcStart;
        }
    }

    //
    // We did not find the requested descriptor.
    //
    return(NULL);
}

//****************************************************************************
//
// Populate a DFU file suffix structure with values appropriate for the
// passed image data.  Parameter pcData points to ulImageLen bytes of data
// with and a 16 byte block of free space at the end into which the suffix is
// written.
//
//****************************************************************************
static void
DFUSuffixPopulate(tLMDFUDeviceState *pState, unsigned char *pcData,
                  unsigned long ulImageLen)

{
    unsigned long ulCRC;

    //
    // Fill in the "static" fields in the DFU suffix at the end of the
    // supplied data.
    //
    pcData[ulImageLen + 11] = 16;
    pcData[ulImageLen + 10] = 'D';
    pcData[ulImageLen + 9] = 'F';
    pcData[ulImageLen + 8] = 'U';
    WRITE_SHORT(0x100, &(pcData[ulImageLen + 6]));
    WRITE_SHORT(pState->usVID, &(pcData[ulImageLen + 4]));
    WRITE_SHORT(pState->usPID, &(pcData[ulImageLen + 2]));
    WRITE_SHORT(pState->usDevice, &(pcData[ulImageLen]));

    //
    // Calculate the CRC of the whole block including the first 12 bytes of
    // the DFU suffix.
    //
    ulCRC = CalculateCRC32(pcData, ulImageLen + 12, 0xFFFFFFFF);

    //
    // Write the CRC to the final 4 bytes of the buffer.
    //
    WRITE_LONG(ulCRC, &(pcData[ulImageLen + 12]));
}

//****************************************************************************
//
// Query the DFU device information structure (for Stellaris-compatible
// devices).
//
//****************************************************************************
static tLMDFUErr
DFUDeviceInfoGet(tLMDFUDeviceState *pState, tDFUDeviceInfo *psDevInfo)
{
    tDFUDownloadInfoHeader sInfoCmd;
    tLMDFUErr eRetcode;

    //
    // Bomb out on bad parameters.
    //
    if(!psDevInfo || !pState)
    {
        return(DFU_ERR_INVALID_ADDR);
    }

    //
    // Set up the command we need to send to request the device information.
    //
    memset(&sInfoCmd, 0, sizeof(tDFUDownloadInfoHeader));
    sInfoCmd.ucCommand = STELLARIS_CMD_INFO;

    //
    // Send a download request containing our info read command.
    //
    eRetcode = DFUDownloadTransfer(pState, true, (unsigned char *)&sInfoCmd,
                                   sizeof(tDFUDownloadInfoHeader));

    //
    // Did we send the download command successfully?
    //
    if(eRetcode == DFU_OK)
    {
        //
        // Yes - now upload the device information structure.
        //
        eRetcode = DFUUploadTransfer(pState, (unsigned char *)psDevInfo,
                                     sizeof(tDFUDeviceInfo));
    }

    return(eRetcode);
}

//****************************************************************************
//
// Determine whether the connected device supports Stellaris's DFU binary
// protocol and, if so, query the device information structure and fill in
// various fields in the state structure.
//
//****************************************************************************
static bool
CheckForStellarisProtocol(tLMDFUDeviceState *pState)
{
    USHORT usCount;
    BOOL bRetcode;
    tLMDFUErr eRetcode;
    tDFUQueryStellarisProtocol sProt;
    tDFUDeviceInfo sDevInfo;

    //
    // Clear out the Stellaris-specific fields in the state structure.
    //
    pState->bSupportsLMProtocol = false;
    pState->ulClassInfo         = 0;
    pState->ulPartInfo          = 0;
    pState->usFirstFlashBlock   = 0;
    pState->usLastFlashBlock    = 0;

    //
    // Send our special request to the device and see if it responds.
    //
    bRetcode = Endpoint0Transfer(pState->hUSB, (REQUEST_TRANSFER_IN |
                          REQUEST_TYPE_CLASS | REQUEST_RECIPIENT_INTERFACE),
                          USBD_DFU_REQUEST_STELLARIS,
                          REQUEST_STELLARIS_VALUE,
                          pState->usInterface,
                          sizeof(tDFUQueryStellarisProtocol),
                          (PUCHAR)&sProt, &usCount);

    //
    // If we got back the amount of data we expected and the values match
    // those we expect, we can be pretty safe in assuming that this device
    // supports our protocol.
    //
    if(bRetcode && (usCount == sizeof(tDFUQueryStellarisProtocol)) &&
       (sProt.usMarker == LM_DFU_PROTOCOL_MARKER) &&
       (sProt.usVersion == LM_DFU_PROTOCOL_VERSION_1))
    {
        //
        // We can safely use Stellaris extensions when talking to this device.
        //
        pState->bSupportsLMProtocol = true;

        //
        // Now get the device information structure so that we know the
        // bounds of the writeable area of the device flash and the part
        // number we are talking to.
        //
        eRetcode = DFUDeviceInfoGet(pState, &sDevInfo);
        if(eRetcode == DFU_OK)
        {
            //
            // Remember the writeable flash bounds.
            //
            pState->usFirstFlashBlock = (unsigned short)(sDevInfo.ulAppStartAddr / 1024);
            pState->usLastFlashBlock = (unsigned short)(sDevInfo.ulFlashTop / 1024);
            pState->ulClassInfo = sDevInfo.ulClassInfo;
            pState->ulPartInfo = sDevInfo.ulPartInfo;
        }
    }

    //
    // Tell the caller whether the device supports our extensions or not.
    //
    return(pState->bSupportsLMProtocol);
}

//****************************************************************************
//
// Given the ulPartInfo value as returned in the DFU device information
// structure, return an integer representing the Stellaris part number in
// use.  For example, if running on an LM3S3748, this function will return
// value 0x3748 (hex).  The function also writes the part number string into
// the supplied buffer which is assumed to be at least NUM_PART_STRING_CHARS
// bytes long.
//
//****************************************************************************
static unsigned long
MapPartInfo(unsigned long ulPartInfo, char *strPart)
{
    int iLoop;
    unsigned char ucPartNo;

    //
    // Extract the part number from the part info value passed.
    //
    ucPartNo = (unsigned char)((ulPartInfo & STELLARIS_PART_M) >>
                               STELLARIS_PART_SHIFT);

    //
    // Search our part number mapping table for the LM3S part number
    // that is represented by the value in ulPartInfo.
    //
    for(iLoop = 0; iLoop < NUM_STELLARIS_PARTS; iLoop++)
    {
        if(g_sPartNumMap[iLoop].ucPartNo == ucPartNo)
        {
            //
            // We found it - copy the part number string and return the
            // part number.
            //
            strcpy_s(strPart, NUM_PART_STRING_CHARS,
                     g_sPartNumMap[iLoop].pcPartString);
            return(g_sPartNumMap[iLoop].ulLM3SPart);
        }
    }

    //
    // If we drop out, the part number we were passed doesn't exist in the
    // table so return 0 to indicate an unknown part.
    //
    return(0);
}

//****************************************************************************
//
// From the part and class information stored in the provided state structure,
// determine the actual Stellaris part number and revision we are talking to
// and fill in the appropriate values in the device info structure passed.
//
//****************************************************************************
void
GetPartNumberAndRev(tLMDFUDeviceState *pState, tLMDFUDeviceInfo *pDevInfo)
{
    //
    // Make sure we tell the caller whether this device supports the
    // Stellaris protocol or not.
    //
    pDevInfo->bSupportsStellarisExtensions = pState->bSupportsLMProtocol;

    //
    // Is the protocol supported?
    //
    if(pState->bSupportsLMProtocol)
    {
        //
        // Yes - fill in the part number and revision information.
        //
        pDevInfo->ulPartNumber = MapPartInfo(pState->ulPartInfo,
                                             pDevInfo->pcPartNumber);
        pDevInfo->cRevisionMajor = (char)((pState->ulClassInfo &
                                           STELLARIS_INFO_MAJ_M)>>
                                          STELLARIS_INFO_MAJ_SHIFT);
        pDevInfo->cRevisionMinor = (char)((pState->ulClassInfo &
                                           STELLARIS_INFO_MIN_M) >>
                                          STELLARIS_INFO_MIN_SHIFT);
    }
    else
    {
        //
        // No - clear the part number and revision fields.
        //
        pDevInfo->ulPartNumber = 0;
        pDevInfo->cRevisionMajor = 0;
        pDevInfo->cRevisionMinor = 0;
    }
}

//****************************************************************************
//
// Maps a DFU status value to a human-readable string for debug use.
//
//****************************************************************************
static char *
MapStatusToString(tDFUStatus eStatus)
{
    int iLoop;

    for(iLoop = 0; iLoop < NUM_STATUS_MAP; iLoop++)
    {
        if(g_psStatusMap[iLoop].eStatus == eStatus)
        {
            return(g_psStatusMap[iLoop].pcStatus);
        }
    }

    return("**UNKNOWN**");
}

//****************************************************************************
//
// Maps a DFU state to a human-readable string for debug use.
//
//****************************************************************************
static char *
MapStateToString(tDFUState eState)
{
    int iLoop;

    for(iLoop = 0; iLoop < NUM_STATE_MAP; iLoop++)
    {
        if(g_psStateMap[iLoop].eState == eState)
        {
            return(g_psStateMap[iLoop].pcState);
        }
    }

    return("**UNKNOWN**");
}

//****************************************************************************
//
// Maps a DFU status value into the equivalent return code  in tLMDFUErr.
//
//****************************************************************************
static tLMDFUErr
MapDFUStatus(tDFUStatus eStatus)
{
    switch(eStatus)
    {
        case STATUS_OK:
            return(DFU_OK);

        case STATUS_ERR_FILE:
        case STATUS_ERR_TARGET:
            return(DFU_ERR_UNSUPPORTED);

        case STATUS_ERR_NOTDONE:
        case STATUS_ERR_WRITE:
        case STATUS_ERR_ERASE:
        case STATUS_ERR_CHECK_ERASED:
        case STATUS_ERR_PROG:
        case STATUS_ERR_VERIFY:
            return(DFU_ERR_DNLOAD_FAIL);

        case STATUS_ERR_ADDRESS:
            return(DFU_ERR_INVALID_ADDR);

        case STATUS_ERR_FIRMWARE:
        case STATUS_ERR_VENDOR:
        case STATUS_ERR_USBR:
        case STATUS_ERR_POR:
        case STATUS_ERR_UNKNOWN:
            return(DFU_ERR_UNKNOWN);

        case STATUS_ERR_STALLEDPKT:
            return(DFU_ERR_STALL);

        default:
            return(DFU_ERR_UNKNOWN);
    }
}

//****************************************************************************
//
// Perform a single DFU download transfer and optionally block until it
// completes.
//
// \param pState is the instance data structure for the connection.
// \param bCheckState is \b true if the function should poll the device state
// and block until the state leaves DNLOAD_SYNC or DNBUSY, or \b false if
// the function should return immediately after posting the download request.
// \param pcData points to the download data to send.
// \param iLength is the number of bytes to send.  This must be less than
// or equal to the maximum transfer size for the target device.
//
// This function performs a single DFU download request and sends the supplied
// data as the request payload.  If bCheckState is \b true, the function will
// poll the device using GET_STATUS requests until the state is no longer
// reported as DNLOAD_SYNC or DNBUSY (indicating that the transfer completed
// or an error is being reported).
//
// \return Returns DFU_OK if the operation completed without any errors
// reported, DFU_ERR_TIMEOUT if the control transaction timed out,
// DFU_ERR_DISCONNECTED if the device was disconnected, DFU_ERR_STALL if the
// device stalled endpoint 0 to indicate an error, DFU_ERR_INVALID_SIZE if
// \e iLength is larger than the maximum transfer size for the target or
// DFU_ERR_UNKNOWN if some other low level USB error occurred.
//
//****************************************************************************
static tLMDFUErr
DFUDownloadTransfer(tLMDFUDeviceState *pState, bool bCheckStatus,
                    unsigned char *pcData, int iLength)
{
    USHORT usCount;
    BOOL bRetcode;
    tDFUGetStatusResponse sStatus;

    TRACE("Downloading %d bytes from 0x%08x\n", iLength, pcData);

    //
    // Make sure the size provided can be sent in a single DFU transfer.
    //
    if(iLength > (int)pState->usTransferSize)
    {
        TRACE("Bad size %d passed.\n", iLength);
        return(DFU_ERR_INVALID_SIZE);
    }

    //
    // Send the download request with the supplied payload.
    //
    bRetcode = Endpoint0Transfer(pState->hUSB, (REQUEST_TRANSFER_OUT |
                          REQUEST_TYPE_CLASS | REQUEST_RECIPIENT_INTERFACE),
                          USBD_DFU_REQUEST_DNLOAD, pState->usBlockNum++,
                          pState->usInterface, iLength, (PUCHAR)pcData,
                          &usCount);

    //
    // Did the transfer complete successfully?
    //
    if(!bRetcode)
    {
        TRACE("Error %d from Endpoint0Transfer\n", GetLastError());
        return(MapRetcode(GetLastError()));
    }

    //
    // Did we process the amount of data we expected?
    //
    if((int)usCount != iLength)
    {
        TRACE("Error - processed %d bytes, expected %d\n", (int)usCount,
              iLength);
        return(DFU_ERR_UNKNOWN);
    }

    if(bCheckStatus)
    {
        //
        // Keep checking the device status until we see a problem reported
        // or the device tells us that the previous download request completed.
        //
        do
        {
            bRetcode = Endpoint0Transfer(pState->hUSB, (REQUEST_TRANSFER_IN |
                                  REQUEST_TYPE_CLASS |
                                  REQUEST_RECIPIENT_INTERFACE),
                                  USBD_DFU_REQUEST_GETSTATUS, 0,
                                  pState->usInterface,
                                  sizeof(tDFUGetStatusResponse),
                                  (PUCHAR)&sStatus, &usCount);

            //
            // Did the transaction succeed?
            //
            if(!bRetcode)
            {
                TRACE("Error %d from Endpoint0Transfer\n", GetLastError());
                return(MapRetcode(GetLastError()));
            }
            else
            {
                //
                // Did we get the data we expected?
                //
                if(usCount != sizeof(tDFUGetStatusResponse))
                {
                    TRACE("Error - read %d bytes, expected %d\n",
                          (int)usCount, sizeof(tDFUGetStatusResponse));
                    return(DFU_ERR_UNKNOWN);
                }

                //
                // If the device is still busy, we need to wait a while before
                // polling again.
                //
                if((sStatus.bState == STATE_DNLOAD_SYNC) ||
                   (sStatus.bState == STATE_DNBUSY))
                {
                    DWORD dwTimeout;

                    //
                    // Read the timeout in milliseconds from the 3 byte field
                    // in the status structure.
                    //
                    dwTimeout = sStatus.bwPollTimeout[0] +
                                (sStatus.bwPollTimeout[1] << 8) +
                                (sStatus.bwPollTimeout[2] << 16);

                    //
                    // Twiddle our thumbs until this number of milliseconds has
                    // elapsed.
                    //
                    Sleep(dwTimeout);
                }
            }
        }
        while((sStatus.bState == STATE_DNLOAD_SYNC) ||
              (sStatus.bState == STATE_DNBUSY));

        //
        // Return the appropriate return code depending upon the device status.
        //
        TRACE("State on completion %s, status %s\n",
              MapStateToString((tDFUState)sStatus.bState),
              MapStatusToString((tDFUStatus)sStatus.bStatus));

        return(MapDFUStatus((tDFUStatus)sStatus.bStatus));
    }
    else
    {
        //
        // We didn't have to check the device status so tell the caller all is
        // apparently well.
        //
        return(DFU_OK);
    }
}

//****************************************************************************
//
// Download a block of data to the device using multiple transfers if necessary.
//
// \param pState is the instance data structure for the connection.
// \param pcData points to the download data to send.
// \param ulLength is the number of bytes to send.  If this is greater than the
// maximum transfer size for the device, multiple transfers will be used to
// complete the download.
//
// This function downloads a block of data to the DFU device using as many
// block transfers as are necessary to complete the operation.  It is assumed
// that the device is in a state ready to receive the data and that any
// necessary preamble commands have been sent prior to this call being made.
//
// \return Returns DFU_OK if the operation completed without any errors
// reported, DFU_ERR_TIMEOUT if the control transaction timed out,
// DFU_ERR_DISCONNECTED if the device was disconnected, DFU_ERR_STALL if the
// device stalled endpoint 0 to indicate an error, DFU_ERR_INVALID_SIZE if
// \e iLength is larger than the maximum transfer size for the target or
// DFU_ERR_UNKNOWN if some other low level USB error occurred.
//
//****************************************************************************
static tLMDFUErr
DFUDownloadBlock(tLMDFUDeviceState *pState, unsigned char *pcData,
                 unsigned long ulImageLen, HWND hwndNotify)
{
    unsigned long ulCount;
    unsigned long ulToSend;
    unsigned long ulTransfers;
    unsigned char *pcSending;
    tLMDFUErr eRetcode;

    //
    // Now we really are ready to download the image!  Send a message to
    // indicate that we are about to start.
    //
    if(hwndNotify)
    {
        //
        // How many transfers will this download take?
        //
        ulTransfers = ((ulImageLen + (pState->usTransferSize - 1)) /
                       pState->usTransferSize) + 1;
        PostMessage(hwndNotify, WM_DFU_DOWNLOAD, (WPARAM)ulTransfers,
                    (LPARAM)pState);
    }

    //
    // Set up for the download loop.
    //
    ulCount = ulImageLen;
    pcSending = pcData;
    ulTransfers = 0;

    //
    // Do we have any more data to send?
    //
    while(ulCount)
    {
        //
        // How many bytes can we send this transfer?
        //
        ulToSend = ((ulCount > pState->usTransferSize) ?
                        pState->usTransferSize : ulCount);

        //
        // Perform a single transfer.
        //
        eRetcode = DFUDownloadTransfer(pState, true, pcSending, ulToSend);

        //
        // If it failed, exit the loop.
        //
        if(eRetcode != DFU_OK)
            break;

        //
        // Move on to the next block.
        //
        pcSending += ulToSend;
        ulCount -= ulToSend;
        ulTransfers++;

        //
        // Send a progress update if required.
        //
        if(hwndNotify)
        {
            PostMessage(hwndNotify, WM_DFU_PROGRESS, (WPARAM)ulTransfers,
                        (LPARAM)pState);
        }
    }

    //
    // If we get here, either we exited the download loop due to an error or
    // the transfer is complete and we need to tell the device that we are
    // done.  Which is it?
    //
    if(eRetcode == DFU_OK)
    {
        //
        // We are done so send a 0 length download request as a signal.
        //
        eRetcode = DFUDownloadTransfer(pState, true, NULL, 0);
        ulTransfers++;
    }

    //
    // Send a message telling the caller either that we finished successfully
    // or that an error was notified.
    //
    if(hwndNotify)
    {
        PostMessage(hwndNotify,
                    (eRetcode == DFU_OK) ? WM_DFU_COMPLETE : WM_DFU_ERROR,
                    (WPARAM)ulTransfers, (LPARAM)pState);
    }

    return(eRetcode);
}

//****************************************************************************
//
// Perform a single DFU upload transfer.
//
// \param pState is the instance data structure for the connection.
// \param pcData points to a buffer which will receive the uploaded data.
// \param iLength is the number of bytes to receive.  This must be less than
// or equal to the maximum transfer size for the target device.
//
// This function performs a single DFU upload request to read a block of data
// back from the DFU device.
//
// \return Returns DFU_OK if the operation completed without any errors
// reported, DFU_ERR_TIMEOUT if the control transaction timed out,
// DFU_ERR_DISCONNECTED if the device was disconnected, DFU_ERR_STALL if the
// device stalled endpoint 0 to indicate an error, DFU_ERR_INVALID_SIZE if
// \e iLength is larger than the maximum transfer size for the target or
// DFU_ERR_UNKNOWN if some other low level USB error occurred.
//
//****************************************************************************
static tLMDFUErr
DFUUploadTransfer(tLMDFUDeviceState *pState, unsigned char *pcData, int iLength)
{
    BOOL bRetcode;
    USHORT usCount;

    //
    // Make sure the size provided can be sent in a single DFU transfer.
    //
    if(iLength > (int)pState->usTransferSize)
    {
        return(DFU_ERR_INVALID_SIZE);
    }

    //
    // Send the download request with the supplied payload.
    //
    bRetcode = Endpoint0Transfer(pState->hUSB, (REQUEST_TRANSFER_IN |
                          REQUEST_TYPE_CLASS | REQUEST_RECIPIENT_INTERFACE),
                          USBD_DFU_REQUEST_UPLOAD,
                          pState->usBlockNum++,
                          pState->usInterface,
                          (USHORT)iLength,
                          (PUCHAR)pcData, &usCount);

    //
    // Did the transfer complete successfully?
    //
    if(!bRetcode)
    {
        //
        // The transfer failed for some reason.
        //
        return(MapRetcode(GetLastError()));
    }
    else
    {
        //
        // The transfer succeeded. Did the amount of data processed match the
        // amount we expected.
        //
        return(((int)usCount == iLength) ? DFU_OK : DFU_ERR_UNKNOWN);
    }
}

//****************************************************************************
//
// Upload a block of data from the device using multiple transfers if necessary.
//
// \param pState is the instance data structure for the connection.
// \param pcData points to a buffer into which uploaded data will be written.
// \param ulLength is the number of bytes to read.  If this is greater than the
// maximum transfer size for the device, multiple transfers will be used to
// complete the upload.
// \param bVerifying indicates whether the WM_DFU_UPLOAD or WM_DFU_VERIFY
// message should be sent to hwndNotify at the start of the operation.
// \param hwndNotify is the handle of a window which will receive periodic
// WM_DFU_PROGRESS messages during the transfer.  If NULL, no messages will be
// sent.
//
// This function uploads a block of data from the DFU device using as many
// block transfers as are necessary to complete the operation.  It is assumed
// that the device is in a state ready to provide the data and that any
// necessary preamble commands have been sent prior to this call being made.
//
// \return Returns DFU_OK if the operation completed without any errors
// reported, DFU_ERR_TIMEOUT if the control transaction timed out,
// DFU_ERR_DISCONNECTED if the device was disconnected, DFU_ERR_STALL if the
// device stalled endpoint 0 to indicate an error or DFU_ERR_UNKNOWN if some
// other low level USB error occurred.
//
//****************************************************************************
static tLMDFUErr
DFUUploadBlock(tLMDFUDeviceState *pState, unsigned char *pcData, int iLength,
               bool bVerifying, HWND hwndNotify)
{
    unsigned long ulCount;
    unsigned long ulToRead;
    unsigned long ulTransfers;
    unsigned char *pcReading;
    tLMDFUErr eRetcode;

    //
    // Have we been passed a window handle?  If so, send the message
    // indicating that we are about to start a new operation.
    //
    if(hwndNotify)
    {
        //
        // How many transfers will this upload take?
        //
        ulTransfers = ((iLength + (pState->usTransferSize - 1)) /
                       pState->usTransferSize) + 1;
        PostMessage(hwndNotify, bVerifying  ? WM_DFU_VERIFY : WM_DFU_UPLOAD,
                    (WPARAM)ulTransfers, (LPARAM)pState);
    }

    //
    // Set up for the upload loop.
    //
    ulCount = (unsigned long)iLength;
    pcReading = pcData;
    ulTransfers = 0;

    //
    // Do we have any more data to send?
    //
    while(ulCount)
    {
        //
        // How many bytes can we send this transfer?
        //
        ulToRead = ((ulCount > pState->usTransferSize) ?
                    pState->usTransferSize : ulCount);

        //
        // Perform a single transfer.
        //
        eRetcode = DFUUploadTransfer(pState, pcReading, ulToRead);

        //
        // If it failed, exit the loop.
        //
        if(eRetcode != DFU_OK)
            break;

        //
        // Move on to the next block.
        //
        pcReading += ulToRead;
        ulCount -= ulToRead;
        ulTransfers++;

        //
        // Send a progress update if required.
        //
        if(hwndNotify)
        {
            PostMessage(hwndNotify, WM_DFU_PROGRESS, (WPARAM)ulTransfers,
                        (LPARAM)pState);
        }
    }

    //
    // Send a message telling the caller either that we finished successfully
    // or that an error was notified.
    //
    if(hwndNotify)
    {
        PostMessage(hwndNotify,
                    (eRetcode == DFU_OK) ? WM_DFU_COMPLETE : WM_DFU_ERROR,
                    (WPARAM)ulTransfers, (LPARAM)pState);
    }

    return(eRetcode);

}

//****************************************************************************
//
// Verify that the content of a block of flash matches the data passed.
//
// \param pState is the instance data structure for the connection.
// \param pcData points to the first byte of data that is to be verified.
// \param iLength is the number of bytes to verify.
// \param hwndNotify is the handle of a window which will receive periodic
// WM_DFU_PROGRESS messages during the transfer.  If NULL, no messages will be
// sent.
//
// This function uploads a block of data from the DFU device and checks that the
// returned data matches the content of the buffer passed to this function.  It
// is assumed that the device has previously been sent any necessary preamble
// commands to inform it of the range of flash to be read back during the
// upload operation.
//
// \return Returns DFU_OK if the operation completed without any errors
// reported, DFU_ERR_VERIFY_FAIL if the data read back did not match the
// contents of pcData, DFU_ERR_TIMEOUT if a control transaction timed out,
// DFU_ERR_DISCONNECTED if the device was disconnected, DFU_ERR_STALL if the
// device stalled endpoint 0 to indicate an error or DFU_ERR_UNKNOWN if some
// other low level USB error occurred.
//
//****************************************************************************
static tLMDFUErr
DFUVerifyBlock(tLMDFUDeviceState *pState, unsigned char *pcData, int iLength,
               HWND hwndNotify)
{
    unsigned char *pcBuffer;
    tLMDFUErr eRetcode;
    int iLoop;

    //
    // Allocate a buffer large enough to hold the downloaded image.
    //
    pcBuffer = (unsigned char *)malloc(iLength);
    if(!pcBuffer)
    {
        return(DFU_ERR_MEMORY);
    }

    //
    // Upload into the newly allocated buffer.
    //
    eRetcode = DFUUploadBlock(pState, (unsigned char *)pcBuffer, iLength,
                              true, hwndNotify);

    //
    // Did the upload complete successfully?
    //
    if(eRetcode == DFU_OK)
    {
        //
        // Yes - now compare the uploaded block with the original data.
        //
        for(iLoop = 0; iLoop < iLength; iLoop++)
        {
            if(pcBuffer[iLoop] != pcData[iLoop])
            {
                eRetcode = DFU_ERR_VERIFY_FAIL;
                TRACE("Verification failure at offset 0x%x - read 0x%02x, "
                      "expected 0x%02x\n", iLoop, pcBuffer[iLoop], pcData[iLoop]);
                break;
            }
        }
    }

    //
    // Free up our buffer and tell the caller whether or not the verification
    // succeeded.
    //
    free(pcBuffer);
    return(eRetcode);
}

//****************************************************************************
//
// Gets the DFU device status.
//
// \param pState is the instance data structure for the connection.
// \param pStatus is a pointer which will be written with the status structure
// returned by the DFU device.
//
// This function issues a USBD_DFU_REQUEST_GETSTATUS to the DFU device and
// returns the data that this provides.
//
// \return Returns DFU_OK if the operation completed without any errors
// reported, DFU_ERR_TIMEOUT if the control transaction timed out,
// DFU_ERR_DISCONNECTED if the device was disconnected, DFU_ERR_STALL if the
// device stalled endpoint 0 to indicate an error or DFU_ERR_UNKNOWN if some
// other low level USB error occurred.
//
//****************************************************************************
static tLMDFUErr
DFUDeviceStatusGet(tLMDFUDeviceState *pState, tDFUGetStatusResponse *pStatus)
{
    BOOL bRetcode;
    USHORT usCount;

    //
    // Request the status information via a control transaction on EP0.
    //
    bRetcode = Endpoint0Transfer(pState->hUSB, (REQUEST_TRANSFER_IN |
                                REQUEST_TYPE_CLASS |
                                REQUEST_RECIPIENT_INTERFACE),
                                USBD_DFU_REQUEST_GETSTATUS,
                                0, pState->usInterface,
                                sizeof(tDFUGetStatusResponse), (PUCHAR)pStatus,
                                &usCount);
    //
    // Did the transfer complete successfully?
    //
    if(!bRetcode && (usCount != sizeof(tDFUGetStatusResponse)))
    {
        return(MapRetcode(GetLastError()));
    }
    else
    {
        return(DFU_OK);
    }
}

//****************************************************************************
//
// Aborts a current operation and returns the device to idle state.
//
// \param pState is the instance data structure for the connection.
//
// This function issues a USBD_DFU_REQUEST_ABORT to the DFU device.
//
// \return Returns DFU_OK if the operation completed without any errors
// reported, DFU_ERR_TIMEOUT if the control transaction timed out,
// DFU_ERR_DISCONNECTED if the device was disconnected, DFU_ERR_STALL if the
// device stalled endpoint 0 to indicate an error or DFU_ERR_UNKNOWN if some
// other low level USB error occurred.
//
//****************************************************************************
static tLMDFUErr
DFUDeviceAbort(tLMDFUDeviceState *pState)
{
    BOOL bRetcode;
    USHORT usCount;

    //
    // Send the download request with the supplied payload.
    //
    bRetcode = Endpoint0Transfer(pState->hUSB, (REQUEST_TRANSFER_OUT |
                                REQUEST_TYPE_CLASS |
                                REQUEST_RECIPIENT_INTERFACE),
                                USBD_DFU_REQUEST_ABORT,
                                0, pState->usInterface, 0, (PUCHAR)NULL,
                                &usCount);
    //
    // Did the transfer complete successfully?
    //
    if(!bRetcode)
    {
        return(MapRetcode(GetLastError()));
    }
    else
    {
        return(DFU_OK);
    }
}

//****************************************************************************
//
// Clears any device error status.
//
// \param pState is the instance data structure for the connection.
//
// This function issues a USBD_DFU_REQUEST_CLRSTATUS to the DFU device.
//
// \return Returns DFU_OK if the operation completed without any errors
// reported, DFU_ERR_TIMEOUT if the control transaction timed out,
// DFU_ERR_DISCONNECTED if the device was disconnected, DFU_ERR_STALL if the
// device stalled endpoint 0 to indicate an error or DFU_ERR_UNKNOWN if some
// other low level USB error occurred.
//
//****************************************************************************
static tLMDFUErr
DFUDeviceStatusClear(tLMDFUDeviceState *pState)
{
    BOOL bRetcode;
    USHORT usCount;

    //
    // Send the download request with the supplied payload.
    //
    bRetcode = Endpoint0Transfer(pState->hUSB, (REQUEST_TRANSFER_OUT |
                                REQUEST_TYPE_CLASS |
                                REQUEST_RECIPIENT_INTERFACE),
                                USBD_DFU_REQUEST_CLRSTATUS,
                                0, pState->usInterface, 0, (PUCHAR)NULL,
                                &usCount);
    //
    // Did the transfer complete successfully?
    //
    if(!bRetcode && (usCount != sizeof(tDFUGetStatusResponse)))
    {
        return(MapRetcode(GetLastError()));
    }
    else
    {
        return(DFU_OK);
    }
}

//****************************************************************************
//
// Checks the status of the DFU device and does what is necessary to make it
// idle in preparation for a new upload or download.
//
// \param pState is the instance data structure for the connection.
//
// This function read the DFU device status and, if it's not in state IDLE,
// performs an ABORT and/or CLRSTATUS to get it back into IDLE state.
//
// \return Returns DFU_OK if the operation completed without any errors
// reported, DFU_ERR_TIMEOUT if the control transaction timed out,
// DFU_ERR_DISCONNECTED if the device was disconnected, DFU_ERR_STALL if the
// device stalled endpoint 0 to indicate an error, DFU_ERR_UNSUPPORTED if the
// device is in APP_IDLE, APP_DETACH or MANIFEST_WAIT_SYNC states or is not
// manifest tolerant and is currently in MANIFEST or MANIFEST_SYNC state,
// or DFU_ERR_UNKNOWN if some other low level USB error occurred.
//
//****************************************************************************
static tLMDFUErr
DFUMakeDeviceIdle(tLMDFUDeviceState *pState)
{
    tDFUGetStatusResponse sStatus;
    tLMDFUErr eRetcode;

    //
    // Get the current device status.
    //
    eRetcode = DFUDeviceStatusGet(pState, &sStatus);

    //
    // Make sure we got the status successfully.
    //
    if(eRetcode == DFU_OK)
    {
        switch(sStatus.bState)
        {
            //
            // Are we already idle?
            //
            case STATE_IDLE:
                break;

            //
            // Are we in the error state?
            //
            case STATE_ERROR:
            {
                //
                // Yes - we need to clear the status to get back to idle.
                //
                eRetcode = DFUDeviceStatusClear(pState);
            }
            break;

            case STATE_MANIFEST:
            case STATE_MANIFEST_SYNC:
            {
                //
                // If the device is manifest tolerant, we can proceed to wait
                // a while then abort to get back to IDLE. If, however, it is
                // not, we can't get back to IDLE without a reset (which we
                // don't do here).
                //
                if(!(pState->ucDFUAttributes && DFU_ATTR_MANIFEST_TOLERANT))
                {
                    return(DFU_ERR_UNSUPPORTED);
                }
            }
            //
            // Drop through.
            //

            //
            // Is the device currently busy?
            //
            case STATE_DNBUSY:
            {
                DWORD dwTimeout;

                //
                // The device is busy. We need to wait for the polling timeout
                // then send an abort.
                //
                dwTimeout = sStatus.bwPollTimeout[0] +
                            (sStatus.bwPollTimeout[1] << 8) +
                            (sStatus.bwPollTimeout[2] << 8);

                Sleep(dwTimeout);
            }
            //
            // Drop through.
            //

            case STATE_DNLOAD_SYNC:
            case STATE_DNLOAD_IDLE:
            case STATE_UPLOAD_IDLE:
            {
                eRetcode = DFUDeviceAbort(pState);
            }
            break;

            //
            // In these states, it is not possible to get back to IDLE without
            // a device reset which we don't do here.  These are, therefore,
            // considered error cases.
            //
            case STATE_MANIFEST_WAIT_RESET:
            case STATE_APP_IDLE:
            case STATE_APP_DETACH:
                return(DFU_ERR_UNSUPPORTED);
        }
    }

    //
    // Tell the caller how things went.
    //
    return(eRetcode);
}

//****************************************************************************
//
// Reset the attached DFU device.
//
// This function sends a command to the attached device informing it that it
// should initiate a system reset.  Only devices supporting the Stellaris
// protocol extensions will be affected by this function call.  Use of a
// protocol extension is required since WinUSB does not provide any method for
// an application to reset a USB device.
//
// \return Returns DFU_ERR_UNSUPPORTED if the device does not support Stellaris
// protocol extensions.
//
//****************************************************************************
static tLMDFUErr
DFUDeviceReset(tLMDFUDeviceState *pState)
{
    tDFUDownloadHeader sHeader;
    tLMDFUErr eRetcode;

    //
    // This operation is only supported if the target device supports the
    // Stellaris protocol.
    //
    if(!pState->bSupportsLMProtocol)
    {
        TRACE("Device does not support Stellaris protocol.\n");
        return(DFU_ERR_UNSUPPORTED);
    }

    //
    // First, make sure the target is idle and ready to receive a new
    // command.
    //
    eRetcode = DFUMakeDeviceIdle(pState);
    if(eRetcode != DFU_OK)
    {
        TRACE("Error %s (%d) attempting to get device into IDLE state.\n",
              LMDFUErrorStringGet(eRetcode), eRetcode);
        return(eRetcode);
    }

    //
    // Send the target the command telling it to reset.  If this succeeds, the
    // device will not be accessible after this function returns.
    //
    sHeader.ucCommand = STELLARIS_CMD_RESET;

    eRetcode = DFUDownloadTransfer(pState, true, (unsigned char *)&sHeader,
                                   sizeof(tDFUDownloadHeader));

    //
    // Tell the caller how things went.
    //
    return(eRetcode);
}

//****************************************************************************
//
//! Initialize the DFU library.
//!
//! This function must be called before any other entry point in the library.
//! It initalizes global data required to access DFU devices.
//!
//! \return Returns DFU_OK.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUInit(void)
{
    TRACE("LMDFUInit\n");

    //
    // Initialize the DFU CRC32 lookup table.
    //
    InitCRC32Table();

    return(DFU_OK);
}

//****************************************************************************
//
//! Opens a DFU device in preparation for download or upload operations.
//!
//! \param iDeviceIndex is a zero-based index indicating which DFU-capable
//! device is to be opened.
//! \param psDevInfo is the structure which will be filled in with information
//! relating to the DFU device which has been opened.
//! \param phHandle points to storage which will be written with a valid
//! DFU device handle on success.
//!
//! This function opens a DFU device and returns a handle allowing further
//! DFU operations to be performed on the device and also information on the
//! device state and capabilities.
//!
//! Note that this function will open DFU devices which are currently in
//! runtime mode.  The caller must ensure that a device is in DFU mode prior
//! to making any requests which are not supported in runtime mode.  Currently,
//! this library does not contain a function to cause a DFU device to switch
//! from runtime to DFU mode.
//!
//! Handles allocated by this function must be closed using a matching call to
//! LMDFUDeviceClose().
//!
//! \return Returns DFU_OK on success (in which case *phHandle and *psDevInfo
//! will also be written with valid information), DFU_ERR_NOT_FOUND if a DFU
//! device with index iDeviceIndex cannot be found, or DFU_ERR_INVALID_ADDR if
//! a NULL pointer is passed in any parameter.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUDeviceOpen(int iDeviceIndex, tLMDFUDeviceInfo *psDevInfo,
                tLMDFUHandle *phHandle)
{
    int iCount = 0;
    int iIntCount;
    LMUSB_HANDLE hHandle;
    tLMDFUDeviceState *pState;
    PUCHAR pucDevice;
    PUCHAR pucConfig;
    PUCHAR pucInterface;
    PUCHAR pucDFU;
    BOOL bInstalled;

    TRACE("LMDFUDeviceOpen %d\n", iDeviceIndex);

    //
    // Check for a bad pointers.
    //
    if(!psDevInfo || !phHandle)
    {
        return(DFU_ERR_INVALID_ADDR);
    }

    //
    // Search for the required device and open it if found.  Note that this
    // call will only find devices identified by the Stellaris DFU GUID.
    //
    hHandle = InitializeDeviceByIndex(DFU_VID, DFU_PID,
                                      (LPGUID)&(GUID_DEVINTERFACE_STELLARIS_DFU),
                                      iDeviceIndex, FALSE, &bInstalled);

    //
    // Did we find a device supporting the Stellaris DFU class?
    //
    if(!hHandle)
    {
        //
        // We couldn't find the device so return an appropriate error.
        //
        return(DFU_ERR_NOT_FOUND);
    }

    //
    // Allocate a state structure for this instance.
    //
    pState = (tLMDFUDeviceState *)
             malloc(sizeof(tLMDFUDeviceState));
    if(!pState)
    {
        return(DFU_ERR_MEMORY);
    }

    //
    // Remember our device handle.
    //
    pState->hUSB = hHandle;

    //
    // Get the device descriptor.
    //
    pucDevice = GetDeviceDescriptor(hHandle);

    if(!pucDevice)
    {
        //
        // For some reason, we can't query the device descriptor.  Without this,
        // we can't get some vital information so return an error.
        //
        free(pState);
        return(DFU_ERR_UNKNOWN);
    }

    //
    // Clear the device information structure.
    //
    memset(psDevInfo, 0, sizeof(tLMDFUDeviceInfo));

    //
    // Extract fields we are interested in from the device descriptor.
    //
    psDevInfo->usVID = READ_SHORT(pucDevice + 8);
    psDevInfo->usPID = READ_SHORT(pucDevice + 10);
    psDevInfo->usDevice = READ_SHORT(pucDevice + 12);
    psDevInfo->ucManufacturerString = pucDevice[14];
    psDevInfo->ucProductString = pucDevice[15];
    psDevInfo->ucSerialString = pucDevice[16];

    //
    // Get the configuration descriptor.  We assume the default configuration
    // is in use.
    //
    pucConfig = GetConfigDescriptor(hHandle);

    if(!pucConfig)
    {
        //
        // For some reason, we can't query the device descriptor.  Without this,
        // we can't get some vital information so return an error.
        //
        free(pucDevice);
        free(pState);
        return(DFU_ERR_UNKNOWN);
    }

    //
    // Find the interface offering DFU functionality.  We start the search at
    // the end of the configuration descriptor itself.
    //
    pucInterface = pucConfig + pucConfig[0];
    for(iIntCount = 0; iIntCount < pucConfig[4]; iIntCount++)
    {
        //
        // Search from the current position in the config descriptor looking
        // for the next interface descriptor.
        //
        pucInterface = FindDescriptor(INTERFACE_DESCRIPTOR_VALUE,
                                      pucInterface,
                                      ((unsigned long)READ_SHORT(pucConfig + 2) -
                                      (unsigned long)(pucInterface - pucConfig)));

        if(!pucInterface)
        {
            //
            // We ran off the end of the config descriptor without finding
            // the interface descriptor we are looking for.  This suggests
            // the device isn't offering a DFU interface after all.
            //
            free(pucConfig);
            free(pucDevice);
            free(pState);
            return(DFU_ERR_UNKNOWN);
        }
        else
        {
            //
            // We found an interface descriptor.  Is it the DFU one?
            //
            if((pucInterface[5] == USB_CLASS_APP_SPECIFIC) &&
               (pucInterface[6] == USB_DFU_SUBCLASS))
            {
                //
                // Yes - break out of the search.
                //
                break;
            }
            else
            {
                //
                // No - skip to the next descriptor to continue our search.
                //
                pucInterface += pucInterface[0];
            }
        }
    }

    //
    // Did we hit the end of the search without finding a DFU interface?
    //
    if(iIntCount == pucConfig[4])
    {
        //
        // Yes - this is an error.
        //
        free(pucConfig);
        free(pucDevice);
        free(pState);
        return(DFU_ERR_UNKNOWN);
    }

    //
    // Remember the interface string descriptor ID and whether or not it is
    // in DFU mode.
    //
    psDevInfo->ucDFUInterfaceString = pucInterface[8];
    psDevInfo->bDFUMode = ((pucInterface[7] == DFU_PROTOCOL) ? true : false);

    //
    // Find the DFU functional descriptor.  It will follow the interface
    // descriptor we just found.
    //
    pucDFU = FindDescriptor(USB_DESC_TYPE_DFU,
                            pucInterface + pucInterface[0],
                            ((unsigned long)(READ_SHORT(pucConfig + 2) - (pucInterface - pucConfig)) -
                             (unsigned long)pucInterface[0]));

    //
    // Was one there?
    //
    if(pucDFU)
    {
        //
        // Yes - extract the useful stuff from it.
        //
        psDevInfo->usDetachTimeOut = DFU_DETACHTIMEOUT(pucDFU);
        psDevInfo->usTransferSize = DFU_MAXTRANSFER(pucDFU);
        psDevInfo->ucDFUAttributes = DFU_ATTRIBUTES(pucDFU);
    }
    else
    {
        //
        // This naughty DFU device doesn't publish a DFU functional descriptor!
        //
        psDevInfo->usDetachTimeOut = 0;
        psDevInfo->usTransferSize = 0;
        psDevInfo->ucDFUAttributes = 0;
    }    
	
	//
    // Remember various important pieces of info about the device then tell
    // the caller we found it, giving them back the handle.
    //
    pState->usVID = psDevInfo->usVID;
    pState->usPID = psDevInfo->usPID;
    pState->usTransferSize = psDevInfo->usTransferSize;
    pState->ucDFUAttributes = psDevInfo->ucDFUAttributes;
    pState->usInterface = (unsigned short)pucInterface[2];
    pState->usBlockNum = 0;

    //
    // Is the device in DFU mode?  If we are in runtime mode, skip these
    // queries since they are only supported in DFU mode.
    //
    if(psDevInfo->bDFUMode)
    {
        //
        // Make sure the device is idle before we do anything that may be
        // illegal in various error states.
        //
        DFUMakeDeviceIdle(pState);

        //
        // Determine whether this device supports the Stellaris-specific binary
        // protocol and, if so, read the device information structure.
        //
        CheckForStellarisProtocol(pState);

        //
        // If possible, determine the specific Stellaris part we are talking to.
        //
        GetPartNumberAndRev(pState, psDevInfo);
    }

    //
    // Free the device and configuration descriptor storage.
    //
    free(pucDevice);
    free(pucConfig);

    //
    // If we get here, all is well so return the handle to the caller.
    //
    *phHandle = (tLMDFUHandle)pState;
    return(DFU_OK);
}

//****************************************************************************
//
//! Closes a DFU device and, optionally, returns it to its run time
//! configuration.
//!
//! \param hHandle is the handle of the DFU device as returned from a
//! previous call to LMDFUDeviceOpen().
//! \param bReset indicates whether to leave the device in DFU mode (\b false)
//! or reset it and return to run time mode (\b true).
//!
//! This function closes a DFU device previously opened using a call to
//! LMDFUDeviceOpen().  If the \e bReset parameter is \b true, the device
//! is reset and returned to its run time mode of operation.  If \e bReset
//! is false, the device is left in DFU mode and can be reopened again
//! without the need to perform a mode switch.
//!
//! \return Returns DFU_OK on success or other error codes on failure.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUDeviceClose(tLMDFUHandle hHandle, bool bReset)
{
    BOOL bRetcode;
    tLMDFUDeviceState *pState;

    TRACE("LMDFUDeviceClose 0x%08x\n", hHandle);

    if(!hHandle)
    {
        return(DFU_ERR_HANDLE);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    //
    // Have we been asked to reset the device?
    //
    if(bReset)
    {
        //
        // Yes - do it.
        //
        DFUDeviceReset(pState);
    }

    //
    // Close the device handle.
    //
    bRetcode = TerminateDevice(pState->hUSB);

    //
    // Free our state data.
    //
    free(pState);

    //
    // Tell the caller if all is well.
    //
    return(bRetcode ? DFU_OK : DFU_ERR_HANDLE);
}

//****************************************************************************
//
//! Retrieves a string descriptor from a DFU device.
//!
//! \param hHandle is the handle of the DFU device from which the string is
//! being queried.
//! \param ucStringIndex is the index of the string that is being queried.
//! \param ucLanguageID is the identifier of the language that the string is
//! to be returned in.
//! \param pcString points to a buffer into which the returned string is to
//! be written.
//! \param pusStringLen points to a variable containing the size of the
//! pcString buffer (in bytes) on entry.  If the string is read, this variable
//! is updated to show the number of bytes written into pcString.
//!
//! This function retrieves Unicode strings from the DFU device. If the
//! requested string is available, DFU_OK is returned and the string is
//! written into the supplied \e pcString buffer.
//!
//! \return Returns DFU_OK on success or DFU_ERR_NOT_FOUND if the supplied
//! string ID or language codes are invalid.  If a NULL pointer is passed,
//! DFU_ERR_INVALID_ADDR is returned.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUDeviceStringGet(tLMDFUHandle hHandle,
                     unsigned char ucStringIndex,
                     unsigned short usLanguageID,
                     char *pcString,
                     unsigned short *pusStringLen)
{
    int iCount;
    tLMDFUDeviceState *pState;
    PUCHAR pucString;

    TRACE("LMDFUDeviceStringGet 0x%08x %d %d\n", hHandle, ucStringIndex,
          usLanguageID);

    //
    // Bomb out on bad parameters.
    //
    if(!hHandle)
    {
        TRACE("Bad handle.\n");
        return(DFU_ERR_HANDLE);
    }

    if(!pcString || !pusStringLen)
    {
        TRACE("NULL pointer.\n");
        return(DFU_ERR_INVALID_ADDR);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    //
    // Get the requested string descriptor.
    //
    pucString = GetStringDescriptor(pState->hUSB, ucStringIndex, usLanguageID);

    //
    // Was a string returned?
    //
    if(pucString == (PUCHAR)NULL)
    {
        TRACE("Failed to read string descriptor %d, language 0x%04x.\n",
              ucStringIndex, usLanguageID);
        return(DFU_ERR_NOT_FOUND);
    }

    //
    // How many bytes to we need to copy?
    //
    iCount = (int)pucString[0] - SIZE_STRING_DESC_HEADER;

    //
    // Is the passed buffer large enough to hold this?
    //
    if(iCount > (int)*pusStringLen)
    {
        //
        // No - truncate the string to the length passed.
        //
        iCount =(int)*pusStringLen;
    }

    //
    // Copy the UNICODE string into the client's buffer.
    //
    memcpy(pcString, pucString + SIZE_STRING_DESC_HEADER, iCount);

    //
    // Update the client's length to indicate the number fof bytes
    // actually written.
    //
    *pusStringLen = (USHORT)iCount;

    //
    // Free the string descriptor.
    //
    free(pucString);

    //
    // All is well so tell the caller they got their string.
    //
    return(DFU_OK);
}

//****************************************************************************
//
//! Retrieves an ASCII string descriptor from a DFU device.
//!
//! \param psDevInfo is the structure defining the device whose string is
//! being queried.
//! \param ucStringIndex is the index of the string that is being queried.
//! \param pcString points to a buffer into which the returned string is to
//! be written.
//! \param pusStringLen points to a variable containing the size of the
//! pcString buffer (in bytes) on entry.  If the string is read, this variable
//! is updated to show the number of bytes written into pcString.
//!
//! This function retrieves a string descriptor from the device and converts it
//! from UNICODE into 8-bit-per-character ASCII, writing the result into the
//! supplied pcString buffer.  The string returned is in the first language
//! supported by the device.
//!
//! \return Returns DFU_OK on success or DFU_ERR_NOT_FOUND if the supplied
//! string ID is invalid.  If a NULL pointer is passed, DFU_ERR_INVALID_ADDR
//! is returned.
//
//****************************************************************************
tLMDFUErr __stdcall LMDFUDeviceASCIIStringGet(tLMDFUHandle hHandle,
                                              unsigned char ucStringIndex,
                                              char *pcString,
                                              unsigned short *pusStringLen)
{
    int iNumChars;
    tLMDFUDeviceState *pState;
    PUCHAR pucLangTable;
    PUCHAR pucDesc;
    errno_t eError;

    TRACE("LMDFUDeviceASCIIStringGet 0x%08x %d\n", hHandle, ucStringIndex);

    //
    // Bomb out on bad parameters.
    //
    if(!hHandle)
    {
        TRACE("Bad handle\n");
        return(DFU_ERR_HANDLE);
    }

    if(!pcString || !pusStringLen)
    {
        TRACE("NULL pointer\n");
        return(DFU_ERR_INVALID_ADDR);
    }

    //
    // We don't allow the caller to query string descriptor 0 via this call.
    //
    if(!ucStringIndex)
    {
        TRACE("String index 0 cannot be read via this call.\n");
        return(DFU_ERR_NOT_FOUND);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    //
    // Get the language table so that we can determine the language code for
    // the first supported language.
    //
    pucLangTable = GetStringDescriptor(pState->hUSB, 0, 0);

    //
    // Did the table exist? If not, there are no strings offered by the device.
    //
    if(!pucLangTable)
    {
        TRACE("Can't read language table.\n");
        return(DFU_ERR_NOT_FOUND);
    }

    //
    // Make sure the table is long enough. It has to have at least 1 language
    // entry in it.
    //
    if(pucLangTable[0] < (SIZE_STRING_DESC_HEADER + 2))
    {
        TRACE("Language table appears corrupted.\n");
        free(pucLangTable);
        return(DFU_ERR_NOT_FOUND);
    }

    //
    // Now that we have the language table, query the actual descriptor that
    // we need.
    //
    pucDesc = GetStringDescriptor(pState->hUSB, ucStringIndex,
                                  READ_SHORT(pucLangTable + 2));

    //
    // Free the language table since we don't need it any more.
    //
    free(pucLangTable);

    //
    // Did we get the requested string descriptor?
    //
    if(!pucDesc)
    {
        TRACE("Can't read string descriptor %d for language 0x%04x.\n",
              ucStringIndex, READ_SHORT(pucLangTable + 2));
        return(DFU_ERR_NOT_FOUND);
    }

    //
    // Is it a sensible length?
    //
    if(pucDesc[0] <= (SIZE_STRING_DESC_HEADER + 1))
    {
        //
        // String must be corrupt - it's too short.
        //
        TRACE("String descriptor appears corrupted.\n");
        free(pucDesc);
        return(DFU_ERR_NOT_FOUND);
    }

    //
    // We got the descriptor and the string is not zero length so now we need
    // to convert it into an ASCII string.  First, how long is it?
    //
    iNumChars = (pucDesc[0] - SIZE_STRING_DESC_HEADER) / 2;

    //
    // Now create a UNICODE string object containing the string from the
    // descriptor.
    //
    CStringW strUnicode((WCHAR *)(pucDesc + SIZE_STRING_DESC_HEADER), iNumChars);

    //
    // If that worked, we now create a new ASCII string based on the Unicode
    // one.
    //
    CStringA strASCII(strUnicode);

    //
    // Now determine the length of the ASCII string and copy the content out
    // to the buffer provided by the caller.
    //
    iNumChars = strASCII.GetLength();

    //
    // Can this string fit in the buffer provided by the caller?
    //
    if(iNumChars >= (int)*pusStringLen)
    {
        //
        // No - we need to truncate the string to the size of the buffer,
        // remembering to leave a place free for the terminating NULL.
        //
        iNumChars = *pusStringLen - 1;
    }

    //
    // Now copy the string into the buffer.
    //
    eError = strcpy_s(pcString, *pusStringLen, (LPCSTR)strASCII);

    //
    // Free our descriptor.
    //
    free(pucDesc);

    //
    // Tell the caller all is well (assuming it is)
    //
    if(eError)
    {
        TRACE("Error converting string from UNICODE to ASCII.\n");
        return(DFU_ERR_UNKNOWN);
    }
    else
    {
        *pusStringLen = iNumChars;
        return(DFU_OK);
    }
}

//****************************************************************************
//
//! Query DFU download-related parameters from a Stellaris DFU device.
//!
//! \param hHandle is the handle of the DFU device whose information is to be
//! read.
//! \param psParams points to a structure which will be written with DFU
//! parameters related to the device.
//!
//! This function retrieves various parameters related to the download from
//! a Stellaris device.  These parameters include the device flash block size,
//! the number of flash blocks (hence the size of the flash), the programmable
//! address range, and the ID of the Stellaris part itself.
//!
//! This function makes use of a Stellaris-specific extension to the
//! DFU protocol so is only available on Stellaris devices.
//!
//! \return Returns DFU_OK on success or other error codes on failure.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUParamsGet(tLMDFUHandle hHandle, tLMDFUParams *psParams)
{
    tLMDFUDeviceState *pState;
    tDFUDownloadInfoHeader sInfoCmd;
    tDFUDeviceInfo sDevInfo;
    tLMDFUErr eRetcode;

    TRACE("LMDFUDeviceParamsGet 0x%08x\n", hHandle);

    //
    // Bomb out on bad parameters.
    //
    if(!hHandle)
    {
        TRACE("Bad handle.\n");
        return(DFU_ERR_HANDLE);
    }

    if(!psParams)
    {
        TRACE("NULL pointer.\n");
        return(DFU_ERR_INVALID_ADDR);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    eRetcode = DFUDeviceInfoGet(pState, &sDevInfo);

    //
    // Set up the command we need to send to request the device information.
    //
    memset(&sInfoCmd, 0, sizeof(tDFUDownloadInfoHeader));
    sInfoCmd.ucCommand = STELLARIS_CMD_INFO;

    //
    // Did the request complete successfully?
    //
    if(eRetcode == DFU_OK)
    {
        //
        // Copy the relevant fields from the device information structure
        // into the client's storage.
        //
        psParams->ulAppStartAddr = sDevInfo.ulAppStartAddr;
        psParams->ulFlashTop = sDevInfo.ulFlashTop;
        psParams->usFlashBlockSize = sDevInfo.usFlashBlockSize;
        psParams->usNumFlashBlocks = sDevInfo.usNumFlashBlocks;
    }

    return(eRetcode);
}

//****************************************************************************
//
//! Determines whether the supplied data is a correctly formatted DFU image.
//!
//! \param hHandle is the handle of the DFU device which the image is destined
//! to be used with.
//! \param pcDFUImage is a pointer to the first byte of the image to check.
//! \param ulImageLen is the number of bytes in the image data pointed to by
//! \e pcDFUImage.
//! \param pbStellarisFormat is a pointer which will be written to \e true if
//! the supplied data appears to start with a valid Stellaris DFU prefix.
//! If the data ends in a valid DFU suffix structure but does not contain the
//! Stellaris prefix, this value will be written to \e false.
//!
//! This function checks a provided binary to determine whether it is a
//! correctly formatted DFU image or not.  A valid image contains a 16 byte
//! suffix with a checksum and the IDs of the intended target device.  An image
//! is considered to be valid if the following criteria are met:
//!
//! 1. The CRC of the whole block calculates to 0 (i.e. the CRC of all but the
//!    last 4 bytes equals the CRC stored in the last 4 bytes).
//! 2. The "DFU" suffix marker exists at the correct place at the end of the
//!    data block.
//! 3. The vendor and products IDs read from the expected positions in the
//!    DFU suffix match the VID and PID of the device whose handle is passed.
//!
//! Additionally, if these conditions are met, the data is examined for the
//! presence of a Stellaris DFU prefix.  This structure contains the
//! address at which to flash the image and also the length of the payload.
//! The pbStellarisFormat pointer is written to \e true if the following
//! additional criteria are met:
//!
//! 1. The first byte of the data block is 0x01.
//! 2. The unsigned long in bytes 4 through 7 of the image matches the
//!    value of the length of the block minus the DFU suffix (length read from
//!    the suffix itself) and Stellaris prefix (8 bytes).
//! 3. The unsigned short in bytes 2 and 3 of the image forms a sensible
//!    flash block number (address / 1024) for a Stellaris device.
//!
//! \return Returns DFU_OK if a valid DFU suffix is found and the IDs it
//! contains match those of the target device.  If the suffix is valid but the
//! IDs do not match, DFU_ERR_UNSUPPORTED is returned.  If the suffix is
//! invalid, DFU_ERR_INVALID_FORMAT is returned.  Parameter errors are
//! indicated by return codes DFU_ERR_INVALID_ADDR and DFU_ERR_HANDLE.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUIsValidImage(tLMDFUHandle hHandle, unsigned char *pcDFUImage,
                   unsigned long ulImageLen, bool *pbStellarisFormat)
{
    tLMDFUDeviceState *pState;
    unsigned long ulCRC;

    TRACE("LMDFUDeviceIsValidImage 0x%08x 0x%08x %d\n", hHandle, pcDFUImage,
           ulImageLen);

    //
    // Catch obviously bad parameters.
    //
    if(!hHandle)
    {
        return(DFU_ERR_HANDLE);
    }

    if(!pcDFUImage || !pbStellarisFormat)
    {
        return(DFU_ERR_INVALID_ADDR);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    //
    // Catch images that are obviously too small to contain the DFU suffix.
    // The suffix will be at least 16 bytes long and contains its length in
    // the 5th last byte of the block.
    //
    if((ulImageLen < 16) || (ulImageLen < pcDFUImage[ulImageLen - 5]))
    {
        return(DFU_ERR_INVALID_FORMAT);
    }

    //
    // First calculate the CRC of the data we've been passed. If this doesn't
    // come out to zero, it's not a valid DFU image.
    //
    ulCRC = CalculateCRC32(pcDFUImage, ulImageLen, 0xFFFFFFFF);

    if(ulCRC)
    {
        return(DFU_ERR_INVALID_FORMAT);
    }

    //
    // The CRC is valid so now check for the expected "DFU" marker in the
    // suffix structure we assume is at the end of the data.
    //
    if((pcDFUImage[ulImageLen - 6] != 'D') ||
       (pcDFUImage[ulImageLen - 7] != 'F') ||
       (pcDFUImage[ulImageLen - 8] != 'U'))
    {
        //
        // The marker is not correct.
        //
        return(DFU_ERR_INVALID_FORMAT);
    }

    //
    // Here we know that we have a valid DFU file intended for use with the
    // device we intend using it with.  Does it have a Stellaris prefix?  It
    // does if the first byte is 0x01 and the length read from the supposed
    // prefix matches the expected length.  Note that we do not check for a
    // valid target address here - we leave the device to do this for us.
    //
    if((pcDFUImage[0] != 0x01) ||
       (READ_LONG(pcDFUImage + 4) != (ulImageLen -
        (pcDFUImage[ulImageLen - 5] + 8))))
    {
        *pbStellarisFormat = false;
    }
    else
    {
        *pbStellarisFormat = true;
    }

    //
    // The DFU suffix looks valid but does it indicate that the image is for
    // the device whose handle we have been passed?
    //
    if((pState->usVID != READ_SHORT(&(pcDFUImage[ulImageLen - 12]))) ||
       (pState->usPID != READ_SHORT(&(pcDFUImage[ulImageLen - 14]))))
    {
        //
        // The VID and PID in the file don't match the current device.
        //
        return(DFU_ERR_UNSUPPORTED);
    }
    else
    {
        //
        // At this point, we know the image is fine.
        //
        return(DFU_OK);
    }
}

//****************************************************************************
//
//! Download a DFU-formatted binary image to device flash.
//!
//! \param hHandle is the handle of the DFU device whose flash is to be written.
//! \param pcDFUImage is a pointer to the first byte of the image to download.
//! \param ulImageLen is the number of bytes in the image data pointed to by
//! \e pcDFUImage.
//! \param bVerify should be set to \b true if the download is to be verified
//! (by reading back the image and checking it against the original data) or
//! \b false if verification is not necessary.
//! \param bIgnoreIDs should be set to \b true if the DFU image is to be
//! downloaded regardless of the fact that the DFU suffix contains a VID or PID
//! that differs from the target device.  If set to \b false, the call will
//! fail with DFU_ERR_UNSUPPORTED if the device VID and PID do not match the
//! values in the DFU suffix.
//! \param hwndNotify is the handle of a window to which periodic notifications
//! will be sent indicating the progress of the operation.  If NULL, no status
//! notifications will be sent.
//!
//! This function downloads a DFU-formatted binary to the device.  A valid
//! binary contains both the standard DFU footer suffix structure and also a
//! Stellaris-specific header informing the device of the address to
//! which the image is to be written.  If the data passed does not appear to
//! contain this information,\b DFU_ERR_INVALID_FORMAT will be returned and
//! the image will not be written to the device flash.
//!
//! This function is synchronous and will not return until the operation is
//! complete.  To receive periodic status updates during the operation, a
//! window handle may be provided.  This window will receive WM_LMDFU_STATUS
//! messages during the download operation allowing an application to update
//! its user interface accordingly.
//!
//! To flash a pure binary image without the DFU suffix or Stellaris
//! prefix, use LMDFUDownloadBin() instead of this function.
//!
//! Note that the Stellaris-specific prefix structure contains the address at
//! which the image is to be flashed so no parameter exists here to provide
//! this information.
//!
//! This function uses no Stellaris-specific extensions to the DFU protocol and
//! should, therefore,  allow a correctly formatted firmware image to be
//! downloaded to any DFU device (though, frankly, this has not been tested).
//!
//! \return Returns DFU_OK on success or other error codes on failure.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUDownload(tLMDFUHandle hHandle, unsigned char *pcDFUImage,
              unsigned long ulImageLen, bool bVerify, bool bIgnoreIDs,
              HWND hwndNotify)
{
    tLMDFUDeviceState *pState;
    tDFUGetStatusResponse sStatus;
    tLMDFUErr eRetcode;
    bool bExtensions;

    TRACE("LMDFUDownload 0x%08x 0x%08x %d %s, %s\n", hHandle, pcDFUImage,
        ulImageLen, (bVerify ? "Verify" : "Don't verify"),
        (bIgnoreIDs ? "Ignore IDs" : "Check IDs"));

    //
    // Bomb out on bad parameters.
    //
    if(!hHandle)
    {
        return(DFU_ERR_HANDLE);
    }

    if(!pcDFUImage)
    {
        return(DFU_ERR_INVALID_ADDR);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    //
    // Make sure this DFU device supports downloads.
    //
    if(!(pState->ucDFUAttributes && DFU_ATTR_CAN_DOWNLOAD))
    {
        return(DFU_ERR_UNSUPPORTED);
    }

    //
    // Make sure the DFU device is in idle state and ready to receive
    // a new image download.
    //
    eRetcode = DFUMakeDeviceIdle(pState);
    if(eRetcode != DFU_OK)
    {
        return(eRetcode);
    }

    //
    // We should be all set to start the download now.  Check to make sure that
    // the provided image is valid.
    //
    eRetcode = LMDFUIsValidImage(hHandle, pcDFUImage, ulImageLen, &bExtensions);

    //
    // Is the image valid?
    //
    if(!((eRetcode == DFU_OK) ||
        (bIgnoreIDs && (eRetcode == DFU_ERR_UNSUPPORTED))))
    {
        //
        // Either the image is invalid or it is for a device other than the
        // one we are talking to and we have not been told to ignore the
        // embedded IDs.  Either way, we bomb out without doing anything.
        //
        return(DFU_ERR_INVALID_FORMAT);
    }

    //
    // The image is valid.  We remove the DFU suffix since this is not sent
    // to the target.
    //
    ulImageLen -= pcDFUImage[ulImageLen - 5];

    //
    // Download the contents of the file minus the DFU suffix which we don't
    // send to the device.
    //
    eRetcode = DFUDownloadBlock(pState, pcDFUImage, ulImageLen, hwndNotify);

    if((eRetcode == DFU_OK) && bVerify)
    {
        //
        // We finished the download so now we need to determine whether or not
        // we can verify the operation.  If the device is manifest tolerant,
        // we query its status and expect it to go back to state IDLE at this
        // point.  If this happens, we can read back the image to verify that
        // it is as expected.
        //
        eRetcode = DFUDeviceStatusGet(pState, &sStatus);
        if((eRetcode == DFU_OK) && (sStatus.bState == STATE_IDLE))
        {
            //
            // The device is in IDLE state so we can proceed to verify the
            // downloaded image.
            //
            eRetcode = DFUVerifyBlock(pState, pcDFUImage, ulImageLen,
                                      hwndNotify);
        }
        else
        {
            //
            // We can't verify the image since the device didn't go back into
            // IDLE state.
            //
            TRACE("Can't verify download. Device is not IDLE.\n");
            return(DFU_ERR_CANT_VERIFY);
        }
    }

    return(eRetcode);
}

//****************************************************************************
//
//! Download a binary image to device flash.
//!
//! \param hHandle is the handle of the DFU device whose flash is to be written.
//! \param pcBinaryImage is a pointer to the first byte of the image to
//! download.
//! \param ulImageLen is the number of bytes in the image data pointed to by
//! \e pcDFUImage.
//! \param ulStartAddr is the flash address at which the image is to be
//! written.  If this parameter is set to 0 and the target device supports
//! Stellaris extensions, the binary image will be downloaded to the
//! currently-configured application start address.
//! \param bVerify should be set to \b true if the download is to be verified
//! (by reading back the image and checking it against the original data) or
//! \b false if verification is not necessary.
//! \param hwndNotify is the handle of a window to which periodic notifications
//! will be send indicating the progress of the operation.  If NULL, no status
//! notifications will be sent.
//!
//! This function downloads a pure binary image containing no DFU suffix or
//! Stellaris header to the device at an address supplied by the caller.
//!
//! This function is synchronous and will not return until the operation is
//! complete.  To receive periodic status updates during the operation, a
//! window handle may be provided.  This window will receive WM_LMDFU_STATUS
//! messages during the download operation allowing an application to update
//! its user interface accordingly.
//!
//! To flash a DFU-formatted image, use LMDFUDownload() instead of this
//! function.
//!
//! This function makes use of a Stellaris-specific extension to the
//! DFU protocol so is only available on Stellaris devices.
//!
//! \return Returns DFU_OK on success, DFU_ERR_INVALID_ADDR if the start address
//! is invalid or if the image is too large to fit within the available flash,
//! or appropriate error codes on other failures.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUDownloadBin(tLMDFUHandle hHandle, unsigned char *pcBinaryImage,
                 unsigned long ulImageLen, unsigned long ulStartAddr,
                 bool bVerify, HWND hwndNotify)
{
    tLMDFUDeviceState *pState;
    tDFUDownloadProgHeader sProg;
    tDFUGetStatusResponse sStatus;
    tLMDFUErr eRetcode;

    TRACE("LMDFUDownloadBin 0x%08x 0x%08x %d %s\n", hHandle, pcBinaryImage,
        ulImageLen, bVerify ? "Verify" : "Don't verify");

    //
    // Bomb out on bad parameters.
    //
    if(!hHandle)
    {
        return(DFU_ERR_HANDLE);
    }

    if(!pcBinaryImage)
    {
        return(DFU_ERR_INVALID_ADDR);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    //
    // We only support binary downloads for Stellaris targets since we
    // need to use a protocol extension to tell the board where to flash the
    // image.
    //
    if(!pState->bSupportsLMProtocol)
    {
        TRACE("Target does not support Stellaris DFU protocol.\n");
        return(DFU_ERR_UNSUPPORTED);
    }

    //
    // Make sure that the start address and length are valid and that the image
    // will fit into the programmable area of flash.
    //
    if((ulStartAddr + ulImageLen) >
       (unsigned long)(pState->usLastFlashBlock * 1024))
    {
        TRACE("Image is located outside writeable area of flash.\n");
        return(DFU_ERR_INVALID_ADDR);
    }

    //
    // First, make sure the target is idle and ready to receive a new
    // download.
    //
    eRetcode = DFUMakeDeviceIdle(pState);
    if(eRetcode != DFU_OK)
    {
        TRACE("Error %s (%d) attempting to get device into IDLE state.\n",
              LMDFUErrorStringGet(eRetcode), eRetcode);
        return(eRetcode);
    }

    //
    // First send the download header that tells the board where to put the
    // image.
    //
    sProg.ucCommand = STELLARIS_CMD_PROG;
    sProg.ucReserved = 0;
    sProg.usStartAddr = (unsigned short)(ulStartAddr / 1024);
    sProg.ulLength = ulImageLen;
    eRetcode = DFUDownloadTransfer(pState, true, (unsigned char *)&sProg,
                                   sizeof(tDFUDownloadProgHeader));
    if(eRetcode != DFU_OK)
    {
        TRACE("Error %s (%d) sending PROG command to device.\n",
              LMDFUErrorStringGet(eRetcode), eRetcode);
        return(eRetcode);
    }

    //
    // Now we really are ready to download the image!
    //
    eRetcode = DFUDownloadBlock(pState, pcBinaryImage, ulImageLen, hwndNotify);

    if((eRetcode == DFU_OK) && bVerify)
    {
        //
        // We finished the download so now we need to determine whether or not
        // we can verify the operation.  If the device is manifest tolerant,
        // we query its satus and expect it to go back to state IDLE at this
        // point.  If this happens, we can read back the image to verify that
        // it is as expected.
        //
        eRetcode = DFUDeviceStatusGet(pState, &sStatus);
        if((eRetcode == DFU_OK) && (sStatus.bState == STATE_IDLE))
        {
            tDFUDownloadBinHeader sBin;
            tDFUDownloadReadCheckHeader sRead;

            //
            // The device is in IDLE state so we can proceed to verify the
            // downloaded image.  First we tell the device that we want it to
            // pass us raw data without any DFU header structures added.
            //
            memset(&sBin, 0, sizeof(tDFUDownloadBinHeader));
            sBin.ucCommand = STELLARIS_CMD_BIN;
            sBin.bBinary = 1;
            eRetcode = DFUDownloadTransfer(pState, true,  (unsigned char *)&sBin,
                                           sizeof(tDFUDownloadBinHeader));
            if(eRetcode == DFU_OK)
            {
                //
                // Now tell it which area of flash we want to read.
                //
                memset(&sRead, 0, sizeof(tDFUDownloadReadCheckHeader));
                sRead.ucCommand = STELLARIS_CMD_READ;
                sRead.usStartAddr = (unsigned short)(ulStartAddr / 1024);
                sRead.ulLength = ulImageLen;

                eRetcode = DFUDownloadTransfer(pState, true, (unsigned char *)&sRead,
                                           sizeof(tDFUDownloadReadCheckHeader));

                if(eRetcode == DFU_OK)
                {
                    //
                    // Finally, read back what we downloaded and make sure that it
                    // agrees with what was sent.
                    //

                    eRetcode = DFUVerifyBlock(pState, pcBinaryImage,
                                              ulImageLen, hwndNotify);
                }

                //
                // Revert to non-binary transfers
                //
                sBin.bBinary = 0;
                DFUDownloadTransfer(pState, true, (unsigned char *)&sBin,
                                    sizeof(tDFUDownloadBinHeader));
            }
        }
        else
        {
            //
            // We can't verify the image since the device didn't go back into
            // IDLE state.
            //
            TRACE("Can't verify download. Device is not IDLE.\n");
            return(DFU_ERR_CANT_VERIFY);
        }
    }

    return(eRetcode);
}

//****************************************************************************
//
//! Erases a section of the device flash.
//!
//! \param hHandle is the handle of the DFU device whose flash is to be erased.
//! \param ulStartAddr is the address of the first byte of flash to be erased.
//! This must correspond to a flash block boundary, typically multiples of 1024
//! bytes.  If this parameter is set to 0 the entire writeable flash region
//! will be erased.
//! \param ulEraseLen is the number of bytes of flash to erase. This must be a
//! multiple of the flash block size, typically 1024 bytes.
//! \param bVerify should be set to \b true if the erase is to be verified
//! (by reading back the flash blocks and ensuring that all bytes contain 0xFF)
//! or \b false if verification is not necessary.
//! \param hwndNotify is the handle of a window to which periodic notifications
//! will be send indicating the progress of the operation.  If NULL, no status
//! notifications will be sent.
//!
//! This function erases a section of the device flash and, optionally, checks
//! that the resulting area has been correctly erased before returning.
//!
//! The function is synchronous and will not return until the operation is
//! complete.  To receive periodic status updates during the operation, a
//! window handle may be provided.  This window will receive WM_LMDFU_STATUS
//! messages during the erase operation allowing an application to update
//! its user interface accordingly.
//!
//! This function makes use of a Stellaris-specific extension to the
//! DFU protocol so is only available on Stellaris devices.
//!
//! \return Returns DFU_OK on success or DFU_ERR_INVALID_ADDR if the start
//! address is not on a flash block boundary or if the erase length is not a
//! multiple of the flash block size.  Appropriate return codes will be used
//! to indicate other errors.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUErase(tLMDFUHandle hHandle, unsigned long ulStartAddr,
           unsigned long ulEraseLen, bool bVerify, HWND hwndNotify)
{
    tLMDFUDeviceState *pState;
    tDFUDownloadEraseHeader sErase;
    tLMDFUErr eRetcode;

    TRACE("LMDFUErase 0x%08x 0x%08x %d %s\n", hHandle, ulStartAddr,
        ulEraseLen, bVerify ? "Verify" : "Don't verify");

    //
    // Bomb out on bad parameters.
    //
    if(!hHandle)
    {
        return(DFU_ERR_HANDLE);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    //
    // This operation is only supported if the target device supports the
    // Stellaris protocol.
    //
    if(!pState->bSupportsLMProtocol)
    {
        TRACE("Device does not support Stellaris protocol.\n");
        return(DFU_ERR_UNSUPPORTED);
    }

    //
    // Make sure we were passed a block start address and a length that is an
    // integer multiple of the block size.
    //
    if(ulStartAddr == 0)
    {
        //
        // If the start address passed is 0, we erase the whole writable region
        // of flash.
        //
        ulStartAddr = pState->usFirstFlashBlock * 1024;
        ulEraseLen = (pState->usLastFlashBlock - pState->usFirstFlashBlock) *
                     1024;
        TRACE("Erasing entire writeable region. %dKB from 0x%x\n",
              ulEraseLen / 1024, ulStartAddr);
    }
    else
    {
        //
        // Is the start address correctly aligned?
        //
        if(ulStartAddr & (1024 - 1))
        {
            TRACE("Address is not a multiple of 1024\n");
            return(DFU_ERR_INVALID_ADDR);
        }

        if(!ulEraseLen || (ulEraseLen & (1024 - 1)))
        {
            TRACE("Size is not a multiple of 1024 or is zero.\n");
            return(DFU_ERR_INVALID_SIZE);
        }
    }

    //
    // First, make sure the target is idle and ready to receive a new
    // command.
    //
    eRetcode = DFUMakeDeviceIdle(pState);
    if(eRetcode != DFU_OK)
    {
        TRACE("Error %s (%d) attempting to get device into IDLE state.\n",
              LMDFUErrorStringGet(eRetcode), eRetcode);
        return(eRetcode);
    }

    //
    // Send the target the command to erase the relevant section of memory.
    //
    sErase.ucCommand = STELLARIS_CMD_ERASE;
    sErase.ucReserved = 0;
    sErase.usStartAddr = (unsigned short)(ulStartAddr / 1024);
    sErase.usNumBlocks = (unsigned short)(ulEraseLen / 1023);
    sErase.ucReserved2[0] = 0;
    sErase.ucReserved2[1] = 0;

    eRetcode = DFUDownloadTransfer(pState, true, (unsigned char *)&sErase,
                                   sizeof(tDFUDownloadEraseHeader));

    //
    // Did the operation complete successfully and do we want to verify the
    // erase operation?
    //
    if((eRetcode == DFU_OK) && bVerify)
    {
        tDFUDownloadReadCheckHeader sCheck;

        //
        // Yes - now go ahead and verify that the flash is indeed erased.
        //
        sCheck.ucCommand = STELLARIS_CMD_CHECK;
        sCheck.ucReserved = 0;
        sCheck.usStartAddr = (unsigned short)(ulStartAddr / 1024);
        sCheck.ulLength = ulEraseLen;

        eRetcode = DFUDownloadTransfer(pState, true, (unsigned char *)&sCheck,
                                       sizeof(tDFUDownloadReadCheckHeader));
    }

    return(eRetcode);
}

//****************************************************************************
//
//! Verify that a section of the device flash is blank.
//!
//! \param hHandle is the handle of the DFU device whose flash is to be checked.
//! \param ulStartAddr is the address of the first byte of flash to be checked.
//! This must correspond to a flash block boundary, typically multiples of 1024
//! bytes.  If this parameter is set to 0 the entire writeable flash region
//! will be checked.
//! \param ulLen is the number of bytes of flash to check.
//!
//! This function checks a region of the device flash and reports whether or
// not it is blank (with all bytes containing value 0xFF).
//!
//! The function is synchronous and will not return until the operation is
//! complete.
//!
//! This function makes use of a Stellaris-specific extension to the
//! DFU protocol so is only available on Stellaris devices.
//!
//! \return Returns DFU_OK on success or DFU_ERR_INVALID_ADDR if the start
//! address is not on a flash block boundary or if the erase length is not a
//! multiple of the flash block size.  If the region is not blank, the return
//! code will be DFU_ERR_DNLOAD_FAIL.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUBlankCheck(tLMDFUHandle hHandle, unsigned long ulStartAddr,
                unsigned long ulLen)
{
    tLMDFUDeviceState *pState;
    tDFUDownloadReadCheckHeader sCheck;
    tLMDFUErr eRetcode;

    TRACE("LMDFUBlankCheck 0x%08x 0x%08x %d\n", hHandle, ulStartAddr,
        ulLen);

    //
    // Bomb out on bad parameters.
    //
    if(!hHandle)
    {
        return(DFU_ERR_HANDLE);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    //
    // This operation is only supported if the target device supports the
    // Stellaris protocol.
    //
    if(!pState->bSupportsLMProtocol)
    {
        TRACE("Device does not support Stellaris protocol.\n");
        return(DFU_ERR_UNSUPPORTED);
    }

    //
    // Make sure we were passed a block start address and a length that is an
    // integer multiple of the block size.
    //
    if(ulStartAddr == 0)
    {
        //
        // If the start address passed is 0, we check the whole writable region
        // of flash.
        //
        ulStartAddr = pState->usFirstFlashBlock * 1024;
        ulLen = (pState->usLastFlashBlock - pState->usFirstFlashBlock) * 1024;
        TRACE("Erasing entire writeable region. %dKB from 0x%x\n",
              ulLen / 1024, ulStartAddr);
    }
    else
    {
        //
        // Is the start address correctly aligned?
        //
        if(ulStartAddr & (1024 - 1))
        {
            TRACE("Address is not a multiple of 1024\n");
            return(DFU_ERR_INVALID_ADDR);
        }

        if(!ulLen || (ulLen & 3))
        {
            TRACE("Size is not a multiple of 4 or is zero.\n");
            return(DFU_ERR_INVALID_SIZE);
        }
    }

    //
    // First, make sure the target is idle and ready to receive a new
    // command.
    //
    eRetcode = DFUMakeDeviceIdle(pState);
    if(eRetcode != DFU_OK)
    {
        TRACE("Error %s (%d) attempting to get device into IDLE state.\n",
              LMDFUErrorStringGet(eRetcode), eRetcode);
        return(eRetcode);
    }

    //
    // Verify that the flash is indeed erased.
    //
    sCheck.ucCommand = STELLARIS_CMD_CHECK;
    sCheck.ucReserved = 0;
    sCheck.usStartAddr = (unsigned short)(ulStartAddr / 1024);
    sCheck.ulLength = ulLen;

    eRetcode = DFUDownloadTransfer(pState, true, (unsigned char *)&sCheck,
                                   sizeof(tDFUDownloadReadCheckHeader));

    //
    // Tell the caller whether the flash is blank or not.
    //
    return(eRetcode);
}

//****************************************************************************
//
//! Read back a section of the device flash.
//!
//! \param hHandle is the handle of the DFU device whose flash is to be read.
//! \param pcBuffer points to a buffer of at least \e ulImageLen bytes into
//! which the returned data will be written.  If \e bRaw is set to \b false,
//! the buffer must be 24 bytes longer than the actual data requested to
//! accommodate the DFU prefix and suffix which are added during the upload
//! process.
//! \param ulStartAddr is the address of the first byte of flash to be read.
//! \param ulImageLen is the number of bytes of flash to read.  If \e bRaw is
//! set to \b false, this length must be increased by 24 bytes to accommodate
//! the DFU prefix and suffix added during the upload process.
//! \param bRaw indicates whether the returned image will be wrapped in a
//! Stellaris-specific prefix and DFU standard suffix. If \e false, the wrappers
//! will be omitted and the raw data returned.  If \e true, the wrappers will
//! be included allowing the returned image to be written to a device later
//! by calling LMDFUDownload().
//! \param hwndNotify is the handle of a window to which periodic notifications
//! will be send indicating the progress of the operation.  If NULL, no status
//! notifications will be sent.
//!
//! This function reads back a section of the device flash into a buffer
//! supplied by the caller.  The data returned may be either raw data containing
//! no DFU control prefix and suffix or a DFU-wrapped image suitable for
//! later download via a call to LMDFUDownload().  If a DFU-wrapped image is
//! requested, the buffer pointed to by pcBuffer must be 24 bytes larger than
//! the number of bytes of device flash which is to be read.  For example, to
//! read 1024 bytes of flash wrapped as a DFU image, \e ulImageLen must be
//! set to (1024 + 24) and \e pcBuffer allocated accordingly.
//!
//! The function is synchronous and will not return until the operation is
//! complete.  To receive periodic status updates during the operation, a
//! window handle may be provided.  This window will receive WM_DFU_PROGRESS
//! messages during the erase operation allowing an application to update
//! its user interface accordingly.
//!
//! This function makes use of a Stellaris-specific extension to the
//! DFU protocol so is only available on Stellaris devices.
//!
//! \return Returns DFU_OK on success or DFU_ERR_INVALID_ADDR if the start
//! address is not on a flash block boundary or if the erase length is not a
//! multiple of the flash block size.  Appropriate return codes will be used
//! to indicate other errors.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUUpload(tLMDFUHandle hHandle, unsigned char *pcBuffer,
            unsigned long ulStartAddr, unsigned long ulImageLen,
            bool bRaw, HWND hwndNotify)
{
    tLMDFUDeviceState *pState;
    tDFUDownloadReadCheckHeader sRead;
    tDFUDownloadBinHeader sBin;
    tLMDFUErr eRetcode;

    TRACE("LMDFUUpload 0x%08x 0x%08x %d %s\n", hHandle, ulStartAddr,
        ulImageLen, bRaw ? "Raw" : "DFU");

    //
    // Bomb out on bad parameters.
    //
    if(!hHandle)
    {
        return(DFU_ERR_HANDLE);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    //
    // This operation is only supported if the target device supports the
    // Stellaris protocol.
    //
    if(!pState->bSupportsLMProtocol)
    {
        TRACE("Device does not support Stellaris protocol.\n");
        return(DFU_ERR_UNSUPPORTED);
    }

    //
    // Make sure we were passed a start address that is an integer multiple of
    // 1024.
    //
    if(ulStartAddr & (1024 - 1))
    {
        TRACE("Start address is not a multiple of 1024.\n");
        return(DFU_ERR_INVALID_ADDR);
    }

    //
    // Make sure that the buffer passed is large enough to hold the DFU prefix
    // and suffix if asked for a DFU-formatted image.
    //
    if(!bRaw && (ulImageLen <= (16 + sizeof(tDFUDownloadProgHeader))))
    {
        TRACE("Buffer too small for prefix and suffix.\n");
        return(DFU_ERR_INVALID_SIZE);
    }

    //
    // First, make sure the target is idle and ready to receive a new
    // command.
    //
    eRetcode = DFUMakeDeviceIdle(pState);
    if(eRetcode != DFU_OK)
    {
        TRACE("Error %s (%d) attempting to get device into IDLE state.\n",
              LMDFUErrorStringGet(eRetcode), eRetcode);
        return(eRetcode);
    }

    //
    // Now we can go ahead and initiate the upoad.  First we need to tell
    // the target what region of flash we want to read.  Note that we adjust
    // the image length to remove the DFU prefix and suffix.
    //
    sRead.ucCommand = STELLARIS_CMD_READ;
    sRead.ucReserved = 0;
    sRead.usStartAddr = (unsigned short)(ulStartAddr / 1024);
    sRead.ulLength = ulImageLen -
                     (bRaw ? 0 : 16 + sizeof(tDFUDownloadProgHeader));
    eRetcode = DFUDownloadTransfer(pState,  true, (unsigned char *)&sRead,
                                   sizeof(tDFUDownloadReadCheckHeader));

    //
    // Only do the next operation if the last one succeeded.
    //
    if(eRetcode == DFU_OK)
    {
        //
        // Now tell the target whether to download an image with a DFU header or
        // just raw binary.
        //
        memset(&sBin, 0, sizeof(tDFUDownloadBinHeader));
        sBin.ucCommand = STELLARIS_CMD_BIN;
        sBin.bBinary = bRaw ? 1 : 0;
        eRetcode = DFUDownloadTransfer(pState, true,  (unsigned char *)&sBin,
                                       sizeof(tDFUDownloadBinHeader));
        if(eRetcode == DFU_OK)
        {
            //
            // If no errors were reported, go ahead and perform the upload.
            //
            if(eRetcode == DFU_OK)
            {
                //
                // Actually read back the data from the device.  The number of
                // bytes we read is the image size passed minus the 16 bytes
                // for the DFU suffix if we have been asked for a DFU image.
                // The device automatically adds the 8 byte prefix structure
                // to the length of data we request via the STELLARIS_CMD_READ
                // sent earlier so we don't need to adjust for the prefix at
                // this point.
                //
                eRetcode = DFUUploadBlock(pState, pcBuffer,
                                          (int)ulImageLen - (bRaw ? 0 : 16),
                                          false, hwndNotify);

                //
                // Revert to the default DFU format for future uploads.
                //
                sBin.bBinary = 0;
                DFUDownloadTransfer(pState, true,  (unsigned char *)&sBin,
                                    sizeof(tDFUDownloadBinHeader));

                //
                // If the upload completed successfully and we have been asked
                // for a DFU-format image, we need to populate the DFU
                // suffix structure at the end of the downloaded data.
                //
                if((eRetcode == DFU_OK) && !bRaw)
                {
                    DFUSuffixPopulate(pState, pcBuffer, ulImageLen - 16);
                }
            }
        }
    }

    return(eRetcode);
}

//****************************************************************************
//
//! Queries the current status of the DFU device.
//!
//! \param hHandle is the handle of the DFU device which is to be queried.
//! \param pStatus points to storage which will be written with the DFU status
//! returned by the device.
//!
//! This call may be made to receive detailed error status from the
//! connected DFU device.
//!
//! \return Returns DFU_OK if the operation completed without any errors
//! reported, DFU_ERR_TIMEOUT if the control transaction timed out,
//! DFU_ERR_DISCONNECTED if the device was disconnected, DFU_ERR_STALL if the
//! device stalled endpoint 0 to indicate an error or DFU_ERR_UNKNOWN if
//! some other low level USB error occurred.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUStatusGet(tLMDFUHandle hHandle, tDFUStatus *pStatus)
{
    tLMDFUErr eRetcode;
    tDFUGetStatusResponse sStatus;
    tLMDFUDeviceState *pState;

    TRACE("LMDFUStatusGet 0x%08x\n", hHandle);

    //
    // Bomb out on bad parameters.
    //
    if(!hHandle)
    {
        return(DFU_ERR_HANDLE);
    }

    if(!pStatus)
    {
        return(DFU_ERR_INVALID_ADDR);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    //
    // Get the current device status structure.
    //
    eRetcode = DFUDeviceStatusGet(pState, &sStatus);

    //
    // Did the request succeed?
    //
    if(eRetcode == DFU_OK)
    {
        //
        // Yes - write the status to the caller's storage.
        //
        TRACE("Status is %d\n", sStatus.bStatus);
        *pStatus = (tDFUStatus)sStatus.bStatus;
    }
    else
    {
        TRACE("Error %d getting status\n", eRetcode);
    }

    //
    // Tell the caller how we got on.
    //
    return(eRetcode);
}

//****************************************************************************
//
//! Returns a string describing the passed error code.
//!
//! \param eError is the error code whose human-readable description is being
//! queried.
//!
//! This call is provided for debug purposes.  It maps the return code from an
//! LMDFU function into a human readable string suitable for, for example,
//! debug trace output.
//!
//! \return Returns a pointer to a string describing the passed return code.
//
//****************************************************************************
char * __stdcall
LMDFUErrorStringGet(tLMDFUErr eError)
{
    switch(eError)
    {
        case DFU_ERR_VERIFY_FAIL:
            return("DFU_ERR_VERIFY_FAIL");

        case DFU_ERR_CANT_VERIFY:
            return("DFU_ERR_CANT_VERIFY");

        case DFU_ERR_DNLOAD_FAIL:
            return("DFU_ERR_DNLOAD_FAIL");

        case DFU_ERR_STALL:
            return("DFU_ERR_STALL");

        case DFU_ERR_TIMEOUT:
            return("DFU_ERR_TIMEOUT");

        case DFU_ERR_DISCONNECTED:
            return("DFU_ERR_DISCONNECTED");

        case DFU_ERR_INVALID_SIZE:
            return("DFU_ERR_INVALID_SIZE");

        case DFU_ERR_INVALID_ADDR:
            return("DFU_ERR_INVALID_ADDR");

        case DFU_ERR_INVALID_FORMAT:
            return("DFU_ERR_INVALID_FORMAT");

        case DFU_ERR_UNSUPPORTED:
            return("DFU_ERR_UNSUPPORTED");

        case DFU_ERR_UNKNOWN:
            return("DFU_ERR_UNKNOWN");

        case DFU_ERR_MEMORY:
            return("DFU_ERR_MEMORY");

        case DFU_ERR_NOT_FOUND:
            return("DFU_ERR_NOT_FOUND");

        case DFU_ERR_HANDLE:
            return("DFU_ERR_HANDLE");

        case DFU_OK:
            return("DFU_OK");

        default:
            return("*** Invalid ***");
    }
}

//****************************************************************************
//
//! Instructs a runtime DFU device to switch into DFU mode.
//!
//! \param hHandle is the handle of the DFU device which is to be switched
//! into DFU mode (from runtime mode).
//!
//! This call may be used to signal a runtime DFU device that it is to switch
//! into DFU mode in preparation for a firmware upgrade.  On return, handle
//! hHandle will have been closed since the device will detach from the USB
//! bus as a result of receiving the signal.  Callers must reenumerate and
//! reopen the device after this function has been executed.
//!
//! \return Returns DFU_OK on success or other error codes on failure.
//
//****************************************************************************
tLMDFUErr __stdcall
LMDFUModeSwitch(tLMDFUHandle hHandle)
{
    tLMDFUDeviceState *pState;
    USHORT usCount;

    TRACE("LMDFUModeSwitch 0x%08x\n", hHandle);

    //
    // Bomb out on bad parameters.
    //
    if(!hHandle)
    {
        return(DFU_ERR_HANDLE);
    }

    //
    // Get our state information.
    //
    pState = (tLMDFUDeviceState *)hHandle;

    //
    // Send a DETACH message to the DFU device.  The device will detach
    // as a result of this request so we expect a bad return code here.
    //
    Endpoint0Transfer(pState->hUSB, (REQUEST_TRANSFER_OUT |
                      REQUEST_TYPE_CLASS | REQUEST_RECIPIENT_INTERFACE),
                      USBD_DFU_REQUEST_DETACH,
                      0, pState->usInterface, 0, (PUCHAR)NULL, &usCount);

    //
    // Close the device handle.
    //
    TerminateDevice(pState->hUSB);

    //
    // Free our state data.
    //
    free(pState);

    //
    // Tell the caller if all is well.
    //
    return(DFU_OK);
}
