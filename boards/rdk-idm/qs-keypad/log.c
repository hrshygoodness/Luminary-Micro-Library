//*****************************************************************************
//
// log.c - Functions to log events on the UART.
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
// This is part of revision 8555 of the RDK-IDM Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/ustdlib.h"
#include "log.h"
#include "qs-keypad.h"

//*****************************************************************************
//
// The size of the software FIFO used for buffering log data sent to the UART.
//
//*****************************************************************************
#define SOFT_FIFO_SIZE          256

//*****************************************************************************
//
// Strings for the names of the months, used to create the date code associated
// with a logged event.
//
//*****************************************************************************
static const char *g_ppcMonths[12] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

//*****************************************************************************
//
// Strings for the names of the week days, used to create the date code
// associated with a logged event.
//
//*****************************************************************************
static const char *g_ppcDays[7] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

//*****************************************************************************
//
// The software FIFO used to store data being transmitted to the UART.
//
//*****************************************************************************
static char g_pcTransmitBuffer[SOFT_FIFO_SIZE];

//*****************************************************************************
//
// The offset into g_pcTransmitBuffer of the next byte of the software FIFO to
// write.  The software FIFO is full when this is one less than g_ulReadPtr
// (modulo the buffer size).
//
//*****************************************************************************
static unsigned long g_ulWritePtr = 0;

//*****************************************************************************
//
// The offset into g_pcTransmitBuffer of the next byte of the software FIFO to
// transfer to the UART.  The software FIFO is empty when this is equal to
// g_ulWritePtr.
//
//*****************************************************************************
static unsigned long g_ulReadPtr = 0;

//*****************************************************************************
//
// A buffer used to construct log messages before they are sent to the UART.
//
//*****************************************************************************
static char g_pcBuffer[64];

//*****************************************************************************
//
// This function handles the UART interrupt.  It will copy data from the
// software FIFO to the hardware.
//
//*****************************************************************************
void
LogIntHandler(void)
{
    //
    // Clear the UART interrupt.
    //
    UARTIntClear(UART1_BASE, UART_INT_TX);

    //
    // Loop while there is more data to be transferred from the software FIFO.
    //
    while(g_ulReadPtr != g_ulWritePtr)
    {
        //
        // Transfer the next byte to the UART.  Break out of the loop if the
        // hardware FIFO is full.
        //
        if(UARTCharPutNonBlocking(UART1_BASE,
                                  g_pcTransmitBuffer[g_ulReadPtr]) == false)
        {
            break;
        }

        //
        // This byte has been transferred to the UART, so remove it from the
        // software FIFO.
        //
        g_ulReadPtr = (g_ulReadPtr + 1) & (SOFT_FIFO_SIZE - 1);
    }
}

//*****************************************************************************
//
// Writes a message to the log.  The message is preceeded by a time stamp, and
// the message can not exceed 28 characters in length (unless g_pcBuffer is
// increased in size).
//
//*****************************************************************************
void
LogWrite(char *pcPtr)
{
    tTime sTime;

    //
    // Convert the current time from seconds since Jan 1, 1970 to the month,
    // day, year, hour, minute, and second equivalent.
    //
    ulocaltime(g_ulTime, &sTime);

    //
    // Construct the log message with the time stamp preceeding it.
    //
    usprintf(g_pcBuffer,
             "%s %s %2d %02d:%02d:%02d.%02d UT %d => %s\r\n",
             g_ppcDays[sTime.ucWday], g_ppcMonths[sTime.ucMon], sTime.ucMday,
             sTime.ucHour, sTime.ucMin, sTime.ucSec, g_ulTimeCount / 10,
             sTime.usYear, pcPtr);

    //
    // Get a pointer to the start of the log message.
    //
    pcPtr = g_pcBuffer;

    //
    // Disable the UART interrupt to prevent the UART interrupt handler from
    // executing, which would cause corruption of the logging state (possibly
    // losing characters in the process).
    //
    UARTIntDisable(UART1_BASE, UART_INT_TX);

    //
    // See if the software FIFO is empty.
    //
    if(g_ulReadPtr == g_ulWritePtr)
    {
        //
        // The software FIFO is empty, so copy as many characters from the log
        // message into the hardware FIFO as will fit.
        //
        while(*pcPtr && (UARTCharPutNonBlocking(UART1_BASE, *pcPtr) == true))
        {
            pcPtr++;
        }
    }

    //
    // Copy as many characters from the log message into the software FIFO as
    // will fit.
    //
    while(*pcPtr &&
          (((g_ulWritePtr + 1) & (SOFT_FIFO_SIZE - 1)) != g_ulReadPtr))
    {
        g_pcTransmitBuffer[g_ulWritePtr] = *pcPtr++;
        g_ulWritePtr = (g_ulWritePtr + 1) & (SOFT_FIFO_SIZE - 1);
    }

    //
    // Re-enable the UART interrupt.
    //
    UARTIntEnable(UART1_BASE, UART_INT_TX);
}

//*****************************************************************************
//
// Initializes the logging interface.
//
//*****************************************************************************
void
LogInit(void)
{
    //
    // Enable the peripherals used to perform logging.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    //
    // Configure the UART pins appropriately.
    //
    GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                         UART_CONFIG_PAR_NONE));
    UARTEnable(UART1_BASE);

    //
    // Enable the UART interrupt.
    //
    UARTIntEnable(UART1_BASE, UART_INT_TX);
    IntEnable(INT_UART1);

    //
    // Write a log message to indicate that the application has started.
    //
    LogWrite("Application started");
}
