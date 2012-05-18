//*****************************************************************************
//
// can_if.c - Functions that are used to interact with the CAN controller.
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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/watchdog.h"
#include "shared/can_proto.h"
#include "can_if.h"
#include "constants.h"
#include "controller.h"
#include "message.h"
#include "param.h"
#include "pins.h"
#include "uart_if.h"

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
// A set of flags to indicate additional actions that need to be taken during a
// CAN interrupt.
//
//*****************************************************************************
static unsigned long g_ulCANFlags;
#define CAN_FLAG_ENUM           1
#define CAN_FLAG_ASSIGN         2
#define CAN_FLAG_PSTATUS        3

//*****************************************************************************
//
// The new CAN ID to be used after an assignment.
//
//*****************************************************************************
static unsigned long g_ulCANNewID;

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
#define MSG_OBJ_BRIDGE_QUERY    11
#define MSG_OBJ_BRIDGE_QUERY_ID (MSG_OBJ_BRIDGE_QUERY + 1)
#define MSG_OBJ_BRIDGE_VER      12
#define MSG_OBJ_BRIDGE_VER_ID   (MSG_OBJ_BRIDGE_VER + 1)
#define MSG_OBJ_BRIDGE_TX       13
#define MSG_OBJ_BRIDGE_TX_ID    (MSG_OBJ_BRIDGE_TX + 1)
#define MSG_OBJ_BRIDGE_RX0      14
#define MSG_OBJ_BRIDGE_RX0_ID   (MSG_OBJ_BRIDGE_RX0 + 1)
#define MSG_OBJ_BRIDGE_RX1      15
#define MSG_OBJ_BRIDGE_RX1_ID   (MSG_OBJ_BRIDGE_RX1 + 1)
#define MSG_OBJ_BRIDGE_RX2      16
#define MSG_OBJ_BRIDGE_RX2_ID   (MSG_OBJ_BRIDGE_RX2 + 1)
#define MSG_OBJ_BRIDGE_RX3      17
#define MSG_OBJ_BRIDGE_RX3_ID   (MSG_OBJ_BRIDGE_RX3 + 1)
#define MSG_OBJ_BRIDGE_RX4      18
#define MSG_OBJ_BRIDGE_RX4_ID   (MSG_OBJ_BRIDGE_RX4 + 1)
#define MSG_OBJ_BRIDGE_RX5      19
#define MSG_OBJ_BRIDGE_RX5_ID   (MSG_OBJ_BRIDGE_RX5 + 1)
#define MSG_OBJ_BRIDGE_RX6      20
#define MSG_OBJ_BRIDGE_RX6_ID   (MSG_OBJ_BRIDGE_RX6 + 1)
#define MSG_OBJ_BRIDGE_RX7      21
#define MSG_OBJ_BRIDGE_RX7_ID   (MSG_OBJ_BRIDGE_RX7 + 1)
#define MSG_OBJ_NUM_OBJECTS     22

//*****************************************************************************
//
// This holds the information for the data send message object that is used
// to send traffic on the CAN network.
//
//*****************************************************************************
static tCANMsgObject g_psMsgObject[MSG_OBJ_NUM_OBJECTS];

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
// This is the buffer for data received by the bridged query message object.
//
//*****************************************************************************
static unsigned char g_pucBridgeQueryData[8];

//*****************************************************************************
//
// This is the buffer for data received by the bridged firmware version message
// object.
//
//*****************************************************************************
static unsigned char g_pucBridgeVersionData[4];

//*****************************************************************************
//
// This is the buffer for data received by the bridge RX message objects.
//
//*****************************************************************************
static unsigned long g_pulBridgeRXData[16];

//*****************************************************************************
//
// This contains the latest value of the CAN Status register.  It is updated
// every CAN interrupt.
//
//*****************************************************************************
static unsigned long g_ulCANStatus = 0;

