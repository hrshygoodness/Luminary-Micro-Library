//*****************************************************************************
//
// finder.c - Program to locate Ethernet-based Stellaris board on the network.
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

#include "logger.h"
#include "windows.h"
#include "setupapi.h"

//*****************************************************************************
//
// The main user interface object.
//
//*****************************************************************************
CLoggerUI *g_pUI;

//*****************************************************************************
//
// A table making it easier for us to handle the various buttons and tie them
// to specific data logging channels.
//
//*****************************************************************************
tChannelControls g_ChannelControls[NUM_PACKET_CHANNELS];

//*****************************************************************************
//
// Various strings used to display information about the state of the
// application.
//
//*****************************************************************************
const char *g_pcStrings[] =
{
        "No file selected",
        "Append",
        "Overwrite",
        "Logging",
        "Searching for a data logger board...",
        "Data logger board connected. Listening for data...",
        "Select ""Host PC"" as the storage destination on the data logger board menu.",
        "Board disconnected.  Waiting for reconnection...",
        "Reading data from data logger board..."
};

#define NUM_STATUS_STRINGS (sizeof(g_pcStrings)/sizeof(char *))

//*****************************************************************************
//
// Timeout values for the UART when communicating with an EK board.  This
// assumes we've set the baud rate to 100KHz or faster (the minimum delay we
// can specify here is 1mS per byte) and is set up to try to minimize
// the possibility of aliased packet markers in the serial port data messing up
// sync after initial connection.
//
//*****************************************************************************
static COMMTIMEOUTS g_NormalTimeouts=
{
    0,      // ReadIntervalTimeout;
    10,     // ReadTotalTimeoutMultiplier;
    2000,   // ReadTotalTimeoutConstant - 2 seconds;
    10,     // WriteTotalTimeoutMultiplier;
    50,     // WriteTotalTimeoutConstant;
};

//*****************************************************************************
//
// The various states that the COM port communication thread can pass through
// as packets are read from the logger board.
//
//*****************************************************************************
typedef enum
{
    STATE_DISCONNECTED,
    STATE_WAIT_HDR_1,
    STATE_WAIT_HDR_2,
    STATE_READ_TIMESTAMP,
    STATE_READ_DATA
}
tCOMState;

//*****************************************************************************
//
// Build the channel information array to make it simpler to update the UI when
// new data packets are received from the EK board.  This must be done after
// the main UI object has been created and initialized.
//
//*****************************************************************************
void ConstructChannelInfo(void)
{
    g_ChannelControls[0].pButton = g_pUI->mCH0Switch;
    g_ChannelControls[0].pOutput = g_pUI->mCH0Output;
    g_ChannelControls[0].pStripChart = g_pUI->mCHStripChart;
    g_ChannelControls[0].ulMask = 0x00000001;
    g_ChannelControls[0].pcName = "CH0";
    g_ChannelControls[0].pcUnit = "V";

    g_ChannelControls[1].pButton = g_pUI->mCH1Switch;
    g_ChannelControls[1].pOutput = g_pUI->mCH1Output;
    g_ChannelControls[1].pStripChart = g_pUI->mCHStripChart;
    g_ChannelControls[1].ulMask = 0x00000002;
    g_ChannelControls[1].pcName = "CH1";
    g_ChannelControls[1].pcUnit = "V";

    g_ChannelControls[2].pButton = g_pUI->mCH2Switch;
    g_ChannelControls[2].pOutput = g_pUI->mCH2Output;
    g_ChannelControls[2].pStripChart = g_pUI->mCHStripChart;
    g_ChannelControls[2].ulMask = 0x00000004;
    g_ChannelControls[2].pcName = "CH2";
    g_ChannelControls[2].pcUnit = "V";

    g_ChannelControls[3].pButton = g_pUI->mCH3Switch;
    g_ChannelControls[3].pOutput = g_pUI->mCH3Output;
    g_ChannelControls[3].pStripChart = g_pUI->mCHStripChart;
    g_ChannelControls[3].ulMask = 0x00000008;
    g_ChannelControls[3].pcName = "CH3";
    g_ChannelControls[3].pcUnit = "V";

    g_ChannelControls[4].pButton = g_pUI->mAccelXSwitch;
    g_ChannelControls[4].pOutput = g_pUI->mAccelXOutput;
    g_ChannelControls[4].pStripChart = g_pUI->mAccelStripChart;
    g_ChannelControls[4].ulMask = 0x00000001;
    g_ChannelControls[4].pcName = "AccelX";
    g_ChannelControls[4].pcUnit = "g";

    g_ChannelControls[5].pButton = g_pUI->mAccelYSwitch;
    g_ChannelControls[5].pOutput = g_pUI->mAccelYOutput;
    g_ChannelControls[5].pStripChart = g_pUI->mAccelStripChart;
    g_ChannelControls[5].ulMask = 0x00000002;
    g_ChannelControls[5].pcName = "AccelY";
    g_ChannelControls[5].pcUnit = "g";

    g_ChannelControls[6].pButton = g_pUI->mAccelZSwitch;
    g_ChannelControls[6].pOutput = g_pUI->mAccelZOutput;
    g_ChannelControls[6].pStripChart = g_pUI->mAccelStripChart;
    g_ChannelControls[6].ulMask = 0x00000004;
    g_ChannelControls[6].pcName = "AccelZ";
    g_ChannelControls[6].pcUnit = "g";

    g_ChannelControls[7].pButton = g_pUI->mTempExtSwitch;
    g_ChannelControls[7].pOutput = g_pUI->mTempExtOutput;
    g_ChannelControls[7].pStripChart = g_pUI->mTempStripChart;
    g_ChannelControls[7].ulMask = 0x00000001;
    g_ChannelControls[7].pcName = "Ext. Temp.";
    g_ChannelControls[7].pcUnit = "C";

    g_ChannelControls[8].pButton = g_pUI->mTempIntSwitch;
    g_ChannelControls[8].pOutput = g_pUI->mTempIntOutput;
    g_ChannelControls[8].pStripChart = g_pUI->mTempStripChart;
    g_ChannelControls[8].ulMask = 0x00000002;
    g_ChannelControls[8].pcName = "Int. Temp.";
    g_ChannelControls[8].pcUnit = "C";

    g_ChannelControls[9].pButton = g_pUI->mCurrentSwitch;
    g_ChannelControls[9].pOutput = g_pUI->mCurrentOutput;
    g_ChannelControls[9].pStripChart = g_pUI->mCurrentStripChart;
    g_ChannelControls[9].ulMask = 0x00000001;
    g_ChannelControls[9].pcName = "CPU Current";
    g_ChannelControls[9].pcUnit = "mA";
}

