//*****************************************************************************
//
// ui_serial.c - A simple control interface utilizing a UART.
//
// Copyright (c) 2006-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-ACIM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "commands.h"
#include "ui_common.h"
#include "ui_serial.h"

//*****************************************************************************
//
//! \page ui_serial_intro Introduction
//!
//! A generic, packet-based serial protocol is utilized for communicating with
//! the motor drive board.  This provides a method to control the motor drive,
//! adjust its parameters, and retrieve real-time performance data.  The serial
//! interface is run at 115,200 baud, with an 8-N-1 data format.  Some of the
//! factors that influenced the design of this protocol include:
//!
//! - The same serial protocol should be used for all motor drive boards,
//!   regardless of the motor type (that is, AC induction, stepper, and
//!   so on).
//! - The protocol should make reasonable attempts to protect against invalid
//!   commands being acted upon.
//! - It should be possible to connect to a running motor drive board and lock
//!   on to the real-time data stream without having to restart the data
//!   stream. 
//!
//! The code for handling the serial protocol is contained in
//! <tt>ui_serial.c</tt>, with <tt>ui_serial.h</tt> containing the definitions
//! for the structures, functions, and variables exported to the remainder
//! of the application.  The file <tt>commands.h</tt> contains the definitions
//! for the commands, parameters, real-time data items, and responses that are
//! used in the serial protocol.
//!
//! \section cmf Command Message Format
//!
//! Commands are sent to the motor drive with the following format:
//!
//! \verbatim
//!     {tag} {length} {command} {optional command data byte(s)} {checksum}
//! \endverbatim
//!
//! - The {tag} byte is 0xff.
//!
//! - The {length} byte contains the overall length of the command packet,
//!   starting with the {tag} and ending with the {checksum}.  The maximum
//!   packet length is 255 bytes.
//!
//! - The {command} byte is the command being sent.  Based on the command,
//!   there may be optional command data bytes that follow.
//!
//! - The {checksum} byte is the value such that the sum of all bytes in the
//!   command packet (including the checksum) will be zero.  This is used to
//!   validate a command packet and allow the target to synchronize with the
//!   command stream being sent by the host.
//!
//! For example, the 0x01 command with no data bytes would be sent as
//! follows:
//!
//! \verbatim
//!     0xff 0x04 0x01 0xfc
//! \endverbatim
//!
//! And the 0x02 command with two data bytes (0xab and 0xcd) would be sent as
//! follows:
//!
//! \verbatim
//!     0xff 0x06 0x02 0xab 0xcd 0x81
//! \endverbatim
//!
//! \section smf Status Message Format
//!
//! Status messages are sent from the motor drive with the following format:
//!
//! \verbatim
//!     {tag} {length} {data bytes} {checksum}
//! \endverbatim
//!
//! - The {tag} byte is 0xfe for command responses and 0xfd for real-time data.
//!
//! - The {length} byte contains the overall length of the status packet,
//!   starting with the {tag} byte and ending with the {checksum}.
//!
//! - The contents of the data bytes are dependent upon the tag byte.
//!
//! - The {checksum} is the value such that the sum of all bytes in the status
//!   packet (including the checksum) will be zero.  This is used to validate a
//!   status packet and allow the user interface to synchronize with the status
//!   stream being sent by the target.
//!
//! For command responses ({tag} = 0xfe), the first data byte is the command
//! that is being responded to.  The remaining bytes are the response, and are
//! dependent upon the command.
//!
//! For real-time data messages ({tag} = 0xfd), each real-time data item is
//! transmitted as a little-endian value (for example, for a 16-bit value, the
//! lower 8 bits first then the upper 8 bits).  The data items are in the same
//! order as returned by the data item list (#CMD_GET_DATA_ITEMS) regardless of
//! the order that they were enabled.
//!
//! For example, if data items 1, 5, and 17 were enabled, and each was two
//! bytes in length, there would be 6 data bytes in the packet:
//!
//! \verbatim
//!     0xfd 0x09 {d1[0:7]} {d1[8:15]} {d5[0:7]} {d5[8:15]} {d17[0:7]}
//!          {d17[8:15]} {checksum}
//! \endverbatim
//!
//! \section pi Parameter Interpretation
//!
//! The size and units of the parameters are dependent upon the motor drive;
//! the units are not conveyed in the serial protocol.  Each parameter value is
//! transmitted in little endian format.  Not all parameters are necessarily
//! supported by a motor drive, only those that are appropriate.
//!
//! \section itta Interface To The Application
//!
//! The serial protocol handler takes care of all the serial communications and
//! command interpretation.  A set of functions provided by the application and
//! an array of structures that describe the parameters and real-time data
//! items supported by the motor drive.  The functions are used when an
//! application-specific action needs to take place as a result of the serial
//! communication (such as starting the motor drive).  The structures are used
//! to handle the parameters and real-time data items of the motor drive.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup ui_serial_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The size of the UART transmit buffer.  This should be appropriately sized
//! such that the maximum burst of output data can be contained in this buffer.
//! This value should be a power of two in order to make the modulo arithmetic
//! be fast (that is, an AND instead of a divide).
//
//*****************************************************************************
#ifndef UISERIAL_MAX_XMIT
#define UISERIAL_MAX_XMIT       64
#endif

