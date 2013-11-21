//****************************************************************************
//
// chronos_drive.c - Remote control of the EVALBOT using an eZ430-Chronos.
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

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>EVALBOT Controlled by an eZ430-Chronos Watch (chronos_drive)</h1>
//!
//! This application allows the EVALBOT to be driven under radio control from
//! a 915 MHz eZ430-Chronos sport watch.  To run it, you need both the sport
//! watch (running its default firmware) and a "CC1101 Evaluation Module
//! 868-915", part number CC1101EMK868-915.  Both of these can be ordered from
//! estore.ti.com or other Texas Instruments part distributors.
//!
//! To run the demonstration:
//!
//! <ol>
//! <li>Place the EVALBOT within a flat, enclosed space then press the
//! "On/Reset" button. You should see scrolling text appear on its OLED display.
//! If you wait 5 seconds at this step and without doing anything else, the
//! robot will start driving, making random turns or turning away from anything
//! it bumps.
//! </li>
//! <li>Hold the Chronos watch level and repeatedly press the bottom left
//! button on the Chronos watch until you see "ACC" displayed, then press the
//! watch's bottom right button to enable the radio.</li>
//! <li>After a few seconds, the EVALBOT links with the watch and stops
//! driving.  The display also indicates that it is connected to a Chronos.  If
//! this does not occur within 5 seconds, turn the Chronos radio off by
//! pressing the bottom right button again and then resetting the EVALBOT using
//! the "On/Reset" button.</li>
//! <li>Once the watch and EVALBOT are connected, drive the robot by tilting the
//! watch. Tilting the watch forward and backward controls the direction -
//! tilting forward moves the robot forward, and tilting backward moves it in
//! reverse.  The speed is controlled by the amount of tilt.  When reversing,
//! the robot beeps.</li>
//! <li>To turn the robot, when it is moving in forward or reverse, tilt the
//! watch left or right to initiate a turn.</li>
//! </ol>
//!
//! While controlling the EVALBOT, the watch buttons perform the following
//! actions:
//!
//! <dl>
//! <dt>Top Left</dt>     <dd>Stop the EVALBOT.</dd>
//! <dt>Bottom Left</dt>  <dd>Restart the EVALBOT.</dd>
//! <dt>Top Right</dt>    <dd>Sound the EVALBOT horn.</dd>
//! <dt>Bottom Right</dt> <dd>Turn Chronos radio on or off.</dd>
//! </dl>
//!
//! When controlling the EVALBOT, maximum speed is achieved with the watch face
//! oriented vertically (90 degrees from the normal, "flat" viewing position).
//! Since the accelerometer calibration varies slightly from watch to watch, the
//! example contains a calibration mode.  Press "Switch 1" on EVALBOT to enter
//! calibration then move the watch through the full range of movement you want
//! to use to control the robot. Once you have finished the movement, press
//! "Switch 2" to resum normal operation with the control inputs scaled
//! appropriately.
//!
//! The application can be easily recompiled to operate using either the 433 MHz
//! or 868 Mhz version of the eZ430-Chronos.  To operate with a 433 MHz watch,
//! a CC1101EMK433 is required in place of the CC1101EMK868-915 since the
//! antenna design is different between these two frequency bands.
//!
//! To compile for different frequencies, modify the
//! <tt>simpliciti-config.h</tt> file to ensure that labels are defined as
//! follows and then rebuild the application.
//!
//! For 915 MHz operation (as used in the USA):
//!
//! \verbatim
//! #define ISM_US
//! #undef ISM_LF
//! #undef ISM_EU
//! \endverbatim
//!
//! For 868 MHz operation (as used in Europe):
//!
//! \verbatim
//! #undef ISM_US
//! #undef ISM_LF
//! #define ISM_EU
//! \endverbatim
//!
//! For 433 MHz operation (as used in Japan):
//!
//! \verbatim
//! #undef ISM_US
//! #define ISM_LF
//! #undef ISM_EU
//! \endverbatim
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/flash.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/udma.h"
#include "drivers/display96x16x1.h"
#include "drivers/motor.h"
#include "drivers/io.h"
#include "drivers/sensors.h"
#include "drivers/dac.h"
#include "drivers/sound.h"
#include "drivers/wav.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "utils/scheduler.h"
#include "simplicitilib.h"
#include "sounds.h"
#include <string.h>

//*****************************************************************************
//
// For debug purposes, define this label to keep the motors disabled at all
// times.
//
//*****************************************************************************
//#define KEEP_MOTORS_DISABLED

//*****************************************************************************
//
// Defines for setting up the scheduler tick counter.
//
//*****************************************************************************
#define TICKS_PER_SECOND 100

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
// Work loop semaphores
//
//*****************************************************************************
static volatile uint8_t g_ucPeerFrameSem = 0;
static volatile uint8_t g_ucJoinSem = 0;

//*****************************************************************************
//
// The number of bytes in the various data packets from the Chronos watch.
//
//*****************************************************************************
#define ACC_PACKET_SIZE     4
#define R2R_PACKET_SIZE     2
#define STATUS_PACKET_SIZE 19

//*****************************************************************************
//
// Command sent to the Chronos watch in Sync mode requesting it to send its
// status information.
//
//*****************************************************************************
#define SYNC_AP_CMD_GET_STATUS 2
#define SYNC_AP_CMD_SET_WATCH  3

//*****************************************************************************
//
// Values in the bottom nibble of the first byte of accelerometer packets.
// These tell us whether the packet contains accelerometer data in addition to
// the button states.
//
//*****************************************************************************
#define SIMPLICITI_EVENT_MASK   0x0F
#define SIMPLICITI_MOUSE_EVENTS 0x01
#define SIMPLICITI_KEY_EVENTS   0x02

//*****************************************************************************
//
// Values in the top nibble of the first byte of accelerometer packets.
// These tell us whenever the state of one of the watch buttons changes.
//
//*****************************************************************************
#define PACKET_BTN_MASK     0x30
#define PACKET_BTN_SHIFT    4

//*****************************************************************************
//
// Bits representing each of the 3 buttons whose states are passed to us in
// accelerometer packets.  These are equivalent to
//
//     (1 << ((acc_packet[0] & PACKET_BTN_MASK) >> PACKET_BTN_SHIFT))
//
// since the button is encoded as an index in the packet.
//
//*****************************************************************************
#define BUTTON_BIT_STAR     0x02
#define BUTTON_BIT_NUM      0x04
#define BUTTON_BIT_UP       0x08
#define BUTTON_BIT(x) (1 << (((x) & PACKET_BTN_MASK) >> PACKET_BTN_SHIFT))

