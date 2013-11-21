//*****************************************************************************
//
// udma_uart_sg.c - uDMA scatter-gather example with UART
//
// Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9107 of the EK-LM3S9B92 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "inc/hw_udma.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>uDMA scatter-gather memory/UART transfer (udma_uart_sg)</h1>
//!
//! This example demonstrates using the scatter-gather mode of the uDMA
//! controller to transfer multiple memory buffers to and from a UART.
//! This example uses UART1 in loopback mode.
//!
//! UART0, connected to the FTDI virtual COM port and running at 115,200,
//! 8-N-1, is used to display messages from this application.
//
//*****************************************************************************

//*****************************************************************************
//
// The following macro is used to compile the scatter-gather configuration
// code using a new simplified API, uDMAChannelScatterGatherSet(), for
// configuring a scatter-gather transfer.  If this macro is not defined, then
// it uses the older method of configuring the scatter-gather transfer via the
// two functions uDMAChannelControlSet() and uDMAChannelTransferSet().
//
//*****************************************************************************
#define USE_SGSET_API

//*****************************************************************************
//
// Several data buffers are defined.  The following buffers are used:
//
// ucSrcBuf - original source data that is populated with a test pattern
//
// ucSrcBuf1-ucSrcBuf3 - source buffers that contain fragments of the original
// source buffer.  These buffers are populated by a memory scatter-gather
// transfer.  These buffers are ideally located in different locations in
// memory and not contiguous, for the purposes of demonstrating scatter-gather.
//
// ucDstBuf1-ucDstBuf3 - destination buffers that contain the data that was
// collected in fragments from the peripheral.  These buffers are ideally
// located in different locations in memory.
//
// ucDstBuf - the destination buffer after the destination fragment buffers
// have been re-assembled by another memory scatter-gather transfer
//
//*****************************************************************************
static unsigned char g_ucSrcBuf[1024];
static unsigned char g_ucSrcBuf1[300];
static unsigned char g_ucDstBuf1[123];
static unsigned char g_ucSrcBuf2[645];
static unsigned char g_ucDstBuf2[345];
static unsigned char g_ucSrcBuf3[79];
static unsigned char g_ucDstBuf3[556];
static unsigned char g_ucDstBuf[1024];
#define SRC_IDX_1 0
#define SRC_IDX_2 sizeof(g_ucSrcBuf1)
#define SRC_IDX_3 (SRC_IDX_2 + sizeof(g_ucSrcBuf2))
#define DST_IDX_1 0
#define DST_IDX_2 sizeof(g_ucDstBuf1)
#define DST_IDX_3 (DST_IDX_2 + sizeof(g_ucDstBuf2))

//*****************************************************************************
//
// Counters used to count how many times certain ISR event occur.
//
//*****************************************************************************
static volatile unsigned long g_ulDMAIntCount = 0;
static volatile unsigned long g_uluDMAErrCount = 0;

