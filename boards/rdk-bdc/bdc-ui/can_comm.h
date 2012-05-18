//*****************************************************************************
//
// can_comm.h - Prototypes for the CAN communication functions.
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
// This is part of revision 8555 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#ifndef __CAN_COMM_H__
#define __CAN_COMM_H__

//*****************************************************************************
//
// This function sends a system halt command.
//
//*****************************************************************************
#define CANSystemHalt()                                                       \
        CANSendMessage(CAN_MSGID_API_SYSHALT, 0, 0, 0)

//*****************************************************************************
//
// This function sends a system resume command.
//
//*****************************************************************************
#define CANSystemResume()                                                     \
        CANSendMessage(CAN_MSGID_API_SYSRESUME, 0, 0, 0)

//*****************************************************************************
//
// This function sends a system reset command.
//
//*****************************************************************************
#define CANSystemReset()                                                      \
        CANSendMessage(CAN_MSGID_API_SYSRST, 0, 0, 0)

//*****************************************************************************
//
// This function sends a device ID assignment command.
//
//*****************************************************************************
#define CANAssign(ucID)                                                       \
        CANSendMessage(CAN_MSGID_API_DEVASSIGN, 1, (ucID) & 0x3f, 0)

//*****************************************************************************
//
// This function sends a device enumeration command.
//
//*****************************************************************************
#define CANEnumerate()                                                        \
        CANSendMessage(CAN_MSGID_API_ENUMERATE, 0, 0, 0)

//*****************************************************************************
//
// This function sends a synchronous update command.
//
//*****************************************************************************
#define CANSyncUpdate(ucGroup)                                                \
        CANSendMessage(CAN_MSGID_API_SYNC, 1, (ucGroup) & 0xff, 0)

//*****************************************************************************
//
// The function sends a firmware version request to the current device ID.
//
//*****************************************************************************
#define CANFirmwareVersion()                                                  \
        CANSendMessage(CAN_MSGID_API_FIRMVER | g_ulCurrentID, 0, 0, 0)

//*****************************************************************************
//
// This function sends a voltage mode enable command to the current device ID.
//
//*****************************************************************************
#define CANVoltageModeEnable()                                                \
        CANSendMessage(LM_API_VOLT_EN | g_ulCurrentID, 0, 0, 0)

//*****************************************************************************
//
// This function sends a voltage mode disable command to the current device ID.
//
//*****************************************************************************
#define CANVoltageModeDisable()                                               \
        CANSendMessage(LM_API_VOLT_DIS | g_ulCurrentID, 0, 0, 0)

//*****************************************************************************
//
// This function sends a voltage set command to the current device ID.
//
//*****************************************************************************
#define CANVoltageSet(sVoltage, ucGroup)                                      \
        CANSendMessage(LM_API_VOLT_SET | g_ulCurrentID, 3,                    \
                       ((sVoltage) & 0xffff) | (((ucGroup) & 0xff) << 16), 0)

//*****************************************************************************
//
// This function sends a voltage ramp rate set command to the current device
// ID.
//
//*****************************************************************************
#define CANVoltageRampSet(usRamp)                                             \
        CANSendMessage(LM_API_VOLT_SET_RAMP | g_ulCurrentID, 2,               \
                       ((usRamp) & 0xffff), 0)

//*****************************************************************************
//
// This function sends a voltage compensation mode enable command to the
// current device ID.
//
//*****************************************************************************
#define CANVCompModeEnable()                                                  \
        CANSendMessage(LM_API_VCOMP_EN | g_ulCurrentID, 0, 0, 0)

//*****************************************************************************
//
// This function sends a voltage compensation mode disable command to the
// current device ID.
//
//*****************************************************************************
#define CANVCompModeDisable()                                                 \
        CANSendMessage(LM_API_VCOMP_DIS | g_ulCurrentID, 0, 0, 0)

//*****************************************************************************
//
// This function sends a voltage compensation set command to the current device
// ID.
//
//*****************************************************************************
#define CANVCompSet(sVoltage, ucGroup)                                        \
        CANSendMessage(LM_API_VCOMP_SET | g_ulCurrentID, 3,                   \
                       ((sVoltage) & 0xffff) | (((ucGroup) & 0xff) << 16), 0)