//*****************************************************************************
//
// EVALBOT states.
//
//*****************************************************************************
typedef enum
{
    //
    // Waiting for a connection from an eZ340-Chronos.
    //
    EVALBOT_STARTUP = 0,

    //
    // Connection open and commands are being received.
    //
    EVALBOT_UNDER_CONTROL,

    //
    // The EVALBOT is currently blocked against an obstruction.
    //
    EVALBOT_BLOCKED,

    //
    // The user has stopped the EVALBOT by pressing a button on the Chronos.
    //
    EVALBOT_STOPPED,

    //
    // The user has stopped the EVALBOT to calibrate the control inputs.
    //
    EVALBOT_CALIBRATING,

    //
    // Connection to eZ430-Chronos broken.  Robot is stopped and is listening
    // for SimpliciTI packets.
    //
    EVALBOT_NO_COMMS,

    //
    // The robot is driving autonomously since no packets have been received
    // from an eZ430-Chronos for at least 5 seconds.
    //
    EVALBOT_AUTONOMOUS,

    //
    // The robot is performing a turn in autonomous mode.
    //
    EVALBOT_AUTONOMOUS_TURNING
}
tEvalBotState;

//*****************************************************************************
//
// The number of accelerometer axis readings we store.
//
//*****************************************************************************
#define NUM_AXES 3

//*****************************************************************************
//
// Reserve space for the maximum possible number of peer Link IDs.
//
//*****************************************************************************
typedef struct
{
    tEvalBotState eState;
    tBoolean bConnected;
    tBoolean bReversing;
    tBoolean bSoundPlaying;
    linkID_t sLinkID;
    short sAccel[NUM_AXES];
    short sLastAccel[NUM_AXES];
    short sMaxAccel[NUM_AXES];
    short sMinAccel[NUM_AXES];
    unsigned char ucButtons;
    unsigned long ulLastRxTime;
    unsigned long ulLastAutonomousChange;
    unsigned long ulAutonomousSegmentTicks;
    tWaveHeader sSoundEffectHeader;
}
tStateVars;

static tStateVars g_sStateInfo;

//*****************************************************************************
//
// Local function prototypes.
//
//*****************************************************************************
static void ProcessAccPacket(uint8_t *pucMsg, unsigned char ucLen);
static void CalibrationModeStart(tStateVars *pState);
static void CalibrationModeStop(tStateVars *pState);
static void CalibrationDefaultsSet(tStateVars *pState);
static void AutonomousModeStart(tStateVars *pState);
static void AutonomousModeStop(tStateVars *pState);
static void AutonomousModeTurnStart(tStateVars *pState);
static void AutonomousModeStraightStart(tStateVars *pState);
static char *StateToString(tEvalBotState eState);
static void EvalBotStop(tStateVars *pState);

//*****************************************************************************
//
// Task functions called by the scheduler.
//
//*****************************************************************************
static void CycleDisplayString(void *pvParam);
static void ScrollTextBanner(void *pvParam);
static void CheckForStateChange(void *pvParam);
static void OutputAccelerometerReadings(void *pvParam);
static void ToggleLED(void *pvParam);
static void UpdateSpeedFromAccelReadings(void *pvParam);
static void CheckForReceivedRadioPacket(void *pvParam);
static void CheckSoundEffect(void *pvParam);

//*****************************************************************************
//
// The strings used for the scrolling banner and a variable storing the position
// of the leftmost character currently displayed.
//
//*****************************************************************************
char *g_pcScrollingBannerStrings[] =
{
    "  Texas Instruments EVALBOT  ",
    "Move watch through its range of motion then press Switch 2  "
};
unsigned long g_ulScrollStringIndex = 0;
unsigned long g_ulScrollStartPos;

#define SCROLL_TI_EVALBOT  0
#define SCROLL_CALIBRATION 1

//*****************************************************************************
//
// This table defines all the tasks that the scheduler is to run, the periods
// between calls to those tasks, and the parameter to pass to the task.
//
//*****************************************************************************
tSchedulerTask g_psSchedulerTable[] =
{
    { ScrollTextBanner, (void *)g_pcScrollingBannerStrings, 20, 0, true},
    { CycleDisplayString, (void *)1, 75, 0, true},
    { ToggleLED, (void *)0, 40, 0, true},
    { UpdateSpeedFromAccelReadings, (void *)&g_sStateInfo, 50, 0, true},
    { OutputAccelerometerReadings, (void *)&g_sStateInfo, 100, 0, true},
    { CheckForStateChange, (void *)&g_sStateInfo, 10, 0, true},
    { CheckSoundEffect, (void *)&g_sStateInfo, 3, 0, true},
    { CheckForReceivedRadioPacket, (void *)&g_sStateInfo, 0, 0, true},
};

//*****************************************************************************
//
// Indices of the various tasks described in g_psSchedulerTable.
//
//*****************************************************************************
#define TASK_SCROLL_BANNER 0
#define TASK_CYCLE_STRINGS 1
#define TASK_TOGGLE_LEDS   2
#define TASK_UPDATE_SPEED  3
#define TASK_OUTPUT_ACCEL  4
#define TASK_CHECK_STATE   5

//*****************************************************************************
//
// The number of entries in the global scheduler task table.
//
//*****************************************************************************
unsigned long g_ulSchedulerNumTasks = (sizeof(g_psSchedulerTable) /
                                       sizeof(tSchedulerTask));

//*****************************************************************************
//
// The number of ticks to wait without receiving a packet before we stop the
// motors.
//
//*****************************************************************************
#define PACKET_TIMEOUT 100

//*****************************************************************************
//
// The number of ticks we wait to another packet to be received before we switch
// into autonomous mode.
//
//*****************************************************************************
#define WAIT_TIMEOUT 500

//*****************************************************************************
//
// Strings which cycle periodically on the display.
//
//*****************************************************************************
char g_pcString1[16];
char g_pcString2[16];

char *g_pcCyclingStrings[] =
{
    g_pcString1,
    g_pcString2
};

#define NUM_CYCLING_STRINGS (sizeof(g_pcCyclingStrings) / sizeof(char *))

//*****************************************************************************
//
// The index of the string which is to be shown next and the number of strings
// that are to be cycled on the display.
//
//*****************************************************************************
unsigned long g_ulCurrentString;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
    UARTprintf("Error at line %d of file %s!\n", ulLine, pcFilename);
    while(1)
    {
        //
        // Hang here.
        //
    }
}
#endif

//*****************************************************************************
//
// Map a state to a human-readable string.
//
//*****************************************************************************
static char *
StateToString(tEvalBotState eState)
{
    switch(eState)
    {
        case EVALBOT_STARTUP:
        {
            return("STARTUP");
        }

        case EVALBOT_UNDER_CONTROL:
        {
            return("UNDER_CONTROL");
        }

        case EVALBOT_BLOCKED:
        {
            return("BLOCKED");
        }

        case EVALBOT_NO_COMMS:
        {
            return("NO_COMMS");
        }

        case EVALBOT_STOPPED:
        {
            return("STOPPED");
        }

        case EVALBOT_CALIBRATING:
        {
            return("CALIBRATING");
        }

        case EVALBOT_AUTONOMOUS:
        {
            return("AUTONOMOUS");
        }

        case EVALBOT_AUTONOMOUS_TURNING:
        {
            return("AUTONOMOUS_TURNING");
        }

        default:
        {
            return("**ILLEGAL**");
        }
    }
}

