//*****************************************************************************
//
// can_if.c - Functions that are used to interact with the CAN controller.
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

#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/watchdog.h"
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
#include "param.h"

extern void CallBootloader(void);

//*****************************************************************************
//
// The firmware version.
//
//*****************************************************************************
const unsigned long g_ulFirmwareVersion = 10636;

//*****************************************************************************
//
// The hardware version.
//
//*****************************************************************************
unsigned char g_ucHardwareVersion = 0;

//*****************************************************************************
//
// This structure holds any pending updates.
//
//*****************************************************************************
static struct
{
    //
    // The pending voltage update.
    //
    short sVoltage;

    //
    // The pending voltage compensation update.
    //
    short sVComp;

    //
    // The pending current update.
    //
    short sCurrent;

    //
    // The pending position update.
    //
    long lPosition;

    //
    // The pending speed update.
    //
    long lSpeed;

    //
    // The group number for a pending voltage update.
    //
    unsigned char ucVoltageGroup;

    //
    // The group number for a pending voltage compensation update.
    //
    unsigned char ucVCompGroup;

    //
    // The group number for a pending current update.
    //
    unsigned char ucCurrentGroup;

    //
    // The group number for a pending position update.
    //
    unsigned char ucPositionGroup;

    //
    // The group number for a pending speed update.
    //
    unsigned char ucSpeedGroup;
}
g_sPendingUpdates;

//*****************************************************************************
//
// The static enumeration data.
//
//*****************************************************************************
static const unsigned char g_pucEnumData[8] =
{
    //
    // Device Function, Motor Controller.
    //
    (unsigned char)(CAN_MSGID_DTYPE_MOTOR >> CAN_MSGID_DTYPE_S),

    //
    // Manufacturer, Luminary Micro.
    //
    (unsigned char)(CAN_MSGID_MFR_LM >> CAN_MSGID_MFR_S),

    //
    // Reserved values.
    //
    0, 0, 0, 0, 0, 0
};

//*****************************************************************************
//
// This structure holds the static definitions used to configure the CAN
// controller bit clock in the proper state given a requested setting.
//
// These are for a 16MHz clock running at 1Mbit CAN rate.
//
//*****************************************************************************
static const tCANBitClkParms CANBitClkSettings =
{
    5, 2, 2, 2
};

//*****************************************************************************
//
// These states are used to determine the mode during enumeration.
//
//*****************************************************************************
static enum
{
    //
    // This is the normal operating state.
    //
    STATE_IDLE,

    //
    // The controller has received a device assignement command and is waiting
    // to accept or reject the change.
    //
    STATE_ASSIGNMENT,

    //
    // The assignment delay has expired, or the button has been pressed, and
    // the CAN interface is ready to be reconfigured appropriately.
    //
    STATE_ASSIGN_END,

    //
    // The controller has received an enumeration request and is waiting to
    // send out the response.
    //
    STATE_ENUMERATE,

    //
    // The enumeration delay has expired and the response is ready to be sent.
    //
    STATE_ENUM_END
}
g_eCANState;

//*****************************************************************************
//
// These globals hold the Event tick information to CAN related timing
// functions.
//
//*****************************************************************************
static unsigned long g_ulTickCount;
static unsigned long g_ulEventTick;

//*****************************************************************************
//
// The message object number and index to the local message object memory to
// use when accessing the messages.
//
//*****************************************************************************
#define MSG_OBJ_BCAST_RX        0
#define MSG_OBJ_BCAST_RX_ID     (MSG_OBJ_BCAST_RX + 1)
#define MSG_OBJ_BCAST_TX        1
#define MSG_OBJ_BCAST_TX_ID     (MSG_OBJ_BCAST_TX + 1)
#define MSG_OBJ_DEV_QUERY       2
#define MSG_OBJ_DEV_QUERY_ID    (MSG_OBJ_DEV_QUERY + 1)
#define MSG_OBJ_VOLT_NO_ACK     3
#define MSG_OBJ_VOLT_NO_ACK_ID  (MSG_OBJ_VOLT_NO_ACK + 1)
#define MSG_OBJ_SPD_NO_ACK      4
#define MSG_OBJ_SPD_NO_ACK_ID   (MSG_OBJ_SPD_NO_ACK + 1)
#define MSG_OBJ_VCOMP_NO_ACK    5
#define MSG_OBJ_VCOMP_NO_ACK_ID (MSG_OBJ_VCOMP_NO_ACK + 1)
#define MSG_OBJ_POS_NO_ACK      6
#define MSG_OBJ_POS_NO_ACK_ID   (MSG_OBJ_POS_NO_ACK + 1)
#define MSG_OBJ_ICTRL_NO_ACK    7
#define MSG_OBJ_ICTRL_NO_ACK_ID (MSG_OBJ_ICTRL_NO_ACK + 1)
#define MSG_OBJ_MC_RX           8
#define MSG_OBJ_MC_RX_ID        (MSG_OBJ_MC_RX + 1)
#define MSG_OBJ_FIRM_VER        9
#define MSG_OBJ_FIRM_VER_ID     (MSG_OBJ_FIRM_VER + 1)
#define MSG_OBJ_UPD_RX          10
#define MSG_OBJ_UPD_RX_ID       (MSG_OBJ_UPD_RX + 1)
#define MSG_OBJ_NUM_OBJECTS     11

//*****************************************************************************
//
// This holds the information for the data send message object that is used
// to send traffic on the CAN network.
//
//*****************************************************************************
static tCANMsgObject g_psMsgObj[MSG_OBJ_NUM_OBJECTS];

//*****************************************************************************
//
// This is the buffer for data received by the broadcast message object.
//
//*****************************************************************************
static unsigned char g_pucBcastData[8];

//*****************************************************************************
//
// Global buffer used to receive buffer commands.
//
//*****************************************************************************
static unsigned char g_pucMCData[8];

//*****************************************************************************
//
// Global buffer used to receive update commands.
//
//*****************************************************************************
static unsigned char g_pucUPDData[8];

//*****************************************************************************
//
// This holds the pending device number during the assignement state.
//
//*****************************************************************************
static unsigned char g_ucDevNumPending;

//*****************************************************************************
//
// This holds the pending device number during the assignement state.
//
//*****************************************************************************
static unsigned char g_ucFlags;

//*****************************************************************************
//
// This contains the latest value of the CAN Status register.  It is updated
// every CAN interrupt.
//
//*****************************************************************************
static unsigned long g_ulCANStatus = 0;

//*****************************************************************************
//
// This flag is set during startup and can be cleared only by another device
// sending a Power Status command.
//
//*****************************************************************************
#define CAN_FLAGS_POR           0x01

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
static unsigned char g_ppucPStatMessages[4][8];

//*****************************************************************************
//
// The length of the periodic status messages.
//
//*****************************************************************************
static unsigned char g_pucPStatMessageLen[4];

//*****************************************************************************
//
// A set of flags that indicate the periodic status messages that are pending
// transmission.
//
//*****************************************************************************
static unsigned long g_ulPStatFlags;

//*****************************************************************************
//
// A local copy of CANDataRegRead that does not use paced reads/writes since
// they are not needed on the LM3S2616.
//
//*****************************************************************************
static void
CANIFDataRegRead(unsigned char *pucData, unsigned long ulRegister,
                 unsigned long ulSize)
{
    unsigned long ulValue, ulIdx;

    //
    // Loop always copies 1 or 2 bytes per iteration.
    //
    for(ulIdx = 0; ulIdx < ulSize; ulIdx += 2)
    {
        //
        // Read out the data 16 bits at a time since this is how the registers
        // are aligned in memory.
        //
        ulValue = HWREG(ulRegister);
        ulRegister += 4;

        //
        // Store the first byte.
        //
        *pucData++ = (unsigned char)ulValue;
        *pucData++ = (unsigned char)(ulValue >> 8);
    }
}

//*****************************************************************************
//
// A local copy of CANDataRegWrite that does not use paced reads/writes since
// they are not needed on the LM3S2616.
//
//*****************************************************************************
static void
CANIFDataRegWrite(unsigned char *pucData, unsigned long ulRegister,
                  unsigned long ulSize)
{
    unsigned long ulValue, ulIdx;

    //
    // Loop always copies 1 or 2 bytes per iteration.
    //
    for(ulIdx = 0; ulIdx < ulSize; ulIdx += 2)
    {
        //
        // Write out the data 16 bits at a time since this is how the registers
        // are aligned in memory.
        //
        ulValue = *pucData++;
        ulValue |= *pucData++ << 8;
        HWREG(ulRegister) = ulValue;
        ulRegister += 4;
    }
}