//*****************************************************************************
//
// Update the string shown in the overwrite/append field of the UI.  The
// prototype is set up to allow use with the fl_awake function.
//
//*****************************************************************************
void UpdateOverwriteStatus(void *pvStringId)
{
    int iIndex;

    //
    // Get the index of the string we have been asked to show.
    //
    iIndex = (int)pvStringId;

    //
    // Is this a valid string index?
    //
    if(iIndex < NUM_STATUS_STRINGS)
    {
        //
        // Yes - display it in the relevant output field.
        //
        g_pUI->mOverwriteOutput->value(g_pcStrings[(int)pvStringId]);
    }
}

//*****************************************************************************
//
// Update the string shown in the main status field of the UI.  The prototype
// is set up to allow use with the fl_awake function.
//
//*****************************************************************************
void UpdateApplicationStatus(void *pvStringId)
{
    int iIndex;

    //
    // Get the index of the string we have been asked to show.
    //
    iIndex = (int)pvStringId;

    //
    // Is this a valid string index?
    //
    if(iIndex < NUM_STATUS_STRINGS)
    {
        //
        // Yes - display it in the relevant output field.
        //
        g_pUI->mStatusOutput->value(g_pcStrings[(int)pvStringId]);
    }
}

//*****************************************************************************
//
// Update the display showing which COM port is in use.  The prototype is set
// up to allow use with the fl_awake function.  The pvCOMPort parameter points
// to a string containing the name of the COM port.
//
//*****************************************************************************
void UpdateCOMStatus(void *pvCOMPort)
{
    char * pcBuffer = (char *)pvCOMPort;

    g_pUI->mCOMOutput->value(pcBuffer);
}

