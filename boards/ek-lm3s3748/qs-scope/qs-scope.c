//*****************************************************************************
//
// qs-scope.c - Quickstart Oscilloscope application main source file.
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

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Quickstart Oscilloscope (qs-scope)</h1>
//!
//! A two channel oscilloscope implemented using the Stellaris
//! microcontroller's analog-to-digital converter (ADC).  The oscilloscope
//! supports sample rates of up to 1M sample per second and will show the
//! captured waveforms on the color STN display.  On-screen menus provide user
//! control over timebase, channel voltage scale and position, trigger type,
//! trigger level and trigger position.  Other features include the ability
//! to save captured data as comma-separated-value files suitable for use with
//! spreadsheet applications or bitmap images on either an installed microSD
//! card or a USB flash drive.  The board may also be connected to a WindowsXP
//! or Vista host system and controlled remotely using a Windows application.
//!
//! \addtogroup example_list
//! <b>Oscilloscope User Interface</b>
//!
//! All oscilloscope controls and settings are accessed using the navigation
//! control on the board.  This control offers up, down, left, right and select
//! functions in a single unit.  Rocking the control in the desired direction
//! sends ``up'', ``down'', ``left'' or ``right'' messages to the application
//! and pressing on the center sends a ``select'' message.
//!
//! Controls and settings are arranged into groups by function such as display
//! settings, trigger settings, file operations and setup choices.  These
//! groups are accessed by pressing ``select'' to display the main menu.  With
//! the menu displayed, use ``up'' and ``down'' to select between the available
//! groups.  When the desired group is highlighted, press ``select'' once again
//! to dismiss the menu.
//!
//! Controls from the currently selected group are shown in the bottom portion
//! of the application display.  Use ``up'' and ``down'' to cycle through the
//! controls in the group and ``left'' and ``right'' to change the value of, or
//! select the action associated with the control which is currently
//! displayed.
//!
//! The control groups and the individual controls offered by each are
//! outlined below:
//!
//! \verbatim
//!   Group    Control       Setting
//!   -----    -------       -------
//!
//!   Display
//!            Channel 2     ON or OFF.
//!            Timebase      Select values from 2uS to 50mS per division.
//!            Ch1 Scale     Select values from 100mV to 10V per division.
//!            Ch2 Scale     Select values from 100mV to 10V per division.
//!            Ch1 Offset    Press and hold "left" or "right" to move the
//!                          waveform up or down in 100mV increments.
//!            Ch2 Offset    Press and hold "left" or "right" to move the
//!                          waveform up or down in 100mV increments.
//!   Trigger
//!            Trigger       The trigger type - Always, Rising, Falling or
//!                          Level.
//!            Trig Channel  1 or 2 to select the channel used for triggering.
//!            Trig Level    Press and hold "left" or "right" to change the
//!                          trigger level in 100mV increments.
//!            Trig Pos      Press and hold "left" or "right" to move the
//!                          trigger position on the display.
//!            Mode          Running or Stopped.
//!            One Shot      If the current mode is "Stopped", pressing
//!                          "left" or "right" initiates capture and display
//!                          of a single waveform.
//!   Setup
//!            Captions      Select ON to show the timebase and scale
//!                          captions or OFF to remove them from the display.
//!            Voltages      Select ON to show the measured voltages for each
//!                          channel or OFF to remove them from the display.
//!            Grid          Select ON to show the graticule lines or OFF to
//!                          remove them from the display.
//!            Ground        Select ON to show dotted lines corresponding
//!                          to the ground levels for each channel or OFF
//!                          to remove them from the display.
//!            Trig Level    Select ON to show a solid horizontal line
//!                          corresponding to the trigger level for the
//!                          trigger channel or OFF to remove this line from
//!                          the display.
//!            Trig Pos      Select ON to show a solid vertical line at the
//!                          trigger position or OFF to remove this line from
//!                          the display.
//!            Clicks        Select ON to enable sounds on button presses or
//!                          OFF to disable them.
//!            USB Mode      Select Host to operate in USB host mode and
//!                          allow use of flash memory sticks, or Device to
//!                          operate as a USB device and allow connection to
//!                          a host PC system.
//!   File
//!            CSV on SD     Save the current waveform data as a text file
//!                          on an installed microSD card.
//!            CSV on USB    Save the current waveform data as a text file
//!                          on an installed USB flash stick (if in USB host
//!                          mode - see the Setup group above).
//!            BMP on SD     Save the current waveform display as a bitmap
//!                          on an installed microSD card.
//!            BMP on USB    Save the current waveform display as a bitmap
//!                          on an installed USB flash stick (if in USB host
//!                          mode - see the Setup group above).
//!   Help
//!            Help          Pressing "left" or "right" will show or hide the
//!                          screen showing oscilloscope connection help.
//!            Channel 1     Pressing "left" or "right" will cause the scale
//!                          and position for the channel 1 waveform to be set
//!                          such that the waveform is visible on the display.
//!            Channel 2     Pressing "left" or "right" will cause the scale
//!                          and position for the channel 2 waveform to be set
//!                          such that the waveform is visible on the display.
//! \endverbatim
//!
//! \addtogroup example_list
//! <b>Oscilloscope Connections</b>
//!
//! The 8 pins immediately above the color STN display panel offer connections
//! for both channels of the oscilloscope and also two test signals that can be
//! used to provide input in the absence of any other suitable signals.  Each
//! channel input must lie in the range -16.5V to +16.5V relative to the board
//! ground allowing differences of up to 33V to be measured.
//!
//! The connections are as follow where pin 1 is the leftmost pin, nearest the
//! microSD card socket:
//!
//! \verbatim
//!  1    Test 1      A test signal connected to one side of the speaker on
//!                   the board.
//!  2    Channel 1+  This is the positive connection for channel 1 of the
//!                   oscilloscope.
//!  3    Channel 1-  This is the negative connection for channel 1 of the
//!                   oscilloscope.
//!  4    Ground      This is connected to board ground.
//!  5    Test 2      A test signal connected to the board Status LED which
//!                   is driven from PWM0.  This signal is configured to
//!                   provide a 1KHz square wave.
//!  6    Channel 2+  This is the positive connection for channel 2 of the
//!                   oscilloscope.
//!  7    Channel 2-  This is the negative connection for channel 2 of the
//!                   oscilloscope.
//!  8    Ground      This is connected to board ground.
//! \endverbatim
//!
//! \addtogroup example_list
//! <b>Triggering and Sample Rate Notes</b>
//!
//! The oscilloscope can sample at a maximum combined rate of 1M samples
//! per second.  When both channels are enabled, therefore, the maximum
//! sample rate on each channel is 500K samples per second.  For maximum
//! resolution at the lowest timebases (maximum samples rates), disable
//! channel 2 if it is not required.  These sample rates give usable waveform
//! capture for signals up to around 100KHz.
//!
//! Trigger detection is performed in software during ADC interrupt handling.
//! At the highest sampling rates, this interrupt service routine consumes
//! almost all the available CPU cycles when searching for trigger conditions.
//! At these sample rates, if a trigger level is set which does not correspond
//! to a voltage that is ever seen in the trigger channel signal, the user
//! interface response can become sluggish.  To combat this, the oscilloscope
//! will abort any pending waveform capture operation if a key is pressed
//! before the capture cycle as completed.  This prevents the user interface
//! from being locked out and allows the trigger level or type to be changed
//! to values more appropriate for the signal being measured.
//!
//! \addtogroup example_list
//! <b>File Operations</b>
//!
//! Comma-separated-value or bitmap files representing the last waveform
//! captured may be saved to either a microSD card or a USB flash drive.
//! In each case, the files are written to the root directory of the microSD
//! card or flash drive with file names of the form ``scopeXXX.csv'' or
//! ``scopeXXX.bmp'' where ``XXX'' represents the lowest, three digit, decimal
//! number which offers a file name which does not already exist on the
//! device.
//!
//! \addtogroup example_list
//! <b>Companion Application</b>
//!
//! A companion application, LMScope, which runs on WindowsXP and Vista PCs
//! and the required device driver installer are available on the software CD
//! and via download from the TI Stellaris web site at
//! http://focus.ti.com/mcu/docs/mcuorphan.tsp?contentId=87903.  This
//! application offers full control of the oscilloscope from the PC and allows
//! waveform display and save to local hard disk.
//!
//! Note that the USB device drivers for the oscilloscope device must be
//! installed prior to running the LMScope application.  If the drivers are not
//! installed when LMScope is run, the application will report that various
//! required DLLs including lmusbdll.dll and winusb.dll are missing.  To install
//! the drivers, make sure that the USB mode is set to ``Device'' in the Setup
//! menu then connect the ek-lm3s3748 board to a PC via the ``USB Device''
//! connector.  This will cause Windows to prompt for device driver
//! installation.  The required driver can be found on the kit installation
//! CD or in the ``SW-USB-windrivers-xxxx'' package downloadable from the
//! software update web site.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/udma.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "utils/uartstdio.h"
#include "drivers/buttons.h"
#include "drivers/class-d.h"
#include "commands.h"
#include "data-acq.h"
#include "file.h"
#include "menu.h"
#include "menu-controls.h"
#include "qs-scope.h"
#include "renderer.h"
#include "usb_device.h"
#include "usb_host.h"
#include "usb_protocol.h"

