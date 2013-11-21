//****************************************************************************
//
// qs-autonomous.c - Autonomous operation example for EVALBOT
//
// Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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
//****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "drivers/display96x16x1.h"
#include "utils/scheduler.h"
#include "utils/uartstdio.h"
#include "drive_task.h"
#include "display_task.h"
#include "led_task.h"
#include "sound_task.h"
#include "auto_task.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>EVALBOT Autonomous Operation (qs-autonomous)</h1>
//!
//! This application controls the EVALBOT, allowing it to operate
//! autonomously.  When first turned on, the EVALBOT shows messages on the
//! display and toggles the two LEDs.  Upon pressing button 1, the robot drives
//! in a straight line.  If one of the bumper sensors is activated, EVALBOT
//! stops immediately, rotates a random angle, and then resumes driving
//! forward.  While driving forward, the EVALBOT also makes turns after a
//! random duration of time.  Pressing button 2 stops the motion.
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
    UARTprintf("DriverLib assertion error in %s@%d\n", pcFilename, ulLine);
    for(;;)
    {
    }
}
#endif

//*****************************************************************************
//
// The DMA control structure table.  This is required by the sound driver.
//
//*****************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable sDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(sDMAControlTable, 1024)
tDMAControlTable sDMAControlTable[64];
#else
tDMAControlTable sDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The Scheduler task table.  This table holds function pointers to all the
// tasks that will be called by the simple scheduler, along with the periodic
// timeout value.
//
//*****************************************************************************
tSchedulerTask g_psSchedulerTable[] =
{
    { DriveTask, 0, 10, 0, true },
    { DisplayTask, 0, 5, 0, true },
    { LEDTask, 0, 100, 0, true },
    { SoundTask, 0, 1, 0, true },
    { AutoTask, 0, 10, 0, true },
};

//*****************************************************************************
//
// The number of entries in the global scheduler task table.
//
//*****************************************************************************
unsigned long g_ulSchedulerNumTasks = (sizeof(g_psSchedulerTable) /
                                       sizeof(tSchedulerTask));

//*****************************************************************************
//
// The main entry point for the qs-autonomous example.  It initializes the
// system, then eners a forever loop where it runs the scheduler.  The
// scheduler then periodically calls all the tasks in the example to keep
// the program running.
//
//*****************************************************************************
int
main (void)
{
    //
    // Set the system clock to run at 50MHz from the PLL
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Enable UART0, to be used as a serial console.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART standard I/O.
    //
    UARTStdioInit(0);
    UARTprintf("EVALBOT starting\n");

    //
    // Initialize the simple scheduler to use a tick rate of 100 Hz.
    //
    SchedulerInit(100);

    //
    // Initialize all the tasks used in the example.
    //
    DriveInit();
    DisplayTaskInit();
    LEDTaskInit();
    SoundTaskInit();
    AutoTaskInit();

    //
    // Enter a forever loop and call the scheduler.  The scheduler will
    // periodically call all the tasks in the system according to the timeout
    // value of each task.
    //
    for(;;)
    {
        SchedulerRun();
    }
}