//*****************************************************************************
//
// This message handler is called in the context of the main loop whenever a
// new data packet is received from the EK board.
//
//*****************************************************************************
void HandleNewPacket(void *pvPacket)
{
    int iLoop, iPrec;
    char pcBuffer[16];
    static unsigned long ulLastMask;
    unsigned long ulChange;
    tSamplePacket *pPacket = (tSamplePacket *)pvPacket;
    float flVals[NUM_PACKET_CHANNELS];

    //
    // Update the channel masks as appropriate.
    //
    if(ulLastMask != pPacket->ulChannelMask)
    {
        //
        // Determine which channels have changed.
        //
        ulChange = ulLastMask ^ pPacket->ulChannelMask;

        //
        // Check each channel in turn.
        //
        for(iLoop = 0; iLoop < NUM_PACKET_CHANNELS; iLoop++)
        {
            //
            // Has this channel changed?
            //
            if(ulChange & (1 << iLoop))
            {
                //
                // Yes.  Has the channel been enabled or disabled?
                //
                if(pPacket->ulChannelMask & (1 << iLoop))
                {
                    //
                    // It has been enabled.  Enable the output field.
                    //
                    g_ChannelControls[iLoop].pOutput->activate();
                }
                else
                {
                    //
                    // It has been disabled.  Deactivate the output field and
                    // set it to indicate absence of data.
                    //
                    g_ChannelControls[iLoop].pOutput->value("No Data");
                    g_ChannelControls[iLoop].pOutput->deactivate();
                }
            }
        }

        ulLastMask = pPacket->ulChannelMask;
    }

    //
    // Add new data for each of the strip charts.
    //
    g_pUI->mCHStripChart->AddData(&pPacket->piSamples[0], 4,
                                   ulLastMask);
    g_pUI->mAccelStripChart->AddData(&pPacket->piSamples[4], 3,
                                     (ulLastMask >> 4));
    g_pUI->mTempStripChart->AddData(&pPacket->piSamples[7], 2,
                                     (ulLastMask >> 7));
    g_pUI->mCurrentStripChart->AddData(&pPacket->piSamples[9], 1,
                                       (ulLastMask >> 9));
    g_pUI->mCHStripChart->damage(FL_DAMAGE_ALL);
    g_pUI->mAccelStripChart->damage(FL_DAMAGE_ALL);
    g_pUI->mTempStripChart->damage(FL_DAMAGE_ALL);
    g_pUI->mCurrentStripChart->damage(FL_DAMAGE_ALL);

    //
    // Update the latest values displayed if the channel is active.
    //
    for(iLoop = 0; iLoop < NUM_PACKET_CHANNELS; iLoop++)
    {
        //
        // Are we receiving a valid sample for this channel?
        //
        if(ulLastMask & (1 << iLoop))
        {
            //
            // Yes - scale the value depending upon the stripchart setting.
            //
            iPrec = g_ChannelControls[iLoop].pStripChart->GetPrecision();
            flVals[iLoop] = ((float)(pPacket->piSamples[iLoop]) /
                              (float)pow(10.0, iPrec));

            //
            // Update the output display field.
            //
            sprintf(pcBuffer, "%.*f", iPrec, flVals[iLoop]);
            g_ChannelControls[iLoop].pOutput->value(pcBuffer);
        }
    }

    //
    // Are we currently logging to file?
    //
    if(g_pUI->mbLogging)
    {
        //
        // Yes - output the packet timestamp first.
        //
        g_pUI->mfLogFile << pPacket->ulTimestamp << ", ";
        g_pUI->mfLogFile << pPacket->ulSubSeconds << ", ";

        //
        // Now output each of the valid values from the packet.
        //
        for(iLoop = 0; iLoop < NUM_PACKET_CHANNELS; iLoop++)
        {
            //
            // Is this sample valid?
            //
            if(pPacket->ulChannelMask & (1 << iLoop))
            {
                //
                // Yes - output the value to the file.  We calculated this
                // earlier.
                //
                g_pUI->mfLogFile << flVals[iLoop] << ", ";
            }
            else
            {
                //
                // Not a valid sample so just leave a gap in the output.
                //
                g_pUI->mfLogFile << ", ";
            }
        }
        g_pUI->mfLogFile << "\n";
    }

    //
    // Free the packet.
    //
    delete pPacket;
}