//*****************************************************************************
//
// A local copy of CANMessageClear that does not use paced reads/writes since
// they are not needed on the LM3S2616.
//
//*****************************************************************************
static void
CANIFMessageClear(unsigned long ulObjID)
{
    //
    // Wait for busy bit to clear
    //
    while(HWREG(CAN0_BASE + CAN_O_IF1CRQ) & CAN_IF1CRQ_BUSY)
    {
    }

    //
    // Clear the message value bit in the arbitration register.  This indicates
    // the message is not valid.
    //
    HWREG(CAN0_BASE + CAN_O_IF1CMSK) = CAN_IF1CMSK_WRNRD | CAN_IF1CMSK_ARB;
    HWREG(CAN0_BASE + CAN_O_IF1ARB1) = 0;
    HWREG(CAN0_BASE + CAN_O_IF1ARB2) = 0;

    //
    // Initiate programming the message object
    //
    HWREG(CAN0_BASE + CAN_O_IF1CRQ) = ulObjID & CAN_IF1CRQ_MNUM_M;
}

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
    // Loop over the data buffer.
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
        // Store the byte.
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
    // Loop over the data buffer.
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
    unsigned long ulIdx;

    //
    // This message object will be used to send broadcast messages on the
    // CAN network.
    //
    g_psMsgObject[MSG_OBJ_BCAST_TX].ulMsgID = 0;
    g_psMsgObject[MSG_OBJ_BCAST_TX].ulMsgIDMask = 0;
    g_psMsgObject[MSG_OBJ_BCAST_TX].ulFlags = MSG_OBJ_EXTENDED_ID;
    g_psMsgObject[MSG_OBJ_BCAST_TX].ulMsgLen = 0;
    g_psMsgObject[MSG_OBJ_BCAST_TX].pucMsgData = (unsigned char *)0xffffffff;

    //
    // This message object will be used to receive broadcast traffic on the
    // CAN network.
    //
    g_psMsgObject[MSG_OBJ_BCAST_RX].ulMsgID = 0;

    //
    // Any API targeted at a devno of 0, manufacturer of 0, and device type
    // of 0.
    //
    g_psMsgObject[MSG_OBJ_BCAST_RX].ulMsgIDMask =
        CAN_MSGID_DTYPE_M | CAN_MSGID_MFR_M | CAN_MSGID_DEVNO_M;

    //
    // Enable extended identifiers, interrupt and using extended message id
    // filtering.
    //
    g_psMsgObject[MSG_OBJ_BCAST_RX].ulFlags =
        (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID | MSG_OBJ_USE_ID_FILTER |
         MSG_OBJ_USE_EXT_FILTER);

    //
    // No data will be read automatically, it will be read when it is received.
    //
    g_psMsgObject[MSG_OBJ_BCAST_RX].ulMsgLen = 8;
    g_psMsgObject[MSG_OBJ_BCAST_RX].pucMsgData = g_pucBcastData;

    //
    // Configure the broadcast receive message object.
    //
    CANIFMessageSet(MSG_OBJ_BCAST_RX_ID, &g_psMsgObject[MSG_OBJ_BCAST_RX],
                    MSG_OBJ_TYPE_RX);

    //
    // This message object will be used to send bridged messages on the CAN
    // network.
    //
    g_psMsgObject[MSG_OBJ_BRIDGE_TX].ulMsgID = 0;
    g_psMsgObject[MSG_OBJ_BRIDGE_TX].ulMsgIDMask = 0;
    g_psMsgObject[MSG_OBJ_BRIDGE_TX].ulFlags = MSG_OBJ_EXTENDED_ID;
    g_psMsgObject[MSG_OBJ_BRIDGE_TX].ulMsgLen = 0;
    g_psMsgObject[MSG_OBJ_BRIDGE_TX].pucMsgData = (unsigned char *)0xffffffff;

    //
    // This message object will be used to send bridged device query messages
    // on the CAN network.
    //
    g_psMsgObject[MSG_OBJ_BRIDGE_QUERY].ulMsgID = 0;
    g_psMsgObject[MSG_OBJ_BRIDGE_QUERY].ulMsgIDMask = 0;
    g_psMsgObject[MSG_OBJ_BRIDGE_QUERY].ulFlags =
        MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID;
    g_psMsgObject[MSG_OBJ_BRIDGE_QUERY].ulMsgLen = 8;
    g_psMsgObject[MSG_OBJ_BRIDGE_QUERY].pucMsgData = g_pucBridgeQueryData;

    //
    // This message object will be used to send bridged firmware version
    // messages on the CAN network.
    //
    g_psMsgObject[MSG_OBJ_BRIDGE_VER].ulMsgID = 0;
    g_psMsgObject[MSG_OBJ_BRIDGE_VER].ulMsgIDMask = 0;
    g_psMsgObject[MSG_OBJ_BRIDGE_VER].ulFlags =
        MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID;
    g_psMsgObject[MSG_OBJ_BRIDGE_VER].ulMsgLen = 4;
    g_psMsgObject[MSG_OBJ_BRIDGE_VER].pucMsgData = g_pucBridgeVersionData;

    //
    // Setup the message FIFO used to receive bridge messages from the CAN
    // network.
    //
    for(ulIdx = 0; ulIdx < 8; ulIdx++)
    {
        //
        // Configure this message object.
        //
        g_psMsgObject[MSG_OBJ_BRIDGE_RX0 + ulIdx].ulMsgID = 0;
        g_psMsgObject[MSG_OBJ_BRIDGE_RX0 + ulIdx].ulMsgIDMask = 0;
        g_psMsgObject[MSG_OBJ_BRIDGE_RX0 + ulIdx].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);
        g_psMsgObject[MSG_OBJ_BRIDGE_RX0 + ulIdx].ulMsgLen = 8;
        g_psMsgObject[MSG_OBJ_BRIDGE_RX0 + ulIdx].pucMsgData =
            (unsigned char *)(g_pulBridgeRXData + (ulIdx * 2));

        //
        // If this is not the last message object in the FIFO, then set the
        // FIFO indicator.
        //
        if(ulIdx != 7)
        {
            g_psMsgObject[MSG_OBJ_BRIDGE_RX0 + ulIdx].ulFlags |= MSG_OBJ_FIFO;
        }

        //
        // Configure this bridge receive message object.
        //
        CANIFMessageSet(MSG_OBJ_BRIDGE_RX0_ID + ulIdx,
                        &(g_psMsgObject[MSG_OBJ_BRIDGE_RX0 + ulIdx]),
                        MSG_OBJ_TYPE_RX);
    }
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
#if MSG_OBJ_BCAST_TX < 16
        ulStatus = (HWREG(CAN0_BASE + CAN_O_TXRQ1) & (1 << MSG_OBJ_BCAST_TX));
