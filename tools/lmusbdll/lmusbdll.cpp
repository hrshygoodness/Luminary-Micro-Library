// lmusbdll.cpp : A thin layer over WinUSB allowing device open/close/
//   read and write functionality.
//
// The WinUSB code in this example is based on information provided in
// Microsoft's white paper, "How to Use WinUSB to Communicate with a USB
// Device", WinUsb_HowTo.doc, available from URL:
//
// http://download.microsoft.com/download/9/c/5/9c5b2167-8017-4bae-9fde-
// d599bac8184a/WinUsb_HowTo.doc
//
#include "stdafx.h"
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <strsafe.h>
#include <initguid.h>
#include "winusb.h"
#include "usb100.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "resource.h"        // main symbols


// CusbdllApp
// See usbdll.cpp for the implementation of this class
//

class ClmusbdllApp : public CWinApp
{
public:
    ClmusbdllApp();

// Overrides
public:
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};

// ClmusbdllApp

BEGIN_MESSAGE_MAP(ClmusbdllApp, CWinApp)
END_MESSAGE_MAP()


// CusbdllApp construction

ClmusbdllApp::ClmusbdllApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}


// The one and only CusbdllApp object

ClmusbdllApp theApp;


// CusbdllApp initialization

BOOL ClmusbdllApp::InitInstance()
{
    CWinApp::InitInstance();

    return TRUE;
}

//****************************************************************************
//
// Buffer size definitions.
//
//****************************************************************************
#define MAX_DEVPATH_LENGTH 256

//****************************************************************************
//
// Flag indicating that a blocking read should be performed.
//
//****************************************************************************
#define LMUSB_WAIT_FOREVER 0xFFFFFFFF

//****************************************************************************
//
// Structure containing handles and information required to communicate with
// the USB bulk device.
//
//****************************************************************************
typedef struct
{
    HANDLE deviceHandle;
    WINUSB_INTERFACE_HANDLE winUSBHandle;
    UCHAR deviceSpeed;
    UCHAR bulkInPipe;
    UCHAR bulkOutPipe;
    HANDLE hReadEvent;
} tDeviceInfoWinUSB;

typedef void *LMUSB_HANDLE;

