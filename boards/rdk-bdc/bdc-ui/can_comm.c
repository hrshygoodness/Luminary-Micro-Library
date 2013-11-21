//*****************************************************************************
//
// can_comm.c - Functions for communicating over the CAN network.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "shared/can_proto.h"
#include "can_comm.h"

//*****************************************************************************
//
// This array describes the message objects used to receive status information
// from the CAN bus.
//
//*****************************************************************************
static const unsigned g_ppulMsgIDs[][2] =
{
    {
        CAN_MSGID_API_ENUMERATE,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M | 3
    },
    {
        CAN_MSGID_API_ENUMERATE | 1,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M | 3
    },
    {
        CAN_MSGID_API_ENUMERATE | 2,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M | 3
    },
    {
        CAN_MSGID_API_ENUMERATE | 3,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M | 3
    },
    {
        LM_API_STATUS_VOLTOUT,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M
    },
    {
        LM_API_STATUS_VOLTBUS,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M
    },
    {
        LM_API_STATUS_CURRENT,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M
    },
    {
        LM_API_STATUS_TEMP,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M
    },
    {
        LM_API_STATUS_POS,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M
    },
    {
        LM_API_STATUS_SPD,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M
    },
    {
        LM_API_STATUS_LIMIT,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M
    },
    {
        LM_API_STATUS_FAULT,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M
    },
    {
        LM_API_STATUS_POWER,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M
    },
    {
        LM_API_ACK,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M
    },
    {
        LM_API_UPD_ACK,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_API_M
    },
    {
        CAN_MSGID_DTYPE_MOTOR | CAN_MSGID_MFR_LM,
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M
    }
};

//*****************************************************************************
//
// The number of status message objects.
//
//*****************************************************************************
#define NUM_STATUS_OBJS         (sizeof(g_ppulMsgIDs) /                       \
                                 sizeof(g_ppulMsgIDs[0]))

//*****************************************************************************
//
// The number of the message object used to send commands.
//
//*****************************************************************************
#define COMMAND_MSG_OBJ         (NUM_STATUS_OBJS + 1)

//*****************************************************************************
//
// The number of the message object used to send the firmware version remote
// request.
//
//*****************************************************************************
#define FIRMWARE_VER_MSG_OBJ    (COMMAND_MSG_OBJ + 1)

//*****************************************************************************
//
// The number of the message object used to send the heartbeat messages.
//
//*****************************************************************************
#define HEARTBEAT_MSG_OBJ       (FIRMWARE_VER_MSG_OBJ + 1)

//*****************************************************************************
//
// This array contains the results of the most recent CAN bus enueration.  The
// first word contains a set of bits that indicate the presence or absence of
// device IDs 1 through 31 (in bit positions 1 through 31 respectively), and
// the second word contains device IDs 32 through 63 (in bit positions 0
// through 31 respectively).
//
//*****************************************************************************
unsigned long g_pulStatusEnumeration[2];

//*****************************************************************************
//
// The status flags, which indicate the validity of the various status items.
//
//*****************************************************************************
unsigned long g_ulStatusFlags;

//*****************************************************************************
//
// The most recently received output voltage.
//
//*****************************************************************************
long g_lStatusVout;

//*****************************************************************************
//
// The most recently received bus voltage.
//
//*****************************************************************************
unsigned long g_ulStatusVbus;

//*****************************************************************************
//
// The most recently received motor current.
//
//*****************************************************************************
long g_lStatusCurrent;

//*****************************************************************************
//
// The most recently received ambient temperature.
//
//*****************************************************************************
unsigned long g_ulStatusTemperature;

//*****************************************************************************
//
// The most recently received position.
//
//*****************************************************************************
long g_lStatusPosition;

//*****************************************************************************
//
// The most recently received speed.
//
//*****************************************************************************
long g_lStatusSpeed;

//*****************************************************************************
//
// The most recently received limit switch values.
//
//*****************************************************************************
unsigned long g_ulStatusLimit;

//*****************************************************************************
//
// The most recently received fault status.
//
//*****************************************************************************
unsigned long g_ulStatusFault;

//*****************************************************************************
//
// The most recently received firmware version.
//
//*****************************************************************************
unsigned long g_ulStatusFirmwareVersion = 0xffffffff;

//*****************************************************************************
//
// A flag that is set when an ACK is received from the boot loader.
//
//*****************************************************************************
unsigned long g_ulUpdateAck = 0;

//*****************************************************************************
//
// The current device ID.
//
//*****************************************************************************
unsigned long g_ulCurrentID = 1;

//*****************************************************************************
//
// A flag that is true if it is possible to read from the current device (in
// other words, it is running firmware newer than 3330).
//
//*****************************************************************************
static unsigned long g_ulCanRead = 0;

//*****************************************************************************
//
// The size of the message queue used to send commands out over the CAN bus.
//
//*****************************************************************************
#define QUEUE_SIZE              32

