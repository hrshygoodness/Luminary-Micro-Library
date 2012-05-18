//
// scope_control.c - Low level functions allowing control of the Luminary
//    Micro Oscilloscope device via USB.
//
#include "stdafx.h"
#include <windows.h>
//#include <setupapi.h>
//#include <devguid.h>
#include <regstr.h>
#include <strsafe.h>
#include <initguid.h>
#include "scope_guids.h"
#include "lmusbdll.h"
#include "lmusbwrap.h"
#include "resource.h"
#define PACKED
#pragma pack(1)
#include "usb_protocol.h"
#pragma pack()
#include "scope_control.h"

//****************************************************************************
//
// Buffer size definitions.
//
//****************************************************************************
#define MAX_DEVPATH_LENGTH 256
#define MAX_STRING_LEN 256

//****************************************************************************
//
// Various timeouts in milliseconds.
//
//****************************************************************************
#define THREAD_END_TIMEOUT 3000
#define CONNECT_RETRY_DELAY 2000
#define MAX_WAIT_DELAY_MS 400

//****************************************************************************
//
// Structure containing handles and information required to communicate with
// the USB bulk device.
//
//****************************************************************************
typedef struct
{
    HANDLE hConnectEvent;
    HANDLE hThreadSignalEvent;
    HANDLE hThreadEndEvent;
    HANDLE hThread;
    DWORD dwThreadID;
    LMUSB_HANDLE hUSB;
    BOOL  bDeviceConnected;
    BOOL  bCommunicating;
    BOOL  bInterfaceFound;
    HWND  hwndNotify;
} tDeviceInfo;

tDeviceInfo devInfo;

typedef enum
{
    SCOPE_OK,
    SCOPE_TIMEOUT,
    SCOPE_DISCONNECTED,
    SCOPE_PROTOCOL_ERROR,
    SCOPE_READ_ERROR,
    SCOPE_END_SIGNALLED
} tDeviceRetcode;

//****************************************************************************
//
// Internal function prototypes.
//
//****************************************************************************
static bool SendScopePacket(unsigned char ucPacketType,
                            unsigned char ucParam,
                            unsigned long ulParam,
                            unsigned long ulDataLength = 0,
                            void *pData = NULL);
static bool ScopeWaitPacket(unsigned char *pucPacketType,
                            unsigned char *pucParam,
                            unsigned long *pulParam,
                            unsigned long *pulDataLength,
                            void **ppData);
static tDeviceRetcode ScopeReadPacket(tDeviceInfo *pDevice,
                                      tScopePacket *psPacket,
                                      void **ppvData,
                                      DWORD dwTimeoutmS);

