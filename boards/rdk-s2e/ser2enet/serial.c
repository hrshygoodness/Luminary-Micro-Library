//*****************************************************************************
//
// serial.c - Serial port driver for S2E Module.
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

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/ringbuf.h"
#include "config.h"
#include "serial.h"
#include "telnet.h"

//*****************************************************************************
//
//! \addtogroup serial_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! The buffer used to hold characters received from the UART0.
//
//*****************************************************************************
static unsigned char g_pucRX0Buffer[RX_RING_BUF_SIZE];

//*****************************************************************************
//
//! The buffer used to hold characters to be sent to the UART0.
//
//*****************************************************************************
static unsigned char g_pucTX0Buffer[TX_RING_BUF_SIZE];

//*****************************************************************************
//
//! The buffer used to hold characters received from the UART1.
//
//*****************************************************************************
static unsigned char g_pucRX1Buffer[RX_RING_BUF_SIZE];

//*****************************************************************************
//
//! The buffer used to hold characters to be sent to the UART1.
//
//*****************************************************************************
static unsigned char g_pucTX1Buffer[TX_RING_BUF_SIZE];

//*****************************************************************************
//
//! The ring buffers used to hold characters received from the UARTs.
//
//*****************************************************************************
static tRingBufObject g_sRxBuf[MAX_S2E_PORTS];

//*****************************************************************************
//
//! The ring buffers used to hold characters to be sent to the UARTs.
//
//*****************************************************************************
static tRingBufObject g_sTxBuf[MAX_S2E_PORTS];

//*****************************************************************************
//
//! The base address for the UART associated with a port.
//
//*****************************************************************************
static const unsigned long g_ulUARTBase[MAX_S2E_PORTS] =
{
    UART0_BASE,
    UART1_BASE
};

//*****************************************************************************
//
//! The interruopt for the UART associated with a port.
//
//*****************************************************************************
static const unsigned long g_ulUARTInterrupt[MAX_S2E_PORTS] =
{
    INT_UART0,
    INT_UART1
};

//*****************************************************************************
//
//! The interrupt for the GPIO Flow Control pin associated with a port.
//
//*****************************************************************************
static const unsigned long g_ulFlowInterrupt[MAX_S2E_PORTS] =
{
    PIN_U0RTS_INT,
    PIN_U1RTS_INT
};

//*****************************************************************************
//
//! The GPIO base address for the flow control output pin.
//
//*****************************************************************************
static const unsigned long g_ulFlowOutBase[MAX_S2E_PORTS] =
{
    PIN_U0CTS_PORT,
    PIN_U1CTS_PORT
};

//*****************************************************************************
//
//! The GPIO base address for the flow control input pin.
//
//*****************************************************************************
static const unsigned long g_ulFlowInBase[MAX_S2E_PORTS] =
{
    PIN_U0RTS_PORT,
    PIN_U1RTS_PORT
};

//*****************************************************************************
//
//! The GPIO pin number for the flow control output pin.
//
//*****************************************************************************
static const unsigned long g_ulFlowOutPin[MAX_S2E_PORTS] =
{
    PIN_U0CTS_PIN,
    PIN_U1CTS_PIN
};

//*****************************************************************************
//
//! The GPIO pin number for the flow control input pin.
//
//*****************************************************************************
static const unsigned long g_ulFlowInPin[MAX_S2E_PORTS] =
{
    PIN_U0RTS_PIN,
    PIN_U1RTS_PIN
};

//*****************************************************************************
//
//! The status of the flow control output based on SerialSetFlowOut().
//
//*****************************************************************************
static unsigned long g_ulSetFlowOutValue[MAX_S2E_PORTS] =
{
    0,
    0
};

//*****************************************************************************
//
//! The current baud rate setting of the serial port
//
//*****************************************************************************
static unsigned long g_ulCurrentBaudRate[MAX_S2E_PORTS] =
{
    0,
    0
};

//*****************************************************************************
//
//! Handles the UART interrupt.
//!
//! \param ulPort is the serial port number to be accessed.
//!
//! This function is called when either of the UARTs generate an interrupt.
//! An interrupt will be generated when data is received and when the transmit
//! FIFO becomes half empty.  The transmit and receive FIFOs are processed as
//! appropriate.
//!
//! \return None.
//
//*****************************************************************************
static void
SerialUARTIntHandler(unsigned long ulPort)
{
    unsigned long ulStatus;
    unsigned char ucChar;

    //
    // Get the cause of the interrupt.
    //
    ulStatus = UARTIntStatus(g_ulUARTBase[ulPort], true);

    //
    // Clear the cause of the interrupt.
    //
    UARTIntClear(g_ulUARTBase[ulPort], ulStatus);

    //
    // See if there is data to be processed in the receive FIFO.
    //
    if(ulStatus & (UART_INT_RT | UART_INT_RX))
    {
        //
        // Loop while there are characters available in the receive FIFO.
        //
        while(UARTCharsAvail(g_ulUARTBase[ulPort]))
        {
            //
            // Get the next character from the receive FIFO.
            //
            ucChar = UARTCharGet(g_ulUARTBase[ulPort]);

            //
            // If Telnet protocol enabled, check for incoming IAC character,
            // and escape it.
            //
            if((g_sParameters.sPort[ulPort].ucFlags &
                        PORT_FLAG_PROTOCOL) == PORT_PROTOCOL_TELNET)
            {
                //
                // If this is a Telnet IAC character, write it twice.
                //
                if((ucChar == TELNET_IAC) &&
                   (RingBufFree(&g_sRxBuf[ulPort]) >= 2))
                {
                    RingBufWriteOne(&g_sRxBuf[ulPort], ucChar);
                    RingBufWriteOne(&g_sRxBuf[ulPort], ucChar);
                }

                //
                // If not a Telnet IAC character, only write it once.
                //
                else if((ucChar != TELNET_IAC) &&
                        (RingBufFree(&g_sRxBuf[ulPort]) >= 1))
                {
                    RingBufWriteOne(&g_sRxBuf[ulPort], ucChar);
                }
            }

            //
            // if not Telnet, then only write the data once.
            //
            else
            {
                RingBufWriteOne(&g_sRxBuf[ulPort], ucChar);
            }
        }
    }

    //
    // If flow control is enabled, check the status of the RX buffer to
    // determine if flow control GPIO needs to be asserted.
    //
    if(g_sParameters.sPort[ulPort].ucFlowControl == SERIAL_FLOW_CONTROL_HW)
    {
        //
        // If the ring buffer is down to less than 25% free, assert the
        // outbound flow control pin.
        //
        if(RingBufFree(&g_sRxBuf[ulPort]) <
           (RingBufSize(&g_sRxBuf[ulPort]) / 4))
        {
            GPIOPinWrite(g_ulFlowOutBase[ulPort], g_ulFlowOutPin[ulPort],
                         g_ulFlowOutPin[ulPort]);
        }
    }

    //
    // See if there is space to be filled in the transmit FIFO.
    //
    if(ulStatus & UART_INT_TX)
    {
        //
        // Loop while there is space in the transmit FIFO and characters to be
        // sent.
        //
        while(!RingBufEmpty(&g_sTxBuf[ulPort]) &&
                UARTSpaceAvail(g_ulUARTBase[ulPort]))
        {
            //
            // Write the next character into the transmit FIFO.
            //
            UARTCharPut(g_ulUARTBase[ulPort],
                        RingBufReadOne(&g_sTxBuf[ulPort]));
        }
    }
}

