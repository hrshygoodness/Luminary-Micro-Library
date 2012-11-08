//*****************************************************************************
//
// packet_handler.c
//
// Copyright (c) 2006-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup packet_handler Packet Handler API
//! This section describes the functions that are responsible for handling the
//! serial packets in the format that is supported by the serial flash loader.
//! These functions call the UART transfer functions that are discussed in the
//! UART Handler API section.
//! @{
//
//*****************************************************************************
#include "packet_handler.h"
#include "uart_handler.h"

//****************************************************************************
//
// local declarations
//
//****************************************************************************
unsigned char CheckSum(unsigned char *pucData, unsigned char ucSize);

//****************************************************************************
//
//! AckPacket() sends an Acknowledge a packet.
//!
//! This function acknowledges a packet has been received from the device.
//!
//! \return The function returns zero to indicated success while any non-zero
//! value indicates a failure.
//
//****************************************************************************
int
AckPacket(void)
{
    unsigned char ucAck;

    ucAck = COMMAND_ACK;
    return(UARTSendData(&ucAck, 1));
}

//****************************************************************************
//
//! NakPacket() sends a No Acknowledge packet.
//!
//! This function sends a no acknowledge for a packet that has been
//! received unsuccessfully from the device.
//!
//! \return The function returns zero to indicated success while any non-zero
//! value indicates a failure.
//
//****************************************************************************
int
NakPacket(void)
{
    unsigned char ucNak;

    ucNak = COMMAND_NAK;
    return(UARTSendData(&ucNak, 1));
}

//*****************************************************************************
//
//! GetPacket() receives a data packet.
//!
//! \param pucData is the location to store the data received from the device.
//! \param pucSize is the number of bytes returned in the pucData buffer that
//! was provided.
//!
//! This function receives a packet of data from UART port.
//!
//! \returns The function returns zero to indicated success while any non-zero
//! value indicates a failure.
//
//*****************************************************************************
int
GetPacket(unsigned char *pucData, unsigned char *pucSize)
{
    unsigned char ucCheckSum;
    unsigned char ucSize;

    //
    // Get the size waht and the checksum.
    //
    do
    {
        if(UARTReceiveData(&ucSize, 1))
        {
            return(-1);
        }
    }
    while(ucSize == 0);

    if(UARTReceiveData(&ucCheckSum, 1))
    {
        return(-1);
    }
    *pucSize = ucSize - 2;

    if(UARTReceiveData(pucData, *pucSize))
    {
        *pucSize = 0;
        return(-1);
    }

    //
    // Calculate the checksum from the data.
    //
    if(CheckSum(pucData, *pucSize) != ucCheckSum)
    {
        *pucSize = 0;
        return(NakPacket());
    }

    return(AckPacket());
}

//*****************************************************************************
//
//! CheckSum() Calculates an 8 bit checksum
//!
//! \param pucData is a pointer to an array of 8 bit data of size ucSize.
//! \param ucSize is the size of the array that will run through the checksum
//!     algorithm.
//!
//! This function simply calculates an 8 bit checksum on the data passed in.
//!
//! \return The function returns the calculated checksum.
//
//*****************************************************************************
unsigned char
CheckSum(unsigned char *pucData, unsigned char ucSize)
{
    int i;
    unsigned char ucCheckSum;

    ucCheckSum = 0;

    for(i = 0; i < ucSize; ++i)
    {
        ucCheckSum += pucData[i];
    }
    return(ucCheckSum);
}

//*****************************************************************************
//
//! SendPacket() sends a data packet.
//!
//! \param pucData is the location of the data to be sent to the device.
//! \param ucSize is the number of bytes to send from puData.
//! \param bAck is a boolean that is true if an ACK/NAK packet should be
//! received in response to this packet.
//!
//! This function sends a packet of data to the device.
//!
//! \returns The function returns zero to indicated success while any non-zero
//!     value indicates a failure.
//
//*****************************************************************************
int
SendPacket(unsigned char *pucData, unsigned char ucSize, unsigned long bAck)
{
    unsigned char ucCheckSum;
    unsigned char ucAck;

    ucCheckSum = CheckSum(pucData, ucSize);

    //
    // Make sure that we add the bytes for the size and checksum to the total.
    //
    ucSize += 2;

    //
    // Send the Size in bytes.
    //
    if(UARTSendData(&ucSize, 1))
    {
        return(-1);
    }

    //
    // Send the CheckSum
    //
    if(UARTSendData(&ucCheckSum, 1))
    {
        return(-1);
    }

    //
    // Now send the remaining bytes out.
    //
    ucSize -= 2;

    //
    // Send the Data
    //
    if(UARTSendData(pucData, ucSize))
    {
        return(-1);
    }

    //
    // Return immediately if no ACK/NAK is expected.
    //
    if(!bAck)
    {
        return(0);
    }

    //
    // Wait for the acknoledge from the device.
    //
    do
    {
        if(UARTReceiveData(&ucAck, 1))
        {
            return(-1);
        }
    }
    while(ucAck == 0);

    if(ucAck != COMMAND_ACK)
    {
        return(-1);
    }
    return(0);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
