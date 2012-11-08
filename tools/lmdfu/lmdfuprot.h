//*****************************************************************************
//
// lmdfuprot.h : Private header file containing definitions of Luminary-
//               specific DFU protocol extensions.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#ifndef __LMDFUPROT_H__
#define __LMDFUPROT_H__

//*****************************************************************************
//
// We wait up to 8 seconds for a control transaction to complete before
// timing out.
//
//*****************************************************************************
#define CONTROL_TIMEOUT 8000

//*****************************************************************************
//
// DFU class-specific request identifiers.
//
//*****************************************************************************
#define USBD_DFU_REQUEST_DETACH         0
#define USBD_DFU_REQUEST_DNLOAD         1
#define USBD_DFU_REQUEST_UPLOAD         2
#define USBD_DFU_REQUEST_GETSTATUS      3
#define USBD_DFU_REQUEST_CLRSTATUS      4
#define USBD_DFU_REQUEST_GETSTATE       5
#define USBD_DFU_REQUEST_ABORT          6

//*****************************************************************************
//
// Stellaris-specific request identifier.  This is used to determine whether
// the target device supports our DFU command protocol.  It is expected that
// a device not supporting our extensions will stall this request.  This
// request is only supported while the DFU device is in STATE_IDLE.
//
// An IN request containing the following parameters will result in the device
// sending back a tDFUQueryStellarisProtocol structure indicating that Stellaris
// extensions are supported.  The actual values in wValue and wIndex have no
// meaning other than to act as markers in the unlikely event that another
// DFU device also choses to use request ID 0x42 for some other purpose.
//
// wValue        - 0x23 (REQUEST_STELLARIS_VALUE)
// wIndex        - interface number
// wLength       - sizeof(tLMDFUQueryStellarisProtocol)
//
//*****************************************************************************
#define USBD_DFU_REQUEST_STELLARIS    0x42
#define REQUEST_STELLARIS_VALUE 0x23

#define LM_DFU_PROTOCOL_MARKER    0x4C4D
#define LM_DFU_PROTOCOL_VERSION_1 0x0001

#ifndef DEPRECATED
//
// Deprecated definitions included for backwards compatibility
//
#define USBD_DFU_REQUEST_LUMINARY USBD_DFU_REQUEST_STELLARIS
#define REQUEST_LUMINARY_VALUE REQUEST_STELLARIS_VALUE
#endif

//*****************************************************************************
//
// The states that the DFU device can be in.  These values are reported to
// the host in response to a USBD_DFU_REQUEST_GETSTATE request.
//
//*****************************************************************************
typedef enum
{
   STATE_APP_IDLE = 0,
   STATE_APP_DETACH,
   STATE_IDLE,
   STATE_DNLOAD_SYNC,
   STATE_DNBUSY,
   STATE_DNLOAD_IDLE,
   STATE_MANIFEST_SYNC,
   STATE_MANIFEST,
   STATE_MANIFEST_WAIT_RESET,
   STATE_UPLOAD_IDLE,
   STATE_ERROR
}
tDFUState;

//*****************************************************************************
//
// Masks and shifts related to the ulClassInfo field of tDFUDeviceInfo
//
//*****************************************************************************
#define STELLARIS_INFO_VER_M       0x70000000  // ClassInfo version mask
#define STELLARIS_INFO_VER_SHIFT   28
#define STELLARIS_INFO_VER_0       0x00000000  // ClassInfo version 0
#define STELLARIS_INFO_VER_1       0x10000000  // ClassInfo version 1
#define STELLARIS_INFO_CLASS_M     0x00FF0000  // Stellaris Device Class
#define STELLARIS_INFO_CLASS_SHIFT 16
#define STELLARIS_INFO_CLASS_DUSTDEVIL \
                                0x00030000    // DustDevil-class Device
#define STELLARIS_INFO_CLASS_TEMPEST   \
                                0x00040000    // Tempest-class Device
#define STELLARIS_INFO_MAJ_M       0x0000FF00  // Major revision mask
#define STELLARIS_INFO_MAJ_SHIFT   8
#define STELLARIS_INFO_MIN_M       0x000000FF  // Minor revision mask
#define STELLARIS_INFO_MIN_SHIFT   0

#define STELLARIS_PART_M           0x00FF0000
#define STELLARIS_PART_SHIFT       16

#define STELLARIS_ERASE_ALL   1
#define STELLARIS_ERASE_BLOCK 0