//*****************************************************************************
//
//! Handles the UART0 interrupt.
//!
//! This function is called when the UART generates an interrupt.  An interrupt
//! will be generated when data is received and when the transmit FIFO becomes
//! half empty.  The transmit and receive FIFOs are processed as appropriate.
//!
//! \return None.
//
//*****************************************************************************
void
SerialUART0IntHandler(void)
{
    SerialUARTIntHandler(0);
}

//*****************************************************************************
//
//! Handles the UART1 interrupt.
//!
//! This function is called when the UART generates an interrupt.  An interrupt
//! will be generated when data is received and when the transmit FIFO becomes
//! half empty.  The transmit and receive FIFOs are processed as appropriate.
//!
//! \return None.
//
//*****************************************************************************
void
SerialUART1IntHandler(void)
{
    SerialUARTIntHandler(1);
}

//*****************************************************************************
//
//! Handles the serial flow control interrupt.
//!
//! \param ulPort is the serial port number to be accessed.
//!
//! This function is called when the GPIO interrupt occurs for a serial port
//! flow control GPIO input.  This function will clear the appropriate
//! interrupt condition, and will toggle the transmitter status based on the
//! flow control input signal level and the flow control setting.
//!
//! \return None.
//
//*****************************************************************************
static void
SerialFlowInIntHandler(unsigned long ulPort)
{
    unsigned long ulStatus;
#if CONFIG_RFC2217_ENABLED
    unsigned char ucModemState;
#endif

    //
    // Clear the interrupt condition.
    //
    ulStatus = GPIOPinIntStatus(g_ulFlowInBase[ulPort], true);
    GPIOPinIntClear(g_ulFlowInBase[ulPort], ulStatus);

    //
    // If flow control is enabled, check the status of the pin and
    // enable/disable the UART transmitter appropriately.
    //
    if(g_sParameters.sPort[ulPort].ucFlowControl == SERIAL_FLOW_CONTROL_HW)
    {
        if(GPIOPinRead(g_ulFlowInBase[ulPort], g_ulFlowInPin[ulPort]))
        {
            //
            // Indicate the CTS (actually flow control input) has changed
            // state to be asserted.
            //
#if CONFIG_RFC2217_ENABLED
            ucModemState = (16 + 1);
#endif
            HWREG(g_ulUARTBase[ulPort] + UART_O_CTL) &= ~UART_CTL_TXE;
        }
        else
        {
            //
            // Indicate the CTS (actually flow control input) has changed
            // state to be deasserted.
            //
#if CONFIG_RFC2217_ENABLED
            ucModemState = (0 + 1);
#endif
            HWREG(g_ulUARTBase[ulPort] + UART_O_CTL) |= UART_CTL_TXE;
        }

        //
        // Notify the telnet RFC2217 process that RTS (actually flow control
        // input) has chagned state.
        //
#if CONFIG_RFC2217_ENABLED
        TelnetNotifyModemState(ulPort, ucModemState);
#endif
    }
}

//*****************************************************************************
//
//! Handles the GPIO B interrupt for flow control (port 0).
//!
//! This function is called when the GPIO port B generates an interrupt.  An
//! interrupt will be generated when the inbound flow control signal changes
//! levels (rising/falling edge).  A notification function will be called to
//! inform the corresponding telnet session that the flow control signal has
//! changed.
//!
//! \return None.
//
//*****************************************************************************
void
SerialGPIOBIntHandler(void)
{
    //
    // Call the generic flow control interrupt routine.
    //
    SerialFlowInIntHandler(0);
}

//*****************************************************************************
//
//! Handles the GPIO A interrupt for flow control (port 1).
//!
//! This function is called when the GPIO port A generates an interrupt.  An
//! interrupt will be generated when the inbound flow control signal changes
//! levels (rising/falling edge).  A notification function will be called to
//! inform the corresponding telnet session that the flow control signal has
//! changed.
//!
//! \return None.
//
//*****************************************************************************
void
SerialGPIOAIntHandler(void)
{
    //
    // Call the generic flow control interrupt routine.
    //
    SerialFlowInIntHandler(1);
}

