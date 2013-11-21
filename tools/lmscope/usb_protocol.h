//*****************************************************************************
//
// usb_protocol.h - Structures and definitions relating to the USB control
//                  protocol used by the Quickstart Oscilloscope application.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#ifndef _USB_PROTOCOL_H_
#define _USB_PROTOCOL_H_

//*****************************************************************************
// Oscilloscope USB Device Protocol Overview
// -----------------------------------------
//
// The oscilloscope application may be controlled remotely by an application
// running on a USB host machine. The oscilloscope offers two bulk endpoints,
// one IN endpoint used to send sample sets and state change information from
// the oscilloscope to the host and one OUT endpoint allowing the host to send
// control requests to the oscilloscope.
//
// All information sent between the device and the host is based upon the
// tScopePacket structure. This structure defines the protocol version in use,
// the command or status update being sent, two packet-specific parameters
// and optional command or status-specific data. Each packet sent is, therefore,
// a tScopePacket structure with optional data appended to the end of it.
//
// Before the device will attempt to send any data to the host, it listens for
// a SCOPE_PKT_HOST_HELLO packet from the host.  This packet indicates that
// the host application is running and informs the device of the version of
// the protocol it is using.  The device then responds with a
// SCOPE_PKT_HELLO_RESPONSE indicating the protocol version that should be
// used, either the protocol version requested by the host or a lower version
// if the device does not support the host's requested version.  This response
// also includes additional data informing the host of the current settings
// of the oscilloscope to allow the application on the host side to initialize
// its user interface appropriately.
//
// After the SCOPE_PKT_HOST_HELLO/SCOPE_PKT_HELLO_RESPONSE handshake, the
// host is free to send any other commands to the device. The device will
// send state updates to the host and also data for all capture sequences
// performed. Communication continues until the device detects that the host
// has disconnected or the host sends a SCOPE_PKT_HOST_GOODBYE. In either of
// these cases, communication is reestablished when another HELLO handshake
// occurs.
//
// Data Transmission from the Oscilloscope to the host
// ---------------------------------------------------
//
// All command and status packets related to device control are completed in
// a single USB packet. In transfering captured data, however, multiple
// packets are required since a single set of capture data is larger than the
// maximum USB packet size. Communication of this data is performed using
// three distinct packet types, SCOPE_PKT_DATA_START, SCOPE_PKT_DATA and
// SCOPE_PKT_DATA_END.
//
// Transmission of a new data set is initiated with a SCOPE_PKT_DATA_START
// packet whose payload provides information on the timing and format of the
// following sample data along with the total number of sample structures that
// are to be sent in following SCOPE_PKT_DATA packets.
//
// After SCOPE_PKT_DATA_START, the device sends multiple SCOPE_PKT_DATA
// packets whose payloads contain an integral number of structures containing
// either single- or dual-channel data depending upon information passed in
// SCOPE_PKT_DATA_START. The SCOPE_PKT_DATA packets also contain a continuity
// counter to allow the host to be sure that no packets have been lost. This
// counter increments by one for each packet in the data set.
//
// Once sufficient SCOPE_PKT_DATA packets have been sent to transfer all the
// sample data, a single SCOPE_PKT_DATA_END packet indicates the end of the
// transfer. The packet count in this packet is one greater than the last
// SCOPE_PKT_DATA packet count value.
//
// If any errors occur, the host may request retransmission of the entire
// data set using SCOPE_PKT_RETRANSMIT which causes the device to resend the
// latest captured data. Alternatively, if continuous capture is taking place,
// the host may simply ignore the damaged data set and wait for the next one.
//
// Host to Device Commands
// -----------------------
//
// This section details the use of the packet-specific parameters in the
// tScopePacket header structure for all command sent from the host to the
// device. In all cases, the ucVersion field must be the protocol version
// number (SCOPE_PROTOCOL_VERSION_1 currently) and ucHdrLength must be set
// to sizeof(tScopePacket) which will equal 12 assuming your compiler packing
// macros are correctly configured.
//
// ucPacketType                ucParam          ulParam        ulDataLength
// ------------                -------          -------        ------------
// SCOPE_PKT_HOST_HELLO        Unused           Unused               0
// SCOPE_PKT_HOST_GOODBYE      Unused           Unused               0
// SCOPE_PKT_SET_TIMEBASE      Unused           uS/Division          0
// SCOPE_PKT_SET_TRIGGER_TYPE  Channel Number   Trigger Type         0
// SCOPE_PKT_SET_TRIGGER_POS   Unused           Trigger Pos          0
// SCOPE_PKT_CAPTURE           Unused           Unused               0
// SCOPE_PKT_STOP              Unused           Unused               0
// SCOPE_PKT_START             Unused           Unused               0
// SCOPE_PKT_SET_TRIGGER_LEVEL Unused           Level in mV          0
// SCOPE_PKT_RETRANSMIT        Unused           Unused               0
// SCOPE_PKT_SET_CHANNEL2      Enable/Disable   Unused               0
// SCOPE_PKT_PING              Echo1            Echo2                0
// SCOPE_PKT_PING_RESPONSE     Echo1            Echo2                0
// SCOPE_PKT_DATA_CONTROL      Enable/Disable   Unused               0
// SCOPE_PKT_FIND              Channel Number   Unused               0
// SCOPE_PKT_SET_POSITION      Channel Number   Position in mV       0
// SCOPE_PKT_SET_SCALE         Channel Number   Scale in mV/div      0
//
// Device to Host Status
// ---------------------
//
// This section details the use of packet-specific parameters in the
// tScopePacket header structure for all status packets sent from the device
// to the host. As for host to device commands, the ucVersion field must be the
// protocol version number (SCOPE_PROTOCOL_VERSION_1 currently) and ucHdrLength
// must be set to sizeof(tScopePacket) which will equal 12 assuming your
// compiler packing macros are correctly configured.
//
// ucPacketType                ucParam          ulParam        ulDataLength
// ------------                -------          -------        ------------
// SCOPE_PKT_HELLO_RESPONSE    Protocol         Unused     size tScopeSettings
// SCOPE_PKT_TIMEBASE_UPDATED  Unused           uS/Division          0
// SCOPE_PKT_TRIGGER_TYPE      Channel Number   Trigger Type         0
// SCOPE_PKT_TRIGGER_LEVEL     Unused           Level in mV          0
// SCOPE_PKT_TRIGGER_POS       Unused           Trigger Pos          0
// SCOPE_PKT_CHANNEL2          Enable/Disable   Unused               0
// SCOPE_PKT_STARTED           Unused           Unused               0
// SCOPE_PKT_STOPPED           Unused           Unused               0
// SCOPE_PKT_DATA_START        0                Elements   size tScopeDataStart
// SCOPE_PKT_DATA              Packet Count     Elements       Payload size
// SCOPE_PKT_DATA_END          Packet Count     Unused               0
// SCOPE_PKT_PING              Echo1            Echo2                0
// SCOPE_PKT_PING_RESPONSE     Echo1            Echo2                0
// SCOPE_PKT_POSITION          Channel Number   Position in mV       0
// SCOPE_PKT_SCALE             Channel Number   Scale in mV/div      0
//
//*****************************************************************************