//*****************************************************************************
//
// An array to contain the data portion of the CAN message queue.
//
//*****************************************************************************
static unsigned char g_pucMsgData[QUEUE_SIZE][8];

//*****************************************************************************
//
// A queue of messages to be send out over the CAN bus.
//
//*****************************************************************************
static tCANMsgObject g_psCANQueue[QUEUE_SIZE];

//*****************************************************************************
//
// The offset of the next packet to be read from the CAN message queue.  The
// queue is empty if this is equal to g_ulCANQueueWrite.
//
//*****************************************************************************
static unsigned long g_ulCANQueueRead = 0;

//*****************************************************************************
//
// The offset of the location in the CAN message queue where a new packet is to
// be written.  The queue is full if this is equal to g_ulCANQueueRead - 1
// (modulo the queue size).
//
//*****************************************************************************
static unsigned long g_ulCANQueueWrite = 0;

//*****************************************************************************
//
// The number of times per second that a heartbeat is generated and the status
// of the motor controller is queried.
//
//*****************************************************************************
#define UPDATES_PER_SECOND      25

//*****************************************************************************
//
// The state of the CAN communication state machine.
//
//*****************************************************************************
#define CAN_STATE_IDLE          0
#define CAN_STATE_WAIT_FOR_SEND 1
#define CAN_STATE_WAIT_FOR_ACK  2
static unsigned long g_ulCANState = CAN_STATE_IDLE;

//*****************************************************************************
//
// The number of milliseconds to wait for a message to be sent.
//
//*****************************************************************************
#define CAN_COUNT_WAIT_FOR_SEND 2

//*****************************************************************************
//
// The number of milliseconds to wait for an ACK packet from the motor
// controller.
//
//*****************************************************************************
#define CAN_COUNT_WAIT_FOR_ACK  5

//*****************************************************************************
//
// The number of milliseconds to wait for an ACK packet from the boot loader.
//
//*****************************************************************************
#define CAN_COUNT_WAIT_FOR_UPD  3000

//*****************************************************************************
//
// This is non-zero when a heartbeat message should be sent out.
//
//*****************************************************************************
static unsigned long g_ulCANHeartbeat;

//*****************************************************************************
//
// The counter used to timeout messages that are sent.
//
//*****************************************************************************
static unsigned long g_ulCANCount;

//*****************************************************************************
//
// This structure holds the static definitions used to configure the CAN
// controller bit clock in the proper state given a requested setting.
//
// These are for a 8MHz clock running at 1Mbit CAN rate.
//
//*****************************************************************************
static const tCANBitClkParms CANBitClkSettings =
{
    5, 2, 2, 1
};

//*****************************************************************************
//
// This is non-zero when a fake CAN interrupt has been generated as a result of
// a timer tick.
//
//*****************************************************************************
static unsigned long g_ulCANTick;

//*****************************************************************************
//
// The counter used to count the timer ticks between heartbeats and status
// queries.
//
//*****************************************************************************
static unsigned long g_ulCANTickCount;

//*****************************************************************************
//
// The counter used to delay all CAN traffic during an enumeration.
//
//*****************************************************************************
static unsigned long g_ulCANDelayCount = 0;

//*****************************************************************************
//
// This is non-zero when an ACK has been received from the boot loader.
//
//*****************************************************************************
volatile unsigned long g_ulCANUpdateAck = 0;

//*****************************************************************************
//
// This function initializes the CAN interface.
//
//*****************************************************************************
void
CANCommInit(void)
{
    tCANMsgObject sMsgObject;
    unsigned long ulIdx;

    //
    // Enable the CAN peripheral and the GPIO port used for the CAN pins.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    //
    // Configure the CAN pins.
    //
    GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the CAN controller.
    //
    CANInit(CAN0_BASE);

    //
    // Configure the bit rate for the CAN device, the clock rate to the CAN
    // controller is fixed at 8MHz for this class of device and the bit rate is
    // set to 1000000.
    //
    CANBitTimingSet(CAN0_BASE, (tCANBitClkParms *)&CANBitClkSettings);

    //
    // Enable the CAN controller.
    //
    CANEnable(CAN0_BASE);

    //
    // Enable interrups from CAN controller.
    //
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER);

    //
    // Configure the message objects used to receive status messages.
    //
    sMsgObject.ulFlags = (MSG_OBJ_EXTENDED_ID | MSG_OBJ_RX_INT_ENABLE |
                          MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);
    sMsgObject.ulMsgLen = 0;
    sMsgObject.pucMsgData = 0;
    for(ulIdx = 0; ulIdx < NUM_STATUS_OBJS; ulIdx++)
    {
        sMsgObject.ulMsgID = g_ppulMsgIDs[ulIdx][0];
        sMsgObject.ulMsgIDMask = g_ppulMsgIDs[ulIdx][1];
        CANMessageSet(CAN0_BASE, ulIdx + 1, &sMsgObject, MSG_OBJ_TYPE_RX);
    }

    //
    // Set the data pointers in the message objects in the CAN message queue.
    //
    for(ulIdx = 0; ulIdx < QUEUE_SIZE; ulIdx++)
    {
        g_psCANQueue[ulIdx].pucMsgData = g_pucMsgData[ulIdx];
    }

    //
    // Clear the message object used to transmit commands.
    //
    CANMessageClear(CAN0_BASE, COMMAND_MSG_OBJ);

    //
    // Enable auto-retry on CAN transmissions.
    //
    CANRetrySet(CAN0_BASE, true);

    //
    // Enable interrupts from the CAN controller.
    //
    IntEnable(INT_CAN0);

    //
    // Initialize the CAN tick counter.
    //
    g_ulCANTickCount = 1000 / UPDATES_PER_SECOND;
}