//****************************************************************************
//
// Returns the device path associated with a provided interface GUID.
//
// \param dwIndex is the zero-based index of the device whose path is to be
// found.
// \param InterfaceGuid is a pointer to the GUID for the interface that
// we are looking for.  Assuming you installed the device using the sample
// INF file, this will be the GUID provided in the Dev_AddReg section.
// \param pcDevicePath is a pointer to a buffer into which will be written
// the path to the device if the function is successful.
// \param BufLen is the size of the buffer pointed to by \f pcDevicePath.
//
// Given an interface GUID, this function determines the path that is
// necessary to open a file handle on the USB device we are interested in
// talking to. It returns the path to the first device which is present in
// the system and which offers the required interface.
//
// \return Returns one of the following Windows system error codes.
//     ERROR_SUCCESS if the operation completed successfully
//     ERROR_DEV_NOT_EXIST if the interface is not found on the system. This
//     implies that the driver for the USB device has not been installed.
//     ERROR_DEVICE_NOT_CONNECTED if the interface has been installed but no
//     device offering it is presently available. This typically indicates
//     that the USB device is not currently connected.
//     ERROR_NOT_ENOUGH_MEMORY if the function fails to allocate any required
//     buffers.
//     ERROR_INSUFFICIENT_BUFFER if the buffer passed is too small to hold
//     the device path.
//****************************************************************************
static DWORD GetDevicePath(DWORD dwIndex, LPGUID InterfaceGuid,
                    PCHAR  pcDevicePath,
                    size_t BufLen)
{
    BOOL bResult = FALSE;
    HDEVINFO deviceInfo;
    SP_DEVICE_INTERFACE_DATA interfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = NULL;
    ULONG length;
    ULONG requiredLength=0;
    HRESULT hr;

    //
    // Get a handle to the device information set containing information on
    // the interface GUID supplied on this PC.
    //
    deviceInfo = SetupDiGetClassDevs(InterfaceGuid,
                                     NULL, NULL,
                                     DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if(deviceInfo == INVALID_HANDLE_VALUE)
    {
        //
        // No device offering the required interface is present. Has the
        // interface been installed? Ask for the information set for all
        // devices and not just those that are presently installed.
        //
        deviceInfo = SetupDiGetClassDevs(InterfaceGuid,
                                         NULL, NULL,
                                         DIGCF_DEVICEINTERFACE);
        if(deviceInfo == INVALID_HANDLE_VALUE)
        {
            return(ERROR_DEV_NOT_EXIST);
        }
        else
        {
            SetupDiDestroyDeviceInfoList(deviceInfo);
            return(ERROR_DEVICE_NOT_CONNECTED);
        }
    }

    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    //
    // SetupDiGetClassDevs returned us a valid information set handle so we
    // now query this set to find the find the first device offering the
    // interface whose GUID was supplied.
    //
    bResult = SetupDiEnumDeviceInterfaces(deviceInfo,
                                          NULL,
                                          InterfaceGuid,
                                          dwIndex,
                                          &interfaceData);
    if(bResult == FALSE)
    {
        //
        // We failed to find the first matching device so tell the caller
        // that no suitable device is connected.
        //
        return(ERROR_DEVICE_NOT_CONNECTED);
    }

    //
    // Now that we have the interface information, we need to query details
    // to retrieve the device path. First determine how much space we need to
    // hold the detail information.
    //
    SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                    &interfaceData,
                                    NULL, 0,
                                    &requiredLength,
                                    NULL);

    //
    // Allocate a buffer to hold the interface details.
    //
    detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LMEM_FIXED,
                   requiredLength);

    if(NULL == detailData)
    {
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    length = requiredLength;

    //
    // Now call once again to retrieve the actual interface detail information.
    //
    bResult = SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                              &interfaceData,
                                              detailData,
                                              length,
                                              &requiredLength,
                                              NULL);

    if(FALSE == bResult)
    {
        SetupDiDestroyDeviceInfoList(deviceInfo);
        LocalFree(detailData);
        return(GetLastError());
    }

    //
    // Copy the device path string from the interface details structure into
    // the caller's buffer.
    //
    hr = StringCchCopy((LPTSTR)pcDevicePath,
                        BufLen,
                        (LPCTSTR)detailData->DevicePath);
    if(FAILED(hr))
    {
        //
        // We failed to copy the device path string!
        //
        SetupDiDestroyDeviceInfoList(deviceInfo);
        LocalFree(detailData);
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    //
    // Clean up and free locally allocated resources.
    //
    SetupDiDestroyDeviceInfoList(deviceInfo);
    LocalFree(detailData);

    return(ERROR_SUCCESS);
}

//****************************************************************************
//
// Opens a given instance of the USB device and returns a file handle.
//
// \param dwIndex is the zero-based index of the device that is to be opened.
// \param lpGUID is a pointer to the GUID of the interface that is to
// be opened.
//
// This function determines whether or not the required USB device is
// available and, if so, creates a file allowing access to it. The file
// handle is returned on success.
//
// \return Returns a valid file handle on success of INVALID_HANDLE_VALUE
// on failure. In cases of failure, GetLastError() can be called to determine
// the cause.
//
//****************************************************************************
static HANDLE OpenDeviceByIndex(DWORD dwIndex, LPGUID lpGUID)
{
    HANDLE hDev = NULL;
    char devicePath[MAX_DEVPATH_LENGTH];
    BOOL retVal;

    //
    // Get the path needed to open a file handle on our USB device.
    //
    retVal = GetDevicePath(dwIndex, lpGUID, devicePath, sizeof(devicePath));
    if(retVal != ERROR_SUCCESS)
    {
        SetLastError(retVal);
        return(INVALID_HANDLE_VALUE);
    }

    //
    // Open the file we will use to communicate with the device.
    //
    hDev = CreateFile((LPCTSTR)devicePath,
                      GENERIC_WRITE | GENERIC_READ,
                      FILE_SHARE_WRITE | FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                      NULL);

    return(hDev);
}

//****************************************************************************
//
// Opens the first instance of a given USB device and returns a file handle.
//
// \param lpGUID is a pointer to the GUID of the interface that is to
// be opened.
//
// This function determines whether or not the required USB device is
// available and, if so, creates a file allowing access to it. The file
// handle is returned on success.
//
// \return Returns a valid file handle on success of INVALID_HANDLE_VALUE
// on failure. In cases of failure, GetLastError() can be called to determine
// the cause.
//
//****************************************************************************
static HANDLE OpenDevice(LPGUID lpGUID)
{
    return(OpenDeviceByIndex(0, lpGUID));
}