//*****************************************************************************
//
//! The size of the UART receive buffer.  This should be appropriately sized
//! such that the maximum size command packet can be contained in this buffer.
//! This value should be a power of two in order to make the modulo arithmetic
//! be fast (that is, an AND instead of a divide).
//
//*****************************************************************************
#ifndef UISERIAL_MAX_RECV
#define UISERIAL_MAX_RECV       64
#endif

//*****************************************************************************
//
//! A buffer to contain data received from the UART.  A packet is processed out
//! of this buffer once the entire packet is contained within the buffer.
//
//*****************************************************************************
static unsigned char g_pucUISerialReceive[UISERIAL_MAX_RECV];

//*****************************************************************************
//
//! The offset of the next byte to be read from g_pucUISerialReceive.
//
//*****************************************************************************
static unsigned long g_ulUISerialReceiveRead;

//*****************************************************************************
//
//! The offset of the next byte to be written to g_pucUISerialReceive.
//
//*****************************************************************************
static unsigned long g_ulUISerialReceiveWrite;

//*****************************************************************************
//
//! A buffer to contain data to be written to the UART.
//
//*****************************************************************************
static unsigned char g_pucUISerialTransmit[UISERIAL_MAX_XMIT];

//*****************************************************************************
//
//! The offset of the next byte to be read from g_pucUISerialTransmit.
//
//*****************************************************************************
static unsigned long g_ulUISerialTransmitRead;

//*****************************************************************************
//
//! The offset of the next byte to be written to g_pucUISerialTransmit.
//
//*****************************************************************************
static unsigned long g_ulUISerialTransmitWrite;

//*****************************************************************************
//
//! A buffer used to construct status packets before they are written to the
//! UART and/or g_pucUISerialTransmit.
//
//*****************************************************************************
static unsigned char g_pucUISerialResponse[UISERIAL_MAX_XMIT];

//*****************************************************************************
//
//! A buffer used to construct real-time data packets before they are written
//! to the UART and/or g_pucUISerialTransmit.
//
//*****************************************************************************
static unsigned char g_pucUISerialData[UISERIAL_MAX_XMIT];

//*****************************************************************************
//
//! A boolean that is true when the real-time data stream is enabled.
//
//*****************************************************************************
static tBoolean g_bEnableRealTimeData;

//*****************************************************************************
//
//! A bit array that contains a flag for each real-time data item.  When the
//! corresponding flag is set, that real-time data item is enabled in the
//! real-time data stream; when the flag is clear, that real-time data item is
//! not part of the real-time data stream.
//
//*****************************************************************************
static unsigned long g_pulUIRealTimeData[(DATA_NUM_ITEMS + 31) / 32];

//*****************************************************************************
//
//! Transmits a packet to the UART.
//!
//! \param pucBuffer is a pointer to the packet to be transmitted.
//!
//! This function will send a packet via the UART.  It will compute the
//! checksum of the packet (based on the length in the second byte) and place
//! it at the end of the packet before sending the packet.  If
//! g_pucUISerialTransmit is empty and there is space in the UART's FIFO, as
//! much of the packet as will fit will be written directly to the UART's FIFO.
//! The remainder of the packet will be buffered for later transmission when
//! space becomes available in the UART's FIFO (which will then be written by
//! the UART interrupt handler).
//!
//! \return Returns \b true if the entire packet fit into the combination of
//! the UART's FIFO and g_pucUISerialTransmit, and \b false otherwise.
//
//*****************************************************************************
static tBoolean
UISerialTransmit(unsigned char *pucBuffer)
{
    unsigned long ulIdx, ulSum, ulLength;

    //
    // Compute the checksum for this packet and put it at the end.
    //
    for(ulIdx = 0, ulSum = 0, ulLength = pucBuffer[1]; ulIdx < (ulLength - 1);
        ulIdx++)
    {
        ulSum -= pucBuffer[ulIdx];
    }
    pucBuffer[ulLength - 1] = ulSum;

    //
    // See if the transmit buffer is empty.
    //
    if(g_ulUISerialTransmitRead == g_ulUISerialTransmitWrite)
    {
        //
        // Loop while there is more data to transmit and space in the UART
        // FIFO.
        //
        while(ulLength && UARTSpaceAvail(UART0_BASE))
        {
            //
            // Write the next character.
            //
            UARTCharPut(UART0_BASE, *pucBuffer++);

            //
            // Decrement the count of characters to send.
            //
            ulLength--;
        }
    }

    //
    // Loop while there are more characters to send and there is more space in
    // the transmit buffer.
    //
    while(ulLength && (((g_ulUISerialTransmitWrite + 1) % UISERIAL_MAX_XMIT) !=
                       g_ulUISerialTransmitRead))
    {
        //
        // Write the next character to the transmit buffer.
        //
        g_pucUISerialTransmit[g_ulUISerialTransmitWrite] = *pucBuffer++;

        //
        // Increment the transmit buffer write pointer.
        //
        g_ulUISerialTransmitWrite =
            (g_ulUISerialTransmitWrite + 1) % UISERIAL_MAX_XMIT;

        //
        // Decrement the count of characters to send.
        //
        ulLength--;
    }

    //
    // Return an error if there are characters to be sent that would not fit
    // into the transmit buffer.
    //
    if(ulLength)
    {
        return(false);
    }
    else
    {
        return(true);
    }
}

