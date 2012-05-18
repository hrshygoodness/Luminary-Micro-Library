//*****************************************************************************
//
// hbridge.c - Driver for the H-bridge.
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
// This is part of revision 8555 of the RDK-BDC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "constants.h"
#include "hbridge.h"
#include "limit.h"
#include "pins.h"

//*****************************************************************************
//
// Alias defines for the generators to relate them to the H-bridge hardware.
//
//*****************************************************************************
#define GEN_M_MINUS             PWM_GEN_0
#define GEN_M_PLUS              PWM_GEN_1
#define GEN_TIMING              PWM_GEN_2
#define GEN_M_MINUS_BIT         PWM_GEN_0_BIT
#define GEN_M_PLUS_BIT          PWM_GEN_1_BIT

//*****************************************************************************
//
// Alias defines for the comparator register offsets.
//
//*****************************************************************************
#define M_MINUS_CMP             PWM_O_0_CMPA
#define M_PLUS_CMP              PWM_O_1_CMPA
#define ADC_CMP                 PWM_O_0_CMPB

//*****************************************************************************
//
// Alias defines for the generator register offset.
//
//*****************************************************************************
#define M_MINUS_CTRL_GEN        PWM_O_0_GENB
#define M_MINUS_PWM_GEN         PWM_O_0_GENA
#define M_PLUS_CTRL_GEN         PWM_O_1_GENB
#define M_PLUS_PWM_GEN          PWM_O_1_GENA

//*****************************************************************************
//
// The PWMxGENy value that results in the output signal being a PWM pulse
// stream.  This is used for the CTRL input to the gate drivers.
//
//*****************************************************************************
#define PULSE                   (PWM_X_GENA_ACTLOAD_ONE |                     \
                                 PWM_X_GENA_ACTCMPAD_ZERO)

//*****************************************************************************
//
// The PWMxGENy value that results in the output signal being off all the time.
// This is used for the CTRL input to the gate drivers.
//
//*****************************************************************************
#define OFF                     (PWM_X_GENA_ACTZERO_ZERO |                    \
                                 PWM_X_GENA_ACTLOAD_ZERO |                    \
                                 PWM_X_GENA_ACTCMPAU_ZERO |                   \
                                 PWM_X_GENA_ACTCMPAD_ZERO |                   \
                                 PWM_X_GENA_ACTCMPBU_ZERO |                   \
                                 PWM_X_GENA_ACTCMPBD_ZERO)

//*****************************************************************************
//
// The PWMxGENy value that results in the output signal being on all the time.
// This is used for the CTRL input to the gate drivers.
//
//*****************************************************************************
#define ON                      (PWM_X_GENA_ACTZERO_ONE |                     \
                                 PWM_X_GENA_ACTLOAD_ONE |                     \
                                 PWM_X_GENA_ACTCMPAU_ONE |                    \
                                 PWM_X_GENA_ACTCMPAD_ONE |                    \
                                 PWM_X_GENA_ACTCMPBU_ONE |                    \
                                 PWM_X_GENA_ACTCMPBD_ONE)

//*****************************************************************************
//
// The PWMxGENy value that results in the output signal being low all the time.
// This is used for the PWM input to the gate drivers.
//
//*****************************************************************************
#define LO                      (PWM_X_GENB_ACTZERO_ZERO |                    \
                                 PWM_X_GENB_ACTLOAD_ZERO |                    \
                                 PWM_X_GENB_ACTCMPAU_ZERO |                   \
                                 PWM_X_GENB_ACTCMPAD_ZERO |                   \
                                 PWM_X_GENB_ACTCMPBU_ZERO |                   \
                                 PWM_X_GENB_ACTCMPBD_ZERO)

//*****************************************************************************
//
// The PWMxGENy value that results in the output signal being high all the
// time.  This is used for the PWM input to the gate drivers.
//
//*****************************************************************************
#define HI                      (PWM_X_GENB_ACTZERO_ONE |                     \
                                 PWM_X_GENB_ACTLOAD_ONE |                     \
                                 PWM_X_GENB_ACTCMPAU_ONE |                    \
                                 PWM_X_GENB_ACTCMPAD_ONE |                    \
                                 PWM_X_GENB_ACTCMPBU_ONE |                    \
                                 PWM_X_GENB_ACTCMPBD_ONE)

//*****************************************************************************
//
// The current output voltage to the H-bridge.  The volatile is not strictly
// required but is used in EW-ARM to workaround some versions of the compiler
// trying to be too clever (resulting in incorrectly generated machine code).
//
//*****************************************************************************
#ifdef ewarm
static volatile long g_lHBridgeV = 0;
#else
static long g_lHBridgeV = 0;
#endif