//*****************************************************************************
//
// All structures defined in this header require byte packing of fields. This
// is usually accomplished using the PACKED modifier macro but, for IAR
// Embedded Workbench, we need to issue a pragma so...
//
//*****************************************************************************
#ifdef ewarm
#pragma pack(1)
#endif

typedef struct
{
    unsigned char ucVersion;
    unsigned char ucHdrLength;
    unsigned char ucPacketType;
    unsigned char ucParam;
    unsigned long ulParam;
    unsigned long ulDataLength;
}
PACKED tScopePacket;

//*****************************************************************************
//
// The protocol version number associated with the definitions in this header
// file.
//
//*****************************************************************************
#define SCOPE_PROTOCOL_VERSION_1 0x01

//*****************************************************************************
//
// Packet types (tScopePacket.ucPacketType) for packets sent from the host
// to the device.
//
//*****************************************************************************

//
// Sent from the host to initiate communication. The ucVersion field of
// tScopePacket indicates the highest protocol version number the host
// supports. The host must ensure that it uses the protocol version which
// appears in the SCOPE_PKT_HELLO_RESPONSE sent from the device in response
// to this packet for all future transactions.
//
// ucParam      - Unused
// ulParam      - Unused
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_HOST_HELLO 0x00

//
// Sent from the host to terminate communication.
//
// ucParam      - Unused
// ulParam      - Unused
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_HOST_GOODBYE 0x01