//*****************************************************************************
//
// Handle for stdio-style debug output via UART0
//
//*****************************************************************************
int g_hStdio;

//*****************************************************************************
//
// Main loop command flags.
//
//*****************************************************************************
unsigned long g_ulCommand;

//*****************************************************************************
//
// System tick counter.
//
//*****************************************************************************
volatile unsigned long g_ulSysTickCounter;
unsigned long g_ulCaptureTick;

//*****************************************************************************
//
// Flag indicating whether or not the connection help screen is currently being
// displayed.
//
//*****************************************************************************
tBoolean g_bShowingHelp = false;

//*****************************************************************************
//
// Flag indicating whether we are capturing oscilloscope data continuously
// or operating in one-shot mode.
//
//*****************************************************************************
tBoolean g_bContinuousCapture = true;

//*****************************************************************************
//
// Flag indicating whether we are waiting for a capture request to complete.
//
//*****************************************************************************
tBoolean g_bCapturePending = false;

//*****************************************************************************
//
// Flag indicating whether we are operating as a USB device or host.
//
//*****************************************************************************
tBoolean g_bUSBModeIsHost = false;

//*****************************************************************************
//
// Flag indicating whether we are to send the next captured waveform data set
// back to the USB host.  This will be the case if the host requested capture
// but has not enabled automatic sending of data.
//
//*****************************************************************************
tBoolean g_bSendData = false;

//*****************************************************************************
//
// Oscilloscope capture buffers
//
//*****************************************************************************
unsigned short g_usScopeData[MAX_SAMPLES_PER_TRIGGER];

//*****************************************************************************
//
// Channels we will display
//
//*****************************************************************************
tBoolean g_pbActiveChannels[2] =
{
    true,
    true
};

//*****************************************************************************
//
// Prototypes for command handling functions.
//
//*****************************************************************************
tBoolean ChangeTimebase(unsigned long ulParam);
tBoolean Channel2Display(unsigned long ulParam);
tBoolean ProcessSysTick(unsigned long ulParam);
tBoolean InitiateCapture(unsigned long ulParam);
tBoolean StartCapture(unsigned long ulParam);
tBoolean StopCapture(unsigned long ulParam);
tBoolean ChangeTrigger(unsigned long ulParam);
tBoolean ChangeTriggerLevel(unsigned long ulParam);
tBoolean ChangeTriggerPos(unsigned long ulParam);
tBoolean ChangeChannel1Scale(unsigned long ulParam);
tBoolean ChangeChannel2Scale(unsigned long ulParam);
tBoolean ChangeChannel1Pos(unsigned long ulParam);
tBoolean ChangeChannel2Pos(unsigned long ulParam);
tBoolean SaveFile(unsigned long ulParam);
tBoolean RetransmitData(unsigned long ulParam);
tBoolean ChangeTriggerChannel(unsigned long ulParam);
tBoolean USBHostConnected(unsigned long ulParam);
tBoolean USBHostDisconnected(unsigned long ulParam);
tBoolean FindChannel(unsigned long ulParam);
tBoolean USBSetMode(unsigned long ulParam);
tBoolean ShowHelpScreen(unsigned long ulParam);

//*****************************************************************************
//
// Command handler function pointer type.
//
//*****************************************************************************
typedef tBoolean (*pfnCommandHandler)(unsigned long);

//*****************************************************************************
//
// Command handler control structure.
//
//*****************************************************************************
typedef struct
{
    //
    // If true, this command handler will be called while a waveform capture
    // is ongoing.  If false, it will be deferred until the capture completes.
    //
    tBoolean bSafeDuringCapture;

    //
    // If true, a debug trace message is output when the command is dispatched
    // to its handler.  If false, the trace message is suppressed.
    //
    tBoolean bTrace;

    //
    // Pointer to the handler for this command.
    //
    pfnCommandHandler pfnHandler;

    //
    // A description string that will be output in the trace message when
    // the command is dispatched.
    //
    const char *pcDesc;
}
tCommandHandler;

//*****************************************************************************
//
// Main loop command handling functions.  The index into this table must be
// equivalent to the bit position used to identify the command in g_ulCommands.
//
//*****************************************************************************
const tCommandHandler g_psCommandHandlers[NUM_SCOPE_COMMANDS] =
{
    { false, true,  ChangeTimebase,       "CHANGE_TIMEBASE" },
    { true,  false, ProcessSysTick,       "SYSTICK" },
    { true,  true,  ChangeTrigger,        "CHANGE_TRIGGER" },
    { false, true,  Channel2Display,      "CH2_DISPLAY" },
    { false, true,  InitiateCapture,      "CAPTURE" },
    { true,  true,  StopCapture,          "STOP" },
    { false, true,  StartCapture,         "START" },
    { true,  true,  ChangeTriggerLevel,   "TRIGGER_LEVEL" },
    { true,  true,  ChangeTriggerPos,     "TRIGGER_POS" },
    { true,  true,  ChangeChannel1Scale,  "CH1_SCALE" },
    { true,  true,  ChangeChannel2Scale,  "CH2_SCALE" },
    { true,  true,  ChangeChannel1Pos,    "CH1_POS" },
    { true,  true,  ChangeChannel2Pos,    "CH2_POS" },
    { false, true,  SaveFile,             "SAVE" },
    { false, true,  RetransmitData,       "RETRANSMIT" },
    { false, true,  ChangeTriggerChannel, "SET_TRIGGER_CH" },
    { true,  true,  USBHostConnected,     "USB_HOST_CONNECT" },
    { true,  true,  USBHostDisconnected,  "USB_HOST_REMOVE" },
    { false, true,  FindChannel,          "FIND" },
    { true,  true,  ShowHelpScreen,       "SHOW_HELP" },
    { false, true,  USBSetMode,           "USB_SET_MODE" }
};