//****************************************************************************
//
// Determines that the required USB device is present, opens it and gathers
// required information to allow us to read and write it.
//
// \param usVID is the vendor ID of the device to be initialized.
// \param usPID is the product ID of the device to be intialized.
// \param lpGUID is the GUID of the device interface that is to be opened.
// \param dwIndex is the zero-based index of the device to open.
// \param bOpenDataEndpoints is \e TRUE to open endpoints for bulk transfer
// of data to and from the host or \e FALSE to open the device without
// attempting to open any additional endpoints.
// \param pbDriverInstalled is written to \e TRUE if the device driver for the
// required class is installed or \e FALSE otherwise.
//
// This function is called to initialize the USB device and perform one-off
// setup required to allocate handles allowing the application to read and
// write the device endpoints.  It offers a superset of the function provided
// by InitializeDevice(), allowing a caller to specify an index to differentiate
// between multiple devices of the same type and also offering the ability to
// open a device without opening endpoint handles for bulk data transfer.
//
// \return Returns a valid handle on success or NULL on failure. In failing
// cases, GetLastError() can be called to determine the cause.
//
//****************************************************************************
extern "C" LMUSB_HANDLE PASCAL EXPORT
InitializeDeviceByIndex(unsigned short usVID,
                        unsigned short usPID,
                        LPGUID lpGUID,
                        DWORD dwIndex,
                        BOOL bOpenDataEndpoints,
                        BOOL *pbDriverInstalled)
{
    BOOL bResult;
    WINUSB_INTERFACE_HANDLE usbHandle;
    USB_INTERFACE_DESCRIPTOR ifaceDescriptor;
    WINUSB_PIPE_INFORMATION pipeInfo;
    UCHAR speed;
    ULONG length;
    DWORD dwError;
    int i;
    tDeviceInfoWinUSB *psDevInfo;

    //
    // Check for NULL pointer parameters.
    //
    if(!lpGUID || !pbDriverInstalled)
    {
        return(NULL);
    }

    //
    // Allocate a new device info structure.
    //
    psDevInfo = (tDeviceInfoWinUSB *)LocalAlloc(LPTR, sizeof(tDeviceInfoWinUSB));
    if(!psDevInfo)
    {
        return(NULL);
    }

    //
    // Determine whether the USB device is present and, if so, generate a file
    // handle to allow access to it.
    //
    psDevInfo->deviceHandle = OpenDeviceByIndex(dwIndex, lpGUID);
    if(psDevInfo->deviceHandle == INVALID_HANDLE_VALUE)
    {
        //
        // We were unable to access the device - is that because the device isn't
        // connected or has the driver not been installed?
        //
        dwError = GetLastError();
        *pbDriverInstalled = (dwError == ERROR_DEV_NOT_EXIST) ? FALSE : TRUE;

        //
        // Free our instance data.
        //
        LocalFree((HLOCAL)psDevInfo);

        //
        // Return an error to the caller.
        //
        return(NULL);
    }

    //
    // The device is opened so we now initialize the WinUSB layer passing it
    // the device handle.
    //
    bResult = WinUsb_Initialize(psDevInfo->deviceHandle, &usbHandle);

    if(bResult)
    {
        //
        // If we managed to initialize the WinUSB layer, we now query the
        // device descriptor to determine the speed of the device.
        //
        psDevInfo->winUSBHandle = usbHandle;
        length = sizeof(UCHAR);
        bResult = WinUsb_QueryDeviceInformation(psDevInfo->winUSBHandle,
                                                DEVICE_SPEED,
                                                &length,
                                                &speed);
    }

    //
    // The device opened correctly.  Do we need to also open pipes to allow us
    // to send and receive data via the bulk endpoints?
    //
    if(bOpenDataEndpoints)
    {
        //
        // Yes - we do need to open the endpoints for data transfer.
        //
        if(bResult)
        {
            //
            // If all is well, now query the interface descriptor. We ask for the
            // first interface only since, in the case of the generic bulk device,
            // this is all that is available.
            //
            psDevInfo->deviceSpeed = speed;
            bResult = WinUsb_QueryInterfaceSettings(psDevInfo->winUSBHandle,
                                                    0,
                                                    &ifaceDescriptor);
        }

        if(bResult)
        {
            //
            // We got the interface descriptor so now we enumerate the endpoints
            // to find the two we require - one bulk IN endpoint and one bulk OUT
            // endpoint.
            //
            for(i=0;i<ifaceDescriptor.bNumEndpoints;i++)
            {
                bResult = WinUsb_QueryPipe(psDevInfo->winUSBHandle, 0, (UCHAR) i,
                                          &pipeInfo);

                if((pipeInfo.PipeType == UsbdPipeTypeBulk) &&
                      USB_ENDPOINT_DIRECTION_IN(pipeInfo.PipeId))
                {
                    psDevInfo->bulkInPipe = pipeInfo.PipeId;
                }
                else if((pipeInfo.PipeType == UsbdPipeTypeBulk) &&
                      USB_ENDPOINT_DIRECTION_OUT(pipeInfo.PipeId))
                {
                    psDevInfo->bulkOutPipe = pipeInfo.PipeId;
                }
                else
                {
                    //
                    // Hmm... we found an endpoint that we didn't expect to see
                    // on this interface. This tends to imply that there is a
                    // mismatch between the device configuration and this
                    // application so we will fail the call after setting an
                    // appropriate error code for the caller to query.
                    //
                    SetLastError(ERROR_NOT_SAME_DEVICE);
                    bResult = FALSE;
                    break;
                }
            }
        }

        //
        // Did all go well so far?
        //
        if(bResult)
        {
            //
            // All is well - create the signal event we need.
            //
            psDevInfo->hReadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            //
            // If we created the event successfully, return a good handle.
            //
            if(psDevInfo->hReadEvent)
            {
                //
                // Everything is good.  Set the return code flag to indicate
                // this.
                //
                bResult = true;
            }
        }
    }
    else
    {
        //
        // Data transfer endpoints do not need to be opened so merely mark
        // this in the device info structure.
        //
        psDevInfo->bulkOutPipe = 0;
        psDevInfo->bulkInPipe = 0;
        psDevInfo->hReadEvent = NULL;
    }

    //
    // Did all go well?
    //
    if(bResult)
    {
        //
        // All is well - return the instance data pointer as a handle to the
        // caller.
        //
        return((LMUSB_HANDLE)psDevInfo);
    }
    else
    {
        //
        // If we drop through to here, something went wrong so free the
        // instance structure and return NULL.
        //
        LocalFree((HLOCAL)psDevInfo);
        return(NULL);
    }
}