//*****************************************************************************
//
//! Enables transmitting and receiving.
//!
//! \param ulPort is the UART port number to be accessed.
//!
//! Sets the UARTEN, and RXE bits, and enables the transmit and receive
//! FIFOs.  Optionally sets the TXE bit if flow control conditions allow.
//!
//! \return None.
//
//*****************************************************************************
static void
SerialUARTEnable(unsigned long ulPort)
{
    //
    // Enable the FIFO.
    //
    HWREG(g_ulUARTBase[ulPort] + UART_O_LCRH) |= UART_LCRH_FEN;

    //
    // Enable RX and the UART.
    //
    HWREG(g_ulUARTBase[ulPort] + UART_O_CTL) |= UART_CTL_UARTEN | UART_CTL_RXE;

    //
    // If flow control is enabled and asserted, don't enable the transmitter.
    // Otherwise, enable it.
    //
    if(!((g_sParameters.sPort[ulPort].ucFlowControl ==
          SERIAL_FLOW_CONTROL_HW) &&
         GPIOPinRead(g_ulFlowInBase[ulPort], g_ulFlowInPin[ulPort])))
    {
        HWREG(g_ulUARTBase[ulPort] + UART_O_CTL) |= UART_CTL_TXE;
    }
}

//*****************************************************************************
//
//! Checks the availability of the serial port output buffer.
//!
//! \param ulPort is the UART port number to be accessed.
//!
//! This function checks to see if there is room on the UART transmit buffer
//! for additional data.
//!
//! \return Returns the number of bytes available in the serial transmit ring
//! buffer.
//
//*****************************************************************************
tBoolean
SerialSendFull(unsigned long ulPort)
{
    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // Return the number of bytes available in the tx ring buffer.
    //
    return(RingBufFull(&g_sTxBuf[ulPort]));
}

//*****************************************************************************
//
//! Sends a character to the UART.
//!
//! \param ulPort is the UART port number to be accessed.
//! \param ucChar is the character to be sent.
//!
//! This function sends a character to the UART.  The character will either be
//! directly written into the UART FIFO or into the UART transmit buffer, as
//! appropriate.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSend(unsigned long ulPort, unsigned char ucChar)
{
    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // Disable the UART transmit interrupt while determining how to handle this
    // character.  Failure to do so could result in the loss of this character,
    // or stalled output due to this character being placed into the UART
    // transmit buffer but never transferred out into the UART FIFO.
    //
    UARTIntDisable(g_ulUARTBase[ulPort], UART_INT_TX);

    //
    // See if the transmit buffer is empty and there is space in the FIFO.
    //
    if(RingBufEmpty(&g_sTxBuf[ulPort]) &&
       (UARTSpaceAvail(g_ulUARTBase[ulPort])))
    {
        //
        // Write this character directly into the FIFO.
        //
        UARTCharPut(g_ulUARTBase[ulPort], ucChar);
    }

    //
    // See if there is room in the transmit buffer.
    //
    else if(!RingBufFull(&g_sTxBuf[ulPort]))
    {
        //
        // Put this character into the transmit buffer.
        //
        RingBufWriteOne(&g_sTxBuf[ulPort], ucChar);
    }

    //
    // Enable the UART transmit interrupt.
    //
    UARTIntEnable(g_ulUARTBase[ulPort], UART_INT_TX);
}

//*****************************************************************************
//
//! Receives a character from the UART.
//!
//! \param ulPort is the UART port number to be accessed.
//!
//! This function receives a character from the relevant port's UART buffer.
//!
//! \return Returns -1 if no data is available or the oldest character held in
//! the receive ring buffer.
//
//*****************************************************************************
long
SerialReceive(unsigned long ulPort)
{
    unsigned long ulData;

    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // See if the receive buffer is empty and there is space in the FIFO.
    //
    if(RingBufEmpty(&g_sRxBuf[ulPort]))
    {
        //
        // Return -1 (EOF) to indicate no data available.
        //
        return(-1);
    }

    //
    // Read a single character.
    //
    ulData = (long)RingBufReadOne(&g_sRxBuf[ulPort]);

    //
    // If flow control is enabled, check the status of the RX buffer to
    // determine if flow control GPIO needs to be de-asserted.
    //
    if(g_sParameters.sPort[ulPort].ucFlowControl == SERIAL_FLOW_CONTROL_HW)
    {
        //
        // If the ring buffer is down to less than 25% used,
        // de-assert the outbound flow control pin.
        //
        if((RingBufUsed(&g_sRxBuf[ulPort]) <
            (RingBufSize(&g_sRxBuf[ulPort]) / 4)) &&
           (g_ulSetFlowOutValue[ulPort] == 0))
        {
            GPIOPinWrite(g_ulFlowOutBase[ulPort], g_ulFlowOutPin[ulPort], 0);
        }
    }

    //
    // Return the data that was read.
    //
    return(ulData);
}

//*****************************************************************************
//
//! Returns number of characters available in the serial ring buffer.
//!
//! \param ulPort is the UART port number to be accessed.
//!
//! This function will return the number of characters available in the
//! serial ring buffer.
//!
//! \return The number of characters available in the ring buffer..
//
//*****************************************************************************
unsigned long
SerialReceiveAvailable(unsigned long ulPort)
{
    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // Return the value.
    //
    return(RingBufUsed(&g_sRxBuf[ulPort]));
}

//*****************************************************************************
//
//! Configures the serial port baud rate.
//!
//! \param ulPort is the serial port number to be accessed.
//! \param ulBaudRate is the new baud rate for the serial port.
//!
//! This function configures the serial port's baud rate.  The current
//! configuration for the serial port will be read.  The baud rate will be
//! modified, and the port will be reconfigured.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetBaudRate(unsigned long ulPort, unsigned long ulBaudRate)
{
    unsigned long ulDiv, ulUARTClk;

    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);
    ASSERT(ulBaudRate != 0);

    //
    // Save the baud rate for future reference.
    //
    g_ulCurrentBaudRate[ulPort] = ulBaudRate;

    //
    // Get and check the clock use by the UART.
    //
    ulUARTClk = SysCtlClockGet();
    ASSERT(ulUARTClk >= (ulBaudRate * 16));

    //
    // Stop the UART.
    //
    UARTDisable(g_ulUARTBase[ulPort]);

    //
    // Compute the fractional baud rate divider.
    //
    ulDiv = (((ulUARTClk * 8) / ulBaudRate) + 1) / 2;

    //
    // Set the baud rate.
    //
    HWREG(g_ulUARTBase[ulPort] + UART_O_IBRD) = ulDiv / 64;
    HWREG(g_ulUARTBase[ulPort] + UART_O_FBRD) = ulDiv % 64;

    //
    // Clear the flags register.
    //
    HWREG(g_ulUARTBase[ulPort] + UART_O_FR) = 0;

    //
    // Start the UART.
    //
    SerialUARTEnable(ulPort);
}