//*****************************************************************************
//
// An array holding a single unsigned long parameter for each of the commands
// described in the g_psCommandHandlers array.
//
//*****************************************************************************
unsigned long g_ulCommandParam[NUM_SCOPE_COMMANDS];

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.  In this application uDMA is only used for USB,
// so only the first 6 channels are needed.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
tDMAControlTable g_sDMAControlTable[6];
#elif defined(ccs)
#pragma DATA_ALIGN(g_sDMAControlTable, 1024)
tDMAControlTable g_sDMAControlTable[6];
#else
tDMAControlTable g_sDMAControlTable[6] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The error routine that is called if any ASSERT fails.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
    UARTprintf("Assertion failed in %s:%d\n", pcFilename, ulLine);
    UARTprintf("Runtime error during test\n");
    while(1)
    {
    }
}
#endif

//*****************************************************************************
//
// Handle the system tick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update our tick counter
    //
    g_ulSysTickCounter++;

    //
    // Set a flag for the main loop to tell it that another tick occurred.
    //
    COMMAND_FLAG_WRITE(SCOPE_SYSTICK, 0);
}

//*****************************************************************************
//
// This function is called from the main loop each time the system tick is
// signaled.  It polls the current state of the buttons and performs any
// actions necessary as a result.
//
// Returns \b true to indicate that a display update is required or \b false
// otherwise.
//
//*****************************************************************************
tBoolean
ProcessSysTick(unsigned long ulParam)
{
    unsigned char ucButtons;
    unsigned char ucChanged;
    unsigned char ucRepeat;
    tBoolean bRetcode;

    //
    // Pass the 10mS tick on to the file system.
    //
    FileTickHandler();

    //
    // Also pass the tick on to the USB host stack if we are in host mode.
    //
    if(g_bUSBModeIsHost)
    {
        ScopeUSBHostTick();
    }

    //
    // Determine the state of the pushbuttons.
    //
    ucButtons = ButtonsPoll(&ucChanged, &ucRepeat);

    //
    // If any button changed state or if we see any autorepeat messages,
    // call the menu to process the key.
    //
    if(ucChanged || ucRepeat)
    {
        bRetcode = MenuProcess(ucButtons, ucChanged, ucRepeat);

        //
        // If the menu indicated that it was dismissed and we are
        // currently showing the help screen, send ourselves a message
        // telling us to refresh the display.
        //
        if(g_bShowingHelp && bRetcode)
        {
            RendererDrawHelpScreen(true);
        }

        return(bRetcode);
    }

    //
    // Update our capture tick counter.
    //
    g_ulCaptureTick++;

    //
    // We didn't have anything to process so no display update can be needed.
    //
    return(false);
}

//*****************************************************************************
//
// Enable PWM0 to provide a 1KHz square wave on the TEST2 output (which also
// happens to be connected to a LED on the board).
//
//*****************************************************************************
void
TestSignalInit(void)
{
    //
    // Enable the PWM peripheral.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    //
    // Enable the PWM0 output pin.
    //
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0);

    //
    // Set the PWM up for 1KHz, 50% duty cycle output
    //
    ROM_PWMGenConfigure(PWM0_BASE, PWM_GEN_0, (PWM_GEN_MODE_DOWN |
                        PWM_GEN_MODE_DBG_RUN | PWM_GEN_MODE_NO_SYNC));
    ROM_PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, (ROM_SysCtlClockGet() / 1000));
    ROM_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, (ROM_SysCtlClockGet() / 2000));

    //
    // Start the output running.
    //
    ROM_PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    ROM_PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);
}

//*****************************************************************************
//
// Once a capture has completed, retrieve information on the samples collected
// and render them to the display.
//
//*****************************************************************************
void
UpdateWaveform(tBoolean bMenuShown, tBoolean bHelpShown, tBoolean bNewData)
{
    tBoolean bOverflow;
    tBoolean bUnderflow;
    tBoolean bRetcode;
    tDataAcqCaptureStatus sStatus;
    unsigned short usTrigLevel;
    unsigned long ulTrigPos;
    tTriggerType eTrigType;

    //
    // Get more thorough information on the capture status.
    //
    bRetcode = DataAcquisitionGetStatus(&sStatus);
    if(!bRetcode)
    {
        UARTprintf("Error reading status!\n");
        return;
    }

    //
    // If the capture was aborted, don't render the new data, merely redisplay
    // what we currently have.
    //
    if(sStatus.eState != DATA_ACQ_ERROR)
    {
        //
        // Check to see if any errors were reported.  If so, the data is likely
        // to be corrupt so don't render it.
        //
        bRetcode = DataAcquisitionDidErrorOccur(true, &bOverflow,
                                                &bUnderflow);

        if(!bRetcode)
        {
            //
            // Get the current trigger information.
            //
            DataAcquisitionGetTrigger(&eTrigType, &ulTrigPos, &usTrigLevel);

            //
            // Display the waveform or waveforms contained in the data set.
            //
            RendererDrawWaveform(&sStatus, &g_sRender, g_pbActiveChannels,
                                 &g_sMeasure);

            //
            // Send the newly captured data back to the USB host if it
            // is connected and has requested it.
            //
            if(bNewData || g_bSendData)
            {
                //
                // Make sure we are currently operating as a USB device.
                //
                if(!g_bUSBModeIsHost)
                {
                    //
                    // We are a device so send the data.
                    //
                    ScopeUSBDeviceSendData(&sStatus, !g_bSendData);
                }

                //
                // Data has been sent (or discarded if we are a USB host) so
                // clear the flag telling us to send it.
                //
                g_bSendData = false;
            }
        }
    }

    //
    // Flush all drawing we have done to the display if we are not currently
    // displaying the menu.
    //
    if(!bMenuShown)
    {
        //
        // If the help screen is not visible, update the waveform normally.
        //
        if(!bHelpShown)
        {
            //
            // Update the waveform display as normal.
            //
            RendererUpdate();
        }
        else
        {
            //
            // The help screen is visible so we only update the alert box,
            // taking care to refresh the help screen when the alert exits.
            //
            RendererUpdateAlert();
        }
    }
}

//*****************************************************************************
//
// Sets default values for various oscilloscope parameters.
//
// This function is called during application initialization to set default
// values for several oscilloscope parameters.
//
// \return None.
//
//*****************************************************************************
void
SetDefaultParameters(void)
{
    g_ulCommandParam[SCOPE_CHANGE_TIMEBASE] = DEFAULT_TIMEBASE_US;
    g_ulCommandParam[SCOPE_CHANGE_TRIGGER] = (unsigned long)TRIGGER_ALWAYS;
    g_ulCommandParam[SCOPE_CH2_DISPLAY] = g_pbActiveChannels[CHANNEL_2];
    g_ulCommandParam[SCOPE_TRIGGER_LEVEL] = DEFAULT_TRIGGER_LEVEL_MV;
    g_ulCommandParam[SCOPE_CH1_SCALE] = DEFAULT_SCALE_MV;
    g_ulCommandParam[SCOPE_CH2_SCALE] = DEFAULT_SCALE_MV;
    g_ulCommandParam[SCOPE_SET_TRIGGER_CH] = CHANNEL_1;
    g_ulCommandParam[SCOPE_SET_USB_MODE] = (unsigned long)g_bUSBModeIsHost;
    g_ulCommandParam[SCOPE_SHOW_HELP] = (unsigned long)g_bShowingHelp;
}

