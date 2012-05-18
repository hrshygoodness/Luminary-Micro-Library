//*****************************************************************************
//
// gui_handlers.cxx - GUI handler functions.
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

#include <libgen.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "gui_handlers.h"
#include "os.h"
#include "uart_handler.h"
#include "gui.h"
#include "can_proto.h"
#include "bdc-comm.h"

#ifdef __WIN32
#include <setupapi.h>
#else
#include <dirent.h>
#endif

//*****************************************************************************
//
// The maximum number of CAN IDs that can be on the network.
//
//*****************************************************************************
#define MAX_CAN_ID        64

//*****************************************************************************
//
// The current board status and board states for all devices.
//
//*****************************************************************************
tBoardStatus g_sBoardStatus;
tBoardState g_sBoardState[MAX_CAN_ID];

//*****************************************************************************
//
// Local module global to manage PStat History Legend buffer
//
//*****************************************************************************
static void *pvPeriodicStatusLegendBuffer = NULL;

//*****************************************************************************
//
// This array holds the strings used for periodic status messages in the GUI.
//
//*****************************************************************************
tPStatMsgEncodes g_sPStatMsgs[] =
{
    { "END MSG", "msg-end", LM_PSTAT_END },
    { "VOUT B0 (%)", "vout-b0", LM_PSTAT_VOLTOUT_B0 },
    { "VOUT B1 (%)", "vout-b1", LM_PSTAT_VOLTOUT_B1 },
    { "VBUS B0", "vbus-b0", LM_PSTAT_VOLTBUS_B0 },
    { "VBUS B1", "vbut-b1", LM_PSTAT_VOLTBUS_B1 },
    { "Current B0", "curr-b0", LM_PSTAT_CURRENT_B0 },
    { "Current B1", "curr-b1", LM_PSTAT_CURRENT_B1 },
    { "Temp B0", "temp-b0", LM_PSTAT_TEMP_B0 },
    { "Temp B1", "temp-b1", LM_PSTAT_TEMP_B1 },
    { "Pos B0", "pos-b0", LM_PSTAT_POS_B0 },
    { "Pos B1", "pos-b1", LM_PSTAT_POS_B1 },
    { "Pos B2", "pos-b2", LM_PSTAT_POS_B2 },
    { "Pos B3", "pos-b3", LM_PSTAT_POS_B3 },
    { "Speed B0", "spd-b0", LM_PSTAT_SPD_B0 },
    { "Speed B1", "spd-b1", LM_PSTAT_SPD_B1 },
    { "Speed B2", "spd-b2", LM_PSTAT_SPD_B2 },
    { "Speed B3", "spd-b3", LM_PSTAT_SPD_B3 },
    { "Limit (NoCLR)", "lim-nclr", LM_PSTAT_LIMIT_NCLR },
    { "Limit (CLR)", "lim-clr", LM_PSTAT_LIMIT_CLR },
    { "Fault", "fault", LM_PSTAT_FAULT },
    { "Stky Fault (NoCLR)", "sfault-nclr", LM_PSTAT_STKY_FLT_NCLR },
    { "Stky Fault (CLR)", "sfault-clr", LM_PSTAT_STKY_FLT_CLR },
    { "VOUT B0 (V)", "vout2-b0", LM_PSTAT_VOUT_B0 },
    { "VOUT B1 (V)", "vout2-b1", LM_PSTAT_VOUT_B1 },
    { "Current Faults", "flt-curr", LM_PSTAT_FLT_COUNT_CURRENT },
    { "Temp Faults", "flt-temp", LM_PSTAT_FLT_COUNT_TEMP },
    { "Voltage Faults", "flt-vbus", LM_PSTAT_FLT_COUNT_VOLTBUS },
    { "Gate Faults", "flt-gate", LM_PSTAT_FLT_COUNT_GATE },
    { "Comm Faults", "flt-comm", LM_PSTAT_FLT_COUNT_COMM },
    { "CAN Status", "cansts", LM_PSTAT_CANSTS },
    { "CAN RxErr", "canerr-b0", LM_PSTAT_CANERR_B0 },
    { "CAN TXErr", "canerr-b1", LM_PSTAT_CANERR_B1 },
    { 0, 0, 0 }
};


//*****************************************************************************
//
// Strings for the PStat legend.
//
//*****************************************************************************
static const char *g_ppcLegentTitles[] =
{
    "   Vout    ",
    "   Vbus    ",
    "  Current  ",
    "Temperature",
    " Position  ",
    "   Speed   ",
    "Curr_Faults",
    "Temp Faults",
    "Vbus Faults",
    "Gate Faults",
    "Comm Faults",
    "CAN Status ",
    "CAN_RX Err ",
    "CAN TX Err ",
    "   Limit   ",
    "   Faults  ",
    0
};

//*****************************************************************************
//
// Used when sorting port numbers for display.
//
//*****************************************************************************
static int
PortNumCompare(const void *a, const void *b)
{
    return(*(unsigned long *)a - *(unsigned long *)b);
}

//*****************************************************************************
//
// Used when selecting UART devices under Linux.
//
//*****************************************************************************
#ifndef __WIN32
static int
DevSelector(const struct dirent64 *pEntry)
{
    if((strncmp(pEntry->d_name, "ttyS", 4) == 0) ||
       (strncmp(pEntry->d_name, "ttyUSB", 6) == 0))
    {
        return(1);
    }
    else
    {
        return(0);
    }
}
#endif

//*****************************************************************************
//
// Fill the drop down box for the UARTs.
//
//*****************************************************************************
int
GUIFillCOMPortDropDown(void)
{
#ifdef __WIN32
#define STRING_BUFFER_SIZE      256
#define MAX_PORTS               256
    unsigned long pulPorts[MAX_PORTS], ulNumPorts = 0;
    char *pcBuffer, *pcData;
    SP_DEVINFO_DATA sDeviceInfoData;
    HDEVINFO sDeviceInfoSet;
    DWORD dwRequiredSize = 0, dwLen;
    HKEY hKeyDevice;
    int iDev = 0;

    //
    // Windows stuff for enumerating COM ports
    //
    SetupDiClassGuidsFromNameA("Ports", 0, 0, &dwRequiredSize);
    if(dwRequiredSize < 1)
    {
        return(0);
    }

    pcData = (char *)malloc(dwRequiredSize * sizeof(GUID));
    if(!pcData)
    {
        return(0);
    }

    if(!SetupDiClassGuidsFromNameA("Ports", (LPGUID)pcData,
                                   dwRequiredSize * sizeof(GUID),
                                   &dwRequiredSize))
    {
        free(pcData);
        return(0);
    }

    sDeviceInfoSet = SetupDiGetClassDevs((LPGUID)pcData, NULL, NULL,
                                         DIGCF_PRESENT);
    if(sDeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        free(pcData);
        return(0);
    }

    //
    // Allocate space for a string buffer
    //
    pcBuffer = (char *)malloc(STRING_BUFFER_SIZE);
    if(!pcBuffer)
    {
        free(pcData);
        return(0);
    }

    //
    // Loop through the device set looking for serial ports
    //
    for(iDev = 0;
        sDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA),
            SetupDiEnumDeviceInfo(sDeviceInfoSet, iDev, &sDeviceInfoData);
        iDev++)
    {
        //
        // Dont exceed the amount of space in the caller supplied list
        //
        if(ulNumPorts == MAX_PORTS)
        {
            break;
        }

        if(SetupDiGetDeviceRegistryProperty(sDeviceInfoSet, &sDeviceInfoData,
                                            SPDRP_UPPERFILTERS, 0,
                                            (BYTE *)pcBuffer,
                                            STRING_BUFFER_SIZE, 0))
        {
            if(strcmp(pcBuffer, "serenum") != 0)
            {
                continue;
            }
        }
        else
        {
            continue;
        }

        if(SetupDiGetDeviceRegistryProperty(sDeviceInfoSet, &sDeviceInfoData,
                                            SPDRP_FRIENDLYNAME, 0,
                                            (BYTE *)pcBuffer,
                                            STRING_BUFFER_SIZE, 0))
        {
        }
        else
        {
            continue;
        }

        hKeyDevice = SetupDiOpenDevRegKey(sDeviceInfoSet, &sDeviceInfoData,
                                          DICS_FLAG_GLOBAL, 0, DIREG_DEV,
                                          KEY_READ);
        if(hKeyDevice != INVALID_HANDLE_VALUE)
        {
            dwLen = STRING_BUFFER_SIZE;
            RegQueryValueEx(hKeyDevice, "portname", 0, 0, (BYTE *)pcBuffer,
                            &dwLen);

            //
            // A device was found, make sure it looks like COMx, and
            // then extract the numeric part and store it in the list.
            //
            if(!strncmp(pcBuffer, "COM", 3))
            {
                pulPorts[ulNumPorts++] = atoi(&pcBuffer[3]);
            }

            RegCloseKey(hKeyDevice);
        }
    }

    //
    // Sort the COM port numbers.
    //
    qsort(pulPorts, ulNumPorts, sizeof(pulPorts[0]), PortNumCompare);

    //
    // Loop through the ports, adding them to the COM port dropdown.
    //
    for(iDev = 0; iDev < ulNumPorts; iDev++)
    {
        snprintf(pcBuffer, STRING_BUFFER_SIZE, "%d", pulPorts[iDev]);
        g_pSelectCOM->add(pcBuffer);
    }

    //
    // Free up allocated buffers and return to the caller.
    //
    free(pcBuffer);
    free(pcData);

    //
    // Select the first COM port.
    //
    if(ulNumPorts != 0)
    {
        g_pSelectCOM->value(0);
        snprintf(g_szCOMName, sizeof(g_szCOMName), "\\\\.\\COM%s",
                 g_pSelectCOM->text(0));
    }

    //
    // Return the number of COM ports found.
    //
    return(ulNumPorts);
#else
    unsigned long ulIdx, ulCount;
    struct dirent64 **pEntries;

    //
    // Scan the /dev directory for serial ports.
    //
    ulCount = scandir64("/dev", &pEntries, DevSelector, alphasort64);

    //
    // Return that there are no serial ports if none were found or if an error
    // was encountered.
    //
    if((ulCount == 0) || (ulCount == -1))
    {
        return(0);
    }

    //
    // Add the found serial ports (which will already be sorted alphabetically)
    // to the drop down selector.
    //
    for(ulIdx = 0; ulIdx < ulCount; ulIdx++)
    {
        g_pSelectCOM->add(pEntries[ulIdx]->d_name);
    }

    //
    // Free the array of directory entries that were found.
    //
    free(pEntries);

    //
    // Select the first serial port by default.
    //
    g_pSelectCOM->value(0);
    strcpy(g_szCOMName, "/dev/");
    strcat(g_szCOMName, g_pSelectCOM->text(0));

    //
    // Success.
    //
    return(ulCount);
#endif
}

//*****************************************************************************
//
// Updates the range of the position slider based on the position reference  in
// use.
//
//*****************************************************************************
void
GUIUpdatePositionSlider(void)
{
    //
    // Do nothing if the current board is not in position control mode.
    //
    if(g_sBoardState[g_ulID].ulControlMode != LM_STATUS_CMODE_POS)
    {
        return;
    }

    //
    // Set the limits for the value and slider box based on the position
    // reference in use.
    //
    if(g_pConfigLimitSwitches->value() == 1)
    {
        double dFwd, dRev, dTemp;

        dFwd = g_pConfigFwdLimitValue->value();
        dRev = g_pConfigRevLimitValue->value();

        if(dRev > dFwd)
        {
            dTemp = dFwd;
            dFwd = dRev;
            dRev = dTemp;
        }

        g_pModeSetBox->range(dRev, dFwd);

        g_pModeSetSlider->range(dRev, dFwd);
        g_pModeSetSlider->activate();
    }
    else if(g_pReference->value() == 1)
    {
        g_pModeSetBox->range(0, g_pConfigPOTTurns->value());

        g_pModeSetSlider->range(0, g_pConfigPOTTurns->value());
        g_pModeSetSlider->activate();
    }
    else
    {
        g_pModeSetBox->range(-32767, 32767);

        g_pModeSetSlider->range(-32767, 32767);
        g_pModeSetSlider->deactivate();
    }

    g_pModeSetBox->precision(3);

    g_pModeSetSlider->step(0.001);
}