//****************************************************************************
//
// This thread is responsible for handling all reads from the USB device
// and also for polling for connections if the device is not yet connected.
//
//****************************************************************************
static DWORD WINAPI ReadConnectThread(LPVOID lpParam)
{
    tDeviceInfo *pDevice = (tDeviceInfo *)lpParam;
    tDeviceRetcode eRetcode;
    DWORD dwRetcode;
    BOOL bResult;
    BOOL bDriverInstalled = FALSE;
    BOOL bDataReadOngoing = FALSE;
    tScopePacket sPacket;
    tScopeDataStart *psDataStart;
    void *pvPacketData;
    static char *pData;
    static unsigned char ucContinuityCount;
    static char *pcElement;
    static unsigned long ulDataSize;

    while(1)
    {
        //
        // If we are not currently in communication with the device, try to
        // open it. If this fails, wait a while and try again.
        //
        while(!pDevice->bDeviceConnected)
        {
            //
            // Try to connect.
            //
            pDevice->hUSB = _InitializeDevice(SCOPE_VID, SCOPE_PID,
                                    (LPGUID)&GUID_DEVINTERFACE_LUMINARY_SCOPE,
                                    &bDriverInstalled);

            //
            // Set the flag we use to determine if we are connected or not.
            //
            pDevice->bDeviceConnected = pDevice->hUSB ? TRUE : FALSE;

            //
            // Was the connection attempt unsuccessfull?
            //
            if(!pDevice->hUSB)
            {
                //
                // We couldn't connect. Was the correct device driver found?
                //
                if(!bDriverInstalled)
                {
                    //
                    // No - post a message to the client telling them that the
                    // driver isn't there. This will be a periodic message
                    // until the driver is installed or the function
                    // ScopeControlTerminate() is called.
                    //
                    PostMessage(pDevice->hwndNotify, WM_SCOPE_NO_DRIVER, 0,
                                0);
                }

                //
                // Sleep for a while but ensure that we catch cases where we
                // are signalled to exit.
                //
                dwRetcode = WaitForSingleObject(pDevice->hThreadEndEvent,
                                                CONNECT_RETRY_DELAY);

                //
                // A zero return code indicates that the event was signalled.
                // On any other return code, we go back and try to connect
                // again.
                //
                if(!dwRetcode)
                {
                    //
                    // The thread end signal was received so we need to exit
                    // here.
                    //
                    SetEvent(pDevice->hThreadSignalEvent);
                    ExitThread(0);
                }
            }
        }

        //
        // At this point, the device is connected but we have not yet
        // established communication. We send a message to the client
        // telling them that a connection has occurred and wait for them
        // to call ScopeControlConnect() to continue the process.
        //
        PostMessage(pDevice->hwndNotify, WM_SCOPE_DEVICE_AVAILABLE, 0, 0);

        //
        // Now we continue reading from the device until it disconnects
        // or the client tells the thread to exit.
        //
        while(pDevice->bDeviceConnected)
        {
            //
            // Get a packet from the oscilloscope
            //
            eRetcode = ScopeReadPacket(pDevice, &sPacket, &pvPacketData,
                                       INFINITE);

            //
            // Did we read the packet as expected?
            //
            if(eRetcode == SCOPE_OK)
            {
                //
                // We got a packet. Now decide what to do with it.
                //
                switch(sPacket.ucPacketType)
                {
                    //
                    // The oscilloscope has sent a handshake response to our
                    // SCOPE_PKT_HELLO packet.
                    //
                    case SCOPE_PKT_HELLO_RESPONSE:
                    {
                        //
                        // A HELLO_RESPONSE packet tells us that the device is
                        // running and communicating. Pass this on to the
                        // client and set our flag to indicate that we are
                        // in full communication.
                        //
                        pDevice->bCommunicating = true;
                        PostMessage(pDevice->hwndNotify,
                                    WM_SCOPE_DEVICE_CONNECTED,
                                    0, (LPARAM)pvPacketData);
                    }
                    break;

                    //
                    // The scope has responded to our outgoing SCOPE_PKT_PING
                    // packet.
                    //
                    case SCOPE_PKT_PING_RESPONSE:
                    {
                        //
                        // Pass the ping response back to the client.
                        //
                        PostMessage(pDevice->hwndNotify,
                                    WM_SCOPE_PING_RESPONSE,
                                    (WPARAM)sPacket.ucParam,
                                    (LPARAM)sPacket.ulParam);
                    }
                    break;

                    //
                    // The oscilloscope has sent the packet which indicates
                    // the start of waveform data transmission.
                    //
                    case SCOPE_PKT_DATA_START:
                    {
                        //
                        // The packet payload should be a structure telling us
                        // about the data we can expect to follow.
                        //
                        psDataStart = (tScopeDataStart *)pvPacketData;

                        //
                        // If we didn't get a payload, leave before doing
                        // anything else.
                        //
                        if(!psDataStart)
                        {
                            break;
                        }

                        //
                        // Start reception of a new data packet. If we have
                        // a previous incomplete packet, throw it away.
                        //
                        if(bDataReadOngoing)
                        {
                            LocalFree(pData);
                        }

                        //
                        // Allocate storage for the header info and the
                        // samples we will be gathering.
                        //
                        if(psDataStart->bDualChannel)
                        {
                            ulDataSize = sizeof(tScopeDataStart) +
                                         psDataStart->ulTotalElements *
                                         sizeof(tScopeDualDataElement);
                        }
                        else
                        {
                            ulDataSize = sizeof(tScopeDataStart) +
                                         psDataStart->ulTotalElements *
                                         sizeof(tScopeDataElement);
                        }

                        //
                        // Allocate enough storage to hold the whole data block.
                        //
                        pData = (char *)LocalAlloc(LMEM_FIXED, ulDataSize);

                        //
                        // If we got the storage, copy the header into it and set up
                        // our pointers to allow the data to be copied from later
                        // SCOPE_PKT_DATA packets.
                        //
                        if(pData)
                        {
                            memcpy(pData, psDataStart, sizeof(tScopeDataStart));

                            //
                            // Set up for the new capture.
                            //
                            ucContinuityCount = 1;
                            bDataReadOngoing = TRUE;
                            pcElement = (pData + sizeof(tScopeDataStart));
                            ulDataSize -= sizeof(tScopeDataStart);
                        }

                        //
                        // Free the packet payload since we are finished with
                        // it now.
                        //
                        LocalFree(pvPacketData);
                    }
                    break;

                    //
                    // The oscilloscope is signalling the end of transmission
                    // for a waveform data block.
                    //
                    case SCOPE_PKT_DATA_END:
                    {
                        //
                        // Data reception is complete so pass the completed
                        // data set back to the client.
                        //
                        bResult = PostMessage(pDevice->hwndNotify, WM_SCOPE_DATA,
                                             (WPARAM)sizeof(tScopeDataStart), (LPARAM)pData);

                        //
                        // Only free the pData buffer if we failed to post
                        // the message to the client queue. If the post is
                        // successful, the client must free the pointer once
                        // it processes the message.
                        //
                        if(!bResult)
                        {
                            LocalFree(pData);
                        }

                        //
                        // We are no longer in the midst of a data set so
                        // record this fact.
                        //
                        bDataReadOngoing = FALSE;
                        pData = NULL;
                    }
                    break;

                    //
                    // The oscilloscope has sent some of the data comprising
                    // capture of a waveform.
                    //
                    case SCOPE_PKT_DATA:
                    {
                        //
                        // Add this data to the buffer we are collecting for
                        // the client (assuming we are collecting data). If
                        // not, merely discard the packet.
                        //
                        if(bDataReadOngoing)
                        {
                            //
                            // Check that the packet number is as expected
                            // and that we have space for the payload.
                            //
                            if((sPacket.ucParam == ucContinuityCount++) &&
                                (ulDataSize >= sPacket.ulDataLength))
                            {
                                //
                                // Copy the payload into our sample buffer.
                                //
                                memcpy(pcElement, pvPacketData,
                                       sPacket.ulDataLength);
                                ulDataSize -= sPacket.ulDataLength;
                                pcElement += sPacket.ulDataLength;
                            }
                            else
                            {
                                //
                                // Packet continuity counter showed an error.
                                // Tidy up and discard everything to the end
                                // of the sequence.
                                //
                                bDataReadOngoing = FALSE;
                                LocalFree(pData);
                            }
                        }

                        //
                        // Free the payload.
                        //
                        LocalFree(pvPacketData);
                    }
                    break;

                    //
                    // The oscilloscope is indicating that automatic waveform
                    // capture has started.
                    //
                    case SCOPE_PKT_STARTED:
                    {
                        //
                        // Pass a message to the client.
                        //
                        PostMessage(pDevice->hwndNotify, WM_SCOPE_STARTED,
                                    0, 0);
                    }
                    break;

                    //
                    // The oscilloscope is indicating that automatic waveform
                    // capture has stopped.
                    //
                    case SCOPE_PKT_STOPPED:
                    {
                        //
                        // Pass a message to the client.
                        //
                        PostMessage(pDevice->hwndNotify, WM_SCOPE_STOPPED,
                                    0, 0);
                    }
                    break;

                    //
                    // The oscilloscope is indicating that the timebase has
                    // been changed.
                    //
                    case SCOPE_PKT_TIMEBASE_UPDATED:
                    {
                        //
                        // Pass the new trigger level to the client.
                        //
                        PostMessage(pDevice->hwndNotify,
                                    WM_SCOPE_TIMEBASE_CHANGED, 0,
                                    (LPARAM)sPacket.ulParam);
                    }
                    break;

                    //
                    // The oscilloscope is indicating that the trigger channel
                    // and/or type has changed.
                    //
                    case SCOPE_PKT_TRIGGER_TYPE:
                    {
                        //
                        // Pass the new trigger level to the client.
                        //
                        PostMessage(pDevice->hwndNotify,
                                    WM_SCOPE_TRIGGER_TYPE_CHANGED,
                                    (WPARAM)sPacket.ucParam,
                                    (LPARAM)sPacket.ulParam);
                    }
                    break;

                    //
                    // The oscilloscope is indicating that the trigger level
                    // has changed.
                    //
                    case SCOPE_PKT_TRIGGER_LEVEL:
                    {
                        //
                        // Pass the new trigger level to the client.
                        //
                        PostMessage(pDevice->hwndNotify,
                                    WM_SCOPE_TRIGGER_LEVEL_CHANGED,
                                    0, (LPARAM)sPacket.ulParam);
                    }
                    break;

                    //
                    // The oscilloscope is indicating that the trigger position
                    // has changed.
                    //
                    case SCOPE_PKT_TRIGGER_POS:
                    {
                        //
                        // Pass the new trigger level to the client.
                        //
                        PostMessage(pDevice->hwndNotify,
                                    WM_SCOPE_TRIGGER_POS_CHANGED,
                                    0, (LPARAM)sPacket.ulParam);
                    }
                    break;

                    //
                    // The oscilloscope is indicating that channel 2 has been
                    // enabled or disabled.
                    //
                    case SCOPE_PKT_CHANNEL2:
                    {
                        //
                        // Pass a message to the client.
                        //
                        PostMessage(pDevice->hwndNotify, WM_SCOPE_CHANNEL2,
                                    (WPARAM)sPacket.ucParam , 0);
                    }
                    break;

                    //
                    // The oscilloscope is informing us that the vertical scaling for one
                    // of the channels has been changed.
                    //
                    case SCOPE_PKT_SCALE:
                    {
                        //
                        // Pass a message to the client.
                        //
                        PostMessage(pDevice->hwndNotify, WM_SCOPE_SCALE_CHANGED,
                                    (WPARAM)sPacket.ucParam , (LPARAM)sPacket.ulParam);
                    }
                    break;


                    //
                    // The oscilloscope is informing us that the vertical position for one
                    // of the channels has been changed.
                    //
                    case SCOPE_PKT_POSITION:
                    {
                        //
                        // Pass a message to the client.
                        //
                        PostMessage(pDevice->hwndNotify, WM_SCOPE_POS_CHANGED,
                                    (WPARAM)sPacket.ucParam , (LPARAM)sPacket.ulParam);
                    }
                    break;

                    //
                    // All other messages drop into this case. We don't
                    // expect to see this but...
                    //
                    default:
                    {
                        //
                        // This is some other kind of packet that we don't
                        // understand so just throw it away.
                        //
                        if(pvPacketData)
                        {
                            LocalFree(pvPacketData);
                        }
                    }
                    break;
                }
            }
            else
            {
                //
                // Did the scope device disconnect?
                //
                if(eRetcode == SCOPE_DISCONNECTED)
                {
                    //
                    // Yes - tidy up before going back to search for
                    // reconnection.
                    //
                    _TerminateDevice(pDevice->hUSB);
                    PostMessage(pDevice->hwndNotify,
                                WM_SCOPE_DEVICE_DISCONNECTED, 0, 0);
                }
            }
        }
    }
}