//*****************************************************************************
//
// Handles all commands sent to the main loop.
//
// \param bCapturePending indicates whether or not we are waiting for a
// capture to complete.  Some commands may need to defer operation until the
// pending capture has completed and this flag allows this.
//
// This function is called from the main loop to process any commands sent to
// it asynchronously or which require the previous capture request to have
// been completed before they can be processed.  It checks for bits set in the
// global command flags variable g_ulCommand and dispatches each command
// to the appropriate handler.
//
// \return Returns \b true if any command indicated that a display update
// is required or \b false if no display update is needed.
//
//*****************************************************************************
tBoolean
ProcessCommands(tBoolean bCapturePending)
{
    unsigned long ulLoop;
    tBoolean bRetcode;
    tBoolean bRedrawNeeded;
    tBoolean bCommandHandled;

    //
    // We assume no redraw is required unless we are later told otherwise.
    //
    bRedrawNeeded = false;

    //
    // Remember that we have not handled any commands yet.
    //
    bCommandHandled = false;

    //
    // Check to see if the user has entered any commands via UART and,
    // if so, process the next one.
    //
    CommandReadAndProcess();

    //
    // Loop through each of the command bits in the global command flags
    // variable looking for any that are set.
    //
    for(ulLoop = 0; ulLoop < NUM_SCOPE_COMMANDS; ulLoop++)
    {
        //
        // If a bit is set, we have something to do.
        //
        if(g_ulCommand & SCOPE_CMD_TO_FLAG(ulLoop))
        {
            //
            // Can this command be processed in the current state?
            //
            if(g_psCommandHandlers[ulLoop].bSafeDuringCapture ||
               !bCapturePending)
            {
                //
                // First we clear the bit since we are about to process it.
                //
                COMMAND_FLAG_CLEAR(ulLoop);

                //
                // Does this command have a handler installed?
                //
                if(g_psCommandHandlers[ulLoop].pfnHandler)
                {
                    //
                    // Remember that we did something if it is something
                    // other than handling the system tick.
                    //
                    if(ulLoop != SCOPE_SYSTICK)
                    {
                        bCommandHandled = true;
                    }

                    //
                    // Dump a trace message to the UART unless this is
                    // a high frequency command like SYSTICK.
                    //
                    if(g_psCommandHandlers[ulLoop].bTrace)
                    {
                        UARTprintf("Processing %s %d\n",
                                   g_psCommandHandlers[ulLoop].pcDesc,
                                   g_ulCommandParam[ulLoop]);
                    }

                    //
                    // Call the handler and pass the parameter supplied by
                    // the code that set the command flag in the first place.
                    //
                    bRetcode = (g_psCommandHandlers[ulLoop].pfnHandler)(
                                    g_ulCommandParam[ulLoop]);
                    if(bRetcode)
                    {
                        bRedrawNeeded = true;
                    }
                }
            }
        }
    }

    //
    // If we did anything, call the menu and have it refresh the current
    // display just in case the command came in from UART or USB and it
    // altered the control which currently has the focus.
    //
    if(bCommandHandled)
    {
        MenuRefresh();
    }

    return(bRedrawNeeded);
}

//*****************************************************************************
//
// Performs all capture and display configuration changes required as a result
// of a user request to change the timebase.
//
// \param ulTimebase is the requested timebase expressed in microseconds per
// division.
//
// This function is called from the main command processor to perform
// capture rate updates and one-off display calculations as a result of a
// change in the requested display timebase.
//
// The function calculates the best capture rate to use for the requested
// timebase and also determines the new display horizontal offset to put
// the trigger point at the center of the screen.
//
// This function is also called when the user decides to enable or disable
// the second channel since, when only 1 channel is running, we are able to
// chose the highest sample rate and, hence, provide higher resolution at
// small timebases.
//
// \return Returns \b true if a display redraw is required, \b false
// otherwise.
//
//*****************************************************************************
tBoolean
ChangeTimebase(unsigned long ulTimebase)
{
    unsigned long ulCaptureTime;
    unsigned long ulRate;

    //
    // Determine the best sample rate to use to support the requested
    // timebase.  The sample rate is chosen to ensure that we capture at least
    // twice as many samples as will fit across the display while still
    // staying within our MAX_SAMPLES_PER_TRIGGER buffer size.
    //

    //
    // With the new timebase, how much time do we have to capture the data?
    // This calculation returns ulCaptureTime in microseconds.
    //
    ulCaptureTime = ((2 * WAVEFORM_WIDTH * ulTimebase) /
                     GRATICULE_SIDE);

    //
    // We want to pick the highest capture rate we can use which provides us
    // with no more than MAX_SAMPLES_PER_TRIGGER samples within the capture
    // time just calculated.  We must remember that we may be capturing two
    // samples per sample period if both channels are enabled.
    //
    // This calculation yields the exact rate (in Hz) required to fill the
    // buffer with samples in the time calculated above.
    //
    ulRate = (MAX_SAMPLES_PER_TRIGGER * 1000000) / ulCaptureTime;

    //
    // If we are using both channels, we need to half the requested rate
    // to compensate for the fact that there are 2 samples gathered per
    // period.  This will allow us to fill the capture buffer in the same
    // time.
    //
    if(g_pbActiveChannels[CHANNEL_2])
    {
        ulRate /= 2;
    }

    //
    // Now that we have the rate that we would like to use, we need to convert
    // it to something that is supported by the data acquisition module.  If we
    // don't find an exact match, we pick the next lower supported rate.
    //
    ulRate = DataAcquisitionGetClosestRate(ulRate,
                                           g_pbActiveChannels[CHANNEL_2]);

    UARTprintf("Setting rate %dHz for timebase %duS/div\n", ulRate,
               ulTimebase);

    //
    // Set the new capture rate.
    //
    g_sRender.uluSPerDivision = ulTimebase;
    DataAcquisitionSetRate(ulRate, g_pbActiveChannels[CHANNEL_2]);

    //
    // Inform the USB host (if connected) that the timebase has changed.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_TIMEBASE_UPDATED, 0, ulTimebase);

    //
    // Tell the caller to update the current display
    //
    return(true);
}

//*****************************************************************************
//
// Enables or disables capture and display of oscilloscope channel 2.
//
// This function is called from the main command processor when the user
// wishes to change the state of oscilloscope channel 2.  It sets the global
// variable which determines whether the second channel is displayed or not
// and recalculates the sample capture rate since this may need to be changed
// if a low timebase is in use.
//
// \return Returns \b true if a display update is required, \b false if not.
//
//*****************************************************************************
tBoolean
Channel2Display(unsigned long ulParam)
{
    tBoolean bTriggerCh1;
    tTriggerType eType;
    unsigned long ulTrigPos;
    unsigned short usLevel;

    //
    // Which channel are we currently triggering on and what trigger
    // parameters are currently in use?
    //
    bTriggerCh1 = DataAcquisitionGetTriggerChannel();
    DataAcquisitionGetTrigger(&eType, &ulTrigPos, &usLevel);

    //
    // If we are disabling channel 2 while it is set to trigger, we need to
    // change the trigger mode to TRIGGER_ALWAYS and revert to triggering on
    // channel 1.
    //
    if((ulParam == false) && !bTriggerCh1)
    {
        ChangeTrigger(TRIGGER_ALWAYS);
        ChangeTriggerChannel(CHANNEL_1);
        RendererSetAlert("Trigger\ndisabled.", 200);
    }

    //
    // Set the variable that tells us whether to display both channels or
    // only one.
    //
    g_pbActiveChannels[CHANNEL_2] = ulParam ? true : false;

    //
    // Inform the USB host (if connected) that channel 2 has been enabled
    // or disabled.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_CHANNEL2, (ulParam ?
                             SCOPE_CHANNEL2_ENABLE : SCOPE_CHANNEL2_DISABLE),
                             0);

    //
    // Turning channel 2 on or off has an effect on the capture rates we can
    // support so recalculate based on the new setting.
    //
    return(ChangeTimebase(g_sRender.uluSPerDivision));
}