//*****************************************************************************
//
//! Finds a parameter by ID.
//!
//! \param ucID is the ID of the parameter to locate.
//!
//! This function searches the list of parameters looking for one that matches
//! the provided ID.
//!
//! \return Returns the index of the parameter found, or 0xffff.ffff if the
//! parameter does not exist in the parameter list.
//
//*****************************************************************************
static unsigned long
UISerialFindParameter(unsigned char ucID)
{
    unsigned long ulIdx;

    //
    // Loop through the parameters.
    //
    for(ulIdx = 0; ulIdx < g_ulUINumParameters; ulIdx++)
    {
        //
        // See if this parameter matches the search ID.
        //
        if(g_sUIParameters[ulIdx].ucID == ucID)
        {
            //
            // Since this parameter matches, return its index.
            //
            return(ulIdx);
        }
    }

    //
    // The parameter could not be located, so return a failure.
    //
    return(0xffffffff);
}

//*****************************************************************************
//
//! Performs range checking on the value of a parameter.
//!
//! \param ulIdx is the index of the parameter to check.
//!
//! This function will perform range checking on the value of a parameter,
//! adjusting the parameter value if necessary to make it reside within the
//! predetermined range.
//!
//! \return None.
//
//*****************************************************************************
static void
UISerialRangeCheck(unsigned long ulIdx)
{
    unsigned long *pulValue;
    unsigned short *pusValue;
    long *plValue, lCheck;
    short *psValue, sCheck;
    char *pcValue, cCheck;

    //
    // See if a range exists for this parameter.  If not, then return without
    // checking the parameter's value.
    //
    if(((g_sUIParameters[ulIdx].ulMin == 0) &&
        (g_sUIParameters[ulIdx].ulMax == 0)) ||
       (g_sUIParameters[ulIdx].ucSize > 4))
    {
        return;
    }

    //
    // See if the value of this parameter is signed or unsigned.
    //
    if(g_sUIParameters[ulIdx].ulMin > g_sUIParameters[ulIdx].ulMax)
    {
        //
        // This is a signed parameter.  Determine the size of the parameter.
        //
        switch(g_sUIParameters[ulIdx].ucSize)
        {
            //
            // This parameter is a signed character.
            //
            case 1:
            {
                //
                // Get the signed value of this parameter.
                //
                pcValue = (char *)(g_sUIParameters[ulIdx].pucValue);

                //
                // Clamp the parameter value to the minimum if it is below the
                // minimum.
                //
                cCheck = (char)g_sUIParameters[ulIdx].ulMin;
                if(*pcValue < cCheck)
                {
                    *pcValue = cCheck;
                }

                //
                // Clamp the parameter value to the maximum if it is above the
                // maximum.
                //
                cCheck = (char)g_sUIParameters[ulIdx].ulMax;
                if(*pcValue > cCheck)
                {
                    *pcValue = cCheck;
                }

                //
                // This parameter value has been handled.
                //
                break;
            }

            //
            // This parameter is a signed short.
            //
            case 2:
            {
                //
                // Get the signed value of this parameter.
                //
                psValue = (short *)(g_sUIParameters[ulIdx].pucValue);

                //
                // Clamp the parameter value to the minimum if it is below the
                // minimum.
                //
                sCheck = (short)g_sUIParameters[ulIdx].ulMin;
                if(*psValue < sCheck)
                {
                    *psValue = sCheck;
                }

                //
                // Clamp the parameter value to the maximum if it is above the
                // maximum.
                //
                sCheck = (short)g_sUIParameters[ulIdx].ulMax;
                if(*psValue > sCheck)
                {
                    *psValue = sCheck;
                }

                //
                // This parameter value has been handled.
                //
                break;
            }

            //
            // This parameter is a signed long.
            //
            case 4:
            default:
            {
                //
                // Get the signed value of this parameter.
                //
                plValue = (long *)(g_sUIParameters[ulIdx].pucValue);

                //
                // Clamp the parameter value to the minimum if it is below the
                // minimum.
                //
                lCheck = (long)g_sUIParameters[ulIdx].ulMin;
                if(*plValue < lCheck)
                {
                    *plValue = lCheck;
                }

                //
                // Clamp the parameter value to the maximum if it is above the
                // maximum.
                //
                lCheck = (long)g_sUIParameters[ulIdx].ulMax;
                if(*plValue > lCheck)
                {
                    *plValue = lCheck;
                }

                //
                // This parameter value has been handled.
                //
                break;
            }
        }
    }
    else
    {
        //
        // This is an unsigned parameter.  Determine the size of the parameter.
        //
        switch(g_sUIParameters[ulIdx].ucSize)
        {
            //
            // This parameter is an unsigned character.
            //
            case 1:
            {
                //
                // Clamp the parameter value to the minimum if it is below the
                // minimum.
                //
                if(g_sUIParameters[ulIdx].pucValue[0] <
                   g_sUIParameters[ulIdx].ulMin)
                {
                    g_sUIParameters[ulIdx].pucValue[0] =
                        g_sUIParameters[ulIdx].ulMin;
                }

                //
                // Clamp the parameter value to the maximum if it is above the
                // maximum.
                //
                if(g_sUIParameters[ulIdx].pucValue[0] >
                   g_sUIParameters[ulIdx].ulMax)
                {
                    g_sUIParameters[ulIdx].pucValue[0] =
                        g_sUIParameters[ulIdx].ulMax;
                }

                //
                // This parameter value has been handled.
                //
                break;
            }

            //
            // This parameter is an unsigned short.
            //
            case 2:
            {
                //
                // Get the value of this parameter.
                //
                pusValue = (unsigned short *)(g_sUIParameters[ulIdx].pucValue);

                //
                // Clamp the parameter value to the minimum if it is below the
                // minimum.
                //
                if(*pusValue < g_sUIParameters[ulIdx].ulMin)
                {
                    *pusValue = g_sUIParameters[ulIdx].ulMin;
                }

                //
                // Clamp the parameter value to the maximum if it is above the
                // maximum.
                //
                if(*pusValue > g_sUIParameters[ulIdx].ulMax)
                {
                    *pusValue = g_sUIParameters[ulIdx].ulMax;
                }

                //
                // This parameter value has been handled.
                //
                break;
            }

            //
            // This parameter is an unsigned long.
            //
            case 4:
            default:
            {
                //
                // Get the value of this parameter.
                //
                pulValue = (unsigned long *)(g_sUIParameters[ulIdx].pucValue);

                //
                // Clamp the parameter value to the minimum if it is below the
                // minimum.
                //
                if(*pulValue < g_sUIParameters[ulIdx].ulMin)
                {
                    *pulValue = g_sUIParameters[ulIdx].ulMin;
                }

                //
                // Clamp the parameter value to the maximum if it is above the
                // maximum.
                //
                if(*pulValue > g_sUIParameters[ulIdx].ulMax)
                {
                    *pulValue = g_sUIParameters[ulIdx].ulMax;
                }

                //
                // This parameter value has been handled.
                //
                break;
            }
        }
    }
}

