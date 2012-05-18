//*****************************************************************************
//
// config.h - Configuration of the serial to Ethernet converter.
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
// This is part of revision 8555 of the RDK-S2E Firmware Package.
//
//*****************************************************************************

#ifndef __CONFIG_H__
#define __CONFIG_H__

//*****************************************************************************
//
//! \addtogroup config_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// During debug, DEBUG_UART may be defined with values 0 or 1 to select which
// of the two UARTs are used to output debug messages.  Debug messages will be
// interleaved with any telnet data also being carried via that UART so great
// care must be exercised when enabling this debug option.  Typically, you
// should use only a single telnet connection when debugging with DEBUG_UART
// set to use the inactive UART.
//
//*****************************************************************************
#ifdef DEBUG_UART
#define DEBUG_MSG UARTprintf
#else
#define DEBUG_MSG while(0) ((int (*)(char *, ...))0)
#endif

//*****************************************************************************
//
//! The number of serial to Ethernet ports supported by this module.
//
//*****************************************************************************
#define MAX_S2E_PORTS           2

//*****************************************************************************
//
//! This structure defines the port parameters used to configure the UART and
//! telnet session associated with a single instance of a port on the S2E
//! module.
//
//*****************************************************************************
typedef struct
{
    //
    //! The baud rate to be used for the UART, specified in bits-per-second
    //! (bps).
    //
    unsigned long ulBaudRate;

    //
    //! The data size to be use for the serial port, specified in bits.  Valid
    //! values are 5, 6, 7, and 8.
    //
    unsigned char ucDataSize;

    //
    //! The parity to be use for the serial port, specified as an enumeration.
    //! Valid values are 1 for no parity, 2 for odd parity, 3 for even parity,
    //! 4 for mark parity, and 5 for space parity.
    //
    unsigned char ucParity;

    //
    //! The number of stop bits to be use for the serial port, specified as
    //! a number.  Valid values are 1 and 2.
    //
    unsigned char ucStopBits;

    //
    //! The flow control to be use for the serial port, specified as an
    //! enumeration.  Valid values are 1 for no flow control and 3 for HW
    //! (RTS/CTS) flow control.
    //
    unsigned char ucFlowControl;

    //
    //! The timeout for the TCP connection used for the telnet session,
    //! specified in seconds.  A value of 0 indicates no timeout is to
    //! be used.
    //
    unsigned long ulTelnetTimeout;

    //
    //! The local TCP port number to be listened on when in server mode or
    //! from which the outgoing connection will be initiated in client mode.
    //
    unsigned short usTelnetLocalPort;

    //
    //! The TCP port number to which a connection will be established when in
    //! telnet client mode.
    //
    unsigned short usTelnetRemotePort;

    //
    //! The IP address which will be connected to when in telnet client mode.
    //
    unsigned long ulTelnetIPAddr;

    //
    //! Miscellaneous flags associated with this connection.
    //
    unsigned char ucFlags;

    //! Padding to ensure consistent parameter block alignment, and
    //! to allow for future expansion of port parameters.
    //
    unsigned char ucReserved0[19];
}
tPortParameters;

//*****************************************************************************
//
//! Bit 0 of field ucFlags in tPortParameters is used to indicate whether to
//! operate as a telnet server or a telnet client.
//
//*****************************************************************************
#define PORT_FLAG_TELNET_MODE   0x01

//*****************************************************************************
//
// Helpful labels used to determine if we are operating as a telnet client or
// server (based on the state of the PORT_FLAG_TELNET_MODE bit in the ucFlags
// field of tPortParameters).
//
//*****************************************************************************
#define PORT_TELNET_SERVER      0x00
#define PORT_TELNET_CLIENT      PORT_FLAG_TELNET_MODE

//*****************************************************************************
//
//! Bit 1 of field ucFlags in tPortParameters is used to indicate whether to
//! bypass the telnet protocol (raw data mode).
//
//*****************************************************************************
#define PORT_FLAG_PROTOCOL      0x02

//*****************************************************************************
//
// Helpful labels used to determine if we are operating in raw data mode, or
// in telnet protocol mode.
//
//*****************************************************************************
#define PORT_PROTOCOL_TELNET    0x00
#define PORT_PROTOCOL_RAW       PORT_FLAG_PROTOCOL

//*****************************************************************************
//
// The length of the ucModName array in the tConfigParameters structure.  NOTE:
// Be extremely careful if changing this since the structure packing relies
// upon this values!
//
//*****************************************************************************
#define MOD_NAME_LEN            40