//*****************************************************************************
//
// This function sends a voltage compensation input ramp rate set command to
// the current device ID.
//
//*****************************************************************************
#define CANVCompInRampSet(usRamp)                                             \
        CANSendMessage(LM_API_VCOMP_IN_RAMP | g_ulCurrentID, 2,               \
                       ((usRamp) & 0xffff), 0)

//*****************************************************************************
//
// This function sends a voltage compensation tracking ramp rate set command to
// the current device ID.
//
//*****************************************************************************
#define CANVCompCompRampSet(usRamp)                                           \
        CANSendMessage(LM_API_VCOMP_COMP_RAMP | g_ulCurrentID, 2,             \
                       ((usRamp) & 0xffff), 0)

//*****************************************************************************
//
// This function sends a speed mode enable command to the current device ID.
//
//*****************************************************************************
#define CANSpeedModeEnable()                                                  \
        CANSendMessage(LM_API_SPD_EN | g_ulCurrentID, 0, 0, 0)

//*****************************************************************************
//
// This function sends a speed mode disable command to the current device ID.
//
//*****************************************************************************
#define CANSpeedModeDisable()                                                 \
        CANSendMessage(LM_API_SPD_DIS | g_ulCurrentID, 0, 0, 0)

//*****************************************************************************
//
// This function sends a speed set command to the current device ID.
//
//*****************************************************************************
#define CANSpeedSet(lSpeed, ucGroup)                                          \
        CANSendMessage(LM_API_SPD_SET | g_ulCurrentID, 5, (lSpeed),           \
                       (ucGroup) & 0xff)

//*****************************************************************************
//
// This function sends a speed controller P gain set command to the current
// device ID.
//
//*****************************************************************************
#define CANSpeedPGainSet(lPGain)                                              \
        CANSendMessage(LM_API_SPD_PC | g_ulCurrentID, 4, (lPGain), 0)

//*****************************************************************************
//
// This function sends a speed controller I gain set command to the current
// device ID.
//
//*****************************************************************************
#define CANSpeedIGainSet(lPGain)                                              \
        CANSendMessage(LM_API_SPD_IC | g_ulCurrentID, 4, (lPGain), 0)

//*****************************************************************************
//
// This function sends a speed controller D gain set command to the current
// device ID.
//
//*****************************************************************************
#define CANSpeedDGainSet(lPGain)                                              \
        CANSendMessage(LM_API_SPD_DC | g_ulCurrentID, 4, (lPGain), 0)

//*****************************************************************************
//
// This function sends a speed controller reference command to the current
// device ID.
//
//*****************************************************************************
#define CANSpeedRefSet(ucRef)                                                 \
        CANSendMessage(LM_API_SPD_REF | g_ulCurrentID, 1, ((ucRef) & 0xff),   \
                       0)

//*****************************************************************************
//
// This function sends a position mode enable command to the current device ID.
//
//*****************************************************************************
#define CANPositionModeEnable(lPos)                                           \
        CANSendMessage(LM_API_POS_EN | g_ulCurrentID, 4, (lPos), 0)

//*****************************************************************************
//
// This function sends a position mode disable command to the current device
// ID.
//
//*****************************************************************************
#define CANPositionModeDisable()                                              \
        CANSendMessage(LM_API_POS_DIS | g_ulCurrentID, 0, 0, 0)

//*****************************************************************************
//
// This function sends a position set command to the current device ID.
//
//*****************************************************************************
#define CANPositionSet(lPosition, ucGroup)                                    \
        CANSendMessage(LM_API_POS_SET | g_ulCurrentID, 5, (lPosition),        \
                       (ucGroup) & 0xff)

//*****************************************************************************
//
// This function sends a position controller P gain set command to the current
// device ID.
//
//*****************************************************************************
#define CANPositionPGainSet(lPGain)                                           \
        CANSendMessage(LM_API_POS_PC | g_ulCurrentID, 4, (lPGain), 0)

//*****************************************************************************
//
// This function sends a position controller I gain set command to the current
// device ID.
//
//*****************************************************************************
#define CANPositionIGainSet(lPGain)                                           \
        CANSendMessage(LM_API_POS_IC | g_ulCurrentID, 4, (lPGain), 0)