//*****************************************************************************
//
//! Queries the serial port baud rate.
//!
//! \param ulPort is the serial port number to be accessed.
//!
//! This function will read the UART configuration and return the currently
//! configured baud rate for the selected port.
//!
//! \return The current baud rate of the serial port.
//
//*****************************************************************************
unsigned long
SerialGetBaudRate(unsigned long ulPort)
{
    unsigned long ulCurrentBaudRate, ulCurrentConfig, ulDif, ulTemp;

    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ulUARTBase[ulPort], SysCtlClockGet(),
                        &ulCurrentBaudRate, &ulCurrentConfig);

    //
    // Calculate the difference between the reported baud rate and the
    // stored nominal baud rate.
    //
    if(ulCurrentBaudRate > g_ulCurrentBaudRate[ulPort])
    {
        ulDif = ulCurrentBaudRate - g_ulCurrentBaudRate[ulPort];
    }
    else
    {
        ulDif = g_ulCurrentBaudRate[ulPort] - ulCurrentBaudRate;
    }

    //
    // Calculate the 1% value of nominal baud rate.
    //
    ulTemp = g_ulCurrentBaudRate[ulPort] / 100;

    //
    // If the difference between calculated and nominal is > 1%, report the
    // calculated rate.  Otherwise, report the nominal rate.
    //
    if(ulDif > ulTemp)
    {
        return(ulCurrentBaudRate);
    }

    //
    // Return the current serial port baud rate.
    //
    return(g_ulCurrentBaudRate[ulPort]);
}

//*****************************************************************************
//
//! Configures the serial port data size.
//!
//! \param ulPort is the serial port number to be accessed.
//! \param ucDataSize is the new data size for the serial port.
//!
//! This function configures the serial port's data size.  The current
//! configuration for the serial port will be read.  The data size will be
//! modified, and the port will be reconfigured.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetDataSize(unsigned long ulPort, unsigned char ucDataSize)
{
    unsigned long ulCurrentBaudRate, ulCurrentConfig, ulNewConfig;

    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);
    ASSERT((ucDataSize >= 5) && (ucDataSize <= 8));

    //
    // Stop the UART.
    //
    UARTDisable(g_ulUARTBase[ulPort]);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ulUARTBase[ulPort], SysCtlClockGet(),
                        &ulCurrentBaudRate, &ulCurrentConfig);

    //
    // Update the configuration with a new data length.
    //
    switch(ucDataSize)
    {
        case 5:
        {
            ulNewConfig = (ulCurrentConfig & ~UART_CONFIG_WLEN_MASK);
            ulNewConfig |= UART_CONFIG_WLEN_5;
            g_sParameters.sPort[ulPort].ucDataSize = ucDataSize;
            break;
        }

        case 6:
        {
            ulNewConfig = (ulCurrentConfig & ~UART_CONFIG_WLEN_MASK);
            ulNewConfig |= UART_CONFIG_WLEN_6;
            g_sParameters.sPort[ulPort].ucDataSize = ucDataSize;
            break;
        }

        case 7:
        {
            ulNewConfig = (ulCurrentConfig & ~UART_CONFIG_WLEN_MASK);
            ulNewConfig |= UART_CONFIG_WLEN_7;
            g_sParameters.sPort[ulPort].ucDataSize = ucDataSize;
            break;
        }

        case 8:
        {
            ulNewConfig = (ulCurrentConfig & ~UART_CONFIG_WLEN_MASK);
            ulNewConfig |= UART_CONFIG_WLEN_8;
            g_sParameters.sPort[ulPort].ucDataSize = ucDataSize;
            break;
        }

        default:
        {
            ulNewConfig = ulCurrentConfig;
            break;
        }
    }

    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(g_ulUARTBase[ulPort] + UART_O_LCRH) = ulNewConfig;

    //
    // Clear the flags register.
    //
    HWREG(g_ulUARTBase[ulPort] + UART_O_FR) = 0;

    //
    // Start the UART.
    //
    SerialUARTEnable(ulPort);
}

//*****************************************************************************
//
//! Queries the serial port data size.
//!
//! \param ulPort is the serial port number to be accessed.
//!
//! This function will read the UART configuration and return the currently
//! configured data size for the selected port.
//!
//! \return None.
//
//*****************************************************************************
unsigned char
SerialGetDataSize(unsigned long ulPort)
{
    unsigned long ulCurrentBaudRate, ulCurrentConfig;
    unsigned char ucCurrentDataSize;

    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ulUARTBase[ulPort], SysCtlClockGet(),
                        &ulCurrentBaudRate, &ulCurrentConfig);

    //
    // Determine the current data size.
    //
    switch(ulCurrentConfig & UART_CONFIG_WLEN_MASK)
    {
        case UART_CONFIG_WLEN_5:
        {
            ucCurrentDataSize = 5;
            break;
        }

        case UART_CONFIG_WLEN_6:
        {
            ucCurrentDataSize = 6;
            break;
        }

        case UART_CONFIG_WLEN_7:
        {
            ucCurrentDataSize = 7;
            break;
        }

        case UART_CONFIG_WLEN_8:
        {
            ucCurrentDataSize = 8;
            break;
        }

        default:
        {
            ucCurrentDataSize = 0;
            break;
        }
    }

    //
    // Return the current data size.
    //
    return(ucCurrentDataSize);
}