//*****************************************************************************
//
// Flags to indicate when the TX and RX DMA operations are completed.
//
//*****************************************************************************
static volatile unsigned int g_bTXdone = 0;
static volatile unsigned int g_bRXdone = 0;

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
tDMAControlTable sControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(sControlTable, 1024)
tDMAControlTable sControlTable[64];
#else
tDMAControlTable sControlTable[64] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// This is the task list that defines the DMA scatter-gather operation for the
// UART TX channel.  This task list starts by copying the original source
// buffer into 3 different fragment buffers.  After that, it copies the data
// from the fragment buffers to the UART output.
//
// For this task list, we can use a trick to cause the first 3 memory
// tasks to execute, even though they are memory operations configured as
// peripheral scatter-gather.  The reason this works is because the peripheral
// is the UART TX channel, and it will always be making a DMA request as long
// as there is room in the TX FIFO.  This means that the first 3 tasks can be
// configured as peripheral scatter-gather, and they will execute because we
// know that the peripheral will be making a request, and the tasks will run.
//
// Task 1-3: each copy a section of the original source buffer to one of 3
// fragment buffers.  These are memory to memory operations, but using the
// peripheral channel.
//
// Task 4-6: each copy one of the fragment buffers to the UART using peripheral
// scatter-gather transfer.
//
// For all the tasks, the data size and increment sizes are 8-bits.
//
//*****************************************************************************
tDMAControlTable g_TaskTableSrc[] =
{
    //
    // Task 1: copy source buffer fragment 1
    //
    uDMATaskStructEntry(sizeof(g_ucSrcBuf1), UDMA_SIZE_8,
                        UDMA_SRC_INC_8, &g_ucSrcBuf[SRC_IDX_1],
                        UDMA_DST_INC_8, g_ucSrcBuf1,
                        UDMA_ARB_8, UDMA_MODE_PER_SCATTER_GATHER),

    //
    // Task 2: copy source buffer fragment 2
    //
    uDMATaskStructEntry(sizeof(g_ucSrcBuf2), UDMA_SIZE_8,
                        UDMA_SRC_INC_8, &g_ucSrcBuf[SRC_IDX_2],
                        UDMA_DST_INC_8, g_ucSrcBuf2,
                        UDMA_ARB_8, UDMA_MODE_PER_SCATTER_GATHER),

    //
    // Task 3: copy source buffer fragment 3
    //
    uDMATaskStructEntry(sizeof(g_ucSrcBuf3), UDMA_SIZE_8,
                        UDMA_SRC_INC_8, &g_ucSrcBuf[SRC_IDX_3],
                        UDMA_DST_INC_8, g_ucSrcBuf3,
                        UDMA_ARB_8, UDMA_MODE_PER_SCATTER_GATHER),

    //
    // Task 4: copy fragment buffer 1 to UART
    // arb size is 8 to match UART FIFO trigger level
    //
    uDMATaskStructEntry(sizeof(g_ucSrcBuf1), UDMA_SIZE_8,
                        UDMA_SRC_INC_8, g_ucSrcBuf1,
                        UDMA_DST_INC_NONE, (void *)(UART1_BASE + UART_O_DR),
                        UDMA_ARB_8, UDMA_MODE_PER_SCATTER_GATHER),

    //
    // Task 5: copy fragment buffer 2 to UART
    // arb size is 8 to match UART FIFO trigger level
    //
    uDMATaskStructEntry(sizeof(g_ucSrcBuf2), UDMA_SIZE_8,
                        UDMA_SRC_INC_8, g_ucSrcBuf2,
                        UDMA_DST_INC_NONE, (void *)(UART1_BASE + UART_O_DR),
                        UDMA_ARB_8, UDMA_MODE_PER_SCATTER_GATHER),

    //
    // Task 6: copy fragment buffer 3 to UART
    // arb size is 8 to match UART FIFO trigger level
    // mode is basic since this is last task
    //
    uDMATaskStructEntry(sizeof(g_ucSrcBuf3), UDMA_SIZE_8,
                        UDMA_SRC_INC_8, g_ucSrcBuf3,
                        UDMA_DST_INC_NONE, (void *)(UART1_BASE + UART_O_DR),
                        UDMA_ARB_8, UDMA_MODE_BASIC)
};