//*****************************************************************************
//
// This function sends a message on the CAN bus by placing it into the CAN
// message queue for handling at the appropriate time.
//
//*****************************************************************************
unsigned long
CANSendMessage(unsigned long ulMsgID, unsigned long ulDataLen,
               unsigned long ulData1, unsigned long ulData2)
{
    //
    // Disable all interrupts while updating the message queue.
    //
    IntMasterDisable();

    //
    // Return an error if there is no space in the message queue.
    //
    if(((g_ulCANQueueWrite + 1) % QUEUE_SIZE) == g_ulCANQueueRead)
    {
        IntMasterEnable();
        return(0);
    }

    //
    // Write this message into the next entry of the message queue.
    //
    g_psCANQueue[g_ulCANQueueWrite].ulMsgID = ulMsgID;
    g_psCANQueue[g_ulCANQueueWrite].ulFlags =
        MSG_OBJ_EXTENDED_ID | MSG_OBJ_TX_INT_ENABLE;
    g_psCANQueue[g_ulCANQueueWrite].ulMsgLen = ulDataLen;
    ((unsigned long *)(g_pucMsgData[g_ulCANQueueWrite]))[0] = ulData1;
    ((unsigned long *)(g_pucMsgData[g_ulCANQueueWrite]))[1] = ulData2;

    //
    // Update the write pointer.
    //
    g_ulCANQueueWrite = (g_ulCANQueueWrite + 1) % QUEUE_SIZE;

    //
    // Re-enable interrupts.
    //
    IntMasterEnable();

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
// This function reads the value of a parameter across the CAN bus.
//
//*****************************************************************************
unsigned long
CANReadParameter(unsigned long ulID, unsigned long *pulDataLen,
                 unsigned long *pulParam1, unsigned long *pulParam2)
{
    static volatile unsigned long pulData[3];

    //
    // If the device does not support reads then return a failure.  All
    // firmware version support reading the power status, so allow that one
    // unconditionally.
    //
    if((g_ulCanRead == 0) && (ulID != LM_API_STATUS_POWER))
    {
        return(0);
    }

    //
    // Set the first word of the data buffer to -1 to indicate that the
    // response has not been received yet.
    //
    pulData[0] = 0xffffffff;

    //
    // Send a message to read the requested parameter.
    //
    if(CANSendMessage(ulID | g_ulCurrentID, 0, (unsigned long)pulData, 0) == 0)
    {
        return(0);
    }

    //
    // Wait until the response has been received or the timeout has occurred.
    //
    while(pulData[0] == 0xffffffff)
    {
    }

    //
    // Return an error if the parameter read timed out.
    //
    if(pulData[0] == 0xfffffffe)
    {
        return(0);
    }

    //
    // Return the length of the data, if requested.
    //
    if(pulDataLen)
    {
        *pulDataLen = pulData[0];
    }

    //
    // Return the first word of the data, if requested.
    //
    if(pulParam1)
    {
        *pulParam1 = pulData[1];
    }

    //
    // Return the second word of the data, if requested.
    //
    if(pulParam2)
    {
        *pulParam2 = pulData[2];
    }

    //
    // Success.
    //
    return(1);
}

//*****************************************************************************
//
// This function sets the ID of the device to which commands are sent.
//
//*****************************************************************************
void
CANSetID(unsigned long ulID)
{
    unsigned long ulPower;

    //
    // Save the new device ID.
    //
    g_ulCurrentID = ulID & CAN_MSGID_DEVNO_M;

    //
    // Until determined otherwise, assume that it is not possible to read from
    // this device.
    //
    g_ulCanRead = 0;

    //
    // All of the status items are not valid.
    //
    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VOUT) = 0;
    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VBUS) = 0;
    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_CURRENT) = 0;
    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_TEMP) = 0;
    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_POS) = 0;
    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_SPEED) = 0;
    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_LIMIT) = 0;
    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_FAULT) = 0;

    //
    // Request the firmware version of the current motor controller.
    //
    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_FIRMVER) = 0;
    g_ulStatusFirmwareVersion = 0xffffffff;
    CANFirmwareVersion();

    //
    // Read the power on reset flag from the new controller.
    //
    if(CANReadParameter(LM_API_STATUS_POWER, 0, &ulPower, 0) == 0)
    {
        return;
    }

    //
    // Check the firmware version.  If it is newer than 3330, then enable the
    // ability to read.
    //
    if(g_ulStatusFirmwareVersion > 3330)
    {
        g_ulCanRead = 1;
    }

    //
    // See if this controller is fresh out of reset.
    //
    if(ulPower)
    {
        //
        // Clear the power on reset flag.
        //
        CANStatusPowerClear();

        //
        // Set the default voltage ramp to 0.3 V/ms, and select the encoder as
        // the default speed and position reference.
        //
        CANVoltageRampSet((30 * 32767) / 1200);
        CANSpeedRefSet(0);
        CANPositionRefSet(0);

        //
        // Read the power on reset flag again to make sure that the above
        // changes have been applied before returning.
        //
        CANReadParameter(LM_API_STATUS_POWER, 0, &ulPower, 0);
    }
}