//
// Sent from the host to set the trigger type to be used in the following
// capture requests.
//
// ucParam      - SCOPE_CHANNEL_1 or SCOPE_CHANNEL_2 to indicate the channel
//                on which triggering is to take place.
// ulParam      - SCOPE_TRIGGER_TYPE_LEVEL, SCOPE_TRIGGER_TYPE_RISING,
//                SCOPE_TRIGGER_TYPE_FALLING or SCOPE_TRIGGER_TYPE_ALWAYS.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_SET_TRIGGER_TYPE 0x02

// Sent from the host to request a change in the timebase. This affects both
// the display and also the sample capture rate.
//
// ucParam      - Unused
// ulParam      - The new timebase expressed in microseconds per division.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_SET_TIMEBASE 0x03

//
// Sent from the host to request a single capture cycle. This command is
// ignored if continuous capture is currently ongoing.
//
// ucParam      - Unused
// ulParam      - Unused
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_CAPTURE 0x04

//
// Sent from the host to stop continuous capture of data.
//
// ucParam      - Unused
// ulParam      - Unused
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_STOP 0x05

//
// Sent from the host to start continuous capture of data.
//
// ucParam      - Unused
// ulParam      - Unused
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_START 0x06

//
// Sent from the host to set the trigger level.
//
// ucParam      - Unused
// ulParam      - The desired trigger level in millivolts.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_SET_TRIGGER_LEVEL 0x07

//
// Sent from the host to set the trigger position.
//
// ucParam      - Unused
// ulParam      - The desired trigger position in pixels (-60, 60).
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_SET_TRIGGER_POS 0x08

//
// Sent from the host to request retransmission of the last data set captured.
// Note that this packet will initiate transmission of a data set even if
// automatic data capture has not previously been enabled via a
// SCOPE_PKT_DATA_CONTROL packet.
//
// ucParam      - Unused
// ulParam      - Unused
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_RETRANSMIT 0x09

//
// Sent from the host to enable or disable capture of data on channel 2
//
// ucParam      - SCOPE_CHANNEL2_DISABLE or SCOPE_CHANNEL2_ENABLE
// ulParam      - Unused
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_SET_CHANNEL2 0x0A

//
// Sent either from the host or device to enquire as to whether or not the
// communication link is still active. The parameters passed in the packet
// will be returned in the matching SCOPE_PKT_PING_RESPONSE packet.
//
// ucParam      - Sender's choice.
// ulParam      - Sender's choice
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_PING 0x0B

//
// Sent from the host to instruct the device to start or stop automatic
// transmission of captured waveform data. By default, the device will not
// transmit SCOPE_PKT_DATA_START/SCOPE_PKT_DATA/SCOPE_PKT_DATA_END unless
// this packet has been sent with a non-zero value in ucParam to enable
// data flow.
//
// ucParam      - Zero to disable automatic data transmission, non-zero to
//                enable it.
// ulParam      - Unused.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_DATA_CONTROL 0x0C

//
// Sent from the host to instruct the device set the offset and scale
// for the given channel to ensure that the waveform is visible on the
// screen. This packet will result in the return of two packets,
// SCOPE_PKT_POSITION and SCOPE_PKT_SCALE indicating the calculated position
// and scale settings.
//
// ucParam      - SCOPE_CHANNEL_1 or SCOPE_CHANNEL_2 to indicate the channel
//                whose display position and scale is to be set.
// ulParam      - Unused.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_FIND 0x0D

//
// Sent from the host to instruct the device to set the vertical position
// offset for a given channel. A SCOPE_PKT_POSITION packet will be returned by
// the device in response to this request.
//
// ucParam      - SCOPE_CHANNEL_1 or SCOPE_CHANNEL_2 to indicate the channel
//                whose display position is to be set.
// ulParam      - Position offset in mV (signed).
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_SET_POSITION 0x0E