//****************************************************************************
//
// Free any events that have already been created for this module.
//
//****************************************************************************
static void DestroyScopeControlEvents(void)
{
    if(devInfo.hConnectEvent)
    {
        CloseHandle(devInfo.hConnectEvent);
        devInfo.hConnectEvent = (HANDLE)NULL;
    }

    if(devInfo.hThreadEndEvent)
    {
        CloseHandle(devInfo.hThreadEndEvent);
        devInfo.hThreadEndEvent = (HANDLE)NULL;
    }

    if(devInfo.hThreadSignalEvent)
    {
        CloseHandle(devInfo.hThreadSignalEvent);
        devInfo.hThreadSignalEvent = (HANDLE)NULL;
    }
}

//****************************************************************************
//
// Initialize this module's internal data and start the read/connect
// thread.
//
//****************************************************************************
bool ScopeControlInit(HWND hwndNotify)
{
    BOOL bRetcode, bDriverInstalled;

    //
    // Clear out our instance data.
    //
    memset(&devInfo, 0, sizeof(tDeviceInfo));

    //
    // Remember the window handle passed.
    //
    devInfo.hwndNotify = hwndNotify;

    //
    // Try to load the USB device driver for the oscilloscope.
    //
    bRetcode = LoadLMUSBLibrary(&bDriverInstalled);
    if(!bRetcode)
    {
        //
        // We couldn't load the driver. Either it's not installed or the
        // installed version is out of sync with the application.  Display
        // a warning and abort the application.
        //
        AfxMessageBox(bDriverInstalled ? IDS_DRIVER_VERSION : IDS_DRIVER_MISSING,
                      MB_OK | MB_ICONSTOP);
        ASSERT(AfxGetMainWnd() != NULL);
        AfxGetMainWnd()->SendMessage(WM_CLOSE);
        return(false);
    }

    //
    // If we are passed a window handle, we assume asynchronous operation
    // with notifications being sent to the host via the window. This
    // requires us to start a thread and create additional signalling
    // resources.
    //
    // If a window handle is not passed, we operate synchronously with
    // blocking reads taking place in the calling context rather than
    // using the background thread.
    //
    if(hwndNotify)
    {
        //
        // Create the events we use to support synchronous read, connect and
        // ping requests.
        //
        devInfo.hConnectEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        devInfo.hThreadSignalEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        devInfo.hThreadEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        //
        // Were the events created successfully?
        //
        if(!devInfo.hConnectEvent || !devInfo.hThreadEndEvent ||
           !devInfo.hThreadSignalEvent)
        {
            DestroyScopeControlEvents();
            return(false);
        }
        else
        {
            //
            // Start our read/connect thread.
            //
            devInfo.hThread = CreateThread(NULL, 0, ReadConnectThread,
                                           &devInfo, 0, &devInfo.dwThreadID);

            //
            // Was the thread created successfully?
            //
            if(devInfo.hThread)
            {
                //
                // All is well. Thread is running and we are ready for
                // operation.
                //
                return(true);
            }
            else
            {
                //
                // We couldn't create the thread so fail this call after
                // freeing the other resources we created earlier.
                //
                DestroyScopeControlEvents();
                return(false);
            }
        }
    }
    else
    {
        //
        // We are operating synchronously.
        //
        return(true);
    }
}