//*****************************************************************************
//
//! Configures the serial port parity.
//!
//! \param ulPort is the serial port number to be accessed.
//! \param ucParity is the new parity for the serial port.
//!
//! This function configures the serial port's parity.  The current
//! configuration for the serial port will be read.  The parity will be
//! modified, and the port will be reconfigured.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetParity(unsigned long ulPort, unsigned char ucParity)
{
    unsigned long ulCurrentBaudRate, ulCurrentConfig, ulNewConfig;

    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);
    ASSERT((ucParity == SERIAL_PARITY_NONE) ||
           (ucParity == SERIAL_PARITY_ODD) ||
           (ucParity == SERIAL_PARITY_EVEN) ||
           (ucParity == SERIAL_PARITY_MARK) ||
           (ucParity == SERIAL_PARITY_SPACE));

    //
    // Stop the UART.
    //
    UARTDisable(g_ulUARTBase[ulPort]);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ulUARTBase[ulPort], SysCtlClockGet(),
                        &ulCurrentBaudRate, &ulCurrentConfig);

    //
    // Update the configuration with a new parity.
    //
    switch(ucParity)
    {
        case SERIAL_PARITY_NONE:
        {
            ulNewConfig = (ulCurrentConfig & ~UART_CONFIG_PAR_MASK);
            ulNewConfig |= UART_CONFIG_PAR_NONE;
            g_sParameters.sPort[ulPort].ucParity = ucParity;
            break;
        }

        case SERIAL_PARITY_ODD:
        {
            ulNewConfig = (ulCurrentConfig & ~UART_CONFIG_PAR_MASK);
            ulNewConfig |= UART_CONFIG_PAR_ODD;
            g_sParameters.sPort[ulPort].ucParity = ucParity;
            break;
        }

        case SERIAL_PARITY_EVEN:
        {
            ulNewConfig = (ulCurrentConfig & ~UART_CONFIG_PAR_MASK);
            ulNewConfig |= UART_CONFIG_PAR_EVEN;
            g_sParameters.sPort[ulPort].ucParity = ucParity;
            break;
        }

        case SERIAL_PARITY_MARK:
        {
            ulNewConfig = (ulCurrentConfig & ~UART_CONFIG_PAR_MASK);
            ulNewConfig |= UART_CONFIG_PAR_ONE;
            g_sParameters.sPort[ulPort].ucParity = ucParity;
            break;
        }

        case SERIAL_PARITY_SPACE:
        {
            ulNewConfig = (ulCurrentConfig & ~UART_CONFIG_PAR_MASK);
            ulNewConfig |= UART_CONFIG_PAR_ZERO;
            g_sParameters.sPort[ulPort].ucParity = ucParity;
            break;
        }

        default:
        {
            ulNewConfig = ulCurrentConfig;
            break;
        }
    }

    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(g_ulUARTBase[ulPort] + UART_O_LCRH) = ulNewConfig;

    //
    // Clear the flags register.
    //
    HWREG(g_ulUARTBase[ulPort] + UART_O_FR) = 0;

    //
    // Start the UART.
    //
    SerialUARTEnable(ulPort);
}

//*****************************************************************************
//
//! Queries the serial port parity.
//!
//! \param ulPort is the serial port number to be accessed.
//!
//! This function will read the UART configuration and return the currently
//! configured parity for the selected port.
//!
//! \return Returns the current parity setting for the port.  This will be one
//! of \b SERIAL_PARITY_NONE, \b SERIAL_PARITY_ODD, \b SERIAL_PARITY_EVEN,
//! \b SERIAL_PARITY_MARK, or \b SERIAL_PARITY_SPACE.
//
//*****************************************************************************
unsigned char
SerialGetParity(unsigned long ulPort)
{
    unsigned long ulCurrentBaudRate, ulCurrentConfig;
    unsigned char ucCurrentParity;

    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ulUARTBase[ulPort], SysCtlClockGet(),
                        &ulCurrentBaudRate, &ulCurrentConfig);

    //
    // Determine the current data size.
    //
    switch(ulCurrentConfig & UART_CONFIG_PAR_MASK)
    {
        case UART_CONFIG_PAR_NONE:
        {
            ucCurrentParity = SERIAL_PARITY_NONE;
            break;
        }

        case UART_CONFIG_PAR_ODD:
        {
            ucCurrentParity = SERIAL_PARITY_ODD;
            break;
        }

        case UART_CONFIG_PAR_EVEN:
        {
            ucCurrentParity = SERIAL_PARITY_EVEN;
            break;
        }

        case UART_CONFIG_PAR_ONE:
        {
            ucCurrentParity = SERIAL_PARITY_MARK;
            break;
        }

        case UART_CONFIG_PAR_ZERO:
        {
            ucCurrentParity = SERIAL_PARITY_SPACE;
            break;
        }

        default:
        {
            ucCurrentParity = SERIAL_PARITY_NONE;
            break;
        }
    }

    //
    // Return the current data size.
    //
    return(ucCurrentParity);
}

//*****************************************************************************
//
//! Configures the serial port stop bits.
//!
//! \param ulPort is the serial port number to be accessed.
//! \param ucStopBits is the new stop bits for the serial port.
//!
//! This function configures the serial port's stop bits.  The current
//! configuration for the serial port will be read.  The stop bits will be
//! modified, and the port will be reconfigured.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetStopBits(unsigned long ulPort, unsigned char ucStopBits)
{
    unsigned long ulCurrentBaudRate, ulCurrentConfig, ulNewConfig;

    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);
    ASSERT((ucStopBits >= 1) && (ucStopBits <= 2));

    //
    // Stop the UART.
    //
    UARTDisable(g_ulUARTBase[ulPort]);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ulUARTBase[ulPort], SysCtlClockGet(),
                        &ulCurrentBaudRate, &ulCurrentConfig);

    //
    // Update the configuration with a new stop bits.
    //
    switch(ucStopBits)
    {
        case 1:
        {
            ulNewConfig = (ulCurrentConfig & ~UART_CONFIG_STOP_MASK);
            ulNewConfig |= UART_CONFIG_STOP_ONE;
            g_sParameters.sPort[ulPort].ucStopBits = ucStopBits;
            break;
        }

        case 2:
        {
            ulNewConfig = (ulCurrentConfig & ~UART_CONFIG_STOP_MASK);
            ulNewConfig |= UART_CONFIG_STOP_TWO;
            g_sParameters.sPort[ulPort].ucStopBits = ucStopBits;
            break;
        }

        default:
        {
            ulNewConfig = ulCurrentConfig;
            break;
        }
    }

    //
    // Set parity, data length, and number of stop bits.
    //
    HWREG(g_ulUARTBase[ulPort] + UART_O_LCRH) = ulNewConfig;

    //
    // Clear the flags register.
    //
    HWREG(g_ulUARTBase[ulPort] + UART_O_FR) = 0;

    //
    // Start the UART.
    //
    SerialUARTEnable(ulPort);
}