//*****************************************************************************
//
// A local copy of CANMessageGet that does not use paced reads/writes since
// they are not needed on the LM3S2616.
//
//*****************************************************************************
static void
CANIFMessageGet(unsigned long ulObjID, tCANMsgObject *pMsgObject)
{
    unsigned short usCmdMaskReg;
    unsigned short usMaskReg0, usMaskReg1;
    unsigned short usArbReg0, usArbReg1;
    unsigned short usMsgCtrl;

    //
    // This is always a read to the Message object as this call is setting a
    // message object.
    //
    usCmdMaskReg = (CAN_IF2CMSK_DATAA | CAN_IF2CMSK_DATAB |
                    CAN_IF2CMSK_CONTROL | CAN_IF2CMSK_MASK | CAN_IF2CMSK_ARB |
                    CAN_IF2CMSK_CLRINTPND);

    //
    // Set up the request for data from the message object.
    //
    HWREG(CAN0_BASE + CAN_O_IF2CMSK) = usCmdMaskReg;

    //
    // Transfer the message object to the message object specified by ulObjID.
    //
    HWREG(CAN0_BASE + CAN_O_IF2CRQ) = ulObjID & CAN_IF2CRQ_MNUM_M;

    //
    // Wait for busy bit to clear
    //
    while(HWREG(CAN0_BASE + CAN_O_IF2CRQ) & CAN_IF2CRQ_BUSY)
    {
    }

    //
    // Read out the IF Registers.
    //
    usMaskReg0 = HWREG(CAN0_BASE + CAN_O_IF2MSK1);
    usMaskReg1 = HWREG(CAN0_BASE + CAN_O_IF2MSK2);
    usArbReg0 = HWREG(CAN0_BASE + CAN_O_IF2ARB1);
    usArbReg1 = HWREG(CAN0_BASE + CAN_O_IF2ARB2);
    usMsgCtrl = HWREG(CAN0_BASE + CAN_O_IF2MCTL);

    pMsgObject->ulFlags = MSG_OBJ_NO_FLAGS;

    //
    // Determine if this is a remote frame by checking the TXRQST and DIR bits.
    //
    if((!(usMsgCtrl & CAN_IF1MCTL_TXRQST) && (usArbReg1 & CAN_IF2ARB2_DIR)) ||
       ((usMsgCtrl & CAN_IF1MCTL_TXRQST) && (!(usArbReg1 & CAN_IF2ARB2_DIR))))
    {
        pMsgObject->ulFlags |= MSG_OBJ_REMOTE_FRAME;
    }

    //
    // Get the identifier out of the register, the format depends on size of
    // the mask.
    //
    if(usArbReg1 & CAN_IF2ARB2_XTD)
    {
        //
        // Set the 29 bit version of the Identifier for this message object.
        //
        pMsgObject->ulMsgID = (((usArbReg1 & CAN_IF2ARB2_ID_M) << 16) |
                               usArbReg0);

        pMsgObject->ulFlags |= MSG_OBJ_EXTENDED_ID;
    }
    else
    {
        //
        // The Identifier is an 11 bit value.
        //
        pMsgObject->ulMsgID = (usArbReg1 & CAN_IF2ARB2_ID_M) >> 2;
    }

    //
    // Indicate that we lost some data.
    //
    if(usMsgCtrl & CAN_IF1MCTL_MSGLST)
    {
        pMsgObject->ulFlags |= MSG_OBJ_DATA_LOST;
    }

    //
    // Set the flag to indicate if ID masking was used.
    //
    if(usMsgCtrl & CAN_IF2MCTL_UMASK)
    {
        if(usArbReg1 & CAN_IF2ARB2_XTD)
        {
            //
            // The Identifier Mask is assumed to also be a 29 bit value.
            //
            pMsgObject->ulMsgIDMask =
                ((usMaskReg1 & CAN_IF2MSK2_IDMSK_M) << 16) | usMaskReg0;

            //
            // If this is a fully specified Mask and a remote frame then don't
            // set the MSG_OBJ_USE_ID_FILTER because the ID was not really
            // filtered.
            //
            if((pMsgObject->ulMsgIDMask != 0x1fffffff) ||
               ((pMsgObject->ulFlags & MSG_OBJ_REMOTE_FRAME) == 0))
            {
                pMsgObject->ulFlags |= MSG_OBJ_USE_ID_FILTER;
            }
        }
        else
        {
            //
            // The Identifier Mask is assumed to also be an 11 bit value.
            //
            pMsgObject->ulMsgIDMask = ((usMaskReg1 & CAN_IF2MSK2_IDMSK_M) >>
                                       2);

            //
            // If this is a fully specified Mask and a remote frame then don't
            // set the MSG_OBJ_USE_ID_FILTER because the ID was not really
            // filtered.
            //
            if((pMsgObject->ulMsgIDMask != 0x7ff) ||
               ((pMsgObject->ulFlags & MSG_OBJ_REMOTE_FRAME) == 0))
            {
                pMsgObject->ulFlags |= MSG_OBJ_USE_ID_FILTER;
            }
        }

        //
        // Indicate if the extended bit was used in filtering.
        //
        if(usMaskReg1 & CAN_IF2MSK2_MXTD)
        {
            pMsgObject->ulFlags |= MSG_OBJ_USE_EXT_FILTER;
        }

        //
        // Indicate if direction filtering was enabled.
        //
        if(usMaskReg1 & CAN_IF2MSK2_MDIR)
        {
            pMsgObject->ulFlags |= MSG_OBJ_USE_DIR_FILTER;
        }
    }

    //
    // Set the interrupt flags.
    //
    if(usMsgCtrl & CAN_IF2MCTL_TXIE)
    {
        pMsgObject->ulFlags |= MSG_OBJ_TX_INT_ENABLE;
    }
    if(usMsgCtrl & CAN_IF2MCTL_RXIE)
    {
        pMsgObject->ulFlags |= MSG_OBJ_RX_INT_ENABLE;
    }

    //
    // See if there is new data available.
    //
    if(usMsgCtrl & CAN_IF2MCTL_NEWDAT)
    {
        //
        // Get the amount of data needed to be read.
        //
        pMsgObject->ulMsgLen = (usMsgCtrl & CAN_IF2MCTL_DLC_M);

        //
        // Don't read any data for a remote frame, there is nothing valid in
        // that buffer anyway.
        //
        if((pMsgObject->ulFlags & MSG_OBJ_REMOTE_FRAME) == 0)
        {
            //
            // Read out the data from the CAN registers.
            //
            CANIFDataRegRead(pMsgObject->pucMsgData, CAN0_BASE + CAN_O_IF2DA1,
                             pMsgObject->ulMsgLen);
        }

        //
        // Now clear out the new data flag.
        //
        HWREG(CAN0_BASE + CAN_O_IF2CMSK) = CAN_IF1CMSK_NEWDAT;

        //
        // Transfer the message object to the message object specified by
        // ulObjID.
        //
        HWREG(CAN0_BASE + CAN_O_IF2CRQ) = ulObjID & CAN_IF1CRQ_MNUM_M;

        //
        // Wait for busy bit to clear
        //
        while(HWREG(CAN0_BASE + CAN_O_IF2CRQ) & CAN_IF1CRQ_BUSY)
        {
        }

        //
        // Indicate that there is new data in this message.
        //
        pMsgObject->ulFlags |= MSG_OBJ_NEW_DATA;
    }
    else
    {
        //
        // Along with the MSG_OBJ_NEW_DATA not being set the amount of data
        // needs to be set to zero if none was available.
        //
        pMsgObject->ulMsgLen = 0;
    }
}

