//*****************************************************************************
//
// uart_handler.cxx - All of the UART handling functions for bdc-comm.
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
// This is part of revision 8555 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#ifdef __WIN32
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#endif

//*****************************************************************************
//
// This variable holds the open handle to the UART in use by this
// application.
//
//*****************************************************************************
#ifdef __WIN32
static HANDLE g_hComPort;
#else
static int g_iComPort = -1;
#endif

//*****************************************************************************
//
// OpenUART() opens the UART port.
//
// \param pszComPort is the text representation of the COM port to be
//     opened. "COM1" is the valid string to use to open COM port 1 on the
//     host.
// \param ulBaudRate is the baud rate to configure the UART to use.
//
// This function is used to open the host UART with the given baud rate.  The
// rest of the settings are fixed at No Parity, 8 data bits and 1 stop bit.
//
// \return The function returns zero to indicated success while any non-zero
//     value indicates a failure.
//
//*****************************************************************************
int
OpenUART(char *pszComPort, unsigned long ulBaudRate)
{
#ifdef __WIN32
    DCB sDCB;
    COMMTIMEOUTS sCommTimeouts;

    g_hComPort = CreateFile(pszComPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                OPEN_EXISTING, 0, NULL);

    if(g_hComPort == INVALID_HANDLE_VALUE)
    {
        return(-1);
    }

    if(GetCommState(g_hComPort, &sDCB) == 0)
    {
        return(-1);
    }

    sDCB.BaudRate = ulBaudRate;
    sDCB.ByteSize = 8;
    sDCB.Parity = NOPARITY;
    sDCB.StopBits = ONESTOPBIT;
    sDCB.fAbortOnError = TRUE;
    sDCB.fOutxCtsFlow = FALSE;
    sDCB.fOutxDsrFlow = FALSE;
    sDCB.fDtrControl = DTR_CONTROL_ENABLE;
    if(SetCommState(g_hComPort, &sDCB) == 0)
    {
        return(-1);
    }

    if(GetCommTimeouts(g_hComPort, &sCommTimeouts) == 0)
    {
        return(-1);
    }

    sCommTimeouts.ReadIntervalTimeout = MAXDWORD;
    sCommTimeouts.ReadTotalTimeoutConstant = 1;
    sCommTimeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    sCommTimeouts.WriteTotalTimeoutConstant = 0;
    sCommTimeouts.WriteTotalTimeoutMultiplier = 10;

    if(SetCommTimeouts(g_hComPort, &sCommTimeouts) == 0)
    {
        return(-1);
    }
    return(0);
#else
    struct termios sOptions;

    g_iComPort = open(pszComPort, O_RDWR | O_NOCTTY | O_NDELAY);
    if(g_iComPort == -1)
    {
        return(-1);
    }

    fcntl(g_iComPort, F_SETFL, 0);

    tcgetattr(g_iComPort, &sOptions);

    if(ulBaudRate == 9600)
    {
        cfsetispeed(&sOptions, B9600);
        cfsetospeed(&sOptions, B9600);
    }
    else if(ulBaudRate == 19200)
    {
        cfsetispeed(&sOptions, B19200);
        cfsetospeed(&sOptions, B19200);
    }
    else if(ulBaudRate == 38400)
    {
        cfsetispeed(&sOptions, B38400);
        cfsetospeed(&sOptions, B38400);
    }
    else if(ulBaudRate == 57600)
    {
        cfsetispeed(&sOptions, B57600);
        cfsetospeed(&sOptions, B57600);
    }
    else if(ulBaudRate == 115200)
    {
        cfsetispeed(&sOptions, B115200);
        cfsetospeed(&sOptions, B115200);
    }
    else
    {
        cfsetispeed(&sOptions, B230400);
        cfsetospeed(&sOptions, B230400);
    }

    sOptions.c_iflag &= ~(ICRNL | IGNCR | INLCR | IXON | IXOFF);

    sOptions.c_cflag |= (CLOCAL | CREAD);

    sOptions.c_cflag &= ~(CSIZE);
    sOptions.c_cflag |= CS8;

    sOptions.c_cflag &= ~(PARENB);
    sOptions.c_cflag &= ~(CSTOPB);

    sOptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN);

    sOptions.c_oflag &= ~(OPOST);

    tcsetattr(g_iComPort, TCSANOW, &sOptions);

    return(0);
#endif
}

//*****************************************************************************
//
// CloseUART() closes the UART port.
//
// This function closes the UART port that was opened by a call to OpenUART().
//
// \returns This function returns zero to indicate success while any non-zero
// value indicates a failure.
//
//*****************************************************************************
int
CloseUART(void)
{
#ifdef __WIN32
    return(CloseHandle(g_hComPort));
#else
    if(g_iComPort != -1)
    {
        close(g_iComPort);
    }

    return(0);
#endif
}

//*****************************************************************************
//
// UARTSendData() sends data over a UART port.
//
// \param pucData
//     The buffer to write out to the UART port.
// \param ucSize
//     The number of bytes provided in pucData buffer that should be written
//     out to the port.
//
// This function sends ucSize bytes of data from the buffer pointed to by
// pucData via the UART port that was opened by a call to OpenUART().
//
// \return This function returns zero to indicate success while any non-zero
// value indicates a failure.
//
//*****************************************************************************
int
UARTSendData(unsigned char const *pucData, unsigned char ucSize)
{
#ifdef __WIN32
    unsigned long ulNumBytes;

    //
    // Send the Ack back to the device.
    //
    if(WriteFile(g_hComPort, pucData, ucSize, &ulNumBytes, NULL) == 0)
    {
        return(-1);
    }
    if(ulNumBytes != ucSize)
    {
        return(-1);
    }
    return(0);
#else
    if(write(g_iComPort, pucData, ucSize) != ucSize)
    {
        return(-1);
    }

    return(0);
#endif
}

//*****************************************************************************
//
// UARTReceiveData() receives data over a UART port.
//
// \param pucData is the buffer to read data into from the UART port.
// \param ucSize is the number of bytes provided in pucData buffer that should
//     be written with data from the UART port.
//
// This function reads back ucSize bytes of data from the UART port, that was
// opened by a call to OpenUART(), into the buffer that is pointed to by
// pucData.
//
// \return This function returns zero to indicate success while any non-zero
//     value indicates a failure.
//
//*****************************************************************************
int
UARTReceiveData(unsigned char *pucData, unsigned char ucSize)
{
#ifdef __WIN32
    unsigned long ulNumBytes;

    if(ReadFile(g_hComPort, pucData, ucSize, &ulNumBytes, NULL) == 0)
    {
        return(-1);
    }
    if(ulNumBytes != ucSize)
    {
        return(-1);
    }
    return(0);
#else
    fd_set sDescriptors;
    struct timeval sTimeout;

    while(ucSize)
    {
        FD_ZERO(&sDescriptors);
        FD_SET(g_iComPort, &sDescriptors);

        sTimeout.tv_sec = 0;
        sTimeout.tv_usec = 10000;

        if(select(g_iComPort + 1, &sDescriptors, 0, 0, &sTimeout) < 1)
        {
            return(-1);
        }

        if(read(g_iComPort, pucData, 1) != 1)
        {
            return(-1);
        }

        pucData++;
        ucSize--;
    }

    return(0);
#endif
}