//*****************************************************************************
//
//! This structure contains the S2E module parameters that are saved to flash.
//! A copy exists in RAM for use during the execution of the application, which
//! is loaded from flash at startup.  The modified parameter block can also be
//! written back to flash for use on the next power cycle.
//
//*****************************************************************************
typedef struct
{
    //
    //! The sequence number of this parameter block.  When in RAM, this value
    //! is not used.  When in flash, this value is used to determine the
    //! parameter block with the most recent information.
    //
    unsigned char ucSequenceNum;

    //
    //! The CRC of the parameter block.  When in RAM, this value is not used.
    //! When in flash, this value is used to validate the contents of the
    //! parameter block (to avoid using a partially written parameter block).
    //
    unsigned char ucCRC;

    //
    //! The version of this parameter block.  This can be used to distinguish
    //! saved parameters that correspond to an old version of the parameter
    //! block.
    //
    unsigned char ucVersion;

    //
    //! Character field used to store various bit flags.
    //
    unsigned char ucFlags;

    //
    //! The TCP port number to be used for access to the UPnP Location URL that
    //! is part of the discovery response message.
    //
    unsigned short usLocationURLPort;

    //
    //! Padding to ensure consistent parameter block alignment.
    //
    unsigned char ucReserved1[2];

    //
    //! The configuration parameters for each port available on the S2E
    //! module.
    //
    tPortParameters sPort[MAX_S2E_PORTS];

    //
    //! An ASCII string used to identify the module to users via UPnP and
    //! web configuration.
    //
    unsigned char ucModName[MOD_NAME_LEN];

    //
    //! The static IP address to use if DHCP is not in use.
    //
    unsigned long ulStaticIP;

    //
    //! The default gateway IP address to use if DHCP is not in use.
    //
    unsigned long ulGatewayIP;

    //
    //! The subnet mask to use if DHCP is not in use.
    //
    unsigned long ulSubnetMask;

    //
    //! Padding to ensure the whole structure is 256 bytes long.
    //
    unsigned char ucReserved2[108];
}
tConfigParameters;

//*****************************************************************************
//
//! If this flag is set in the ucFlags field of tConfigParameters, the module
//! uses a static IP.  If not, DHCP and AutoIP are used to obtain an IP
//! address.
//
//*****************************************************************************
#define CONFIG_FLAG_STATICIP    0x80

//*****************************************************************************
//
//! The address of the first block of flash to be used for storing parameters.
//
//*****************************************************************************
#define FLASH_PB_START          0x00017800

//*****************************************************************************
//
//! The address of the last block of flash to be used for storing parameters.
//! Since the end of flash is used for parameters, this is actually the first
//! address past the end of flash.
//
//*****************************************************************************
#define FLASH_PB_END            0x00018000

//*****************************************************************************
//
//! The size of the parameter block to save.  This must be a power of 2,
//! and should be large enough to contain the tConfigParameters structure.
//
//*****************************************************************************
#define FLASH_PB_SIZE           256

//*****************************************************************************
//
//! The size of the ring buffers used for interface between the UART and
//! telnet session (RX).
//
//*****************************************************************************
#define RX_RING_BUF_SIZE        (256 * 2)

//*****************************************************************************
//
//! The size of the ring buffers used for interface between the UART and
//! telnet session (TX).
//
//*****************************************************************************
#define TX_RING_BUF_SIZE        (256 * 6)

//*****************************************************************************
//
//! Enable RFC-2217 (COM-PORT-OPTION) in the telnet server code.
//
//*****************************************************************************
#define CONFIG_RFC2217_ENABLED  1

//*****************************************************************************
//
//! The GPIO port on which the XVR_ON pin resides.
//
//*****************************************************************************
#define PIN_XVR_ON_PORT         GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the XVR_ON pin resides.
//
//*****************************************************************************
#define PIN_XVR_ON_PIN          GPIO_PIN_4

//*****************************************************************************
//
//! The GPIO port on which the XVR_OFF_N pin resides.
//
//*****************************************************************************
#define PIN_XVR_OFF_N_PORT      GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the XVR_OFF_N pin resides.
//
//*****************************************************************************
#define PIN_XVR_OFF_N_PIN       GPIO_PIN_5

//*****************************************************************************
//
//! The GPIO port on which the XVR_INV_N pin resides.
//
//*****************************************************************************
#define PIN_XVR_INV_N_PORT      GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the XVR_INV_N pin resides.
//
//*****************************************************************************
#define PIN_XVR_INV_N_PIN       GPIO_PIN_2