//*****************************************************************************
//
// This function is called when the CAN controller generates an interrupt.
//
//*****************************************************************************
void
CAN0IntHandler(void)
{
    unsigned long ulStatus, *pulMsgData;
    unsigned char pucData[8];
    tCANMsgObject sMsgObject;

    //
    // Provide a data buffer to the local message object.
    //
    sMsgObject.pucMsgData = pucData;

    //
    // Loop while there are still messages objects generating an interrupt.
    //
    while((ulStatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE)) != 0)
    {
        //
        // Read this message object and clear its interrupt.
        //
        CANMessageGet(CAN0_BASE, ulStatus, &sMsgObject, true);

        //
        // If this is the command message object, then set the MSB of the
        // message ID to indicate that this is a transmit success interrupt and
        // not the potential response message.
        //
        if(ulStatus == COMMAND_MSG_OBJ)
        {
            sMsgObject.ulMsgID |= 0x80000000;
        }

        //
        // Deteremine how to handle this based on the message ID.
        //
        switch(sMsgObject.ulMsgID & ~(CAN_MSGID_DEVNO_M))
        {
            //
            // This is a response to an enumeration request.
            //
            case CAN_MSGID_API_ENUMERATE:
            {
                //
                // Extract the device number from the message ID.
                //
                ulStatus = sMsgObject.ulMsgID & CAN_MSGID_DEVNO_M;

                //
                // Set the corresponding bit in the enumeration array.
                //
                g_pulStatusEnumeration[ulStatus / 32] |= 1 << (ulStatus & 31);

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is a response to a output voltage status request.
            //
            case LM_API_STATUS_VOLTOUT:
            {
                //
                // Only accept status responses from the current device ID.
                //
                if((sMsgObject.ulMsgID & CAN_MSGID_DEVNO_M) == g_ulCurrentID)
                {
                    g_lStatusVout = *(short *)pucData;
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VOUT) = 1;
                }
                else
                {
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VOUT) = 0;
                }

                //
                // Now, query the bus voltage.
                //
                CANSendMessage(LM_API_STATUS_VOLTBUS | g_ulCurrentID, 0, 0, 0);

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is a response to a bus voltage status request.
            //
            case LM_API_STATUS_VOLTBUS:
            {
                //
                // Only accept status responses from the current device ID.
                //
                if((sMsgObject.ulMsgID & CAN_MSGID_DEVNO_M) == g_ulCurrentID)
                {
                    g_ulStatusVbus = *(unsigned short *)pucData;
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VBUS) = 1;
                }
                else
                {
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VBUS) = 0;
                }

                //
                // Now, query the motor current.
                //
                CANSendMessage(LM_API_STATUS_CURRENT | g_ulCurrentID, 0, 0, 0);

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is a response to a motor current status request.
            //
            case LM_API_STATUS_CURRENT:
            {
                //
                // Only accept status responses from the current device ID.
                //
                if((sMsgObject.ulMsgID & CAN_MSGID_DEVNO_M) == g_ulCurrentID)
                {
                    g_lStatusCurrent = *(unsigned short *)pucData;
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_CURRENT) = 1;
                }
                else
                {
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_CURRENT) = 0;
                }

                //
                // Now, query the ambient temperature.
                //
                CANSendMessage(LM_API_STATUS_TEMP | g_ulCurrentID, 0, 0, 0);

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is a response to an ambient temperature status request.
            //
            case LM_API_STATUS_TEMP:
            {
                //
                // Only accept status responses from the current device ID.
                //
                if((sMsgObject.ulMsgID & CAN_MSGID_DEVNO_M) == g_ulCurrentID)
                {
                    g_ulStatusTemperature = *(unsigned short *)pucData;
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_TEMP) = 1;
                }
                else
                {
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_TEMP) = 0;
                }

                //
                // Now, query the position.
                //
                CANSendMessage(LM_API_STATUS_POS | g_ulCurrentID, 0, 0, 0);

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is a response to a position status request.
            //
            case LM_API_STATUS_POS:
            {
                //
                // Only accept status responses from the current device ID.
                //
                if((sMsgObject.ulMsgID & CAN_MSGID_DEVNO_M) == g_ulCurrentID)
                {
                    g_lStatusPosition = *(long *)pucData;
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_POS) = 1;
                }
                else
                {
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_POS) = 0;
                }

                //
                // See if this was an explicit position read request.
                //
                pulMsgData = (unsigned long *)
                    (*(unsigned long *)(g_psCANQueue[g_ulCANQueueRead].
                                        pucMsgData));
                if(pulMsgData != 0)
                {
                    //
                    // Save the contents of the message.
                    //
                    pulMsgData[1] = *(unsigned long *)pucData;
                    pulMsgData[2] = *(unsigned long *)(pucData + 4);
                    pulMsgData[0] = sMsgObject.ulMsgLen;
                }

                //
                // Now, query the speed.
                //
                CANSendMessage(LM_API_STATUS_SPD | g_ulCurrentID, 0, 0, 0);

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is a response to a speed status request.
            //
            case LM_API_STATUS_SPD:
            {
                //
                // Only accept status responses from the current device ID.
                //
                if((sMsgObject.ulMsgID & CAN_MSGID_DEVNO_M) == g_ulCurrentID)
                {
                    g_lStatusSpeed = *(unsigned long *)pucData;
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_SPEED) = 1;
                }
                else
                {
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_SPEED) = 0;
                }

                //
                // Now, query the limit switches.
                //
                CANSendMessage(LM_API_STATUS_LIMIT | g_ulCurrentID, 0, 0, 0);

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is a response to a limit switch status request.
            //
            case LM_API_STATUS_LIMIT:
            {
                //
                // Only accept status responses from the current device ID.
                //
                if((sMsgObject.ulMsgID & CAN_MSGID_DEVNO_M) == g_ulCurrentID)
                {
                    g_ulStatusLimit = pucData[0];
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_LIMIT) = 1;
                }
                else
                {
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_LIMIT) = 0;
                }

                //
                // Now, query the fault status.
                //
                CANSendMessage(LM_API_STATUS_FAULT | g_ulCurrentID, 0, 0, 0);

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is a response to a fault status request.
            //
            case LM_API_STATUS_FAULT:
            {
                //
                // Only accept status responses from the current device ID.
                //
                if((sMsgObject.ulMsgID & CAN_MSGID_DEVNO_M) == g_ulCurrentID)
                {
                    g_ulStatusFault = *(unsigned short *)pucData;
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_FAULT) = 1;
                }
                else
                {
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_FAULT) = 0;
                }

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is a response to a power status request.
            //
            case LM_API_STATUS_POWER:
            {
                //
                // Make sure that this response corresponds to the message at
                // the head of the queue.
                //
                if((sMsgObject.ulMsgID ==
                    g_psCANQueue[g_ulCANQueueRead].ulMsgID) &&
                   (g_psCANQueue[g_ulCANQueueRead].ulMsgLen == 0))
                {
                    //
                    // Save the contents of the message.
                    //
                    pulMsgData = (unsigned long *)
                        (*(unsigned long *)(g_psCANQueue[g_ulCANQueueRead].
                                            pucMsgData));
                    pulMsgData[1] = *(unsigned long *)pucData;
                    pulMsgData[2] = *(unsigned long *)(pucData + 4);
                    pulMsgData[0] = sMsgObject.ulMsgLen;
                }

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is a response to a firmware version request.
            //
            case CAN_MSGID_API_FIRMVER:
            {
                //
                // Only accept status responses from the current device ID.
                //
                if((sMsgObject.ulMsgID & CAN_MSGID_DEVNO_M) == g_ulCurrentID)
                {
                    g_ulStatusFirmwareVersion = *(unsigned long *)pucData;
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_FIRMVER) = 1;
                }
                else
                {
                    HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_FIRMVER) = 0;
                }

                //
                // The CAN interface is now idle.
                //
                g_ulCANState = CAN_STATE_IDLE;
                g_ulCANCount = 0;

                //
                // Advance past this entry in the CAN message queue.
                //
                g_ulCANQueueRead = (g_ulCANQueueRead + 1) % QUEUE_SIZE;

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is a response to a parameter read.
            //
            case LM_API_VOLT_SET:
            case LM_API_VOLT_SET_RAMP:
            case LM_API_VCOMP_SET:
            case LM_API_VCOMP_IN_RAMP:
            case LM_API_VCOMP_COMP_RAMP:
            case LM_API_SPD_SET:
            case LM_API_SPD_PC:
            case LM_API_SPD_IC:
            case LM_API_SPD_DC:
            case LM_API_SPD_REF:
            case LM_API_POS_SET:
            case LM_API_POS_PC:
            case LM_API_POS_IC:
            case LM_API_POS_DC:
            case LM_API_POS_REF:
            case LM_API_ICTRL_SET:
            case LM_API_ICTRL_PC:
            case LM_API_ICTRL_IC:
            case LM_API_ICTRL_DC:
            case LM_API_CFG_NUM_BRUSHES:
            case LM_API_CFG_ENC_LINES:
            case LM_API_CFG_POT_TURNS:
            case LM_API_CFG_BRAKE_COAST:
            case LM_API_CFG_LIMIT_MODE:
            case LM_API_CFG_LIMIT_FWD:
            case LM_API_CFG_LIMIT_REV:
            case LM_API_CFG_MAX_VOUT:
            {
                //
                // Make sure that this response corresponds to the message at
                // the head of the queue.
                //
                if((sMsgObject.ulMsgID ==
                    g_psCANQueue[g_ulCANQueueRead].ulMsgID) &&
                   (g_psCANQueue[g_ulCANQueueRead].ulMsgLen == 0))
                {
                    //
                    // Save the contents of the message.
                    //
                    pulMsgData = (unsigned long *)
                        (*(unsigned long *)(g_psCANQueue[g_ulCANQueueRead].
                                            pucMsgData));
                    pulMsgData[1] = *(unsigned long *)pucData;
                    pulMsgData[2] = *(unsigned long *)(pucData + 4);
                    pulMsgData[0] = sMsgObject.ulMsgLen;
                }

                //
                // The CAN interface is now idle.
                //
                g_ulCANState = CAN_STATE_IDLE;
                g_ulCANCount = 0;

                //
                // Advance past this entry in the CAN message queue.
                //
                g_ulCANQueueRead = (g_ulCANQueueRead + 1) % QUEUE_SIZE;

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is an ACK response from a motor control command.
            //
            case LM_API_ACK:
            {

                //
                // The CAN interface is now idle.
                //
                g_ulCANState = CAN_STATE_IDLE;
                g_ulCANCount = 0;

                //
                // Advance past this entry in the CAN message queue.
                //
                g_ulCANQueueRead = (g_ulCANQueueRead + 1) % QUEUE_SIZE;

                //
                // This message has been handled.
                //
                break;
            }

            //
            // This is an ACK response from the boot loader.
            //
            case LM_API_UPD_ACK:
            {
                //
                // The CAN interface is now idle.
                //
                g_ulCANState = CAN_STATE_IDLE;
                g_ulCANCount = 0;

                //
                // Advance past this entry in the CAN message queue.
                //
                g_ulCANQueueRead = (g_ulCANQueueRead + 1) % QUEUE_SIZE;

                //
                // Indicate that an ACK has been received from the boot loader.
                //
                g_ulCANUpdateAck = 1;

                //
                // This message has been handled.
                //
                break;
            }

            //
            // All other messages are handled here.
            //
            default:
            {
                //
                // Mask off the MSB that may have been added above.
                //
                sMsgObject.ulMsgID &= 0x7fffffff;

                //
                // Broadcast messages and the boot loader reset command will
                // not have ACK packets.
                //
                if(((sMsgObject.ulMsgID &
                     (CAN_MSGID_MFR_M | CAN_MSGID_DTYPE_M)) == 0) ||
                   (sMsgObject.ulMsgID == LM_API_UPD_RESET))
                {
                    //
                    // The CAN interface is now idle.
                    //
                    g_ulCANState = CAN_STATE_IDLE;
                    g_ulCANCount = 0;

                    //
                    // Advance past this entry in the CAN message queue.
                    //
                    g_ulCANQueueRead = (g_ulCANQueueRead + 1) % QUEUE_SIZE;
                }

                //
                // Otherwise, only advance to the ACK state if the ACK has not
                // been seen yet.
                //
                else if(g_ulCANState != CAN_STATE_IDLE)
                {
                    //
                    // Advance to the ACK state.
                    //
                    g_ulCANState = CAN_STATE_WAIT_FOR_ACK;

                    //
                    // Provide a longer timeout for boot loader ACKs.
                    //
                    if((sMsgObject.ulMsgID == LM_API_UPD_PING) ||
                       (sMsgObject.ulMsgID == LM_API_UPD_DOWNLOAD) ||
                       (sMsgObject.ulMsgID == LM_API_UPD_SEND_DATA))
                    {
                        g_ulCANCount = CAN_COUNT_WAIT_FOR_UPD;
                    }
                    else
                    {
                        g_ulCANCount = CAN_COUNT_WAIT_FOR_ACK;
                    }
                }

                //
                // This message has been handled.
                //
                break;
            }
        }
    }

    //
    // See if a heartbeat should be sent.
    //
    if((g_ulCANHeartbeat == 1) && (g_ulCANDelayCount == 0))
    {
        //
        // Send a heartbeat message.
        //
        sMsgObject.ulMsgID = CAN_MSGID_API_HEARTBEAT;
        sMsgObject.ulFlags = MSG_OBJ_EXTENDED_ID;
        sMsgObject.ulMsgLen = 0;
        CANMessageSet(CAN0_BASE, HEARTBEAT_MSG_OBJ, &sMsgObject,
                      MSG_OBJ_TYPE_TX);

        //
        // Clear the heartbeat request.
        //
        g_ulCANHeartbeat = 0;
    }

    //
    // See if the CAN interface is idle and there is a message to send.
    //
    if((g_ulCANState == CAN_STATE_IDLE) && (g_ulCANDelayCount == 0) &&
       (g_ulCANQueueRead != g_ulCANQueueWrite))
    {
        //
        // See if this is a bus enumeration request.
        //
        if(g_psCANQueue[g_ulCANQueueRead].ulMsgID == CAN_MSGID_API_ENUMERATE)
        {
            //
            // Reset the enumeration data.
            //
            g_pulStatusEnumeration[0] = 0;
            g_pulStatusEnumeration[1] = 0;

            //
            // Delay all further CAN activity for 80ms while the enumeration
            // occurs.
            //
            g_ulCANDelayCount = 80;
        }

        //
        // Clear the update ACK indicator if this is an update command.
        //
        if((g_psCANQueue[g_ulCANQueueRead].ulMsgID &
            (CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M)) == LM_API_UPD)
        {
            g_ulCANUpdateAck = 0;
        }

        //
        // Determine the message object to be used to send this command.
        //
        if((g_psCANQueue[g_ulCANQueueRead].ulMsgID & ~(CAN_MSGID_DEVNO_M)) ==
           CAN_MSGID_API_FIRMVER)
        {
            //
            // Use the firmware version messsage object for this firmware
            // version request.
            //
            g_psCANQueue[g_ulCANQueueRead].ulFlags &= ~(MSG_OBJ_TX_INT_ENABLE);
            g_psCANQueue[g_ulCANQueueRead].ulFlags |= MSG_OBJ_RX_INT_ENABLE;
            CANMessageSet(CAN0_BASE, FIRMWARE_VER_MSG_OBJ,
                          g_psCANQueue + g_ulCANQueueRead,
                          MSG_OBJ_TYPE_TX_REMOTE);
        }
        else
        {
            //
            // Use the command message object for this message.
            //
            CANMessageSet(CAN0_BASE, COMMAND_MSG_OBJ,
                          g_psCANQueue + g_ulCANQueueRead, MSG_OBJ_TYPE_TX);
        }

        //
        // Wait for a short period of time for this message to be sent.
        //
        g_ulCANState = CAN_STATE_WAIT_FOR_SEND;
        g_ulCANCount = CAN_COUNT_WAIT_FOR_SEND;
    }

    //
    // Return if the timer has not generated a fake CAN interrupt.
    //
    if(g_ulCANTick == 0)
    {
        return;
    }

    //
    // Clear the timer interrupt flag.
    //
    g_ulCANTick = 0;

    //
    // Return if there is not a active timeout counter.
    //
    if(g_ulCANCount == 0)
    {
        return;
    }

    //
    // Decrement the timeout counter.
    //
    g_ulCANCount--;

    //
    // Return if the timeout counter has not expired yet.
    //
    if(g_ulCANCount != 0)
    {
        return;
    }

    //
    // Return if the CAN interface is idle.
    //
    if(g_ulCANState == CAN_STATE_IDLE)
    {
        g_ulCANCount = 0;
        return;
    }

    //
    // Clear the message object so it no longer attempts to send out the
    // message.
    //
    CANMessageClear(CAN0_BASE, COMMAND_MSG_OBJ);

    //
    // Determine the message that was being sent.
    //
    switch(g_psCANQueue[g_ulCANQueueRead].ulMsgID & ~(CAN_MSGID_DEVNO_M))
    {
        //
        // The output voltage was being requested.
        //
        case LM_API_STATUS_VOLTOUT:
        {
            //
            // Indicate that the output voltage status is no longer valid.
            //
            HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VOUT) = 0;
            break;
        }

        //
        // The bus voltage was being requested.
        //
        case LM_API_STATUS_VOLTBUS:
        {
            //
            // Indicate that the bus voltage status is no longer valid.
            //
            HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_VBUS) = 0;
            break;
        }

        //
        // The motor current was being requested.
        //
        case LM_API_STATUS_CURRENT:
        {
            //
            // Indicate that the motor current status is no longer valid.
            //
            HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_CURRENT) = 0;
            break;
        }

        //
        // The ambient temperature was being requested.
        //
        case LM_API_STATUS_TEMP:
        {
            //
            // Indicate that the ambient temperature status is no longer valid.
            //
            HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_TEMP) = 0;
            break;
        }

        //
        // The position was being requested.
        //
        case LM_API_STATUS_POS:
        {
            //
            // Indicate that the position status is no longer valid.
            //
            HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_POS) = 0;

            //
            // See if this was an explicit position read request.
            //
            if(g_psCANQueue[g_ulCANQueueRead].pucMsgData)
            {
                //
                // Indicate that the parameter read timed out.
                //
                pulMsgData = (unsigned long *)
                    (*(unsigned long *)(g_psCANQueue[g_ulCANQueueRead].
                                        pucMsgData));
                pulMsgData[0] = 0xfffffffe;
            }
            break;
        }

        //
        // The speed was being requested.
        //
        case LM_API_STATUS_SPD:
        {
            //
            // Indicate that the speed status is no longer valid.
            //
            HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_SPEED) = 0;
            break;
        }

        //
        // The limit switch state was being requested.
        //
        case LM_API_STATUS_LIMIT:
        {
            //
            // Indicate that the limit switch status is no longer valid.
            //
            HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_LIMIT) = 0;
            break;
        }

        //
        // The fault state was being requested.
        //
        case LM_API_STATUS_FAULT:
        {
            //
            // Indicate that the fault status is no longer valid.
            //
            HWREGBITW(&g_ulStatusFlags, STATUS_FLAG_FAULT) = 0;
            break;
        }

        //
        // A parameter is being read from the motor controller.
        //
        case LM_API_VOLT_SET:
        case LM_API_VOLT_SET_RAMP:
        case LM_API_SPD_SET:
        case LM_API_SPD_PC:
        case LM_API_SPD_IC:
        case LM_API_SPD_DC:
        case LM_API_SPD_REF:
        case LM_API_POS_SET:
        case LM_API_POS_PC:
        case LM_API_POS_IC:
        case LM_API_POS_DC:
        case LM_API_POS_REF:
        case LM_API_ICTRL_SET:
        case LM_API_ICTRL_PC:
        case LM_API_ICTRL_IC:
        case LM_API_ICTRL_DC:
        case LM_API_CFG_NUM_BRUSHES:
        case LM_API_CFG_ENC_LINES:
        case LM_API_CFG_POT_TURNS:
        case LM_API_CFG_BRAKE_COAST:
        case LM_API_CFG_LIMIT_MODE:
        case LM_API_CFG_LIMIT_FWD:
        case LM_API_CFG_LIMIT_REV:
        case LM_API_CFG_MAX_VOUT:
        case LM_API_STATUS_POWER:
        {
            //
            // Indicate that the parameter read timed out.
            //
            if(g_psCANQueue[g_ulCANQueueRead].ulMsgLen == 0)
            {
                pulMsgData = (unsigned long *)
                    (*(unsigned long *)(g_psCANQueue[g_ulCANQueueRead].
                                        pucMsgData));
                pulMsgData[0] = 0xfffffffe;
            }
            break;
        }
    }

    //
    // Advance past this entry in the CAN message queue.
    //
    g_ulCANQueueRead = (g_ulCANQueueRead + 1) % QUEUE_SIZE;

    //
    // The CAN interface is now idle.
    //
    g_ulCANState = CAN_STATE_IDLE;
}