//*****************************************************************************
//
// Open a named serial port, configure it appropriately for the application
// and return a handle or INVALID_HANDLE_VALUE if there was an error.
//
//*****************************************************************************
HANDLE OpenSerialPort(char *szName)
{
    char szPortName[80];
    DCB dcbSerialParams;
    HANDLE hSerial;

    //
    // Open the COM port.
    //
    sprintf(szPortName, "\\\\.\\%s", szName);
    hSerial = CreateFile(szPortName, GENERIC_READ, 0, 0,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(hSerial == INVALID_HANDLE_VALUE)
    {
        return(hSerial);
    }

    //
    // Get the current configuration of the serial port.
    //
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if(GetCommState(hSerial, &dcbSerialParams) == 0)
    {
        //
        // The serial port configuration could not be retrieved, so close the
        // port and return an error.
        //
        CloseHandle(hSerial);
        return(INVALID_HANDLE_VALUE);
    }

    //
    // Configure the serial port for 1Mbps, 8-N-1 operation.
    //
    dcbSerialParams.BaudRate = 1000000;
    dcbSerialParams.fBinary = TRUE;
    dcbSerialParams.fParity = FALSE;
    dcbSerialParams.fOutxCtsFlow = FALSE;
    dcbSerialParams.fOutxDsrFlow = FALSE;
    dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
    dcbSerialParams.fDsrSensitivity = FALSE;
    dcbSerialParams.fOutX = FALSE;
    dcbSerialParams.fInX = FALSE;
    dcbSerialParams.fNull = FALSE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.StopBits = ONESTOPBIT;
    if(SetCommState(hSerial, &dcbSerialParams) == 0)
    {
        //
        // The serial port could not be configured, so close the port and
        // return an error.
        //
        CloseHandle(hSerial);
        return(INVALID_HANDLE_VALUE);
    }

    //
    // Set the timeout parameters for the serial port.
    //
    if(SetCommTimeouts(hSerial, &g_NormalTimeouts) == 0)
    {
        //
        // The timeout parameters could not be set, so close the port and
        // return an error.
        //
        CloseHandle(hSerial);
        return(INVALID_HANDLE_VALUE);
    }

    //
    // Tell the caller all is well by returning a valid handle.
    //
    return(hSerial);
}

//*****************************************************************************
//
// Find and open the first Stellaris data logger device attached via USB.
//
// This function will scan for available data logger devices and open the
// first one it finds, returning a handle to the caller.
//
// \return Returns a valid serial handle if a suitable device is found or
// INVALID_HANDLE_VALUE if no data logger device is connected.
//
//*****************************************************************************
HANDLE OpenLoggerDevice(std::string &strName)
{
    DWORD dwGuids;
    LPGUID pGuids;
    HDEVINFO hDevInfo;
    ULONG ulDeviceIndex;
    SP_DEVINFO_DATA sDevInfoData;
    HKEY hKeyDevice;
    char szName[128];
    DWORD dwSize;
    DWORD dwType;
    char szDesc[128];
    DWORD dwNumDevs = 0;
    HANDLE hSerial;

    //
    // The Stellaris data logger devices can be identified in this way ...
    // Class - "Ports"
    // FriendlyName - "Stellaris Data Logger Serial Port (COMx)" (where x is
    //                1,2,3, etc)
    //
    // Setup the data structures for looking at devices that are of the
    // "Ports" device class.  The data logger application appears as a virtual
    // COM port and will show up in this list if attached.
    //
    dwGuids = 0;
    SetupDiClassGuidsFromName("Ports", 0, 0, &dwGuids);
    if(dwGuids < 1)
    {
        return(INVALID_HANDLE_VALUE);
    }
    pGuids = (LPGUID)malloc(dwGuids * sizeof(GUID));
    if(!pGuids)
    {
        return(INVALID_HANDLE_VALUE);
    }
    if(!SetupDiClassGuidsFromName("Ports", pGuids, dwGuids * sizeof(GUID),
                                  &dwGuids))
    {
        free(pGuids);
        return(INVALID_HANDLE_VALUE);
    }
    hDevInfo = SetupDiGetClassDevs(pGuids, NULL, NULL, DIGCF_PRESENT);
    if(hDevInfo == INVALID_HANDLE_VALUE)
    {
        free(pGuids);
        return(INVALID_HANDLE_VALUE);
    }

    //
    // Loop through all of the devices in the "Ports" device class,
    // until we find one that has a "Friendly Name" beginning with "Stellaris
    // ICDI (COM"
    //
    ulDeviceIndex = 0;
    while(1)
    {
        //
        // Enumerate the current device.  If it can't enumerate, then we
        // must be done, so return an error since we have obviously not found
        // a data logger device.
        //
        sDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if(!SetupDiEnumDeviceInfo(hDevInfo, ulDeviceIndex, &sDevInfoData))
        {
            break;
        }

        dwSize = sizeof(szName);
        dwType = 0;
        szName[0] = '\0';

        //
        // Get the port name of this device.
        //
        hKeyDevice = SetupDiOpenDevRegKey(hDevInfo, &sDevInfoData,
                                          DICS_FLAG_GLOBAL, 0, DIREG_DEV,
                                          KEY_READ);
        if(hKeyDevice != INVALID_HANDLE_VALUE)
        {
            if((RegQueryValueEx(hKeyDevice, "PortName", NULL, &dwType,
               (PBYTE)szName, &dwSize) != ERROR_SUCCESS) ||
               (dwType != REG_SZ))
            {
                szName[0] = '\0';
            }
            RegCloseKey(hKeyDevice);
        }

        dwSize = sizeof(szDesc);
        dwType = 0;
        szDesc[0] = '\0';
        if(SetupDiGetDeviceRegistryProperty(hDevInfo, &sDevInfoData,
                                            SPDRP_FRIENDLYNAME, &dwType,
                                            (PBYTE)szDesc, dwSize, &dwSize) &&
                                            (dwType == REG_SZ))
        {
            //
            // Is this device a data logger?
            //
            if(_strnicmp(szDesc,
                         "Stellaris Data Logger Serial Port (COM", 38) == 0)
            {
                //
                // Open the serial port associated with the device.
                //
                hSerial = OpenSerialPort(szName);

                //
                // Did we open the port successfully?
                //
                if(hSerial)
                {
                    //
                    // Yes - free our GUID table since we won't need it any
                    // more.
                    //
                    free(pGuids);

                    //
                    // Remember the port name.
                    //
                    strName = szName;

                    //
                    // Return the handle.
                    //
                    return(hSerial);
                }

                //
                // We couldn't open this port for some reason so keep looking
                // for another instance and try to open that one instead.
                //
            }
        }
        ulDeviceIndex++;
    }

    //
    // Free up allocated buffers.
    //
    free(pGuids);

    //
    // If we get here, we didn't find the device we are looking for so return
    // an error.
    //
    return(INVALID_HANDLE_VALUE);
}

//*****************************************************************************
//
// Determine if a data logger device is currently attached.
//
// This function will scan for available data logger devices report whether
// or not one is found.
//
// \return Returns TRUE if a data logger device is found and FALSE otherwise.
//
//*****************************************************************************
BOOL LoggerDevicePresent(void)
{
    DWORD dwGuids;
    LPGUID pGuids;
    HDEVINFO hDevInfo;
    ULONG ulDeviceIndex;
    SP_DEVINFO_DATA sDevInfoData;
    HKEY hKeyDevice;
    char szName[128];
    DWORD dwSize;
    DWORD dwType;
    char szDesc[128];
    DWORD dwNumDevs = 0;

    //
    // The Stellaris data logger devices can be identified in this way ...
    // Class - "Ports"
    // FriendlyName - "Stellaris Data Logger Serial Port (COMx)" (where x is
    //                1,2,3, etc)
    //
    // Setup the data structures for looking at devices that are of the
    // "Ports" device class.  The data logger application appears as a virtual
    // COM port and will show up in this list if attached.
    //
    dwGuids = 0;
    SetupDiClassGuidsFromName("Ports", 0, 0, &dwGuids);
    if(dwGuids < 1)
    {
        return(FALSE);
    }
    pGuids = (LPGUID)malloc(dwGuids * sizeof(GUID));
    if(!pGuids)
    {
        return(FALSE);
    }
    if(!SetupDiClassGuidsFromName("Ports", pGuids, dwGuids * sizeof(GUID),
                                  &dwGuids))
    {
        free(pGuids);
        return(FALSE);
    }
    hDevInfo = SetupDiGetClassDevs(pGuids, NULL, NULL, DIGCF_PRESENT);
    if(hDevInfo == INVALID_HANDLE_VALUE)
    {
        free(pGuids);
        return(FALSE);
    }

    //
    // Loop through all of the devices in the "Ports" device class,
    // until we find one that has a "Friendly Name" beginning with "Stellaris
    // ICDI (COM"
    //
    ulDeviceIndex = 0;
    while(1)
    {
        //
        // Enumerate the current device.  If it can't enumerate, then we
        // must be done, so return an error since we have obviously not found
        // a data logger device.
        //
        sDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if(!SetupDiEnumDeviceInfo(hDevInfo, ulDeviceIndex, &sDevInfoData))
        {
            break;
        }

        dwSize = sizeof(szName);
        dwType = 0;
        szName[0] = '\0';

        //
        // Get the port name of this device.
        //
        hKeyDevice = SetupDiOpenDevRegKey(hDevInfo, &sDevInfoData,
                                          DICS_FLAG_GLOBAL, 0, DIREG_DEV,
                                          KEY_READ);
        if(hKeyDevice != INVALID_HANDLE_VALUE)
        {
            if((RegQueryValueEx(hKeyDevice, "PortName", NULL, &dwType,
               (PBYTE)szName, &dwSize) != ERROR_SUCCESS) ||
               (dwType != REG_SZ))
            {
                szName[0] = '\0';
            }
            RegCloseKey(hKeyDevice);
        }

        dwSize = sizeof(szDesc);
        dwType = 0;
        szDesc[0] = '\0';
        if(SetupDiGetDeviceRegistryProperty(hDevInfo, &sDevInfoData,
                                            SPDRP_FRIENDLYNAME, &dwType,
                                            (PBYTE)szDesc, dwSize, &dwSize) &&
                                            (dwType == REG_SZ))
        {
            //
            // Is this device a data logger?
            //
            if(_strnicmp(szDesc,
                         "Stellaris Data Logger Serial Port (COM", 38) == 0)
            {
                //
                // Yes - tell the caller.
                //
                free(pGuids);
                return(TRUE);
            }
        }
        ulDeviceIndex++;
    }

    //
    // Free up allocated buffers.
    //
    free(pGuids);

    //
    // If we get here, we didn't find the device we are looking for so return
    // an error.
    //
    return(FALSE);
}

//*****************************************************************************
//
// A worker thread that manages the COM port communication in the background.
// This thread is responsible for connecting to the target board, reading
// sample data and reformatting it before sending the information to the main
// thread for logging and UI update.
//
//*****************************************************************************
#ifdef __WIN32
void
#else
void *
#endif
WorkerThread(void *pvData)
{
    static HANDLE hSerial = INVALID_HANDLE_VALUE;
    unsigned char pucBuffer[MAX_LOGGER_PACKET_SIZE];
    tSamplePacket *pNewPacket;
    int iLoop, iReadIndex;
    unsigned short usMask, usCheck;
    unsigned long ulTimestamp;
    DWORD dwToRead, dwRead;
    std::string strPortName;
    tCOMState eState;
    BOOL bRetcode, bMsgToggle, bReading;

    //
    // Tell the user we are trying to find a data logger board.
    //
    Fl::awake(UpdateApplicationStatus, (void *)INDEX_SEARCHING);

    //
    // We keep running this thread until it is killed along with the rest of
    // the application.
    //
    while(1)
    {
        //
        // Have the main loop update the UI to show that we are trying to
        // find and connect to a device.
        //
        eState = STATE_DISCONNECTED;
        bReading = FALSE;

        //
        // First we try to open the virtual COM port associated with the data
        // logger.  This loop keeps trying every second for as long as it takes
        // to find the device and open it successfully.
        //
        while(hSerial == INVALID_HANDLE_VALUE)
        {
            //
            // Try to get a handle for our logger device.
            //
            hSerial = OpenLoggerDevice(strPortName);

            //
            // If we didn't succeed, wait a couple of seconds before trying
            // again.
            //
#ifndef __WIN32
            usleep(2000000);
#else
            Sleep(2000);
#endif
        }

        //
        // At this point, we've opened the serial device so update the UI and
        // start looking for data from the device.
        //
        Fl::awake(UpdateCOMStatus, (void *)strPortName.data());
        Fl::awake(UpdateApplicationStatus, (void *)INDEX_LISTENING);
        bMsgToggle = false;

        //
        // Flush the serial buffers so that we don't read any stale data that
        // may be hanging around.
        //
        PurgeComm(hSerial, (PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR |
                  PURGE_TXCLEAR));

        //
        // Clear any existing serial errors.
        //
        ClearCommError(hSerial, NULL, NULL);

        //
        // The logger will send data at least once per second.  If the sample
        // rate is slower than this, keep alive packets are inserted.  Listen
        // for data with a 2 second timeout.  If we don't get a good packet in
        // this period, assume the application is not configured to send data
        // to the PC and warn the user.
        //
        eState = STATE_WAIT_HDR_1;
        dwToRead = 1;
        iReadIndex = 0;

        //
        // Keep reading until the device disconnects.
        //
        while(eState != STATE_DISCONNECTED)
        {
            //
            // Read as many bytes as we have been asked to read into the packet
            // buffer at the position requested.
            //
            bRetcode = ReadFile(hSerial, &pucBuffer[iReadIndex],
                                dwToRead, &dwRead, NULL);

            //
            // Now that we've read something or, at least, the read function
            // completed, determine what to do next.  First we deal with the
            // cases that are the same regardless of our current state.
            //

            //
            // Was an error reported while attempting to receive?
            //
            if(!bRetcode)
            {
                //
                // We move back to disconnected state and try to connect
                // again on any communication error.
                //
                eState = STATE_DISCONNECTED;
                break;
            }
            else
            {
                //
                // Did we receive the expected number of bytes?
                //
                if(dwToRead != dwRead)
                {
                    //
                    // No - we read less than expected.  Since we know that no
                    // error occurred, this indicates a timeout which likely
                    // indicates that we've lost sync, possibly due to an
                    // aliased packet start marker inside the data while
                    // initially trying to set up communication.  Unfortunately
                    // Windows doesn't report any error from ReadFile in cases
                    // where the USB device is disconnected so we have to check
                    // here to determine whether or not the device is still
                    // there.  If it is, we jump back and start looking for
                    // the next header.  If it is not, we disconnect and try to
                    // reconnect.
                    //
                    if(LoggerDevicePresent())
                    {
                        eState = STATE_WAIT_HDR_1;
                        iReadIndex = 0;
                        bReading = FALSE;

                        //
                        // Toggle the string in the UI status line to tell the
                        // user what to do to set up data transfer to the PC.
                        //
                        bMsgToggle = !bMsgToggle;
                        Fl::awake(UpdateApplicationStatus,
                                  (bMsgToggle ?
                                   (void *)INDEX_SELECT_PC_SAVE :
                                   (void *)INDEX_LISTENING));
                        continue;
                    }
                    else
                    {
                        eState = STATE_DISCONNECTED;
                        break;
                    }
                }
            }

            //
            // Now perform state-dependent processing.  At this point, we know
            // there was no error in our last attempt to read data and that
            // we read the number of bytes expected so these checks are
            // absent below.
            //
            switch(eState)
            {
                //
                // We're waiting for the first byte of the packet header.  Did
                // we receive it?
                //
                case STATE_WAIT_HDR_1:
                {
                    //
                    // Did we read a 'Q'?
                    //
                    if(pucBuffer[0] == 'Q')
                    {
                        eState = STATE_WAIT_HDR_2;
                        iReadIndex = 1;
                    }
                    else
                    {
                        //
                        // If the byte was not 'Q', we just remain in
                        // the same state and keep reading bytes until
                        // we find a 'Q'.
                        //
                    }
                 }
                break;

                //
                // We're waiting for the first byte of the packet header.  Did
                // we receive it?
                //
                case STATE_WAIT_HDR_2:
                {
                    //
                    // Did we read the expected 'S'?
                    //
                    if(pucBuffer[1] == 'S')
                    {
                        //
                        // Yes - move on to read the 8 bytes containing
                        // the timestamp and mask.
                        //
                        eState = STATE_READ_TIMESTAMP;
                        iReadIndex = 2;
                        dwToRead = 8;
                    }
                    else
                    {
                        //
                        // If the byte was not 'S', we move back to
                        // looking for the start of the header again.
                        //
                        eState = STATE_WAIT_HDR_1;
                        iReadIndex = 0;
                    }
                }
                break;

                //
                // We're waiting for the first byte of the packet header.  Did
                // we receive it?
                //
                case STATE_READ_TIMESTAMP:
                {
                    //
                    // We finished reading the timestamp and mask so now we can
                    // set up to read the sample data and checksum.  Start by
                    // extracting the mask and setting our read counter for
                    // the two trailing checksum bytes.
                    //
                    dwToRead = 2;
                    usMask = (unsigned short)pucBuffer[8] +
                             ((unsigned short)pucBuffer[9] << 8);

                    //
                    // Loop through each of the bits in the mask and add 2 to
                    // our read counter for every bit that is set.
                    //
                    for(iLoop = 0; iLoop < 15; iLoop++)
                    {
                        dwToRead += (usMask & (1 << iLoop)) ? 2 : 0;
                    }

                    //
                    // Transition to the data reading state and tell the read
                    // function to read into the 11th byte of the buffer.
                    //
                    eState = STATE_READ_DATA;
                    iReadIndex = 10;
                }
                break;

                //
                // We have finished reading the number of bytes requested for
                // the main data sample payload.  Now we check the validity of
                // the packet and, if all is OK, reformat and send to the main
                // thread for logging and display.
                //
                case STATE_READ_DATA:
                {
                    //
                    // Loop through the whole packet read and validate the
                    // checksum.
                    //
                    usCheck = 0;
                    for(iLoop = 0; iLoop < (int)(dwToRead + 10); iLoop += 2)
                    {
                        usCheck += (unsigned short)pucBuffer[iLoop] +
                                   ((unsigned short)pucBuffer[iLoop + 1] << 8);
                    }

                    //
                    // Extract the second timestamp from the received packet.
                    //
                    ulTimestamp = pucBuffer[2] | (pucBuffer[3] << 8) |
                                  (pucBuffer[4] << 16) | (pucBuffer[5] << 24);

                    //
                    // If this packet is valid and we have not yet seen a valid
                    // packet, change the state display to tell the user that
                    // we are now reading data from the board.
                    //
                    if(!usCheck && !bReading)
                    {
                        Fl::awake(UpdateApplicationStatus, INDEX_READING);
                        bReading = TRUE;
                    }

                    //
                    // Does this look like a valid packet that we need to
                    // process?  For this to be the case, the checksum must be
                    // zero and the timestamp must be non-zero (a zero
                    // timestamp marks a keep-alive packet which we ignore).
                    //
                    if(!usCheck && ulTimestamp)
                    {
                        //
                        // This isn't a keep-alive packet and the checksum was
                        // valid so reformat it and send it on to the main
                        // thread for further processing.
                        //
                        pNewPacket = new tSamplePacket;

                        if(pNewPacket)
                        {
                            //
                            // Populate the packet.
                            //
                            pNewPacket->ulTimestamp = ulTimestamp;
                            pNewPacket->ulSubSeconds = pucBuffer[6] +
                                                       (pucBuffer[7] << 8);
                            pNewPacket->ulChannelMask = pucBuffer[8] +
                                                        (pucBuffer[9] << 8);

                            //
                            // Extract samples from the original packet and
                            // reformat them into the packet we send to the
                            // main thread.
                            //
                            dwRead = 10;
                            for(iLoop = 0; iLoop < NUM_PACKET_CHANNELS; iLoop++)
                            {
                                //
                                // Do we have a sample for this channel?
                                //
                                if(pNewPacket->ulChannelMask & (1 << iLoop))
                                {
                                    //
                                    // Yes - copy it into the new packet.
                                    //
                                    pNewPacket->piSamples[iLoop] =
                                            (int)((short)pucBuffer[dwRead] +
                                            (short)(pucBuffer[dwRead + 1] << 8));
                                    dwRead += 2;
                                }
                                else
                                {
                                    //
                                    // No - write 0 to the slot for this
                                    // channel in the packet.
                                    //
                                    pNewPacket->piSamples[iLoop] = 0;
                                }
                            }

                            //
                            // Send the new packet to the main thread and have
                            // it call the packet handler on our behalf.
                            //
                            Fl::awake(HandleNewPacket, (void *)pNewPacket);
                        }
                    }

                    //
                    // Set things up to read the next packet header.
                    //
                    iReadIndex = 0;
                    dwToRead = 1;
                    eState = STATE_WAIT_HDR_1;
                }
                break;
             }
        }

        //
        // If we dropped out here, the device disconnected or we experienced
        // an error so close the handle and go back to try again.
        //
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        Fl::awake(UpdateApplicationStatus, (void *)INDEX_DISCONNECTED);
        Fl::awake(UpdateCOMStatus, (void *)"");
    }
}

//*****************************************************************************
//
// This program finds Stellaris boards that provide the locator service on the
// Ethernet.
//
//*****************************************************************************
int
main(int argc, char *argv[])
{
#ifndef __WIN32
    pthread_t thread;
#endif

    //
    // If running on Windows, initialize the COM library (required for multi-
    // threading).
    //
#ifdef __WIN32
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif

    //
    // Create the main window.
    //
    g_pUI = new CLoggerUI(100, 100);

    //
    // Prepare FLTK for double-buffered operation.
    //
    Fl::visual(FL_DOUBLE|FL_INDEX);

    //
    // Initialize the various controls in the window.
    //
    g_pUI->InitUI();

    //
    // Construct our channel information array.
    //
    ConstructChannelInfo();

    //
    // Show the window.
    //
    g_pUI->show();

    //
    // Prepare FLTK for multi-threaded and double buffered operation.
    //
    Fl::lock();

    //
    // Create the worker thread that performs the COM port scan and
    // handles communication with the data logger board in the background.
    //
#ifdef __WIN32
    _beginthread(WorkerThread, 0, 0);
#else
    pthread_create(&thread, 0, WorkerThread, 0);
#endif

    //
    // Handle the FLTK events in the main thread.
    //
    return(Fl::run());
}

//*****************************************************************************
//
// The button handler for the channel enable/disable buttons.
//
//*****************************************************************************
void HandleButton(Fl_Light_Button *hButton, void *pvData)
{
    int iLoop;

    //
    // First, find out which channel this button controls.
    //
    for(iLoop = 0; iLoop < NUM_PACKET_CHANNELS; iLoop++)
    {
        if(g_ChannelControls[iLoop].pButton == hButton)
        {
            unsigned int uiVisible;
            CStripChart *pChart = g_ChannelControls[iLoop].pStripChart;

            //
            // We found the button.  Enable or disable the channel as
            // required.
            //
            uiVisible = pChart->GetChannelVisibility();
            if(hButton->value())
            {
                uiVisible |= g_ChannelControls[iLoop].ulMask;
            }
            else
            {
                uiVisible &= ~g_ChannelControls[iLoop].ulMask;
            }
            pChart->SetChannelVisibility(uiVisible);

            //
            // We've handled this button so no need to continue the loop.
            //
            return;
        }
    }
}