//*****************************************************************************
//
// The maximum output voltage to the H-bridge.
//
//*****************************************************************************
static long g_lHBridgeVMax = 32767;

//*****************************************************************************
//
// The configuration of the brake/coast setting.  By default, the state of the
// jumper is used to determine brake/coast.
//
//*****************************************************************************
static unsigned long g_ulHBridgeBrakeCoast = HBRIDGE_JUMPER;

//*****************************************************************************
//
// Sets the H-bridge into either brake or coast mode, based on the current
// configuration.  Note that coast is also referred to as fast decay and brake
// is also referred to as slow decay.
//
//*****************************************************************************
static void
HBridgeBrakeCoast(void)
{
    //
    // See if the drive is configured for braking or coasting.
    //
    if((g_ulHBridgeBrakeCoast == HBRIDGE_COAST) ||
       ((g_ulHBridgeBrakeCoast == HBRIDGE_JUMPER) &&
        (ROM_GPIOPinRead(BRAKECOAST_PORT, BRAKECOAST_PIN) ==
         BRAKECOAST_COAST)))
    {
        //
        // Place the H-bridge into coast mode.
        //
        HWREG(PWM0_BASE + M_MINUS_CTRL_GEN) = LO;
        HWREG(PWM0_BASE + M_MINUS_PWM_GEN) = OFF;
        HWREG(PWM0_BASE + M_PLUS_CTRL_GEN) = LO;
        HWREG(PWM0_BASE + M_PLUS_PWM_GEN) = OFF;
    }
    else
    {
        //
        // Place the H-bridge into brake mode.
        //
        HWREG(PWM0_BASE + M_MINUS_CTRL_GEN) = LO;
        HWREG(PWM0_BASE + M_MINUS_PWM_GEN) = ON;
        HWREG(PWM0_BASE + M_PLUS_CTRL_GEN) = LO;
        HWREG(PWM0_BASE + M_PLUS_PWM_GEN) = ON;
    }
}

//*****************************************************************************
//
// Initializes the H-Brige interface.
//
//*****************************************************************************
void
HBridgeInit(void)
{
    //
    // Initialize the brake/coast port.  Enable the weak pull down to default
    // to brake if the jumper is not installed.
    //
    ROM_GPIODirModeSet(BRAKECOAST_PORT, BRAKECOAST_PIN, GPIO_DIR_MODE_IN);
    ROM_GPIOPadConfigSet(BRAKECOAST_PORT, BRAKECOAST_PIN, GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPD);

    //
    // Initialize the H-bridge PWM outputs.
    //
    ROM_GPIOPinTypePWM(HBRIDGE_PWMA_PORT, HBRIDGE_PWMA_PIN);
    ROM_GPIOPinTypePWM(HBRIDGE_CTRLA_PORT, HBRIDGE_CTRLA_PIN);
    ROM_GPIOPinTypePWM(HBRIDGE_PWMB_PORT, HBRIDGE_PWMB_PIN);
    ROM_GPIOPinTypePWM(HBRIDGE_CTRLB_PORT, HBRIDGE_CTRLB_PIN);

    //
    // Configure the PWM generators.  The ROM version is not used here since it
    // does not handle the generator sync mode.
    //
    PWMGenConfigure(PWM0_BASE, GEN_M_MINUS,
                    (PWM_GEN_MODE_DOWN | PWM_GEN_MODE_SYNC |
                     PWM_GEN_MODE_DBG_STOP | PWM_GEN_MODE_GEN_SYNC_GLOBAL));
    PWMGenConfigure(PWM0_BASE, GEN_M_PLUS,
                    (PWM_GEN_MODE_DOWN | PWM_GEN_MODE_SYNC |
                     PWM_GEN_MODE_DBG_STOP | PWM_GEN_MODE_GEN_SYNC_GLOBAL));
    PWMGenConfigure(PWM0_BASE, GEN_TIMING,
                    PWM_GEN_MODE_DOWN | PWM_GEN_MODE_DBG_STOP);

    //
    // Set the counter period in each generator.
    //
    ROM_PWMGenPeriodSet(PWM0_BASE, GEN_M_MINUS, SYSCLK_PER_PWM_PERIOD);
    ROM_PWMGenPeriodSet(PWM0_BASE, GEN_M_PLUS, SYSCLK_PER_PWM_PERIOD);
    ROM_PWMGenPeriodSet(PWM0_BASE, GEN_TIMING, SYSCLK_PER_UPDATE);

    //
    // Set the default output.  The value for this depends on the brake/coast
    // setting.
    //
    HBridgeBrakeCoast();

    //
    // Configure the timing to interrupt midway of the cycle so that there is
    // time before the initial interrupt
    //
    HWREG(PWM0_BASE + ADC_CMP) = SYSCLK_PER_PWM_PERIOD / 2;

    //
    // Set the trigger enable for the M- generator to initiate the ADC sample
    // sequence.  Set the interrupt enable for the timing generator.
    //
    ROM_PWMGenIntTrigEnable(PWM0_BASE, GEN_M_MINUS, PWM_TR_CNT_BD);
    ROM_PWMGenIntTrigEnable(PWM0_BASE, GEN_TIMING, PWM_INT_CNT_ZERO);

    //
    // Synchronize the counters in all generators.
    //
    ROM_PWMSyncTimeBase(PWM0_BASE, GEN_M_MINUS_BIT | GEN_M_PLUS_BIT);

    //
    // If the debugger stops the system, the PWM outputs should be shut down,
    // otherwise, they could be left in a full on state, which might be
    // dangerous (depending upon what is connected to the motor).  Instead, the
    // motor is put into coast mode while the processor is halted by the
    // debugger.
    //
    ROM_PWMOutputFault(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT |
                                   PWM_OUT_2_BIT | PWM_OUT_3_BIT), true);

    //
    // Enable the PWM counters in the generators specified.
    //
    ROM_PWMGenEnable(PWM0_BASE, GEN_M_MINUS);
    ROM_PWMGenEnable(PWM0_BASE, GEN_M_PLUS);
    ROM_PWMGenEnable(PWM0_BASE, GEN_TIMING);

    //
    // Force a global sync so that any pending updates to CMPA, GENA, or GENB
    // are handled.
    //
    ROM_PWMSyncUpdate(PWM0_BASE, GEN_M_MINUS_BIT | GEN_M_PLUS_BIT);

    //
    // Enable the output signals of the PWM unit.
    //
    ROM_PWMOutputState(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT |
                                   PWM_OUT_2_BIT | PWM_OUT_3_BIT), true);

    //
    // Enable the timing interrupt.
    //
    ROM_PWMIntEnable(PWM0_BASE, PWM_GEN_2_BIT);
    ROM_IntEnable(INT_PWM0_2);
}