#ifndef DEPRECATED
//
// Deprecated definitions included for backwards compatibility
//
#define LUMINARY_INFO_VER_M             STELLARIS_INFO_VER_M
#define LUMINARY_INFO_VER_SHIFT         STELLARIS_INFO_VER_SHIFT
#define LUMINARY_INFO_VER_0             STELLARIS_INFO_VER_0
#define LUMINARY_INFO_VER_1             STELLARIS_INFO_VER_1
#define LUMINARY_INFO_CLASS_M           STELLARIS_INFO_CLASS_M
#define LUMINARY_INFO_CLASS_SHIFT       STELLARIS_INFO_CLASS_SHIFT
#define LUMINARY_INFO_CLASS_DUSTDEVIL   STELLARIS_INFO_CLASS_DUSTDEVIL
#define LUMINARY_INFO_MAJ_M             STELLARIS_INFO_MAJ_M
#define LUMINARY_INFO_MAJ_SHIFT         STELLARIS_INFO_MAJ_SHIFT
#define LUMINARY_INFO_MIN_M             STELLARIS_INFO_MIN_M
#define LUMINARY_INFO_MIN_SHIFT         STELLARIS_INFO_MIN_SHIFT

#define LUMINARY_PART_M                 STELLARIS_PART_M
#define LUMINARY_PART_SHIFT             STELLARIS_PART_SHIFT

#define LUMINARY_ERASE_ALL              STELLARIS_ERASE_ALL
#define LUMINARY_ERASE_BLOCK            STELLARIS_ERASE_BLOCK
#endif

//*****************************************************************************
//
// Stellaris-specific command identifiers.  These are passed to the device
// in standard DFU download requests.
//
//*****************************************************************************
#define STELLARIS_CMD_PROG  0x01
#define STELLARIS_CMD_READ  0x02
#define STELLARIS_CMD_CHECK 0x03
#define STELLARIS_CMD_ERASE 0x04
#define STELLARIS_CMD_INFO  0x05
#define STELLARIS_CMD_BIN   0x06
#define STELLARIS_CMD_RESET 0x07

#ifndef DEPRECATED
//
// Deprecated definitions included for backwards compatibility
//
#define LUMINARY_CMD_PROG   STELLARIS_CMD_PROG
#define LUMINARY_CMD_READ   STELLARIS_CMD_READ
#define LUMINARY_CMD_CHECK  STELLARIS_CMD_CHECK
#define LUMINARY_CMD_ERASE  STELLARIS_CMD_ERASE
#define LUMINARY_CMD_INFO   STELLARIS_CMD_INFO
#define LUMINARY_CMD_BIN    STELLARIS_CMD_BIN
#define LUMINARY_CMD_RESET  STELLARIS_CMD_RESET
#endif

//*****************************************************************************
//
// The structure sent to the host when a valid USBD_DFU_REQUEST_STELLARIS is
// received while the DFU device is in idle state.
//
//*****************************************************************************
#pragma pack(1)
typedef struct
{
    unsigned short usMarker;        // LM_DFU_PROTOCOL_MARKER
    unsigned short usVersion;       // LM_DFU_PROTOCOL_VERSION_1
}
tDFUQueryStellarisProtocol;

#ifndef DEPRECATED
//
// Deprecated definition included for backwards compatibility
//
#define tDFUQueryLuminaryProtocol tDFUQueryStellarisProtocol
#endif

//****************************************************************************
//
// Generic download command header.
//
//***************************************************************************
typedef struct
{
    unsigned char ucCommand;     // Command identifier.
    unsigned char ucData[7];     // Command-specific data elements.
}
tDFUDownloadHeader;

#define STELLARIS_CMD_LEN (sizeof(tDFUDownloadHeader))

#ifndef DEPRECATED
//
// Deprecated definition included for backwards compatibility
//
#define LUMINARY_CMD_LEN STELLARIS_CMD_LEN
#endif

//*****************************************************************************
//
// Header for the STELLARIS_CMD_PROG command.
//
// This command is used to program a section of the flash with the binary data
// which immediately follows the header. The start address of the data is
// expressed as a 1KB block number so 0 would represent the bottom of flash
// (which, incidentally, the USB boot loader will not let you program) and 0x10
// would represent address 16KB or 16384 (0x4000).  The usLength field contains
// the total number of bytes of data in the following programming operation.
// The DFU device will not look for any command header on following
// USBD_DFU_REQUEST_DNLOAD requests until the operation is completed or
// aborted.
//
// By using this protocol, the STELLARIS_CMD_PROG command header may be used as
// a simple header on the binary files to be sent to the DFU device for
// programming.  If we enforce the requirement that the STELLARIS_CMD_PROG
// header is applied to each USBD_DFU_REQUEST_DNLOAD (one per block), this
// means that the host-side DFU application must be aware of the underlying
// protocol and insert these headers dynamically during programming operations.
// This could be handled by post processing the binary to insert the headers at
// the appropriate points but this would then tie the binary structure to the
// chosen transfer size and break the operation if the transfer size were to
// change in the future.
//
//***************************************************************************
typedef struct
{
    unsigned char  ucCommand;   // STELLARIS_CMD_PROG
    unsigned char  ucReserved;  // Reserved - set to 0x00.
    unsigned short usStartAddr; // Block start address / 1024
    unsigned long  ulLength;    // Total length, in bytes, of following data
                                // for the complete download operation.
}
tDFUDownloadProgHeader;