//*****************************************************************************
//
// SimpliciTI receive callback.  This function runs in interrupt context.
// Reading the frame should be done in the application main loop or thread not
// in the ISR.  Here we merely set flags to tell the main loop what is needs
// to do.
//
//*****************************************************************************
static uint8_t
ReceiveCallback(linkID_t sLinkID)
{
    //
    // Have we been sent a frame on an active link?
    //
    if (sLinkID)
    {
        //
        // Yes - Set the semaphore indicating we need to receive the frame.
        // This will be done in the main loop.
        //
        g_ucPeerFrameSem++;
    }
    else
    {
        //
        // No - a new device has joined the network but has not linked to us.
        // Set the semaphore indicating that we should listen for an incoming
        // link request.
        //
        g_ucJoinSem++;
    }

    //
    // Leave the frame to be read by the main loop of the application.
    //
    return 0;
}

//*****************************************************************************
//
// Set the SimpliciTI device address as the least significant 4 digits of the
// device Ethernet MAC address.  This ensures that the address is unique across
// Stellaris devices.  If the MAC address has not been set, we return false to
// indicate failure.
//
//*****************************************************************************
tBoolean
SetSimpliciTIAddress(void)
{
    unsigned long ulUser0, ulUser1;
    addr_t sAddr;

    //
    // Make sure we are using 4 byte addressing.
    //
    ASSERT(NET_ADDR_SIZE == 4);

    //
    // Get the MAC address from the non-volatile user registers.
    //
    FlashUserGet(&ulUser0, &ulUser1);

    //
    // Has the MAC address been programmed?
    //
    if((ulUser0 == 0xffffffff) || (ulUser1 == 0xffffffff))
    {
        //
        // No - we don't have an address so return a failure.
        //
        UARTprintf("Flash user registers are clear");
        UARTprintf("Error - address not set!");
        Display96x16x1StringDraw("MAC not set!", 0, 1);
        return(false);
    }
    else
    {
        //
        // The MAC address is stored with 3 bytes in each of the 2 flash user
        // registers.  Extract the least significant 4 MAC bytes for use as the
        // SimpliciTI device address.
        //
        sAddr.addr[0] = ((ulUser1 >> 16) & 0xff);
        sAddr.addr[1] = ((ulUser1 >>  8) & 0xff);
        sAddr.addr[2] = ((ulUser1 >>  0) & 0xff);
        sAddr.addr[3] = ((ulUser0 >> 16) & 0xff);

        //
        // SimpliciTI requires that the first byte of the device address is
        // never either 0x00 or 0xFF so we check for these cases and invert the
        // first bit if either is detected.  This does result in the
        // possibility of two devices having the same address but, for example
        // purposes, is likely to be fine.
        //
        if((sAddr.addr[0] == 0x00) || (sAddr.addr[0] == 0xFF))
        {
            sAddr.addr[0] ^= 0x80;
        }

        //
        // Tell the SimpliciTI stack which device address we want to use.
        //
        SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &sAddr);
    }

    //
    // If we get here, all is well.
    //
    return(true);
}