//****************************************************************************
//
// Shut down the scope control module, free all resources and kill the
// read/connect thread.
//
//****************************************************************************
bool ScopeControlTerm(void)
{
    DWORD dwRetcode;
    bool bRetcode;

    //
    // Tell the thread to die and wait for its response.
    //
    SetEvent(devInfo.hThreadEndEvent);
    dwRetcode = WaitForSingleObject(devInfo.hThreadSignalEvent,
                                    THREAD_END_TIMEOUT);
    if(dwRetcode)
    {
        //
        // The thread failed to signal that it is ending. There's not a lot
        // we can do here other than go on and free resources but we do
        // remember to return a bad return code to the caller anyway.
        //
        bRetcode = false;
    }
    else
    {
        bRetcode = true;
    }

    //
    // Destroy our signalling events.
    //
    DestroyScopeControlEvents();

    return(bRetcode);
}

//****************************************************************************
//
// Send a PING packet to the device and wait for its response.
//
//****************************************************************************
bool ScopeControlPing(unsigned char ucEcho1, unsigned long ulEcho2)
{
    bool bRetcode;
    unsigned char ucParam;
    unsigned char ucType;
    unsigned long ulParam;
    unsigned long ulLength;
    void *pData;

    //
    // Send a new ping packet to the device.
    //
    bRetcode = SendScopePacket(SCOPE_PKT_PING, ucEcho1, ulEcho2);
    if(bRetcode)
    {
        //
        // Are we operating synchronously or asynchronously?
        //
        if(!devInfo.hwndNotify)
        {
            //
            // We are operating synchronously so we block and wait for the
            // response from the device.
            //

            //
            // If we sent the packet successfully, wait for the scope to
            // respond.
            //
            bRetcode = ScopeWaitPacket(&ucType, &ucParam, &ulParam, &ulLength,
                                       (void **)&pData);
            if(bRetcode)
            {
                //
                // Check to make sure that the response was correct. The
                // parameters should have been echoed back to us.
                //
                if(!((ucType == SCOPE_PKT_PING_RESPONSE) &&
                   (ulLength == 0) && (ucParam == ucEcho1) && (ulParam == ulEcho2)))
                {
                    bRetcode = false;
                }
            }
        }
    }

    return(bRetcode);
}

//****************************************************************************
//
// Disconnect from the oscilloscope device.
//
//****************************************************************************
bool ScopeControlDisconnect(void)
{
    BOOL bRetcode = TRUE;

    //
    // If we are connected and communicating, tell the scope that we are going away.
    //
    if(devInfo.bCommunicating)
    {
        SendScopePacket(SCOPE_PKT_HOST_GOODBYE, 0, 0);
    }

    if(!devInfo.hwndNotify)
    {
        //
        // Free our Windows-side resources.
        //
        if(devInfo.bDeviceConnected)
        {
            bRetcode = _TerminateDevice(devInfo.hUSB);
        }

        //
        // Clean up our state so that we are ready for another call to
        // ScopeControlConnect().
        //
        devInfo.bCommunicating = false;
    }

    return(bRetcode ? true : false);
}