//*****************************************************************************
//
// This is the task list that defines the DMA scatter-gather operation for the
// UART RX channel.  This task list starts receiving UART data into fragment
// buffers.  Once that is complete, it re-assembles all the fragment buffers
// into one buffer.
//
// As with the previous task list, we need a trick in order to perform both
// peripheral and memory-to-memory operations.  The first 3 tasks are
// configured as peripheral scatter-gather.  The peripheral (UART RX) will make
// a request whenever there is received data available, and the uDMA controller
// will execute the tasks and transfer the incoming data into 3 fragment
// buffers.
//
// The third task copies all of the remaining data, except for one byte.  This
// ensures that the peripheral will make one more request for the last byte.
// After the third task executes, the uDMA will go back to waiting for another
// request from the peripheral.  This is because the entire operation is
// configured as peripheral scatter-gather (despite the fact that some tasks
// are memory scatter-gather).  Now when the UART receives the last byte it
// will make another request and the uDMA controller will execute the fourth
// task.  This task copies the last byte from the UART to the last fragment
// buffer, and it is configured as a memory scatter-gather.  This means it will
// no longer wait for the peripheral to make a request.
//
// This trick is used only for the purpose of being able to follow peripheral
// operations by memory operations in a single task list.  For a straightforward
// set of memory or peripheral tasks, this "trick" would not be necessary.
//
// After task 4, which copies the last byte from the UART, there are 3 more
// tasks (4-7) that perform memory-to-memory copies of the fragment buffers
// into one contiguous buffer (re-assembly).  All of these remaining tasks are
// configured as memory scatter-gather and they will execute without waiting
// for a request from the peripheral.
//
// Task 1-3: each copy data from the UART to a fragment buffer using peripheral
// scatter-gather transfer.
//
// Task 4: copy the last received byte from the UART to the last fragment
// buffer, using memory scatter-gather.  This allows the remaining tasks to
// run without waiting for a request from the peripheral.
//
// Task 5-7: each copy a fragment buffer to the final destination buffer using
// memory scatter-gather transfers.
//
// For all the tasks, the data size and increment sizes are 8-bits.
//
//*****************************************************************************
tDMAControlTable g_TaskTableDst[] =
{
    //
    // Task 1: copy UART data into fragment buffer 1
    //
    uDMATaskStructEntry(sizeof(g_ucDstBuf1), UDMA_SIZE_8,
                        UDMA_SRC_INC_NONE, (void *)(UART1_BASE + UART_O_DR),
                        UDMA_DST_INC_8, g_ucDstBuf1,
                        UDMA_ARB_8, UDMA_MODE_PER_SCATTER_GATHER),

    //
    // Task 2: copy UART data into fragment buffer 2
    //
    uDMATaskStructEntry(sizeof(g_ucDstBuf2), UDMA_SIZE_8,
                        UDMA_SRC_INC_NONE, (void *)(UART1_BASE + UART_O_DR),
                        UDMA_DST_INC_8, g_ucDstBuf2,
                        UDMA_ARB_8, UDMA_MODE_PER_SCATTER_GATHER),

    //
    // Task 3: copy UART data into fragment buffer 3,
    // all but last 1 byte
    //
    uDMATaskStructEntry(sizeof(g_ucDstBuf3) - 1, UDMA_SIZE_8,
                        UDMA_SRC_INC_NONE, (void *)(UART1_BASE + UART_O_DR),
                        UDMA_DST_INC_8, g_ucDstBuf3,
                        UDMA_ARB_8, UDMA_MODE_PER_SCATTER_GATHER),

    //
    // Task 4: copy last 1 byte from UART to end of last fragment buffer
    // This transfer lets us switch to memory scatter gather for the remaining
    // tasks
    //
    uDMATaskStructEntry(1, UDMA_SIZE_8,
                        UDMA_SRC_INC_NONE, (void *)(UART1_BASE + UART_O_DR),
                        UDMA_DST_INC_8, &g_ucDstBuf3[sizeof(g_ucDstBuf3) - 1],
                        UDMA_ARB_8, UDMA_MODE_MEM_SCATTER_GATHER),

    //
    // Task 5: copy destination fragment 1 to final buffer
    //
    uDMATaskStructEntry(sizeof(g_ucDstBuf1), UDMA_SIZE_8,
                        UDMA_SRC_INC_8, g_ucDstBuf1,
                        UDMA_DST_INC_8, &g_ucDstBuf[DST_IDX_1],
                        UDMA_ARB_8, UDMA_MODE_MEM_SCATTER_GATHER),

    //
    // Task 6: copy destination fragment 2 to final buffer
    //
    uDMATaskStructEntry(sizeof(g_ucDstBuf2), UDMA_SIZE_8,
                        UDMA_SRC_INC_8, g_ucDstBuf2,
                        UDMA_DST_INC_8, &g_ucDstBuf[DST_IDX_2],
                        UDMA_ARB_8, UDMA_MODE_MEM_SCATTER_GATHER),

    //
    // Task 7: copy destination fragment 3 to final buffer
    // the mode is AUTO since this is the last task
    //
    uDMATaskStructEntry(sizeof(g_ucDstBuf3), UDMA_SIZE_8,
                        UDMA_SRC_INC_8, g_ucDstBuf3,
                        UDMA_DST_INC_8, &g_ucDstBuf[DST_IDX_3],
                        UDMA_ARB_8, UDMA_MODE_AUTO)
};

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
// The interrupt handler for uDMA errors.  This interrupt will occur if the
// uDMA encounters a bus error while trying to perform a transfer.  This
// handler just increments a counter if an error occurs.
//
//*****************************************************************************
void
uDMAErrorHandler(void)
{
    unsigned long ulStatus;

    //
    // Check for uDMA error bit
    //
    ulStatus = MAP_uDMAErrorStatusGet();

    //
    // If there is a uDMA error, then clear the error and increment
    // the error counter.
    //
    if(ulStatus)
    {
        MAP_uDMAErrorStatusClear();
        g_uluDMAErrCount++;
    }
}