//*****************************************************************************
//
// This function is called periodically by the scheduler to check to see if
// 1 second has elapsed since receiving a packet from the eZ430-Chronos.  If
// this timeout occurs, the motors are stopped and the EVALBOT waits for 5
// seconds before switching into automomous mode.
//
//*****************************************************************************
static void
CheckForStateChange(void *pvParam)
{
    tStateVars *pState;
    tEvalBotState eOriginalState;
    unsigned long ulElapsed;
    bspIState_t intState;
    smplStatus_t eRetcode;


    //
    // Get a pointer to the instance data.
    //
    pState = (tStateVars *)pvParam;
    eOriginalState = pState->eState;

    //
    // How long has it been since the last packet was received?
    //
    ulElapsed = SchedulerElapsedTicksGet(pState->ulLastRxTime);

    switch(pState->eState)
    {
        //
        // We are starting up so check to see if there has been an incoming
        // link and, if so, listen for the new connection.
        //
        case EVALBOT_STARTUP:
        {
            //
            // Has a device joined our network and are we still waiting for a
            // device to link?
            //
            if (g_ucJoinSem)
            {
                //
                // If we don't already have an active connection, listen for
                // a new one.
                //
                if(g_sStateInfo.bConnected == false)
                {
                    //
                    // Listen for the incoming connection request.
                    //
                    eRetcode = SMPL_LinkListen(&pState->sLinkID);
                    if (SMPL_SUCCESS == eRetcode)
                    {
                        //
                        // The connection attempt succeeded so break out of the
                        // loop after noting that we are now connected.
                        //
                        usnprintf(g_pcString2, 16, "Connected");
                        g_sStateInfo.bConnected = true;

                        //
                        // Set our state now that we have a connection.  This
                        // state indicates that we are waiting for accelerometer
                        // packets from the watch.
                        //
                        g_sStateInfo.eState = EVALBOT_NO_COMMS;
                    }

                    //
                    // Decrement the join semaphore.
                    //
                    BSP_ENTER_CRITICAL_SECTION(intState);
                    g_ucJoinSem--;
                    BSP_EXIT_CRITICAL_SECTION(intState);
                }
            }

            //
            // Have we waited for a connection long enough that we need to
            // switch into autonomous mode?
            //
            if(ulElapsed > WAIT_TIMEOUT)
            {
                //
                // Yes - switch into autonomous mode.
                //
                AutonomousModeStart(pState);
                pState->eState = EVALBOT_AUTONOMOUS;
                usnprintf(g_pcString2, 16, "AUTONOMOUS");
            }
            break;
        }

        //
        // We have a connection but have not received a packet within a
        // particular timeout period.  Check to see if we have received a
        // packet and, if so, switch into UNDER_CONTROL state.  Otherwise,
        // if we've waited more than WAIT_TIMEOUT ticks, switch to autonomous
        // mode.
        //
        case EVALBOT_NO_COMMS:
        {
            //
            // Do we need to switch into autonomous mode?
            //
            if(ulElapsed > WAIT_TIMEOUT)
            {
                //
                // Yes - switch into autonomous mode.
                //
                AutonomousModeStart(pState);
                pState->eState = EVALBOT_AUTONOMOUS;
                usnprintf(g_pcString2, 16, "AUTONOMOUS");
            }
            else
            {
                //
                // Have we recently received a packet?
                //
                if(ulElapsed < PACKET_TIMEOUT)
                {
                    //
                    // Yes - switch into UNDER_CONTROL mode.
                    //
                    SchedulerTaskEnable(TASK_UPDATE_SPEED, true);
                    pState->eState = EVALBOT_UNDER_CONTROL;
                    usnprintf(g_pcString2, 16, "Connected");
                }
            }
            break;
        }

        //
        // The EVALBOT is driving under the control of a Chronos.
        //
        case EVALBOT_UNDER_CONTROL:
        {
            //
            // Check to see if we need to enter calibration mode.
            //
            if(!PushButtonGetStatus(BUTTON_1))
            {
                CalibrationModeStart(pState);
                pState->eState = EVALBOT_CALIBRATING;
            }
            else
            {
                //
                // Have we received a packet within PACKET_TIMOUT ticks?
                //
                if(ulElapsed >= PACKET_TIMEOUT)
                {
                    //
                    // No - switch into NO_COMMS state and stop the motors.
                    //
                    SchedulerTaskDisable(TASK_UPDATE_SPEED);
                    EvalBotStop(pState);
                    pState->eState = EVALBOT_NO_COMMS;
                    usnprintf(g_pcString2, 16, "No Chronos");
                }
                else
                {
                    //
                    // We have received a packet recently. First see if the "*"
                    // button was pressed indicating that we should stop.
                    //
                    if(pState->ucButtons == BUTTON_BIT_STAR)
                    {
                        EvalBotStop(pState);
                        pState->eState = EVALBOT_STOPPED;
                        usnprintf(g_pcString2, 16, "** STOPPED **");
                    }
                    else
                    {
                        //
                        // Now check to see if we've hit something.
                        //
                        if(!BumpSensorGetStatus(LEFT_SIDE) ||
                           !BumpSensorGetStatus(RIGHT_SIDE))
                        {
                            //
                            // One or other bumper is signalling a hit so stop
                            // the motors and change state.
                            //
                            MotorStop(LEFT_SIDE);
                            MotorStop(RIGHT_SIDE);
                            pState->eState = EVALBOT_BLOCKED;
                            usnprintf(g_pcString2, 16, "** BLOCKED **");
                        }
                    }
                }
            }
            break;
        }

        //
        // The EVALBOT has been stopped by the user pressing a button on the
        // Chronos.  We stay in this state until a button is pressed again.
        //
        case EVALBOT_STOPPED:
        {
            //
            // Check to see if we need to enter calibration mode.
            //
            if(!PushButtonGetStatus(BUTTON_1))
            {
                CalibrationModeStart(pState);
                pState->eState = EVALBOT_CALIBRATING;
            }
            else
            {
                //
                // If the "#" button is pressed, we resume motion.
                //
                if(pState->ucButtons == BUTTON_BIT_NUM)
                {
                    pState->eState = EVALBOT_UNDER_CONTROL;
                    usnprintf(g_pcString2, 16, "Connected");
                    SchedulerTaskEnable(TASK_UPDATE_SPEED, true);
                }
            }
            break;
        }

        case EVALBOT_CALIBRATING:
        {
            //
            // Check to see if we need to exit calibration mode.
            //
            if(!PushButtonGetStatus(BUTTON_2))
            {
                pState->eState = EVALBOT_UNDER_CONTROL;
                CalibrationModeStop(pState);
            }

            break;
        }

        //
        // The EVALBOT is currently blocked against an obstacle while being
        // controlled by a Chronos.
        //
        case EVALBOT_BLOCKED:
        {
            //
            // Are the bumpers now clear or are we being ordered to reverse?
            //
            if((BumpSensorGetStatus(LEFT_SIDE) &&
                BumpSensorGetStatus(RIGHT_SIDE)) || pState->bReversing)
            {
                //
                // Yes - switch back to UNDER_CONTROL mode to continue normal
                // driving.
                //
                pState->eState = EVALBOT_UNDER_CONTROL;
                usnprintf(g_pcString2, 16, "Connected");
            }

            break;
        }

        //
        // We are driving autonomously in a straight line.
        //
        case EVALBOT_AUTONOMOUS:
        {
            //
            // Have we received a new incoming link when no existing Chronos
            // connection is in place?
            //
            if (g_ucJoinSem && !pState->bConnected)
            {
                //
                // Yes - a Chronos is trying to talk to us so go back and
                // listen for the incoming connection.
                //
                pState->eState = EVALBOT_STARTUP;
            }
            else
            {
                //
                // Have we received a packet recently?
                //
                if(ulElapsed < PACKET_TIMEOUT)
                {
                    //
                    // Yes - switch back into UNDER_CONTROL mode to allow user
                    // control of the EVALBOT.
                    //
                    AutonomousModeStop(pState);
                    pState->eState = EVALBOT_UNDER_CONTROL;
                    usnprintf(g_pcString2, 16, "Connected");
                }
                else
                {
                    //
                    // How long have we been driving in this segment?
                    //
                    ulElapsed = SchedulerElapsedTicksGet(
                                               pState->ulLastAutonomousChange);

                    //
                    // If we've been driving long enough or one of the bumpers
                    // is registering a hit, turn a random amount.
                    //
                    if((ulElapsed >= pState->ulAutonomousSegmentTicks) ||
                       !BumpSensorGetStatus(LEFT_SIDE) ||
                       !BumpSensorGetStatus(RIGHT_SIDE))
                    {
                        AutonomousModeTurnStart(pState);
                        pState->eState = EVALBOT_AUTONOMOUS_TURNING;
                    }
                }
            }
            break;
        }

        //
        // We are making a turn in autonomous mode.
        //
        case EVALBOT_AUTONOMOUS_TURNING:
        {
            //
            // Have we received a new incoming link when no existing Chronos
            // connection is in place?
            //
            if (g_ucJoinSem && !pState->bConnected)
            {
                //
                // Yes - a Chronos is trying to talk to us so go back and
                // listen for the incoming connection.
                //
                pState->eState = EVALBOT_STARTUP;
                usnprintf(g_pcString2, 16, "Listening");
            }
            else
            {
                //
                // Have we received a packet recently?
                //
                if(ulElapsed < PACKET_TIMEOUT)
                {
                    //
                    // Yes - switch back into UNDER_CONTROL mode to allow user
                    // control of the EVALBOT.
                    //
                    AutonomousModeStop(pState);
                    pState->eState = EVALBOT_UNDER_CONTROL;
                    usnprintf(g_pcString2, 16, "Connected");
                }
                else
                {
                    //
                    // How long have we been driving in this segment?
                    //
                    ulElapsed = SchedulerElapsedTicksGet(
                                               pState->ulLastAutonomousChange);

                    //
                    // If we've been turning long enough drive straight for a
                    // while.
                    //
                    if(ulElapsed >= pState->ulAutonomousSegmentTicks)
                    {

                        AutonomousModeStraightStart(pState);
                        pState->eState = EVALBOT_AUTONOMOUS;
                    }
                }
            }

            break;
        }
    }

    //
    // Output the state transition to the UART if a transition occured.
    //
    if(eOriginalState != pState->eState)
    {
        UARTprintf("%s -> %s\n", StateToString(eOriginalState),
                   StateToString(pState->eState));
    }
}