//****************************************************************************
//
// Connect to the device and exchange HOST_HELLO/HELLO_RESPONSE.
//
//****************************************************************************
bool ScopeControlConnect(tScopeSettings *psSettings)
{
    bool bRetcode;
    BOOL bDriverInstalled;
    unsigned char ucType;
    unsigned char ucParam;
    unsigned long ulParam;
    unsigned long ulLength;
    tScopeSettings *psLocalSettings;

    //
    // Are we operating in synchronous mode? If not, the device open handling
    // is performed in the read/connect thread.
    //
    if(!devInfo.hwndNotify)
    {
        //
        // Yes so try to open the device.
        //
        devInfo.hUSB = _InitializeDevice(SCOPE_VID, SCOPE_PID,
                                  (LPGUID)&(GUID_DEVINTERFACE_LUMINARY_SCOPE),
                                  &bDriverInstalled);

        //
        // If the InitializeDevice call was successful, we are connected to
        // the oscilloscope device and have queried its endpoints successfully.
        // We are ready to start sending packets back and forward.
        //
        devInfo.bDeviceConnected = (devInfo.hUSB ? TRUE : FALSE);
    }
    else
    {
        //
        // We are in asynchronous mode. The read/connect thread handles
        // the basic initiation of communication so, if this has not yet
        // succeeded, return false to indicate that connection is not
        // possible just now.
        //
        if(!devInfo.bDeviceConnected)
        {
            return(false);
        }
    }

    //
    // Now we need to send the HELLO packet to the scope and wait for
    // its response.
    //
    bRetcode = SendScopePacket(SCOPE_PKT_HOST_HELLO, 0, 0, 0);

    //
    // If we sent the packet successfully and we are operating in synchronous
    // mode, wait for the device to respond with a HELLO_RESPONSE. In
    // asynchronous mode, this is passed to the client directly when it
    // appears using WM_SCOPE_DEVICE_CONNECTED.
    //
    if(!devInfo.hwndNotify && bRetcode)
    {
        //
        // If we sent the packet successfully, wait for the scope to respond.
        //
        bRetcode = ScopeWaitPacket(&ucType, &ucParam, &ulParam, &ulLength,
                                   (void **)&psLocalSettings);
        if(bRetcode)
        {
            //
            // Yay - we are in communication with the scope (or, at least, it's
            // talking to us). Check what it sent to make sure it was a HELLO
            // response packet.
            //
            if((ucType == SCOPE_PKT_HELLO_RESPONSE) &&
               (ulLength == sizeof(tScopeSettings)))
            {
                //
                // We got what we expected so all is well. We are now in
                // communication with the scope device.
                //
                devInfo.bCommunicating = TRUE;

                //
                // Copy the response into the user's buffer and free the pointer
                // we were passed by ScopeWaitPacket().
                //
                *psSettings = *psLocalSettings;
                LocalFree(psLocalSettings);
            }
        }
    }

    return(bRetcode);
}

//****************************************************************************
//
// Start or stop automatic data capture.
//
//****************************************************************************
bool ScopeControlStartStop(bool bStart)
{
    bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope to stop or start automatic capture as requested.
        //
        bRetcode = SendScopePacket(bStart ? SCOPE_PKT_START : SCOPE_PKT_STOP, 0, 0);

        return(bRetcode);
    }
    else
    {
        return(false);
    }
}

//****************************************************************************
//
// Start or stop automatic data capture.
//
//****************************************************************************
bool ScopeControlEnableChannel2(bool bEnable)
{
    bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope to enable or disable channel 2.
        //
        bRetcode = SendScopePacket(SCOPE_PKT_SET_CHANNEL2, (bEnable ?
                                   SCOPE_CHANNEL2_ENABLE :
                                   SCOPE_CHANNEL2_DISABLE), 0);

        return(bRetcode);
    }
    else
    {
        return(false);
    }
}

//****************************************************************************
//
// Request capture of a single waveform from the oscilloscope.
//
//****************************************************************************
bool ScopeControlCapture(void)
{
    bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope capture a single waveform.
        //
        bRetcode = SendScopePacket(SCOPE_PKT_CAPTURE, 0, 0);

        return(bRetcode);
    }
    else
    {
        return(false);
    }
}

//****************************************************************************
//
// Set the capture timebase
//
//****************************************************************************
bool ScopeControlSetTimebase(unsigned long ulTimebaseuSdiv)
{
    bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope capture a single waveform.
        //
        bRetcode = SendScopePacket(SCOPE_PKT_SET_TIMEBASE, 0, ulTimebaseuSdiv);

        return(bRetcode);
    }
    else
    {
        return(false);
    }
}

//****************************************************************************
//
// Set the oscilloscope trigger level.
//
//****************************************************************************
bool ScopeControlSetTriggerLevel(unsigned long ulLevelmV)
{
    bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope capture a single waveform.
        //
        bRetcode = SendScopePacket(SCOPE_PKT_SET_TRIGGER_LEVEL, 0, ulLevelmV);

        return(bRetcode);
    }
    else
    {
        return(false);
    }
}


//****************************************************************************
//
// Set the oscilloscope trigger level.
//
//****************************************************************************
bool ScopeControlSetTriggerPos(long lPos)
{
    bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope capture a single waveform.
        //
        bRetcode = SendScopePacket(SCOPE_PKT_SET_TRIGGER_POS, 0, lPos);

        return(bRetcode);
    }
    else
    {
        return(false);
    }
}

//****************************************************************************
//
// Set the trigger type and channel.
//
//****************************************************************************
bool ScopeControlSetTrigger(unsigned char ucChannel, unsigned long ulType)
{
    bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope capture a single waveform.
        //
        bRetcode = SendScopePacket(SCOPE_PKT_SET_TRIGGER_TYPE, ucChannel, ulType);

        return(bRetcode);
    }
    else
    {
        return(false);
    }
}

//****************************************************************************
//
// Set the vertical position for a particular oscilloscope channel waveform.
//
//****************************************************************************
bool ScopeControlSetPosition(unsigned char ucChannel,
                             long lPosmV)
{
    bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope to set the vertical position for the given
        // channel.
        //
        bRetcode = SendScopePacket(SCOPE_PKT_SET_POSITION, ucChannel,
                                   (unsigned long)lPosmV);

        return(bRetcode);
    }
    else
    {
        return(false);
    }
}

//****************************************************************************
//
// Set the vertical scaling for a particular oscilloscope channel waveform.
//
//****************************************************************************
bool ScopeControlSetScale(unsigned char ucChannel,
                          unsigned long ulScalemVdiv)
{
        bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope to set the scaling for the given channel.
        //
        bRetcode = SendScopePacket(SCOPE_PKT_SET_SCALE, ucChannel,
                                   ulScalemVdiv);

        return(bRetcode);
    }
    else
    {
        return(false);
    }
}

