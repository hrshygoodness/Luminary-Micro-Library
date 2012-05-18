//*****************************************************************************
//
// message.c - Generic message handling functions for the UART and CAN
//             interfaces.
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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "shared/can_proto.h"
#include "adc_ctrl.h"
#include "can_if.h"
#include "commands.h"
#include "constants.h"
#include "controller.h"
#include "encoder.h"
#include "hbridge.h"
#include "led.h"
#include "limit.h"
#include "message.h"
#include "param.h"
#include "uart_if.h"

extern void CallBootloader(void);

//*****************************************************************************
//
// The buffer that contains the response message.
//
//*****************************************************************************
unsigned char g_pucResponse[12];

//*****************************************************************************
//
// The length of the response message.
//
//*****************************************************************************
unsigned long g_ulResponseLength;

//*****************************************************************************
//
// The state of the message state machine.
//
//*****************************************************************************
#define STATE_IDLE              0
#define STATE_ASSIGN            1
#define STATE_ENUM              2
static unsigned long g_ulMessageState = STATE_IDLE;

//*****************************************************************************
//
// The number of clock ticks until the current state is finished.
//
//*****************************************************************************
static unsigned long g_ulTickCount = 0;

//*****************************************************************************
//
// This holds the pending device number during the assignment state.
//
//*****************************************************************************
static unsigned char g_ucDevNumPending;

//*****************************************************************************
//
// This holds the interface to respond on when enumerating.
//
//*****************************************************************************
static unsigned long g_ulEnumInterface;

//*****************************************************************************
//
// The group number for pending update to the target voltage, voltage
// compensation, current, speed, and position.
//
//*****************************************************************************
static unsigned char g_ucVoltageGroup = 0;
static unsigned char g_ucVCompGroup = 0;
static unsigned char g_ucCurrentGroup = 0;
static unsigned char g_ucSpeedGroup = 0;
static unsigned char g_ucPositionGroup = 0;

//*****************************************************************************
//
// The value for the pending update to the target voltage, voltage
// compensation, current, speed, and position.
//
//*****************************************************************************
static long g_lPendingVoltage;
static long g_lPendingVComp;
static long g_lPendingCurrent;
static long g_lPendingSpeed;
static long g_lPendingPosition;

//*****************************************************************************
//
// The Period and Enable state for each of the four periodic status messages.
// A 0 value means disabled, a value from 1 - 65535 enables the message at that
// period in ms.
//
//*****************************************************************************
static unsigned short g_pusPStatPeriod[4];

//*****************************************************************************
//
// The configured format for each periodic status messages.
//
//*****************************************************************************
static unsigned char g_ppucPStatFormat[4][8];

//*****************************************************************************
//
// The period counters for the periodic status messages.
//
//*****************************************************************************
static unsigned short g_pusPStatCounter[4];

//*****************************************************************************
//
// The periodic status messages that need to be sent out.
//
//*****************************************************************************
unsigned char g_ppucPStatMessages[4][8];

//*****************************************************************************
//
// The length of the periodic status messages.
//
//*****************************************************************************
unsigned char g_pucPStatMessageLen[4];

//*****************************************************************************
//
// A set of flags that indicate the periodic status messages that are pending
// transmission.
//
//*****************************************************************************
unsigned long g_ulPStatFlags;

//*****************************************************************************
//
// Constructs the response to a message, placing it into a buffer that is used
// by the active interface to perform the actual transfer.
//
//*****************************************************************************
static void
MessageSendResponse(unsigned long ulID, unsigned char *pucData,
                    unsigned long ulMsgLen)
{
    unsigned long *pulData, *pulResponse;

    //
    // Create local pointers of a different type to avoid later type casting.
    //
    pulData = (unsigned long *)pucData;
    pulResponse = (unsigned long *)g_pucResponse;

    //
    // Save the message ID at the beginning of the buffer.
    //
    pulResponse[0] = ulID;

    //
    // Save the data words (if any) to the buffer.
    //
    if(pucData)
    {
        pulResponse[1] = pulData[0];
        pulResponse[2] = pulData[1];
    }

    //
    // Save the length of the message.
    //
    g_ulResponseLength = ulMsgLen + 4;
}