//*****************************************************************************
//
// This function is called periodically by the scheduler to play sound effects.
// If a sound is currently playing, it ensures that data is fed to the wav
// driver as required.  If no sound is playing, it checks for bump or reverse
// conditions and starts playing a sound if required.
//
//*****************************************************************************
static void
CheckSoundEffect(void *pvParam)
{
    tStateVars *pState;
    tWaveReturnCode eRetcode;
    const unsigned char *pcNewSound;
    tBoolean bComplete;

    //
    // Get a pointer to the instance data.
    //
    pState = (tStateVars *)pvParam;

    //
    // Are we currently playing a sound effect?
    //
    if(pState->bSoundPlaying)
    {
        //
        // Feed the wave file processor.
        //
        bComplete = WavePlayContinue(&pState->sSoundEffectHeader);

        //
        // Did we finish playback of this sound effect?
        //
        if(bComplete)
        {
            //
            // Yes - remember that we are no longer playing anything.
            //
            pState->bSoundPlaying = false;

            //
            // Clear the button indicator if we are not currently in reverse.
            // This is somewhat hacky but it prevents the horn from sounding
            // twice when the button is pressed without preventing you from
            // sounding the horn while the EVALBOT is rolling in reverse.
            //
            if(!pState->bReversing)
            {
                pState->ucButtons = 0;
            }
        }
    }
    else
    {
        //
        // We are not currently playing a sound.  By default, we won't start
        // playback of any new sound effect.
        pcNewSound = (unsigned char *)0;

        // Check if we've hit anything.
        //
        if(!BumpSensorGetStatus(LEFT_SIDE) ||
           !BumpSensorGetStatus(RIGHT_SIDE))
        {
            //
            // We hit something so play the bump sound.
            //
            pcNewSound = g_pcBumpSound;
        }
        else
        {
            //
            // Did the driver press the horn button on the watch?
            //
            if(pState->ucButtons == BUTTON_BIT_UP)
            {
                //
                // Yes - play the horn sound next.
                //
                pcNewSound = g_pcHornSound;

                //
                // Clear the button indicator now that we've seen it.
                //
                pState->ucButtons = 0;
            }
            else
            {
                //
                // Are we reversing?
                //
                if(pState->bReversing)
                {
                    //
                    // Yes - play the reversing sound.
                    //
                    pcNewSound = g_pcReverseSound;
                }
            }
        }

        //
        // Do we need to start playback of a new sound effect?
        //
        if(pcNewSound)
        {
            //
            // Yes - parse the sound header.
            //
            eRetcode = WaveOpen((unsigned long *)pcNewSound,
                                &pState->sSoundEffectHeader);

            //
            // Did we parse the sound successfully?
            //
            if(eRetcode == WAVE_OK)
            {
                //
                // Remember that we are playing a sound effect.
                //
                pState->bSoundPlaying = true;

                //
                // Start playback of the wave file data.
                //
                WavePlayStart(&pState->sSoundEffectHeader);
            }
        }
    }
}

//*****************************************************************************
//
// This function is called periodically by the scheduler to display the latest
// accelerometer readings on the UART output.
//
//*****************************************************************************
static void
OutputAccelerometerReadings(void *pvParam)
{
    tStateVars *pState;

    //
    // Get a pointer to the instance data.
    //
    pState = (tStateVars *)pvParam;

    //
    // Output the latest accelerometer readings to the UART.
    //
    UARTprintf("X: %4d Y: %4d Z: %4d\n", pState->sAccel[0],
               pState->sAccel[1], pState->sAccel[2]);
}

//*****************************************************************************
//
// This function may be called periodically by the scheduler to change the
// string shown on the given line of the OLED display.  The line to display
// the string on is provided in the pvParam parameter - top line is 0, bottom
// line is 1.
//
//*****************************************************************************
static void
CycleDisplayString(void *pvParam)
{
    //
    // Show the next string on the appropriate line of the display.
    //
    Display96x16x1StringDrawCentered(g_pcCyclingStrings[g_ulCurrentString],
                                     (unsigned long)pvParam, true);

    //
    // Update the string that we will display next, taking care to wrap when
    // we reach the end of the array.
    //
    g_ulCurrentString++;
    if(g_ulCurrentString >= NUM_CYCLING_STRINGS)
    {
        g_ulCurrentString = 0;
    }
}

//*****************************************************************************
//
// This function is called periodically by the scheduler to change the string
// shown on a given line of the OLED display.  The parameter passed is a
// pointer to the string table in use. Global variables control which string
// in the table to display, and the current position on the display.
//
//*****************************************************************************
static void
ScrollTextBanner(void *pvParam)
{
    unsigned long ulToDraw, ulStringLen;
    char *pcString;

    //
    // Get a pointer to the string we need to scroll.
    //
    pcString = ((char **)pvParam)[g_ulScrollStringIndex];

    //
    // How many characters do we need to draw for the left portion of the
    // display?
    //
    ulStringLen = strlen(pcString);

    //
    // Check that the scroll start position is within the string.
    //
    if(g_ulScrollStartPos >= ulStringLen)
    {
        g_ulScrollStartPos = 0;
    }

    //
    // How many characters do we need to draw on the left of the display?
    //
    ulToDraw =  ulStringLen - g_ulScrollStartPos;
    if(ulToDraw > CHARS_PER_LINE)
    {
        ulToDraw = CHARS_PER_LINE;
    }

    //
    // Draw the first substring on the display.
    //
    Display96x16x1StringDrawLen(pcString + g_ulScrollStartPos, ulToDraw, 0, 0);

    //
    // Do we need to draw a second string?
    //
    if(ulToDraw < CHARS_PER_LINE)
    {
        //
        // Draw the start of the string at the correct position on the display.
        //
        Display96x16x1StringDrawLen(pcString, (CHARS_PER_LINE - ulToDraw),
                                    ulToDraw * CHAR_CELL_WIDTH, 0);
    }

    //
    // Update the scroll text start position and, if necessary, wrap it back to
    // the start.
    //
    g_ulScrollStartPos++;

    if(g_ulScrollStartPos == (ulStringLen - 1))
    {
        g_ulScrollStartPos = 0;
    }
}