//*****************************************************************************
//
// The interrupt handler for UART1.  This interrupt will occur when a DMA
// transfer is complete using the UART1 uDMA channel.  It will also be
// triggered if the peripheral signals an error.  This interrupt handler
// will set a flag when each scatter-gather transfer is complete (one for each
// of UART RX and TX).
//
//*****************************************************************************
void
UART1IntHandler(void)
{
    unsigned long ulStatus;

    //
    // Read the interrupt status of the UART.
    //
    ulStatus = MAP_UARTIntStatus(UART1_BASE, 1);

    //
    // Clear any pending status, even though there should be none since no UART
    // interrupts were enabled.  If UART error interrupts were enabled, then
    // those interrupts could occur here and should be handled.  Since uDMA is
    // used for both the RX and TX, then neither of those interrupts should be
    // enabled.
    //
    MAP_UARTIntClear(UART1_BASE, ulStatus);

    //
    // Count the number of times this interrupt occurred.
    //
    g_ulDMAIntCount++;

    //
    // Check the UART TX DMA channel to see if it is enabled.  When it is
    // finished with the transfer it will be automatically disabled.
    //
    if(!MAP_uDMAChannelIsEnabled(UDMA_CHANNEL_UART1TX))
    {
        g_bTXdone= 1;
    }

    //
    // Check the UART RX DMA channel to see if it is enabled.  When it is
    // finished with the transfer it will be automatically disabled.
    //
    if(!MAP_uDMAChannelIsEnabled(UDMA_CHANNEL_UART1RX))
    {
        g_bRXdone= 1;
    }
}

//*****************************************************************************
//
// The main function sets up the peripherals for the example, then enters
// a wait loop until the DMA transfers are complete.  At the end some
// information is printed for the user.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulIdx;

    //
    // Set the clocking to run directly from the PLL at 50 MHz.
    //
    MAP_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Initialize the console UART and write a message to the terminal.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);
    UARTprintf("\033[2JMemory/UART scatter-gather uDMA example\n\n");

    //
    // Configure UART1 to be used for the loopback peripheral
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

    //
    // Configure the UART communication parameters.
    //
    MAP_UARTConfigSetExpClk(UART1_BASE, MAP_SysCtlClockGet(), 115200,
                            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_PAR_NONE);

    //
    // Set both the TX and RX trigger thresholds to one-half (8 bytes).  This
    // will be used by the uDMA controller to signal when more data should be
    // transferred.  The uDMA TX and RX channels will be configured so that it
    // can transfer 8 bytes in a burst when the UART is ready to transfer more
    // data.
    //
    MAP_UARTFIFOLevelSet(UART1_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8);

    //
    // Enable the UART for operation, and enable the uDMA interface for both TX
    // and RX channels.
    //
    MAP_UARTEnable(UART1_BASE);
    MAP_UARTDMAEnable(UART1_BASE, UART_DMA_RX | UART_DMA_TX);

    //
    // This register write will set the UART to operate in loopback mode.  Any
    // data sent on the TX output will be received on the RX input.
    //
    HWREG(UART1_BASE + UART_O_CTL) |= UART_CTL_LBE;

    //
    // Enable the UART peripheral interrupts.  Note that no UART interrupts
    // were enabled, but the uDMA controller will cause an interrupt on the
    // UART interrupt signal when a uDMA transfer is complete.
    //
    MAP_IntEnable(INT_UART1);

    //
    // Enable the uDMA peripheral clocking.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);

    //
    // Enable the uDMA controller.
    //
    MAP_uDMAEnable();

    //
    // Point at the control table to use for channel control structures.
    //
    MAP_uDMAControlBaseSet(sControlTable);

    //
    // Configure the UART TX channel for scatter-gather
    // Peripheral scatter-gather is used because transfers are gated by
    // requests from the peripheral
    //
    UARTprintf("Configuring UART TX uDMA channel for scatter-gather\n");