//*****************************************************************************
//
// Sets up the enable/disable and ranges on the GUI elements based on the
// control mode.
//
//*****************************************************************************
void
GUIControlUpdate(void)
{
    //
    // Enable the default GUI items.
    //
    g_pSelectBoard->activate();
    g_pSelectMode->activate();
    g_pModeSync->activate();
    g_pModeSetBox->activate();
    g_pModeSetSlider->activate();
    g_pConfigEncoderLines->activate();
    g_pConfigPOTTurns->activate();
    g_pConfigMaxVout->activate();
    g_pConfigFaultTime->activate();
    g_pConfigStopJumper->activate();
    g_pConfigStopBrake->activate();
    g_pConfigStopCoast->activate();
    g_pConfigLimitSwitches->activate();
    g_pSystemAssignValue->activate();
    g_pSystemAssign->activate();
    g_pSystemHalt->activate();
    g_pSystemResume->activate();
    g_pSystemReset->activate();
    g_pMenuUpdate->activate();
    g_pSystemButtonExtendedStatus->activate();
    g_pPeriodicSelectStatusMessage->activate();
    g_pPeriodicStatusFilterUsed->activate();


    //
    // Get the current control mode.
    //
    strcpy(g_argv[0], "stat");
    strcpy(g_argv[1], "cmode");
    CmdStatus(2, g_argv);

    //
    // Determine which mode is in use.
    //
    switch(g_sBoardState[g_ulID].ulControlMode)
    {
        //
        // Voltage control mode.
        //
        case LM_STATUS_CMODE_VOLT:
        {
            //
            // Deactivate unneeded GUI items.
            //
            g_pModeCompRamp->deactivate();
            g_pReference->deactivate();
            g_pModeP->deactivate();
            g_pModeI->deactivate();
            g_pModeD->deactivate();

            //
            // Activate used GUI items.
            //
            g_pModeRamp->activate();
            g_pModeSetSlider->activate();

            //
            // Set new limits for the "Value" box.
            //
            g_pModeSetBox->range(-100, 100);
            g_pModeSetBox->precision(0);

            //
            // Set the new limits for the slider.
            //
            g_pModeSetSlider->range(-100, 100);
            g_pModeSetSlider->step(1);

            //
            // Update the label for the set value box.
            //
            g_pModeSetBox->label("Value (%):");

            //
            // Get the voltage setpoint.
            //
            strcpy(g_argv[0], "volt");
            strcpy(g_argv[1], "set");
            CmdVoltage(2, g_argv);

            //
            // Get the current ramp value.
            //
            strcpy(g_argv[1], "ramp");
            CmdVoltage(2, g_argv);

            //
            // Unselect the reference chooser.
            //
            g_pReference->value(0);

            //
            // Get the current P value.
            //
            g_pModeP->value(0.0);

            //
            // Get the current I value.
            //
            g_pModeI->value(0.0);

            //
            // Get the current D value.
            //
            g_pModeD->value(0.0);

            //
            // Done.
            //
            break;
        }

        //
        // Voltage compensation mode.
        //
        case LM_STATUS_CMODE_VCOMP:
        {
            //
            // Deactivate unneeded GUI items.
            //
            g_pReference->deactivate();
            g_pModeP->deactivate();
            g_pModeI->deactivate();
            g_pModeD->deactivate();

            //
            // Activate used GUI items.
            //
            g_pModeRamp->activate();
            g_pModeCompRamp->activate();
            g_pModeSetSlider->activate();

            //
            // Set new limits for the "Value" box.
            //
            g_pModeSetBox->range(-24, 24);
            g_pModeSetBox->precision(2);

            //
            // Set the new limits for the slider.
            //
            g_pModeSetSlider->range(-24, 24);
            g_pModeSetSlider->step(0.01);

            //
            // Update the label for the set value box.
            //
            g_pModeSetBox->label("Value (V):");

            //
            // Get the voltage setpoint.
            //
            strcpy(g_argv[0], "vcomp");
            strcpy(g_argv[1], "set");
            CmdVComp(2, g_argv);

            //
            // Get the current ramp value.
            //
            strcpy(g_argv[1], "ramp");
            CmdVComp(2, g_argv);

            //
            // Get the current compensation rate value.
            //
            strcpy(g_argv[1], "comp");
            CmdVComp(2, g_argv);

            //
            // Unselect the reference chooser.
            //
            g_pReference->value(0);

            //
            // Get the current P value.
            //
            g_pModeP->value(0.0);

            //
            // Get the current I value.
            //
            g_pModeI->value(0.0);

            //
            // Get the current D value.
            //
            g_pModeD->value(0.0);

            //
            // Done.
            //
            break;
        }

        //
        // Current control mode.
        //
        case LM_STATUS_CMODE_CURRENT:
        {
            //
            // Deactivate unneeded GUI items.
            //
            g_pModeRamp->deactivate();
            g_pModeCompRamp->deactivate();
            g_pReference->deactivate();

            //
            // Activate used GUI items.
            //
            g_pModeP->activate();
            g_pModeI->activate();
            g_pModeD->activate();
            g_pModeSetSlider->activate();

            //
            // Set new limits for the "Value" box.
            //
            g_pModeSetBox->range(-40, 40);
            g_pModeSetBox->precision(1);

            //
            // Set the new limits for the slider.
            //
            g_pModeSetSlider->range(-40, 40);
            g_pModeSetSlider->step(0.1);

            //
            // Update the label for the set value box.
            //
            g_pModeSetBox->label("Value (A):");

            //
            // Get the current setpoint.
            //
            strcpy(g_argv[0], "cur");
            strcpy(g_argv[1], "set");
            CmdCurrent(2, g_argv);

            //
            // Get the current ramp value.
            //
            g_pModeRamp->value(0.0);

            //
            // Unselect the reference chooser.
            //
            g_pReference->value(0);

            //
            // Get the current P value.
            //
            strcpy(g_argv[1], "p");
            CmdCurrent(2, g_argv);

            //
            // Get the current I value.
            //
            strcpy(g_argv[1], "i");
            CmdCurrent(2, g_argv);

            //
            // Get the current D value.
            //
            strcpy(g_argv[1], "d");
            CmdCurrent(2, g_argv);

            //
            // Done.
            //
            break;
        }

        //
        // Speed control mode.
        //
        case LM_STATUS_CMODE_SPEED:
        {
            //
            // Deactivate unneeded GUI items.
            //
            g_pModeRamp->deactivate();
            g_pModeCompRamp->deactivate();
            g_pReferencePotentiometer->hide();

            //
            // Activate used GUI items.
            //
            g_pModeP->activate();
            g_pModeI->activate();
            g_pModeD->activate();
            g_pReference->activate();
            g_pReferenceInvEncoder->show();
            g_pReferenceQuadEncoder->show();
            g_pModeSetSlider->activate();

            //
            // Set new limits for the "Value" box.
            //
            g_pModeSetBox->range(-32767, 32767);
            g_pModeSetBox->precision(0);

            //
            // Set the new limits for the slider.
            //
            g_pModeSetSlider->range(-32767, 32767);
            g_pModeSetSlider->step(1);

            //
            // Update the label for the set value box.
            //
            g_pModeSetBox->label("Value (rpm):");

            //
            // Get the speed setpoint.
            //
            strcpy(g_argv[0], "speed");
            strcpy(g_argv[1], "set");
            CmdSpeed(2, g_argv);

            //
            // Get the current ramp value.
            //
            g_pModeRamp->value(0.0);

            //
            // Get the current P value.
            //
            strcpy(g_argv[1], "p");
            CmdSpeed(2, g_argv);

            //
            // Get the current I value.
            //
            strcpy(g_argv[1], "i");
            CmdSpeed(2, g_argv);

            //
            // Get the current D value.
            //
            strcpy(g_argv[1], "d");
            CmdSpeed(2, g_argv);

            //
            // Update the reference chooser.
            //
            strcpy(g_argv[1], "ref");
            CmdSpeed(2, g_argv);

            //
            // Select the encoder reference if no reference has been selected.
            //
            if(g_pReference->value() == -1)
            {
                strcpy(g_argv[2], "0");
                CmdSpeed(3, g_argv);
                CmdSpeed(2, g_argv);
            }

            //
            // Done.
            //
            break;
        }

        //
        // Position control mode.
        //
        case LM_STATUS_CMODE_POS:
        {
            //
            // Deactivate unneeded GUI items.
            //
            g_pModeRamp->deactivate();
            g_pModeCompRamp->deactivate();
            g_pReferenceInvEncoder->hide();
            g_pReferenceQuadEncoder->hide();

            //
            // Activate used GUI items.
            //
            g_pModeP->activate();
            g_pModeI->activate();
            g_pModeD->activate();
            g_pModeSetSlider->activate();
            g_pReference->activate();
            g_pReferencePotentiometer->show();

            //
            // Update the label for the set value box.
            //
            g_pModeSetBox->label("Value:");

            //
            // Get the current ramp value.
            //
            g_pModeRamp->value(0.0);

            //
            // Get the current P value.
            //
            strcpy(g_argv[1], "p");
            CmdPosition(2, g_argv);

            //
            // Get the current I value.
            //
            strcpy(g_argv[1], "i");
            CmdPosition(2, g_argv);

            //
            // Get the current D value.
            //
            strcpy(g_argv[1], "d");
            CmdPosition(2, g_argv);

            //
            // Update the reference chooser.
            //
            strcpy(g_argv[1], "ref");
            CmdPosition(2, g_argv);

            //
            // Select the encoder reference if no reference has been selected.
            //
            if(g_pReference->value() == -1)
            {
                strcpy(g_argv[2], "0");
                CmdPosition(3, g_argv);
                CmdPosition(2, g_argv);
            }

            //
            // Update the position slider.
            //
            GUIUpdatePositionSlider();

            //
            // Get the position setpoint.
            //
            strcpy(g_argv[0], "pos");
            strcpy(g_argv[1], "set");
            CmdPosition(2, g_argv);

            //
            // Done.
            //
            break;
        }
    }
}

//*****************************************************************************
//
// Updates the status at the bottom of the GUI.
//
//*****************************************************************************
void
GUIConfigUpdate(void)
{
    //
    // Get the hardware version.
    //
    strcpy(g_argv[0], "system");
    strcpy(g_argv[1], "hwver");
    CmdSystem(2, g_argv);

    //
    // Get the firmware version
    //
    strcpy(g_argv[1], "version");
    CmdSystem(2, g_argv);

    //
    // Get the board information.
    //
    strcpy(g_argv[1], "query");
    CmdSystem(2, g_argv);

    //
    // Get the number of encoder lines.
    //
    strcpy(g_argv[0], "config");
    strcpy(g_argv[1], "lines");
    CmdConfig(2, g_argv);

    //
    // Get the number of POT turns.
    //
    strcpy(g_argv[1], "turns");
    CmdConfig(2, g_argv);

    //
    // Get the brake action.
    //
    strcpy(g_argv[1], "brake");
    CmdConfig(2, g_argv);

    //
    // Get the soft limit switch enable.
    //
    strcpy(g_argv[1], "limit");
    CmdConfig(2, g_argv);

    //
    // Get the forward soft limit setting.
    //
    strcpy(g_argv[1], "fwd");
    CmdConfig(2, g_argv);

    //
    // Get the forward reverse limit setting.
    //
    strcpy(g_argv[1], "rev");
    CmdConfig(2, g_argv);

    //
    // Get the maximum Vout.
    //
    strcpy(g_argv[1], "maxvout");
    CmdConfig(2, g_argv);

    //
    // Get the fault time.
    //
    strcpy(g_argv[1], "faulttime");
    CmdConfig(2, g_argv);

    //
    // Query the Periodic Message enables/periods.
    //
    strcpy(g_argv[0], "pstat");
    strcpy(g_argv[1], "int");
    strcpy(g_argv[2], "0");
    CmdPStatus(3, g_argv);
    strcpy(g_argv[2], "1");
    CmdPStatus(3, g_argv);
    strcpy(g_argv[2], "2");
    CmdPStatus(3, g_argv);
    strcpy(g_argv[2], "3");
    CmdPStatus(3, g_argv);

    //
    // Query the Periodic Message configurations.
    //
    strcpy(g_argv[1], "cfg");
    strcpy(g_argv[2], "0");
    CmdPStatus(3, g_argv);
    strcpy(g_argv[2], "1");
    CmdPStatus(3, g_argv);
    strcpy(g_argv[2], "2");
    CmdPStatus(3, g_argv);
    strcpy(g_argv[2], "3");
    CmdPStatus(3, g_argv);

    //
    // Trigger a widget update for PStatus.
    //
    GUIPeriodicStatusDropDownStatusMessage();
}