//*****************************************************************************
//
// This function is called by the scheduler to toggle one or both LEDs.
//
//*****************************************************************************
static void
ToggleLED(void *pvParam)
{
    switch((unsigned long)pvParam)
    {
        case 0:
        {
            LED_Toggle(BOTH_LEDS);
            break;
        }

        case 1:
        {
            LED_Toggle(LED_1);
            break;
        }

        case 2:
        {
            LED_Toggle(LED_2);
            break;
        }
    }
}

//*****************************************************************************
//
// Scale one accelerometer reading into the range [-50, 50] using current
// calibration information.
//
//*****************************************************************************
static short
NormalizeReading(tStateVars *pState, unsigned long ulIndex)
{
    short sOffset, sLen;
    long lResult;

    //
    // What is the range of control defined by the current calibration info?
    //
    sLen = pState->sMaxAccel[ulIndex] - pState->sMinAccel[ulIndex];

    //
    // Clip the current accelerometer reading to the current calibration range.
    // This guards against cases where high readings result from non-gravity-
    // induced acceleration (knocks, bumps, rapid movements).
    //
    if(pState->sAccel[ulIndex] > pState->sMaxAccel[ulIndex])
    {
        pState->sAccel[ulIndex] = pState->sMaxAccel[ulIndex];
    }
    if(pState->sAccel[ulIndex] < pState->sMinAccel[ulIndex])
    {
        pState->sAccel[ulIndex] = pState->sMinAccel[ulIndex];
    }

    //
    // What is the offset of the current reading from the minimum value?
    //
    sOffset = pState->sAccel[ulIndex] - pState->sMinAccel[ulIndex];

    //
    // Calculate the equivalent value in the [-50, 50] range.
    //
    lResult = (100 * (long)sOffset) / (long)sLen;
    lResult -= 50;

    return((short)lResult);
}

//*****************************************************************************
//
// Use the current control calibration data to scale the X and Y acceleration
// readings into the range (-50, 50).
//
//*****************************************************************************
static void
NormalizeAccelReadings(tStateVars *pState, short *pX, short *pY)
{
    //
    // First process the X reading.
    //
    *pX = NormalizeReading(pState, 0);

    //
    // Now process the Y reading.
    //
    *pY = NormalizeReading(pState, 1);
}

//*****************************************************************************
//
// Calculate left- and right-side motor speeds based upon the latest
// accelerometer readings.
//
//*****************************************************************************
static void
UpdateSpeedFromAccelReadings(void *pvParam)
{
    tStateVars *pState;
    short sX, sY;
    long lLeftSpeed, lRightSpeed;

    pState = (tStateVars *)pvParam;

    //
    // Has either X or Y accelerometer reading changed?
    //
    if((pState->sLastAccel[0] != pState->sAccel[0]) ||
       (pState->sLastAccel[1] != pState->sAccel[1]))
    {
        //
        // Calculate normalized the X and Y accelerometer readings based on the
        // current calibration information.
        //
        NormalizeAccelReadings(pState, &sX, &sY);

        //
        // We are directly controlling the forward speed using the X
        // accelerometer reading.
        //
        lLeftSpeed = (long)(-sX * 2);
        lRightSpeed = lLeftSpeed;

        //
        // Remember whether we are running forward or backwards.
        //
        pState->bReversing = (lLeftSpeed < 0) ? true : false;

        //
        // The Y value governs how fast we turn and acts to control the
        // difference in speed between the left and right motors.  Using this
        // control method, the Y value is used as a multiplier and retards the
        // motor on the side we want to turn towards.  This is a bit more like
        // the way a car would turn - you need some speed to make a turn.
        //

        //
        // Are we applying a right turn?
        //
        if(sY > 0)
        {
            //
            // Yes - slow the right wheel according to the Y reading.
            //
            lRightSpeed = (lRightSpeed * 2 * (long)(50 - sY)) / 100;
        }
        else
        {
            //
            // No - we're making a left turn so slow the left wheel.
            //
            lLeftSpeed = (lLeftSpeed * 2 * (long)(50 + sY)) / 100;
        }

        //
        // One final (somewhat hacky) check -  the motor speed doesn't like
        // being set to 100% so check for this and reduce to 99% if necessary.
        //
        lLeftSpeed = (lLeftSpeed > 99) ? 99 : lLeftSpeed;
        lRightSpeed = (lRightSpeed > 99) ? 99 : lRightSpeed;

        //
        //
        // Update the string used to display the motor speeds.
        //
        usnprintf(g_pcString1, 16, "L:%5d R:%5d", lLeftSpeed, lRightSpeed);

        //
        // Set the motor speed appropriately.
        //
        MotorDir(LEFT_SIDE, (lLeftSpeed > 0) ? FORWARD : REVERSE);
        MotorDir(RIGHT_SIDE, (lRightSpeed > 0) ? FORWARD : REVERSE);

        //
        // Determine the absolute motor speeds now that we've set the direction.
        //
        lLeftSpeed = (lLeftSpeed < 0) ? -lLeftSpeed : lLeftSpeed;
        lRightSpeed = (lRightSpeed < 0) ? -lRightSpeed : lRightSpeed;

        MotorSpeed(LEFT_SIDE, ((short)lLeftSpeed << 8));
        MotorSpeed(RIGHT_SIDE, ((short)lRightSpeed << 8));

        //
        // Start the motors if we are not blocked unless we are in reverse.
        //
#ifndef KEEP_MOTORS_DISABLED
        if((pState->eState != EVALBOT_BLOCKED) || pState->bReversing)
        {
            //
            // Start both motors.
            //
            MotorRun(LEFT_SIDE);
            MotorRun(RIGHT_SIDE);
        }
#endif
    }
}

//*****************************************************************************
//
// This function is called by the scheduler to check for incoming SimpliciTI
// radio packets.  If a packet is available, it is read and parsed.
//
//*****************************************************************************
static void
CheckForReceivedRadioPacket(void *pvParam)
{
    unsigned long ulLoop;
    unsigned char ucLen;
    unsigned char pucMsg[MAX_APP_PAYLOAD];
    bspIState_t intState;
    tStateVars *pState;

    //
    // Get a pointer to our state information.
    //
    pState = (tStateVars *)pvParam;

    //
    // Have we received a radio frame?
    //
    if (g_ucPeerFrameSem)
    {
        //
        // Receive the message.
        //
        if (SMPL_SUCCESS == SMPL_Receive(pState->sLinkID, pucMsg,
                                         &ucLen))
        {
            //
            // Does the packet size indicate that this is likely to be an
            // accelerometer packet?
            //
            if(ucLen == ACC_PACKET_SIZE)
            {
                //
                // Process the accelerometer packet.
                //
                ProcessAccPacket(pucMsg, ucLen);

                //
                // Grab a timestamp so that we know when we received the
                // last packet.
                //
                pState->ulLastRxTime = SchedulerTickCountGet();

                //
                // If we are in calibration mode, update the signal bounds.
                //
                if(pState->eState == EVALBOT_CALIBRATING)
                {
                    for(ulLoop = 0; ulLoop < 3; ulLoop++)
                    {
                        //
                        // Update the maximum value if this acceleration
                        // reading is above the current maximum.
                        //
                        if(pState->sAccel[ulLoop] > pState->sMaxAccel[ulLoop])
                        {
                            pState->sMaxAccel[ulLoop] = pState->sAccel[ulLoop];
                        }

                        //
                        // Update the minimum value if this acceleration
                        // reading is below the current minimum.
                        //
                        if(pState->sAccel[ulLoop] < pState->sMinAccel[ulLoop])
                        {
                            pState->sMinAccel[ulLoop] = pState->sAccel[ulLoop];
                        }
                    }
                }
            }

            //
            // Decrement our frame semaphore.
            //
            BSP_ENTER_CRITICAL_SECTION(intState);
            g_ucPeerFrameSem--;
            BSP_EXIT_CRITICAL_SECTION(intState);
        }
    }
}