#ifndef USE_SGSET_API
    //
    // Use the original method for configuring the scatter-gather transfer
    //
    uDMAChannelControlSet(UDMA_CHANNEL_UART1TX, UDMA_SIZE_32 |
                          UDMA_SRC_INC_32 | UDMA_DST_INC_32 | UDMA_ARB_4);
    uDMAChannelTransferSet(UDMA_CHANNEL_UART1TX, UDMA_MODE_PER_SCATTER_GATHER,
                           g_TaskTableSrc,
                           &sControlTable[UDMA_CHANNEL_UART1TX], 6 * 4);
#else
    //
    // Use the simplified API for configuring the scatter-gather transfer
    //
    uDMAChannelScatterGatherSet(UDMA_CHANNEL_UART1TX, 6, g_TaskTableSrc, 1);
#endif

    //
    // Configure the UART RX channel for scatter-gather task list.
    // This is set to peripheral s-g because it starts by receiving data
    // from the UART
    //
    UARTprintf("Configuring UART RX uDMA channel for scatter-gather\n");
#ifndef USE_SGSET_API
    //
    // Use the original method for configuring the scatter-gather transfer
    //
    uDMAChannelControlSet(UDMA_CHANNEL_UART1RX, UDMA_SIZE_32 |
                          UDMA_SRC_INC_32 | UDMA_DST_INC_32 | UDMA_ARB_4);
    uDMAChannelTransferSet(UDMA_CHANNEL_UART1RX, UDMA_MODE_PER_SCATTER_GATHER,
                           g_TaskTableDst,
                           &sControlTable[UDMA_CHANNEL_UART1RX], 7 * 4);
#else
    //
    // Use the simplified API for configuring the scatter-gather transfer
    //
    uDMAChannelScatterGatherSet(UDMA_CHANNEL_UART1RX, 7, g_TaskTableDst, 1);
#endif

    //
    // Fill the source buffer with a pattern
    //
    for(ulIdx = 0; ulIdx < 1024; ulIdx++)
    {
        g_ucSrcBuf[ulIdx] = ulIdx + (ulIdx / 256);
    }

    //
    // Enable the uDMA controller error interrupt.  This interrupt will occur
    // if there is a bus error during a transfer.
    //
    IntEnable(INT_UDMAERR);

    //
    // Enable the UART RX DMA channel.  It will wait for data to be available
    // from the UART.
    //
    UARTprintf("Enabling uDMA channel for UART RX\n");
    MAP_uDMAChannelEnable(UDMA_CHANNEL_UART1RX);

    //
    // Enable the UART TX DMA channel.  Since the UART TX will be asserting
    // a DMA request (since the TX FIFO is empty), this will cause this
    // DMA channel to start running.
    //
    UARTprintf("Enabling uDMA channel for UART TX\n");
    MAP_uDMAChannelEnable(UDMA_CHANNEL_UART1TX);

    //
    // Wait for the TX task list to be finished
    //
    UARTprintf("Waiting for TX task list to finish ... ");
    while(!g_bTXdone)
    {
    }
    UARTprintf("done\n");

    //
    // Wait for the RX task list to be finished
    //
    UARTprintf("Waiting for RX task list to finish ... ");
    while(!g_bRXdone)
    {
    }
    UARTprintf("done\n");

    //
    // Verify that all the counters are in the expected state
    //
    UARTprintf("Verifying counters\n");
    if(g_ulDMAIntCount != 2)
    {
        UARTprintf("ERROR in interrupt count, found %d, expected 2\n",
                  g_ulDMAIntCount);
    }
    if(g_uluDMAErrCount != 0)
    {
        UARTprintf("ERROR in error counter, found %d, expected 0\n",
                  g_uluDMAErrCount);
    }

    //
    // Now verify the contents of the final destination buffer.  Compare it
    // to the original source buffer.
    //
    UARTprintf("Verifying buffer contents ... ");
    for(ulIdx = 0; ulIdx < 1024; ulIdx++)
    {
        if(g_ucDstBuf[ulIdx] != g_ucSrcBuf[ulIdx])
        {
            UARTprintf("ERROR\n    @ index %d: expected 0x%02X, found 0x%02X\n",
                       ulIdx, g_ucSrcBuf[ulIdx], g_ucDstBuf[ulIdx]);
            UARTprintf("Checking stopped.  There may be additional errors\n");
            break;
        }
    }
    if(ulIdx == 1024)
    {
        UARTprintf("OK\n");
    }

    //
    // End of program, loop forever
    //
    for(;;)
    {
    }
}