//*****************************************************************************
//
// A local copy of CANMessageSet that does not use paced reads/writes since
// they are not needed on the LM3S2616.
//
//*****************************************************************************
void
CANIFMessageSet(unsigned long ulObjID, tCANMsgObject *pMsgObject,
                tMsgObjType eMsgType)
{
    unsigned short usCmdMaskReg;
    unsigned short usMaskReg0, usMaskReg1;
    unsigned short usArbReg0, usArbReg1;
    unsigned short usMsgCtrl;
    tBoolean bTransferData;

    bTransferData = 0;

    //
    // Wait for busy bit to clear
    //
    while(HWREG(CAN0_BASE + CAN_O_IF1CRQ) & CAN_IF1CRQ_BUSY)
    {
    }

    //
    // This is always a write to the Message object as this call is setting a
    // message object.  This call will also always set all size bits so it sets
    // both data bits.  The call will use the CONTROL register to set control
    // bits so this bit needs to be set as well.
    //
    usCmdMaskReg = (CAN_IF1CMSK_WRNRD | CAN_IF1CMSK_DATAA | CAN_IF1CMSK_DATAB |
                    CAN_IF1CMSK_CONTROL);

    //
    // Initialize the values to a known state before filling them in based on
    // the type of message object that is being configured.
    //
    usArbReg0 = 0;
    usArbReg1 = 0;
    usMsgCtrl = 0;
    usMaskReg0 = 0;
    usMaskReg1 = 0;

    switch(eMsgType)
    {
        //
        // Transmit message object.
        //
        case MSG_OBJ_TYPE_TX:
        {
            //
            // Set the TXRQST bit and the reset the rest of the register.
            //
            usMsgCtrl |= CAN_IF1MCTL_TXRQST;
            usArbReg1 = CAN_IF1ARB2_DIR;
            bTransferData = 1;
            break;
        }

        //
        // Transmit remote request message object
        //
        case MSG_OBJ_TYPE_TX_REMOTE:
        {
            //
            // Set the TXRQST bit and the reset the rest of the register.
            //
            usMsgCtrl |= CAN_IF1MCTL_TXRQST;
            usArbReg1 = 0;
            break;
        }

        //
        // Receive message object.
        //
        case MSG_OBJ_TYPE_RX:
        {
            //
            // This clears the DIR bit along with everything else.  The TXRQST
            // bit was cleared by defaulting usMsgCtrl to 0.
            //
            usArbReg1 = 0;
            break;
        }

        //
        // Receive remote request message object.
        //
        case MSG_OBJ_TYPE_RX_REMOTE:
        {
            //
            // The DIR bit is set to one for remote receivers.  The TXRQST bit
            // was cleared by defaulting usMsgCtrl to 0.
            //
            usArbReg1 = CAN_IF1ARB2_DIR;

            //
            // Set this object so that it only indicates that a remote frame
            // was received and allow for software to handle it by sending back
            // a data frame.
            //
            usMsgCtrl = CAN_IF1MCTL_UMASK;

            //
            // Use the full Identifier by default.
            //
            usMaskReg0 = 0xffff;
            usMaskReg1 = 0x1fff;

            //
            // Make sure to send the mask to the message object.
            //
            usCmdMaskReg |= CAN_IF1CMSK_MASK;
            break;
        }

        //
        // Remote frame receive remote, with auto-transmit message object.
        //
        case MSG_OBJ_TYPE_RXTX_REMOTE:
        {
            //
            // Oddly the DIR bit is set to one for remote receivers.
            //
            usArbReg1 = CAN_IF1ARB2_DIR;

            //
            // Set this object to auto answer if a matching identifier is seen.
            //
            usMsgCtrl = CAN_IF1MCTL_RMTEN | CAN_IF1MCTL_UMASK;

            //
            // The data to be returned needs to be filled in.
            //
            bTransferData = 1;
            break;
        }

        //
        // This case should never happen due to the ASSERT statement at the
        // beginning of this function.
        //
        default:
        {
            return;
        }
    }

    //
    // Configure the Mask Registers.
    //
    if(pMsgObject->ulFlags & MSG_OBJ_USE_ID_FILTER)
    {
        //
        // Set the 29 bits of Identifier mask that were requested.
        //
        usMaskReg0 = pMsgObject->ulMsgIDMask & CAN_IF1MSK1_IDMSK_M;
        usMaskReg1 = (pMsgObject->ulMsgIDMask >> 16) & CAN_IF1MSK2_IDMSK_M;
    }

    //
    // If the caller wants to filter on the extended ID bit then set it.
    //
    if((pMsgObject->ulFlags & MSG_OBJ_USE_EXT_FILTER) ==
       MSG_OBJ_USE_EXT_FILTER)
    {
        usMaskReg1 |= CAN_IF1MSK2_MXTD;
    }

    //
    // The caller wants to filter on the message direction field.
    //
    if((pMsgObject->ulFlags & MSG_OBJ_USE_DIR_FILTER) ==
       MSG_OBJ_USE_DIR_FILTER)
    {
        usMaskReg1 |= CAN_IF1MSK2_MDIR;
    }

    if(pMsgObject->ulFlags & (MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_DIR_FILTER |
                              MSG_OBJ_USE_EXT_FILTER))
    {
        //
        // Set the UMASK bit to enable using the mask register.
        //
        usMsgCtrl |= CAN_IF1MCTL_UMASK;

        //
        // Set the MASK bit so that this gets transferred to the Message Object.
        //
        usCmdMaskReg |= CAN_IF1CMSK_MASK;
    }

    //
    // Set the Arb bit so that this gets transferred to the Message object.
    //
    usCmdMaskReg |= CAN_IF1CMSK_ARB;

    //
    // Set the 29 bit version of the Identifier for this message object.
    //
    usArbReg0 |= pMsgObject->ulMsgID & CAN_IF1ARB1_ID_M;
    usArbReg1 |= (pMsgObject->ulMsgID >> 16) & CAN_IF1ARB2_ID_M;

    //
    // Mark the message as valid and set the extended ID bit.
    //
    usArbReg1 |= CAN_IF1ARB2_MSGVAL | CAN_IF1ARB2_XTD;

    //
    // Set the data length since this is set for all transfers.  This is also a
    // single transfer and not a FIFO transfer so set EOB bit.
    //
    usMsgCtrl |= (pMsgObject->ulMsgLen & CAN_IF1MCTL_DLC_M);

    //
    // Mark this as the last entry if this is not the last entry in a FIFO.
    //
    if((pMsgObject->ulFlags & MSG_OBJ_FIFO) == 0)
    {
        usMsgCtrl |= CAN_IF1MCTL_EOB;
    }

    //
    // Enable transmit interrupts if they should be enabled.
    //
    if(pMsgObject->ulFlags & MSG_OBJ_TX_INT_ENABLE)
    {
        usMsgCtrl |= CAN_IF1MCTL_TXIE;
    }

    //
    // Enable receive interrupts if they should be enabled.
    //
    if(pMsgObject->ulFlags & MSG_OBJ_RX_INT_ENABLE)
    {
        usMsgCtrl |= CAN_IF1MCTL_RXIE;
    }

    //
    // Write the data out to the CAN Data registers if needed.
    //
    if(bTransferData)
    {
        CANIFDataRegWrite(pMsgObject->pucMsgData, CAN0_BASE + CAN_O_IF1DA1,
                          pMsgObject->ulMsgLen);
    }

    //
    // Write out the registers to program the message object.
    //
    HWREG(CAN0_BASE + CAN_O_IF1CMSK) = usCmdMaskReg;
    HWREG(CAN0_BASE + CAN_O_IF1MSK1) = usMaskReg0;
    HWREG(CAN0_BASE + CAN_O_IF1MSK2) = usMaskReg1;
    HWREG(CAN0_BASE + CAN_O_IF1ARB1) = usArbReg0;
    HWREG(CAN0_BASE + CAN_O_IF1ARB2) = usArbReg1;
    HWREG(CAN0_BASE + CAN_O_IF1MCTL) = usMsgCtrl;

    //
    // Transfer the message object to the message object specified by ulObjID.
    //
    HWREG(CAN0_BASE + CAN_O_IF1CRQ) = ulObjID & CAN_IF1CRQ_MNUM_M;
}

//*****************************************************************************
//
// Initial configuration of the network, this is just for the broadcast
// message configuration.
//
//*****************************************************************************
static void
CANConfigureNetwork(void)
{
    //
    // This message object will be used to send broadcast messages on the
    // CAN network.
    //
    g_psMsgObj[MSG_OBJ_BCAST_TX].ulMsgID = 0;
    g_psMsgObj[MSG_OBJ_BCAST_TX].ulMsgIDMask = 0;
    g_psMsgObj[MSG_OBJ_BCAST_TX].ulFlags = MSG_OBJ_EXTENDED_ID;
    g_psMsgObj[MSG_OBJ_BCAST_TX].ulMsgLen = 0;
    g_psMsgObj[MSG_OBJ_BCAST_TX].pucMsgData = (unsigned char *)0xffffffff;

    //
    // This message object will be used to receive broadcast traffic on the
    // CAN network.
    //
    g_psMsgObj[MSG_OBJ_BCAST_RX].ulMsgID = 0;

    //
    // Any API targeted at a devno of 0, manufacturer of 0, and device type
    // of 0.
    //
    g_psMsgObj[MSG_OBJ_BCAST_RX].ulMsgIDMask =
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_DEVNO_M;

    //
    // Enable extended identifiers, interrupt and using extended message id
    // filtering.
    //
    g_psMsgObj[MSG_OBJ_BCAST_RX].ulFlags =
        MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID | MSG_OBJ_USE_ID_FILTER |
        MSG_OBJ_USE_EXT_FILTER;

    //
    // No data will be read automatically, it will be read when it is received.
    //
    g_psMsgObj[MSG_OBJ_BCAST_RX].ulMsgLen = 8;
    g_psMsgObj[MSG_OBJ_BCAST_RX].pucMsgData = g_pucBcastData;

    //
    // Configure the broadcast receive message object.
    //
    CANIFMessageSet(MSG_OBJ_BCAST_RX_ID, &g_psMsgObj[MSG_OBJ_BCAST_RX],
                    MSG_OBJ_TYPE_RX);
}

//*****************************************************************************
//
// This function sends a message on the CAN bus.
//
// \param ulID is the CAN message identifier to transmit.
// \param pucData is a pointer to the data to transmit.
// \param ulSize is the number of bytes in the pucData buffer to transmit.
//
// This function is used to send response messages to the motor controller
// via the CAN bus.  This command will timeout if a previous message is failing
// to be sent out on the CAN bus.  Otherwise this function will schedule the
// CAN message to go out.
//
// \return This function will return zero to indicate successful scheduling
// of this message.  Any other value indicates that the message could not be
// sent at this time.
//
//*****************************************************************************
static unsigned long
CANSendBroadcastMsg(unsigned long ulId,  unsigned char *pucData,
                    unsigned long ulSize)
{
    int iIndex;
    unsigned long ulStatus;

    //
    // Set the number of times to retry reading the status before a timeout
    // and generating an error.
    //
    iIndex = 1000;

    //
    // Make sure the last transmission was successful.
    //
    while(1)
    {
        //
        // Wait for the transmit status to indicate that the previous message
        // was transmitted.
        //
        ulStatus = CANStatusGet(CAN0_BASE, CAN_STS_TXREQUEST);

        //
        // If the status ever goes to zero then exit the loop.
        //
        if(ulStatus == 0)
        {
            break;
        }

        //
        // If the timeout index ever goes to zero then exit the loop.
        //
        if(--iIndex == 0)
        {
            break;
        }
    }

    //
    // If there was no timeout then the interface is available so send message.
    //
    if(iIndex != 0)
    {
        //
        // Send the response packet.
        //
        g_psMsgObj[MSG_OBJ_BCAST_TX].pucMsgData = pucData;
        g_psMsgObj[MSG_OBJ_BCAST_TX].ulMsgLen = ulSize;
        g_psMsgObj[MSG_OBJ_BCAST_TX].ulMsgID = ulId;
        CANIFMessageSet(MSG_OBJ_BCAST_TX_ID, &g_psMsgObj[MSG_OBJ_BCAST_TX],
                        MSG_OBJ_TYPE_TX);
    }
    else
    {
        //
        // The timeout reached zero so return with a non-zero value.
        //
        return(0xffffffff);
    }
    return(0);
}