//*****************************************************************************
//
// Handles the system commands.
//
//*****************************************************************************
static void
MessageSystemHandler(unsigned long ulID, unsigned char *pucData,
                     unsigned long ulMsgLen)
{
    unsigned long pulData[2];

    //
    // Determine which message has been received.
    //
    switch(ulID & (~CAN_MSGID_DEVNO_M))
    {
        //
        // A system halt request was received.
        //
        case CAN_MSGID_API_SYSHALT:
        {
            //
            // Reset pending updates on a system halt.
            //
            g_ucVoltageGroup = 0;
            g_ucVCompGroup = 0;
            g_ucCurrentGroup = 0;
            g_ucSpeedGroup = 0;
            g_ucPositionGroup = 0;

            //
            // Force the motor to neutral.
            //
            CommandForceNeutral();

            //
            // Set the halt flag so that further motion commands are ignored
            // until a resume.
            //
            ControllerHaltSet();

            //
            // This message has been handled.
            //
            break;
        }

        //
        // A system resume request was received.
        //
        case CAN_MSGID_API_SYSRESUME:
        {
            //
            // Clear the halt flag so that further motion commands can be
            // received.
            //
            ControllerHaltClear();

            //
            // This message has been handled.
            //
            break;
        }

        //
        // A system reset request was received.
        //
        case CAN_MSGID_API_SYSRST:
        {
            //
            // Reset the microcontroller.
            //
            ROM_SysCtlReset();

            //
            // Control should never get here, but just in case...
            //
            while(1)
            {
            }
        }

        //
        // A enumeration request was received.
        //
        case CAN_MSGID_API_ENUMERATE:
        {
            //
            // Enumeration should be ignored if in assignment state or if there
            // is no device number set.
            //
            if((g_ulMessageState == STATE_IDLE) &&
               (g_sParameters.ucDeviceNumber != 0))
            {
                //
                // Wait 1ms * the current device number.
                //
                g_ulTickCount = ((UPDATES_PER_SECOND *
                                  g_sParameters.ucDeviceNumber) / 1000);

                //
                // Switch to the enumeration state to wait to send out the
                // enumeration data.
                //
                g_ulMessageState = STATE_ENUM;

                //
                // Save the current interface type so that we can respond in
                // the face of the interface changing.
                //
                g_ulEnumInterface = ControllerLinkType();
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // This was a request to assign a new device identifier.
        //
        case CAN_MSGID_API_DEVASSIGN:
        {
            //
            // Ignore this request if the required ID is not supplied.
            //
            if(ulMsgLen == 1)
            {
                //
                // If an out of bounds device ID was specified then ignore the
                // request.
                //
                if(pucData[0] > CAN_MSGID_DEVNO_M)
                {
                }
                else if(pucData[0] != 0)
                {
                    //
                    // Save the pending address.
                    //
                    g_ucDevNumPending = pucData[0];

                    //
                    // Set the tick that will trigger leaving assignment
                    // mode.
                    //
                    g_ulTickCount = 5 * UPDATES_PER_SECOND;

                    //
                    // This is pending until commited.
                    //
                    g_ulMessageState = STATE_ASSIGN;

                    //
                    // Force the motor to neutral.
                    //
                    CommandForceNeutral();

                    //
                    // Let the world know that assigment state has started.
                    //
                    LEDAssignStart();
                }
                else
                {
                    //
                    // Reset the CAN device number.
                    //
                    CANIFSetID(0);

                    //
                    // Reset the device number.
                    //
                    g_sParameters.ucDeviceNumber = 0;

                    //
                    // Save the new device number.
                    //
                    ParamSave();

                    //
                    // Force the state machine into the idle state.
                    //
                    g_ulMessageState = STATE_IDLE;
                }
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Handle the device query command.
        //
        case CAN_MSGID_API_DEVQUERY:
        {
            //
            // Ignore this command if the device ID does not match the current
            // assignment.
            //
            if(((ulID & CAN_MSGID_DEVNO_M) >> CAN_MSGID_DEVNO_S) ==
               g_sParameters.ucDeviceNumber)
            {
                //
                // Send back the response to the device query.
                //
                pulData[0] = 0;
                pulData[1] = 0;
                ((unsigned char *)pulData)[0] =
                    (CAN_MSGID_DTYPE_MOTOR >> CAN_MSGID_DTYPE_S);
                ((unsigned char *)pulData)[1] =
                    (CAN_MSGID_MFR_LM >> CAN_MSGID_MFR_S);
                MessageSendResponse(ulID, (unsigned char *)pulData, 8);
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // This was a request to start a firmware update.
        //
        case CAN_MSGID_API_UPDATE:
        {
            //
            // Check if there is an ID to update and if it belongs to this
            // board.
            //
            if((ulMsgLen != 1) || (pucData[0] != g_sParameters.ucDeviceNumber))
            {
                break;
            }

            //
            // Call the boot loader, this call will not return.
            //
            CallBootloader();
        }

        //
        // Handle the sync command.
        //
        case CAN_MSGID_API_SYNC:
        {
            //
            // Ignore this command if there is no data supplied.
            //
            if(ulMsgLen == 1)
            {
                //
                // If there is a pending voltage update then set the value now.
                //
                if((g_ucVoltageGroup & pucData[0]) != 0)
                {
                    //
                    // Send the voltage on to the handler.
                    //
                    CommandVoltageSet(g_lPendingVoltage);

                    //
                    // Update is no longer pending.
                    //
                    g_ucVoltageGroup = 0;
                }

                //
                // If there is a pending voltage compensation update then set
                // the value now.
                //
                if((g_ucVCompGroup & pucData[0]) != 0)
                {
                    //
                    // Send the voltage on to the handler.
                    //
                    CommandVCompSet(g_lPendingVComp);

                    //
                    // Update is no longer pending.
                    //
                    g_ucVCompGroup = 0;
                }

                //
                // If there is a pending current update then set the value now.
                //
                if((g_ucCurrentGroup & pucData[0]) != 0)
                {
                    //
                    // Send the current on to the handler.
                    //
                    CommandCurrentSet(g_lPendingCurrent);

                    //
                    // Update is no longer pending.
                    //
                    g_ucCurrentGroup = 0;
                }

                //
                // If there is a pending speed update then set the value now.
                //
                if((g_ucSpeedGroup & pucData[0]) != 0)
                {
                    //
                    // Send the speed setting to the handler.
                    //
                    CommandSpeedSet(g_lPendingSpeed);

                    //
                    // Update is no longer pending.
                    //
                    g_ucSpeedGroup = 0;
                }

                //
                // If there is a pending position update then set the value
                // now.
                //
                if((g_ucPositionGroup & pucData[0]) != 0)
                {
                    //
                    // Send the speed setting to the handler.
                    //
                    CommandPositionSet(g_lPendingPosition);

                    //
                    // Update is no longer pending.
                    //
                    g_ucPositionGroup = 0;
                }
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Handle the firmware version command.
        //
        case CAN_MSGID_API_FIRMVER:
        {
            //
            // Ignore this command if the device ID does not match the current
            // assignment.
            //
            if(((ulID & CAN_MSGID_DEVNO_M) >> CAN_MSGID_DEVNO_S) ==
               g_sParameters.ucDeviceNumber)
            {
                //
                // Send back the firmware version.
                //
                MessageSendResponse(ulID,
                                    (unsigned char *)&g_ulFirmwareVersion, 4);
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Nothing is done in response to a heart beat command, this just causes
        // the controller to hit the watchdog below.
        //
        case CAN_MSGID_API_HEARTBEAT:
        {
            //
            // This message has been handled.
            //
            break;
        }

        //
        // An unknown command was received.
        //
        default:
        {
            //
            // This message has been handled.
            //
            break;
        }
    }
}

//*****************************************************************************
//
// Handles the voltage control mode commands.
//
//*****************************************************************************
static unsigned long
MessageVoltageHandler(unsigned long ulID, unsigned char *pucData,
                      unsigned long ulMsgLen)
{
    unsigned long ulValue, ulAck, ulAPI;
    unsigned short *pusData;
    short *psData;

    //
    // Create local pointers of different types to the message data to avoid
    // later type casting.
    //
    pusData = (unsigned short *)pucData;
    psData = (short *)pucData;

    //
    // By default, no ACK should be supplied.
    //
    ulAck = 0;

    //
    // Determine the specific API being called.
    //
    ulAPI = ulID & (~CAN_MSGID_DEVNO_M);

    //
    // Mask out the device number and see what the command is.
    //
    switch(ulAPI)
    {
        //
        // Enable voltage control mode.
        //
        case LM_API_VOLT_EN:
        {
            //
            // Ignore this command if the controller is halted.
            //
            if(!ControllerHalted())
            {
                //
                // Enable voltage control mode.
                //
                CommandVoltageMode(true);

                //
                // Reset pending updates when switching modes.
                //
                g_ucVoltageGroup = 0;
                g_ucVCompGroup = 0;
                g_ucCurrentGroup = 0;
                g_ucSpeedGroup = 0;
                g_ucPositionGroup = 0;
            }

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Disable voltage control mode.
        //
        case LM_API_VOLT_DIS:
        {
            //
            // Disable voltage control mode.
            //
            CommandVoltageMode(false);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the output voltage.
        //
        case LM_API_VOLT_SET:
        case LM_API_VOLT_SET_NO_ACK:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the target output voltage in response.
                //
                ulValue = ControllerVoltageTargetGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);
            }
            else if((ulMsgLen == 2) || (ulMsgLen == 3))
            {
                //
                // Ignore this command if the controller is halted.
                //
                if(!ControllerHalted())
                {
                    //
                    // If there was either no group specified or if the value
                    // specified was zero then update the voltage, otherwise
                    // the voltage update is pending until it is committed.
                    //
                    if((ulMsgLen == 2) || (pucData[2] == 0))
                    {
                        //
                        // Send the voltage on to the handler.
                        //
                        CommandVoltageSet(psData[0]);
                    }
                    else
                    {
                        //
                        // Save the voltage setting and the group.
                        //
                        g_lPendingVoltage = psData[0];
                        g_ucVoltageGroup = pucData[2];
                    }
                }

                //
                // Ack this command if it is meant to be acked.
                //
                if(ulAPI != LM_API_VOLT_SET_NO_ACK)
                {
                    ulAck = 1;
                }
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Motor controller Set Voltage Ramp Rate received.
        //
        case LM_API_VOLT_SET_RAMP:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the voltage ramp rate in response.
                //
                ulValue = ControllerVoltageRateGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // Send the voltage ramp rate to the handler.
                //
                CommandVoltageRateSet(pusData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // An unknown command was received.
        //
        default:
        {
            //
            // This message has been handled.
            //
            break;
        }
    }

    //
    // Return the ACK indicator.
    //
    return(ulAck);
}

//*****************************************************************************
//
// Handles the voltage compensation control mode commands.
//
//*****************************************************************************
static unsigned long
MessageVoltageCompHandler(unsigned long ulID, unsigned char *pucData,
                          unsigned long ulMsgLen)
{
    unsigned long ulValue, ulAck, ulAPI;
    unsigned short *pusData;
    short *psData;

    //
    // Create local pointers of different types to the message data to avoid
    // later type casting.
    //
    pusData = (unsigned short *)pucData;
    psData = (short *)pucData;

    //
    // By default, no ACK should be supplied.
    //
    ulAck = 0;

    //
    // Determine the specific API being called.
    //
    ulAPI = ulID & (~CAN_MSGID_DEVNO_M);

    //
    // Mask out the device number and see what the command is.
    //
    switch(ulAPI)
    {
        //
        // Enable voltage compensation control mode.
        //
        case LM_API_VCOMP_EN:
        {
            //
            // Ignore this command if the controller is halted.
            //
            if(!ControllerHalted())
            {
                //
                // Enable voltage compensation control mode.
                //
                CommandVCompMode(true);

                //
                // Reset pending updates when switching modes.
                //
                g_ucVoltageGroup = 0;
                g_ucVCompGroup = 0;
                g_ucCurrentGroup = 0;
                g_ucSpeedGroup = 0;
                g_ucPositionGroup = 0;
            }

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Disable voltage compensation control mode.
        //
        case LM_API_VCOMP_DIS:
        {
            //
            // Disable voltage compensation control mode.
            //
            CommandVCompMode(false);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the output voltage.
        //
        case LM_API_VCOMP_SET:
        case LM_API_VCOMP_SET_NO_ACK:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the target output voltage in response.
                //
                ulValue = ControllerVCompTargetGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);
            }
            else if((ulMsgLen == 2) || (ulMsgLen == 3))
            {
                //
                // Ignore this command if the controller is halted.
                //
                if(!ControllerHalted())
                {
                    //
                    // If there was either no group specified or if the value
                    // specified was zero then update the voltage, otherwise
                    // the voltage update is pending until it is committed.
                    //
                    if((ulMsgLen == 2) || (pucData[2] == 0))
                    {
                        //
                        // Send the voltage on to the handler.
                        //
                        CommandVCompSet(psData[0]);
                    }
                    else
                    {
                        //
                        // Save the voltage compenstaion setting and the group.
                        //
                        g_lPendingVComp = psData[0];
                        g_ucVCompGroup = pucData[2];
                    }
                }

                //
                // Ack this command if it is meant to be acked.
                //
                if(ulAPI != LM_API_VCOMP_SET_NO_ACK)
                {
                    ulAck = 1;
                }
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Motor controller set input voltage ramp rate received.
        //
        case LM_API_VCOMP_IN_RAMP:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the input voltage ramp rate in response.
                //
                ulValue = ControllerVCompInRateGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // Send the input voltage ramp rate to the handler.
                //
                CommandVCompInRampSet(pusData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Motor controller set compensation voltage ramp rate received.
        //
        case LM_API_VCOMP_COMP_RAMP:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the compensation voltage ramp rate in response.
                //
                ulValue = ControllerVCompCompRateGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // Send the compensation voltage ramp rate to the handler.
                //
                CommandVCompCompRampSet(pusData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // An unknown command was received.
        //
        default:
        {
            //
            // This message has been handled.
            //
            break;
        }
    }

    //
    // Return the ACK indicator.
    //
    return(ulAck);
}

//*****************************************************************************
//
// Handles the current control mode commands.
//
//*****************************************************************************
static unsigned long
MessageCurrentHandler(unsigned long ulID, unsigned char *pucData,
                      unsigned long ulMsgLen)
{
    unsigned long ulValue, ulAck, *pulData, ulAPI;
    short *psData;

    //
    // Create local pointers of different types to the message data to avoid
    // later type casting.
    //
    pulData = (unsigned long *)pucData;
    psData = (short *)pucData;

    //
    // By default, no ACK should be supplied.
    //
    ulAck = 0;

    //
    // Determine the specific API being called.
    //
    ulAPI = ulID & (~CAN_MSGID_DEVNO_M);

    //
    // Mask out the device number and see what the command is.
    //
    switch(ulAPI)
    {
        //
        // Enable current control mode.
        //
        case LM_API_ICTRL_EN:
        {
            //
            // Ignore this command if the controller is halted.
            //
            if(!ControllerHalted())
            {
                //
                // Enable current control mode.
                //
                CommandCurrentMode(true);

                //
                // Reset pending updates when switching modes.
                //
                g_ucVoltageGroup = 0;
                g_ucVCompGroup = 0;
                g_ucCurrentGroup = 0;
                g_ucSpeedGroup = 0;
                g_ucPositionGroup = 0;
            }

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Disable current control mode.
        //
        case LM_API_ICTRL_DIS:
        {
            //
            // Disable current control mode.
            //
            CommandCurrentMode(false);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the target winding current for the motor.
        //
        case LM_API_ICTRL_SET:
        case LM_API_ICTRL_SET_NO_ACK:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the target current in response.
                //
                ulValue = ControllerCurrentTargetGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);
            }
            else if((ulMsgLen == 2) || (ulMsgLen == 3))
            {
                //
                // Ignore this command if the controller is halted.
                //
                if(!ControllerHalted())
                {
                    //
                    // If there was either no group specified or if the value
                    // specified was zero then update the current, otherwise
                    // the current update is pending until it is committed.
                    //
                    if((ulMsgLen == 2) || (pucData[2] == 0))
                    {
                        //
                        // The value is a 8.8 fixed-point value that specifies
                        // the current in Amperes.
                        //
                        CommandCurrentSet(psData[0]);
                    }
                    else
                    {
                        //
                        // Save the current setting and the group.
                        //
                        g_lPendingCurrent = psData[0];
                        g_ucCurrentGroup = pucData[2];
                    }
                }

                //
                // Ack this command if it is meant to be acked.
                //
                if(ulAPI != LM_API_ICTRL_SET_NO_ACK)
                {
                    ulAck = 1;
                }
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the proportional constant used in the PID algorithm.
        //
        case LM_API_ICTRL_PC:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the proportional constant in response.
                //
                ulValue = ControllerCurrentPGainGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the proportional constant.
                //
                CommandCurrentPSet(pulData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the integral constant used in the PID algorithm.
        //
        case LM_API_ICTRL_IC:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the integral constant in response.
                //
                ulValue = ControllerCurrentIGainGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the integral constant.
                //
                CommandCurrentISet(pulData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the differential constant used in the PID algorithm.
        //
        case LM_API_ICTRL_DC:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the differential constant in response.
                //
                ulValue = ControllerCurrentDGainGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the differential constant.
                //
                CommandCurrentDSet(pulData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // An unknown command was received.
        //
        default:
        {
            //
            // This message has been handled.
            //
            break;
        }
    }

    //
    // Return the ACK indicator.
    //
    return(ulAck);
}

//*****************************************************************************
//
// Handles the speed control mode commands.
//
//*****************************************************************************
static unsigned long
MessageSpeedHandler(unsigned long ulID, unsigned char *pucData,
                    unsigned long ulMsgLen)
{
    unsigned long ulValue, ulAck, ulAPI;
    long *plData;

    //
    // Create local pointers of different types to the message data to avoid
    // later type casting.
    //
    plData = (long *)pucData;

    //
    // By default, no ACK should be supplied.
    //
    ulAck = 0;

    //
    // Determine the specific API being called.
    //
    ulAPI = ulID & (~CAN_MSGID_DEVNO_M);

    //
    // Mask out the device number and see what the command is.
    //
    switch(ulAPI)
    {
        //
        // Enable speed control mode.
        //
        case LM_API_SPD_EN:
        {
            //
            // Ignore this command if the controller is halted.
            //
            if(!ControllerHalted())
            {
                //
                // Enable speed control mode.
                //
                CommandSpeedMode(true);

                //
                // Reset pending updates when switching modes.
                //
                g_ucVoltageGroup = 0;
                g_ucVCompGroup = 0;
                g_ucCurrentGroup = 0;
                g_ucSpeedGroup = 0;
                g_ucPositionGroup = 0;
            }

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Disable speed control mode.
        //
        case LM_API_SPD_DIS:
        {
            //
            // Disable speed control mode.
            //
            CommandSpeedMode(false);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the target speed for the motor.
        //
        case LM_API_SPD_SET:
        case LM_API_SPD_SET_NO_ACK:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the target speed in response.
                //
                ulValue = ControllerSpeedTargetGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);
            }
            else if((ulMsgLen == 4) || (ulMsgLen == 5))
            {
                //
                // Ignore this command if the controller is halted.
                //
                if(!ControllerHalted())
                {
                    //
                    // If there was either no group specified or if the value
                    // specified was zero then update the speed, otherwise the
                    // speed update is pending until it is committed.
                    //
                    if((ulMsgLen == 4) || (pucData[4] == 0))
                    {
                        //
                        // When read as an unsigned short the value in pucData
                        // is the rotational speed in revolution per second.
                        //
                        CommandSpeedSet(plData[0]);
                    }
                    else
                    {
                        //
                        // Save the speed setting and the group.
                        //
                        g_lPendingSpeed = plData[0];
                        g_ucSpeedGroup = pucData[4];
                    }
                }

                //
                // Ack this command if it is meant to be acked.
                //
                if(ulAPI != LM_API_SPD_SET_NO_ACK)
                {
                    ulAck = 1;
                }
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the proportional constant used in the PID algorithm.
        //
        case LM_API_SPD_PC:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the proportional constant in response.
                //
                ulValue = ControllerSpeedPGainGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the proportional constant.
                //
                CommandSpeedPSet(plData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the integral constant used in the PID algorithm.
        //
        case LM_API_SPD_IC:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the integral constant in response.
                //
                ulValue = ControllerSpeedIGainGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the integral constant.
                //
                CommandSpeedISet(plData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the differential constant used in the PID algorithm.
        //
        case LM_API_SPD_DC:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the differential constant in response.
                //
                ulValue = ControllerSpeedDGainGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the differential constant.
                //
                CommandSpeedDSet(plData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the speed measurement reference.
        //
        case LM_API_SPD_REF:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the speed reference in response.
                //
                ulValue = ControllerSpeedSrcGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // Set the speed reference.
                //
                CommandSpeedSrcSet(pucData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // An unknown command was received.
        //
        default:
        {
            //
            // This message has been handled.
            //
            break;
        }
    }

    //
    // Return the ACK indicator.
    //
    return(ulAck);
}

//*****************************************************************************
//
// Handles the position control mode commands.
//
//*****************************************************************************
static unsigned long
MessagePositionHandler(unsigned long ulID, unsigned char *pucData,
                       unsigned long ulMsgLen)
{
    unsigned long ulValue, ulAck, ulAPI;
    long *plData;

    //
    // Create local pointers of different types to the message data to avoid
    // later type casting.
    //
    plData = (long *)pucData;

    //
    // By default, no ACK should be supplied.
    //
    ulAck = 0;

    //
    // Determine the specific API being called.
    //
    ulAPI = ulID & (~CAN_MSGID_DEVNO_M);

    //
    // Mask out the device number and see what the command is.
    //
    switch(ulAPI)
    {
        //
        // Enable position control mode.
        //
        case LM_API_POS_EN:
        {
            //
            // Ignore this command if the controller is halted.
            //
            if(!ControllerHalted())
            {
                //
                // See if the required data was supplied.
                //
                if(ulMsgLen == 4)
                {
                    //
                    // Enable position control mode and set the initial position
                    // as requested.
                    //
                    CommandPositionMode(true, plData[0]);

                    //
                    // Reset pending updates when switching modes.
                    //
                    g_ucVoltageGroup = 0;
                    g_ucVCompGroup = 0;
                    g_ucCurrentGroup = 0;
                    g_ucSpeedGroup = 0;
                    g_ucPositionGroup = 0;
                }
            }

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Disable position control mode.
        //
        case LM_API_POS_DIS:
        {
            //
            // Disable position control mode.
            //
            CommandPositionMode(false, 0);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the target shaft position.
        //
        case LM_API_POS_SET:
        case LM_API_POS_SET_NO_ACK:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the target position in response.
                //
                ulValue = ControllerPositionTargetGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);
            }
            else if((ulMsgLen == 4) || (ulMsgLen == 5))
            {
                //
                // Ignore this command if the controller is halted.
                //
                if(!ControllerHalted())
                {
                    //
                    // If there was either no group specified or if the value
                    // specified was zero then update the position, otherwise
                    // the position update is pending until it is committed.
                    //
                    if((ulMsgLen == 4) || (pucData[4] == 0))
                    {
                        //
                        // Send the 32 bit position to move to.
                        //
                        CommandPositionSet(plData[0]);
                    }
                    else
                    {
                        //
                        // Save the position setting and the group.
                        //
                        g_lPendingPosition = plData[0];
                        g_ucPositionGroup = pucData[4];
                    }
                }

                //
                // Ack this command if it is meant to be acked.
                //
                if(ulAPI != LM_API_POS_SET_NO_ACK)
                {
                    ulAck = 1;
                }
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the proportional constant used in the PID algorithm.
        //
        case LM_API_POS_PC:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the proportional constant in response.
                //
                ulValue = ControllerPositionPGainGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the proportional constant.
                //
                CommandPositionPSet(plData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the integral constant used in the PID algorithm.
        //
        case LM_API_POS_IC:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the integral constant in response.
                //
                ulValue = ControllerPositionIGainGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the integral constant.
                //
                CommandPositionISet(plData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the differential constant used in the PID algorithm.
        //
        case LM_API_POS_DC:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the differential constant in response.
                //
                ulValue = ControllerPositionDGainGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the differential constant.
                //
                CommandPositionDSet(plData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the the reference measurement source for position measurement.
        //
        case LM_API_POS_REF:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the position reference in response.
                //
                ulValue = ControllerPositionSrcGet();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // Set the position reference.
                //
                CommandPositionSrcSet(pucData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // An unknown command was received.
        //
        default:
        {
            //
            // This message has been handled.
            //
            break;
        }
    }

    //
    // Return the ACK indicator.
    //
    return(ulAck);
}

//*****************************************************************************
//
// Handles the update class commands.
//
//*****************************************************************************
unsigned long
MessageUpdateHandler(unsigned long ulID, unsigned char *pucData,
                     unsigned long ulMsgLen)
{
    unsigned long ulAck;
    //
    // By default, not response is supplied.
    //
    g_ulResponseLength = 0;

    //
    // By default, no ACK should be supplied.
    //
    ulAck = 0;

    switch(ulID & (~CAN_MSGID_DEVNO_M))
    {
        //
        // Handle the hardware version request.
        //
        case LM_API_HWVER:
        {
            unsigned char pucResponse[2];

            //
            // This was a request from another node on the system for the
            // hardware version.  If it was for this device then return the
            // hardware version.
            //
            if((ulID & CAN_MSGID_DEVNO_M) == g_sParameters.ucDeviceNumber)
            {
                //
                // Respond with the device number and the hardware version.
                //
                pucResponse[0] = g_sParameters.ucDeviceNumber;
                pucResponse[1] = g_ucHardwareVersion;

                //
                // Send back the hardware version.
                //
                MessageSendResponse(ulID, pucResponse, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // This is a response for the hardware version from another
                // node on the system that needs to be sent out the UART.
                //
                UARTIFSendMessage(LM_API_HWVER, pucData, 2);
            }

            //
            // This message has been handled.
            //
            break;
        }

        default:
        {
            break;
        }
    }

    return(ulAck);
}

//*****************************************************************************
//
// Reads the limit switch status, optionally clearing the sticky status.
//
//*****************************************************************************
static unsigned long
LimitStatusGet(unsigned long bClear)
{
    unsigned long ulValue;

    //
    // Default is that the limits are not "good".
    //
    ulValue = 0;

    //
    // See if the forward limit is in a "good" state.
    //
    if(LimitForwardOK())
    {
        ulValue |= LM_STATUS_LIMIT_FWD;
    }

    //
    // See if the forward soft limit is in a "good" state.
    //
    if(LimitSoftForwardOK())
    {
        ulValue |= LM_STATUS_LIMIT_SFWD;
    }

    //
    // See if the sticky forward limit flag is in a "good" state.
    //
    if(LimitStickyForwardOK())
    {
        ulValue |= LM_STATUS_LIMIT_STKY_FWD;
    }
    else if(bClear)
    {
        //
        // Clear the sticky forward limit flag since it has now been checked as
        // "bad".
        //
        LimitStickyForwardClear();
    }

    //
    // See if the sticky limit flag for the soft forward limit is in a "good"
    // state.
    //
    if(LimitStickySoftForwardOK())
    {
        ulValue |= LM_STATUS_LIMIT_STKY_SFWD;
    }
    else if(bClear)
    {
        //
        // Clear the sticky soft forward limit flag since it has now been
        // checked as "bad".
        //
        LimitStickySoftForwardClear();
    }

    //
    // See if the reverse limit is in a "good" state.
    //
    if(LimitReverseOK())
    {
        ulValue |= LM_STATUS_LIMIT_REV;
    }

    //
    // See if the reverse soft limit is in a "good" state.
    //
    if(LimitSoftReverseOK())
    {
        ulValue |= LM_STATUS_LIMIT_SREV;
    }

    //
    // See if the sticky reverse limit flag is in a "good" state.
    //
    if(LimitStickyReverseOK())
    {
        ulValue |= LM_STATUS_LIMIT_STKY_REV;
    }
    else if(bClear)
    {
        //
        // Clear the sticky reverse limit flag since it has now been checked as
        // "bad".
        //
        LimitStickyReverseClear();
    }

    //
    // See if the sticky limit flag for the soft reverse limit is in a "good"
    // state.
    //
    if(LimitStickySoftReverseOK())
    {
        ulValue |= LM_STATUS_LIMIT_STKY_SREV;
    }
    else if(bClear)
    {
        //
        // Clear the sticky soft reverse limit flag since it has now been
        // checked as "bad".
        //
        LimitStickySoftReverseClear();
    }

    //
    // Return the limit switch status.
    //
    return(ulValue);
}

//*****************************************************************************
//
// Handles the status commands.
//
//*****************************************************************************
static unsigned long
MessageStatusHandler(unsigned long ulID, unsigned char *pucData,
                     unsigned long ulMsgLen)
{
    unsigned char pucMessage[8], ucValue;
    unsigned long ulValue, ulAck;

    //
    // By default, no ACK should be supplied.
    //
    ulAck = 0;

    //
    // Mask out the device number and see what the command is.
    //
    switch(ulID & (~CAN_MSGID_DEVNO_M))
    {
        //
        // Read the output voltage in percent.
        //
        case LM_API_STATUS_VOLTOUT:
        {
            //
            // Get the output voltage.
            //
            ulValue = ControllerVoltageGet();

            //
            // Send a message back with the output voltage.
            //
            MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the input bus voltage.
        //
        case LM_API_STATUS_VOLTBUS:
        {
            //
            // Get the input bus voltage.
            //
            ulValue = ADCVBusGet();

            //
            // Send a message back with the input bus voltage.
            //
            MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the fault status.
        //
        case LM_API_STATUS_FAULT:
        {
            //
            // Get the fault status.
            //
            ulValue = ControllerFaultsActive();

            //
            // Send a message back with the fault status.
            //
            MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the motor current.
        //
        case LM_API_STATUS_CURRENT:
        {
            //
            // Get the motor current.
            //
            ulValue = ADCCurrentGet();

            //
            // Send a message back with the motor current.
            //
            MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the temperature.
        //
        case LM_API_STATUS_TEMP:
        {
            //
            // Get the temperature.
            //
            ulValue = ADCTemperatureGet();

            //
            // Send a message back with the temperature.
            //
            MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the motor position.
        //
        case LM_API_STATUS_POS:
        {
            //
            // Get the motor position.
            //
            ulValue = ControllerPositionGet();

            //
            // Send a message back with the motor position.
            //
            MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the motor speed.
        //
        case LM_API_STATUS_SPD:
        {
            //
            // Get the motor speed.
            //
            ulValue = ControllerSpeedGet();

            //
            // Send a message back with the motor speed.
            //
            MessageSendResponse(ulID, (unsigned char *)&ulValue, 4);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the state of the limit switches.
        //
        case LM_API_STATUS_LIMIT:
        {
            //
            // Read the limit switch status and clear the sticky status flags.
            //
            ulValue = LimitStatusGet(1);

            //
            // Send a message back with the limit switch status.
            //
            MessageSendResponse(ulID, (unsigned char *)&ulValue, 1);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the power status.
        //
        case LM_API_STATUS_POWER:
        {
            //
            // See if this is a get or set request.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send back the power state flags.
                //
                ulValue = ControllerPowerStatus();
                MessageSendResponse(ulID, (unsigned char *)&ulValue, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // See if the power status should be cleared.
                //
                if(pucData[0])
                {
                    ControllerPowerStatusClear();
                }
            }

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the current control mode.
        //
        case LM_API_STATUS_CMODE:
        {
            //
            // Get the current control mode.
            //
            ucValue = ControllerControlModeGet();

            //
            // Send a message back with the control mode.
            //
            MessageSendResponse(ulID, &ucValue, 1);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the output voltage in volts.
        //
        case LM_API_STATUS_VOUT:
        {
            //
            // Get the output voltage.
            //
            ulValue = (ControllerVoltageGet() * ADCVBusGet()) / 32768;

            //
            // Send a message back with the output voltage.
            //
            MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the status of the sticky fault flags.
        // This clears the sticky fault flags.
        //
        case LM_API_STATUS_STKY_FLT:
        {
            //
            // Get the sticky fault status subsequently clearing any sticky
            // faults.
            //
            ulValue = ControllerStickyFaultsActive(1);

            //
            // Send a message back with the fault status.
            //
            MessageSendResponse(ulID, (unsigned char *)&ulValue, 2);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Read the values of the Fault Counters and CAN status registers.
        //
        case LM_API_STATUS_FLT_COUNT:
        {
            //
            // Place the current fault counter in the 1st byte.
            //
            pucMessage[0] = ControllerCurrentFaultsGet();

            //
            // Place the temperature fault counter in the 2nd byte.
            //
            pucMessage[1] = ControllerTemperatureFaultsGet();

            //
            // Place the Vbus fault counter in the 3rd byte.
            //
            pucMessage[2] = ControllerVBusFaultsGet();

            //
            // Place the gate fault counter in the 4th byte.
            //
            pucMessage[3] = ControllerGateFaultsGet();

            //
            // Place the communication fault counter in the 5th byte.
            //
            pucMessage[4] = ControllerCommunicationFaultsGet();

            //
            // Place CANSTS in the 6th byte.
            //
            pucMessage[5] = CANStatusRegGet() & 0x000000ff;

            //
            // Place CANERR in the 7th and 8th bytes.
            //
            *(unsigned short *)(pucMessage + 6) =
                CANErrorRegGet() & 0x0000ffff;

            //
            // If the message received had a 1 byte payload, reset the
            // indicated counters.
            //
            if(ulMsgLen == 1)
            {
                //
                // Reset the fault counters
                //
                ControllerFaultCountReset(pucData[0]);

                //
                // Set the CANSTS register to 0x07.
                //
                if(pucData[0] & 0x20)
                {
                    CANStatusWriteLECNoEvent();
                }
            }

            //
            // Send a message back with the fault counters, CANSTS, and CANERR
            // registers.
            //
            MessageSendResponse(ulID, pucMessage, 8);

            //
            // Ack this command.
            //
            ulAck = 1;

            //
            // This message has been handled.
            //
            break;
        }

        //
        // An unknown command was received.
        //
        default:
        {
            //
            // This message has been handled.
            //
            break;
        }
    }

    //
    // Return the ACK indicator.
    //
    return(ulAck);
}

//*****************************************************************************
//
// Handles the configuration commands.
//
//*****************************************************************************
static unsigned long
MessageConfigurationHandler(unsigned long ulID, unsigned char *pucData,
                            unsigned long ulMsgLen)
{
    unsigned long pulValue[2], *pulData, ulAck;
    unsigned short *pusData;

    //
    // Create local pointers of different types to the message data to avoid
    // later type casting.
    //
    pulData = (unsigned long *)pucData;
    pusData = (unsigned short *)pucData;

    //
    // By default, no ACK should be supplied.
    //
    ulAck = 0;

    //
    // Mask out the device number and see what the command is.
    //
    switch(ulID & (~CAN_MSGID_DEVNO_M))
    {
        //
        // Set the number of brushes in the motor.
        //
        case LM_API_CFG_NUM_BRUSHES:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the number of brushes in response.
                //
                pulValue[0] = 0;
                MessageSendResponse(ulID, (unsigned char *)pulValue, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // Set the number of brushes.
                //
                CommandNumBrushesSet(pucData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the number of lines in the encoder.
        //
        case LM_API_CFG_ENC_LINES:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the number of encoder lines in response.
                //
                pulValue[0] = EncoderLinesGet();
                MessageSendResponse(ulID, (unsigned char *)pulValue, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // Set the number of encoder lines.
                //
                CommandEncoderLinesSet(pusData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the number of turns in the potentiometer.
        //
        case LM_API_CFG_POT_TURNS:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the number of potentiometer turns in response.
                //
                pulValue[0] = ADCPotTurnsGet();
                MessageSendResponse(ulID, (unsigned char *)pulValue, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // Set the number of potentiometer turns.
                //
                CommandPotTurnsSet(pusData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the braking mode to brake, coast, or jumper select.
        //
        case LM_API_CFG_BRAKE_COAST:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the brake/coast mode in response.
                //
                pulValue[0] = HBridgeBrakeCoastGet();;
                MessageSendResponse(ulID, (unsigned char *)pulValue, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // Set the brake/coast mode.
                //
                CommandBrakeCoastSet(pucData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the mode of the position limit switches.
        //
        case LM_API_CFG_LIMIT_MODE:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the position limit switch configuration in response.
                //
                pulValue[0] = LimitPositionActive();
                MessageSendResponse(ulID, (unsigned char *)pulValue, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // Configure the position limit switches.
                //
                CommandPositionLimitMode(pucData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the configuration of the forward position limit switch.
        //
        case LM_API_CFG_LIMIT_FWD:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the forward position limit switch configuration in
                // response.
                //
                LimitPositionForwardGet((long *)pulValue, pulValue + 1);
                MessageSendResponse(ulID, (unsigned char *)pulValue, 5);
            }
            else if(ulMsgLen == 5)
            {
                //
                // Set the forward position limit switch.
                //
                CommandPositionLimitForwardSet(pulData[0], pucData[4]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the configuration of the reverse position limit switch.
        //
        case LM_API_CFG_LIMIT_REV:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the reverse position limit switch configuration in
                // response.
                //
                LimitPositionReverseGet((long *)pulValue, pulValue + 1);
                MessageSendResponse(ulID, (unsigned char *)pulValue, 5);
            }
            else if(ulMsgLen == 5)
            {
                //
                // Set the reverse position limit switch.
                //
                CommandPositionLimitReverseSet(pulData[0], pucData[4]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the maximum output voltage.
        //
        case LM_API_CFG_MAX_VOUT:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the maximum output voltage in response.
                //
                pulValue[0] = HBridgeVoltageMaxGet();
                MessageSendResponse(ulID, (unsigned char *)pulValue, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // Set the maximum output voltage.
                //
                CommandMaxVoltageSet(pusData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the fault time.
        //
        case LM_API_CFG_FAULT_TIME:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the fault time in response.
                //
                pulValue[0] = ControllerFaultTimeGet();
                MessageSendResponse(ulID, (unsigned char *)pulValue, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // Set the fault time.
                //
                ControllerFaultTimeSet(pusData[0]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // An unknown command was received.
        //
        default:
        {
            //
            // This message has been handled.
            //
            break;
        }
    }

    //
    // Return the ACK indicator.
    //
    return(ulAck);
}

//*****************************************************************************
//
// Handles the Periodic Status API calls.
//
//*****************************************************************************
static unsigned long
MessagePStatusHandler(unsigned long ulID, unsigned char *pucData,
                            unsigned long ulMsgLen)
{
    unsigned long ulIdx, ulAck, *pulData;
    unsigned short *pusData;

    //
    // Create local pointers of different types to the message data to avoid
    // later type casting.
    //
    pulData = (unsigned long *)pucData;
    pusData = (unsigned short *)pucData;

    //
    // By default, no ACK should be supplied.
    //
    ulAck = 0;

    //
    // Mask out the device number and see what the command is.
    //
    switch(ulID & (~CAN_MSGID_DEVNO_M))
    {
        //
        // Set the period of periodic message S0.
        //
        case LM_API_PSTAT_PER_EN_S0:
        {
            //
            // See if no data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the period in response.
                //
                MessageSendResponse(ulID,
                                    (unsigned char *)(g_pusPStatPeriod + 0),
                                    2);
            }

            //
            // See if one data byte was supplied and it is zero.
            //
            else if((ulMsgLen == 1) && (pucData[0] == 0))
            {
                //
                // Disable the periodic message.
                //
                g_pusPStatPeriod[0] = 0;

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // See if two data bytes were supplied.
            //
            else if(ulMsgLen == 2)
            {
                //
                // Read the period and set it to the appropriate periodic
                // message.
                //
                g_pusPStatPeriod[0] = pusData[0];

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the period of periodic message S1.
        //
        case LM_API_PSTAT_PER_EN_S1:
        {
            //
            // See if no data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the period in response.
                //
                MessageSendResponse(ulID,
                                    (unsigned char *)(g_pusPStatPeriod + 1),
                                    2);
            }

            //
            // See if one data byte was supplied and it is zero.
            //
            else if((ulMsgLen == 1) && (pucData[0] == 0))
            {
                //
                // Disable the periodic message.
                //
                g_pusPStatPeriod[1] = 0;

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // See if two data bytes were supplied.
            //
            else if(ulMsgLen == 2)
            {
                //
                // Read the period and set it to the appropriate periodic
                // message.
                //
                g_pusPStatPeriod[1] = pusData[0];

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the period of periodic message S2.
        //
        case LM_API_PSTAT_PER_EN_S2:
        {
            //
            // See if no data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the period in response.
                //
                MessageSendResponse(ulID,
                                    (unsigned char *)(g_pusPStatPeriod + 2),
                                    2);
            }

            //
            // See if one data byte was supplied and it is zero.
            //
            else if((ulMsgLen == 1) && (pucData[0] == 0))
            {
                //
                // Disable the periodic message.
                //
                g_pusPStatPeriod[2] = 0;

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // See if two data bytes were supplied.
            //
            else if(ulMsgLen == 2)
            {
                //
                // Read the period and set it to the appropriate periodic
                // message.
                //
                g_pusPStatPeriod[2] = pusData[0];

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Set the period of periodic message S3.
        //
        case LM_API_PSTAT_PER_EN_S3:
        {
            //
            // See if no data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the period in response.
                //
                MessageSendResponse(ulID,
                                    (unsigned char *)(g_pusPStatPeriod + 3),
                                    2);
            }

            //
            // See if one data byte was supplied and it is zero.
            //
            else if((ulMsgLen == 1) && (pucData[0] == 0))
            {
                //
                // Disable the periodic message.
                //
                g_pusPStatPeriod[3] = 0;

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // See if two data bytes were supplied.
            //
            else if(ulMsgLen == 2)
            {
                //
                // Read the period and set it to the appropriate periodic
                // message.
                //
                g_pusPStatPeriod[3] = pusData[0];

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Configure the format for periodic message S0
        //
        case LM_API_PSTAT_CFG_S0:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the message format in response.
                //
                MessageSendResponse(ulID, g_ppucPStatFormat[0], 8);
            }
            else if(ulMsgLen == 8)
            {
                //
                // Set the format for the particular message.
                //
                for(ulIdx = 0; ulIdx < 8; ulIdx++)
                {
                    g_ppucPStatFormat[0][ulIdx] = pucData[ulIdx];
                }

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Configure the format for periodic message S1
        //
        case LM_API_PSTAT_CFG_S1:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the message format in response.
                //
                MessageSendResponse(ulID, g_ppucPStatFormat[1], 8);
            }
            else if(ulMsgLen == 8)
            {
                //
                // Set the format for the particular message.
                //
                for(ulIdx = 0; ulIdx < 8; ulIdx++)
                {
                    g_ppucPStatFormat[1][ulIdx] = pucData[ulIdx];
                }

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Configure the format for periodic message S2
        //
        case LM_API_PSTAT_CFG_S2:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the message format in response.
                //
                MessageSendResponse(ulID, g_ppucPStatFormat[2], 8);
            }
            else if(ulMsgLen == 8)
            {
                //
                // Set the format for the particular message.
                //
                for(ulIdx = 0; ulIdx < 8; ulIdx++)
                {
                    g_ppucPStatFormat[2][ulIdx] = pucData[ulIdx];
                }

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Configure the format for periodic message S3
        //
        case LM_API_PSTAT_CFG_S3:
        {
            //
            // See if any data was supplied.
            //
            if(ulMsgLen == 0)
            {
                //
                // Send the message format in response.
                //
                MessageSendResponse(ulID, g_ppucPStatFormat[3], 8);
            }
            else if(ulMsgLen == 8)
            {
                //
                // Set the format for the particular message.
                //
                for(ulIdx = 0; ulIdx < 8; ulIdx++)
                {
                    g_ppucPStatFormat[3][ulIdx] = pucData[ulIdx];
                }

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            //
            // This message has been handled.
            //
            break;
        }

        //
        // An unknown command was received.
        //
        default:
        {
            //
            // This message has been handled.
            //
            break;
        }
    }

    //
    // Return the ACK indicator.
    //
    return(ulAck);
}

//*****************************************************************************
//
// Handles general commands.
//
//*****************************************************************************
unsigned long
MessageCommandHandler(unsigned long ulID, unsigned char *pucData,
                      unsigned long ulMsgLen)
{
    unsigned long ulAck;

    //
    // By default, not response is supplied.
    //
    g_ulResponseLength = 0;

    //
    // By default, no ACK should be supplied.
    //
    ulAck = 0;

    //
    // Determine which message has been received.
    //
    switch(ulID & (CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M |
                   CAN_MSGID_API_CLASS_M))
    {
        //
        // The system commands.
        //
        case 0:
        {
            //
            // Call the system command handler.
            //
            MessageSystemHandler(ulID, pucData, ulMsgLen);

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Voltage motor control commands.
        //
        case LM_API_VOLT:
        {
            //
            // Call the voltage handler.
            //
            ulAck = MessageVoltageHandler(ulID, pucData, ulMsgLen);

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Voltage compensation motor control commands.
        //
        case LM_API_VCOMP:
        {
            //
            // Call the voltage compensation handler.
            //
            ulAck = MessageVoltageCompHandler(ulID, pucData, ulMsgLen);

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Current motor control commands.
        //
        case LM_API_ICTRL:
        {
            //
            // Call the current handler.
            //
            ulAck = MessageCurrentHandler(ulID, pucData, ulMsgLen);

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Speed motor control commands.
        //
        case LM_API_SPD:
        {
            //
            // Call the speed handler.
            //
            ulAck = MessageSpeedHandler(ulID, pucData, ulMsgLen);

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Position motor control commands.
        //
        case LM_API_POS:
        {
            //
            // Call the position handler.
            //
            ulAck = MessagePositionHandler(ulID, pucData, ulMsgLen);

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Status motor control commands.
        //
        case LM_API_STATUS:
        {
            //
            // Call the status handler.
            //
            ulAck = MessageStatusHandler(ulID, pucData, ulMsgLen);

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Configuration motor control commands.
        //
        case LM_API_CFG:
        {
            //
            // Call the configuration handler.
            //
            ulAck = MessageConfigurationHandler(ulID, pucData, ulMsgLen);

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Update commands.
        //
        case LM_API_UPD:
        {
            //
            // Call the update command message handler.
            //
            ulAck = MessageUpdateHandler(ulID, pucData, ulMsgLen);

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Periodic status commands.
        //
        case LM_API_PSTAT:
        {
            //
            // Call the periodic status message handler.
            //
            ulAck = MessagePStatusHandler(ulID, pucData, ulMsgLen);

            //
            // This message has been handled.
            //
            break;
        }

        //
        // An unknown command was received.
        //
        default:
        {
            //
            // This message has been handled.
            //
            break;
        }
    }

    //
    // Return the ACK indicator.
    //
    return(ulAck);
}

//*****************************************************************************
//
// Handles a periodic tick in order to process timed message events (device
// assignment and enumeration).
//
//*****************************************************************************
void
MessageTick(void)
{
    unsigned short usVout, usVbus, usImotor, usTamb, usCANErr;
    unsigned long ulMsg, ulIdx, ulPos, ulSpeed, ulFlags;

    //
    // See if there is an active assignment in progress.
    //
    if(g_ulMessageState == STATE_ASSIGN)
    {
        //
        // See if the assignment state has timed out.
        //
        if(--g_ulTickCount == 0)
        {
            //
            // Move to the idle state.
            //
            g_ulMessageState = STATE_IDLE;

            //
            // If the pending change was not accepted and was the the same as
            // it was before then set the device number to 0 and accept it.
            //
            if(g_ucDevNumPending == g_sParameters.ucDeviceNumber)
            {
                //
                // Reset the CAN device number.
                //
                CANIFSetID(0);

                //
                // Reset the device number.
                //
                g_sParameters.ucDeviceNumber = 0;

                //
                // Save the new device number.
                //
                ParamSave();
            }

            //
            // Indicate that the CAN controller has left assignment mode.
            //
            LEDAssignStop();
        }
    }

    //
    // See if there is an enumeration in progress.
    //
    if(g_ulMessageState == STATE_ENUM)
    {
        //
        // See if the enumeration delay has timed out.
        //
        if(--g_ulTickCount == 0)
        {
            //
            // Move to the idle state.
            //
            g_ulMessageState = STATE_IDLE;

            //
            // Determine the control link that is in use.
            //
            switch(g_ulEnumInterface)
            {
                //
                // CAN is the active control link.
                //
                case LINK_TYPE_CAN:
                {
                    //
                    // Send out an enumeration response on the CAN bus.
                    //
                    CANIFEnumerate();

                    //
                    // Done handling this link.
                    //
                    break;
                }

                //
                // UART is the active control link.
                //
                case LINK_TYPE_UART:
                {
                    //
                    // Send out an enumeration response on the UART.
                    //
                    UARTIFEnumerate();

                    //
                    // Done handling this link.
                    //
                    break;
                }
            }
        }
    }

    //
    // Set the message flags to 0 to indicate that no periodic status messages
    // need to be sent.
    //
    ulFlags = 0;

    //
    // Set the multi-byte data items to 0.  When the first periodic status
    // message is found (if any) that needs to be sent, the multi-byte data
    // items will be fetched into these variables.
    //
    usVout = 0;
    usVbus = 0;
    usImotor = 0;
    usTamb = 0;
    ulPos = 0;
    ulSpeed = 0;
    usCANErr = 0;

    //
    // Loop through the periodic status messages.
    //
    for(ulMsg = 0; ulMsg < 4; ulMsg++)
    {
        //
        // Skip this message if it is disabled.
        //
        if(g_pusPStatPeriod[ulMsg] == 0)
        {
            g_pusPStatCounter[ulMsg] = 0;
            continue;
        }

        //
        // Increment the counter for this message and skip to the next message
        // if its counter has not expired.
        //
        g_pusPStatCounter[ulMsg]++;
        if(g_pusPStatCounter[ulMsg] < g_pusPStatPeriod[ulMsg])
        {
            continue;
        }

        //
        // Reset the counter for this message.
        //
        g_pusPStatCounter[ulMsg] = 0;

        //
        // Fetch the multi-byte data items if they have not been previously
        // fetched.
        //
        if(ulFlags == 0)
        {
            usVout = ControllerVoltageGet();
            usVbus = ADCVBusGet();
            usImotor = ADCCurrentGet();
            usTamb = ADCTemperatureGet();
            ulPos = ControllerPositionGet();
            ulSpeed = ControllerSpeedGet();
            usCANErr = CANErrorRegGet();
        }

        //
        // Set a flag indicating that this periodic message needs to be sent.
        //
        ulFlags |= 1 << ulMsg;

        //
        // Loop through the bytes of this periodic message, building the data
        // packet.
        //
        for(ulIdx = 0; ulIdx < 8; ulIdx++)
        {
            //
            // Break out of the loop if this entry contains the end marker.
            //
            if(g_ppucPStatFormat[ulMsg][ulIdx] == LM_PSTAT_END)
            {
                break;
            }

            //
            // Determine the data to be inserted into this byte.
            //
            switch(g_ppucPStatFormat[ulMsg][ulIdx])
            {
                //
                // The LSB of the Vout percentage.
                //
                case LM_PSTAT_VOLTOUT_B0:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = usVout & 0xff;
                    break;
                }

                //
                // The MSB of the Vout percentage.
                //
                case LM_PSTAT_VOLTOUT_B1:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = (usVout >> 8) & 0xff;
                    break;
                }

                //
                // The LSB of the Vbus value.
                //
                case LM_PSTAT_VOLTBUS_B0:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = usVbus & 0xff;
                    break;
                }

                //
                // The MSB of the Vbus value.
                //
                case LM_PSTAT_VOLTBUS_B1:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = (usVbus >> 8) & 0xff;
                    break;
                }

                //
                // The LSB of the Imotor value.
                //
                case LM_PSTAT_CURRENT_B0:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = usImotor & 0xff;
                    break;
                }

                //
                // The MSB of the Imotor value.
                //
                case LM_PSTAT_CURRENT_B1:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = (usImotor >> 8) & 0xff;
                    break;
                }

                //
                // The LSB of the Tambient value.
                //
                case LM_PSTAT_TEMP_B0:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = usTamb & 0xff;
                    break;
                }

                //
                // The MSB of the Tambient value.
                //
                case LM_PSTAT_TEMP_B1:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = (usTamb >> 8) & 0xff;
                    break;
                }

                //
                // The LSB of the motor position value.
                //
                case LM_PSTAT_POS_B0:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = ulPos & 0xff;
                    break;
                }

                //
                // The second significant byte of the motor position value.
                //
                case LM_PSTAT_POS_B1:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = (ulPos >> 8) & 0xff;
                    break;
                }

                //
                // The third significant byte of the motor position value.
                //
                case LM_PSTAT_POS_B2:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = (ulPos >> 16) & 0xff;
                    break;
                }

                //
                // The MSB of the motor position value.
                //
                case LM_PSTAT_POS_B3:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = (ulPos >> 24) & 0xff;
                    break;
                }

                //
                // The LSB of the motor speed value.
                //
                case LM_PSTAT_SPD_B0:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = ulSpeed & 0xff;
                    break;
                }

                //
                // The second significant byte of the motor speed value.
                //
                case LM_PSTAT_SPD_B1:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = (ulSpeed >> 8) & 0xff;
                    break;
                }

                //
                // The third significant byte of the motor speed value.
                //
                case LM_PSTAT_SPD_B2:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = (ulSpeed >> 16) & 0xff;
                    break;
                }

                //
                // The MSB of the motor speed value.
                //
                case LM_PSTAT_SPD_B3:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = (ulSpeed >> 24) & 0xff;
                    break;
                }

                //
                // The limit switch values, read without clearing the sticky
                // limit switch status.
                //
                case LM_PSTAT_LIMIT_NCLR:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = LimitStatusGet(0);
                    break;
                }

                //
                // The limit switch values, causing the sticky limit switch
                // status to be cleared.
                //
                case LM_PSTAT_LIMIT_CLR:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = LimitStatusGet(1);
                    break;
                }

                //
                // The fault conditions.
                //
                case LM_PSTAT_FAULT:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] =
                        ControllerFaultsActive();
                    break;
                }

                //
                // The sticky fault conditions, without clearing the sticky
                // faults.
                //
                case LM_PSTAT_STKY_FLT_NCLR:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] =
                        ControllerStickyFaultsActive(0);
                    break;
                }

                //
                // The sticky fault conditions, causing the sticky faults to be
                // cleared.
                //
                case LM_PSTAT_STKY_FLT_CLR:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] =
                        ControllerStickyFaultsActive(1);
                    break;
                }

                //
                // The LSB of the Vout value.
                //
                case LM_PSTAT_VOUT_B0:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] =
                        ((usVout * usVbus) / 32768) & 0xff;
                    break;
                }

                //
                // The MSB of the Vout value.
                //
                case LM_PSTAT_VOUT_B1:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] =
                        (((usVout * usVbus) / 32768) >> 8) & 0xff;
                    break;
                }

                //
                // The count of current faults.
                //
                case LM_PSTAT_FLT_COUNT_CURRENT:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] =
                        ControllerCurrentFaultsGet();
                    break;
                }

                //
                // The count of temperature faults.
                //
                case LM_PSTAT_FLT_COUNT_TEMP:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] =
                        ControllerTemperatureFaultsGet();
                    break;
                }

                //
                // The count of Vbus faults.
                //
                case LM_PSTAT_FLT_COUNT_VOLTBUS:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] =
                        ControllerVBusFaultsGet();
                    break;
                }

                //
                // The count of gate driver faults.
                //
                case LM_PSTAT_FLT_COUNT_GATE:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] =
                        ControllerGateFaultsGet();
                    break;
                }

                //
                // The count of communication faults.
                //
                case LM_PSTAT_FLT_COUNT_COMM:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] =
                        ControllerCommunicationFaultsGet();
                    break;
                }

                //
                // The value of the CANSTS register.
                //
                case LM_PSTAT_CANSTS:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = CANStatusRegGet();
                    break;
                }

                //
                // The LSB of the CANERR register.
                //
                case LM_PSTAT_CANERR_B0:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = usCANErr & 0xff;
                    break;
                }

                //
                // The MSB of the CANERR register.
                //
                case LM_PSTAT_CANERR_B1:
                {
                    g_ppucPStatMessages[ulMsg][ulIdx] = (usCANErr >> 8) & 0xff;
                    break;
                }
            }
        }

        //
        // Save the length of this periodic status message.
        //
        g_pucPStatMessageLen[ulMsg] = ulIdx;
    }

    //
    // Send the periodic status messages, if any.
    //
    if(ulFlags != 0)
    {
        //
        // Save the set of periodic status messages that need to be sent.
        //
        g_ulPStatFlags = ulFlags;

        //
        // Trigger a communication interrupt on the appropriate communication
        // interface.
        //
        switch(ControllerLinkType())
        {
            //
            // The current active interface is CAN.
            //
            case LINK_TYPE_CAN:
            {
                //
                // Indicate to the CAN interface that there are messages to
                // send
                //
                CANIFPStatus();
            }

            //
            // The current active interface is UART.
            //
            case LINK_TYPE_UART:
            {
                //
                // Indicate to the UART interface that there are messages to
                // send.
                //
                UARTIFPStatus();
            }
        }
    }
}

//*****************************************************************************
//
// Handles presses of the button if not using the servo connection.
//
//*****************************************************************************
void
MessageButtonPress(void)
{
    //
    // If the device was in the assignment state then save the new value.
    //
    if(g_ulMessageState == STATE_ASSIGN)
    {
        //
        // Set the CAN device number.
        //
        CANIFSetID(g_ucDevNumPending);

        //
        // See if the device number has changed.
        //
        if(g_sParameters.ucDeviceNumber != g_ucDevNumPending)
        {
            //
            // Save the new device number.
            //
            g_sParameters.ucDeviceNumber = g_ucDevNumPending;

            //
            // Save the new device number.
            //
            ParamSave();
        }

        //
        // Move to the idle state.
        //
        g_ulMessageState = STATE_IDLE;
    }

    //
    // Blink the device ID.
    //
    LEDBlinkID(g_sParameters.ucDeviceNumber);
}