//*****************************************************************************
//
// This function handles connecting to the selected UART.
//
//*****************************************************************************
void
GUIConnect(void)
{
    int iIndex;

    //
    // Get the current selected item.
    //
    iIndex = g_pSelectCOM->value();

    //
    // Get the name of this COM port.
    //
#ifdef __WIN32
    snprintf(g_szCOMName, sizeof(g_szCOMName), "\\\\.\\COM%s",
             g_pSelectCOM->text(iIndex));
#else
    snprintf(g_szCOMName, sizeof(g_szCOMName), "/dev/%s",
             g_pSelectCOM->text(iIndex));
#endif

    //
    // Open the connection to the COM port.
    //
    if(OpenUART(g_szCOMName, 115200))
    {
        fl_alert("Could not connect to specified COM port.");
        return;
    }

    //
    // Indicate that the COM port is open.
    //
    g_bConnected = true;

    //
    // Update the menu with the status.
    //
    g_pMenuStatus->label("&Status: Connected");
    g_pMenuStatusButton->label("&Disconnect...");

    //
    // Enable device recovery, device ID assignment controls, and the enumerate
    // button whenever there is an open COM port.
    //
    g_pMenuRecover->activate();
    g_pSystemAssignValue->activate();
    g_pSystemAssignValue->value("1");
    g_pSystemAssign->activate();
    g_pSystemEnumerate->activate();

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Remove all existing items in the 'Boards' drop down.
    //
    for(iIndex = 0; iIndex < g_pSelectBoard->size(); iIndex++)
    {
        //
        // Remove the item.
        //
        g_pSelectBoard->remove(iIndex);
    }

    //
    // Clear the drop down list (clean up after removing the items).
    //
    g_pSelectBoard->clear();

    //
    // Find the Jaguars in the network.
    //
    FindJaguars();

    //
    // Make sure that there are boards connected.
    //
    if(g_pSelectBoard->size() == 0)
    {
        //
        // Add a dummy entry to the board ID select.
        //
        g_pSelectBoard->add("--");
        g_pSelectBoard->value(0);

        //
        // Select the System tab.
        //
        g_pTabMode->hide();
        g_pTabConfiguration->hide();
        g_pTabSystem->show();

        //
        // Set the default board ID to 1.
        //
        g_ulID = 1;

        //
        // Force FLTK to redraw the UI.
        //
        Fl::redraw();

        //
        // Done.
        //
        return;
    }

    //
    // Get the ID of the first board in the chain and set as the active.
    //
    g_ulID = strtoul(g_pSelectBoard->text(0), 0, 0);

    //
    // Select the first item in the list by default.
    //
    g_pSelectBoard->value(0);

    //
    // Set the value of the board ID update field to the same as the
    // current selected item.
    //
    g_pSystemAssignValue->value(g_pSelectBoard->text(0));

    //
    // Update the configuration.
    //
    GUIConfigUpdate();

    //
    // Update the controls.
    //
    GUIControlUpdate();

    //
    // Force FLTK to redraw the UI.
    //
    Fl::redraw();

    //
    // Set the global flag for the board status thread to active.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// This function handles disconnecting from the UART and closing down all
// connections.
//
//*****************************************************************************
void
GUIDisconnectAndClear(void)
{
    int iIndex;

    //
    // Indicate that there is no connection.
    //
    g_bConnected = false;

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Wait some time to make sure all threads have stopped using the UART.
    //
    usleep(50000);

    //
    // Close the connection to the current COM port.
    //
    CloseUART();

    //
    // Update the menu with the status.
    //
    g_pMenuStatus->label("&Status: Disconnected");
    g_pMenuStatusButton->label("&Connect...");

    //
    // Zero out the board state structure.
    //
    for(iIndex = 0; iIndex < MAX_CAN_ID; iIndex++)
    {
        g_sBoardState[iIndex].ulControlMode = LM_STATUS_CMODE_VOLT;
    }

    //
    // Deactivate unneeded GUI items.
    //
    g_pSelectBoard->deactivate();
    g_pSelectMode->deactivate();
    g_pModeSync->deactivate();
    g_pModeSetBox->deactivate();
    g_pModeSetSlider->deactivate();
    g_pModeRamp->deactivate();
    g_pModeCompRamp->deactivate();
    g_pReference->deactivate();
    g_pModeP->deactivate();
    g_pModeI->deactivate();
    g_pModeD->deactivate();
    g_pConfigEncoderLines->deactivate();
    g_pConfigPOTTurns->deactivate();
    g_pConfigMaxVout->deactivate();
    g_pConfigFaultTime->deactivate();
    g_pConfigStopJumper->deactivate();
    g_pConfigStopBrake->deactivate();
    g_pConfigStopCoast->deactivate();
    g_pConfigLimitSwitches->deactivate();
    g_pConfigFwdLimitLt->deactivate();
    g_pConfigFwdLimitGt->deactivate();
    g_pConfigFwdLimitValue->deactivate();
    g_pConfigRevLimitLt->deactivate();
    g_pConfigRevLimitGt->deactivate();
    g_pConfigRevLimitValue->deactivate();
    g_pSystemAssignValue->deactivate();
    g_pSystemAssign->deactivate();
    g_pSystemHalt->deactivate();
    g_pSystemResume->deactivate();
    g_pSystemReset->deactivate();
    g_pSystemEnumerate->deactivate();
    g_pMenuUpdate->deactivate();
    g_pMenuRecover->deactivate();
    g_pPeriodicSelectStatusMessage->deactivate();
    g_pPeriodicStatusInterval->deactivate();
    g_pPeriodicStatusMessagesList->deactivate();
    g_pPeriodicStatusFilterUsed->deactivate();
    g_pPeriodicStatusRemoveStatus->deactivate();
    g_pPeriodicStatusPayloadList->deactivate();
    g_pPeriodicStatusAddStatus->deactivate();
    g_pSystemButtonExtendedStatus->deactivate();


    //
    // Choose voltage control mode.
    //
    g_pSelectMode->value(0);

    //
    // Update the label for the set value box.
    //
    g_pModeSetBox->label("Value (%):");

    //
    // Set new limits for the "Value" box.
    //
    g_pModeSetBox->range(-100, 100);
    g_pModeSetBox->precision(0);

    //
    // Set the new limits for the slider.
    //
    g_pModeSetSlider->range(-100, 100);
    g_pModeSetSlider->step(1);
    g_pModeSetSlider->value(0);

    //
    // Remove all existing items in the 'Boards' drop down.
    //
    for(iIndex = 0; iIndex < g_pSelectBoard->size(); iIndex++)
    {
        //
        // Remove the item.
        //
        g_pSelectBoard->remove(iIndex);
    }

    //
    // Clear the drop down list (clean up after removing the items).
    //
    g_pSelectBoard->clear();

    //
    // Add something showing that no boards are present.
    //
    g_pSelectBoard->add("--");
    g_pSelectBoard->value(0);

    //
    // Clear all of the items in the Mode tab and set to voltage mode
    // (default).
    //
    g_pModeSetBox->value(0.0);
    g_pModeSetSlider->value(0);
    g_pModeRamp->value(0.0);
    g_pModeP->value(0.0);
    g_pModeI->value(0.0);
    g_pModeD->value(0.0);

    //
    // Clear all of the items in the Config tab.
    //
    g_pConfigEncoderLines->value(0.0);
    g_pConfigPOTTurns->value(0.0);
    g_pConfigMaxVout->value(0.0);
    g_pConfigFaultTime->value(0.0);

    //
    // Clear all of the items in the System tab.
    //
    g_pSystemBoardInformation->value("");
    g_pSystemFirmwareVer->value(0);
    g_pSystemHardwareVer->value(0);
    g_pSystemAssignValue->value("");

    //
    // Clear all of the status boxes at the bottom of the screen.
    //
    g_pStatusVout->value(0);
    g_pStatusVbus->value(0);
    g_pStatusCurrent->value(0);
    g_pStatusTemperature->value(0);
    g_pStatusPosition->value(0);
    g_pStatusSpeed->value(0);
    g_pStatusLimit->value(0);
}

//*****************************************************************************
//
// This function is used to update the menu status.
//
//*****************************************************************************
void
GUIMenuStatus(void)
{
    int iStatus;

    if(g_bConnected)
    {
        GUIDisconnectAndClear();
    }
    else
    {
        GUIConnect();
    }

    //
    // Re-draw the GUI widgets.
    //
    Fl::redraw();
}

//*****************************************************************************
//
// Handle drawing and filling the list of devices found.
//
//*****************************************************************************
void
GUIDropDownBoardID(void)
{
    char pcBuffer[16];
    int iIndex;

    iIndex = g_pSelectBoard->value();

    //
    // Check to see if we are connected to a board.
    //
    if(strcmp(g_pSelectBoard->text(iIndex), "--") == 0)
    {
    }
    else
    {
        //
        // Disable board status updates.
        //
        g_ulBoardStatus = 0;

        //
        // Wait for any pending board status updates.
        //
        while(g_bBoardStatusActive)
        {
            usleep(10000);
        }

        //
        // Get the current selected item.
        //
        iIndex = g_pSelectBoard->value();

        //
        // Get the ID of the newly selected item.
        //
        g_ulID = strtoul(g_pSelectBoard->text(iIndex), NULL, 10);

        //
        // Set the value of the board ID update field to the same as the
        // current selected item.
        //
        snprintf(pcBuffer, sizeof(pcBuffer), "%ld", g_ulID);
        g_pSystemAssignValue->value(pcBuffer);

        //
        // Reset the Sticky fault indicators.
        //
        GUI_DISABLE_INDICATOR(g_pStickyFaultIndicatorPowr);
        GUI_DISABLE_INDICATOR(g_pStickyFaultIndicatorCurr);
        GUI_DISABLE_INDICATOR(g_pStickyFaultIndicatorTemp);
        GUI_DISABLE_INDICATOR(g_pStickyFaultIndicatorVbus);
        GUI_DISABLE_INDICATOR(g_pStickyFaultIndicatorGate);
        GUI_DISABLE_INDICATOR(g_pStickyFaultIndicatorComm);

        //
        // Refresh the parameters in the Mode tab.
        //
        GUIControlUpdate();

        //
        // Update the control mode.
        //
        g_pSelectMode->value(g_sBoardState[g_ulID].ulControlMode);

        //
        // Clear all of the status boxes at the bottom of the screen.
        //
        g_pStatusVout->value(0);
        g_pStatusVbus->value(0);
        g_pStatusCurrent->value(0);
        g_pStatusTemperature->value(0);
        g_pStatusPosition->value(0);
        g_pStatusSpeed->value(0);

        //
        // Reset the Periodic Status tab.
        //
        g_pPeriodicSelectStatusMessage->value(0);
        g_sBoardStatus.lBoardFlags = 0;

        //
        // Reset the Periodic Status History display.
        //
        g_pExtendedStatusSelectMessage->value(0);
        g_pPeriodicMessageHistory->buffer(NULL);

        //
        // Reset Fault Counters display.
        //
        g_pStatusCurrentFaults->value(0);
        g_pStatusTemperatureFaults->value(0);
        g_pStatusVoltageFaults->value(0);
        g_pStatusGateFaults->value(0);
        g_pStatusCommFaults->value(0);

        //
        // Reset Sticky Limit displays.
        //
        g_pStatusStickyLimit->value("..");
        g_pStatusSoftStickyLimit->value("..");

        //
        // Reset CAN interface status display.
        //
        g_pStatusCanSts->value(0);
        g_pStatusCanRxErr->value(0);
        g_pStatusCanTxErr->value(0);

        //
        // Update the configuration.
        //
        GUIConfigUpdate();

        //
        // Re-draw the GUI to make sure there are no text side effects from the
        // previous mode.
        //
        Fl::redraw();

        //
        // Re-enable board status updates.
        //
        g_ulBoardStatus = 1;
    }
}

//*****************************************************************************
//
// Handle drawing the COM port drop down.
//
//*****************************************************************************
void
GUIDropDownCOMPort(void)
{
    int iIndex;

    //
    // Close the current port.
    //
    if(g_bConnected)
    {
        GUIDisconnectAndClear();
    }

    //
    // Open the new port.
    //
    GUIConnect();
}

//*****************************************************************************
//
// Handle drawing the current mode drop down.
//
//*****************************************************************************
void
GUIModeDropDownMode(void)
{
    int iIndex;

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Update window for the new mode and enable it.
    //
    switch(g_pSelectMode->value())
    {
        case LM_STATUS_CMODE_VOLT:
        {
            //
            // Send the enable command.
            //
            strcpy(g_argv[0], "volt");
            strcpy(g_argv[1], "en");
            CmdVoltage(2, g_argv);

            break;
        }

        case LM_STATUS_CMODE_VCOMP:
        {
            //
            // Send the enable command.
            //
            strcpy(g_argv[0], "vcomp");
            strcpy(g_argv[1], "en");
            CmdVComp(2, g_argv);

            break;
        }

        case LM_STATUS_CMODE_CURRENT:
        {
            //
            // Send the enable command.
            //
            strcpy(g_argv[0], "cur");
            strcpy(g_argv[1], "en");
            CmdCurrent(2, g_argv);

            break;
        }

        case LM_STATUS_CMODE_SPEED:
        {
            //
            // Send the enable command.
            //
            strcpy(g_argv[0], "speed");
            strcpy(g_argv[1], "en");
            CmdSpeed(2, g_argv);

            //
            // Set the speed reference to the encoder.
            //
            strcpy(g_argv[1], "ref");
            strcpy(g_argv[2], "0");
            CmdSpeed(3, g_argv);

            break;
        }

        case LM_STATUS_CMODE_POS:
        {
            //
            // Send the enable command.
            //
            strcpy(g_argv[0], "pos");
            strcpy(g_argv[1], "en");
            sprintf(g_argv[2], "%ld",
                    (long)(g_sBoardStatus.fPosition * 65536));
            CmdPosition(3, g_argv);

            break;
        }
    }

    //
    // Update the controls.
    //
    GUIControlUpdate();

    //
    // Re-draw the GUI to make sure there are no text side effects from the
    // previous mode.
    //
    Fl::redraw();

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the sync button.
//
//*****************************************************************************
void
GUIModeButtonSync(void)
{
    //
    // Toggle the synchronous update flag.
    //
    g_bSynchronousUpdate ^= 1;

    if(g_bSynchronousUpdate)
    {
        //
        // Change the button color to indicate that this feature is active.
        //
        g_pModeSync->color((Fl_Color)1);
        g_pModeSync->labelcolor((Fl_Color)7);
    }
    else
    {
        //
        // Change the button color back to it's normal state.
        //
        g_pModeSync->color((Fl_Color)49);
        g_pModeSync->labelcolor((Fl_Color)0);

        //
        // Send the command to do the update.
        //
        strcpy(g_argv[0], "system");
        strcpy(g_argv[1], "sync");
        sprintf(g_argv[2], "%d", 1);
        CmdSystem(3, g_argv);
    }
}

//*****************************************************************************
//
// Handle mode set changes.
//
//*****************************************************************************
void
GUIModeValueSet(int iBoxOrSlider)
{
    double dValue;
    float fIntegral;
    float fFractional;
    int iIntegral;
    int iFractional;
    int iValue;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // If the slider caused the interrupt, update the box value.
    //
    if(iBoxOrSlider)
    {
        //
        // Get the value from the slider.
        //
        dValue = g_pModeSetSlider->value();

        //
        // Update the box with the slider value.
        //
        g_pModeSetBox->value(dValue);
    }
    else
    {
        //
        // Get the value from the box.
        //
        dValue = g_pModeSetBox->value();

        //
        // Update the slider with the box value.
        //
        g_pModeSetSlider->value(dValue);
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Choose the mode.
    //
    switch(g_sBoardState[g_ulID].ulControlMode)
    {
        case LM_STATUS_CMODE_VOLT:
        {
            //
            // Change the percentage to a real voltage.
            //
            dValue = (dValue * 32767) / 100;

            //
            // Set up the variables for the "set" update.  If synchronous
            // update is active, send a group number with the new value.
            //
            if(g_bSynchronousUpdate)
            {
                sprintf(g_argv[3], "%d", 1);
            }

            strcpy(g_argv[0], "volt");
            strcpy(g_argv[1], "set");
            sprintf(g_argv[2], "%ld", (long)dValue);
            CmdVoltage(g_bSynchronousUpdate ? 4 : 3, g_argv);

            break;
        }

        case LM_STATUS_CMODE_VCOMP:
        {
            //
            // Change the voltage to the proper units.
            //
            dValue *= 256;

            //
            // Set up the variables for the "set" update.  If synchronous
            // update is active, send a group number with the new value.
            //
            if(g_bSynchronousUpdate)
            {
                sprintf(g_argv[3], "%d", 1);
            }

            strcpy(g_argv[0], "vcomp");
            strcpy(g_argv[1], "set");
            sprintf(g_argv[2], "%ld", (long)dValue);
            CmdVComp(g_bSynchronousUpdate ? 4 : 3, g_argv);

            break;
        }

        case LM_STATUS_CMODE_CURRENT:
        {
            //
            // Put the desired value into the required format.
            //
            dValue *= 256;

            //
            // Set up the variables for the "set" update.  If synchronous
            // update is active, send a group number with the new value.
            //
            if(g_bSynchronousUpdate)
            {
                sprintf(g_argv[3], "%d", 1);
            }

            strcpy(g_argv[0], "cur");
            strcpy(g_argv[1], "set");
            sprintf(g_argv[2], "%ld", ((long)dValue));
            CmdCurrent(g_bSynchronousUpdate ? 4 : 3, g_argv);

            break;
        }

        case LM_STATUS_CMODE_SPEED:
        {
            //
            // Put the desired value into the required format.
            //
            dValue *= 65536;

            //
            // Set up the variables for the "set" update.  If synchronous
            // update is active, send a group number with the new value.
            //
            if(g_bSynchronousUpdate)
            {
                sprintf(g_argv[3], "%d", 1);
            }

            strcpy(g_argv[0], "speed");
            strcpy(g_argv[1], "set");
            sprintf(g_argv[2], "%ld", ((long)dValue));
            CmdSpeed(g_bSynchronousUpdate ? 4 : 3, g_argv);

            break;
        }

        case LM_STATUS_CMODE_POS:
        {
            //
            // Put the desired value into the required format.
            //
            dValue *= 65536;

            //
            // Set up the variables for the "set" update.  If synchronous
            // update is active, send a group number with the new value.
            //
            if(g_bSynchronousUpdate)
            {
                sprintf(g_argv[3], "%d", 1);
            }

            strcpy(g_argv[0], "pos");
            strcpy(g_argv[1], "set");
            sprintf(g_argv[2], "%ld", ((long)dValue));
            CmdPosition(g_bSynchronousUpdate ? 4 : 3, g_argv);

            break;
        }
    }

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the spinner control.
//
//*****************************************************************************
void
GUIModeSpinnerRamp(void)
{
    double dValue;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Get the value from the spinner box.
    //
    dValue = g_pModeRamp->value();

    //
    // Update the voltage ramp.
    //
    strcpy(g_argv[1], "ramp");
    sprintf(g_argv[2], "%ld", ((long)dValue));
    if(g_sBoardState[g_ulID].ulControlMode == LM_STATUS_CMODE_VOLT)
    {
        strcpy(g_argv[0], "volt");
        CmdVoltage(3, g_argv);
    }
    else
    {
        strcpy(g_argv[0], "vcomp");
        CmdVComp(3, g_argv);
    }

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the spinner control.
//
//*****************************************************************************
void
GUIModeSpinnerCompRamp(void)
{
    double dValue;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Get the value from the spinner box.
    //
    dValue = g_pModeCompRamp->value();

    //
    // Update the voltage ramp.
    //
    strcpy(g_argv[0], "vcomp");
    strcpy(g_argv[1], "comp");
    sprintf(g_argv[2], "%ld", ((long)dValue));
    CmdVComp(3, g_argv);

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the speed/position reference for the gui.
//
//*****************************************************************************
void
GUIModeDropDownReference(void)
{
    //
    // Do nothing if there is another command being sent.
    //
    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Send the command.
    //
    strcpy(g_argv[1], "ref");
    sprintf(g_argv[2], "%ld", (long)g_pReference->value());
    if(g_sBoardState[g_ulID].ulControlMode == LM_STATUS_CMODE_SPEED)
    {
        strcpy(g_argv[0], "speed");
        CmdSpeed(3, g_argv);
    }
    else
    {
        strcpy(g_argv[0], "pos");
        CmdPosition(3, g_argv);
    }

    //
    // Update the position slider.
    //
    if(g_sBoardState[g_ulID].ulControlMode == LM_STATUS_CMODE_POS)
    {
        GUIUpdatePositionSlider();
    }

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the PID settings.
//
//*****************************************************************************
void
GUIModeSpinnerPID(int iChoice)
{
    double dValue;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Choose the mode.
    //
    switch(g_sBoardState[g_ulID].ulControlMode)
    {
        //
        // Does not apply to voltage mode (should never get here).
        //
        case LM_STATUS_CMODE_VOLT:
        {
            break;
        }

        case LM_STATUS_CMODE_CURRENT:
        {
            strcpy(g_argv[0], "cur");
            break;
        }
        case LM_STATUS_CMODE_SPEED:
        {
            strcpy(g_argv[0], "speed");
            break;
        }
        case LM_STATUS_CMODE_POS:
        {
            strcpy(g_argv[0], "pos");
            break;
        }
    }

    //
    // Add the P, I or D command and the new value.
    //
    if(g_sBoardState[g_ulID].ulControlMode != LM_STATUS_CMODE_VOLT)
    {
        switch(iChoice)
        {
            case 0:
            {
                //
                // Get the value from the spinner box.
                //
                dValue = g_pModeP->value();

                //
                // Add the "P" command.
                //
                strcpy(g_argv[1], "p");

                break;
            }
            case 1:
            {
                //
                // Get the value from the spinner box.
                //
                dValue = g_pModeI->value();

                //
                // Add the "I" command.
                //
                strcpy(g_argv[1], "i");

                break;
            }
            case 2:
            {
                //
                // Get the value from the spinner box.
                //
                dValue = g_pModeD->value();

                //
                // Add the "D" command.
                //
                strcpy(g_argv[1], "d");

                break;
            }
        }

        //
        // Add the new value.
        //
        sprintf(g_argv[2], "%ld", (long)(dValue * 65536));
    }

    //
    // Send the command.
    //
    switch(g_sBoardState[g_ulID].ulControlMode)
    {
        //
        // Does not apply to voltage or voltage compensation modes (should
        // never get here).
        //
        case LM_STATUS_CMODE_VOLT:
        case LM_STATUS_CMODE_VCOMP:
        {
            break;
        }

        case LM_STATUS_CMODE_CURRENT:
        {
            CmdCurrent(3, g_argv);
            break;
        }
        case LM_STATUS_CMODE_SPEED:
        {
            CmdSpeed(3, g_argv);
            break;
        }
        case LM_STATUS_CMODE_POS:
        {
            CmdPosition(3, g_argv);
            break;
        }
    }

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the number of encoder lines setting.
//
//*****************************************************************************
void
GUIConfigSpinnerEncoderLines(void)
{
    double dValue;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Get the value from the spinner box.
    //
    dValue = g_pConfigEncoderLines->value();

    //
    // Send the command.
    //
    strcpy(g_argv[0], "config");
    strcpy(g_argv[1], "lines");
    sprintf(g_argv[2], "%ld", ((long)dValue));
    CmdConfig(3, g_argv);

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the number of potentiometer turns setting.
//
//*****************************************************************************
void
GUIConfigSpinnerPOTTurns(void)
{
    double dValue;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Get the value from the spinner box.
    //
    dValue = g_pConfigPOTTurns->value();

    //
    // Send the command.
    //
    strcpy(g_argv[0], "config");
    strcpy(g_argv[1], "turns");
    sprintf(g_argv[2], "%ld", ((long)dValue));
    CmdConfig(3, g_argv);

    //
    // Update the position mode slider/value.
    //
    GUIUpdatePositionSlider();

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the Maximum voltage out setting.
//
//*****************************************************************************
void
GUIConfigSpinnerMaxVout(void)
{
    double dValue;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Get the value from the spinner box.
    //
    dValue = g_pConfigMaxVout->value();

    //
    // Send the command.
    //
    strcpy(g_argv[0], "config");
    strcpy(g_argv[1], "maxvout");
    sprintf(g_argv[2], "%ld", ((long)((dValue * 0xc00) / 100)));
    CmdConfig(3, g_argv);

    g_dMaxVout = dValue;

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the Fault time out setting.
//
//*****************************************************************************
void
GUIConfigSpinnerFaultTime(void)
{
    double dValue;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Get the value from the spinner box.
    //
    dValue = g_pConfigFaultTime->value();

    //
    // Send the command.
    //
    strcpy(g_argv[0], "config");
    strcpy(g_argv[1], "faulttime");
    sprintf(g_argv[2], "%ld", ((long)dValue));
    CmdConfig(3, g_argv);

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the braking mode setting.
//
//*****************************************************************************
void
GUIConfigRadioStopAction(int iChoice)
{
    static const char *ppucChoices[] = { "jumper", "brake", "coast" };

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Send the command.
    //
    strcpy(g_argv[0], "config");
    strcpy(g_argv[1], "brake");
    strcpy(g_argv[2], ppucChoices[iChoice]);
    CmdConfig(3, g_argv);

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the limit mode switch settings.
//
//*****************************************************************************
void
GUIConfigCheckLimitSwitches(void)
{
    double dValue;
    double dFwdLimit;
    double dRevLimit;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Get the value from the spinner box.
    //
    dValue = g_pConfigLimitSwitches->value();

    //
    // Depending on the value, enable or disable soft limit items.
    //
    if(dValue)
    {
        g_pConfigFwdLimitLt->activate();
        g_pConfigFwdLimitGt->activate();
        g_pConfigFwdLimitValue->activate();
        g_pConfigRevLimitLt->activate();
        g_pConfigRevLimitGt->activate();
        g_pConfigRevLimitValue->activate();
    }
    else
    {
        g_pConfigFwdLimitLt->deactivate();
        g_pConfigFwdLimitGt->deactivate();
        g_pConfigFwdLimitValue->deactivate();
        g_pConfigRevLimitLt->deactivate();
        g_pConfigRevLimitGt->deactivate();
        g_pConfigRevLimitValue->deactivate();
    }

    //
    // Send the command.
    //
    strcpy(g_argv[0], "config");
    strcpy(g_argv[1], "limit");
    sprintf(g_argv[2], dValue ? "on" : "off");
    CmdConfig(3, g_argv);

    //
    // Update the position slider.
    //
    GUIUpdatePositionSlider();

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the forward limit setting.
//
//*****************************************************************************
void
GUIConfigValueFwdLimit(void)
{
    double dValue;
    double dRevLimit;
    long lLtGt;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Figure out which item is selected.
    //
    if(g_pConfigFwdLimitLt->value())
    {
        lLtGt = 0;
    }
    else
    {
        lLtGt = 1;
    }

    //
    // Get the position value.
    //
    dValue = g_pConfigFwdLimitValue->value();

    //
    // Send the command.
    //
    strcpy(g_argv[0], "config");
    strcpy(g_argv[1], "fwd");
    sprintf(g_argv[2], "%ld", ((long)(dValue * 65536)));
    sprintf(g_argv[3], lLtGt ? "lt" : "gt");
    CmdConfig(4, g_argv);

    //
    // Update the position slider.
    //
    GUIUpdatePositionSlider();

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the reverse limit setting.
//
//*****************************************************************************
void
GUIConfigValueRevLimit(void)
{
    double dValue;
    double dFwdLimit;
    long lLtGt;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Figure out which item is selected.
    //
    if(g_pConfigRevLimitLt->value())
    {
        lLtGt = 0;
    }
    else
    {
        lLtGt = 1;
    }

    //
    // Get the position value.
    //
    dValue = g_pConfigRevLimitValue->value();

    //
    // Send the command.
    //
    strcpy(g_argv[0], "config");
    strcpy(g_argv[1], "rev");
    sprintf(g_argv[2], "%ld", ((long)(dValue * 65536)));
    sprintf(g_argv[3], lLtGt ? "lt" : "gt");
    CmdConfig(4, g_argv);

    //
    // Update the position slider.
    //
    GUIUpdatePositionSlider();

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the system assignment setting.
//
//*****************************************************************************
void
GUISystemAssignValue(void)
{
    unsigned long ulValue;

    if(g_ulBoardStatus == 0)
    {
        return;
    }

    //
    // Get the current value of the box.
    //
    ulValue = strtoul(g_pSystemAssignValue->value(), 0, 0);

    //
    // Make sure that the value is valid.
    //
    if(ulValue < 1)
    {
        g_pSystemAssignValue->value("1");
    }
    if(ulValue > 63)
    {
        g_pSystemAssignValue->value("63");
    }
}

//*****************************************************************************
//
// Handle the assignment button.
//
//*****************************************************************************
void
GUISystemButtonAssign(void)
{
    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Disable the previous mode.
    //
    switch(g_sBoardState[g_ulID].ulControlMode)
    {
        case LM_STATUS_CMODE_VOLT:
        {
            strcpy(g_argv[0], "volt");
            strcpy(g_argv[1], "dis");
            CmdVoltage(2, g_argv);
            break;
        }

        case LM_STATUS_CMODE_VCOMP:
        {
            strcpy(g_argv[0], "vcomp");
            strcpy(g_argv[1], "dis");
            CmdVComp(2, g_argv);
            break;
        }

        case LM_STATUS_CMODE_CURRENT:
        {
            strcpy(g_argv[0], "cur");
            strcpy(g_argv[1], "dis");
            CmdCurrent(2, g_argv);
            break;
        }

        case LM_STATUS_CMODE_SPEED:
        {
            strcpy(g_argv[0], "speed");
            strcpy(g_argv[1], "dis");
            CmdSpeed(2, g_argv);
            break;
        }

        case LM_STATUS_CMODE_POS:
        {
            strcpy(g_argv[0], "pos");
            strcpy(g_argv[1], "dis");
            CmdPosition(2, g_argv);
            break;
        }
    }

    //
    // Send the command.
    //
    strcpy(g_argv[0], "system");
    strcpy(g_argv[1], "assign");
    strcpy(g_argv[2], g_pSystemAssignValue->value());
    CmdSystem(3, g_argv);

    //
    // Re-enumerate.
    //
    GUISystemButtonEnumerate();
}

//*****************************************************************************
//
// Handle the halt button.
//
//*****************************************************************************
void
GUISystemButtonHalt(void)
{
    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Send the command.
    //
    strcpy(g_argv[0], "system");
    strcpy(g_argv[1], "halt");
    CmdSystem(2, g_argv);

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the resume button.
//
//*****************************************************************************
void
GUISystemButtonResume(void)
{
    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Send the command.
    //
    strcpy(g_argv[0], "system");
    strcpy(g_argv[1], "resume");
    CmdSystem(2, g_argv);

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;
}

//*****************************************************************************
//
// Handle the system reset button.
//
//*****************************************************************************
void
GUISystemButtonReset(void)
{
    int iIndex;

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Send the command.
    //
    strcpy(g_argv[0], "system");
    strcpy(g_argv[1], "reset");
    CmdSystem(2, g_argv);

    //
    // Clear the trust structure trusted flags.
    //
    for(iIndex = 0; iIndex < MAX_CAN_ID; iIndex++)
    {
        g_sBoardState[iIndex].ulControlMode = LM_STATUS_CMODE_VOLT;
    }

    //
    // Re-enable board status updates.
    //
    g_ulBoardStatus = 1;

    //
    // Disconnect the GUI.
    //
    GUIDisconnectAndClear();

    //
    // Re-draw the GUI widgets.
    //
    Fl::redraw();
}

//*****************************************************************************
//
// Handle the enumerate button.
//
//*****************************************************************************
void
GUISystemButtonEnumerate(void)
{
    //
    // Disconnect.
    //
    GUIDisconnectAndClear();

    //
    // Re-open the communication port, which will re-enumerate the bus.
    //
    GUIConnect();
}

//*****************************************************************************
//
// Handle the heart beat enable/disable button.
//
//*****************************************************************************
void
GUISystemCheckHeartbeat(void)
{
    g_ulHeartbeat = (g_pSystemHeartbeat->value() ? 1 : 0);
}

//*****************************************************************************
//
// Handle user's request to retrieve the Sticky Fault status
//
//*****************************************************************************
void
GUISystemButtonStickyFaultsGet(void)
{

}

//*****************************************************************************
//
// Handle the firmware update button.
//
//*****************************************************************************
void
GUIUpdateFirmware(void)
{
    //
    // Produce an error and return without updating if no firmware filename has
    // been specified.
    //
    if(strlen(g_pathname) == 0)
    {
        g_pFirmwareUpdateWindow->hide();
        delete g_pFirmwareUpdateWindow;
        g_pFirmwareUpdateWindow = 0;
        fl_alert("No firmware was specified");
        Fl::check();
        return;
    }

    //
    // Show the progress bar.
    //
    g_pUpdateProgress->show();

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Update the firmware.
    //
    strcpy(g_argv[0], "update");
    strcpy(g_argv[1], g_pathname);
    CmdUpdate(2, g_argv);

    //
    // Kill the firmware update window.
    //
    g_pFirmwareUpdateWindow->hide();
    delete g_pFirmwareUpdateWindow;
    g_pFirmwareUpdateWindow = 0;

    //
    // Disconnect from the board.
    //
    GUIDisconnectAndClear();

    //
    // Wait some time for the board to reset.
    //
    OSSleep(1);

    //
    // Reconnect.
    //
    GUIConnect();
}

//*****************************************************************************
//
// Handle the recover device button.
//
//*****************************************************************************
void
GUIRecoverDevice(void)
{
    //
    // Produce an error and return without updating if no firmware filename has
    // been specified.
    //
    if(strlen(g_pathname) == 0)
    {
        g_pRecoverDeviceWindow->hide();
        delete g_pRecoverDeviceWindow;
        g_pRecoverDeviceWindow = 0;
        fl_alert("No firmware was specified");
        Fl::check();
        return;
    }

    //
    // Show the progress bar.
    //
    g_pRecoverProgress->show();

    //
    // Disable board status updates.
    //
    g_ulBoardStatus = 0;

    //
    // Wait for any pending board status updates.
    //
    while(g_bBoardStatusActive)
    {
        usleep(10000);
    }

    //
    // Set the device ID to zero in order to perform a recovery.
    //
    g_ulID = 0;

    //
    // Update the firmware.
    //
    strcpy(g_argv[0], "update");
    strcpy(g_argv[1], g_pathname);
    CmdUpdate(2, g_argv);

    //
    // Kill the firmware update window.
    //
    g_pRecoverDeviceWindow->hide();
    delete g_pRecoverDeviceWindow;
    g_pRecoverDeviceWindow = 0;

    //
    // Disconnect from the board.
    //
    GUIDisconnectAndClear();

    //
    // Wait some time for the board to reset.
    //
    OSSleep(1);

    //
    // Reconnect.
    //
    GUIConnect();
}

//*****************************************************************************
//
// Handle the user request for selecting the main fault indicator.
//
//*****************************************************************************
void
GUIFaultIndicatorSelect(void)
{
    //
    // User selected the fault indicator. Standard behavior is to clear if
    // selected.
    //
    g_sBoardStatus.lFault = 0;
    g_pStatusFault->hide();
}

//*****************************************************************************
//
// Return the space available for requested PStatus message.
//
//*****************************************************************************
static int
PeriodicStatusSpaceAvailable(char cPStatusMsg)
{
    int iSpaceAvail, iMsgIdx;
    unsigned long ulMsgID;

    if((cPStatusMsg < 0) || (cPStatusMsg >= PSTATUS_MSGS_NUM))
    {
        //
        // Invalid message number.
        //
        return(-1);
    }

    //
    // Periodic Status messages can carry 8 bytes total.
    //
    iSpaceAvail = PSTATUS_PAYLOAD_SZ;

    //
    // Count bytes used by requested PStatus configuration.
    //
    for(iMsgIdx = 0; iMsgIdx < PSTATUS_PAYLOAD_SZ; iMsgIdx++)
    {
            ulMsgID = g_sBoardStatus.pulPStatusMsgCfgs[cPStatusMsg][iMsgIdx];

            //
            // Check for configuration end ID.
            //
            if(ulMsgID == LM_PSTAT_END)
            {
                //
                // No more messages in the configuration.
                //
                break;
            }

            //
            // Update counter.
            //
            iSpaceAvail -= 1;
    }

    return(iSpaceAvail);
}

//*****************************************************************************
//
// Check for status message ID in current PayloadList.
//
//*****************************************************************************
static int
PeriodicStatusIsMessageTypeInCurrent(unsigned long ulMsgID)
{
    int iListIdx;
    Fl_Browser *pPayloadList;

    pPayloadList = g_pPeriodicStatusPayloadList;
    for(iListIdx = 0; iListIdx <= pPayloadList->size(); iListIdx++)
    {
        if(ulMsgID == (unsigned long)pPayloadList->data(iListIdx))
        {
            //
            // Present in PayloadList.
            //
            return(1);
        }
    }

    //
    // Not present in PayloadList.
    //
    return(0);
}

//*****************************************************************************
//
// Refresh the Message List items with filtering as needed.
//
//*****************************************************************************
static void
PeriodicStatusMessageListRefresh(int iPaySpace)
{
    bool bAddToMessageList;
    int iMsgIdx;
    unsigned long ulMsgID;
    Fl_Browser *pMsgList;

    pMsgList = g_pPeriodicStatusMessagesList;
    pMsgList->clear();

    if(iPaySpace <= 0)
    {
        //
        // If there's no space, the list should be empty.
        //
        return;
    }

    for(iMsgIdx = 1; g_sPStatMsgs[iMsgIdx].pcMsgString != NULL;
        iMsgIdx++)
    {
        ulMsgID = g_sPStatMsgs[iMsgIdx].ulMsgID;

        //
        // There's at least one byte of space.
        //
        bAddToMessageList = true;

        //
        // Verify it's not in the current message payload.
        //
        if(bAddToMessageList &&
            (PeriodicStatusIsMessageTypeInCurrent(ulMsgID)))
        {
            bAddToMessageList = false;
        }

        //
        // Finally, if FilterUsed checked, verify it's not in use
        // in any other active payloads.
        //
        if(bAddToMessageList && g_pPeriodicStatusFilterUsed->value() &&
            PeriodicStatusIsMessageOn(ulMsgID))
        {
                bAddToMessageList = false;
        }

        //
        // If allowed, add it to the MessageList.
        //
        if(bAddToMessageList)
        {
            pMsgList->add(g_sPStatMsgs[iMsgIdx].pcMsgString,
                (void *)g_sPStatMsgs[iMsgIdx].ulMsgID);
        }
   }
}

//*****************************************************************************
//
// Refresh the Payload List items based on selected periodic status message.
//
//*****************************************************************************
static void
PeriodicStatusPayloadListRefresh(void)
{
    char cPMsg;
    int iCfgIdx, iMsgIdx;
    tBoardStatus *psBoard;

    //
    // Get the currently selected Periodic Message number.
    //
    cPMsg = (char)g_pPeriodicSelectStatusMessage->value();

    //
    // Clear the current Payload List
    //
    g_pPeriodicStatusPayloadList->clear();

    //
    // Loop through the active PStatus message's IDs, match them to the
    // display strings and add those to the Payload List.
    // strings to the PayloadList.
    //
    psBoard = &g_sBoardStatus;
    for(iCfgIdx = 0; iCfgIdx < PSTATUS_PAYLOAD_SZ; iCfgIdx++)
    {
        if(psBoard->pulPStatusMsgCfgs[cPMsg][iCfgIdx] != LM_PSTAT_END)
        {
            psBoard->pulPStatusMsgCfgs[cPMsg][iCfgIdx];
            for(iMsgIdx = 1; g_sPStatMsgs[iMsgIdx].pcMsgString != NULL;
                iMsgIdx++)
            {
                if(g_sPStatMsgs[iMsgIdx].ulMsgID ==
                    psBoard->pulPStatusMsgCfgs[cPMsg][iCfgIdx])
                {
                        //
                        // This is the target ID.
                        //
                        g_pPeriodicStatusPayloadList->
                            add(g_sPStatMsgs[iMsgIdx].pcMsgString,
                                (void *)g_sPStatMsgs[iMsgIdx].ulMsgID);

                        break;
                }
            }
        }
    }
}

//*****************************************************************************
//
// Add message to the target Periodic Status configuration.
// Returns payload space available on success or -1 on failure.
//
//*****************************************************************************
static int
PeriodicStatusAddConfig(unsigned long ulMsgID)
{
    char cMsgSel;
    int iCfgSlot, iPaySlot;

    //
    // Get the currently selected Periodic Message number.
    //
    cMsgSel = (char)g_pPeriodicSelectStatusMessage->value();

    //
    // Find the next open ID slot for currently selected periodic message and
    // assign the new message.
    //
    for(iCfgSlot = 0; iCfgSlot < PSTATUS_PAYLOAD_SZ; iCfgSlot++)
    {
        if(g_sBoardStatus.pulPStatusMsgCfgs[cMsgSel][iCfgSlot] ==
            LM_PSTAT_END)
        {
            g_sBoardStatus.pulPStatusMsgCfgs[cMsgSel][iCfgSlot] = ulMsgID;
            break;
        }
    }

    if(cMsgSel == g_pExtendedStatusSelectMessage->value())
    {
        GUIExtendedStatusDropDownMessageSelect();
    }

    return(PSTATUS_PAYLOAD_SZ - (iCfgSlot + 1));
}

//*****************************************************************************
//
// Remove message from the target Periodic Status configuration.
// Returns byte payload space available on success or -1 on failure.
//
//*****************************************************************************
static int
PeriodicStatusRemoveConfig(unsigned long ulMsgID)
{
    char cMsgSel;
    int iCfgIdx, iCfg2Idx, iMsgIdx, iPaySpc;
    unsigned long pulNewMsgCfg[PSTATUS_PAYLOAD_SZ];

    //
    // Init the local message config array.
    //
    memset(pulNewMsgCfg, LM_PSTAT_END, sizeof(pulNewMsgCfg));

    //
    // Get the currently selected Periodic Message number.
    //
    cMsgSel = (char)g_pPeriodicSelectStatusMessage->value();

    //
    // Copy the IDs in the currently selected Config, skipping the
    // first instance of the ID to be removed.
    //
    for(iCfg2Idx = 0, iCfgIdx = 0; iCfgIdx < PSTATUS_PAYLOAD_SZ; iCfgIdx++)
    {
        if(g_sBoardStatus.pulPStatusMsgCfgs[cMsgSel][iCfgIdx] == ulMsgID)
        {
            //
            // Found, don't copy it and replace our search pattern with
            // the END marker.
            //
            ulMsgID = LM_PSTAT_END;
        }
        else
        {
            pulNewMsgCfg[iCfg2Idx++] =
                g_sBoardStatus.pulPStatusMsgCfgs[cMsgSel][iCfgIdx];
        }
    }

    //
    // If the MsgID was not found (and it was not CFG_END), return failure.
    //
    if(ulMsgID != LM_PSTAT_END)
    {
        return(-1);
    }

    //
    // Clear the currently selected Message's config.
    //
    memset(&g_sBoardStatus.pulPStatusMsgCfgs[cMsgSel][0], LM_PSTAT_END,
        sizeof(unsigned long) * PSTATUS_PAYLOAD_SZ);

    //
    // Add all the messages from the local (revised) copy.
    //
    for(iCfgIdx = 0; iCfgIdx < PSTATUS_PAYLOAD_SZ; iCfgIdx++)
    {
        if(pulNewMsgCfg[iCfgIdx] != LM_PSTAT_END)
        {
            if((iPaySpc = PeriodicStatusAddConfig(pulNewMsgCfg[iCfgIdx])) < 0)
            {
                return(-1);
            }
        }
    }

    if(cMsgSel == g_pExtendedStatusSelectMessage->value())
    {
        GUIExtendedStatusDropDownMessageSelect();
    }

    //
    // Return the last payload space indicator.
    //
    return(iPaySpc);
}

//*****************************************************************************
//
// Clear and setup Periodic Status History display legend.
//
//*****************************************************************************
static void
GUIPeriodicStatusHistoryLegendSetup(char cMsgSel)
{
    int iIdx;
    unsigned long ulMsgID;
    Fl_Text_Buffer *pBuffer;
    unsigned long ulLegendFlags;
    char pcTempString[255];

    //
    // Range check the index.
    //
    if((cMsgSel > 3) || (cMsgSel < 0))
    {
        return;
    }

    //
    // If necessary, allocate Legend buffer.
    //
    if(pvPeriodicStatusLegendBuffer == NULL)
    {
        pBuffer = new Fl_Text_Buffer();
        pvPeriodicStatusLegendBuffer = pBuffer;
    }

    //
    // Assign the Legend buffer to a local variable and add it to the
    // widget.
    //
    pBuffer = (Fl_Text_Buffer *)pvPeriodicStatusLegendBuffer;
    g_pPeriodicMessageHistoryLegend->buffer(pBuffer);

    //
    // Clear the Legend if not empty.
    //
    if(pBuffer->length() > 0)
    {
        pBuffer->remove(0, pBuffer->length());
    }

    //
    // Add in the proper legend. First is always the Timestamp.
    //
    pBuffer->text("TimeStamp ");

    //
    // Parse the config and flag strings to be added.
    //
    for(iIdx = 0, ulLegendFlags = 0; iIdx < PSTATUS_PAYLOAD_SZ; iIdx++)
    {
        ulMsgID = g_sBoardStatus.pulPStatusMsgCfgs[cMsgSel][iIdx];
        switch(ulMsgID)
        {
            case LM_PSTAT_END:
            case LM_PSTAT_VOLTOUT_B0:
            case LM_PSTAT_VOLTOUT_B1:
            {
                //
                // Ignore.
                //
                break;
            }

            case LM_PSTAT_VOUT_B0:
            case LM_PSTAT_VOUT_B1:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_VOUT;
                break;
            }

            case LM_PSTAT_VOLTBUS_B0:
            case LM_PSTAT_VOLTBUS_B1:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_VBUS;
                break;
            }

            case LM_PSTAT_CURRENT_B0:
            case LM_PSTAT_CURRENT_B1:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_CURR;
                break;
            }

            case LM_PSTAT_TEMP_B0:
            case LM_PSTAT_TEMP_B1:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_TEMP;
                break;
            }

            case LM_PSTAT_POS_B0:
            case LM_PSTAT_POS_B1:
            case LM_PSTAT_POS_B2:
            case LM_PSTAT_POS_B3:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_POS;
                break;
            }

            case LM_PSTAT_SPD_B0:
            case LM_PSTAT_SPD_B1:
            case LM_PSTAT_SPD_B2:
            case LM_PSTAT_SPD_B3:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_SPD;
                break;
            }

            case LM_PSTAT_LIMIT_NCLR:
            case LM_PSTAT_LIMIT_CLR:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_LIMIT;
                break;
            }

            case LM_PSTAT_FAULT:
            case LM_PSTAT_STKY_FLT_NCLR:
            case LM_PSTAT_STKY_FLT_CLR:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_FAULTS;
                break;
            }

            case LM_PSTAT_FLT_COUNT_CURRENT:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_CURR_FLT;
                break;
            }

            case LM_PSTAT_FLT_COUNT_TEMP:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_TEMP_FLT;
                break;
            }

            case LM_PSTAT_FLT_COUNT_VOLTBUS:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_VBUS_FLT;
                break;
            }

            case LM_PSTAT_FLT_COUNT_GATE:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_GATE_FLT;
                break;
            }

            case LM_PSTAT_FLT_COUNT_COMM:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_COMM_FLT;
                break;
            }

            case LM_PSTAT_CANSTS:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_CAN_STS;
                break;
            }

            case LM_PSTAT_CANERR_B0:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_CAN_RX_ERR;
                break;
            }

            case LM_PSTAT_CANERR_B1:
            {
                ulLegendFlags |= PSTAT_LEGEND_F_CAN_TX_ERR;
                break;
            }
        }
    }

    if(ulLegendFlags > 0)
    {
        pBuffer->append("|");
        for(iIdx = 0; ulLegendFlags > 0; iIdx++, ulLegendFlags >>= 1)
        {
            if(ulLegendFlags & 0x01)
            {
                pBuffer->append(g_ppcLegentTitles[iIdx]);

                //
                // If more flags are set, add the spacer.
                //
                if((ulLegendFlags >> 1) > 0)
                {
                    pBuffer->append("|");
                }
            }
        }
    }
}

//*****************************************************************************
//
// Setup Periodic Status History buffer for display.
//
//*****************************************************************************
void
GUIPeriodicStatusHistorySetup(char cMsgSel)
{
    int iIdx;
    Fl_Text_Buffer *pBuffer;

    //
    // Range check the index.
    //
    if((cMsgSel > 3) || (cMsgSel < 0))
    {
        return;
    }

    //
    // If buffer not allocated, do so now.
    //
    if(g_sBoardStatus.ppvPStatusMsgHistory[cMsgSel] == NULL)
    {
        pBuffer = new Fl_Text_Buffer();
        g_sBoardStatus.ppvPStatusMsgHistory[cMsgSel] =  pBuffer;
    }

    pBuffer = (Fl_Text_Buffer *)g_sBoardStatus.ppvPStatusMsgHistory[cMsgSel];
    g_pPeriodicMessageHistory->buffer(pBuffer);

    //
    // Force a drawing update.
    //
    Fl::redraw();
}

//*****************************************************************************
//
// Clear and setup PStat history buffer for provided configuration.
//
//*****************************************************************************
void
GUIPeriodicStatusHistorySetupAndClear(char cMsgSel)
{
    Fl_Text_Buffer *pBuffer;

    //
    // Range check the index.
    //
    if((cMsgSel > 3) || (cMsgSel < 0))
    {
        return;
    }

    //
    // Setup the history buffer.
    //
    GUIPeriodicStatusHistorySetup(cMsgSel);

    //
    // Get the buffer pointer from the global structure.
    //
    pBuffer = (Fl_Text_Buffer *)g_sBoardStatus.ppvPStatusMsgHistory[cMsgSel];

    if(pBuffer != NULL)
    {
        //
        // Clear the buffer.
        //
        if(pBuffer->length() > 0)
        {
            pBuffer->remove(0, pBuffer->length());
        }
    }

    //
    // Force a drawing update.
    //
    Fl::redraw();
}

//*****************************************************************************
//
// Copy the PStat history buffer for provided configuration to the clipboard.
//
//*****************************************************************************
void
GUIPeriodicStatusHistoryCopy(char cMsgSel)
{
    int iIdx;
    unsigned long ulMsgID;
    Fl_Text_Buffer *pBuffer;
    char *pcTempString;

    //
    // Range check the index.
    //
    if((cMsgSel > 3) || (cMsgSel < 0))
    {
        return;
    }

    //
    // Get the buffer pointer from the global structure.
    //
    pBuffer = (Fl_Text_Buffer *)g_sBoardStatus.ppvPStatusMsgHistory[cMsgSel];

    //
    // Make certain these are setup before manipulating.
    //
    if(pBuffer == NULL)
    {
        return;
    }

    //
    // Select and copy the buffer contents to the OS clipboard via FLTK.
    //
    if(pBuffer->length() > 0)
    {
        pBuffer->select(0, pBuffer->length());
        pcTempString = pBuffer->selection_text();
        Fl::copy(pcTempString, strlen(pcTempString), 1);
        free(pcTempString);
        pBuffer->unselect();
    }
    Fl::redraw();
}


//*****************************************************************************
//
// Check for status message ID in active periodic status messages.
//
//*****************************************************************************
int
PeriodicStatusIsMessageOn(unsigned long ulMsgID)
{
    int iCfgIdx, iMsgIdx;

    for(iMsgIdx = 0; iMsgIdx < PSTATUS_MSGS_NUM; iMsgIdx++)
    {
        //
        // If interval not set, message is not active. Skip to the next
        // message.
        //
        if(!g_sBoardStatus.fPStatusMsgIntervals[iMsgIdx])
        {
            continue;
        }

        //
        // Parse config to see if message ID is present.
        //
        for(iCfgIdx = 0; iCfgIdx < PSTATUS_PAYLOAD_SZ; iCfgIdx++)
        {

            if(g_sBoardStatus.pulPStatusMsgCfgs[iMsgIdx][iCfgIdx] == ulMsgID)
            {
                //
                // Message ID found.
                //
                return(1);
            }
        }
    }

    //
    // Not present in any of the active PStatus configurations.
    //
    return(0);
}

//*****************************************************************************
//
// Handle updating widgets on a PStatus Message drop down selection.
//
//*****************************************************************************
void
GUIPeriodicStatusDropDownStatusMessage(void)
{
    char cMsgSel;
    int iPaySpace, iPayIdx;
    double dValue;

    //
    // Get the currently selected Periodic Message number.
    //
    cMsgSel = (char)g_pPeriodicSelectStatusMessage->value();

    //
    // Add any configured messages to the Payload List.
    //
    PeriodicStatusPayloadListRefresh();
    if(g_pPeriodicStatusPayloadList->size() == 0)
    {
        g_pPeriodicStatusInterval->deactivate();
        g_pPeriodicStatusPayloadList->deactivate();
    }
    else
    {
        //
        // If there's a payload, interval can be set/updated
        //
        g_pPeriodicStatusInterval->activate();

        //
        // Payload List should be active.
        //
        g_pPeriodicStatusPayloadList->activate();
    }

    dValue = g_sBoardStatus.fPStatusMsgIntervals[cMsgSel];
    g_pPeriodicStatusInterval->value(dValue);

    //
    // Determine how much payload space is available.
    //
    iPaySpace = PeriodicStatusSpaceAvailable(cMsgSel);

    //
    // Update the Messages List next.
    //
    PeriodicStatusMessageListRefresh(iPaySpace);

    //
    // Activate Periodic Status Message list if it has elements.
    //
    if(g_pPeriodicStatusMessagesList->size() > 0)
    {
        g_pPeriodicStatusMessagesList->activate();
    }

    //
    // Flag the Enable checkbox if the state flag shows this
    // periodic status message is enabled.
    //
    if(g_sBoardStatus.lBoardFlags & (PSTAT_STATEF_EN << cMsgSel))
    {
        g_pPeriodicStatusEnablePSMsg->value(1);

        //
        // Don't allow modifying an active periodic status
        // message.
        //
        g_pPeriodicStatusInterval->deactivate();
        g_pPeriodicStatusPayloadList->deactivate();
        g_pPeriodicStatusMessagesList->deactivate();
    }
    else
    {
        //
        // State flag shows this periodic status message is disabled.
        //
         g_pPeriodicStatusEnablePSMsg->value(0);
    }

    //
    // Deactivate the Add/Remove buttons as nothing is selected
    // now.
    //
    g_pPeriodicStatusRemoveStatus->deactivate();
    Fl::redraw();
}

//*****************************************************************************
//
// Handle interval set changes.
//
//*****************************************************************************
void
GUIPeriodicStatusIntervalSet(void)
{
    char cMsgSel;

    //
    // Get the currently selected Periodic Message number.
    //
    cMsgSel = (char)g_pPeriodicSelectStatusMessage->value();

    //
    // Save the value to board status.
    //
    g_sBoardStatus.fPStatusMsgIntervals[cMsgSel] =
        g_pPeriodicStatusInterval->value();
}

//*****************************************************************************
//
// Handle user request to toggle filter Include list for only unused messages.
//
//*****************************************************************************
void
GUIPeriodicStatusCheckFilterUsed(void)
{
    char cMsgSel;
    int iPaySpace, iPayIdx;

    //
    // Get the currently selected Periodic Message number.
    //
    cMsgSel = (char)g_pPeriodicSelectStatusMessage->value();

    //
    // Determine how much payload space is available.
    //
    iPaySpace = PeriodicStatusSpaceAvailable(cMsgSel);

    //
    // Refresh the Message List as this filter state changed.
    //
    PeriodicStatusMessageListRefresh(iPaySpace);
}

//*****************************************************************************
//
// Handle user request to toggle Periodic Status Enable checkbox.
//
//*****************************************************************************
void
GUIPeriodicStatusCheckEnableSelect(int iCheckBoxState)
{
    int iByteIdx;
    char cMsgSel;

    //
    // Get the currently selected Periodic Message number.
    //
    cMsgSel = (char)g_pPeriodicSelectStatusMessage->value();

    if(g_pPeriodicStatusEnablePSMsg->value() > 0)
    {
        //
        // User requested the periodic message be enabled.
        // Check the widgets to see if interval has been set and
        // Payload is configured.
        //
        if((g_sBoardStatus.fPStatusMsgIntervals[cMsgSel] == 0) ||
            (g_sBoardStatus.pulPStatusMsgCfgs[cMsgSel][0] == 0))
        {
            //
            // Interval or Payload not set. Flip the checkbox to unchecked
            // and signal the user that interval and Payload
            // must be setup.
            //
            g_pPeriodicStatusEnablePSMsg->value(0);
            fl_alert("Payload and Interval must be configured to activate"
                " a Periodic Status message.");
            return;
        }

        //
        // Deactivate the Interval, Messages, Add, Remove and Payload widgets.
        // The user needs to disable the PStatus message to change these.
        //
        g_pPeriodicStatusInterval->deactivate();
        g_pPeriodicStatusPayloadList->deactivate();
        g_pPeriodicStatusMessagesList->deactivate();
        g_pPeriodicStatusAddStatus->deactivate();
        g_pPeriodicStatusRemoveStatus->deactivate();


        //
        // All the data we need has been provided.
        // Send the config down first, then the interval.
        //
        strcpy(g_argv[0], "pstat");
        strcpy(g_argv[1], "cfg");
        sprintf(g_argv[2], "%d", cMsgSel);
        for(iByteIdx = 0; iByteIdx < PSTATUS_PAYLOAD_SZ; iByteIdx++)
        {
            sprintf(g_argv[3 + iByteIdx], "%ld",
                g_sBoardStatus.pulPStatusMsgCfgs[cMsgSel][iByteIdx]);
        }
        CmdPStatus(11, g_argv);
        usleep(1000);
        strcpy(g_argv[0], "pstat");
        strcpy(g_argv[1], "int");
        sprintf(g_argv[2], "%d", cMsgSel);
        sprintf(g_argv[3], "%ld", (long)g_pPeriodicStatusInterval->value());
        CmdPStatus(4, g_argv);

        //
        // Set the StateFlag to track this periodic status message is
        // enabled.
        //
        g_sBoardStatus.lBoardFlags |= (PSTAT_STATEF_EN << cMsgSel);
    }
    else
    {
        //
        // User requested the periodic message be disabled.
        //
        strcpy(g_argv[0], "pstat");
        strcpy(g_argv[1], "int");
        sprintf(g_argv[2], "%d", cMsgSel);
        strcpy(g_argv[3], "0");
        CmdPStatus(4, g_argv);

        //
        // Activate the Interval, Messages, and Payload widgets.
        //
        g_pPeriodicStatusInterval->activate();
        g_pPeriodicStatusPayloadList->activate();
        g_pPeriodicStatusMessagesList->activate();

        //
        // Unset the StateFlag to track this periodic status message is
        // disabled.
        //
        g_sBoardStatus.lBoardFlags &= ~(PSTAT_STATEF_EN << cMsgSel);
    }
}

//*****************************************************************************
//
// Message List item selected, activate the Add button.
//
//*****************************************************************************
void
GUIPeriodicStatusMessagesListSelect(void)
{
    static int iLastSelected = 0;
    static time_t tClickTime = 0;
    int iListIdx;
    Fl_Browser *pList;
    time_t tCurrentTime;

    //
    // Local pointer assignment.
    //
    pList = g_pPeriodicStatusMessagesList;

    //
    // Loop through the list to find selected.
    //
    for(iListIdx = 1; iListIdx <= pList->size(); iListIdx++)
    {
        if(pList->selected(iListIdx))
        {
            //
            // Detect a double-click event.
            //
            time(&tCurrentTime);
            if((iLastSelected == iListIdx) &&
                (difftime(tCurrentTime, tClickTime) <= 0.5f))
            {
                GUIPeriodicStatusButtonAddRemoveSelect(1);
                iLastSelected = 0;
            }
            else
            {
                //
                // Update the click-tracking variable.
                //
                iLastSelected = iListIdx;

                //
                // Activate the AddStatus button.
                //
                g_pPeriodicStatusAddStatus->activate();
            }

            //
            // Save the updated click time.
            //
            tClickTime = tCurrentTime;
        }
    }
}

//*****************************************************************************
//
// Payload List item selected, activate the Remove button.
//
//*****************************************************************************
void
GUIPeriodicStatusPayloadListSelect(void)
{
    static int iLastSelected = 0;
    static time_t tClickTime = 0;
    int iListIdx;
    Fl_Browser *pList;
    time_t tCurrentTime;

    //
    // Local pointer assignment.
    //
    pList = g_pPeriodicStatusPayloadList;

    //
    // Loop through the list to find selected.
    //
    for(iListIdx = 1; iListIdx <= pList->size(); iListIdx++)
    {
        if(pList->selected(iListIdx))
        {
            //
            // Detect a double-click event.
            //
            time(&tCurrentTime);
            if((iLastSelected == iListIdx) &&
                (difftime(tCurrentTime, tClickTime) <= 0.5f))
            {
                GUIPeriodicStatusButtonAddRemoveSelect(0);
                iLastSelected = 0;
            }
            else
            {
                //
                // Update the click-tracking variable.
                //
                iLastSelected = iListIdx;

                //
                // Activate the RemoveStatus button.
                //
                g_pPeriodicStatusRemoveStatus->activate();
            }
            //
            // Save the updated click time.
            //
            tClickTime = tCurrentTime;
        }
    }
}

//*****************************************************************************
//
// Handle user request to add or remove a message from one of the lists.
//
//*****************************************************************************
void
GUIPeriodicStatusButtonAddRemoveSelect(int iDirection)
{
    char cMsgSel;
    int iListIdx, iMsgIdx, iPaySpace, iTemp;
    unsigned long ulSelectedMsgID;
    Fl_Browser *pToList, *pFromList;

    //
    // GUI callback setup determines direction.
    //
    if(iDirection)
    {
        pToList = g_pPeriodicStatusPayloadList;
        pFromList = g_pPeriodicStatusMessagesList;
    }
    else
    {
        pToList = g_pPeriodicStatusMessagesList;
        pFromList = g_pPeriodicStatusPayloadList;
    }

    //
    // Loop through the FromList to find selected.
    //
    for(iListIdx = 1; iListIdx <= pFromList->size(); iListIdx++)
    {
        if(pFromList->selected(iListIdx))
        {
            //
            // Item is selected. Get the ID, match it and move this message to
            // the ToList.
            //
            ulSelectedMsgID = (unsigned long)pFromList->data(iListIdx);
            for(iMsgIdx = 1; g_sPStatMsgs[iMsgIdx].pcMsgString != NULL;
                iMsgIdx++)
            {
                if(g_sPStatMsgs[iMsgIdx].ulMsgID != ulSelectedMsgID)
                {
                        //
                        // Skip to the next index.
                        //
                        continue;
                }

                //
                // Update the periodic message config based on direction.
                //
                if(iDirection)
                {
                    iPaySpace = PeriodicStatusAddConfig(ulSelectedMsgID);

                    //
                    // If a message was added, interval can be set.
                    //
                    g_pPeriodicStatusInterval->activate();
                }
                else
                {
                    iPaySpace = PeriodicStatusRemoveConfig(ulSelectedMsgID);
                }

                //
                // If there was no error with the add/remove, move the
                // string from one list to the other.
                //
                if(iPaySpace >= 0)
                {
                    pToList->activate();
                    pToList->add(g_sPStatMsgs[iMsgIdx].pcMsgString,
                        (void *)g_sPStatMsgs[iMsgIdx].ulMsgID);
                    pFromList->remove(iListIdx);
                }
            }

            //
            // Only allowing single item selection to give
            // user immediate feedback on payload space via
            // message list filtering.
            //
            break;
        }
    }

    //
    // Re-filter the current MessageList if a valid selection/move
    // was made and the add/remove did not return an error as payload
    // space will have changed.
    //
    if(ulSelectedMsgID && (iPaySpace >= 0))
    {
        //
        // Save the vertical scroll position to allow a reposition after
        // refresh.
        //
        iTemp = g_pPeriodicStatusMessagesList->position();
        PeriodicStatusMessageListRefresh(iPaySpace);
        g_pPeriodicStatusMessagesList->position(iTemp);
    }

    //
    // If the list is now empty, disable it.
    //
    if(pFromList->size() == 0)
    {
        pFromList->deactivate();

        if(iDirection == 0)
        {
            //
            // If no messages in payload, Interval needs to be inactive.
            //
            g_pPeriodicStatusInterval->deactivate();
        }
    }

    //
    // Clear and add the updated legend to the PStat history display.
    //
    cMsgSel = (char)g_pPeriodicSelectStatusMessage->value();
    GUIPeriodicStatusHistorySetupAndClear(cMsgSel);
    GUIExtendedStatusDropDownMessageSelect();

    //
    // Disable the Add/Remove buttons as nothing is selected now.
    //
    g_pPeriodicStatusAddStatus->deactivate();
    g_pPeriodicStatusRemoveStatus->deactivate();
}

//*****************************************************************************
//
// Animate Main Window resize via FL's timer callbacks.
//
//*****************************************************************************
void
GUIExtendedStatusAnimatedResize(void *pDirection)
{
    bool bExpanding;

    //
    // Get provided direction of the resize.
    //
    bExpanding = (bool)pDirection;

    if(bExpanding)
    {
        //
        // Main Window is expanding.
        //
        if((g_pMainWindow->w() + MAIN_WIN_SIZE_STEP) > MAIN_WIN_EXPANDED_WIDTH)
        {
           	g_pMainWindow->size(MAIN_WIN_EXPANDED_WIDTH, g_pMainWindow->h());
            g_pMainMenuBar->size(MAIN_WIN_EXPANDED_WIDTH, g_pMainMenuBar->h());
        }
        else
        {
           	g_pMainWindow->size(g_pMainWindow->w() + MAIN_WIN_SIZE_STEP,
                g_pMainWindow->h());
            //
            // The menu is expanded beyond the window during the animation to
            // avoid a visual artifact.
            //
            g_pMainMenuBar->size(g_pMainMenuBar->w() + (MAIN_WIN_SIZE_STEP * 2),
                g_pMainMenuBar->h());
        }

        if(g_pMainWindow->w() != MAIN_WIN_EXPANDED_WIDTH)
        {
            //
            // Not done yet. Add another timeout callback.
            //
            Fl::add_timeout((MAIN_WIN_RESIZE_SPD_MS / 1000),
                (Fl_Timeout_Handler)GUIExtendedStatusAnimatedResize,
                (void *)true);
        }
        else
        {
            g_pSystemButtonExtendedStatus->label(EXT_STAT_LEFT_ARROW);
        }
    }
    else
    {
        //
        // Main Window is shrinking.
        //
        if((g_pMainWindow->w() - MAIN_WIN_SIZE_STEP) < MAIN_WIN_NORMAL_WIDTH)
        {
            g_pMainWindow->size(MAIN_WIN_NORMAL_WIDTH, g_pMainWindow->h());
            g_pMainMenuBar->size(MAIN_WIN_NORMAL_WIDTH, g_pMainMenuBar->h());
        }
        else
        {
           	g_pMainWindow->size(g_pMainWindow->w() - MAIN_WIN_SIZE_STEP,
                g_pMainWindow->h());
            g_pMainMenuBar->size(g_pMainMenuBar->w() - MAIN_WIN_SIZE_STEP,
                g_pMainMenuBar->h());
        }

        if(g_pMainWindow->w() != MAIN_WIN_NORMAL_WIDTH)
        {
            //
            // Not done yet. Add another timeout callback.
            //
            Fl::add_timeout((MAIN_WIN_RESIZE_SPD_MS / 1000),
                (Fl_Timeout_Handler)GUIExtendedStatusAnimatedResize,
                (void *)false);
        }
        else
        {
            g_pSystemButtonExtendedStatus->label(EXT_STAT_RIGHT_ARROW);
        }
    }

    //
    // Force a redraw of the GUI elements.
    //
    Fl::redraw();
}

//*****************************************************************************
//
// Handle user selection of Extended Status button.
//
//*****************************************************************************
void
GUIExtendedStatusButtonToggle(void)
{
    if(g_pMainWindow->w() > MAIN_WIN_NORMAL_WIDTH)
    {
        Fl::add_timeout((MAIN_WIN_RESIZE_SPD_MS / 1000),
            (Fl_Timeout_Handler)GUIExtendedStatusAnimatedResize,
            (void *)false);

    }
    else
    {
        Fl::add_timeout((MAIN_WIN_RESIZE_SPD_MS / 1000),
            (Fl_Timeout_Handler)GUIExtendedStatusAnimatedResize,
            (void *)true);
    }

    //
    // Force an update to the PeriodicStatus History display.
    //
    GUIExtendedStatusDropDownMessageSelect();
}

//*****************************************************************************
//
// Handle user request to clear a Fault Counter.
//
//*****************************************************************************
void
GUIExtendedStatusFaultCountSelect(int iFault)
{
    if(iFault > 0)
    {
        //
        // Request a clear for this fault flag.
        //
        strcpy(g_argv[0], "pstat");
        strcpy(g_argv[1], "faultcnts");
        sprintf(g_argv[2], "%d", iFault);
        CmdStatus(3, g_argv);
    }
}

//*****************************************************************************
//
// Handle user request to clear the Sticky Limit indicator.
//
//*****************************************************************************
void
GUIExtendedStatusStkyLimitSelect(void)
{
    //
    // Reverse logic, set to clear.
    //
    g_sBoardState[g_ulID].ucLimits |= (LM_STATUS_LIMIT_STKY_FWD |
        LM_STATUS_LIMIT_STKY_REV);
}

//*****************************************************************************
//
// Handle user request to clear the Sticky Soft Limit indicator.
//
//*****************************************************************************
void
GUIExtendedStatusStkySoftLimitSelect(void)
{
    //
    // Reverse logic, set to clear.
    //
    g_sBoardState[g_ulID].ucLimits |= (LM_STATUS_LIMIT_STKY_SFWD |
        LM_STATUS_LIMIT_STKY_SREV);
}

//*****************************************************************************
//
// Handle user request to clear the Power Reset flag indicator.
//
//*****************************************************************************
void
GUIExtendedStatusPowerSelect(void)
{
    //
    // Send the clear request.
    //
    strcpy(g_argv[0], "stat");
    strcpy(g_argv[1], "power");
    sprintf(g_argv[2], "%d", 1);
    CmdStatus(3, g_argv);


    //
    // Clear the indicator.
    //
    g_sBoardStatus.lPower = 0;
    GUI_DISABLE_INDICATOR(g_pStickyFaultIndicatorPowr);
    g_pMainWindow->redraw();
}


//*****************************************************************************
//
// Handle user request to clear a Sticky Fault message.
//
//*****************************************************************************
void
GUIExtendedStatusStickyFaultSelect(int iFault)
{
    Fl_Output *pIndicator;

    //
    // Use the provided bit flag to identify which indicator to modify.
    //
    switch(iFault)
    {
        case LM_FAULT_CURRENT:
        {
            pIndicator = g_pStickyFaultIndicatorCurr;
            break;
        }

        case LM_FAULT_TEMP:
        {
            pIndicator = g_pStickyFaultIndicatorTemp;
            break;
        }

        case LM_FAULT_VBUS:
        {
            pIndicator = g_pStickyFaultIndicatorVbus;
            break;
        }

        case LM_FAULT_GATE_DRIVE:
        {
            pIndicator = g_pStickyFaultIndicatorGate;
            break;
        }

        case LM_FAULT_COMM:
        {
            pIndicator = g_pStickyFaultIndicatorComm;
            break;
        }

        default:
        {
            pIndicator = NULL;
            break;
        }
    }

    if(iFault > 0)
    {
        g_sBoardState[g_ulID].ulStkyFault &= ~(iFault);
    }

    if(pIndicator != NULL)
    {
        GUI_DISABLE_INDICATOR(pIndicator);
    }
}

//*****************************************************************************
//
// Handle user selection of Status Message number.
//
//*****************************************************************************
void
GUIExtendedStatusDropDownMessageSelect(void)
{
    char cMsgSel;
    Fl_Text_Buffer *pBuffer;

    //
    // Get the currently selected Periodic Message number.
    //
    cMsgSel = (char)g_pExtendedStatusSelectMessage->value();

    //
    // Setup the legend.
    //
    GUIPeriodicStatusHistoryLegendSetup(cMsgSel);

    //
    // Setup the history display.
    //
    GUIPeriodicStatusHistorySetup(cMsgSel);

    //
    //  Make certain these widgets get redrawn.
    //
    g_pPeriodicMessageHistoryLegend->redraw();
    g_pPeriodicMessageHistory->redraw();
    g_pPeriodicMessageHistoryBar->redraw();
}

