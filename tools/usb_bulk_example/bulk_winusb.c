//
// Implementation of the bulk_usb interface using Microsoft's WinUSB API.
//

#ifdef USE_WINUSB

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <strsafe.h>
#include <initguid.h>
#include "bulk_usb.h"
#include "luminary_guids.h"
#include "winusb.h"
#include "usb100.h"

//****************************************************************************
//
// Buffer size definitions.
//
//****************************************************************************
#define MAX_DEVPATH_LENGTH 256

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

} tDeviceInfoWinUSB;

tDeviceInfoWinUSB devInfo;


//****************************************************************************
//
// Returns the device path associated with a provided interface GUID.
//
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
static DWORD GetDevicePath(LPGUID InterfaceGuid,
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
                                          0,
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
// Opens the USB device and returns a file handle.
//
// \param None.
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
static HANDLE OpenDevice(void)
{
    HANDLE hDev = NULL;
    char devicePath[MAX_DEVPATH_LENGTH];
    BOOL retVal;

    //
    // Get the path needed to open a file handle on our USB device.
    //
    retVal = GetDevicePath((LPGUID)&GUID_DEVINTERFACE_LUMINARY_BULK,
                           devicePath,
                           sizeof(devicePath));
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
// Determines that the required USB device is present, opens it and gathers
// required information to allow us to read and write it.
//
// \param None.
//
// This function is called to initialize the USB device and perform one-off
// setup required to allocate handles allowing the application to read and
// write the device endpoints.
//
// \return Returns \e TRUE on success or \e FALSE on failure. In failing
// cases, GetLastError() can be called to determine the cause.
//
//****************************************************************************
BOOL InitializeDevice(void)
{
    BOOL bResult;
    WINUSB_INTERFACE_HANDLE usbHandle;
    USB_INTERFACE_DESCRIPTOR ifaceDescriptor;
    WINUSB_PIPE_INFORMATION pipeInfo;
    UCHAR speed;
    ULONG length;
    int i;

    //
    // Determine whether the USB device is present and, if so, generate a file
    // handle to allow access to it.
    //
    devInfo.deviceHandle = OpenDevice();
    if(devInfo.deviceHandle == INVALID_HANDLE_VALUE)
    {
        //
        // We were unable to access the device - return a failure.
        //
        return(FALSE);
    }

    //
    // The device is opened so we now initialize the WinUSB layer passing it
    // the device handle.
    //
    bResult = WinUsb_Initialize(devInfo.deviceHandle, &usbHandle);

    if(bResult)
    {
        //
        // If we managed to initialize the WinUSB layer, we now query the
        // device descriptor to determine the speed of the device.
        //
        devInfo.winUSBHandle = usbHandle;
        length = sizeof(UCHAR);
        bResult = WinUsb_QueryDeviceInformation(devInfo.winUSBHandle,
                                                DEVICE_SPEED,
                                                &length,
                                                &speed);
    }

    if(bResult)
    {
        //
        // If all is well, now query the interface descriptor. We ask for the
        // first interface only since, in the case of the generic bulk device,
        // this is all that is available.
        //
        devInfo.deviceSpeed = speed;
        bResult = WinUsb_QueryInterfaceSettings(devInfo.winUSBHandle,
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
            bResult = WinUsb_QueryPipe(devInfo.winUSBHandle, 0, (UCHAR) i,
                                      &pipeInfo);

            if((pipeInfo.PipeType == UsbdPipeTypeBulk) &&
                  USB_ENDPOINT_DIRECTION_IN(pipeInfo.PipeId))
            {
                devInfo.bulkInPipe = pipeInfo.PipeId;
            }
            else if((pipeInfo.PipeType == UsbdPipeTypeBulk) &&
                  USB_ENDPOINT_DIRECTION_OUT(pipeInfo.PipeId))
            {
                devInfo.bulkOutPipe = pipeInfo.PipeId;
            }
            else
            {
                //
                // Hmm... we found and endpoint that we didn't expect to see
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
  return(bResult);
}

//****************************************************************************
//
// Cleans up and free resources associated with the USB device communication
// prior to exiting the application.
//
// \param None.
//
// This function should be called prior to exiting the application to free
// the resources allocated during InitializeDevice().
//
// \return Returns \e TRUE on success or \e FALSE on failure.
//
//****************************************************************************
BOOL TerminateDevice(void)
{
    BOOL bRetcode;
    BOOL bRetcode2;

    bRetcode = WinUsb_Free(devInfo.winUSBHandle);

    bRetcode2 = CloseHandle(devInfo.deviceHandle);

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
BOOL WriteUSBPacket(unsigned char *pcBuffer, unsigned long ulSize,
                    unsigned long *pulWritten)
{
    BOOL bResult;

    bResult = WinUsb_WritePipe(devInfo.winUSBHandle,
                               devInfo.bulkOutPipe,
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
// \param pcBuffer points to a buffer into which the data from the device will
// be written.
// \param ulSize contains the number of bytes that are requested from the 
// device.
// \param pulRead is a pointer which will be written with the number of 
// bytes of data actually read from the device.
//
// This function is used to receivedata from the USB device via its bulk IN
// endpoint.
//
// \return Returns \e TRUE on success or \e FALSE on failure.
//
//****************************************************************************
BOOL ReadUSBPacket(unsigned char *pcBuffer, unsigned long ulSize,
                   unsigned long *pulRead)
{
    BOOL bResult;
    unsigned long ulSecondRead;

    bResult = WinUsb_ReadPipe(devInfo.winUSBHandle,
                              devInfo.bulkInPipe,
                              pcBuffer,
                              ulSize,
                              pulRead,
                              NULL);

    //
    // Make sure we got the number of bytes expected. If we got less, try to
    // read the remainder.
    //
    if(bResult && (*pulRead < ulSize))
    {
        bResult = WinUsb_ReadPipe(devInfo.winUSBHandle,
                                  devInfo.bulkInPipe,
                                  pcBuffer + *pulRead,
                                  ulSize - *pulRead,
                                  &ulSecondRead,
                                  NULL);
        if(bResult)
        {
            *pulRead += ulSecondRead;
        }
    }

    return(bResult);
}

#endif // USE_WINUSB
