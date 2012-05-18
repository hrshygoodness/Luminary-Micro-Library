//*****************************************************************************
//
// gui_handlers.h - Prototypes for the GUI handler functions.
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

#ifndef __GUI_HANDLERS_H__
#define __GUI_HANDLERS_H__


//*****************************************************************************
//
// GUI manipulation defines.
//
//*****************************************************************************
#define MAIN_WIN_NORMAL_WIDTH   430
#define MAIN_WIN_EXPANDED_WIDTH 860
#define MAIN_WIN_RESIZE_SPD_MS  150
#define MAIN_WIN_SIZE_STEP      50
#define GUI_DISABLED_TEXT       FL_BACKGROUND_COLOR
#define GUI_WHITE_TEXT          1
#define GUI_RED_BACKGROUND      FL_BACKGROUND2_COLOR
#define GUI_DEFAULT_BACKGROUND  16

//*****************************************************************************
//
// GUI manipulation macros.
//
//*****************************************************************************
#define GUI_ENABLE_INDICATOR(x)                         \
            x->color((Fl_Color)GUI_WHITE_TEXT);         \
            x->textcolor((Fl_Color)GUI_RED_BACKGROUND); \
            x->redraw();

#define GUI_DISABLE_INDICATOR(x)                            \
            x->color((Fl_Color)GUI_DISABLED_TEXT);          \
            x->textcolor((Fl_Color)GUI_DEFAULT_BACKGROUND); \
            x->redraw();

//*****************************************************************************
//
// GUI strings.
//
//*****************************************************************************
#define EXT_STAT_RIGHT_ARROW    "Extended Status @-4->"
#define EXT_STAT_LEFT_ARROW     "Extended Status @-4<-"

//*****************************************************************************
//
// User-friendly defines for array sizes.
//
//*****************************************************************************
#define PSTATUS_MSGS_NUM    4
#define PSTATUS_PAYLOAD_SZ  8

//*****************************************************************************
//
// BoardStatus : PStat state flag defines.
//
//*****************************************************************************
#define PSTAT_STATEF_UPD   0x01
#define PSTAT_STATEF_EN    0x10

//*****************************************************************************
//
// PStatus Legend/History helper bit flag defines.
//
//*****************************************************************************
#define PSTAT_LEGEND_F_VOUT         0x0001
#define PSTAT_LEGEND_F_VBUS         0x0002
#define PSTAT_LEGEND_F_CURR         0x0004
#define PSTAT_LEGEND_F_TEMP         0x0008
#define PSTAT_LEGEND_F_POS          0x0010
#define PSTAT_LEGEND_F_SPD          0x0020
#define PSTAT_LEGEND_F_CURR_FLT     0x0040
#define PSTAT_LEGEND_F_TEMP_FLT     0x0080
#define PSTAT_LEGEND_F_VBUS_FLT     0x0100
#define PSTAT_LEGEND_F_GATE_FLT     0x0200
#define PSTAT_LEGEND_F_COMM_FLT     0x0400
#define PSTAT_LEGEND_F_CAN_STS      0x0800
#define PSTAT_LEGEND_F_CAN_RX_ERR   0x1000
#define PSTAT_LEGEND_F_CAN_TX_ERR   0x2000
#define PSTAT_LEGEND_F_LIMIT        0x4000
#define PSTAT_LEGEND_F_FAULTS       0x8000


//*****************************************************************************
//
// This structure holds the current status for a device.
//
//*****************************************************************************
typedef struct
{
    float   fVout;
    float   fVbus;
    long    lFault;
    float   fCurrent;
    float   fTemperature;
    float   fPosition;
    float   fSpeed;
    long    lPower;
    long    lBoardFlags;
    float   fPStatusMsgIntervals[PSTATUS_MSGS_NUM];
    unsigned long   pulPStatusMsgCfgs[PSTATUS_MSGS_NUM][PSTATUS_PAYLOAD_SZ];
    unsigned char   pucPStatusMsgPayload[PSTATUS_MSGS_NUM][PSTATUS_PAYLOAD_SZ];
    void *          ppvPStatusMsgHistory[PSTATUS_MSGS_NUM];
    unsigned char   ucCurrentFaults;
    unsigned char   ucTemperatureFaults;
    unsigned char   ucVoltageFaults;
    unsigned char   ucGateFaults;
    unsigned char   ucCommFaults;
    unsigned char   ucCANStatus;
    unsigned char   ucCANRxErrors;
    unsigned char   ucCANTxErrors;
}
tBoardStatus;