//*****************************************************************************
//
// Requests capture of a single set of samples from the data acquisition
// module.
//
// This function is called from the main command processor when the user
// requests to capture a single set of samples.
//
// \return Returns \b true if a display update is required, \b false if not.
//
//*****************************************************************************
tBoolean
InitiateCapture(unsigned long ulParam)
{
    tTriggerType eType;
    unsigned long ulTrigPos;
    unsigned short usTrigLevel;

    //
    // Get the current trigger type.  We will have changed this to
    // TRIGGER_ALWAYS during StopCapture() so we need to revert to the
    // original trigger when we restart.
    //
    DataAcquisitionGetTrigger(&eType, &ulTrigPos, &usTrigLevel);
    DataAcquisitionSetTrigger(
                          (tTriggerType)g_ulCommandParam[SCOPE_CHANGE_TRIGGER],
                          ulTrigPos, usTrigLevel);

    //
    // Display a message telling the user we are waiting for a trigger.
    //
    RendererSetAlert("Waiting for trigger", 0);

    //
    // Request another triggered capture from the data acquisition module.
    //
    DataAcquisitionRequestCapture();
    g_bCapturePending = true;
    g_bSendData = true;

    return(false);
}

//*****************************************************************************
//
// Stops continuous capture.
//
// This function is called from the main command processor when the user
// requests to stop continuous capture of oscilloscope data.  Stopping is
// achieved by forcing the data acquisition module to trigger immediately.
// This results in us receiving one complete set of valid samples regardless
// of whether or not a trigger was actually requested.  If we were to stop
// merely by canceling the outstanding acquisition request, we would end up
// with a partial data set which would not look good on the display.
//
// \return Returns \b true if a display update is required, \b false if not.
//
//*****************************************************************************
tBoolean
StopCapture(unsigned long ulParam)
{
    tTriggerType eType;
    unsigned long ulTrigPos;
    unsigned short usTrigLevel;

    //
    // We stop continuous capture by setting the trigger mode to
    // TRIGGER_ALWAYS.  This ensures that we end up with a complete set of
    // samples and allows the display to be redrawn cleanly.
    //
    DataAcquisitionGetTrigger(&eType, &ulTrigPos, &usTrigLevel);
    DataAcquisitionSetTrigger(TRIGGER_ALWAYS, ulTrigPos, usTrigLevel);

    //
    // Tell the main loop not to continue capture.
    //
    g_bContinuousCapture = false;

    //
    // Inform the USB host (if connected) that automatic capture has stopped.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_STOPPED, 0, 0);

    //
    // No display updated is needed.
    //
    return(false);
}

//*****************************************************************************
//
// Starts continuous capture.
//
// This function is called from the main command processor when the user
// requests to start continuous capture of oscilloscope data.  We set the
// trigger mode correctly and set the global variable \e g_bContinuousCapture
// to tell the main loop to start capturing data again.
//
// \return Returns \b true if a display update is required, \b false if not.
//
//*****************************************************************************
tBoolean
StartCapture(unsigned long ulParam)
{
    tTriggerType eType;
    unsigned long ulTrigPos;
    unsigned short usTrigLevel;

    //
    // Get the current trigger type.  We will have changed this to
    // TRIGGER_ALWAYS during StopCapture() so we need to revert to the
    // original trigger when we restart.
    //
    DataAcquisitionGetTrigger(&eType, &ulTrigPos, &usTrigLevel);
    DataAcquisitionSetTrigger(
                        (tTriggerType)g_ulCommandParam[SCOPE_CHANGE_TRIGGER],
                        ulTrigPos, usTrigLevel);

    //
    // Tell the main loop that we want to perform continuous capture once
    // again.
    //
    g_bContinuousCapture = true;

    //
    // Inform the USB host (if connected) that automatic capture has stopped.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_STARTED, 0, 0);

    //
    // No display update is needed.
    //
    return(false);
}

//*****************************************************************************
//
// Changes the current trigger type.
//
// \param ulTrigger is the requested trigger type.  This value is a member
// of the enumerated type \e tTriggerType cast to an unsigned long purely
// since this is a standard command processor function.
//
// This function is called from the main command processor when the user
// wishes to change the trigger type.
//
// \return Returns \b true if a display update is required, \b false if not.
//
//*****************************************************************************
tBoolean
ChangeTrigger(unsigned long ulTrigger)
{
    tBoolean bRetcode;
    tTriggerType eType;
    unsigned long ulTrigPos;
    unsigned short usLevel;
    unsigned long ulTrigType;
    tBoolean bTriggerCh1;

    //
    // Get the existing trigger parameters.
    //
    DataAcquisitionGetTrigger(&eType, &ulTrigPos, &usLevel);
    bTriggerCh1 = DataAcquisitionGetTriggerChannel();

    //
    // Update with the new trigger type.
    //
    bRetcode = DataAcquisitionSetTrigger((tTriggerType)ulTrigger, ulTrigPos,
                                         usLevel);
    if(!bRetcode)
    {
        UARTprintf("Error setting trigger type %d\n", ulTrigger);
    }
    else
    {
        UARTprintf("Set trigger type %d\n", ulTrigger);
    }

    //
    // Map from the internal trigger type enum to the values used in the
    // USB protocol.
    //
    switch(ulTrigger)
    {
        case TRIGGER_RISING:
        {
            ulTrigType = SCOPE_TRIGGER_TYPE_RISING;
            break;
        }

        case TRIGGER_FALLING:
        {
            ulTrigType = SCOPE_TRIGGER_TYPE_FALLING;
            break;
        }

        case TRIGGER_LEVEL:
        {
            ulTrigType = SCOPE_TRIGGER_TYPE_LEVEL;
            break;
        }

        case TRIGGER_ALWAYS:
        default:
        {
            ulTrigType = SCOPE_TRIGGER_TYPE_ALWAYS;
            break;
        }
    }

    //
    // Inform the USB host (if connected) that the trigger type has changed.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_TRIGGER_TYPE, (bTriggerCh1 ?
                             SCOPE_CHANNEL_1 : SCOPE_CHANNEL_2), ulTrigType);

    //
    // No display update is needed
    //
    return(false);
}