//
// Sent from the host to instruct the device to set the vertical scaling
// offset for a given channel. A SCOPE_PKT_SCALE packet will be returned by
// the device in response to this request.
//
// ucParam      - SCOPE_CHANNEL_1 or SCOPE_CHANNEL_2 to indicate the channel
//                whose vertical scale is to be set.
// ulParam      - Scale in mV per division.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_SET_SCALE 0x0F

//*****************************************************************************
//
// Packet types (tScopePacket.ucPacketType) for packets sent from the device
// to the host.
//
//*****************************************************************************

//
// Sent from the device in response to SCOPE_PKT_HOST_HELLO. The ucVersion
// field of tScopePacket must be set to the protocol version that will be
// used for all future communication. This will be the lower of the version
// supported by the device and the version requested by the host in the
// SCOPE_PKT_HOST_HELLO packet. Both host and device must ensure that they
// use this protocol in all future transactions.
//
// ucParam      - Unused
// ulParam      - Unused
// ulDataLength - sizeof(tScopeSettings)
//
// A tScopeSettings structure is sent following the tScopePacket header. This
// contains the current oscilloscope settings at the device site and allows
// the application to initialize any user interface controls that mirror
// those settings.
//
#define SCOPE_PKT_HELLO_RESPONSE 0x80

//
// Sent from the device whenever the timebase is update either via the device
// user interface or as a result of a SCOPE_PKT_SET_TIMEBASE packet from the
// host.
//
// ucParam      - Unused
// ulParam      - The new timebase expressed in microseconds per division.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_TIMEBASE_UPDATED 0x81

//
// Sent from the device whenever the trigger type is changed either via the
// device user interface or as a result of a SCOPE_PKT_SET_TRIGGER_TYPE
// packet from the host.
//
// ucParam      - SCOPE_CHANNEL_1 or SCOPE_CHANNEL_2 to indicate the channel
//                on which triggering is to take place.
// ulParam      - SCOPE_TRIGGER_TYPE_LEVEL, SCOPE_TRIGGER_TYPE_RISING,
//                SCOPE_TRIGGER_TYPE_FALLING or SCOPE_TRIGGER_TYPE_ALWAYS.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_TRIGGER_TYPE 0x82

//
// Sent from the device whenever the trigger level is changed either via the
// device user interface or as a result of a SCOPE_PKT_SET_TRIGGER_LEVEL
// packet from the host.
//
// ucParam      - Unused
// ulParam      - The new trigger level in millivolts.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_TRIGGER_LEVEL 0x83

//
// Sent from the device whenever the trigger position is changed either via
// the device user interface or as a result of a SCOPE_PKT_SET_TRIGGER_POS
// packet from the host.
//
// ucParam      - Unused
// ulParam      - The new trigger position in pixels from center (-60,60).
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_TRIGGER_POS 0x84

//
// Sent from the device whenever channel 2 is enabled or disabled either via
// the device user interface or as a result of a SCOPE_PKT_SET_CHANNEL2
// packet from the host.
//
// ucParam      - SCOPE_CHANNEL2_DISABLE or SCOPE_CHANNEL2_ENABLE
// ulParam      - Unused
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_CHANNEL2 0x85

//
// Sent from the device in preparation for transmission of a capture data
// set to the host.
//
// ucParam      - 0
// ulParam      - Total number of tScopeDataElement or tScopeDualDataElement
//                structures that will be transmitted to complete the data
//                set. The type of structure that the host should expect
//                depends upon the bDualChannel field of the tScopeStartData
//                structure passed in the payload of this packet.
// ulDataLength - sizeof(tScopeStartData)
//
// A fully populated tScopeStartData structure follows the tScopePacket
// structure in this packet.
//
#define SCOPE_PKT_DATA_START 0x86