//*****************************************************************************
//
// Configure the message objects based on a given device number.
//
// \param ucDevNum is the device number to use for receiving messages.
//
// This function is used to configure the CAN controller to receive messages
// for the device number passed in parameter ucDevNum.  If ucDevNum is zero
// then this function will reset all of the device number specific message
// object to not receive messages.  If ucDevNum is not zero then the message
// objects will be configured to receive messages based on the ucDevNum that
// is passed in.
//
// \return None.
//
//*****************************************************************************
static void
CANDeviceNumSet(unsigned char ucDevNum)
{
    //
    // Save the new device number.
    //
    ucDevNum = ucDevNum & CAN_MSGID_DEVNO_M;

    if(ucDevNum != 0)
    {
        //
        // This message object will be used to receive Motor control messages
        // on the CAN network.
        //
        g_psMsgObj[MSG_OBJ_MC_RX].ulMsgID =
            CAN_MSGID_MFR_LM | CAN_MSGID_DTYPE_MOTOR | ucDevNum;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // and the device type.
        //
        g_psMsgObj[MSG_OBJ_MC_RX].ulMsgIDMask =
            CAN_MSGID_DEVNO_M | CAN_MSGID_MFR_M | CAN_MSGID_DTYPE_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObj[MSG_OBJ_MC_RX].ulFlags =
            MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
            MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER;

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObj[MSG_OBJ_MC_RX].ulMsgLen = 8;
        g_psMsgObj[MSG_OBJ_MC_RX].pucMsgData = g_pucMCData;

        //
        // Configure the motor control message receive message object.
        //
        CANIFMessageSet(MSG_OBJ_MC_RX_ID, &g_psMsgObj[MSG_OBJ_MC_RX],
                        MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive no-ack voltage set
        // message.
        //
        g_psMsgObj[MSG_OBJ_VOLT_NO_ACK].ulMsgID =
            ucDevNum | LM_API_VOLT | LM_API_VOLT_SET_NO_ACK;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // device type, API class and API (the full message ID).
        //
        g_psMsgObj[MSG_OBJ_VOLT_NO_ACK].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObj[MSG_OBJ_VOLT_NO_ACK].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObj[MSG_OBJ_VOLT_NO_ACK].ulMsgLen = 8;
        g_psMsgObj[MSG_OBJ_VOLT_NO_ACK].pucMsgData = g_pucMCData;

        //
        // Configure the no-ack voltage set message object.
        //
        CANIFMessageSet(MSG_OBJ_VOLT_NO_ACK_ID,
                        &g_psMsgObj[MSG_OBJ_VOLT_NO_ACK], MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive no-ack speed
        // set message.
        //
        g_psMsgObj[MSG_OBJ_SPD_NO_ACK].ulMsgID =
            ucDevNum | LM_API_VOLT | LM_API_SPD_SET_NO_ACK;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // device type, API class and API (the full message ID).
        //
        g_psMsgObj[MSG_OBJ_SPD_NO_ACK].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObj[MSG_OBJ_SPD_NO_ACK].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObj[MSG_OBJ_SPD_NO_ACK].ulMsgLen = 8;
        g_psMsgObj[MSG_OBJ_SPD_NO_ACK].pucMsgData = g_pucMCData;

        //
        // Configure the no-ack speed set message object.
        //
        CANIFMessageSet(MSG_OBJ_SPD_NO_ACK_ID, &g_psMsgObj[MSG_OBJ_SPD_NO_ACK],
                        MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive no-ack voltage
        // compensation set message.
        //
        g_psMsgObj[MSG_OBJ_VCOMP_NO_ACK].ulMsgID =
            ucDevNum | LM_API_VOLT | LM_API_VCOMP_SET_NO_ACK;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // device type, API class and API (the full message ID).
        //
        g_psMsgObj[MSG_OBJ_VCOMP_NO_ACK].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObj[MSG_OBJ_VCOMP_NO_ACK].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObj[MSG_OBJ_VCOMP_NO_ACK].ulMsgLen = 8;
        g_psMsgObj[MSG_OBJ_VCOMP_NO_ACK].pucMsgData = g_pucMCData;

        //
        // Configure the no-ack voltage compensation set message object.
        //
        CANIFMessageSet(MSG_OBJ_VCOMP_NO_ACK_ID,
                        &g_psMsgObj[MSG_OBJ_VCOMP_NO_ACK], MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive no-ack position
        // set message.
        //
        g_psMsgObj[MSG_OBJ_POS_NO_ACK].ulMsgID =
            ucDevNum | LM_API_VOLT | LM_API_POS_SET_NO_ACK;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // device type, API class and API (the full message ID).
        //
        g_psMsgObj[MSG_OBJ_POS_NO_ACK].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObj[MSG_OBJ_POS_NO_ACK].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObj[MSG_OBJ_POS_NO_ACK].ulMsgLen = 8;
        g_psMsgObj[MSG_OBJ_POS_NO_ACK].pucMsgData = g_pucMCData;

        //
        // Configure the no-ack voltage compensation set message object.
        //
        CANIFMessageSet(MSG_OBJ_POS_NO_ACK_ID, &g_psMsgObj[MSG_OBJ_POS_NO_ACK],
                        MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive no-ack current
        // conrol set message.
        //
        g_psMsgObj[MSG_OBJ_ICTRL_NO_ACK].ulMsgID =
            ucDevNum | LM_API_VOLT | LM_API_ICTRL_SET_NO_ACK;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // device type, API class and API (the full message ID).
        //
        g_psMsgObj[MSG_OBJ_ICTRL_NO_ACK].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObj[MSG_OBJ_ICTRL_NO_ACK].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObj[MSG_OBJ_ICTRL_NO_ACK].ulMsgLen = 8;
        g_psMsgObj[MSG_OBJ_ICTRL_NO_ACK].pucMsgData = g_pucMCData;

        //
        // Configure the no-ack voltage compensation set message object.
        //
        CANIFMessageSet(MSG_OBJ_ICTRL_NO_ACK_ID,
                        &g_psMsgObj[MSG_OBJ_ICTRL_NO_ACK], MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive update related
        // messages on the CAN network.
        //
        g_psMsgObj[MSG_OBJ_UPD_RX].ulMsgID = LM_API_UPD | ucDevNum;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // and the device type.
        //
        g_psMsgObj[MSG_OBJ_UPD_RX].ulMsgIDMask =
            CAN_MSGID_DEVNO_M | CAN_MSGID_MFR_M | CAN_MSGID_DTYPE_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObj[MSG_OBJ_UPD_RX].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObj[MSG_OBJ_UPD_RX].ulMsgLen = 8;
        g_psMsgObj[MSG_OBJ_UPD_RX].pucMsgData = g_pucUPDData;

        //
        // Configure the motor control message receive message object.
        //
        CANIFMessageSet(MSG_OBJ_UPD_RX_ID, &g_psMsgObj[MSG_OBJ_UPD_RX],
                        MSG_OBJ_TYPE_RX);

        //
        // Configure the auto-responding device indentification data message
        // object.
        //
        g_psMsgObj[MSG_OBJ_DEV_QUERY].ulMsgID = (CAN_MSGID_API_DEVQUERY |
                                                    ucDevNum);

        //
        // Only look at the API and Device identifier.
        //
        g_psMsgObj[MSG_OBJ_DEV_QUERY].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObj[MSG_OBJ_DEV_QUERY].ulFlags = (MSG_OBJ_EXTENDED_ID |
                                                 MSG_OBJ_USE_ID_FILTER |
                                                 MSG_OBJ_USE_EXT_FILTER);

        //
        // The identification data has 8 bytes of data so initialize the values.
        //
        g_psMsgObj[MSG_OBJ_DEV_QUERY].ulMsgLen = 8;
        g_psMsgObj[MSG_OBJ_DEV_QUERY].pucMsgData =
            (unsigned char *)g_pucEnumData;

        //
        // Send the configuration to the device identification message object.
        //
        CANIFMessageSet(MSG_OBJ_DEV_QUERY_ID, &g_psMsgObj[MSG_OBJ_DEV_QUERY],
                        MSG_OBJ_TYPE_RXTX_REMOTE);

        //
        // Configure the auto-responding firmware version message object to
        // look for a fully specified message identifier.
        //
        g_psMsgObj[MSG_OBJ_FIRM_VER].ulMsgID =
            CAN_MSGID_API_FIRMVER | ucDevNum;
        g_psMsgObj[MSG_OBJ_FIRM_VER].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObj[MSG_OBJ_FIRM_VER].ulFlags = (MSG_OBJ_EXTENDED_ID |
                                                MSG_OBJ_USE_ID_FILTER |
                                                MSG_OBJ_USE_EXT_FILTER);

        //
        // Set the firmware version response to 4 bytes long and initialize
        // the data to the value in g_ulFirmwareVerion.
        //
        g_psMsgObj[MSG_OBJ_FIRM_VER].ulMsgLen = 4;
        g_psMsgObj[MSG_OBJ_FIRM_VER].pucMsgData =
            (unsigned char *)&g_ulFirmwareVersion;

        //
        // Send the configuration to the Firmware message object.
        //
        CANIFMessageSet(MSG_OBJ_FIRM_VER_ID, &g_psMsgObj[MSG_OBJ_FIRM_VER],
                        MSG_OBJ_TYPE_RXTX_REMOTE);
    }
    else
    {
        //
        // Reset the state of all the message object and the state of the CAN
        // module to a known state.
        //
        CANInit(CAN0_BASE);

        //
        // Set up the message object(s) that will receive messages on the CAN
        // bus.
        //
        CANConfigureNetwork();

        //
        // Take the CAN1 device out of INIT state.
        //
        CANEnable(CAN0_BASE);

        //
        // Enable interrupts from CAN controller.
        //
        CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR);
    }

    //
    // If the value has changed then it needs to be saved.
    //
    if(ucDevNum != g_sParameters.ucDeviceNumber)
    {
        //
        // Save the pending change.
        //
        g_sParameters.ucDeviceNumber = ucDevNum;

        //
        // Save the new a new ID.
        //
        ParamSave();
    }
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
// This function handles CAN Status API messages.
//
// \param ulID is the fully specified ID that was received.
// \param pucData is a buffer that holds the data that was received.
// \param ulMsgLen is the number of valid bytes of data in pucData buffer.
//
// This function is called when a CAN status API message is received.  If
// necessary, it will retrieve the information requested from the board and
// return it to the requestor.
//
// \return The function will return one if the command should be ACKed and zero
// if it should not be ACKed.
//
//*****************************************************************************
static unsigned long
StatusHandler(unsigned long ulID, unsigned char *pucData,
              unsigned long ulMsgLen)
{
    unsigned char pucMessage[8], ucData;
    unsigned long ulData;

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
            ulData = ControllerVoltageGet();

            //
            // Send a message back with the output voltage.
            //
            CANSendBroadcastMsg(ulID, (unsigned char *)&ulData, 2);

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
            ulData = ADCVBusGet();

            //
            // Send a message back with the input bus voltage.
            //
            CANSendBroadcastMsg(ulID, (unsigned char *)&ulData, 2);

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
            ulData = ControllerFaultsActive();

            //
            // Send a message back with the fault status.
            //
            CANSendBroadcastMsg(ulID, (unsigned char *)&ulData, 2);

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
            ulData = ADCCurrentGet();

            //
            // Send a message back with the motor current.
            //
            CANSendBroadcastMsg(ulID, (unsigned char *)&ulData, 2);

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
            ulData = ADCTemperatureGet();

            //
            // Send a message back with the temperature.
            //
            CANSendBroadcastMsg(ulID, (unsigned char *)&ulData, 2);

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
            ulData = ControllerPositionGet();

            //
            // Send a message back with the motor position.
            //
            CANSendBroadcastMsg(ulID, (unsigned char *)&ulData, 4);

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
            ulData = ControllerSpeedGet();

            //
            // Send a message back with the motor speed.
            //
            CANSendBroadcastMsg(ulID, (unsigned char *)&ulData, 4);

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
            ucData = LimitStatusGet(1);

            //
            // Send a message back with the limit switch status.
            //
            CANSendBroadcastMsg(ulID, &ucData, 1);

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
                CANSendBroadcastMsg(ulID, &g_ucFlags, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // Only POR can be cleared at this time.
                //
                if(pucData[0] & CAN_FLAGS_POR)
                {
                    g_ucFlags &= ~CAN_FLAGS_POR;
                }
            }

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
            ucData = ControllerControlModeGet();

            //
            // Send a message back with the control mode.
            //
            CANSendBroadcastMsg(ulID, &ucData, 1);

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
            ulData = (ControllerVoltageGet() * ADCVBusGet()) / 32768;

            //
            // Send a message back with the output voltage.
            //
            CANSendBroadcastMsg(ulID, (unsigned char *)&ulData, 2);

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
            ulData = ControllerStickyFaultsActive(1);

            //
            // Send a message back with the fault status.
            //
            CANSendBroadcastMsg(ulID, (unsigned char *)&ulData, 2);

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
            pucMessage[5] = g_ulCANStatus & 0x000000ff;

            //
            // Place CANERR in the 7th and 8th bytes.
            //
            *(unsigned short *)(pucMessage + 6) =
                HWREG(CAN0_BASE + CAN_O_ERR) & 0x0000ffff;

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
                    HWREG(CAN0_BASE + CAN_O_STS) = CAN_STS_LEC_NOEVENT;
                }
            }

            //
            // Send a message back with the fault counters, CANSTS, and CANERR
            // registers.
            //
            CANSendBroadcastMsg(ulID, pucMessage, 8);

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
            // This message should not be ACKed.
            //
            return(0);
        }
    }

    //
    // This message should be ACKed.
    //
    return(1);
}

//*****************************************************************************
//
// Handles the Periodic Status API calls.
//
//*****************************************************************************
static unsigned long
PStatusHandler(unsigned long ulID, unsigned char *pucData,
               unsigned long ulMsgLen)
{
    unsigned long ulIdx;
    unsigned short *pusData;

    //
    // Create local pointers of different types to the message data to avoid
    // later type casting.
    //
    pusData = (unsigned short *)pucData;

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
                CANSendBroadcastMsg(ulID,
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
                CANSendBroadcastMsg(ulID,
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
                CANSendBroadcastMsg(ulID,
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
                CANSendBroadcastMsg(ulID,
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
                CANSendBroadcastMsg(ulID, g_ppucPStatFormat[0], 8);
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
                CANSendBroadcastMsg(ulID, g_ppucPStatFormat[1], 8);
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
                CANSendBroadcastMsg(ulID, g_ppucPStatFormat[2], 8);
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
                CANSendBroadcastMsg(ulID, g_ppucPStatFormat[3], 8);
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
            // This message should not be ACKed.
            //
            return(0);
        }
    }

    //
    // This message should be ACKed.
    //
    return(1);
}

//*****************************************************************************
//
// This function handles CAN Voltage API messages.
//
// \param ulID is the fully specified ID that was received.
// \param pucData is a buffer that holds the data that was received.
// \param ulMsgLen is the number of valid bytes of data in pucData buffer.
//
// This function is called when a CAN voltage API message is received.  This
// function will parse the voltage command and call the appropriate motor
// controller function.
//
// \return The function will return one if the command should be ACKed and zero
// if it should not be ACKed.
//
//*****************************************************************************
static unsigned long
VoltageHandler(unsigned long ulID, unsigned char *pucData,
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
                g_sPendingUpdates.ucCurrentGroup = 0;
                g_sPendingUpdates.ucVoltageGroup = 0;
                g_sPendingUpdates.ucVCompGroup = 0;
                g_sPendingUpdates.ucPositionGroup = 0;
                g_sPendingUpdates.ucSpeedGroup = 0;
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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 2);
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
                        g_sPendingUpdates.sVoltage = psData[0];
                        g_sPendingUpdates.ucVoltageGroup = pucData[2];
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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 2);
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
// This function handles CAN Voltage Compensation API messages.
//
// \param ulID is the fully specified ID that was received.
// \param pucData is a buffer that holds the data that was received.
// \param ulMsgLen is the number of valid bytes of data in pucData buffer.
//
// This function is called when a CAN voltage compensation API message is
// received.  This function will parse the voltage compensation command and
// call the appropriate motor controller function.
//
// \return The function will return one if the command should be ACKed and zero
// if it should not be ACKed.
//
//*****************************************************************************
static unsigned long
VCompHandler(unsigned long ulID, unsigned char *pucData,
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
                g_sPendingUpdates.ucVoltageGroup = 0;
                g_sPendingUpdates.ucVCompGroup = 0;
                g_sPendingUpdates.ucCurrentGroup = 0;
                g_sPendingUpdates.ucSpeedGroup = 0;
                g_sPendingUpdates.ucPositionGroup = 0;
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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 2);
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
                        g_sPendingUpdates.sVComp = psData[0];
                        g_sPendingUpdates.ucVCompGroup = pucData[2];
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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 2);
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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 2);
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
// This function handles CAN Speed API messages.
//
// \param ulID is the fully specified ID that was received.
// \param pucData is a buffer that holds the data that was received.
// \param ulMsgLen is the number of valid bytes of data in pucData buffer.
//
// This function is called when a CAN speed API message is received.  This
// function will parse the speed command and call the appropriate motor
// controller function.
//
// \return The function will return one if the command should be ACKed and zero
// if it should not be ACKed.
//
//*****************************************************************************
static unsigned long
SpeedHandler(unsigned long ulID, unsigned char *pucData,
             unsigned long ulMsgLen)
{
    unsigned long ulValue, ulAck, ulAPI;

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
                // Enable speed mode.
                //
                CommandSpeedMode(true);

                //
                // Reset pending updates when switching modes.
                //
                g_sPendingUpdates.ucCurrentGroup = 0;
                g_sPendingUpdates.ucVoltageGroup = 0;
                g_sPendingUpdates.ucVCompGroup = 0;
                g_sPendingUpdates.ucPositionGroup = 0;
                g_sPendingUpdates.ucSpeedGroup = 0;
            }

            //
            // Ack this command.
            //
            ulAck = 1;

            break;
        }

        //
        // Disable speed control mode.
        //
        case LM_API_SPD_DIS:
        {
            //
            // Disable speed mode.
            //
            CommandSpeedMode(false);

            //
            // Ack this command.
            //
            ulAck = 1;

            break;
        }

        //
        // Set the Speed.
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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 4);
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
                    if((ulMsgLen <= 4) || (pucData[4] == 0))
                    {
                        //
                        // When read as an unsigned short the value in pucData
                        // is the rotational speed in revolution per second.
                        //
                        CommandSpeedSet(*(unsigned long *)pucData);
                    }
                    else
                    {
                        //
                        // Save the speed setting and the group.
                        //
                        g_sPendingUpdates.lSpeed = *(long *)pucData;
                        g_sPendingUpdates.ucSpeedGroup = pucData[4];
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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the proportional constant.
                //
                CommandSpeedPSet(*(unsigned long *)pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the integral constant.
                //
                CommandSpeedISet(*(unsigned long *)pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the differential constant.
                //
                CommandSpeedDSet(*(unsigned long *)pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // Set the speed reference.
                //
                CommandSpeedSrcSet(*pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            break;
        }
    }

    return(ulAck);
}

//*****************************************************************************
//
// This function handles CAN position API messages.
//
// \param ulID is the fully specified ID that was received.
// \param pucData is a buffer that holds the data that was received.
// \param ulMsgLen is the number of valid bytes of data in pucData buffer.
//
// This function is called when a CAN position API message is received.  This
// function will parse the position command and call the appropriate motor
// controller function.
//
// \return The function will return one if the command should be ACKed and zero
// if it should not be ACKed.
//
//*****************************************************************************
static unsigned long
PositionHandler(unsigned long ulID, unsigned char *pucData,
                unsigned long ulMsgLen)
{
    unsigned long ulValue, ulAck, ulAPI;

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
                // The 32 bit value in pucData holds the initial position.
                // The motor controller should set its output voltage to
                // 0V(neutral)
                //
                CommandPositionMode(true, *(long *)pucData);

                //
                // Reset pending updates when switching modes.
                //
                g_sPendingUpdates.ucCurrentGroup = 0;
                g_sPendingUpdates.ucVoltageGroup = 0;
                g_sPendingUpdates.ucVCompGroup = 0;
                g_sPendingUpdates.ucPositionGroup = 0;
                g_sPendingUpdates.ucSpeedGroup = 0;
            }

            //
            // Ack this command.
            //
            ulAck = 1;

            break;
        }

        //
        // Disable position control mode.
        //
        case LM_API_POS_DIS:
        {
            //
            // Switch out of position mode and back to the default mode.
            //
            CommandPositionMode(false, 0);

            //
            // Ack this command.
            //
            ulAck = 1;

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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 4);
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
                    if((ulMsgLen <= 4) || (pucData[4] == 0))
                    {
                        //
                        // Send the 32 bit position to move to.
                        //
                        CommandPositionSet(*(long *)pucData);
                    }
                    else
                    {
                        //
                        // Save the position setting and the group.
                        //
                        g_sPendingUpdates.lPosition = *(long *)pucData;
                        g_sPendingUpdates.ucPositionGroup = pucData[4];
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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the proportional constant.
                //
                CommandPositionPSet(*(long *)pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the integral constant.
                //
                CommandPositionISet(*(long *)pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 4);
            }
            else if(ulMsgLen == 4)
            {
                //
                // Set the differential constant.
                //
                CommandPositionDSet(*(long *)pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // Set the position reference.
                //
                CommandPositionSrcSet(*pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            break;
        }
    }

    return(ulAck);
}

//*****************************************************************************
//
// This function handles CAN current API messages.
//
// \param ulID is the fully specified ID that was received.
// \param pucData is a buffer that holds the data that was received.
// \param ulMsgLen is the number of valid bytes of data in pucData buffer.
//
// This function is called when a CAN current API message is received.  This
// function will parse the current command and call the appropriate motor
// controller function.
//
// \return The function will return one if the command should be ACKed and zero
// if it should not be ACKed.
//
//*****************************************************************************
static unsigned long
CurrentHandler(unsigned long ulID, unsigned char *pucData,
                  unsigned long ulMsgLen)
{
    unsigned long *pulData, ulValue, ulAck, ulAPI;
    short *psData;

    //
    // Create some local pointers to avoid later type casting.
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
        // Enable the current control mode.
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
                g_sPendingUpdates.ucCurrentGroup = 0;
                g_sPendingUpdates.ucVoltageGroup = 0;
                g_sPendingUpdates.ucVCompGroup = 0;
                g_sPendingUpdates.ucPositionGroup = 0;
                g_sPendingUpdates.ucSpeedGroup = 0;
            }

            //
            // Ack this command.
            //
            ulAck = 1;

            break;
        }

        //
        // Disable the current control mode.
        //
        case LM_API_ICTRL_DIS:
        {
            //
            // Disable current control mode and return to the default mode.
            //
            CommandCurrentMode(false);

            //
            // Ack this command.
            //
            ulAck = 1;

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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 2);
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
                    if((ulMsgLen <= 2) || (pucData[2] == 0))
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
                        g_sPendingUpdates.sCurrent = psData[0];
                        g_sPendingUpdates.ucCurrentGroup = pucData[2];
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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 4);
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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 4);
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
                CANSendBroadcastMsg(ulID, (unsigned char *)&ulValue, 4);
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

            break;
        }
    }

    return(ulAck);
}