//****************************************************************************
//
// Determines that the required USB device is present, opens it and gathers
// required information to allow us to read and write it.
//
// \param usVID is the vendor ID of the device to be initialized.
// \param usPID is the product ID of the device to be intialized.
// \param lpGUID is the GUID of the device interface that is to be opened.
// \param pbDriverInstalled is written to \e TRUE if the device driver for the
// required class is installed or \e FALSE otherwise.
//
// This function is called to initialize the USB device and perform one-off
// setup required to allocate handles allowing the application to read and
// write the device endpoints.
//
// InitializeDevice() always opens the first instance of a device and assumes
// that the caller will always want to open handles allowing communication via
// only IN and OUT bulk endpoints. To open an instance of a device other than
// the first or to open a device without opening handles on any endpoints other
// than the control endpoint, InitializeDeviceByIndex() may be used instead.
//
// \return Returns a valid handle on success or NULL on failure. In failing
// cases, GetLastError() can be called to determine the cause.
//
//****************************************************************************
extern "C" LMUSB_HANDLE PASCAL EXPORT
InitializeDevice(unsigned short usVID,
                 unsigned short usPID,
                 LPGUID lpGUID,
                 BOOL *pbDriverInstalled)
{
    //
    // Call our more capable cousin to open the first instance of the device
    // and set up for data transfer via bulk IN and OUT endpoints.
    //
    return(InitializeDeviceByIndex(usVID, usPID, lpGUID, 0, true,
                                   pbDriverInstalled));
}