//*****************************************************************************
//
// This function sends a position controller D gain set command to the current
// device ID.
//
//*****************************************************************************
#define CANPositionDGainSet(lPGain)                                           \
        CANSendMessage(LM_API_POS_DC | g_ulCurrentID, 4, (lPGain), 0)

//*****************************************************************************
//
// This function sends a position controller reference command to the current
// device ID.
//
//*****************************************************************************
#define CANPositionRefSet(ucRef)                                              \
        CANSendMessage(LM_API_POS_REF | g_ulCurrentID, 1, ((ucRef) & 0xff),   \
                       0)

//*****************************************************************************
//
// This function sends a current mode enable command to the current device ID.
//
//*****************************************************************************
#define CANCurrentModeEnable()                                                \
        CANSendMessage(LM_API_ICTRL_EN | g_ulCurrentID, 0, 0, 0)

//*****************************************************************************
//
// This function sends a current mode disable command to the current device ID.
//
//*****************************************************************************
#define CANCurrentModeDisable()                                               \
        CANSendMessage(LM_API_ICTRL_DIS | g_ulCurrentID, 0, 0, 0)

//*****************************************************************************
//
// This function sends a current set command to the current device ID.
//
//*****************************************************************************
#define CANCurrentSet(sCurrent, ucGroup)                                      \
        CANSendMessage(LM_API_ICTRL_SET | g_ulCurrentID, 3,                   \
                       ((sCurrent) & 0xffff) | (((ucGroup) & 0xff) << 16), 0)

//*****************************************************************************
//
// This function sends a current controller P gain set command to the current
// device ID.
//
//*****************************************************************************
#define CANCurrentPGainSet(lPGain)                                            \
        CANSendMessage(LM_API_ICTRL_PC | g_ulCurrentID, 4, (lPGain), 0)

//*****************************************************************************
//
// This function sends a current controller I gain set command to the current
// device ID.
//
//*****************************************************************************
#define CANCurrentIGainSet(lPGain)                                            \
        CANSendMessage(LM_API_ICTRL_IC | g_ulCurrentID, 4, (lPGain), 0)

//*****************************************************************************
//
// This function sends a current controller D gain set command to the current
// device ID.
//
//*****************************************************************************
#define CANCurrentDGainSet(lPGain)                                            \
        CANSendMessage(LM_API_ICTRL_DC | g_ulCurrentID, 4, (lPGain), 0)

//*****************************************************************************
//
// This function sends a number of brushes configuration command to the current
// device ID.
//
//*****************************************************************************
#define CANConfigNumBrushes(ucBrushes)                                        \
        CANSendMessage(LM_API_CFG_NUM_BRUSHES | g_ulCurrentID, 1,             \
                       (ucBrushes), 0)

//*****************************************************************************
//
// This function sends a number of encoder lines configuration command to the
// current device ID.
//
//*****************************************************************************
#define CANConfigEncoderLines(usLines)                                        \
        CANSendMessage(LM_API_CFG_ENC_LINES | g_ulCurrentID, 2, (usLines), 0)

//*****************************************************************************
//
// This function sends a number of pot turns configuration command to the
// current device ID.
//
//*****************************************************************************
#define CANConfigPotTurns(usTurns)                                            \
        CANSendMessage(LM_API_CFG_POT_TURNS | g_ulCurrentID, 2, (usTurns), 0)

//*****************************************************************************
//
// This function sends a brake/coast configuration command to the current
// device ID.
//
//*****************************************************************************
#define CANConfigBrakeCoast(ucBrakeCoast)                                     \
        CANSendMessage(LM_API_CFG_BRAKE_COAST | g_ulCurrentID, 1,             \
                       (ucBrakeCoast), 0)

//*****************************************************************************
//
// This function sends a soft limit switch configuration command to the current
// device ID.
//
//*****************************************************************************
#define CANConfigLimitMode(ucMode)                                            \
        CANSendMessage(LM_API_CFG_LIMIT_MODE | g_ulCurrentID, 1, (ucMode), 0)

//*****************************************************************************
//
// This function sends a forward soft limit switch configuration command to the
// current device ID.
//
//*****************************************************************************
#define CANConfigLimitForward(lPos, ucCompare)                                \
        CANSendMessage(LM_API_CFG_LIMIT_FWD | g_ulCurrentID, 5, (lPos),       \
                       (ucCompare))