//*****************************************************************************
//
// The handler called to process packets containing button and, optionally,
// accelerometer data.
//
// On return from this function, g_sStateInfo.ucButtons and g_sStateInfo.sAccel
// have been updated.
//
//*****************************************************************************
static void
ProcessAccPacket(uint8_t *pucMsg, unsigned char ucLen)
{
    unsigned long ulLoop;
    unsigned char ucMode;

    //
    // Which mode is the watch in?  We tell this from the first byte of
    // the packet.
    //
    ucMode = pucMsg[0] & SIMPLICITI_EVENT_MASK;

    //
    // Make sure the packet contains accelerometer and/or button data.  Ignore
    // it if not.
    //
    if((ucMode != SIMPLICITI_MOUSE_EVENTS) && (ucMode != SIMPLICITI_KEY_EVENTS))
    {
        return;
    }

    //
    // Has any button been pressed?
    //
    if((pucMsg[0] & PACKET_BTN_MASK))
    {
        //
        // Yes, a button has been pressed.  Remember which one.
        //
        g_sStateInfo.ucButtons = BUTTON_BIT(pucMsg[0]);
    }

    //
    // If this packet contains accelerometer data, update it.
    //
    if(ucMode == SIMPLICITI_MOUSE_EVENTS)
    {
        //
        // Update our copy of the acceleration values.  We read all 3 axes but
        // end up only using the X and Y data.
        //
        for(ulLoop = 0; ulLoop < NUM_AXES; ulLoop++)
        {
            //
            // Update the acceleration stored.  We use a simple filter to
            // smooth the accelerometer readings.
            //
            g_sStateInfo.sAccel[ulLoop] =
                ((g_sStateInfo.sAccel[ulLoop] * 3) / 4) +
                (((short)((signed char)pucMsg[ulLoop + 1])) / 4);
        }
    }
}

//*****************************************************************************
//
// Enter control calibration mode.
//
//*****************************************************************************
static void
CalibrationModeStart(tStateVars *pState)
{
    unsigned long ulLoop;

    //
    // Stop the EVALBOT while we are calibrating the controls.
    //
    EvalBotStop(pState);

    //
    // Replace the scrolling string with instructions.
    //
    g_ulScrollStringIndex = SCROLL_CALIBRATION;
    g_ulScrollStartPos = 0;

    //
    // Make sure the user knows what is going on.
    //
    usnprintf(g_pcString2, 16, "CALIBRATING");

    //
    // Turn off speed control via the accelerometers.
    //
    SchedulerTaskDisable(TASK_UPDATE_SPEED);

    //
    // Clear the current maximum and minimum acceleration values to ensure that
    // we read the new values from the watch.
    //
    for(ulLoop = 0; ulLoop < NUM_AXES; ulLoop++)
    {
        pState->sMinAccel[ulLoop] = (short)0x7FFF;
        pState->sMaxAccel[ulLoop] = (short)0x8000;
    }

    //
    // Display some status on the UART output.
    //
    UARTprintf("Entering control calibration mode.\n");
}

//*****************************************************************************
//
// Exit control calibration mode.
//
//*****************************************************************************
static void
CalibrationModeStop(tStateVars *pState)
{
    //
    // Revert to the normal scrolling banner string.
    //
    g_ulScrollStringIndex = SCROLL_TI_EVALBOT;
    g_ulScrollStartPos = 0;

    //
    // Update the mode string.
    //
    usnprintf(g_pcString2, 16, "Connected");

    //
    // Reinstate motor control based on the accelerometer readings.
    //
    SchedulerTaskEnable(TASK_UPDATE_SPEED, true);

    //
    // Dump the new calibration values to the UART.
    //
    UARTprintf("New calibration settings - X [%d, %d], Y [%d, %d]\n",
               pState->sMinAccel[0], pState->sMaxAccel[0],
               pState->sMinAccel[1], pState->sMaxAccel[1]);
}

//*****************************************************************************
//
// Set the default calibration values for the control system.
//
//*****************************************************************************
static void
CalibrationDefaultsSet(tStateVars *pState)
{
    unsigned long ulLoop;

    //
    // Reset the bounds of the accelerometer readings to the default values.
    //
    for(ulLoop = 0; ulLoop < NUM_AXES; ulLoop++)
    {
        pState->sMaxAccel[ulLoop] = 50;
        pState->sMinAccel[ulLoop] = -50;
    }
}

//*****************************************************************************
//
// Enter autonomous driving mode.
//
//*****************************************************************************
static void
AutonomousModeStart(tStateVars *pState)
{
    //
    // Turn off speed control via the accelerometers.
    //
    SchedulerTaskDisable(TASK_UPDATE_SPEED);

    //
    // Start by moving forward for a random time.
    //
    AutonomousModeStraightStart(pState);
}

//*****************************************************************************
//
// Exit autonomous driving mode.
//
//*****************************************************************************
static void
AutonomousModeStop(tStateVars *pState)
{
    //
    // Reinstate motor control based on the accelerometer readings.
    //
    SchedulerTaskEnable(TASK_UPDATE_SPEED, true);
}

