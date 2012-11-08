//*****************************************************************************
//
// qs-scope.h - Shared variables and functions exported by qs-scope.c
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
// This is part of revision 9453 of the EK-LM3S3748 Firmware Package.
//
//*****************************************************************************

#ifndef __QS_SCOPE_H__
#define __QS_SCOPE_H__

//*****************************************************************************
//
// Interrupt priorities.  Note that ADC0 must have higher priority than any
// other interrupt except the GPIO port which is being used to allow us to
// abort a capture request in cases where the ADC is sampling at very high
// rates.
//
//*****************************************************************************
#define ABORT_INT_PRIORITY      0x00
#define ADC_INT_PRIORITY        0x20
#define SYSTICK_INT_PRIORITY    0xE0
#define UART_INT_PRIORITY       0xE0
#define AUDIO_INT_PRIORITY      0xE0

//*****************************************************************************
//
// The SysTick frequency and tick counter.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND     100
extern volatile unsigned long g_ulSysTickCounter;

//*****************************************************************************
//
// The minimum time between capture requests expressed in terms of system
// ticks.  10 represents 1/10 second with SYSTICKS_PER_SECOND at 100.
//
//*****************************************************************************
#define CAPTURES_PER_SECOND     10

//*****************************************************************************
//
// The default values for various oscilloscope parameters.
//
//*****************************************************************************
#define DEFAULT_TRIGGER_LEVEL_MV \
                                0
#define DEFAULT_TIMEBASE_US     100
#define DEFAULT_SCALE_MV        1000

//*****************************************************************************
//
// Main loop command codes.
//
// Note: These commands must be encoded such that the bit position which is
// set matches the index into the main command handler table,
// g_pfnCommandHandlerArray in qs_scope.c.
//
//*****************************************************************************
#define NUM_SCOPE_COMMANDS      21

#define SCOPE_CHANGE_TIMEBASE   0
#define SCOPE_SYSTICK           1
#define SCOPE_CHANGE_TRIGGER    2
#define SCOPE_CH2_DISPLAY       3
#define SCOPE_CAPTURE           4
#define SCOPE_STOP              5
#define SCOPE_START             6
#define SCOPE_TRIGGER_LEVEL     7
#define SCOPE_TRIGGER_POS       8
#define SCOPE_CH1_SCALE         9
#define SCOPE_CH2_SCALE         10
#define SCOPE_CH1_POS           11
#define SCOPE_CH2_POS           12
#define SCOPE_SAVE              13
#define SCOPE_RETRANSMIT        14
#define SCOPE_SET_TRIGGER_CH    15
#define SCOPE_USB_HOST_CONNECT  16
#define SCOPE_USB_HOST_REMOVE   17
#define SCOPE_FIND              18
#define SCOPE_SHOW_HELP         19
#define SCOPE_SET_USB_MODE      20

#define SCOPE_CMD_TO_FLAG(x)    (1 << (x))

//*****************************************************************************
//
// Labels defining parameters passed alongside the SCOPE_SAVE command.
//
//*****************************************************************************
#define SCOPE_SAVE_CSV          0x00000000
#define SCOPE_SAVE_BMP          0x00000001
#define SCOPE_SAVE_SD           0x00000000
#define SCOPE_SAVE_USB          0x80000000

#define SCOPE_SAVE_FORMAT_MASK  0x00000001
#define SCOPE_SAVE_DRIVE_MASK   0x80000000

//*****************************************************************************
//
// Main loop command flags and their associated parameters.
//
//*****************************************************************************
extern unsigned long g_ulCommand;
extern unsigned long g_ulCommandParam[];

//*****************************************************************************
//
// Macro used to set a bit in the global command flags variable.
//
// ulCmd must be one of the SCOPE_XXX commands defined above.
//
//*****************************************************************************
#define COMMAND_FLAG_WRITE(ulCmd, ulParam)           \
        do                                           \
        {                                            \
            tBoolean bRet;                           \
            ASSERT(ulCmd < NUM_SCOPE_COMMANDS);      \
            bRet = IntMasterDisable();               \
            g_ulCommand |= SCOPE_CMD_TO_FLAG(ulCmd); \
            g_ulCommandParam[ulCmd] = ulParam;       \
            if(!bRet)                                \
            {                                        \
                IntMasterEnable();                   \
            }                                        \
        }                                            \
        while(0)

//*****************************************************************************
//
// Macro used to clear a bit in the global command flags variable.
//
// ulCmd must be one of the SCOPE_XXX commands defined above.
//
//*****************************************************************************
#define COMMAND_FLAG_CLEAR(ulCmd)                                    \
        do                                                           \
        {                                                            \
            tBoolean bRet;                                           \
            ASSERT(ulCmd < NUM_SCOPE_COMMANDS);                      \
            bRet = IntMasterDisable();                               \
            g_ulCommand = (g_ulCommand & ~SCOPE_CMD_TO_FLAG(ulCmd)); \
            if(!bRet)                                                \
            {                                                        \
                IntMasterEnable();                                   \
            }                                                        \
        }                                                            \
        while(0)

//*****************************************************************************
//
// I/O handle.
//
//*****************************************************************************
extern int g_hStdio;

//*****************************************************************************
//
// Flag indicating whether we are operating as a USB device or host.
//
//*****************************************************************************
extern tBoolean g_bUSBModeIsHost;

//*****************************************************************************
//
// Flag indicating whether we are capturing oscilloscope data continuously
// or operating in one-shot mode.
//
//*****************************************************************************
extern tBoolean g_bContinuousCapture;

//*****************************************************************************
//
// Flag indicating whether or not the connection help screen is currently being
// displayed.
//
//*****************************************************************************
extern tBoolean g_bShowingHelp;

//*****************************************************************************
//
// Global used to store the trigger type between the user making the request
// and it being applied.
//
//*****************************************************************************
extern tTriggerType g_eNewTrigger;

//*****************************************************************************
//
// Oscilloscope capture buffer.  This buffer needs to be large enough to hold
// 4 times as many samples as there are pixels across the waveform display area
// of the screen.  This ensure that, when performing dual channel capture, we
// can set the trigger point at the center of the buffer and still have enough
// samples before and after it to move the trigger point anywhere on the
// display and still have samples visible in the display window.
//
// For best performance, this value should also be a power of two.
//
//*****************************************************************************
#define MAX_SAMPLES_PER_TRIGGER 512
extern unsigned short g_usScopeData[MAX_SAMPLES_PER_TRIGGER];

//*****************************************************************************
//
// Stores flags indicating which channels are currently enabled.
//
//*****************************************************************************
extern tBoolean g_pbActiveChannels[];

//*****************************************************************************
//
// Return the smaller of two values.
//
//*****************************************************************************
#define min(a, b)               ((a) < (b) ? (a) : (b))

//*****************************************************************************
//
// Return the absolute value of a number.
//
//*****************************************************************************
#define abs(a)                  ((a) < 0 ? -(a) : (a))

//*****************************************************************************
//
// Error check and exit macro.
//
//*****************************************************************************
#define ERROR_CHECK(expr, msg) \
        if(!expr)              \
        {                      \
            UARTprintf(msg);   \
            while(1)           \
            {                  \
            }                  \
        }

//*****************************************************************************
//
// Functions exported from qs_scope.c
//
//*****************************************************************************
extern void UpdateWaveform(tBoolean bMenuShown, tBoolean bHelpShown,
                           tBoolean bNewData);

#endif // __QS_SCOPE_H__