//*****************************************************************************
//
// This function sends a reverse soft limit switch configuration command to the
// current device ID.
//
//*****************************************************************************
#define CANConfigLimitReverse(lPos, ucCompare)                                \
        CANSendMessage(LM_API_CFG_LIMIT_REV | g_ulCurrentID, 5, (lPos),       \
                       (ucCompare))

//*****************************************************************************
//
// This function sends a maximum output voltage configuration command to the
// current device ID.
//
//*****************************************************************************
#define CANConfigMaxVOut(usVoltage)                                           \
        CANSendMessage(LM_API_CFG_MAX_VOUT | g_ulCurrentID, 2, (usVoltage), 0)

//*****************************************************************************
//
// This function sends a firmware update start command to the current device
// ID.
//
//*****************************************************************************
#define CANUpdateStart()                                                      \
        CANSendMessage(CAN_MSGID_API_UPDATE, 1, g_ulCurrentID & 0xff, 0)

//*****************************************************************************
//
// This function sends a boot loader ping command.
//
//*****************************************************************************
#define CANUpdatePing()                                                       \
        CANSendMessage(LM_API_UPD_PING, 0, 0, 0)

//*****************************************************************************
//
// This function sends a boot loader download command.
//
//*****************************************************************************
#define CANUpdateDownload(ulAddr, ulSize)                                     \
        CANSendMessage(LM_API_UPD_DOWNLOAD, 8, ulAddr, ulSize)

//*****************************************************************************
//
// This function sends a boot loader send data command.
//
//*****************************************************************************
#define CANUpdateSendData(ulSize, ulData1, ulData2)                           \
        CANSendMessage(LM_API_UPD_SEND_DATA, ulSize, ulData1, ulData2)

//*****************************************************************************
//
// This function sends a boot loader reset command.
//
//*****************************************************************************
#define CANUpdateReset()                                                      \
        CANSendMessage(LM_API_UPD_RESET, 0, 0, 0)

//*****************************************************************************
//
// This function sends a clear power status command.
//
//*****************************************************************************
#define CANStatusPowerClear()                                                 \
        CANSendMessage(LM_API_STATUS_POWER | g_ulCurrentID, 1, 1, 0)

//*****************************************************************************
//
// The bit positions of the flags in g_ulStatusFlags.
//
//*****************************************************************************
#define STATUS_FLAG_VOUT        0
#define STATUS_FLAG_VBUS        1
#define STATUS_FLAG_CURRENT     2
#define STATUS_FLAG_TEMP        3
#define STATUS_FLAG_POS         4
#define STATUS_FLAG_SPEED       5
#define STATUS_FLAG_LIMIT       6
#define STATUS_FLAG_FAULT       7
#define STATUS_FLAG_FIRMVER     8
#define STATUS_ENABLED          31

//*****************************************************************************
//
// Prototypes.
//
//*****************************************************************************
extern unsigned long g_pulStatusEnumeration[2];
extern unsigned long g_ulStatusFlags;
extern long g_lStatusVout;
extern unsigned long g_ulStatusVbus;
extern long g_lStatusCurrent;
extern unsigned long g_ulStatusTemperature;
extern long g_lStatusPosition;
extern long g_lStatusSpeed;
extern unsigned long g_ulStatusLimit;
extern unsigned long g_ulStatusFault;
extern unsigned long g_ulStatusFirmwareVersion;
extern unsigned long g_ulCurrentID;
extern volatile unsigned long g_ulCANUpdateAck;
extern void CANCommInit(void);
extern unsigned long CANSendMessage(unsigned long ulMsgID,
                                    unsigned long ulDataLen,
                                    unsigned long ulData1,
                                    unsigned long ulData2);
extern unsigned long CANReadParameter(unsigned long ulID,
                                      unsigned long *pulDataLen,
                                      unsigned long *pulParam1,
                                      unsigned long *pulParam2);
extern void CANSetID(unsigned long ulID);
extern void CAN0IntHandler(void);
extern void CANTick(void);
extern void CANStatusEnable(void);
extern void CANStatusDisable(void);

#endif // __CAN_COMM_H__