//*****************************************************************************
//
// This function handles CAN configuration API messages.
//
// \param ulID is the fully specified ID that was received.
// \param pucData is a buffer that holds the data that was received.
// \param ulMsgLen is the number of valid bytes of data in pucData buffer.
//
// This function is called when a CAN configuration API message is received.
// This function will parse the configuration command and call the appropriate
// motor controller function.
//
// \return The function will return one if the command should be ACKed and zero
// if it should not be ACKed.
//
//*****************************************************************************
static unsigned long
ConfigHandler(unsigned long ulID, unsigned char * pucData,
              unsigned long ulMsgLen)
{
    unsigned long pulValue[2], ulAck;

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
                CANSendBroadcastMsg(ulID, (unsigned char *)pulValue, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // Set the number of brushes.
                //
                CommandNumBrushesSet(*pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)pulValue, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // Set the number of encoder lines.
                //
                CommandEncoderLinesSet(*(unsigned short *)pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)pulValue, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // Set the number of potentiometer turns.
                //
                CommandPotTurnsSet(*(unsigned short *)pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)pulValue, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // Set the brake/coast mode.
                //
                CommandBrakeCoastSet(*pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)pulValue, 1);
            }
            else if(ulMsgLen == 1)
            {
                //
                // Configure the position limit switches.
                //
                CommandPositionLimitMode(*pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)pulValue, 5);
            }
            else if(ulMsgLen == 5)
            {
                //
                // Set the forward position limit switch.
                //
                CommandPositionLimitForwardSet(*(unsigned long *)pucData,
                                               pucData[4]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)pulValue, 5);
            }
            else if(ulMsgLen == 5)
            {
                //
                // Set the reverse position limit switch.
                //
                CommandPositionLimitReverseSet(*(unsigned long *)pucData,
                                               pucData[4]);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)pulValue, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // Set the maximum output voltage.
                //
                CommandMaxVoltageSet(*(unsigned short *)pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

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
                CANSendBroadcastMsg(ulID, (unsigned char *)pulValue, 2);
            }
            else if(ulMsgLen == 2)
            {
                //
                // Set the fault time.
                //
                ControllerFaultTimeSet(*(unsigned short *)pucData);

                //
                // Ack this command.
                //
                ulAck = 1;
            }

            break;
        }
    }

    return(ulAck);
}

