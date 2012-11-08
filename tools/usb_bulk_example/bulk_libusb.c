//
// Implementation of the bulk_usb interface using the linusb-win32 API.
//

#ifndef USE_WINUSB

#include "libusb-win32\include\usb.h"

//****************************************************************************
//
// Structure containing handles and information required to communicate with
// the USB bulk device.
//
//****************************************************************************
typedef struct
{
    usb_dev_handle *hDevice;

} tDeviceInfo;

tDeviceInfo devInfo;

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
    //
    // Initialize the libusb-win32 subsystem.
    //
    usb_init();

    //
    // 
    //

    return(TRUE);
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
    return(TRUE);
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
    return(TRUE);
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
    return(TRUE);
}

#endif // not defined USE_WINUSB
