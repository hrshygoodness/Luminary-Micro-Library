//*****************************************************************************
//
// boot_demo2.c - Second boot loader example.
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
// This is part of revision 8555 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_flash.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "grlib/grlib.h"
#include "drivers/formike128x128x16.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Boot Loader Demo 2 (boot_demo2)</h1>
//!
//! An example to demonstrate the use of the ROM-based boot loader.  At
//! startup, the application will configure the UART, wait for the select
//! button to be pressed, and then branch to the boot loader to await the start
//! of an update.  The UART will always be configured at 115,200 baud and does
//! not require the use of auto-bauding.
//!
//! The boot_demo1 application can be used along with this application to
//! easily demonstrate that the boot loader is actually updating the on-chip
//! flash.
//
//*****************************************************************************

//*****************************************************************************
//
// Graphics context used to show text on the CSTN display.
//
//*****************************************************************************
tContext g_sContext;

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
// Demonstrate the use of the boot loader.
//
//*****************************************************************************
int
main(void)
{
    tRectangle sRect;

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_8MHZ);

    //
    // If running on Rev A0 silicon, write the FMPRE[2-3]/FMPPE[2-3] registers
    // to zero.  This is a workaround to allow the mass erase in the ROM-based
    // boot loader to succeed if a locked device recovery has been performed.
    //
    if(REVISION_IS_A0)
    {
        HWREG(FLASH_FMPPE2) = 0;
        HWREG(FLASH_FMPPE3) = 0;
        HWREG(FLASH_FMPRE2) = 0;
        HWREG(FLASH_FMPRE3) = 0;
    }

    //
    // Enable the UART and GPIO modules.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Make the UART pins be peripheral controlled.
    //
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    //
    // Initialize the display driver.
    //
    Formike128x128x16Init();

    //
    // Turn on the backlight.
    //
    Formike128x128x16BacklightOn();

    //
    // Initialize the graphics context.
    //
    GrContextInit(&g_sContext, &g_sFormike128x128x16);

    //
    // Fill the top 15 rows of the screen with blue to create the banner.
    //
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&g_sContext) - 1;
    sRect.sYMax = 14;
    GrContextForegroundSet(&g_sContext, ClrDarkBlue);
    GrRectFill(&g_sContext, &sRect);

    //
    // Put a white box around the banner.
    //
    GrContextForegroundSet(&g_sContext, ClrWhite);
    GrRectDraw(&g_sContext, &sRect);

    //
    // Put the application name in the middle of the banner.
    //
    GrContextFontSet(&g_sContext, g_pFontFixed6x8);
    GrStringDrawCentered(&g_sContext, "boot_demo2", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 7, 0);

    //
    // Indicate what is happening.
    //
    GrStringDrawCentered(&g_sContext, "Press the select", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 24, 0);
    GrStringDrawCentered(&g_sContext, "button to start the", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 32, 0);
    GrStringDrawCentered(&g_sContext, "boot loader", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 40, 0);

    //
    // Enable the GPIO pin to read the select button.
    //
    ROM_GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);

    //
    // Wait until the select button has been pressed.
    //
    while(ROM_GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_7) != 0)
    {
    }

    //
    // Drain any data that may be in the UART fifo.
    //
    while(ROM_UARTCharsAvail(UART0_BASE))
    {
        ROM_UARTCharGet(UART0_BASE);
    }

    //
    // Indicate that the boot loader is being called.
    //
    GrStringDrawCentered(&g_sContext, "The boot loader is", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 56, 0);
    GrStringDrawCentered(&g_sContext, "now running and", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 64, 0);
    GrStringDrawCentered(&g_sContext, "awaiting an update", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 72, 0);
    GrStringDrawCentered(&g_sContext, "over UART0 at", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 80, 0);
    GrStringDrawCentered(&g_sContext, "115200, 8-N-1.", -1,
                         GrContextDpyWidthGet(&g_sContext) / 2, 88, 0);

    //
    // Call the boot loader so that it will listen for an update on the UART.
    //
    ROM_UpdateUART();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