//*****************************************************************************
//
// This function handles CAN update API messages.
//
// \param ulID is the fully specified ID that was received.
// \param pucData is a buffer that holds the data that was received.
// \param ulMsgLen is the number of valid bytes of data in pucData buffer.
//
// This function is called when a CAN update API message is received.
// This function will parse the update commands and call the appropriate
// motor controller function.
//
// \return The function will return one if the command should be ACKed and zero
// if it should not be ACKed.
//
//*****************************************************************************
static unsigned long
UpdateHandler(unsigned long ulID, unsigned char * pucData,
              unsigned long ulMsgLen)
{
    unsigned long ulAck;
    unsigned char pucResponse[2];

    //
    // Do not send Ack by default.
    //
    ulAck = 0;

    //
    // Mask out the device number and see what the command is.
    //
    switch(ulID & (~CAN_MSGID_DEVNO_M))
    {
        //
        // Handle the firmware version command.
        //
        case LM_API_HWVER:
        {
            //
            // Respond with the device number and the hardware version.
            //
            pucResponse[0] = g_sParameters.ucDeviceNumber;
            pucResponse[1] = g_ucHardwareVersion;

            //
            // Send back the firmware version.
            //
            CANSendBroadcastMsg(ulID, pucResponse, 2);

            //
            // This message has been handled.
            //
            break;
        }

        //
        // Unknown command so just return.
        //
        default:
        {
            break;
        }
    }

    //
    // Return if the calling function should ack the command.
    //
    return(ulAck);
}

//*****************************************************************************
//
// This function configures the CAN hardware and the basic message objects so
// that the interface is ready to use once the application returns from this
// function.
//
// \return None.
//
//*****************************************************************************
void
CANIFInit(void)
{
    //
    // Default state is idle.
    //
    g_eCANState = STATE_IDLE;

    //
    // Configure CAN Pins
    //
    ROM_SysCtlPeripheralEnable(CAN0RX_PERIPH);
    ROM_GPIOPinTypeCAN(CAN0RX_PORT, CAN0RX_PIN | CAN0TX_PIN);

    //
    // Enable the CAN controllers.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

    //
    // Make sure all pending updates are canceled.
    //
    g_sPendingUpdates.ucCurrentGroup = 0;
    g_sPendingUpdates.ucVoltageGroup = 0;
    g_sPendingUpdates.ucVCompGroup = 0;
    g_sPendingUpdates.ucPositionGroup = 0;
    g_sPendingUpdates.ucSpeedGroup = 0;

    //
    // Reset the global flags.
    //
    g_ucFlags = CAN_FLAGS_POR;

    //
    // Reset the state of all the message object and the state of the CAN
    // module to a known state.
    //
    CANInit(CAN0_BASE);

    //
    // Configure the bit rate for the CAN device, the clock rate to the CAN
    // controller is based on the system clock for this class of device and the
    // bit rate is set to 1000000.
    //
    CANBitTimingSet(CAN0_BASE, (tCANBitClkParms *)&CANBitClkSettings);

    //
    // Take the CAN0 device out of INIT state.
    //
    CANEnable(CAN0_BASE);

    //
    // Enable interrups from CAN controller.
    //
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR);

    //
    // Set up the message object(s) that will receive messages on the CAN bus.
    //
    CANConfigureNetwork();

    //
    // Enable auto-retry on CAN transmit.
    //
    CANRetrySet(CAN0_BASE, true);

    //
    // If the initial device number is not zero then configure the CAN to use
    // the saved device id.
    //
    if(g_sParameters.ucDeviceNumber != 0)
    {
        //
        // Configure the message objects that are based on the device number.
        //
        CANDeviceNumSet(g_sParameters.ucDeviceNumber);

        //
        // Send an enumeration response message to the CAN bus to indicate that
        // the firmware has just started.
        //
        CANSendBroadcastMsg((CAN_MSGID_API_ENUMERATE |
                             g_sParameters.ucDeviceNumber), 0, 0);
    }

    //
    // Enable the CAN0 interrupts.
    //
    ROM_IntEnable(INT_CAN0);
}

//*****************************************************************************
//
// This function is called by external functions when a button press has been
// detected.  If the communication link is via CAN bus and the interface was in
// the Assignment state, then the CAN controller will accept the current
// device identifier assignment and store the value in the parameter block.
//
// \return None.
//
//*****************************************************************************
void
CANIFButtonPress(void)
{
    //
    // If the device was in the assignment state then save the new value.
    //
    if(g_eCANState == STATE_ASSIGNMENT)
    {
        //
        // Move to the assignment end state.
        //
        g_eCANState = STATE_ASSIGN_END;

        //
        // Trigger a fake CAN interrupt, during which the CAN interface will be
        // reconfigured for the new device ID.
        //
        HWREG(NVIC_SW_TRIG) = INT_CAN0 - 16;

        //
        // Blink the new device ID.
        //
        LEDBlinkID(g_ucDevNumPending);
    }
    else
    {
        //
        // Blink the device ID.
        //
        LEDBlinkID(g_sParameters.ucDeviceNumber);
    }
}