//*****************************************************************************
//
// This function is called periodically to perform timed CAN actions.
//
//*****************************************************************************
void
CANTick(void)
{
    //
    // Decrement the CAN tick counter and see if it has reached zero.
    //
    if(--g_ulCANTickCount == 0)
    {
        //
        // Indicate that a heartbeat message needs to be sent.
        //
        g_ulCANHeartbeat = 1;

        //
        // See if status querying is enabled.
        //
        if(HWREGBITW(&g_ulStatusFlags, STATUS_ENABLED) == 1)
        {
            //
            // Request the output voltage from the current motor controller.
            //
            CANSendMessage(LM_API_STATUS_VOLTOUT | g_ulCurrentID, 0, 0, 0);
        }

        //
        // Reset the CAN tick counter.
        //
        g_ulCANTickCount = 1000 / UPDATES_PER_SECOND;
    }

    //
    // Decrement the CAN delay count if it is non-zero.
    //
    if(g_ulCANDelayCount)
    {
        g_ulCANDelayCount--;
    }

    //
    // Generate a fake CAN interrupt.
    //
    g_ulCANTick = 1;
    HWREG(NVIC_SW_TRIG) |= INT_CAN0 - 16;
}

//*****************************************************************************
//
// Enabled the CAN-based query of status information.
//
//*****************************************************************************
void
CANStatusEnable(void)
{
    //
    // Commence the status requests.
    //
    HWREGBITW(&g_ulStatusFlags, STATUS_ENABLED) = 1;
}

//*****************************************************************************
//
// Disables the CAN-based query of status information.
//
//*****************************************************************************
void
CANStatusDisable(void)
{
    //
    // Halt the status requests.
    //
    HWREGBITW(&g_ulStatusFlags, STATUS_ENABLED) = 0;
}