//*****************************************************************************
//
// Sets the maximum H-bridge output voltage.  This is used to scale the
// requested voltage.
//
//*****************************************************************************
void
HBridgeVoltageMaxSet(long lVoltage)
{
    //
    // Save the new maximum output voltage.  This will be applied to the
    // H-bridge on the next H-bridge tick.
    //
    g_lHBridgeVMax = (lVoltage * 32767) / (12 * 256);
}

//*****************************************************************************
//
// Gets the maximum H-bridge output voltage.
//
//*****************************************************************************
long
HBridgeVoltageMaxGet(void)
{
    //
    // Return the maximum output voltage.
    //
    return((g_lHBridgeVMax * 12 * 256) / 32767);
}

//*****************************************************************************
//
// Sets the H-bridge output voltage.
//
//*****************************************************************************
void
HBridgeVoltageSet(long lVoltage)
{
    //
    // Save the new output voltage.  This will be applied to the H-bridge on
    // the next H-bridge tick.
    //
    g_lHBridgeV = lVoltage;
}

//*****************************************************************************
//
// Sets the H-bridge brake/coast configuration.
//
//*****************************************************************************
void
HBridgeBrakeCoastSet(unsigned long ulState)
{
    //
    // Save the new brake/coast configuration.  This will be applied on the
    // next H-bridge tick in which the motor is in neutral.
    //
    g_ulHBridgeBrakeCoast = ulState;
}

//*****************************************************************************
//
// Gets the H-bridge brake/coast configuration.
//
//*****************************************************************************
unsigned long
HBridgeBrakeCoastGet(void)
{
    //
    // Return the brake/coast configuration.
    //
    return(g_ulHBridgeBrakeCoast);
}