//
// Sent from the device to pass a portion of a captured data set to the host.
//
// ucParam      - Incrementing packet counter. The first SCOPE_PKT_DATA
//                packet for a data set must have ucParam set to 1 with
//                subsequent packets increasing this value by 1 on each
//                packet.
// ulParam      - The number of tScopeDataElement or tScopeDualDataElement
//                structures contained in the payload of this packet.
// ulDataLength - The number of bytes of payload data following. This will
//                be either (ulParam * sizeof(tScopeDataElement)) or
//                (ulParam * sizeof(tScopeDualDataElement)) depending upon
//                whether single or dual channel data is being returned.
//
// A packed array of ulParam tScopeDataElement or tScopeDualDataElement
// structures follows the tScopePacket header in this packet.
//
#define SCOPE_PKT_DATA 0x87

//
// Sent from the device to indicate that transmission of a data set has been
// completed.
//
// ucParam      - Final packet count. This value must be 1 greater than the
//                packet count passed in the final SCOPE_PKT_DATA packet for
//                this data set.
// ulParam      - Unused
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_DATA_END 0x88

//
// Sent either from the host or device in response to an incoming packet of
// type SCOPE_PKT_PING.
//
// ucParam      - From matching PING packet.
// ulParam      - From matching PING packet.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_PING_RESPONSE 0x89

//
// Sent from the device when automatic capture is started.
//
// ucParam      - Unused.
// ulParam      - Unused.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_STARTED 0x8A

//
// Sent from the device when automatic capture is stopped.
//
// ucParam      - Unused.
// ulParam      - Unused.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_STOPPED 0x8B

//
// Sent from the device to inform the host of a change in the vertical position
// offset for a given channel.
//
// ucParam      - SCOPE_CHANNEL_1 or SCOPE_CHANNEL_2 to indicate the channel
//                whose display position has been changed.
// ulParam      - Position offset in mV (signed).
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_POSITION 0x8C

//
// Sent from the device to inform the host of a change in the vertical
// scale for a given channel.
//
// ucParam      - SCOPE_CHANNEL_1 or SCOPE_CHANNEL_2 to indicate the channel
//                whose vertical scale is to be set.
// ulParam      - Scale in mV per division.
// ulDataLength - 0 (no additional payload).
//
#define SCOPE_PKT_SCALE 0x8D

//*****************************************************************************
//
// Payload of a SCOPE_PKT_HELLO_RESPONSE packet. This structure provides
// information on the current state of various user-configurable settings
// allowing the host to appropriately configure its user interface controls.
//
//*****************************************************************************
typedef struct
{
    unsigned long ulTriggerLevelmV;
    unsigned long ulTimebaseuS;
    long lTriggerPos;
    short sChannel1OffsetmV;
    short sChannel2OffsetmV;
    unsigned short usChannel1ScalemVdiv;
    unsigned short usChannel2ScalemVdiv;
    unsigned char ucChannel2Enabled;
    unsigned char ucTriggerType;
    unsigned char ucStarted;
    unsigned char ucTriggerChannel;
}
PACKED tScopeSettings;

//*****************************************************************************
//
// Payload of a SCOPE_PKT_DATA_START packet. This structure provides timing
// information about the dataset that will follow in SCOPE_PKT_DATA packets.
//
//*****************************************************************************
typedef struct
{
    //
    // This field contains the time difference between consecutive samples
    // from the same channel expressed in microseconds.
    //
    unsigned long ulSampleOffsetuS;

    //
    // When dual channel data is being returned, this field contains
    // information on the number of microseconds between capture of the sample
    // returned in tScopeDualDataElement.usSample1mVolts and
    // tScopeDualDataElement.usSample2mVolts with usSample2mVolts having
    // been captured later than Sample1mVolts. This information may be used to
    // correctly position the channel 2 waveform on the display.
    //
    // When single channel data is being returned, this value can be ignored.
    //
    unsigned long ulSamplePerioduS;

    //
    // The index the sample at which the trigger event was detected. The index
    // indicates which tScopeDualDataElement or tScopeDataElement structure
    // to be returned in following SCOPE_PKT_DATA packets contains the trigger.
    // Triggers are always associated with the sample in the usSample1mVolts
    // field. The trigger channel can be inferred from the bCh2SampleFirst
    // field below.
    //
    unsigned long ulTriggerIndex;

    //
    // The total number of data elements which are to be transmitted in
    // follow-on SCOPE_PKT_DATA packets. Each element contains either one
    // or two samples depending upon the state of bDualChannel below.
    //
    unsigned long ulTotalElements;

    //
    //
    // If non-zero, this field indicates that the following SCOPE_PKT_DATA
    // packets contain dual channel data represented using
    // tScopeDualElementData structures. If zero, the following SCOPE_PKT_DATA
    // packets contain single channel data represented using tScopeDataElement
    // structures.
    //
    unsigned char bDualChannel;

    //
    // If bDualChannel is non-zero then this field indicates the order of the
    // samples to be returned in the following SCOPE_PKT_DATA packets. If
    // zero, usSample1mVolts contains a sample for channel 1 of the oscilloscope
    // and usSample2mVolts contains a channel 2 sample. If non-zero, the
    // channels are swapped with usSample1mVolts containing a channel 2 sample
    // and usSample2mVolts containing a channel 1 sample.
    //
    unsigned char bCh2SampleFirst;
}
PACKED tScopeDataStart;