//****************************************************************************
//
// Enable or disable automatic data transmission from the device.
//
//****************************************************************************
bool ScopeControlAutomaticDataTransmission(bool bEnable)
{
    bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope to enable or disable channel 2.
        //
        bRetcode = SendScopePacket(SCOPE_PKT_DATA_CONTROL, bEnable, 0);

        return(bRetcode);
    }
    else
    {
        return(false);
    }

}

//****************************************************************************
//
// Request transmission of the last data captured by the oscilloscope.
//
//****************************************************************************
bool ScopeControlDataRequest(void)
{
    bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope to send us the last data captured.
        //
        bRetcode = SendScopePacket(SCOPE_PKT_RETRANSMIT, 0, 0);

        return(bRetcode);
    }
    else
    {
        return(false);
    }
}

//****************************************************************************
//
// Perform a blocking read of a waveform data set from the USB device. This
// function must not be called if operating in asynchronous mode (where a
// valid window handle was passed on ScopeControlInit()).
//
//****************************************************************************
bool ScopeControlReadData(tScopeDataStart *pStartInfo,
                          unsigned long *pulNumElements, void **ppvData)
{
    unsigned char ucPacketCount;
    unsigned char ucPacketType;
    unsigned char ucParam;
    unsigned long ulParam;
    unsigned long ulDataLength;
    unsigned long ulBlockSize;
    unsigned long ulWritePos;
    unsigned char *pcSamples;
    bool bRetcode;
    bool bErrorInSequence;

    //
    // Don't allow this operation if we are running in asynchronous mode
    //
    if(devInfo.hwndNotify)
    {
        return(false);
    }

    //
    // Wait for a SCOPE_PKT_DATA_START packet. We throw away anything else
    // we see while waiting.
    //
    while(1)
    {
        //
        // Read a packet.
        //
        bRetcode = ScopeWaitPacket(&ucPacketType, &ucParam, &ulParam, &ulDataLength,
                                   ppvData);
        if(bRetcode)
        {
            //
            // Was this a data start packet?
            //
            if(ucPacketType == SCOPE_PKT_DATA_START)
            {
                //
                // Yes - copy the payload into the passed structure.
                //
                *pStartInfo = *(tScopeDataStart *)(*ppvData);
                LocalFree(*ppvData);

                //
                // Allocate a buffer large enough for the data we are going to receive.
                //
                ulBlockSize = ulParam * (pStartInfo->bDualChannel ?
                              sizeof(tScopeDualDataElement) : sizeof(tScopeDataElement));
                *ppvData = LocalAlloc(LMEM_FIXED, ulBlockSize);
                *pulNumElements = ulParam;

                //
                // Did we get the buffer successfully?
                //
                if(!ppvData)
                {
                    //
                    // Out of memory
                    //
                    return(false);
                }

                //
                // Now read SCOPE_PKT_DATA packets and copy their payloads into our
                // data buffer.
                //
                ulWritePos = 0;
                ucPacketCount = 1;
                bErrorInSequence = false;

                while(ucPacketType != SCOPE_PKT_DATA_END)
                {
                    //
                    // Read a packet.
                    //
                    bRetcode = ScopeWaitPacket(&ucPacketType, &ucParam,
                                               &ulParam, &ulDataLength,
                                               (void **)&pcSamples);
                    if(bRetcode)
                    {
                        //
                        // Is this a data packet? If it is, and we have not seen an error in the
                        // sequence already, go ahead and process it.
                        //
                        if((ucPacketType == SCOPE_PKT_DATA) && !bErrorInSequence)
                        {
                            //
                            // Check that the packet number is as expected.
                            //
                            if(ucPacketCount == ucParam)
                            {
                                //
                                // Copy the payload into our sample buffer.
                                //
                                memcpy((unsigned char *)(*ppvData) + ulWritePos, pcSamples,
                                       ulDataLength);
                                ulWritePos += ulDataLength;
                                ucPacketCount++;
                            }
                            else
                            {
                                //
                                // Packet continuity counter showed an error. Read and
                                // discard everything to the end of the sequence.
                                //
                                bErrorInSequence = true;
                            }
                        }

                        //
                        // Free the data block if there was one in the last packet read.
                        //
                        if(pcSamples)
                        {
                            LocalFree(pcSamples);
                        }
                    }
                }

                //
                // We got to the end of the data sequence. Did we see any errors?
                //
                if(bErrorInSequence)
                {
                    //
                    // Yes - discard the data and report an error to the caller.
                    //
                    LocalFree(*ppvData);
                    return(false);
                }
                else
                {
                    //
                    // The sequence was captured properly. Return the good news to
                    // the caller.
                    //
                    return(true);
                }
            }
            else
            {
                //
                // This wasn't a data start packet so free any payload we were passed and wait
                // for the next packet.
                //
                if(*ppvData)
                {
                    LocalFree(*ppvData);
                    *ppvData = NULL;
                }
            }
        }
        else
        {
            //
            // We experienced an error reading a packet. Return to the caller.
            //
            return(false);
        }
    }
}

//****************************************************************************
//
// Request that the oscilloscope rescale and reposition one channel's
// waveform to ensure that it is visible on the display.
//
// \param ucChannel indicates the channel that is to be found, either
// SCOPE_CHANNEL_1 or SCOPE_CHANNEL_2.
//
//****************************************************************************
bool ScopeControlFind(unsigned char ucChannel)
{
    bool bRetcode;

    if(devInfo.bCommunicating)
    {
        //
        // Tell the oscilloscope to send us the last data captured.
        //
        bRetcode = SendScopePacket(SCOPE_PKT_FIND, ucChannel, 0);

        return(bRetcode);
    }
    else
    {
        return(false);
    }
}