//*****************************************************************************
//
// This structure type is used to hold state information about a board.
//
//*****************************************************************************
typedef struct
{
    unsigned long ulControlMode;
    unsigned long ulStkyFault;
    unsigned char ucLimits;
}
tBoardState;

//*****************************************************************************
//
// Simple struct to hold Fl_Browser data.
//
//*****************************************************************************
typedef struct
{
    const char *pcMsgString;
    const char *pcMsgMnemonic;
    unsigned long ulMsgID;
}
tPStatMsgEncodes;

extern int GUIFillCOMPortDropDown(void);
extern void GUIConnect(void);
extern void GUIDisconnectAndClear(void);
extern void GUIRefreshControlParameters(void);
extern void GUIMenuUpdateFirmware(void);
extern void GUIMenuStatus(void);
extern void GUIDropDownBoardID(void);
extern void GUIDropDownCOMPort(void);
extern void GUIButtonRun(void);
extern void GUIButtonStop(void);
extern void GUIModeDropDownMode(void);
extern void GUIModeButtonSync(void);
extern void GUIModeValueSet(int iBoxOrSlider);
extern void GUIModeSpinnerRamp(void);
extern void GUIModeSpinnerCompRamp(void);
extern void GUIModeDropDownReference(void);
extern void GUIModeSpinnerPID(int iChoice);
extern void GUIConfigSpinnerEncoderLines(void);
extern void GUIConfigSpinnerPOTTurns(void);
extern void GUIConfigSpinnerMaxVout(void);
extern void GUIConfigSpinnerFaultTime(void);
extern void GUIConfigRadioStopAction(int iChoice);
extern void GUIConfigCheckLimitSwitches(void);
extern void GUIConfigValueFwdLimit(void);
extern void GUIConfigValueRevLimit(void);
extern void GUISystemAssignValue(void);
extern void GUISystemButtonAssign(void);
extern void GUISystemButtonHalt(void);
extern void GUISystemButtonResume(void);
extern void GUISystemButtonReset(void);
extern void GUISystemButtonEnumerate(void);
extern void GUISystemCheckHeartbeat(void);
extern void GUIMenuUpdateFirmware(void);
extern void GUIUpdateFirmware(void);
extern void GUIRecoverDevice(void);
extern void GUIPeriodicStatusDropDownStatusMessage(void);
extern void GUIPeriodicStatusIntervalSet(void);
extern void GUIPeriodicStatusMessagesListSelect(void);
extern void GUIPeriodicStatusPayloadListSelect(void);
extern void GUIPeriodicStatusButtonAddRemoveSelect(int iDirection);
extern void GUIPeriodicStatusCheckFilterUsed(void);
extern void GUIPeriodicStatusCheckEnableSelect(int iCheckBoxState);
extern int PeriodicStatusIsMessageOn(unsigned long ulMsgID);
extern void GUIPeriodicStatusHistorySetup(char cMsgSel);
extern void GUIPeriodicStatusHistorySetupAndClear(char cMsgSel);
extern void GUIPeriodicStatusHistoryCopy(char cMsgSel);
extern void GUIExtendedStatusButtonToggle(void);
extern void GUIExtendedStatusDropDownMessageSelect(void);
extern void GUIExtendedStatusStkyLimitSelect(void);
extern void GUIExtendedStatusStkySoftLimitSelect(void);
extern void GUIExtendedStatusFaultCountSelect(int iFault);
extern void GUIExtendedStatusPowerSelect(void);
extern void GUIFaultIndicatorSelect(void);
extern void GUIExtendedStatusStickyFaultSelect(int iIdx);

extern tBoardStatus g_sBoardStatus;
extern tBoardState g_sBoardState[];
extern tPStatMsgEncodes g_sPStatMsgs[];

#endif