//****************************************************************************
//
// Performs a control transfer on endpoint 0.
//
// \param hUSB is the device handle as returned from InitializeDevice().
// \param ucRequestType is the value to be placed in the bmRequestType field
// of the setup data as defined in table 9-2 of the USB 2.0 specification.  The
// value is an ORed combination of the REQUEST_xxx flags found in lmusbdll.h.
// \param ucRequest is the specific request identifier which will be placed in
// the bRequest field of the setup data for the transaction.
// \param usValue is the request-specific value placed into the wValue field of
// the setup data for the transaction.
// \param usIndex is the value placed into the wIndex field of the setup data
// for the transaction.
// \param usLength is the number of bytes of data to transfer from or to the
// caller-supplied buffer during the transaction.  This value is used to set
// the wLength field in the setup data for the transaction.
// \param pucBuffer is a pointer to a buffer containing at least usLength bytes.
// If ucRequestType contains REQUEST_TRANSFER_IN, usLength bytes of data will
// be transfered from the device and written into this buffer. If ucRequestType
// contains REQUEST_TRANSFER_OUT, usLength bytes of data will be transfered
// from this buffer to the device.
// \param pusCount will be written with the number of bytes actually written to
// or read from the device.
//
// This function performs a control transfer to the device.  This transaction
// may transfer data to or from the device depending upon the value of the
// ucRequestType parameter.
//
// \return Returns \e TRUE on success or \e FALSE on failure.  If \e FALSE is
// returned, GetLastError() may be called to determine the cause of the failure.
//
//****************************************************************************
extern "C" BOOL PASCAL EXPORT
Endpoint0Transfer(LMUSB_HANDLE hUSB, UCHAR ucRequestType, UCHAR ucRequest,
                  USHORT usValue, USHORT usIndex, USHORT usLength,
                  PUCHAR pucBuffer, PUSHORT pusCount)
{
    tDeviceInfoWinUSB *psDevInfo = (tDeviceInfoWinUSB *)hUSB;
    WINUSB_SETUP_PACKET sSetup;
    ULONG ulCount;
    ULONG ulError;
    BOOL bRetcode;

    //
    // Check for invalid parameters.  We require a buffer pointer if the
    // transfer length is non-zero.
    //
    if(!hUSB || (usLength && !pucBuffer))
    {
        return(0);
    }

    //
    // Fill in the setup data structure.
    //
    sSetup.RequestType = ucRequestType;
    sSetup.Request = ucRequest;
    sSetup.Value = usValue;
    sSetup.Index = usIndex;
    sSetup.Length = usLength;

    //
    // Issue the control transaction.
    //
    bRetcode = WinUsb_ControlTransfer(psDevInfo->winUSBHandle,
                                      sSetup,
                                      pucBuffer,
                                      (ULONG)usLength,
                                      &ulCount,
                                      NULL);

    //
    // Return either the number of bytes written/read or 0 if there was an
    // error.
    //
    if(bRetcode)
    {
        *pusCount = (USHORT)ulCount;
    }
    else
    {
        ulError = GetLastError();
        *pusCount = 0;
    }
    return(bRetcode);
}