//*****************************************************************************
//
//! Queries the serial port stop bits.
//!
//! \param ulPort is the serial port number to be accessed.
//!
//! This function will read the UART configuration and return the currently
//! configured stop bits for the selected port.
//!
//! \return None.
//
//*****************************************************************************
unsigned char
SerialGetStopBits(unsigned long ulPort)
{
    unsigned long ulCurrentBaudRate, ulCurrentConfig;
    unsigned char ucCurrentStopBits;

    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // Get the current configuration of the UART.
    //
    UARTConfigGetExpClk(g_ulUARTBase[ulPort], SysCtlClockGet(),
                        &ulCurrentBaudRate, &ulCurrentConfig);

    //
    // Determine the current data size.
    //
    switch(ulCurrentConfig & UART_CONFIG_STOP_MASK)
    {
        case UART_CONFIG_STOP_ONE:
        {
            ucCurrentStopBits = 1;
            break;
        }

        case UART_CONFIG_STOP_TWO:
        {
            ucCurrentStopBits = 2;
            break;
        }

        default:
        {
            ucCurrentStopBits = 0;
            break;
        }
    }

    //
    // Return the current data size.
    //
    return(ucCurrentStopBits);
}

//*****************************************************************************
//
//! Sets the serial port flow control output signal.
//!
//! \param ulPort is the UART port number to be accessed.
//! \param ucFlowValue is the value to program to the flow control pin.  Valid
//! values are \b SERIAL_FLOW_OUT_SET and \b SERIAL_FLOW_OUT_CLEAR.
//!
//! This function will set the flow control output pin to a specified value.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetFlowOut(unsigned long ulPort, unsigned char ucFlowValue)
{
    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);
    ASSERT((ucFlowValue == SERIAL_FLOW_OUT_SET) ||
           (ucFlowValue == SERIAL_FLOW_OUT_CLEAR));

    //
    // If we are to assert the pin, save the value and assert the pin.
    //
    if(ucFlowValue == SERIAL_FLOW_OUT_SET)
    {
        g_ulSetFlowOutValue[ulPort] = g_ulFlowOutPin[ulPort];
        GPIOPinWrite(g_ulFlowOutBase[ulPort], g_ulFlowOutPin[ulPort],
                     g_ulFlowOutPin[ulPort]);
    }

    //
    // If we are to deassert the pin, save the value, and deassert the pin
    // only if flow control would be deasserted anyway (in other words, the
    // fifo level is ok).
    //
    else
    {
        g_ulSetFlowOutValue[ulPort] = 0;

        //
        // If flow control is enabled, check the status of the RX buffer to
        // determine if flow control GPIO needs to be de-asserted.
        //
        if(g_sParameters.sPort[ulPort].ucFlowControl == SERIAL_FLOW_CONTROL_HW)
        {
            //
            // If the ring buffer is down to less than 25% used,
            // de-assert the outbound flow control pin.
            //
            if(RingBufUsed(&g_sRxBuf[ulPort]) <
               (RingBufSize(&g_sRxBuf[ulPort]) / 4))
            {
                GPIOPinWrite(g_ulFlowOutBase[ulPort], g_ulFlowOutPin[ulPort],
                             0);
            }
        }
        else
        {
            GPIOPinWrite(g_ulFlowOutBase[ulPort], g_ulFlowOutPin[ulPort], 0);
        }
    }
}

//*****************************************************************************
//
//! Gets the serial port flow control output signal.
//!
//! \param ulPort is the UART port number to be accessed.
//!
//! This function will set the flow control output pin to a specified value.
//!
//! \return Returns \b SERIAL_FLOW_OUT_SET or \b SERIAL_FLOW_OUT_CLEAR.
//
//*****************************************************************************
unsigned char
SerialGetFlowOut(unsigned long ulPort)
{
    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // If flow control is set (ON) return an "11", otherwise, return
    // a "12" (based on the RFC2217 values).
    //
    if(GPIOPinRead(g_ulFlowOutBase[ulPort], g_ulFlowOutPin[ulPort]) ==
       g_ulFlowOutPin[ulPort])
    {
        return(SERIAL_FLOW_OUT_SET);
    }
    else
    {
        return(SERIAL_FLOW_OUT_CLEAR);
    }
}