#else
        ulStatus = (HWREG(CAN0_BASE + CAN_O_TXRQ2) &
                    (1 << (MSG_OBJ_BCAST_TX - 16)));
#endif

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
        g_psMsgObject[MSG_OBJ_BCAST_TX].pucMsgData = pucData;
        g_psMsgObject[MSG_OBJ_BCAST_TX].ulMsgLen = ulSize;
        g_psMsgObject[MSG_OBJ_BCAST_TX].ulMsgID = ulId;
        CANIFMessageSet(MSG_OBJ_BCAST_TX_ID, &g_psMsgObject[MSG_OBJ_BCAST_TX],
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
        g_psMsgObject[MSG_OBJ_MC_RX].ulMsgID =
            CAN_MSGID_MFR_LM | CAN_MSGID_DTYPE_MOTOR | ucDevNum;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // and the device type.
        //
        g_psMsgObject[MSG_OBJ_MC_RX].ulMsgIDMask =
            CAN_MSGID_DEVNO_M | CAN_MSGID_MFR_M | CAN_MSGID_DTYPE_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObject[MSG_OBJ_MC_RX].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObject[MSG_OBJ_MC_RX].ulMsgLen = 8;
        g_psMsgObject[MSG_OBJ_MC_RX].pucMsgData = g_pucMCData;

        //
        // Configure the motor control message receive message object.
        //
        CANIFMessageSet(MSG_OBJ_MC_RX_ID, &g_psMsgObject[MSG_OBJ_MC_RX],
                        MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive no-ack voltage set
        // message.
        //
        g_psMsgObject[MSG_OBJ_VOLT_NO_ACK].ulMsgID =
            ucDevNum | LM_API_VOLT | LM_API_VOLT_SET_NO_ACK;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // device type, API class and API (the full message ID).
        //
        g_psMsgObject[MSG_OBJ_VOLT_NO_ACK].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObject[MSG_OBJ_VOLT_NO_ACK].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObject[MSG_OBJ_VOLT_NO_ACK].ulMsgLen = 8;
        g_psMsgObject[MSG_OBJ_VOLT_NO_ACK].pucMsgData = g_pucMCData;

        //
        // Configure the no-ack voltage set message object.
        //
        CANIFMessageSet(MSG_OBJ_VOLT_NO_ACK_ID,
                        &g_psMsgObject[MSG_OBJ_VOLT_NO_ACK], MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive no-ack speed
        // set message.
        //
        g_psMsgObject[MSG_OBJ_SPD_NO_ACK].ulMsgID =
            ucDevNum | LM_API_VOLT | LM_API_SPD_SET_NO_ACK;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // device type, API class and API (the full message ID).
        //
        g_psMsgObject[MSG_OBJ_SPD_NO_ACK].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObject[MSG_OBJ_SPD_NO_ACK].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObject[MSG_OBJ_SPD_NO_ACK].ulMsgLen = 8;
        g_psMsgObject[MSG_OBJ_SPD_NO_ACK].pucMsgData = g_pucMCData;

        //
        // Configure the no-ack speed set message object.
        //
        CANIFMessageSet(MSG_OBJ_SPD_NO_ACK_ID,
                        &g_psMsgObject[MSG_OBJ_SPD_NO_ACK], MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive no-ack voltage
        // compensation set message.
        //
        g_psMsgObject[MSG_OBJ_VCOMP_NO_ACK].ulMsgID =
            ucDevNum | LM_API_VOLT | LM_API_VCOMP_SET_NO_ACK;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // device type, API class and API (the full message ID).
        //
        g_psMsgObject[MSG_OBJ_VCOMP_NO_ACK].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObject[MSG_OBJ_VCOMP_NO_ACK].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObject[MSG_OBJ_VCOMP_NO_ACK].ulMsgLen = 8;
        g_psMsgObject[MSG_OBJ_VCOMP_NO_ACK].pucMsgData = g_pucMCData;

        //
        // Configure the no-ack voltage compensation set message object.
        //
        CANIFMessageSet(MSG_OBJ_VCOMP_NO_ACK_ID,
                        &g_psMsgObject[MSG_OBJ_VCOMP_NO_ACK], MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive no-ack position
        // set message.
        //
        g_psMsgObject[MSG_OBJ_POS_NO_ACK].ulMsgID =
            ucDevNum | LM_API_VOLT | LM_API_POS_SET_NO_ACK;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // device type, API class and API (the full message ID).
        //
        g_psMsgObject[MSG_OBJ_POS_NO_ACK].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObject[MSG_OBJ_POS_NO_ACK].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObject[MSG_OBJ_POS_NO_ACK].ulMsgLen = 8;
        g_psMsgObject[MSG_OBJ_POS_NO_ACK].pucMsgData = g_pucMCData;

        //
        // Configure the no-ack voltage compensation set message object.
        //
        CANIFMessageSet(MSG_OBJ_POS_NO_ACK_ID,
                        &g_psMsgObject[MSG_OBJ_POS_NO_ACK], MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive no-ack current
        // conrol set message.
        //
        g_psMsgObject[MSG_OBJ_ICTRL_NO_ACK].ulMsgID =
            ucDevNum | LM_API_VOLT | LM_API_ICTRL_SET_NO_ACK;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // device type, API class and API (the full message ID).
        //
        g_psMsgObject[MSG_OBJ_ICTRL_NO_ACK].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObject[MSG_OBJ_ICTRL_NO_ACK].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObject[MSG_OBJ_ICTRL_NO_ACK].ulMsgLen = 8;
        g_psMsgObject[MSG_OBJ_ICTRL_NO_ACK].pucMsgData = g_pucMCData;

        //
        // Configure the no-ack voltage compensation set message object.
        //
        CANIFMessageSet(MSG_OBJ_ICTRL_NO_ACK_ID,
                        &g_psMsgObject[MSG_OBJ_ICTRL_NO_ACK], MSG_OBJ_TYPE_RX);

        //
        // This message object will be used to receive update related
        // messages on the CAN network.
        //
        g_psMsgObject[MSG_OBJ_UPD_RX].ulMsgID = LM_API_UPD | ucDevNum;

        //
        // Set the filter mask to look at the device identifier, manufacturer,
        // and the device type.
        //
        g_psMsgObject[MSG_OBJ_UPD_RX].ulMsgIDMask =
            CAN_MSGID_DEVNO_M | CAN_MSGID_MFR_M | CAN_MSGID_DTYPE_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObject[MSG_OBJ_UPD_RX].ulFlags =
            (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID |
             MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER);

        //
        // This just initializes the data pointer and does nothing else.
        //
        g_psMsgObject[MSG_OBJ_UPD_RX].ulMsgLen = 8;
        g_psMsgObject[MSG_OBJ_UPD_RX].pucMsgData = g_pucUPDData;

        //
        // Configure the motor control message receive message object.
        //
        CANIFMessageSet(MSG_OBJ_UPD_RX_ID, &g_psMsgObject[MSG_OBJ_UPD_RX],
                        MSG_OBJ_TYPE_RX);

        //
        // Configure the auto-responding device indentification data message
        // object.
        //
        g_psMsgObject[MSG_OBJ_DEV_QUERY].ulMsgID = (CAN_MSGID_API_DEVQUERY |
                                                    ucDevNum);

        //
        // Only look at the API and Device identifier.
        //
        g_psMsgObject[MSG_OBJ_DEV_QUERY].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObject[MSG_OBJ_DEV_QUERY].ulFlags = (MSG_OBJ_EXTENDED_ID |
                                                    MSG_OBJ_USE_ID_FILTER |
                                                    MSG_OBJ_USE_EXT_FILTER);

        //
        // The identification data has 8 bytes of data so initialize the values.
        //
        g_psMsgObject[MSG_OBJ_DEV_QUERY].ulMsgLen = 8;
        g_psMsgObject[MSG_OBJ_DEV_QUERY].pucMsgData =
            (unsigned char *)g_pucEnumData;

        //
        // Send the configuration to the device identification message object.
        //
        CANIFMessageSet(MSG_OBJ_DEV_QUERY_ID,
                        &g_psMsgObject[MSG_OBJ_DEV_QUERY],
                        MSG_OBJ_TYPE_RXTX_REMOTE);

        //
        // Configure the auto-responding firmware version message object to
        // look for a fully specified message identifier.
        //
        g_psMsgObject[MSG_OBJ_FIRM_VER].ulMsgID =
            CAN_MSGID_API_FIRMVER | ucDevNum;
        g_psMsgObject[MSG_OBJ_FIRM_VER].ulMsgIDMask = CAN_MSGID_FULL_M;

        //
        // Enable filtering based on the mask.
        //
        g_psMsgObject[MSG_OBJ_FIRM_VER].ulFlags = (MSG_OBJ_EXTENDED_ID |
                                                   MSG_OBJ_USE_ID_FILTER |
                                                   MSG_OBJ_USE_EXT_FILTER);

        //
        // Set the firmware version response to 4 bytes long and initialize
        // the data to the value in g_ulFirmwareVerion.
        //
        g_psMsgObject[MSG_OBJ_FIRM_VER].ulMsgLen = 4;
        g_psMsgObject[MSG_OBJ_FIRM_VER].pucMsgData =
            (unsigned char *)&g_ulFirmwareVersion;

        //
        // Send the configuration to the Firmware message object.
        //
        CANIFMessageSet(MSG_OBJ_FIRM_VER_ID, &g_psMsgObject[MSG_OBJ_FIRM_VER],
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
        // Enable interrups from CAN controller.
        //
        CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR);
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
    unsigned long ulStat, ulAck, *pulResponse;

    //
    // Create a local pointer of a different type to avoid later type casting.
    //
    pulResponse = (unsigned long *)g_pucResponse;

    //
    // See if the enumeration delay has expired.
    //
    if(HWREGBITW(&g_ulCANFlags, CAN_FLAG_ENUM) == 1)
    {
        //
        // Send out the enumeration data.
        //
        CANSendBroadcastMsg((CAN_MSGID_API_ENUMERATE |
                             g_sParameters.ucDeviceNumber), 0, 0);

        //
        // Clear the enumeration flag.
        //
        HWREGBITW(&g_ulCANFlags, CAN_FLAG_ENUM) = 0;
    }

    //
    // See if the device ID has changed.
    //
    if(HWREGBITW(&g_ulCANFlags, CAN_FLAG_ASSIGN) == 1)
    {
        //
        // Set the configuration of the message objects.
        //
        CANDeviceNumSet(g_ulCANNewID);

        //
        // Clear the assign flag.
        //
        HWREGBITW(&g_ulCANFlags, CAN_FLAG_ASSIGN) = 0;
    }

    //
    // See if periodic status messages need to be sent.
    //
    if(HWREGBITW(&g_ulCANFlags, CAN_FLAG_PSTATUS) == 1)
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
        HWREGBITW(&g_ulCANFlags, CAN_FLAG_PSTATUS) = 0;
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
                // This is a valid CAN message received and not a bridge
                // message.
                //
                ControllerLinkGood(LINK_TYPE_CAN);

                //
                // Retrieve the CAN message and process it.
                //
                CANIFMessageGet(ulStat, &g_psMsgObject[ulStat - 1]);

                //
                // Handle this system command.
                //
                MessageCommandHandler(g_psMsgObject[ulStat - 1].ulMsgID,
                                      g_psMsgObject[ulStat - 1].pucMsgData,
                                      g_psMsgObject[ulStat - 1].ulMsgLen);

                //
                // Send back the response if one was generated.
                //
                if(g_ulResponseLength != 0)
                {
                    CANSendBroadcastMsg(pulResponse[0], g_pucResponse + 4,
                                        g_ulResponseLength - 4);
                }

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
                // This is a valid CAN message received and not a bridge
                // message.
                //
                ControllerLinkGood(LINK_TYPE_CAN);

                //
                // Retrieve the CAN message and process it.
                //
                CANIFMessageGet(ulStat, &g_psMsgObject[ulStat - 1]);

                //
                // Handle this device-specific command.
                //
                ulAck =
                    MessageCommandHandler(g_psMsgObject[ulStat - 1].ulMsgID,
                                          g_psMsgObject[ulStat - 1].pucMsgData,
                                          g_psMsgObject[ulStat - 1].ulMsgLen);

                //
                // Send back the response if one was generated.
                //
                if(g_ulResponseLength != 0)
                {
                    CANSendBroadcastMsg(pulResponse[0], g_pucResponse + 4,
                                        g_ulResponseLength - 4);
                }

                //
                // Send back an ACK if required.
                //
                if(ulAck == 1)
                {
                    CANSendBroadcastMsg(LM_API_ACK |
                                        g_sParameters.ucDeviceNumber, 0, 0);
                }

                break;
            }

            //
            // Handle the update class commands.
            //
            case MSG_OBJ_UPD_RX_ID:
            {
                //
                // This is a valid CAN message received and not a bridge
                // message.
                //
                ControllerLinkGood(LINK_TYPE_CAN);

                //
                // Retrieve the CAN message and process it.
                //
                CANIFMessageGet(ulStat, &g_psMsgObject[ulStat - 1]);

                //
                // Handle this device-specific command.
                //
                ulAck =
                    MessageUpdateHandler(g_psMsgObject[ulStat - 1].ulMsgID,
                                         g_psMsgObject[ulStat - 1].pucMsgData,
                                         g_psMsgObject[ulStat - 1].ulMsgLen);

                //
                // Send back the response if one was generated.
                //
                if(g_ulResponseLength != 0)
                {
                    CANSendBroadcastMsg(pulResponse[0], g_pucResponse + 4,
                                        g_ulResponseLength - 4);
                }

                //
                // Send back an ACK if required.
                //
                if(ulAck == 1)
                {
                    CANSendBroadcastMsg(LM_API_ACK |
                                        g_sParameters.ucDeviceNumber, 0, 0);
                }

                break;
            }

            //
            // These message objects are used to receive other traffic that
            // should be bridged to the UART.
            //
            case MSG_OBJ_BRIDGE_QUERY_ID:
            case MSG_OBJ_BRIDGE_VER_ID:
            case MSG_OBJ_BRIDGE_RX0_ID:
            case MSG_OBJ_BRIDGE_RX1_ID:
            case MSG_OBJ_BRIDGE_RX2_ID:
            case MSG_OBJ_BRIDGE_RX3_ID:
            case MSG_OBJ_BRIDGE_RX4_ID:
            case MSG_OBJ_BRIDGE_RX5_ID:
            case MSG_OBJ_BRIDGE_RX6_ID:
            case MSG_OBJ_BRIDGE_RX7_ID:
            {
                //
                // Retrieve the CAN message.
                //
                CANIFMessageGet(ulStat, &g_psMsgObject[ulStat - 1]);

                //
                // Send this message out on the UART.
                //
                UARTIFSendMessage(g_psMsgObject[ulStat - 1].ulMsgID,
                                  g_psMsgObject[ulStat - 1].pucMsgData,
                                  g_psMsgObject[ulStat - 1].ulMsgLen);

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
                    // Write a zero to INIT bit of CANCTL register to initiate
                    // a bus-off recovery.
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
    // Tell the controller that CAN activity was detected.
    //
    ControllerWatchdog(LINK_TYPE_CAN);
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
    // Configure the CAN pins.
    //
#if CAN_RX_PORT == CAN_TX_PORT
    ROM_GPIOPinTypeCAN(CAN_RX_PORT, CAN_RX_PIN | CAN_TX_PIN);
#else
    ROM_GPIOPinTypeCAN(CAN_TX_PORT, CAN_TX_PIN);
    ROM_GPIOPinTypeCAN(CAN_RX_PORT, CAN_RX_PIN);
#endif

    //
    // Enable the CAN controllers.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

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
    // Enable interrupts from CAN controller.
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
// Sets a new device ID into the CAN interface.
//
//*****************************************************************************
void
CANIFSetID(unsigned long ulID)
{
    //
    // Save the new ID.
    //
    g_ulCANNewID = ulID;

    //
    // Set the assign flag.
    //
    HWREGBITW(&g_ulCANFlags, CAN_FLAG_ASSIGN) = 1;

    //
    // Trigger a fake CAN interrupt, during which the CAN interface will be
    // reconfigured for the new device ID.
    //
    HWREG(NVIC_SW_TRIG) = INT_CAN0 - 16;
}

//*****************************************************************************
//
// Indicates that an enumeration response should be sent for this device.
//
//*****************************************************************************
void
CANIFEnumerate(void)
{
    //
    // Set the enumerate flag.
    //
    HWREGBITW(&g_ulCANFlags, CAN_FLAG_ENUM) = 1;

    //
    // Trigger a fake CAN interrupt, during which the enumeration data will be
    // sent back.
    //
    HWREG(NVIC_SW_TRIG) = INT_CAN0 - 16;
}

//*****************************************************************************
//
// Indicates that there are Periodic Status messages to be sent out.
//
//*****************************************************************************
void
CANIFPStatus(void)
{
    //
    // Set the flag that indicates that there is a pending periodic status
    // message ready for transmission.
    //
    HWREGBITW(&g_ulCANFlags, CAN_FLAG_PSTATUS) = 1;

    //
    // Trigger a fake CAN interrupt during which the Periodic Status messages
    // will be sent out.
    //
    HWREG(NVIC_SW_TRIG) = INT_CAN0 - 16;
}

//*****************************************************************************
//
// Sends a bridged message on the CAN bus.  This must only be called from the
// UART interrupt handler, where the interrupt priority is the same as the CAN
// interrupt (therefore providing the required mutual exclusion between the
// UART interrupt access to the CAN module and the CAN interrupt access to the
// CAN module).  Failure to meet this requirement will result in unpredictable
// behavior of the CAN module.
//
//*****************************************************************************
void
CANIFSendBridgeMessage(unsigned long ulID, unsigned char *pucData,
                       unsigned long ulMsgLen)
{
    unsigned long ulCount;

    if((ulID & ~(CAN_MSGID_DEVNO_M)) == CAN_MSGID_API_DEVQUERY)
    {
        g_psMsgObject[MSG_OBJ_BRIDGE_QUERY].ulMsgID = ulID;
        CANIFMessageSet(MSG_OBJ_BRIDGE_QUERY_ID,
                        &g_psMsgObject[MSG_OBJ_BRIDGE_QUERY],
                        MSG_OBJ_TYPE_TX_REMOTE);
    }
    else if((ulID & ~(CAN_MSGID_DEVNO_M)) == CAN_MSGID_API_FIRMVER)
    {
        g_psMsgObject[MSG_OBJ_BRIDGE_VER].ulMsgID = ulID;
        CANIFMessageSet(MSG_OBJ_BRIDGE_VER_ID,
                        &g_psMsgObject[MSG_OBJ_BRIDGE_VER],
                        MSG_OBJ_TYPE_TX_REMOTE);
    }
    else
    {
        //
        // Setup the message object with the data for this message.
        //
        g_psMsgObject[MSG_OBJ_BRIDGE_TX].ulMsgID = ulID;
        g_psMsgObject[MSG_OBJ_BRIDGE_TX].pucMsgData = pucData;
        g_psMsgObject[MSG_OBJ_BRIDGE_TX].ulMsgLen = ulMsgLen;

        //
        // If there is a pending transmit request on the bridge transmit
        // message object, then assume that it has taken too long and cancel it
        // now.
        //
#if MSG_OBJ_BRIDGE_TX < 16
        if((HWREG(CAN0_BASE + CAN_O_TXRQ1) & (1 << MSG_OBJ_BRIDGE_TX)) != 0)
        {
            CANIFMessageClear(MSG_OBJ_BRIDGE_TX_ID);
        }
#else
        if((HWREG(CAN0_BASE + CAN_O_TXRQ2) &
            (1 << (MSG_OBJ_BRIDGE_TX - 16))) != 0)
        {
            CANIFMessageClear(MSG_OBJ_BRIDGE_TX_ID);
        }
#endif

        //
        // Send this message.
        //
        CANIFMessageSet(MSG_OBJ_BRIDGE_TX_ID,
                        &g_psMsgObject[MSG_OBJ_BRIDGE_TX], MSG_OBJ_TYPE_TX);
    }

    //
    // If this was a reset, wait around to let it go so that we don't reset
    // before sending on the message.
    //
    if((ulID & ~(CAN_MSGID_DEVNO_M)) == CAN_MSGID_API_SYSRST)
    {
        //
        // Number of times to read the status before timeout and resetting
        // the controller.  Since each pass through the loop was taking around
        // 60us, a count of 100 is around a 6ms timeout before allowing a
        // reset.
        //
        ulCount = 100;

        //
        // Wait while the transfer is still pending and the timeout count
        // has not reached zero.
        //
#if MSG_OBJ_BRIDGE_TX < 16
        while((HWREG(CAN0_BASE + CAN_O_TXRQ1) & (1 << MSG_OBJ_BRIDGE_TX)) &&
              (ulCount != 0))
        {
            ulCount--;
        }
#else
        while((HWREG(CAN0_BASE + CAN_O_TXRQ2) &
               (1 << (MSG_OBJ_BRIDGE_TX - 16))) && (ulCount != 0))
        {
            ulCount--;
        }
#endif
    }
}

//*****************************************************************************
//
// This functions writes No Event to the CANSTS LEC field.
//
//*****************************************************************************
void
CANStatusWriteLECNoEvent(void)
{
    //
    // Write No Event to the CANSTS LEC field.
    //
    HWREG(CAN0_BASE + CAN_O_STS) = CAN_STS_LEC_NOEVENT;
}

//*****************************************************************************
//
// Return the value of the CAN Status register.
//
//*****************************************************************************
unsigned long
CANStatusRegGet(void)
{
    //
    // Return the global CAN Status variable.
    //
    return(g_ulCANStatus);
}

//*****************************************************************************
//
// Return the value of the CAN Error register.
//
//*****************************************************************************
unsigned long
CANErrorRegGet(void)
{
    //
    // Return the CAN Error register.
    //
    return(HWREG(CAN0_BASE + CAN_O_ERR));
}