//*****************************************************************************
//
// Changes the current trigger level.
//
// \param ulLevel is the requested trigger level expressed as a signed number
// of millivolts.
//
// This function is called from the main command processor when the user
// wishes to change the trigger level.
//
// \return Returns \b true if a display update is required, \b false if not.
//
//*****************************************************************************
tBoolean
ChangeTriggerLevel(unsigned long ulLevel)
{
    tBoolean bRetcode;
    tTriggerType eType;
    unsigned long ulTrigPos;
    unsigned short usLevel;

    //
    // Get the existing trigger parameters.
    //
    DataAcquisitionGetTrigger(&eType, &ulTrigPos, &usLevel);

    //
    // Update with the new trigger type.
    //
    bRetcode = DataAcquisitionSetTrigger(eType, ulTrigPos,
                             (unsigned short)MV_TO_ADC_SAMPLE((long)ulLevel));
    if(!bRetcode)
    {
        UARTprintf("Error setting trigger level %dmV\n", ulLevel);
    }

    //
    // Update the rendering parameters with the new trigger level.
    //
    g_sRender.lTriggerLevelmV = (long)ulLevel;

    //
    // Inform the USB host (if connected) that the trigger level has
    // changed.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_TRIGGER_LEVEL, 0, ulLevel);

    //
    // A display update is needed.
    //
    return(true);
}

//*****************************************************************************
//
// Changes the current trigger position on the display.
//
// \param ulPos is the requested trigger position expressed in pixels from
// the center of the waveform display area.  This is a signed value cast to
// an unsigned long to conform to the standard command processor function
// prototype.
//
// This function is called from the main command processor when the user
// wishes to change the trigger position.  Note that we don't actually change
// the trigger position as far as the data acquisition is concerned, we
// merely move the display left or right to show the data before or after
// the existing trigger (which is in the center of the data set).
//
// \return Returns \b true if a display update is required, \b false if not.
//
//*****************************************************************************
tBoolean
ChangeTriggerPos(unsigned long ulPos)
{
    long lPos;

    //
    // Set the new offset if we were passed a valid number.
    //
    lPos = (long)ulPos;
    if((lPos > (WAVEFORM_WIDTH / 2)) || (lPos < -(WAVEFORM_WIDTH / 2)))
    {
        UARTprintf("Invalid trigger position %d.\n", lPos);
    }
    else
    {
        //
        // Update the global offset.  This will take effect on the next redraw.
        //
        g_sRender.lHorizontalOffset = lPos;
    }

    //
    // Pass the new position back to the USB host, if connected.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_TRIGGER_POS, 0, lPos);

    //
    // A display update is needed.
    //
    return(true);
}

//*****************************************************************************
//
// Changes the vertical scaling (mV/division) for channel 1.
//
// \param ulScale is the desired vertical scaling for channel 1 expressed
// in millivolts per division.
//
// This function is called from the main command processor when the user
// wishes to change the scaling for channel 1.
//
// \return Returns \b true if a display update is required, \b false if not.
//
//*****************************************************************************
tBoolean
ChangeChannel1Scale(unsigned long ulScale)
{
    //
    // Update the global scaling information.
    //
    g_sRender.ulmVPerDivision[CHANNEL_1] = ulScale;

    //
    // Pass the new scale back to the USB host, if connected.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_SCALE, SCOPE_CHANNEL_1, ulScale);

    //
    // A display update is needed.
    //
    return(true);
}

//*****************************************************************************
//
// Changes the vertical scaling (mV/division) for channel 2.
//
// \param ulScale is the desired vertical scaling for channel 2 expressed
// in millivolts per division.
//
// This function is called from the main command processor when the user
// wishes to change the scaling for channel 2.
//
// \return Returns \b true if a display update is required, \b false if not.
//
//*****************************************************************************
tBoolean
ChangeChannel2Scale(unsigned long ulScale)
{
    //
    // Update the global scaling information.
    //
    g_sRender.ulmVPerDivision[CHANNEL_2] = ulScale;

    //
    // Pass the new scale back to the USB host, if connected.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_SCALE, SCOPE_CHANNEL_2, ulScale);

    //
    // A display update is needed.
    //
    return(true);
}

//*****************************************************************************
//
// Changes the vertical offset (mV) for channel 1.
//
// \param ulScale is the desired vertical scaling for channel 1 expressed
// in millivolts.
//
// This function is called from the main command processor when the user
// wishes to change the scaling for channel 1.
//
// \return Returns \b true if a display update is required, \b false if not.
//
//*****************************************************************************
tBoolean
ChangeChannel1Pos(unsigned long ulPos)
{
    //
    // Update the global scaling information.
    //
    g_sRender.lVerticalOffsetmV[CHANNEL_1] = (long)ulPos;

    //
    // Pass the new scale back to the USB host, if connected.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_POSITION, SCOPE_CHANNEL_1, ulPos);

    //
    // A display update is needed.
    //
    return(true);
}

//*****************************************************************************
//
// Changes the vertical offset (mV) for channel 2.
//
// \param ulScale is the desired vertical scaling for channel 2 expressed
// in millivolts per division.
//
// This function is called from the main command processor when the user
// wishes to change the scaling for channel 2.
//
// \return Returns \b true if a display update is required, \b false if not.
//
//*****************************************************************************
tBoolean
ChangeChannel2Pos(unsigned long ulPos)
{
    //
    // Update the global scaling information.
    //
    g_sRender.lVerticalOffsetmV[CHANNEL_2] = (long)ulPos;

    //
    // Pass the new scale back to the USB host, if connected.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_POSITION, SCOPE_CHANNEL_2, ulPos);

    //
    // A display update is needed.
    //
    return(true);
}

//*****************************************************************************
//
// Saves the latest captured data as a bitmap or CSV file.
//
// \param ulType indicates the file format to save.
//
// This function is called from the main command processor when the user
// wishes to save a file containing the current waveform data.  Files are
// saved on an installed microSD card.  If \e ulType is SCOPE_SAVE_BMP, a
// Windows bitmap file of the current waveform display is saved.  If
// \e ulType is SCOPE_SAVE_CSV then a comma separated value file suitable
// for use with spreadsheets is saved.
//
// \return Returns \b false to indicate that no display update is needed.
//
//*****************************************************************************
tBoolean
SaveFile(unsigned long ulType)
{
    tBoolean bRetcode;
    tDataAcqCaptureStatus sStatus;

    //
    // Determine the status of the last capture.
    //
    bRetcode = DataAcquisitionGetStatus(&sStatus);
    if(!bRetcode)
    {
        UARTprintf("Error reading status!\n");
        return(false);
    }

    //
    // If there was an error while the data was being captured or if there
    // is not data actually captured yet, defer the file save operation until
    // there is data available.
    //
    if(sStatus.eState != DATA_ACQ_COMPLETE)
    {
        //
        // Reschedule the save command until later.
        //
        COMMAND_FLAG_WRITE(SCOPE_SAVE, ulType);
        return(false);
    }

    //
    // At this point, we have good data so go ahead and save it.  First
    // determine from the parameter which format is being requested.
    //
    if((ulType & SCOPE_SAVE_FORMAT_MASK) == SCOPE_SAVE_BMP)
    {
        //
        // Save a bitmap.  Extract the destination from the parameter and
        // call the File module to save the information to the relevant disk.
        //
        FileWriteBitmap(&sStatus,
                        ((ulType & SCOPE_SAVE_DRIVE_MASK) == SCOPE_SAVE_SD));
    }
    else
    {
        //
        // Save a CSV file.  Extract the destination from the parameter and
        // call the File module to save the information to the relevant disk.
        //
        FileWriteCSV(&sStatus,
                     ((ulType & SCOPE_SAVE_DRIVE_MASK) == SCOPE_SAVE_SD));
    }

    //
    // No redraw necessary.
    //
    return(false);
}