//*****************************************************************************
//
// Set up to drive straight ahead at a random speed.
//
//*****************************************************************************
static void
AutonomousModeStraightStart(tStateVars *pState)
{
    unsigned long ulTime, ulSpeed;

    //
    // Get a random number.
    //
    ulTime = (unsigned long)urand();

    //
    // Shift it to reduce the range into [0-512].
    //
    ulTime >>= 23;

    //
    // Add 500 so that we run for at least five seconds (this counts ticks).
    //
    ulTime += 500;

    //
    // Remember how long we are going to turn for.
    //
    pState->ulAutonomousSegmentTicks = ulTime;

    //
    // Pick a random speed between 50% and 99%.
    //
    ulSpeed = ((((unsigned long)urand() >> 24) * 50) / 256) + 50;


    //
    // Set the motors to run at the same speed.
    //
    MotorSpeed(LEFT_SIDE, (ulSpeed << 8));
    MotorSpeed(RIGHT_SIDE, (ulSpeed << 8));

    //
    // Set both motors to run forwards.
    //
    MotorDir(LEFT_SIDE, FORWARD);
    MotorDir(RIGHT_SIDE, FORWARD);

#ifndef KEEP_MOTORS_DISABLED
    //
    // Turn both motors on.
    //
    MotorRun(LEFT_SIDE);
    MotorRun(RIGHT_SIDE);
#endif

    //
    // Remember when we made the last change under autonomous control.
    //
    pState->ulLastAutonomousChange = SchedulerTickCountGet();

    UARTprintf("0x%08x: Straight at %d%% for %d ticks\n",
               pState->ulLastAutonomousChange, ulSpeed, ulTime);
}

//*****************************************************************************
//
// Set up to turn on the spot for a random period of time.
//
//*****************************************************************************
static void
AutonomousModeTurnStart(tStateVars *pState)
{
    unsigned long ulTime;
    tBoolean bTurnLeft, bBump;

    //
    // Set the motors to run at the same speed.
    //
    MotorSpeed(LEFT_SIDE, (50 << 8));
    MotorSpeed(RIGHT_SIDE, (50 << 8));

    //
    // Determine which direction we should turn.  If either of the bumpers is
    // registering a hit, turn away from that side, otherwise pick a random
    // direction.  If a bump is registered, we will turn further than if the
    // road ahead seems clear.
    //
    if(!BumpSensorGetStatus(LEFT_SIDE))
    {
        bTurnLeft = false;
        bBump = true;
    }
    else
    {
        if(!BumpSensorGetStatus(RIGHT_SIDE))
        {
            bTurnLeft = true;
            bBump = true;
        }
        else
        {
            bTurnLeft = ((unsigned long)urand() > 0x80000000) ? true : false;
            bBump = false;
        }
    }


    //
    // Get a random number.
    //
    ulTime = (unsigned long)urand();

    //
    // Did we hit something?
    //
    if(bBump)
    {
        //
        // Yes - set a turning time that will give us a larger turn.  Scale the
        // random number into the range [0, 63] then add 80 so give us a turn
        // time between 80 and 143 ticks.
        //
        ulTime >>= 26;
        ulTime += 80;
    }
    else
    {
        //
        // We have not bumped into anything so scale the random turn time to
        // give is a smaller turn.  This calculation gives us a turn time of
        // between 20 and 52 ticks.
        //
        ulTime >>= 27;
        ulTime += 20;
    }

    //
    // Remember how long we are going to turn for.
    //
    pState->ulAutonomousSegmentTicks = ulTime;

    //
    // Set the motor directions appropriately.
    //
    MotorDir(LEFT_SIDE, bTurnLeft ? REVERSE : FORWARD);
    MotorDir(RIGHT_SIDE, bTurnLeft ? FORWARD : REVERSE);

#ifndef KEEP_MOTORS_DISABLED
    //
    // Turn both motors on.
    //
    MotorRun(LEFT_SIDE);
    MotorRun(RIGHT_SIDE);
#endif

    //
    // Remember when we made the last change under autonomous control.
    //
    pState->ulLastAutonomousChange = SchedulerTickCountGet();

    UARTprintf("%08x: Turning %s for %d ticks\n",
               pState->ulLastAutonomousChange, bTurnLeft ? "left" : "right",
               ulTime);
}

//*****************************************************************************
//
// This function is called to stop the EVALBOT.
//
//*****************************************************************************
static void
EvalBotStop(tStateVars *pState)
{
    unsigned long ulLoop;

    //
    // Stop the motors.
    //
    MotorStop(LEFT_SIDE);
    MotorStop(RIGHT_SIDE);

    //
    // Clear the flag we use to indicate we are reversing.
    //
    pState->bReversing = false;

    //
    // Clear the accelerometer readings.
    //
    for(ulLoop = 0; ulLoop < NUM_AXES; ulLoop++)
    {
        pState->sAccel[ulLoop] = 0;
    }

    //
    // Tell the scheduler to stop calling the speed update function.
    //
    SchedulerTaskDisable(TASK_UPDATE_SPEED);
}

//*****************************************************************************
//
// Main application entry point.
//
//*****************************************************************************
int
main (void)
{
    unsigned char ucPower;
    tBoolean bRetcode;

    //
    // Set the system clock to run at 50MHz from the PLL
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // Perform any necessary SimpliciTI BSP initialization.
    //
    BSP_Init();

    //
    // Enable UART0.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART standard I/O.
    //
    UARTStdioInit(0);

    //
    // Print a nice welcoming banner on the serial output.
    //
    UARTprintf("EVALBOT Remote Control\n");
    UARTprintf("----------------------\n");

    //
    // Initialize the LED display.
    //
    Display96x16x1Init(true);
    Display96x16x1Clear();

    //
    // Initialize the board LEDs.
    //
    LEDsInit();

    //
    // Initialize the motors.
    //
    MotorsInit();

    //
    // Initialize the front bump sensors.
    //
    BumpSensorsInit();

    //
    // Initialize the sound driver.
    //
    SoundInit();

    //
    // Set the default calibration values for the Chronos accelerometer control.
    //
    CalibrationDefaultsSet(&g_sStateInfo);

    //
    // Set our SimpliciTI device address using the Ethernet MAC address.
    //
    bRetcode = SetSimpliciTIAddress();

    //
    // Did we set the device address successfully?
    //
    if(!bRetcode)
    {
        //
        // No - display an error message.
        //
        Display96x16x1StringDrawCentered("No address set!", 1, true);

        //
        // Now hang solid.
        //
        while(1)
        {
        }
    }

    //
    // Initialize the SimpliciTI stack and register our receive callback.
    //
    SMPL_Init(ReceiveCallback);

    //
    // Set output power to +1.1dBm (868MHz) / +1.3dBm (915MHz)
    //
    ucPower = IOCTL_LEVEL_2;
    SMPL_Ioctl(IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SETPWR, &ucPower);

    //
    // Initialize the strings that we cycle onto the display.
    //
    g_sStateInfo.eState = EVALBOT_STARTUP;
    g_sStateInfo.ulLastRxTime = 0;
    usnprintf(g_pcString1, 16, "Listening");
    usnprintf(g_pcString2, 16, "No Chronos");

    //
    // Initialize the task scheduler.
    //
    SchedulerInit(TICKS_PER_SECOND);

    //
    // Turn on interrupts.
    //
    IntMasterEnable();

    //
    // Drop into the main loop.
    //
    while(1)
    {
        //
        // Tell the scheduler to call any periodic tasks that are due to be
        // called.
        //
        SchedulerRun();
    }
}