//*****************************************************************************
//
// The CAN interface interrupt handler.
//
// This function handles all interrupts from the can controller and parses out
// the various commands to their appropriate handlers.
//
// \return None.
//
//*****************************************************************************
void
CAN0IntHandler(void)
{
    unsigned long ulStat, ulAck, ulGroup;

    //
    // This resets the watchdog timeout for CAN messages.
    //
    ControllerLinkGood(LINK_TYPE_CAN);

    //
    // See if the enumeration delay has expired.
    //
    if(g_eCANState == STATE_ENUM_END)
    {
        //
        // Send out the enumeration data.
        //
        CANSendBroadcastMsg((CAN_MSGID_API_ENUMERATE |
                             g_sParameters.ucDeviceNumber), 0, 0);

        //
        // Return to the idle state.
        //
        g_eCANState = STATE_IDLE;
    }

    //
    // See if the assignment mode has ended.
    //
    if(g_eCANState == STATE_ASSIGN_END)
    {
        //
        // Set the configuration of the message objects.
        //
        CANDeviceNumSet(g_ucDevNumPending);

        //
        // Return to the idle state.
        //
        g_eCANState = STATE_IDLE;
    }

    //
    // See if periodic status messages need to be sent.
    //
    if(g_ulPStatFlags != 0)
    {
        //
        // Send out the first periodic status message if it needs to be sent.
        //
        if(g_ulPStatFlags & 1)
        {
            CANSendBroadcastMsg((LM_API_PSTAT_DATA_S0 |
                                 g_sParameters.ucDeviceNumber),
                                g_ppucPStatMessages[0],
                                g_pucPStatMessageLen[0]);
        }

        //
        // Send out the second periodic status message if it needs to be sent.
        //
        if(g_ulPStatFlags & 2)
        {
            CANSendBroadcastMsg((LM_API_PSTAT_DATA_S1 |
                                 g_sParameters.ucDeviceNumber),
                                g_ppucPStatMessages[1],
                                g_pucPStatMessageLen[1]);
        }

        //
        // Send out the third periodic status message if it needs to be sent.
        //
        if(g_ulPStatFlags & 4)
        {
            CANSendBroadcastMsg((LM_API_PSTAT_DATA_S2 |
                                 g_sParameters.ucDeviceNumber),
                                g_ppucPStatMessages[2],
                                g_pucPStatMessageLen[2]);
        }

        //
        // Send out the fourth periodic status message if it needs to be sent.
        //
        if(g_ulPStatFlags & 8)
        {
            CANSendBroadcastMsg((LM_API_PSTAT_DATA_S3 |
                                 g_sParameters.ucDeviceNumber),
                                g_ppucPStatMessages[3],
                                g_pucPStatMessageLen[3]);
        }

        //
        // Clear the periodic status message flag.
        //
        g_ulPStatFlags = 0;
    }

    //
    // Loop while there are interrupts being asserted by the CAN module.
    //
    while((ulStat = HWREG(CAN0_BASE + CAN_O_INT)) != 0)
    {
        //
        // Determine the cause of the interrupt.
        //
        switch(ulStat)
        {
            //
            // This message object is used to receive broadcast messages.
            //
            case MSG_OBJ_BCAST_RX_ID:
            {
                //
                // Retrieve the CAN message and process it.
                //
                CANIFMessageGet(MSG_OBJ_BCAST_RX_ID,
                                &g_psMsgObj[MSG_OBJ_BCAST_RX]);

                //
                // The API requested is determined based on the message
                // identifier that was received.
                //
                switch(g_psMsgObj[MSG_OBJ_BCAST_RX].ulMsgID)
                {
                    //
                    // A system halt request was received.
                    //
                    case CAN_MSGID_API_SYSHALT:
                    {
                        //
                        // Reset pending updates on a system halt.
                        //
                        g_sPendingUpdates.ucCurrentGroup = 0;
                        g_sPendingUpdates.ucVoltageGroup = 0;
                        g_sPendingUpdates.ucVCompGroup = 0;
                        g_sPendingUpdates.ucPositionGroup = 0;
                        g_sPendingUpdates.ucSpeedGroup = 0;

                        //
                        // Force the motor to neutral.
                        //
                        CommandForceNeutral();

                        //
                        // Set the halt flag so that further motion commands
                        // are ignored until a resume.
                        //
                        ControllerHaltSet();

                        break;
                    }

                    //
                    // A system resume request was received.
                    //
                    case CAN_MSGID_API_SYSRESUME:
                    {
                        //
                        // Clear the halt flag so that further motion commands
                        // can be received.
                        //
                        ControllerHaltClear();

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
                        SysCtlReset();

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
                        // Enumeration should be ignored if in assignment state
                        // or if there is no device number set.
                        //
                        if((g_eCANState == STATE_IDLE) &&
                           (g_sParameters.ucDeviceNumber != 0))
                        {
                            //
                            // Switch to the enumeration state to wait to send
                            // out the enumeration data.
                            //
                            g_eCANState = STATE_ENUMERATE;

                            //
                            // Wait 1ms * the current device number.
                            //
                            g_ulEventTick = g_ulTickCount +
                                ((UPDATES_PER_SECOND *
                                  g_sParameters.ucDeviceNumber) / 1000);
                        }

                        break;
                    }

                    //
                    // This was a request to assign a new device identifier.
                    //
                    case CAN_MSGID_API_DEVASSIGN:
                    {
                        //
                        // If an out of bounds device ID was specified then
                        // ignore the request.
                        //
                        if(g_psMsgObj[MSG_OBJ_BCAST_RX].pucMsgData[0] >
                           CAN_MSGID_DEVNO_M)
                        {
                        }
                        else if(g_psMsgObj[MSG_OBJ_BCAST_RX].pucMsgData[0] != 0)
                        {
                            //
                            // Save the pending address.
                            //
                            g_ucDevNumPending =
                                g_psMsgObj[MSG_OBJ_BCAST_RX].pucMsgData[0];

                            //
                            // This is pending until committed.
                            //
                            g_eCANState = STATE_ASSIGNMENT;

                            //
                            // Force the motor to neutral.
                            //
                            CommandForceNeutral();

                            //
                            // Set the tick that will trigger leaving
                            // assignment mode.
                            //
                            g_ulEventTick = g_ulTickCount +
                                (UPDATES_PER_SECOND * CAN_ASSIGN_WAIT_SECONDS);

                            //
                            // Let the world know that assignment state has
                            // started.
                            //
                            LEDAssignStart();
                        }
                        else
                        {
                            //
                            // Set the configuration of the message objects and
                            // the device identifier to zero immediately.
                            //
                            CANDeviceNumSet(0);

                            //
                            // Force the CAN state machine into the idle state.
                            //
                            g_eCANState = STATE_IDLE;
                        }

                        break;
                    }

                    //
                    // This was a request to start a firmware update.
                    //
                    case CAN_MSGID_API_UPDATE:
                    {
                        //
                        // Check if there is an ID to update and if it belongs
                        // to this board.
                        //
                        if((g_psMsgObj[MSG_OBJ_BCAST_RX].pucMsgData[0] !=
                            g_sParameters.ucDeviceNumber) ||
                           (g_psMsgObj[MSG_OBJ_BCAST_RX].ulMsgLen != 1))
                        {
                            break;
                        }

                        //
                        // Call the boot loader, this call will not return.
                        //
                        CallBootloader();
                    }

                    //
                    // Handle the sync commands.
                    //
                    case CAN_MSGID_API_SYNC:
                    {
                        //
                        // Retrieve the group identifier.
                        //
                        ulGroup = g_psMsgObj[MSG_OBJ_BCAST_RX].pucMsgData[0];

                        //
                        // If there are pending voltage updates then set the
                        // values now.
                        //
                        if((g_sPendingUpdates.ucVoltageGroup & ulGroup) != 0)
                        {
                            //
                            // Send the volatage on to the handler.
                            //
                            CommandVoltageSet(g_sPendingUpdates.sVoltage);

                            //
                            // Update is no longer pending.
                            //
                            g_sPendingUpdates.ucVoltageGroup = 0;
                        }

                        //
                        // If there is a pending voltage compensation update
                        // then set the value now.
                        //
                        if((g_sPendingUpdates.ucVCompGroup & ulGroup) != 0)
                        {
                            //
                            // Send the voltage on to the handler.
                            //
                            CommandVCompSet(g_sPendingUpdates.sVComp);

                            //
                            // Update is no longer pending.
                            //
                            g_sPendingUpdates.ucVCompGroup = 0;
                        }

                        //
                        // If there are pending current updates then set the
                        // values now.
                        //
                        if((g_sPendingUpdates.ucCurrentGroup & ulGroup) != 0)
                        {
                            //
                            // Send the current on to the handler.
                            //
                            CommandCurrentSet(g_sPendingUpdates.sCurrent);

                            //
                            // Update is no longer pending.
                            //
                            g_sPendingUpdates.ucCurrentGroup = 0;
                        }

                        //
                        // If there are pending speed updates then set the
                        // values now.
                        //
                        if((g_sPendingUpdates.ucSpeedGroup & ulGroup) != 0)
                        {
                            //
                            // Send the speed setting to the handler.
                            //
                            CommandSpeedSet(g_sPendingUpdates.lSpeed);

                            //
                            // Update is no longer pending.
                            //
                            g_sPendingUpdates.ucSpeedGroup = 0;
                        }

                        //
                        // If there are pending position updates then set the
                        // values now.
                        //
                        if((g_sPendingUpdates.ucPositionGroup & ulGroup) != 0)
                        {
                            //
                            // Send the speed setting to the handler.
                            //
                            CommandPositionSet(g_sPendingUpdates.lPosition);

                            //
                            // Update is no longer pending.
                            //
                            g_sPendingUpdates.ucPositionGroup = 0;
                        }

                        break;
                    }

                    //
                    // Nothing is done in response to a heartbeat command, this
                    // just causes the controller to hit the watchdog below.
                    //
                    case CAN_MSGID_API_HEARTBEAT:
                    {
                        break;
                    }

                    default:
                    {
                        break;
                    }
                }
                break;
            }

            //
            // The broadcast Transmit interrupt was received.  This is a stub
            // as this interrupt should not occur.
            //
            case MSG_OBJ_BCAST_TX_ID:
            {
                break;
            }

            //
            // This message object is used to receive motor control commands.
            //
            case MSG_OBJ_MC_RX_ID:
            case MSG_OBJ_VOLT_NO_ACK_ID:
            case MSG_OBJ_SPD_NO_ACK_ID:
            case MSG_OBJ_VCOMP_NO_ACK_ID:
            case MSG_OBJ_POS_NO_ACK_ID:
            case MSG_OBJ_ICTRL_NO_ACK_ID:
            {
                //
                // Retrieve the CAN message and process it.
                //
                CANIFMessageGet(ulStat, &g_psMsgObj[ulStat - 1]);

                switch(g_psMsgObj[ulStat - 1].ulMsgID & CAN_MSGID_API_CLASS_M)
                {
                    //
                    // Voltage motor control commands.
                    //
                    case CAN_API_MC_VOLTAGE:
                    {
                        //
                        // Call the Voltage handler.
                        //
                        ulAck =
                            VoltageHandler(g_psMsgObj[ulStat - 1].ulMsgID,
                                           g_psMsgObj[ulStat - 1].pucMsgData,
                                           g_psMsgObj[ulStat - 1].ulMsgLen);
                        break;
                    }

                    //
                    // Voltage compensation motor control commands.
                    //
                    case CAN_API_MC_VCOMP:
                    {
                        //
                        // Call the voltage compensation handler.
                        //
                        ulAck =
                            VCompHandler(g_psMsgObj[ulStat - 1].ulMsgID,
                                         g_psMsgObj[ulStat - 1].pucMsgData,
                                         g_psMsgObj[ulStat - 1].ulMsgLen);
                        break;
                    }

                    //
                    // Handle the Speed commands.
                    //
                    case CAN_API_MC_SPD:
                    {
                        //
                        // Call the speed handler.
                        //
                        ulAck =
                            SpeedHandler(g_psMsgObj[ulStat - 1].ulMsgID,
                                         g_psMsgObj[ulStat - 1].pucMsgData,
                                         g_psMsgObj[ulStat - 1].ulMsgLen);
                        break;
                    }

                    //
                    // Handle the position control class commands.
                    //
                    case CAN_API_MC_POS:
                    {
                        //
                        // Call the position command handler.
                        //
                        ulAck =
                            PositionHandler(g_psMsgObj[ulStat - 1].ulMsgID,
                                            g_psMsgObj[ulStat - 1].pucMsgData,
                                            g_psMsgObj[ulStat - 1].ulMsgLen);
                        break;
                    }

                    //
                    // Handle the Current control class commands.
                    //
                    case CAN_API_MC_ICTRL:
                    {
                        //
                        // Call the current command handler.
                        //
                        ulAck =
                            CurrentHandler(g_psMsgObj[ulStat - 1].ulMsgID,
                                           g_psMsgObj[ulStat - 1].pucMsgData,
                                           g_psMsgObj[ulStat - 1].ulMsgLen);
                        break;
                    }

                    //
                    // Handle the status commands.
                    //
                    case CAN_API_MC_STATUS:
                    {
                        //
                        // Call the status command handler.
                        //
                        ulAck =
                            StatusHandler(g_psMsgObj[ulStat - 1].ulMsgID,
                                          g_psMsgObj[ulStat - 1].pucMsgData,
                                          g_psMsgObj[ulStat - 1].ulMsgLen);
                        break;
                    }

                    //
                    // Handle the periodic status commands.
                    //
                    case CAN_API_MC_PSTAT:
                    {
                        //
                        // Call the periodic status message handler.
                        //
                        ulAck =
                            PStatusHandler(g_psMsgObj[ulStat - 1].ulMsgID,
                                           g_psMsgObj[ulStat - 1].pucMsgData,
                                           g_psMsgObj[ulStat - 1].ulMsgLen);
                        break;
                    }

                    //
                    // Handle the configuration commands.
                    //
                    case CAN_API_MC_CFG:
                    {
                        //
                        // Call the status command handler.
                        //
                        ulAck =
                            ConfigHandler(g_psMsgObj[ulStat - 1].ulMsgID,
                                          g_psMsgObj[ulStat - 1].pucMsgData,
                                          g_psMsgObj[ulStat - 1].ulMsgLen);
                        break;
                    }

                    //
                    // Unknown command class.
                    //
                    default:
                    {
                        ulAck = 0;
                        break;
                    }
                }

                //
                // See if an ACK needs to be sent.
                //
                if(ulAck != 0)
                {
                    //
                    // Send out the ACK.
                    //
                    CANSendBroadcastMsg((LM_API_ACK |
                                         (g_psMsgObj[ulStat - 1].ulMsgID &
                                          CAN_MSGID_DEVNO_M)), 0, 0);
                }

                break;
            }

            //
            // Handle the update class commands.
            //
            case MSG_OBJ_UPD_RX_ID:
            {
                //
                // Retrieve the CAN message and process it.
                //
                CANIFMessageGet(MSG_OBJ_UPD_RX_ID,
                                &g_psMsgObj[MSG_OBJ_UPD_RX]);

                //
                // Handle this device-specific command.
                //
                ulAck = UpdateHandler(g_psMsgObj[MSG_OBJ_UPD_RX].ulMsgID,
                                      g_psMsgObj[MSG_OBJ_UPD_RX].pucMsgData,
                                      g_psMsgObj[MSG_OBJ_UPD_RX].ulMsgLen);

                //
                // Send back an ACK if required.
                //
                if(ulAck != 0)
                {
                    CANSendBroadcastMsg(LM_API_ACK |
                                        g_sParameters.ucDeviceNumber, 0, 0);
                }

                break;
            }

            //
            // The interrupt was a status interrupt.
            //
            case CAN_INT_INTID_STATUS:
            {
                //
                // Read the CAN Status register and save the value.  Reading
                // this register clears the interrupt.
                //
                g_ulCANStatus = HWREG(CAN0_BASE + CAN_O_STS);

                //
                // If the cause of the status interrupt was the CAN controller
                // going into a Bus-Off state, start the recovery sequence.
                // Otherwise, do nothing.
                //
                if((g_ulCANStatus & CAN_STS_BOFF))
                {
                    //
                    // Write a zero to INIT bit fo CANCTL register to
                    // initiate a bus-off recovery.
                    //
                    HWREG(CAN0_BASE + CAN_O_CTL) = CAN_CTL_EIE | CAN_CTL_IE;

                    //
                    // Indicate that the controller went bus-off by sending a
                    // COMM fault.
                    //
                    ControllerFaultSignal(LM_FAULT_COMM);
                }

                break;
            }

            default:
            {
                break;
            }
        }
    }

    //
    // Delay the watchdog interrupt since a CAN command was received.
    //
    ControllerWatchdog(LINK_TYPE_CAN);
}