//*****************************************************************************
//
//! Scans for packets in the receive buffer.
//!
//! This function will scan through g_pucUISerialReceive looking for valid
//! command packets.  When found, the command packets will be handled.
//!
//! \return None.
//
//*****************************************************************************
static void
UISerialScanReceive(void)
{
    unsigned char ucSum, ucSize;
    unsigned long ulIdx;

    //
    // Loop while there is data in the receive buffer.
    //
    while(g_ulUISerialReceiveRead != g_ulUISerialReceiveWrite)
    {
        //
        // See if this character is the tag for the start of a command packet.
        //
        if(g_pucUISerialReceive[g_ulUISerialReceiveRead] != TAG_CMD)
        {
            //
            // Skip this character.
            //
            g_ulUISerialReceiveRead =
                (g_ulUISerialReceiveRead + 1) % UISERIAL_MAX_RECV;

            //
            // Keep scanning for a start of command packet tag.
            //
            continue;
        }

        //
        // See if there are additional characters in the receive buffer.
        //
        if(((g_ulUISerialReceiveRead + 1) % UISERIAL_MAX_RECV) ==
           g_ulUISerialReceiveWrite)
        {
            //
            // There are no additional characters in the receive buffer after
            // the start of command packet byte, so stop scanning for now.
            //
            break;
        }

        //
        // See if the packet size byte is valid.  A command packet must be at
        // least four bytes and can not be larger than the receive buffer size.
        //
        ucSize = g_pucUISerialReceive[(g_ulUISerialReceiveRead + 1) %
                                      UISERIAL_MAX_RECV];
        if((ucSize < 4) || (ucSize > (UISERIAL_MAX_RECV - 1)))
        {
            //
            // The packet size is too large, so either this is not the start of
            // a packet or an invalid packet was received.  Skip this start of
            // command packet tag.
            //
            g_ulUISerialReceiveRead =
                (g_ulUISerialReceiveRead + 1) % UISERIAL_MAX_RECV;

            //
            // Keep scanning for a start of command packet tag.
            //
            continue;
        }

        //
        // Determine the number of bytes in the receive buffer.
        //
        ulIdx = g_ulUISerialReceiveWrite - g_ulUISerialReceiveRead;
        if(g_ulUISerialReceiveRead > g_ulUISerialReceiveWrite)
        {
            ulIdx += UISERIAL_MAX_RECV;
        }

        //
        // If the entire command packet is not in the receive buffer then stop
        // scanning for now.
        //
        if(ulIdx < ucSize)
        {
            break;
        }

        //
        // The entire command packet is in the receive buffer, so compute its
        // checksum.
        //
        for(ulIdx = 0, ucSum = 0; ulIdx < ucSize; ulIdx++)
        {
            ucSum += g_pucUISerialReceive[(g_ulUISerialReceiveRead + ulIdx) %
                                          UISERIAL_MAX_RECV];
        }

        //
        // Skip this packet if the checksum is not correct (that is, it is
        // probably not really the start of a packet).
        //
        if(ucSum != 0)
        {
            //
            // Skip this character.
            //
            g_ulUISerialReceiveRead =
                (g_ulUISerialReceiveRead + 1) % UISERIAL_MAX_RECV;

            //
            // Keep scanning for a start of command packet tag.
            //
            continue;
        }

        //
        // A valid command packet was received, so process it now.
        //
        switch(g_pucUISerialReceive[(g_ulUISerialReceiveRead + 2) %
                                    UISERIAL_MAX_RECV])
        {
            //
            // The command to get the target type.
            //
            case CMD_ID_TARGET:
            {
                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = 0x05;
                g_pucUISerialResponse[2] = CMD_ID_TARGET;
                g_pucUISerialResponse[3] = g_ulUITargetType;

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to upgrade the firmware.
            //
            case CMD_UPGRADE:
            {
                //
                // Pass the upgrade request to the application.
                //
                UIUpgrade();

                //
                // Control should never return here, but enter an infinite loop
                // just in case it does.
                //
                while(1)
                {
                }
            }

            //
            // The command to get a list of the parameters.
            //
            case CMD_GET_PARAMS:
            {
                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = g_ulUINumParameters + 4;
                g_pucUISerialResponse[2] = CMD_GET_PARAMS;
                for(ulIdx = 0; ulIdx < g_ulUINumParameters; ulIdx++)
                {
                    g_pucUISerialResponse[ulIdx + 3] =
                        g_sUIParameters[ulIdx].ucID;
                }

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to get a description of a parameter.
            //
            case CMD_GET_PARAM_DESC:
            {
                //
                // Find the parameter.
                //
                ucSum =
                    g_pucUISerialReceive[(g_ulUISerialReceiveRead + 3) %
                                         UISERIAL_MAX_RECV];
                ulIdx = UISerialFindParameter(ucSum);

                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[2] = CMD_GET_PARAM_DESC;

                //
                // If a parameter was not specified, or if the parameter could
                // not be found, then return a zero length.
                //
                if((ucSize != 5) || (ulIdx == 0xffffffff))
                {
                    g_pucUISerialResponse[1] = 0x05;
                    g_pucUISerialResponse[3] = 0x00;
                }

                //
                // If the length of the parameter is greater than a 32-bit
                // value, then return just the size of the parameter.
                //
                else if(g_sUIParameters[ulIdx].ucSize > 4)
                {
                    g_pucUISerialResponse[1] = 0x05;
                    g_pucUISerialResponse[3] = g_sUIParameters[ulIdx].ucSize;
                }

                //
                // Otherwise, return the size, minimum, maximum, and step size
                // for the parameter.
                //
                else
                {
                    //
                    // Set the size of the response packet.
                    //
                    g_pucUISerialResponse[1] =
                        (g_sUIParameters[ulIdx].ucSize * 3) + 5;

                    //
                    // Set the size of the parameter value.
                    //
                    g_pucUISerialResponse[3] = g_sUIParameters[ulIdx].ucSize;

                    //
                    // Loop through the bytes of the parameter value.
                    //
                    ucSize = g_sUIParameters[ulIdx].ucSize;
                    for(ucSum = 0; ucSum < ucSize; ucSum++)
                    {
                        //
                        // Set this byte of the parameter minimum value.
                        //
                        g_pucUISerialResponse[ucSum + 4] =
                            ((g_sUIParameters[ulIdx].ulMin >> (ucSum * 8)) &
                             0xff);

                        //
                        // Set this byte of the parameter maximum value.
                        //
                        g_pucUISerialResponse[ucSum + ucSize + 4] =
                            ((g_sUIParameters[ulIdx].ulMax >> (ucSum * 8)) &
                             0xff);

                        //
                        // Set this byte of the parameter step size.
                        //
                        g_pucUISerialResponse[ucSum + (ucSize << 1) + 4] =
                            ((g_sUIParameters[ulIdx].ulStep >> (ucSum * 8)) &
                             0xff);
                    }
                }

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to get the value of a parameter.
            //
            case CMD_GET_PARAM_VALUE:
            {
                //
                // Find the parameter.
                //
                ucSum =
                    g_pucUISerialReceive[(g_ulUISerialReceiveRead + 3) %
                                         UISERIAL_MAX_RECV];
                ulIdx = UISerialFindParameter(ucSum);

                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[2] = CMD_GET_PARAM_VALUE;

                //
                // If a parameter was not specified, or if the parameter could
                // not be found, then return no value.
                //
                if((ucSize != 5) || (ulIdx == 0xffffffff))
                {
                    g_pucUISerialResponse[1] = 0x04;
                }

                //
                // Return the current value of the parameter.
                //
                else
                {
                    //
                    // Set the response packet size based on the size of the
                    // parameter.
                    //
                    g_pucUISerialResponse[1] =
                        g_sUIParameters[ulIdx].ucSize + 4;

                    //
                    // Copy the parameter value to the response packet.
                    //
                    for(ucSum = 0; ucSum < g_sUIParameters[ulIdx].ucSize;
                        ucSum++)
                    {
                        g_pucUISerialResponse[ucSum + 3] =
                            g_sUIParameters[ulIdx].pucValue[ucSum];
                    }
                }

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to set the value of a parameter.
            //
            case CMD_SET_PARAM_VALUE:
            {
                //
                // Find the parameter.
                //
                ucSum =
                    g_pucUISerialReceive[(g_ulUISerialReceiveRead + 3) %
                                         UISERIAL_MAX_RECV];
                ulIdx = UISerialFindParameter(ucSum);

                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = 0x04;
                g_pucUISerialResponse[2] = CMD_SET_PARAM_VALUE;

                //
                // Only set the value of the parameter if a value was
                // specified, the parameter could be found, and the parameter
                // is not read-only.
                //
                if((ucSize > 5) && (ulIdx != 0xffffffff) &&
                   (g_sUIParameters[ulIdx].ulStep != 0))
                {
                    //
                    // Loop through the bytes of this parameter value.
                    //
                    for(ucSum = 0; ucSum < g_sUIParameters[ulIdx].ucSize;
                        ucSum++)
                    {
                        //
                        // See if this byte was supplied.
                        //
                        if(ucSum < (ucSize - 5))
                        {
                            //
                            // Set this byte of the parameter value based on
                            // the supplied byte.
                            //
                            g_sUIParameters[ulIdx].pucValue[ucSum] =
                                g_pucUISerialReceive[(g_ulUISerialReceiveRead +
                                                      ucSum + 4) %
                                                     UISERIAL_MAX_RECV];
                        }
                        else
                        {
                            //
                            // Set this byte of the parameter value to zero
                            // since it was not supplied.
                            //
                            g_sUIParameters[ulIdx].pucValue[ucSum] = 0;
                        }
                    }

                    //
                    // Perform range checking on the parameter value.
                    //
                    UISerialRangeCheck(ulIdx);

                    //
                    // If there is an update function for this parameter then
                    // call it now.
                    //
                    if(g_sUIParameters[ulIdx].pfnUpdate)
                    {
                        g_sUIParameters[ulIdx].pfnUpdate();
                    }
                }

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to load parameters from flash.
            //
            case CMD_LOAD_PARAMS:
            {
                //
                // Pass the parameter load request to the application.
                //
                UIParamLoad();

                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = 0x04;
                g_pucUISerialResponse[2] = CMD_LOAD_PARAMS;

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to save parameters to flash.
            //
            case CMD_SAVE_PARAMS:
            {
                //
                // Pass the parameter save request to the application.
                //
                UIParamSave();

                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = 0x04;
                g_pucUISerialResponse[2] = CMD_LOAD_PARAMS;

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to get a list of the real-time data items.
            //
            case CMD_GET_DATA_ITEMS:
            {
                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = (g_ulUINumRealTimeData * 2) + 4;
                g_pucUISerialResponse[2] = CMD_GET_DATA_ITEMS;
                for(ulIdx = 0; ulIdx < g_ulUINumRealTimeData; ulIdx++)
                {
                    g_pucUISerialResponse[(ulIdx * 2) + 3] =
                        g_sUIRealTimeData[ulIdx].ucID;
                    g_pucUISerialResponse[(ulIdx * 2) + 4] =
                        g_sUIRealTimeData[ulIdx].ucSize;
                }

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to enable a real-time data item.
            //
            case CMD_ENABLE_DATA_ITEM:
            {
                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = 0x04;
                g_pucUISerialResponse[2] = CMD_ENABLE_DATA_ITEM;

                //
                // Enable the data item if it is was validly specified.
                //
                ucSum = g_pucUISerialReceive[(g_ulUISerialReceiveRead + 3) %
                                                        UISERIAL_MAX_RECV];
                if((ucSize == 5) && (ucSum < DATA_NUM_ITEMS))
                {
                    g_pulUIRealTimeData[ucSum / 32] |= 1 << (ucSum % 32);
                }

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to disable a real-time data item.
            //
            case CMD_DISABLE_DATA_ITEM:
            {
                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = 0x04;
                g_pucUISerialResponse[2] = CMD_DISABLE_DATA_ITEM;

                //
                // Disable the data item if it is was validly specified.
                //
                ucSum = g_pucUISerialReceive[(g_ulUISerialReceiveRead + 3) %
                                                        UISERIAL_MAX_RECV];
                if((ucSize == 5) && (ucSum < DATA_NUM_ITEMS))
                {
                    g_pulUIRealTimeData[ucSum / 32] &= ~(1 << (ucSum % 32));
                }

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to start the real-time data stream.
            //
            case CMD_START_DATA_STREAM:
            {
                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = 0x04;
                g_pucUISerialResponse[2] = CMD_START_DATA_STREAM;

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Enable the real-time data stream.
                //
                g_bEnableRealTimeData = true;

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to stop the real-time data stream.
            //
            case CMD_STOP_DATA_STREAM:
            {
                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = 0x04;
                g_pucUISerialResponse[2] = CMD_STOP_DATA_STREAM;

                //
                // Disable the real-time data stream.
                //
                g_bEnableRealTimeData = false;

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to start the motor drive.
            //
            case CMD_RUN:
            {
                //
                // Pass the run request to the application.
                //
                UIRun();

                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = 0x04;
                g_pucUISerialResponse[2] = CMD_RUN;

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command to stop the motor drive.
            //
            case CMD_STOP:
            {
                //
                // Pass the stop request to the application.
                //
                UIStop();

                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = 0x04;
                g_pucUISerialResponse[2] = CMD_STOP;

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // The command for an emmergency stop of the motor drive.
            //
            case CMD_EMERGENCY_STOP:
            {
                //
                // Pass the emergency stop request to the application.
                //
                UIEmergencyStop();

                //
                // Fill in the response.
                //
                g_pucUISerialResponse[0] = TAG_STATUS;
                g_pucUISerialResponse[1] = 0x04;
                g_pucUISerialResponse[2] = CMD_EMERGENCY_STOP;

                //
                // Send the response.
                //
                UISerialTransmit(g_pucUISerialResponse);

                //
                // Done with this command.
                //
                break;
            }

            //
            // An unrecognized command was received.  Simply ignore it.
            //
            default:
            {
                //
                // Done with this command.
                //
                break;
            }
        }

        //
        // Skip this command packet.
        //
        g_ulUISerialReceiveRead =
            (g_ulUISerialReceiveRead + ucSize) % UISERIAL_MAX_RECV;
    }
}

//*****************************************************************************
//
//! Handles the UART interrupt.
//!
//! This is the interrupt handler for the UART.  It will write new data to the
//! UART when there is data to be written, and read new data from the UART when
//! it is available.  Reception of new data results in the receive buffer being
//! scanned for command packets.
//!
//! \return None.
//
//*****************************************************************************
void
UART0IntHandler(void)
{
    unsigned long ulStatus;

    //
    // Get the current UART interrupt status.
    //
    ulStatus = UARTIntStatus(UART0_BASE, true);

    //
    // Clear the UART interrupts that will be handled.
    //
    UARTIntClear(UART0_BASE, ulStatus);

    //
    // See if the transmit interrupt is being asserted.
    //
    if(ulStatus & UART_INT_TX)
    {
        //
        // Loop while there is data in the transmit buffer and space in the
        // UART FIFO.
        //
        while((g_ulUISerialTransmitRead != g_ulUISerialTransmitWrite) &&
              UARTSpaceAvail(UART0_BASE))
        {
            //
            // Send the next character from the transmit buffer.
            //
            UARTCharPut(UART0_BASE,
                        g_pucUISerialTransmit[g_ulUISerialTransmitRead]);

            //
            // Increment the transmit buffer read pointer.
            //
            g_ulUISerialTransmitRead =
                (g_ulUISerialTransmitRead + 1) % UISERIAL_MAX_XMIT;
        }
    }

    //
    // See if the receive interrupt is being asserted.
    //
    if(ulStatus & (UART_INT_RX | UART_INT_RT))
    {
        //
        // Loop while there are characters in the UART FIFO.
        //
        while(UARTCharsAvail(UART0_BASE))
        {
            //
            // Read the next character into the receive buffer.
            //
            g_pucUISerialReceive[g_ulUISerialReceiveWrite] =
                UARTCharGet(UART0_BASE);

            //
            // Increment the receive buffer write pointer.
            //
            g_ulUISerialReceiveWrite =
                (g_ulUISerialReceiveWrite + 1) % UISERIAL_MAX_RECV;

            //
            // Scan the receive buffer for command packets if it is full.
            //
            if(((g_ulUISerialReceiveWrite + 1) % UISERIAL_MAX_RECV) ==
               g_ulUISerialReceiveRead)
            {
                UISerialScanReceive();
            }
        }

        //
        // Scan the receive buffer for command packets.
        //
        UISerialScanReceive();
    }
}

//*****************************************************************************
//
//! Sends a real-time data packet.
//!
//! This function will construct a real-time data packet with the current
//! values of the enabled real-time data items.  Once constructed, the packet
//! will be sent out.
//!
//! \return None.
//
//*****************************************************************************
void
UISerialSendRealTimeData(void)
{
    unsigned long ulIdx, ulPos, ulItem, ulCount;
    unsigned char pucValue[4];

    //
    // Do nothing if the real-time data stream is not enabled.
    //
    if(!g_bEnableRealTimeData)
    {
        return;
    }

    //
    // Loop through the available real-time data items.
    //
    for(ulItem = 0, ulPos = 2; ulItem < g_ulUINumRealTimeData; ulItem++)
    {
        //
        // Get the ID for this real-time data item.
        //
        ulIdx = g_sUIRealTimeData[ulItem].ucID;

        //
        // See if this real-time data item is enabled.
        //
        if(g_pulUIRealTimeData[ulIdx / 32] & (1 << (ulIdx % 32)))
        {
            //
            // Perform an atomic copy of the value to a local variable.  This
            // prevents the value from being changed midway through adding it
            // to the output real-time data stream.
            //
            *((unsigned long *)pucValue) =
                *((unsigned long *)g_sUIRealTimeData[ulItem].pucValue);

            //
            // Copy the value of this real-time data item, byte by byte, to the
            // packet.
            //
            for(ulCount = 0; ulCount < g_sUIRealTimeData[ulItem].ucSize;
                ulCount++)
            {
                g_pucUISerialData[ulPos++] = pucValue[ulCount];
            }
        }
    }

    //
    // Put the header and length on the real-time data packet.
    //
    g_pucUISerialData[0] = TAG_DATA;
    g_pucUISerialData[1] = ulPos + 1;

    //
    // Send the real-time data packet.  The UART interrupt is disabled during
    // this time to prevent a UART interrupt from inserting a status packet in
    // the middle of the real-time data packet in the UART output stream.
    //
    IntDisable(INT_UART0);
    UISerialTransmit(g_pucUISerialData);
    IntEnable(INT_UART0);
}

//*****************************************************************************
//
//! Initializes the serial user interface.
//!
//! This function prepares the serial user interface for operation.  The UART
//! is configured for 115,200, 8-N-1 operation.  This function should be called
//! before any other serial user interface operations.
//!
//! \return None.
//
//*****************************************************************************
void
UISerialInit(void)
{
    //
    // Enable GPIO port A and UART 0.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure PA0 and PA1 as UART pins.
    //
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE |
                         UART_CONFIG_STOP_ONE));

    //
    // Enable the UART transmit, receive, and receive timeout interrupts.
    //
    UARTIntEnable(UART0_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);
    IntEnable(INT_UART0);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