//****************************************************************************
//
// Cleans up and free resources associated with the USB device communication
// prior to exiting the application.
//
// \param hUSB is the device handle as returned from InitializeDevice().
//
// This function should be called prior to exiting the application to free
// the resources allocated during InitializeDevice().
//
// \return Returns \e TRUE on success or \e FALSE on failure.
//
//****************************************************************************
extern "C" BOOL PASCAL EXPORT TerminateDevice(LMUSB_HANDLE hUSB)
{
    BOOL bRetcode;
    BOOL bRetcode2;
    tDeviceInfoWinUSB *psDevInfo = (tDeviceInfoWinUSB *)hUSB;

    //
    // Check for a bad handle.
    //
    if(!hUSB)
    {
        return(false);
    }

    //
    // Free WinUSB and Windows resources.
    //
    bRetcode = WinUsb_Free(psDevInfo->winUSBHandle);
    bRetcode2 = CloseHandle(psDevInfo->deviceHandle);

    //
    // Free the device instance structure.
    //
    LocalFree((HLOCAL)psDevInfo);

    //
    // Did all go well?
    //
    if(bRetcode & bRetcode2)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


//****************************************************************************
//
// Writes a buffer of data to the USB device via the bulk OUT endpoint.
//
// \param hUSB is the device handle as returned from InitializeDevice().
// \param pcBuffer points to the first byte of data to send.
// \param ulSize contains the number of bytes of data to send.
// \param pulWritten is a pointer which will be written with the number of
// bytes of data actually written to the device.
//
// This function is used to send data to the USB device via its bulk OUT endpoint.
//
// \return Returns \e TRUE on success or \e FALSE on failure.
//
//****************************************************************************
extern "C" BOOL PASCAL EXPORT
WriteUSBPacket(LMUSB_HANDLE hUSB, unsigned char *pcBuffer,
               unsigned long ulSize,
               unsigned long *pulWritten)
{
    BOOL bResult;
    tDeviceInfoWinUSB *psDevInfo = (tDeviceInfoWinUSB *)hUSB;

    //
    // Check for bad parameters.
    //
    if(!hUSB || !pcBuffer || !pulWritten || !ulSize)
    {
        return(false);
    }

    //
    // Ask WinUSB to write the data for us.
    //
    bResult = WinUsb_WritePipe(psDevInfo->winUSBHandle,
                               psDevInfo->bulkOutPipe,
                               pcBuffer,
                               ulSize,
                               pulWritten,
                               NULL);

    return(bResult);
}

//****************************************************************************
//
// Reads data from the USB device via the bulk IN endpoint.
//
// \param hUSB is the device handle as returned from InitializeDevice().
// \param pcBuffer points to a buffer into which the data from the device will
// be written.
// \param ulSize contains the number of bytes that are requested from the
// device.
// \param pulRead is a pointer which will be written with the number of
// bytes of data actually read from the device.
// \param ulTimeoutmS is the number of milliseconds to wait for the
// data before returning an error. To block indefinitely, set this parameter
// to INFINITE.
// \param hBreak is the handle of an event that may be used to break out of
// the read operation.  If this is a valid event handle and the event is
// signalled, the read operation will return immediately.  Set this to
// NULL if the additional break condition is not required.
//
// This function is used to receive data from the USB device via its bulk IN
// endpoint.
//
// \return Returns \e ERROR_SUCCESS on success, WAIT_TIMEOUT if the data as
// not received within ulTimeoutmS milliseconds, WAIT_OBJECT_0 if the break
// event was signalled or other Windows system error codes on failure.
//
//****************************************************************************
extern "C" DWORD PASCAL EXPORT
ReadUSBPacket(LMUSB_HANDLE hUSB, unsigned char *pcBuffer, unsigned long ulSize,
              unsigned long *pulRead, unsigned long ulTimeoutmS, HANDLE hBreak)
{
    BOOL bResult;
    DWORD dwError;
    OVERLAPPED sOverlapped;
    HANDLE phSignals[2];
    tDeviceInfoWinUSB *psDevInfo = (tDeviceInfoWinUSB *)hUSB;

    //
    // Check for bad parameters.
    //
    if(!hUSB || !pcBuffer || !pulRead || !ulSize)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Tell WinUSB how to signal us when reads are completed (if blocking)
    //
    sOverlapped.hEvent = psDevInfo->hReadEvent;
    sOverlapped.Offset = 0;
    sOverlapped.OffsetHigh = 0;

    //
    // Perform the read.
    //
    bResult = WinUsb_ReadPipe(psDevInfo->winUSBHandle,
                              psDevInfo->bulkInPipe,
                              pcBuffer,
                              ulSize,
                              pulRead,
                              &sOverlapped);

    //
    // A good return code indicates success regardless of whether we performed
    // a blocking or non-blocking read.
    //
    if(bResult)
    {
        dwError =  ERROR_SUCCESS;
    }
    else
    {
        //
        // An error occurred or the read will complete asynchronously.
        // Which is it?
        //
        dwError = GetLastError();

        //
        // Check for error cases other than the one we expect.
        //
        if(dwError == ERROR_IO_PENDING)
        {
            //
            // The IO is pending so wait for it to complete or for a
            // timeout to occur.
            //
            phSignals[0] = psDevInfo->hReadEvent;
            phSignals[1] = hBreak;
            dwError = WaitForMultipleObjects(hBreak ? 2 : 1, phSignals, FALSE,
                                             ulTimeoutmS);

            //
            // At this stage, one of three things could have occurred.
            // Either we read a packet or we timed out or the break
            // signal was detected.  Which was it?
            //
            if(dwError == WAIT_OBJECT_0)
            {
                //
                // The overlapped IO request completed so check to see how
                // many bytes we got.
                //
                bResult = GetOverlappedResult(psDevInfo->deviceHandle,
                                              &sOverlapped,
                                              pulRead, FALSE);
                if(bResult)
                {
                    dwError = ERROR_SUCCESS;
                }
                else
                {
                    //
                    // Something went wrong.  Return the Windows error code.
                    //
                    dwError = GetLastError();
                }
            }
            else
            {
                //
                // Something went wrong - abort the transfer
                //
                WinUsb_AbortPipe(psDevInfo->winUSBHandle,
                                 psDevInfo->bulkInPipe);

                //
                // Was the break event signalled?
                //
                if(dwError  == (WAIT_OBJECT_0 + 1))
                {
                    //
                    // The break event was signalled - abort the read and return.
                    //
                    dwError = WAIT_OBJECT_0;
                }
            }
        }
    }

    //
    // Pass the result back to the caller.
    //
    return(dwError);
}