//*****************************************************************************
//
// Retransmits the latest captured waveform data to the USB host.
//
// \param ulParam is ignored.
//
// This function is called from the main command processor in response to
// an incoming USB request to retransmit the last captured waveform data
// set.
//
// \return Returns \b false to indicate that no display update is needed.
//
//*****************************************************************************
tBoolean
RetransmitData(unsigned long ulParam)
{
    tDataAcqCaptureStatus sCapInfo;
    tBoolean bRetcode;

    //
    // Get the current capture information from the data acquisition
    // module.
    //
    bRetcode = DataAcquisitionGetStatus(&sCapInfo);

    //
    // Ask the USB layer to retransmit the current data set assuming it
    // is complete and does not indicate any errors.
    //
    if(bRetcode && (sCapInfo.eState == DATA_ACQ_COMPLETE))
    {
        ScopeUSBDeviceSendData(&sCapInfo, false);
    }
    else
    {
        //
        // If we couldn't retransmit the current data, set our command
        // flag so that we try again next capture.
        //
        COMMAND_FLAG_WRITE(SCOPE_RETRANSMIT, 0);
    }

    //
    //
    // Indicate to the caller that no display update is required.
    //
    return(false);
}

//*****************************************************************************
//
// Sets the channel that is to be used for capture triggering.
//
// \param ulChannel is either CHANNEL_1 or CHANNEL_2.
//
// This function is called from the main command processor to set the
// trigger channel for the oscilloscope.
//
// \return Returns \b false to indicate that no display update is needed.
//
//*****************************************************************************
tBoolean
ChangeTriggerChannel(unsigned long ulChannel)
{
    tTriggerType eType;
    unsigned long ulTrigPos;
    unsigned short usLevel;
    unsigned long ulTrigType;

    //
    // We can only set the trigger on a channel which is currently
    // active so check for this.
    //
    if(!g_pbActiveChannels[ulChannel])
    {
        //
        // Update the menu in case this command came from there.
        //
        g_ulCommandParam[SCOPE_SET_TRIGGER_CH] =
                (ulChannel == CHANNEL_2) ? CHANNEL_1 : CHANNEL_2;
        MenuRefresh();
    }
    else
    {
        //
        // Tell the data acquisition module which channel to trigger on.
        //
        DataAcquisitionSetTriggerChannel((ulChannel == CHANNEL_1) ?
                                          true : false);
    }

    //
    // Get the existing trigger parameters.
    //
    DataAcquisitionGetTrigger(&eType, &ulTrigPos, &usLevel);

    //
    // Map from the internal trigger type enum to the values used in the
    // USB protocol.
    //
    switch(eType)
    {
        case TRIGGER_RISING:
        {
            ulTrigType = SCOPE_TRIGGER_TYPE_RISING;
            break;
        }

        case TRIGGER_FALLING:
        {
            ulTrigType = SCOPE_TRIGGER_TYPE_FALLING;
            break;
        }

        case TRIGGER_LEVEL:
        {
            ulTrigType = SCOPE_TRIGGER_TYPE_LEVEL;
            break;
        }

        case TRIGGER_ALWAYS:
        default:
        {
            ulTrigType = SCOPE_TRIGGER_TYPE_ALWAYS;
            break;
        }
    }

    //
    // Inform the USB host (if connected) that the trigger type has changed.
    //
    ScopeUSBDeviceSendPacket(SCOPE_PKT_TRIGGER_TYPE, (ulChannel == CHANNEL_1 ?
                             SCOPE_CHANNEL_1 : SCOPE_CHANNEL_2), ulTrigType);

    //
    // Indicate to the caller that no display update is required.
    //
    return(false);
}

//*****************************************************************************
//
// Displays an alert message to the user indicating that the USB host
// has connected.
//
// \param ulParam - ignored.
//
// This function is called from the main command processor when the USB
// host connects.
//
// \return Returns \b false to indicate that no display update is needed.
//
//*****************************************************************************
tBoolean
USBHostConnected(unsigned long ulParam)
{
    RendererSetAlert("USB host\nconnected.", 200);
    return(false);
}

//*****************************************************************************
//
// Displays an alert message to the user indicating that the USB host has
// been disconnected.
//
// \param ulParam - ignored.
//
// This function is called from the main command processor when the USB
// host disconnects.
//
// \return Returns \b false to indicate that no display update is needed.
//
//*****************************************************************************
tBoolean
USBHostDisconnected(unsigned long ulParam)
{
    RendererSetAlert("USB host\ndisconnected.", 200);
    return(false);
}

//*****************************************************************************
//
// Automatically adjusts the vertical offset and scale for the given channel
// to ensure that its waveform is visible on the display.
//
// \param ulChannel is either CHANNEL_1 or CHANNEL_2.
//
// This function is called from the main command processor when the user
// requests a "Find" operation either via the user interface or USB.
//
// \return Returns \b false to indicate that no display update is needed
// (since this function merely posts two other commands to set the scale and
// position for the channel).
//
//*****************************************************************************
tBoolean
FindChannel(unsigned long ulChannel)
{
    long lPos;
    unsigned long ulAmplitudemV;
    unsigned long ulScalemV;

    //
    // Pull the average value of the last captured waveform to the
    // center of the display be adjusting the vertical offset.  Round
    // to the nearest 100mV to keep the offset a multiple of 100.
    //
    lPos = -g_sMeasure.sInfo[ulChannel].lMeanmV;
    lPos = (lPos / 100) * 100;

    //
    // Set the vertical scaling so that the signal fits into the
    // display.  Only particular scale factors are allowed so we need
    // to find the next higher supported value after calculating our
    // desired scale.
    //
    ulAmplitudemV = (g_sMeasure.sInfo[ulChannel].lMaxmV -
                     g_sMeasure.sInfo[ulChannel].lMinmV);

    //
    // Scale the waveform so that, peak to peak, it is half the height
    // of the waveform area
    //
    ulScalemV = (ulAmplitudemV /
                 (WAVEFORM_HEIGHT / (GRATICULE_SIDE * 2)));

    //
    // Map the calculated scale to the closest supported scaling
    // value.
    //
    ulScalemV = ClosestSupportedScaleFactor(ulScalemV);

    //
    // Tell the main loop to update the vertical scaling and
    // offset to bring the waveform into view.  We don't call for an
    // immediate redraw since this will be done when the command flag
    // is processed in the main loop.
    //
    COMMAND_FLAG_WRITE(((ulChannel == CHANNEL_1) ? SCOPE_CH1_SCALE :
                        SCOPE_CH2_SCALE), ulScalemV);
    COMMAND_FLAG_WRITE(((ulChannel == CHANNEL_1) ? SCOPE_CH1_POS :
                        SCOPE_CH2_POS), (unsigned long)lPos);
    return(false);
}

//*****************************************************************************
//
// Sets the application to act as either a USB device or USB MSC host.
//
// \param ulMode is either 0 (for device mode) or 1 (for host mode).
//
// This function is called from the main command processor when the user
// changes the USB mode from the menu.  If the mode has changed, it will
// shut down USB and reconfigure for the new mode.
//
// \return Returns \b false to indicate that no display update is needed.
//
//*****************************************************************************
tBoolean
USBSetMode(unsigned long ulParam)
{
    tBoolean bRetcode;

    //
    // Are we changing from host to device?
    //
    if(g_bUSBModeIsHost && !ulParam)
    {
        //
        // Shut down USB host operation.
        //
        ScopeUSBHostTerm();

        //
        // Start up USB host operation.
        //
        bRetcode = ScopeUSBDeviceInit();

        //
        // Did things go as planned?
        //
        if(!bRetcode)
        {
            //
            // Dump an error to the UART if we failed to initialize.
            //
            UARTprintf("Unable to configure as a USB device!\n");
        }

        g_bUSBModeIsHost = false;
    }
    //
    // Are we changing from device to host?
    //
    else if(!g_bUSBModeIsHost && ulParam)
    {
        //
        // Shut down USB device operation.
        //
        ScopeUSBDeviceTerm();

        //
        // Start up USB host operation.
        //
        bRetcode = ScopeUSBHostInit();

        //
        // Did things go as planned?
        //
        if(!bRetcode)
        {
            //
            // Dump an error to the UART if we failed to initialize.
            //
            UARTprintf("Unable to configure as a USB host!\n");
        }

        //
        // Remember the mode we are in.
        //
        g_bUSBModeIsHost = true;
    }

    //
    // No display update is required as a result of this command.
    //
    return(false);
}