//*****************************************************************************
//
// Handles the periodic update to the H-bridge output.
//
//*****************************************************************************
void
HBridgeTick(void)
{
    unsigned long ulCompare;

    //
    // See if the output voltage should be zero (in other words, neutral).
    //
    if(g_lHBridgeV == 0)
    {
        //
        // Set the H-bridge to brake or coast mode.  This is done every
        // interrupt to handle the cases where the jumper is being used and it
        // is being driven by an external source (such as another
        // microcontroller).
        //
        HBridgeBrakeCoast();
    }

    //
    // See if the output voltage is negative (in other words, reverse).
    //
    else if(g_lHBridgeV < 0)
    {
        //
        // Make sure that the limit switches allow the motor to run in reverse.
        //
        if(LimitReverseOK())
        {
            //
            // Compute the PWM compare value from the desired output voltage.
            // Limit the compare value such that the output pulse doesn't
            // become too short or too long.
            //
            ulCompare = (((((g_lHBridgeV * g_lHBridgeVMax) / 32767) + 32768) *
                          SYSCLK_PER_PWM_PERIOD) / 32768);
            if(ulCompare < 24)
            {
                ulCompare = 24;
            }
            if(ulCompare > (SYSCLK_PER_PWM_PERIOD - 24))
            {
                ulCompare = SYSCLK_PER_PWM_PERIOD - 24;
            }

            //
            // Update the compare registers with the encoded pulse width.
            //
            HWREG(PWM0_BASE + M_MINUS_CMP) = ulCompare;
            HWREG(PWM0_BASE + M_PLUS_CMP) = ulCompare;
            HWREG(PWM0_BASE + ADC_CMP) =
                (SYSCLK_PER_PWM_PERIOD + ulCompare) / 2;

            //
            // Update the generator registers with the required drive pattern.
            //
            HWREG(PWM0_BASE + M_MINUS_CTRL_GEN) = HI;
            HWREG(PWM0_BASE + M_MINUS_PWM_GEN) = PULSE;
            HWREG(PWM0_BASE + M_PLUS_CTRL_GEN) = LO;
            HWREG(PWM0_BASE + M_PLUS_PWM_GEN) = ON;
        }
        else
        {
            //
            // The limit switches do not allow the motor to be driven in
            // reverse, so set the H-bridge to brake or coast mode.
            //
            HBridgeBrakeCoast();
        }
    }

    //
    // Otherwise, the output voltage is positive (in other words, forward).
    //
    else
    {
        //
        // Make sure that the limit switches allow the motor to run forward.
        //
        if(LimitForwardOK())
        {
            //
            // Compute the PWM compare value from the desired output voltage.
            // Limit the compare value such that the output pulse doesn't
            // become too short or too long.
            //
            ulCompare = (((32767 - ((g_lHBridgeV * g_lHBridgeVMax) / 32767)) *
                          SYSCLK_PER_PWM_PERIOD) / 32767);
            if(ulCompare < 24)
            {
                ulCompare = 24;
            }
            if(ulCompare > (SYSCLK_PER_PWM_PERIOD - 24))
            {
                ulCompare = SYSCLK_PER_PWM_PERIOD - 24;
            }

            //
            // Update the compare registers with the encoded pulse width.
            //
            HWREG(PWM0_BASE + M_MINUS_CMP) = ulCompare;
            HWREG(PWM0_BASE + M_PLUS_CMP) = ulCompare;
            HWREG(PWM0_BASE + ADC_CMP) =
                (SYSCLK_PER_PWM_PERIOD + ulCompare) / 2;

            //
            // Update the generator registers with the required drive pattern.
            //
            HWREG(PWM0_BASE + M_MINUS_CTRL_GEN) = LO;
            HWREG(PWM0_BASE + M_MINUS_PWM_GEN) = ON;
            HWREG(PWM0_BASE + M_PLUS_CTRL_GEN) = HI;
            HWREG(PWM0_BASE + M_PLUS_PWM_GEN) = PULSE;
        }
        else
        {
            //
            // The limit switches do not allow the motor to be driven forward,
            // so set the H-bridge to brake or coast mode.
            //
            HBridgeBrakeCoast();
        }
    }

    //
    // Force a global sync so that any pending updates to CMPA, GENA, or GENB
    // are handled.
    //
    ROM_PWMSyncUpdate(PWM0_BASE, GEN_M_MINUS_BIT | GEN_M_PLUS_BIT);
}

//*****************************************************************************
//
// Immediately places the H-bridge into neutral in preparation for a firmware
// update.
//
//*****************************************************************************
void
HBridgeFirmwareUpdate(void)
{
    //
    // Set the H-bridge to brake or coast mode.
    //
    HBridgeBrakeCoast();

    //
    // Force a global sync so that any pending updates to CMPA, GENA, or GENB
    // are handled.
    //
    ROM_PWMSyncUpdate(PWM0_BASE, GEN_M_MINUS_BIT | GEN_M_PLUS_BIT);
}