//*****************************************************************************
//
// The payload of SCOPE_PKT_DATA_START packets consists of an integral number
// of tScopeDataElement or tScopeDualDataElement structures. tScopeDataElement
// is used to transfer data sets when only a single channel of data is in use.
// tScopeDualDataElement is used when dual channel capture is enabled.
//
//*****************************************************************************
typedef struct
{
    //
    // The time at which this sample was taken expressed as a microsecond
    // offset from the sample time for the first sample in the current
    // capture sequence.
    //
    unsigned long ulTimeuS;

    //
    // The sampled voltage at time ulTimeuS expressed in millivolts.
    //
    short sSamplemVolts;
}
PACKED tScopeDataElement;

typedef struct
{
    //
    // The time at which the usSample1mVolts sample was taken expressed as a
    // microsecond offset from the sample time for the first sample in the
    // current capture sequence.
    //
    unsigned long ulTimeuS;

    //
    // The sampled voltage at time ulTimeuS expressed in millivolts. The input
    // sampled to give this voltage is determined by the bCh2SampleFirst field
    // in tScopeDataStart previously sent in a SCOPE_PKT_DATA_START packet.
    //
    short sSample1mVolts;

    //
    // The sampled voltage at time (ulTimeuS + ulSampleOffsetuS) where value
    // ulSampleOffsetuS is the value previously sent in a SCOPE_PKT_DATA_START
    // packet. The input sampled to give this voltage is determined by the
    // bCh2SampleFirst field in tScopeDataStart previously sent in a
    // SCOPE_PKT_DATA_START packet.
    //
    short sSample2mVolts;
}
PACKED tScopeDualDataElement;

//*****************************************************************************
//
// tScopePacket.ucParam for SCOPE_PKT_SET_TRIGGER_TYPE and
// SCOPE_PKT_TRIGGER_TYPE.
//
//*****************************************************************************
#define SCOPE_CHANNEL_1         0x00
#define SCOPE_CHANNEL_2         0x01

//*****************************************************************************
//
// tScopePacket.ulParam for SCOPE_PKT_SET_TRIGGER_TYPE and
// SCOPE_PKT_TRIGGER_TYPE.
//
//*****************************************************************************
#define SCOPE_TRIGGER_TYPE_LEVEL     0x00000000
#define SCOPE_TRIGGER_TYPE_RISING    0x00000001
#define SCOPE_TRIGGER_TYPE_FALLING   0x00000002
#define SCOPE_TRIGGER_TYPE_ALWAYS    0x00000003

//*****************************************************************************
//
// tScopePacket.ucParam for SCOPE_PKT_SET_CHANNEL2 and SCOPE_PKT_CHANNEL2.
//
//*****************************************************************************
#define SCOPE_CHANNEL2_DISABLE  0x00
#define SCOPE_CHANNEL2_ENABLE   0x01

//*****************************************************************************
//
// Return to default packing when using the IAR Embedded Workbench compiler.
//
//*****************************************************************************
#ifdef ewarm
#pragma pack()
#endif

#endif // _USB_PROTOCOL_H_