//*****************************************************************************
//
//! Configures the serial port flow control option.
//!
//! \param ulPort is the UART port number to be accessed.
//! \param ucFlowControl is the new flow control setting for the serial port.
//!
//! This function configures the serial port's flow control.  This function
//! will enable/disable the flow control interrupt and the UART transmitter
//! based on the value of the flow control setting and/or the flow control
//! input signal.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetFlowControl(unsigned long ulPort, unsigned char ucFlowControl)
{
    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);
    ASSERT((ucFlowControl == SERIAL_FLOW_CONTROL_NONE) ||
           (ucFlowControl == SERIAL_FLOW_CONTROL_HW));

    //
    // Save the new flow control setting.
    //
    g_sParameters.sPort[ulPort].ucFlowControl = ucFlowControl;

    //
    // If flow control is enabled.
    //
    if(g_sParameters.sPort[ulPort].ucFlowControl == SERIAL_FLOW_CONTROL_HW)
    {
        //
        // Enable/Disable the transmitter, based on the flow control
        // input pin status.
        //
        if(GPIOPinRead(g_ulFlowInBase[ulPort], g_ulFlowInPin[ulPort]))
        {
            HWREG(g_ulUARTBase[ulPort] + UART_O_CTL) &= ~UART_CTL_TXE;
        }
        else
        {
            HWREG(g_ulUARTBase[ulPort] + UART_O_CTL) |= UART_CTL_TXE;
        }

        //
        // Enable the Flow Control interrupt.
        //
        IntEnable(g_ulFlowInterrupt[ulPort]);
    }

    //
    // If flow control is disabled.
    //
    else
    {
        //
        // Disable the Flow Control interrupt.
        //
        IntDisable(g_ulFlowInterrupt[ulPort]);

        //
        // Enable the transmitter.
        //
        HWREG(g_ulUARTBase[ulPort] + UART_O_CTL) |= UART_CTL_TXE;
    }
}

//*****************************************************************************
//
//! Queries the serial port flow control.
//!
//! \param ulPort is the serial port number to be accessed.
//!
//! This function will return the currently configured flow control for
//! the selected port.
//!
//! \return None.
//
//*****************************************************************************
unsigned char
SerialGetFlowControl(unsigned long ulPort)
{
    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // Return the current flow control.
    //
    return(g_sParameters.sPort[ulPort].ucFlowControl);
}

//*****************************************************************************
//
//! Purges the serial port data queue(s).
//!
//! \param ulPort is the serial port number to be accessed.
//! \param ucPurgeCommand is the command indicating which queue's to purge.
//!
//! This function will purge data from the tx, rx, or both serial port queues.
//!
//! \return None.
//
//*****************************************************************************
void
SerialPurgeData(unsigned long ulPort, unsigned char ucPurgeCommand)
{
    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);
    ASSERT((ucPurgeCommand >= 1) && (ucPurgeCommand <= 3));

    //
    // Disable the UART.
    //
    UARTDisable(g_ulUARTBase[ulPort]);

    //
    // Purge the receive data if requested.
    //
    if(ucPurgeCommand & 0x01)
    {
        RingBufFlush(&g_sRxBuf[ulPort]);
    }

    //
    // Purge the transmit data if requested.
    //
    if(ucPurgeCommand & 0x02)
    {
        RingBufFlush(&g_sTxBuf[ulPort]);
    }

    //
    // Renable the UART.
    //
    SerialUARTEnable(ulPort);
}

//*****************************************************************************
//
//! Configures the serial port to a default setup.
//!
//! \param ulPort is the UART port number to be accessed.
//!
//! This function resets the serial port to a default configuration.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetDefault(unsigned long ulPort)
{
    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // Disable interrupts.
    //
    IntDisable(g_ulUARTInterrupt[ulPort]);

    //
    // Set the baud rate.
    //
    SerialSetBaudRate(ulPort, g_psDefaultParameters->sPort[ulPort].ulBaudRate);

    //
    // Set the data size.
    //
    SerialSetDataSize(ulPort, g_psDefaultParameters->sPort[ulPort].ucDataSize);

    //
    // Set the parity.
    //
    SerialSetParity(ulPort, g_psDefaultParameters->sPort[ulPort].ucParity);

    //
    // Set the stop bits.
    //
    SerialSetStopBits(ulPort, g_psDefaultParameters->sPort[ulPort].ucStopBits);

    //
    // Set the flow control.
    //
    SerialSetFlowControl(ulPort,
                      g_psDefaultParameters->sPort[ulPort].ucFlowControl);

    //
    // Purge the Serial Tx/Rx Ring Buffers.
    //
    SerialPurgeData(ulPort, 0x03);

    //
    // (Re)enable the UART transmit and receive interrupts.
    //
    UARTIntEnable(g_ulUARTBase[ulPort],
                 (UART_INT_RX | UART_INT_RT | UART_INT_TX));
    IntEnable(g_ulUARTInterrupt[ulPort]);
}

//*****************************************************************************
//
//! Configures the serial port according to the current working parameter
//! values.
//!
//! \param ulPort is the UART port number to be accessed.  Valid values are 0
//! and 1.
//!
//! This function configures the serial port according to the current working
//! parameters in g_sParameters.sPort for the specified port.  The actual
//! parameter set is then read back and g_sParameters.sPort updated to ensure
//! that the structure is correctly synchronized with the hardware.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetCurrent(unsigned long ulPort)
{
    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // Disable interrupts.
    //
    IntDisable(g_ulUARTInterrupt[ulPort]);

    //
    // Set the baud rate.
    //
    SerialSetBaudRate(ulPort, g_sParameters.sPort[ulPort].ulBaudRate);
    g_sParameters.sPort[ulPort].ulBaudRate = SerialGetBaudRate(ulPort);

    //
    // Set the data size.
    //
    SerialSetDataSize(ulPort, g_sParameters.sPort[ulPort].ucDataSize);
    g_sParameters.sPort[ulPort].ucDataSize = SerialGetDataSize(ulPort);

    //
    // Set the parity.
    //
    SerialSetParity(ulPort, g_sParameters.sPort[ulPort].ucParity);
    g_sParameters.sPort[ulPort].ucParity = SerialGetParity(ulPort);

    //
    // Set the stop bits.
    //
    SerialSetStopBits(ulPort, g_sParameters.sPort[ulPort].ucStopBits);
    g_sParameters.sPort[ulPort].ucStopBits = SerialGetStopBits(ulPort);

    //
    // Set the flow control.
    //
    SerialSetFlowControl(ulPort,
                      g_sParameters.sPort[ulPort].ucFlowControl);
    g_sParameters.sPort[ulPort].ucFlowControl = SerialGetFlowControl(ulPort);

    //
    // Purge the Serial Tx/Rx Ring Buffers.
    //
    SerialPurgeData(ulPort, 0x03);

    //
    // (Re)enable the UART transmit and receive interrupts.
    //
    UARTIntEnable(g_ulUARTBase[ulPort],
                 (UART_INT_RX | UART_INT_RT | UART_INT_TX));
    IntEnable(g_ulUARTInterrupt[ulPort]);
}