//****************************************************************************
//
// Sends a single packet and optional additional data to the oscilloscope
// device if it is connected.
//
// \param ucPacketType is the type of packet to send.
// \param ucParam is the packet-specific byte parameter.
// \param ulParam is the packet-specific word parameter.
// \param ulDataLength is the length of the optional packet payload in bytes.
// \param pvData is a pointer to the optional packet payload data.
//
// This function sends a single packet to the oscilloscope of it is currently
// connected. A packet comprises a tScopePacket header structure followed,
// optionally, by a block of data.
//
// \return Returns \e TRUE on success or \e FALSE on failure.
//
//****************************************************************************
static bool SendScopePacket(unsigned char ucPacketType,
                            unsigned char ucParam,
                            unsigned long ulParam,
                            unsigned long ulDataLength,
                            void *pData)
{
    BOOL bResult;
    ULONG ulWritten;
    tScopePacket sPacket;

    //
    // We only try to send the packet if we are currently communicating with the
    // device.
    //
    if(devInfo.bDeviceConnected)
    {
        //
        // Populate the packet header.
        //
        sPacket.ucVersion = SCOPE_PROTOCOL_VERSION_1;
        sPacket.ucHdrLength = sizeof(tScopePacket);
        sPacket.ucPacketType = ucPacketType;
        sPacket.ucParam = ucParam;
        sPacket.ulParam = ulParam;
        sPacket.ulDataLength = ulDataLength;

        //
        // Write the user's string to the device.
        //

        bResult = _WriteUSBPacket(devInfo.hUSB, (unsigned char *)&sPacket,
                                  sizeof(tScopePacket), &ulWritten);
        if((!bResult) || (ulWritten != sizeof(tScopePacket)))
        {
            return(FALSE);
        }
        else
        {
            //
            // If we have optional data to send, append it to the packet.
            //
            if(ulDataLength != 0)
            {
                bResult = _WriteUSBPacket(devInfo.hUSB, (unsigned char *)pData,
                                          ulDataLength, &ulWritten);
                if((!bResult) || (ulWritten != ulDataLength))
                {
                    return(FALSE);
                }
            }
        }

        //
        // We sent all the data successfully
        //
        return(TRUE);
    }

    //
    // The device is not connected so there is no point trying to send
    // the packet.
    //
    return(FALSE);
}

//****************************************************************************
//
// Reads a single packet and optional additional data from the oscilloscope,
// blocking until the packet is available.
//
// \param pucPacketType is storage for the type of packet read.
// \param pucParam is storage for the packet-specific byte parameter.
// \param pulParam is storage the packet-specific word parameter.
// \param pulDataLength is storage for the length of the optional packet
//        payload in bytes.
// \param ppData is storage for a pointer to the optional packet payload
//        data.
//
// This function blocks until a packet is received from the oscilloscope
// then returns information on the packet to the caller. If the packet
// contains an optional data block, this is copied into a buffer whose
// pointer is passed back to the caller.
//
// \note The caller is responsible for freeing the packet whose pointer is
// returned in *ppData in cases where this is not NULL. The pointer must be
// freed using a call to LocalFree().
//
// \return Returns \e TRUE on success or \e FALSE on failure.
//
//****************************************************************************
static bool ScopeWaitPacket(unsigned char *pucPacketType,
                            unsigned char *pucParam,
                            unsigned long *pulParam,
                            unsigned long *pulDataLength,
                            void **ppData)
{
    DWORD dwResult;
    ULONG ulRead;
    tScopePacket sPacket;

    //
    // Only try to read if the device is connected.
    //
    if(devInfo.bDeviceConnected)
    {
        dwResult = _ReadUSBPacket(devInfo.hUSB, (unsigned char *)&sPacket,
                                  sizeof(tScopePacket), &ulRead, INFINITE,
                                  NULL);
        if((dwResult != ERROR_SUCCESS) || (ulRead != sizeof(tScopePacket)))
        {
            //
            // An error occurred trying to read the packet.
            //
            return(FALSE);
        }
        else
        {
            //
            // We got the scope packet header. Make sure that the
            // protocol and size fields are as expected.
            //
            if((sPacket.ucVersion == SCOPE_PROTOCOL_VERSION_1) &&
                (sPacket.ucHdrLength == sizeof(tScopePacket)))
            {
                //
                // Packet header seems fine. Do we have any optional data to read?
                //
                if(sPacket.ulDataLength != 0)
                {
                    //
                    // We have optional data so read it.
                    //
                    *ppData = LocalAlloc(LMEM_FIXED, sPacket.ulDataLength);

                    //
                    // Get the data
                    //
                    dwResult = _ReadUSBPacket(devInfo.hUSB,
                                              (unsigned char *)*ppData,
                                              sPacket.ulDataLength,
                                              &ulRead, INFINITE, NULL);

                    //
                    // Was the read successful?
                    //
                    if((dwResult != ERROR_SUCCESS) ||
                       (ulRead != sPacket.ulDataLength))
                    {
                        //
                        // No - tidy things up and return an error.
                        //
                        LocalFree(*ppData);
                        return(FALSE);
                    }
                }
                else
                {
                    //
                    // Tell the caller that there is no optional data for
                    // this packet.
                    //
                    *ppData = NULL;
                }

                //
                // Fill in the caller's return parameters.
                //
                *pucPacketType = sPacket.ucPacketType;
                *pucParam = sPacket.ucParam;
                *pulParam = sPacket.ulParam;
                *pulDataLength = sPacket.ulDataLength;

                return(TRUE);
            }
            else
            {
                //
                // There was a protocol error - the header was not valid.
                //
                return(FALSE);
            }
        }
    }
    else
    {
        //
        // The device is not connected so we can't read from it.
        //
        return(FALSE);
    }
}