//****************************************************************************
//
// Header for the STELLARIS_CMD_READ and STELLARIS_CMD_CHECK commands.
//
// The STELLARIS_CMD_READ command may be used to set the address range whose
// content will be returned on subsequent USBD_DFU_REQUEST_UPLOAD requests from
// the host.
//
// To read back a the contents of a region of flash, the host should send
// USBD_DFU_REQUEST_DNLOAD with ucCommand STELLARIS_CMD_READ, usStartAddr set
// to the 1KB block start address and ulLength set to the number of bytes to
// read. The host should then send one or more USBD_DFU_REQUEST_UPLOAD requests
// to receive the current flash contents from the configured addresses.  Data
// returned in this way contains no header or suffix.
//
// To check that a region of flash is erased, the STELLARIS_CMD_CHECK command
// should be sent with usStartAddr and ulLength set to describe the region to
// check. The host should then send a USBD_DFU_REQUEST_GETSTATUS.  If the erase
// check was successful, the returned bStatus value will be STATUS_OK,
// otherwise it will be STATUS_ERR_CHECK_ERASED. Note that ulLength passed must
// be a multiple of 4.  If this is not the case, the value will be truncated
// before the check is performed.
//
//****************************************************************************
typedef struct
{
    unsigned char  ucCommand;    // STELLARIS_CMD_READ or STELLARIS_CMD_CHECK
    unsigned char  ucReserved;   // Reserved - write to 0
    unsigned short usStartAddr;  // Block start address / 1024
    unsigned long  ulLength;     // The number of bytes of data to read back or
                                 // check.
}
tDFUDownloadReadCheckHeader;

//****************************************************************************
//
// Header for the STELLARIS_CMD_ERASE command.
//
// This command may be used to erase a number of flash blocks.  The address of
// the first block to be erased is passed in usStartAddr with usNumBlocks
// containing the number of blocks to be erased from this address.  The block
// size of the device may be determined using the STELLARIS_CMD_INFO command.
//
//*****************************************************************************
typedef struct
{
    unsigned char  ucCommand;      // STELLARIS_CMD_ERASE
    unsigned char  ucReserved;     // Reserved - set to 0.
    unsigned short usStartAddr;    // Block start address / 1024
    unsigned short usNumBlocks;    // The number of blocks to erase.
    unsigned char  ucReserved2[2]; // Reserved - set to 0.
}
tDFUDownloadEraseHeader;

//****************************************************************************
//
// Header for the STELLARIS_CMD_INFO command.
//
// This command may be used to query information about the connected device.
// After sending the command, the information is returned on the next
// USBD_DFU_REQUEST_UPLOAD request.
//
//****************************************************************************
typedef struct
{
    unsigned char ucCommand;     // STELLARIS_CMD_INFO
    unsigned char ucReserved[7]; // Reserved - set to 0.
}
tDFUDownloadInfoHeader;

//****************************************************************************
//
// Header for the STELLARIS_CMD_BIN command.
//
// This command may be used to tell the device whether or not to include the
// STELLARIS_CMD_PROG header before downloaded data.  By default, this is the
// behaviour used.  The DFU spec states that it must be possible to
// download an uploaded image back to the target and the header contains
// information on the image length and start address but in some cases it is
// convenient to be able to upload an image without this header.
//
//****************************************************************************
typedef struct
{
    unsigned char ucCommand;     // STELLARIS_CMD_BIN
    unsigned bBinary;            // Zero to include the DFU header, non-zero to
                                 // omit it and send binary data.
    unsigned char ucReserved[6]; // Reserved - set to 0.
}
tDFUDownloadBinHeader;

//*****************************************************************************
//
// Payload returned in response to the STELLARIS_CMD_INFO command.
//
// This is structure is returned in response to the first
// USBD_DFU_REQUEST_UPLOAD request following a STELLARIS_CMD_INFO command.
//
//*****************************************************************************
typedef struct
{
    unsigned short usFlashBlockSize;  // The size of a flash block in bytes.
    unsigned short usNumFlashBlocks;  // The number of blocks of flash in the
                                      // device.  Total flash size is
                                      // usNumFlashBlocks * usFlashBlockSize.
    unsigned long ulPartInfo;         // Information on the part number,
                                      // family, version and package as
                                      // read from SYSCTL register DID1.
    unsigned long ulClassInfo;        // Information on the part class as read
                                      // from SYSCTL register DID0.
    unsigned long ulFlashTop;         // Address 1 byte above the highest
                                      // location the boot loader can access.
    unsigned long ulAppStartAddr;     // Lowest address the boot loader can
                                      // write or erase.
}
tDFUDeviceInfo;

//*****************************************************************************
//
// Structure sent to the host in response to USBD_DFU_REQUEST_GETSTATUS.
//
//*****************************************************************************
typedef struct
{
    unsigned char bStatus;
    unsigned char bwPollTimeout[3];
    unsigned char bState;
    unsigned char iString;
}
tDFUGetStatusResponse;

#pragma pack()

//*****************************************************************************
//
// LIBUSB return codes that are missing from the Windows version of errno.h.
//
//*****************************************************************************
#define ETIMEDOUT 116

#endif // __LMDFUPROT_H__