//*****************************************************************************
//
// Shows or hides the help screen.
//
// \param ulShow is 0 to hide the help screen or any other value to show
// it.
//
// This function is called from the main command processor when the user
// requests the help screen from the menu or when the help screen is
// dismissed.
//
// \return Returns \b false to indicate that no further screen redraw is
// required
//
//*****************************************************************************
tBoolean
ShowHelpScreen(unsigned long ulShow)
{
    //
    // If we are currently showing help and are being asked to hide it,
    // refresh the display.
    //
    g_bShowingHelp = (ulShow == 0) ? false : true;

    //
    // Show or hide the help information as required.
    //
    RendererDrawHelpScreen(g_bShowingHelp);

    //
    // No further screen redraw is needed.
    //
    return(false);
}

//*****************************************************************************
//
// Main entry function for the quickstart oscilloscope application.
//
// This is the main entry function for the quickstart oscilloscope
// application and holds system initialization code and the main application
// loop.
//
// \return None.
//
//*****************************************************************************
int
main(void)
{
    tBoolean bRetcode;

    //
    // Set the system clock to run at 50MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_8MHZ);

    //
    // Set the system tick to fire 100 times per second.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICKS_PER_SECOND);
    ROM_SysTickIntEnable();
    ROM_SysTickEnable();

    //
    // Configure the relevant pins such that UART0 owns them.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Open UART0 for debug output.
    //
    UARTStdioInit(0);

    //
    // Enable the uDMA controller and set up the control table base.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    uDMAEnable();
    uDMAControlBaseSet(g_sDMAControlTable);

    //
    // Initialize the Class-D amplifier driver.
    //
    ClassDInit(SysCtlClockGet());

    //
    // Set default values for various parameters.
    //
    SetDefaultParameters();

    //
    // Initialize the SD card and file system.
    //
    bRetcode = FileInit();
    ERROR_CHECK(bRetcode, "ERROR! Can't initialize file system!\n");

    //
    // Initialize the pushbuttons.
    //
    ButtonsInit();

    //
    // Initialize the display.
    //
    RendererInit();

    //
    // Show the startup animation
    //
    RendererShowStartupScreen();

    //
    // Wait until the Class-D amplifier driver is done starting up.
    //
    while(ClassDBusy())
    {
    }

    //
    // Initialize the menu controls.
    //
    MenuInit();

    //
    // Initialize as a USB oscilloscope device.
    //
    ScopeUSBDeviceInit();

    //
    // Start up PWM0 to provide a square wave on the board TEST2 pin.
    //
    TestSignalInit();

    //
    // Initialize the data acquisition module.
    //
    bRetcode = DataAcquisitionInit();
    ERROR_CHECK(bRetcode, "ERROR! Can't initialize data acquisition!\n");

    //
    // Set the oscilloscope capture buffer.
    //
    bRetcode = DataAcquisitionSetCaptureBuffer(MAX_SAMPLES_PER_TRIGGER,
                                               g_usScopeData);
    ERROR_CHECK(bRetcode, "ERROR! Can't set channel buffer!\n");

    //
    // Set the default oscilloscope sample rate and triggering mode.  We set
    // the trigger position in the center of the data buffer and the level
    // at the 0 crossing point.  The trigger is set to ALWAYS initially to
    // ensure that we see something on the display regardless of the input
    // to channel 1.
    //
    ChangeTimebase(g_sRender.uluSPerDivision);
    bRetcode = DataAcquisitionSetTrigger(TRIGGER_ALWAYS,
                                  MAX_SAMPLES_PER_TRIGGER/2,
                                  MV_TO_ADC_SAMPLE(DEFAULT_TRIGGER_LEVEL_MV));
    ERROR_CHECK(bRetcode, "ERROR! Can't set trigger!\n");

    //
    // Default to triggering from channel 1.
    //
    bRetcode = DataAcquisitionSetTriggerChannel(true);
    ERROR_CHECK(bRetcode, "ERROR! Can't set trigger channel!\n");

    //
    // Set the interrupt priorities.  We set the GPIO used as the capture abort
    // handler as the highest priority, ADC as the next highest and set up
    // the priority grouping such that these interrupts will preempt all the
    // others.
    //
    IntPriorityGroupingSet(4);
    IntPrioritySet(INT_ADC0SS0, ADC_INT_PRIORITY);
    IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);
    IntPrioritySet(INT_UART0, UART_INT_PRIORITY);
    IntPrioritySet(ABORT_GPIO_INT, ABORT_INT_PRIORITY);
    IntPrioritySet(INT_PWM0_1, AUDIO_INT_PRIORITY);

    //
    // Print out a nice announcement heading.
    //
    UARTprintf("\nQuickstart Oscilloscope\n");
    UARTprintf("-----------------------\n\n");
    UARTprintf("Enter \"help\" for information on commands.\n\n");

    //
    // Request our first data capture.
    //
    g_bCapturePending = true;
    bRetcode = DataAcquisitionRequestCapture();

    //
    // Show the user prompt on the terminal
    //
    UARTprintf(">");

    //
    // Set an initial message giving the user a hint as to how to get
    // into the menu.
    //
    RendererSetAlert("Press Navigate to\nshow the menu.", 400);

    //
    // Now jump into the main loop.
    //
    while(1)
    {
        //
        // Is there currently a capture request outstanding?
        //
        if(g_bCapturePending)
        {
            //
            // Yes - check to see if the data has been captured.
            //
            if(DataAcquisitionIsComplete())
            {
                //
                // Capture has completed so get status and render the
                // waveform.  If no errors were seen, send the data back to
                // the USB host if connected.
                //
                UpdateWaveform(g_bMenuShown, g_bShowingHelp, true);

                //
                // We are no longer waiting for the capture to complete.
                //
               g_bCapturePending = false;
            }
        }

        //
        // Process any commands that we have been sent.
        //
        bRetcode = ProcessCommands(g_bCapturePending);

        //
        // Did any of the commands just processed require is to update the
        // waveform display?
        //
        if(bRetcode)
        {
            UpdateWaveform(g_bMenuShown, g_bShowingHelp, false);
        }

        //
        // If we are performing continuous capture and we don't have a
        // capture request pending, prime for the next capture assuming at
        // least CAPTURES_PER_SECOND ticks have elapsed since the last one.
        // This is to ensure that the UI has some time when capturing at the
        // highest sample rates.
        //
        if(!g_bCapturePending && !g_bMenuShown &&
           g_bContinuousCapture && (g_ulCaptureTick >= CAPTURES_PER_SECOND))
        {
            g_ulCaptureTick = 0;
            g_bCapturePending = true;
            DataAcquisitionRequestCapture();
        }
    }
}
