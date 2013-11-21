//*****************************************************************************
//
// olimex_led.c - Simple olimex_led world example.
//
// Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "utils/softssi.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>olimex_led World (olimex_led)</h1>
//!
//! A very simple ``hello world'' example.  It simply displays ``Hello World!''
//! on the UART and is a starting point for more complicated applications.
//!
//! UART0, connected to the Stellaris Virtual Serial Port and running at 
//! 115,200, 8-N-1, is used to display messages from this application.
//! 
//! Displays a series of faces on the LED matrix.
//
//*****************************************************************************


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// Number of bytes to send and receive.
//
//*****************************************************************************
#define NUM_SSI_DATA            8

//*****************************************************************************
//
// The persistent state of the SoftSSI peripheral.
//
//*****************************************************************************
tSoftSSI g_sSoftSSI;

//*****************************************************************************
//
// The data buffer that is used as the transmit FIFO.  The size of this buffer
// can be increased or decreased as required to match the transmit buffering
// requirements of your application.
//
//*****************************************************************************
unsigned short g_pusTxBuffer[16];

//*****************************************************************************
//
// The data buffer that is used as the receive FIFO.  The size of this buffer
// can be increased or decreased as required to match the receive buffering
// requirements of your application.
//
//*****************************************************************************
unsigned short g_pusRxBuffer[16];

//*****************************************************************************
//
// Index control variable to cycle through faces.
//
//*****************************************************************************
volatile unsigned long g_ulFaceIndex;

//*****************************************************************************
//
// Matrices that correspond to faces on the LED display.
//
//*****************************************************************************
#define NONE    0
#define LAUGH   1
#define HAPPY   2
#define SMILE   3
#define ANGRY   4
#define NUM_FACES 5

const unsigned char g_ucFaces[NUM_FACES][8] = 
                    {   
                    {0x03, 0x03, 0x40, 0x4E, 0x4E, 0x40, 0x03, 0x03},
                    {0x43, 0xC3, 0xC0, 0xCE, 0xCE, 0xC0, 0xC3, 0x43},    
                    {0x66, 0x86, 0x80, 0x9C, 0x9C, 0x80, 0x86, 0x66},    
                    {0x46, 0x86, 0x80, 0x9C, 0x9C, 0x80, 0x86, 0x46},    
                    {0x86, 0x46, 0x40, 0x5C, 0x5C, 0x40, 0x46, 0x86}
                    };  

//*****************************************************************************
//
// Bit-wise reverses a number.
//
//*****************************************************************************
unsigned char
Reverse(unsigned char ucNumber)
{
    unsigned short ucIndex;
    unsigned short ucReversedNumber = 0;
    for(ucIndex=0; ucIndex<8; ucIndex++)
    {
        ucReversedNumber = ucReversedNumber << 1;
        ucReversedNumber |= ((1 << ucIndex) & ucNumber) >> ucIndex;
    }
    return ucReversedNumber;
}

//*****************************************************************************
//
// The interrupt handler for the SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    static unsigned long ulTickCounter;
    //
    // Call the SoftSSI timer tick.
    //
    SoftSSITimerTick(&g_sSoftSSI);
    
    //
    // Keep track of ticks and scroll through the faces in the array
    // 
    ulTickCounter++;
    if(ulTickCounter > 10000)
    {
        ulTickCounter = 0;
        g_ulFaceIndex++;
        if(g_ulFaceIndex >= NUM_FACES)
        {
            g_ulFaceIndex = 0;
        }
    }
}

//*****************************************************************************
//
// Print a graphic on the 8x8 LED screen.
// 
//*****************************************************************************
void
PrintByteArray(const unsigned char * pucBytes)
{
    unsigned long ulIndex; 
    unsigned long ulData;
    
    //
    // Display indication that the SoftSSI is transmitting data.
    //
    UARTprintf("\n\nSent:\n  ");

    //
    // Send the 8 bytes of data.
    //
    for(ulIndex = 0; ulIndex < NUM_SSI_DATA; ulIndex++)
    {
        ulData = (Reverse(pucBytes[ulIndex]) << 8) + (1 << ulIndex);
        
        //
        // Display the data that SSI is transferring.
        //
        UARTprintf("'%x' ", ulData);
        
        //
        // Send the data using the "blocking" put function.  This function
        // will wait until there is room in the send FIFO before returning.
        // This allows you to assure that all the data you send makes it
        // into the send FIFO.
        //
        SoftSSIDataPut(&g_sSoftSSI, ulData);

        //
        // Wait until SoftSSI is done transferring all the data in the 
        // transmit FIFO.
        //
        while(SoftSSIBusy(&g_sSoftSSI))
        {
        }
        
        //
        // Raise the clear signal to show the new value.
        //
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_PIN_5);
        SysCtlDelay(100);
        //
        // Clear the Shift Register
        //
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0);
        
    }
}


//*****************************************************************************
//
// Print "Hello World!" to the UART on the Stellaris evaluation board.
//
//*****************************************************************************
int
main(void)
{
    
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPUEnable();
    ROM_FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Initialize the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);

    //
    // Hello!
    //
    UARTprintf("Hello, world!\n");

    //
    // Initialize the GPIOs we will need for communication with the LED Matrix
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    
		//
		// Initialize the clear signal
		//
		GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_5);
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0);

    //
    // Configure the SoftSSI module.  The size of the FIFO buffers can be
    // changed to accommodate the requirements of your application.  The GPIO
    // pins utilized can also be changed.
    // The pins are assigned as follows:
    //      PB4 - SoftSSICLK
    //      PB6 - SoftSSITx
    //
    memset(&g_sSoftSSI, 0, sizeof(g_sSoftSSI));
    SoftSSIClkGPIOSet(&g_sSoftSSI, GPIO_PORTB_BASE, GPIO_PIN_4);
    SoftSSITxGPIOSet(&g_sSoftSSI, GPIO_PORTB_BASE, GPIO_PIN_6);
    SoftSSIRxBufferSet(&g_sSoftSSI, g_pusRxBuffer,
                       sizeof(g_pusRxBuffer) / sizeof(g_pusRxBuffer[0]));
    SoftSSITxBufferSet(&g_sSoftSSI, g_pusTxBuffer,
                       sizeof(g_pusTxBuffer) / sizeof(g_pusTxBuffer[0]));

    //
    // Configure the SoftSSI module.  Use idle clock level low and active low
    // clock (mode 0) and 8-bit data.  You can set the polarity of the SoftSSI
    // clock when the SoftSSI module is idle.  You can also configure what
    // clock edge you want to capture data on.  Please reference the datasheet
    // for more information on the different SPI modes.
    //
    SoftSSIConfigSet(&g_sSoftSSI, SOFTSSI_FRF_MOTO_MODE_0, 16);

    //
    // Enable the SoftSSI module.
    //
    SoftSSIEnable(&g_sSoftSSI);

    //
    // Configure SysTick to provide an interrupt at a 10 KHz rate.  This is
    // used to control the clock rate of the SoftSSI module; the clock rate of
    // the SoftSSI Clk signal will be 1/2 the interrupt rate.
    //
    SysTickPeriodSet(SysCtlClockGet() / 20000);
    SysTickIntEnable();
    SysTickEnable();

  while(1)
    {
		//
        // Cycle through all available faces...
        // Face index updated inside the SysTickIntHandler.
        //
        PrintByteArray(g_ucFaces[g_ulFaceIndex]);
        
    }

}