//*****************************************************************************
//
// This performs any timing related operations needed by the CAN interface.
//
// When the CAN controller is in STATE_ASSIGNMENT, this allows the controller
// to time out after CAN_ASSIGN_WAIT_SECONDS seconds and resume normal
// operation.  When the CAN controller is in STATE_ENUMERATE state this allows
// the controller to wait the specified time before responding to the
// enumeration request to prevent all controllers on a network from responding
// at once.
//
//*****************************************************************************
void
CANIFTick(void)
{
    unsigned short usVout, usVbus, usImotor, usTamb, usCANErr;
    unsigned long ulMsg, ulIdx, ulPos, ulSpeed, ulFlags;

    //
    // Increment the tick count.
    //
    g_ulTickCount++;

    //
    // Assignment state is handled in this case.
    //
    if(g_eCANState == STATE_ASSIGNMENT)
    {
        //
        // Wait for the correct tick time.
        //
        if(g_ulEventTick == g_ulTickCount)
        {
            //
            // Return to the idle state.
            //
            g_eCANState = STATE_IDLE;

            //
            // If the pending change was not accepted and was the the same as
            // it was before then set the device number to 0 and accept it.
            //
            if(g_ucDevNumPending == g_sParameters.ucDeviceNumber)
            {
                //
                // Reset the pending device number.
                //
                g_ucDevNumPending = 0;

                //
                // Move to the assignment end state.
                //
                g_eCANState = STATE_ASSIGN_END;

                //
                // Trigger a fake CAN interrupt, during which the CAN interface
                // will be reconfigured for the new device ID.
                //
                HWREG(NVIC_SW_TRIG) = INT_CAN0 - 16;
            }

            //
            // Indicate that the CAN controller has left assignment mode.
            //
            LEDAssignStop();
        }
    }

    //
    // Enumeration state is handled in this case.
    //
    else if(g_eCANState == STATE_ENUMERATE)
    {
        //
        // Wait for the correct tick time.
        //
        if(g_ulEventTick == g_ulTickCount)
        {
            //
            // Move to the enumeration end state.
            //
            g_eCANState = STATE_ENUM_END;

            //
            // Trigger a fake CAN interrupt, during which the enumeration
            // response will be sent.
            //
            HWREG(NVIC_SW_TRIG) = INT_CAN0 - 16;
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
            usCANErr = HWREG(CAN0_BASE + CAN_O_ERR);
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
                    g_ppucPStatMessages[ulMsg][ulIdx] = g_ulCANStatus & 0xff;
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
        // Trigger a fake CAN interrupt, during which the periodic status
        // messages will be sent.
        //
        HWREG(NVIC_SW_TRIG) = INT_CAN0 - 16;
    }
}