//****************************************************************************
//
// Performs a read from lmusbdll with optional timeout and also ensures that
// signals from the client context telling us to close the thread are
// correctly noted.
//
// \param pDevice is a pointer to the device instance data.
// \param pDest is a pointer to storage for the data read.
// \param dwSize is the number of bytes of data to read.
// \param pdwCountRead is storage for the returned count of bytes actually
//        read.
// \param dwTimeoutmS is the maximum time to wait for data before
//        reporting a timeout.
//
// This function reads a block of data from the oscilloscope and returns
// that data in a buffer supplied by the caller.
//
// \return Returns SCOPE_OK if no error was detected, SCOPE_TIMEOUT if no
// packet was received within the supplied timeout, SCOPE_DISCONNECTED if
// the device was found to have disconnected, SCOPE_PROTOCOL_ERROR if an
// invalid or unrecognised protocol or header length is read,
// SCOPE_READ_ERROR if an unexpected failure is reported by lmusbdll or
// SCOPE_END_SIGNALLED if the thread end signal is detected while waiting for
// the packet.
//
//****************************************************************************
static tDeviceRetcode ScopeLMUSBRead(tDeviceInfo *pDevice, PUCHAR pDest,
                                     DWORD dwSize, DWORD *pdwCountRead,
                                     DWORD dwTimeoutmS)
{
    DWORD dwError;

    //
    // Try to read the requested data.
    //
    dwError = _ReadUSBPacket(pDevice->hUSB, pDest, dwSize, pdwCountRead,
                             dwTimeoutmS, pDevice->hThreadEndEvent);

    //
    // Did we get the data?
    //
    if(dwError == ERROR_SUCCESS)
    {
        //
        // We got the data - tell the caller all is well.
        //
        return(SCOPE_OK);
    }
    else
    {
        //
        // We didn't get the data. Was this a timeout?
        //
        if(dwError == WAIT_OBJECT_0)
        {
            return(SCOPE_END_SIGNALLED);
        }
        else
        {
            //
            // Some other error was reported so assume the device
            // disconnected and return to the caller.
            //
            pDevice->bCommunicating = false;
            pDevice->bDeviceConnected = false;
            return(SCOPE_DISCONNECTED);
        }
    }

    //
    // If we get here, the read must have timed out.
    //
    return(SCOPE_TIMEOUT);
}

//****************************************************************************
//
// Reads a single packet and optional additional data from the oscilloscope
// blocking for a maximum period before timing out.
//
// \param pDevice is a pointer to the device instance data.
// \param psPacket is a pointer to storage for the returned packet header.
// \param ppvData is storage for a pointer to the optional packet payload
//        data.
// \param dwTimeoutmS is the maximum time to wait for a packet before
//        reporting a timeout.
//
// This function reads a packet from the oscilloscope and returns the packet
// to the caller. It is intended for use from the ReadConnectThread only
// since it also returns prematurely if the thread end signal is detected.
//
// \note The caller is responsible for freeing the packet whose pointer is
// returned in *ppData in cases where this is not NULL. The pointer must be
// freed using a call to LocalFree().
//
// \return Returns SCOPE_OK if no error was detected, SCOPE_TIMEOUT if no
// packet was received within the supplied timeout, SCOPE_DISCONNECTED if
// the device was found to have disconnected, SCOPE_PROTOCOL_ERROR if an
// invalid or unrecognised protocol or header length is read,
// SCOPE_READ_ERROR if an unexpected failure is reported by lmusbdll or
// SCOPE_END_SIGNALLED if the thread end signal is detected while waiting for
// the packet.
//
//****************************************************************************
static tDeviceRetcode ScopeReadPacket(tDeviceInfo *pDevice,
                                      tScopePacket *psPacket,
                                      void **ppvData,
                                      DWORD dwTimeoutmS)
{
    ULONG ulRead;
    tDeviceRetcode eRetcode;

    //
    // Read a packet header from the oscilloscope.
    //
    eRetcode = ScopeLMUSBRead(pDevice, (PUCHAR)psPacket, sizeof(tScopePacket),
                              &ulRead, dwTimeoutmS);

    if(eRetcode == SCOPE_OK)
    {
        //
        // At this point, we know the asynchronous read completed
        // successfully. Make sure that the protocol and size fields are as
        // expected.
        //
        if((psPacket->ucVersion == SCOPE_PROTOCOL_VERSION_1) &&
           (psPacket->ucHdrLength == ulRead))
        {
            //
            // Packet header seems fine. Do we have any optional data to read?
            //
            if(psPacket->ulDataLength != 0)
            {
                //
                // We have optional data so read it.
                //
                *ppvData = LocalAlloc(LMEM_FIXED, psPacket->ulDataLength);

                //
                // Get the data
                //
                eRetcode = ScopeLMUSBRead(pDevice,
                                          (PUCHAR)*ppvData,
                                          psPacket->ulDataLength,
                                          &ulRead,
                                          dwTimeoutmS);

                //
                // Was the read successful?
                //
                if((eRetcode != SCOPE_OK) ||
                   (ulRead != psPacket->ulDataLength))
                {
                    //
                    // No - tidy things up and return an error.
                    //
                    LocalFree(*ppvData);
                    *ppvData = NULL;
                    return((eRetcode == SCOPE_OK) ?
                            SCOPE_READ_ERROR : eRetcode);
                }
            }
            else
            {
                //
                // Tell the caller that there is no optional data for
                // this packet.
                //
                *ppvData = NULL;
            }
        }
        else
        {
            //
            // There was a protocol error - the header was not valid.
            //
           eRetcode = SCOPE_PROTOCOL_ERROR;
        }
    }

    //
    // Tell the caller how we got on.
    //
    return(eRetcode);
}