//*****************************************************************************
//
//! Configures the serial port to the factory default setup.
//!
//! \param ulPort is the UART port number to be accessed.
//!
//! This function resets the serial port to a default configuration.
//!
//! \return None.
//
//*****************************************************************************
void
SerialSetFactory(unsigned long ulPort)
{
    //
    // Check the arguments.
    //
    ASSERT(ulPort < MAX_S2E_PORTS);

    //
    // Disable interrupts.
    //
    IntDisable(g_ulUARTInterrupt[ulPort]);

    //
    // Set the baud rate.
    //
    SerialSetBaudRate(ulPort, g_psFactoryParameters->sPort[ulPort].ulBaudRate);

    //
    // Set the data size.
    //
    SerialSetDataSize(ulPort, g_psFactoryParameters->sPort[ulPort].ucDataSize);

    //
    // Set the parity.
    //
    SerialSetParity(ulPort, g_psFactoryParameters->sPort[ulPort].ucParity);

    //
    // Set the stop bits.
    //
    SerialSetStopBits(ulPort, g_psFactoryParameters->sPort[ulPort].ucStopBits);

    //
    // Set the flow control.
    //
    SerialSetFlowControl(ulPort,
                      g_psFactoryParameters->sPort[ulPort].ucFlowControl);

    //
    // Purge the Serial Tx/Rx Ring Buffers.
    //
    SerialPurgeData(ulPort, 0x03);

    //
    // (Re)enable the UART transmit and receive interrupts.
    //
    UARTIntEnable(g_ulUARTBase[ulPort],
                 (UART_INT_RX | UART_INT_RT | UART_INT_TX));
    IntEnable(g_ulUARTInterrupt[ulPort]);
}

//*****************************************************************************
//
//! Initializes the serial port driver.
//!
//! This function initializes and configures the serial port driver.
//!
//! \return None.
//
//*****************************************************************************
void
SerialInit(void)
{
    //
    // Initialize the ring buffers used by the UART Drivers.
    //
    RingBufInit(&g_sRxBuf[0], g_pucRX0Buffer, sizeof(g_pucRX0Buffer));
    RingBufInit(&g_sTxBuf[0], g_pucTX0Buffer, sizeof(g_pucTX0Buffer));
    RingBufInit(&g_sRxBuf[1], g_pucRX1Buffer, sizeof(g_pucRX1Buffer));
    RingBufInit(&g_sTxBuf[1], g_pucTX1Buffer, sizeof(g_pucTX1Buffer));

    //
    // Configure the Port 0 pins appropriately.
    //
    GPIOPinTypeUART(PIN_U0RX_PORT, PIN_U0RX_PIN);
    GPIOPinTypeUART(PIN_U0TX_PORT, PIN_U0TX_PIN);
    GPIOPinTypeGPIOInput(PIN_U0RTS_PORT, PIN_U0RTS_PIN);
    GPIOPinTypeGPIOOutput(PIN_U0CTS_PORT, PIN_U0CTS_PIN);
    GPIOPinWrite(PIN_U0CTS_PORT, PIN_U0CTS_PIN, 0);
    GPIOIntTypeSet(PIN_U0RTS_PORT, PIN_U0RTS_PIN, GPIO_BOTH_EDGES);
    GPIOPinIntEnable(PIN_U0RTS_PORT, PIN_U0RTS_PIN);
    IntEnable(PIN_U0RTS_INT);

    //
    // Configure the Port 1 pins appropriately.
    //
    GPIOPinTypeUART(PIN_U1RX_PORT, PIN_U1RX_PIN);
    GPIOPinTypeUART(PIN_U1TX_PORT, PIN_U1TX_PIN);
    GPIOPinTypeGPIOInput(PIN_U1RTS_PORT, PIN_U1RTS_PIN);
    GPIOPinTypeGPIOOutput(PIN_U1CTS_PORT, PIN_U1CTS_PIN);
    GPIOPinWrite(PIN_U1CTS_PORT, PIN_U1CTS_PIN, 0);
    GPIOIntTypeSet(PIN_U1RTS_PORT, PIN_U1RTS_PIN, GPIO_BOTH_EDGES);
    GPIOPinIntEnable(PIN_U1RTS_PORT, PIN_U1RTS_PIN);
    IntEnable(PIN_U1RTS_INT);

    //
    // Configure the RS-232 Transceiver pins appropriately.
    // (Note:  This is for Port 1 only).
    //
    GPIOPinTypeGPIOInput(PIN_XVR_INV_N_PORT, PIN_XVR_INV_N_PIN);
    GPIOPinTypeGPIOOutput(PIN_XVR_ON_PORT, PIN_XVR_ON_PIN);
    GPIOPinTypeGPIOOutput(PIN_XVR_OFF_N_PORT, PIN_XVR_OFF_N_PIN);
    GPIOPinTypeGPIOOutput(PIN_XVR_RDY_PORT, PIN_XVR_RDY_PIN);

    //
    // Enable the RS-232 Transciever.
    //
    GPIOPinWrite(PIN_XVR_RDY_PORT, PIN_XVR_RDY_PIN, 0);
    GPIOPinWrite(PIN_XVR_OFF_N_PORT, PIN_XVR_OFF_N_PIN, PIN_XVR_OFF_N_PIN);
    GPIOPinWrite(PIN_XVR_ON_PORT, PIN_XVR_ON_PIN, PIN_XVR_ON_PIN);

    //
    // Configure Port 0.
    //
    SerialSetDefault(0);

    //
    // Configure Port 1.
    //
    SerialSetDefault(1);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