//*****************************************************************************
//
//! The GPIO port on which the XVR_RDY pin resides.
//
//*****************************************************************************
#define PIN_XVR_RDY_PORT        GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the XVR_RDY pin resides.
//
//*****************************************************************************
#define PIN_XVR_RDY_PIN         GPIO_PIN_3

//*****************************************************************************
//
//! The GPIO port on which the U0RX pin resides.
//
//*****************************************************************************
#define PIN_U0RX_PORT           GPIO_PORTA_BASE

//*****************************************************************************
//
//! The GPIO pin on which the U0RX pin resides.
//
//*****************************************************************************
#define PIN_U0RX_PIN            GPIO_PIN_0

//*****************************************************************************
//
//! The GPIO port on which the U0TX pin resides.
//
//*****************************************************************************
#define PIN_U0TX_PORT           GPIO_PORTA_BASE

//*****************************************************************************
//
//! The GPIO pin on which the U0TX pin resides.
//
//*****************************************************************************
#define PIN_U0TX_PIN            GPIO_PIN_1

//*****************************************************************************
//
//! The GPIO port on which the U0RTS pin resides.
//
//*****************************************************************************
#define PIN_U0RTS_PORT          GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the U0RTS pin resides.
//
//*****************************************************************************
#define PIN_U0RTS_PIN           GPIO_PIN_1

//*****************************************************************************
//
//! The interrupt on which the U0RTS pin resides.
//
//*****************************************************************************
#define PIN_U0RTS_INT           INT_GPIOB

//*****************************************************************************
//
//! The GPIO port on which the U0CTS pin resides.
//
//*****************************************************************************
#define PIN_U0CTS_PORT          GPIO_PORTB_BASE

//*****************************************************************************
//
//! The GPIO pin on which the U0CTS pin resides.
//
//*****************************************************************************
#define PIN_U0CTS_PIN           GPIO_PIN_0

//*****************************************************************************
//
//! The GPIO port on which the U1RX pin resides.
//
//*****************************************************************************
#define PIN_U1RX_PORT           GPIO_PORTD_BASE

//*****************************************************************************
//
//! The GPIO pin on which the U1RX pin resides.
//
//*****************************************************************************
#define PIN_U1RX_PIN            GPIO_PIN_2

//*****************************************************************************
//
//! The GPIO port on which the U1TX pin resides.
//
//*****************************************************************************
#define PIN_U1TX_PORT           GPIO_PORTD_BASE

//*****************************************************************************
//
//! The GPIO pin on which the U1TX pin resides.
//
//*****************************************************************************
#define PIN_U1TX_PIN            GPIO_PIN_3

//*****************************************************************************
//
//! The GPIO port on which the U1RTS pin resides.
//
//*****************************************************************************
#define PIN_U1RTS_PORT          GPIO_PORTA_BASE

//*****************************************************************************
//
//! The GPIO pin on which the U1RTS pin resides.
//
//*****************************************************************************
#define PIN_U1RTS_PIN           GPIO_PIN_2

//*****************************************************************************
//
//! The interrupt on which the U1RTS pin resides.
//
//*****************************************************************************
#define PIN_U1RTS_INT           INT_GPIOA

//*****************************************************************************
//
//! The GPIO port on which the U1CTS pin resides.
//
//*****************************************************************************
#define PIN_U1CTS_PORT          GPIO_PORTA_BASE

//*****************************************************************************
//
//! The GPIO pin on which the U1CTS pin resides.
//
//*****************************************************************************
#define PIN_U1CTS_PIN           GPIO_PIN_3

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// A flag to indicate that a firmware update has been requested.
//
//*****************************************************************************
extern tBoolean g_bStartBootloader;

//*****************************************************************************
//
// Flags sent to the main loop indicating that it should update the IP address
// after a short delay (to allow us to send a suitable page back to the web
// browser telling it the address has changed).
//
//*****************************************************************************
extern unsigned char g_cUpdateRequired;

#define UPDATE_IP_ADDR 0x01
#define UPDATE_ALL     0x02

//*****************************************************************************
//
// Prototypes for the globals exported from the configuration module, along
// with public API function prototypes.
//
//*****************************************************************************
extern tConfigParameters g_sParameters;
extern const tConfigParameters *g_psDefaultParameters;
extern const tConfigParameters *const g_psFactoryParameters;
extern void ConfigInit(void);
extern void ConfigLoadFactory(void);
extern void ConfigLoad(void);
extern void ConfigSave(void);
extern void ConfigWebInit(void);
extern void ConfigUpdateIPAddress(void);
extern void ConfigUpdateAllParameters(tBoolean bUpdateIP);

#endif // __CONFIG_H__
