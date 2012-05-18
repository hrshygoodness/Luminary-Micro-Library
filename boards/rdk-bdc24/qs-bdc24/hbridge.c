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
// This is part of revision 8555 of the RDK-BDC24 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "shared/can_proto.h"
#include "constants.h"
#include "controller.h"
#include "hbridge.h"
#include "limit.h"
#include "pins.h"

//*****************************************************************************
//
// Alias defines for the generators to relate them to the H-bridge hardware.
//
//*****************************************************************************
#define GEN_M_PLUS              PWM_GEN_0
#define GEN_M_MINUS             PWM_GEN_1
#define GEN_TIMING              PWM_GEN_2
#define GEN_M_PLUS_BIT          PWM_GEN_0_BIT
#define GEN_M_MINUS_BIT         PWM_GEN_1_BIT

//*****************************************************************************
//
// Alias defines for the comparator register offsets.
//
//*****************************************************************************
#define M_PLUS_CMP              PWM_O_0_CMPA
#define M_MINUS_CMP             PWM_O_1_CMPA
#define M_ADC_CMP               PWM_O_0_CMPB

//*****************************************************************************
//
// Alias defines for the generator register offset.
//
//*****************************************************************************
#define M_PLUS_GENH             PWM_O_0_GENA
#define M_PLUS_GENL             PWM_O_0_GENB
#define M_MINUS_GENH            PWM_O_1_GENA
#define M_MINUS_GENL            PWM_O_1_GENB

//*****************************************************************************
//
// The value to program into a PWMxGEN for the high side to make it be on all
// the time.  This must be coupled by setting the high side comparator to
// IGNORE (otherwise it will continue to generate pulses).
//
//*****************************************************************************
#define HIGH_SIDE_ON            (PWM_X_GENA_ACTZERO_ONE |                     \
                                 PWM_X_GENA_ACTCMPAU_INV |                    \
                                 PWM_X_GENA_ACTCMPAD_INV)

//*****************************************************************************
//
// The value to program into a PWMxGEN for the high side to make it produce
// pulses.  This must be coupled by setting the high side comparator to an
// appropriate value to control the width of the pulse.
//
//*****************************************************************************
#define HIGH_SIDE_PULSE         (PWM_X_GENA_ACTZERO_ZERO |                    \
                                 PWM_X_GENA_ACTCMPAU_INV |                    \
                                 PWM_X_GENA_ACTCMPAD_INV)

//*****************************************************************************
//
// The value to program into a PWMxGEN for the high side to make it be off all
// the time.  This must be coupled by setting the high side comparator to
// IGNORE (otherwise it will continue to generate pulses).
//
//*****************************************************************************
#define HIGH_SIDE_OFF           (PWM_X_GENA_ACTZERO_ZERO |                    \
                                 PWM_X_GENA_ACTCMPAU_INV |                    \
                                 PWM_X_GENA_ACTCMPAD_INV)

//*****************************************************************************
//
// The value to program into a PWMxGEN for the low side to make it be on all
// the time.  This must be coupled by setting the low side comparator to IGNORE
// (otherwise it will continue to generate pulses).
//
//*****************************************************************************
#define LOW_SIDE_ON             (PWM_X_GENA_ACTZERO_ONE |                     \
                                 PWM_X_GENA_ACTCMPAU_INV |                    \
                                 PWM_X_GENA_ACTCMPAD_INV)

//*****************************************************************************
//
// The value to program into a PWMxGEN for the low side to make it produce
// pulses.  This must be coupled by setting the low side comparator to an
// appropriate value to control the width of the pulse.
//
//*****************************************************************************
#define LOW_SIDE_PULSE          (PWM_X_GENA_ACTZERO_ONE |                     \
                                 PWM_X_GENA_ACTCMPAU_INV |                    \
                                 PWM_X_GENA_ACTCMPAD_INV)

//*****************************************************************************
//
// The value to program into a PWMxGEN for the low side to make it be off all
// the time.  This must be coupled by setting the low side comparator to IGNORE
// (otherwise it will continue to generate pulses).
//
//*****************************************************************************
#define LOW_SIDE_OFF            (PWM_X_GENA_ACTZERO_ZERO |                    \
                                 PWM_X_GENA_ACTCMPAU_INV |                    \
                                 PWM_X_GENA_ACTCMPAD_INV)

//*****************************************************************************
//
// The value to program into a comparator (high or low side) in order to make
// the comparator be ignored.  Since the comparators are ignored when they
// trigger at the same time as the zero event, they are set to zero.
//
//*****************************************************************************
#define IGNORE                  0

//*****************************************************************************
//
// The number of clocks prior to the high-side falling edge that the ADC is
// triggered.  This must be sufficiently large to allow the sample-and-hold
// circuit to capture the current reading prior to the injected switching noise
// at the falling edge.
//
//*****************************************************************************
#define ADC_SAMPLE_DELTA        16

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
// Determines if the drive is configured for brake or coast mode.
//
//*****************************************************************************
static unsigned long
HBridgeBrakeCoastMode(void)
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
        // The drive is configured for coast mode.
        //
        return(HBRIDGE_COAST);
    }
    else
    {
        //
        // The drive is configured for brake mode.
        //
        return(HBRIDGE_BRAKE);
    }
}

//*****************************************************************************
//
// Sets the H-bridge into either brake or coast mode, based on the current
// configuration.  Note that coast is also referred to as fast decay and brake
// is also referred to as slow decay.
//
//*****************************************************************************
static void
HBridgeBrakeCoast(unsigned long ulMode)
{
    //
    // The PWM comparison registers are not needed when braking or coasting, so
    // move them to a place where they will be ignored by the hardware.
    //
    HWREG(PWM0_BASE + M_PLUS_CMP) = IGNORE;
    HWREG(PWM0_BASE + M_MINUS_CMP) = IGNORE;
    HWREG(PWM0_BASE + M_ADC_CMP) = ADC_SAMPLE_DELTA;

    //
    // See if the drive is configured for braking or coasting.
    //
    if(ulMode == HBRIDGE_COAST)
    {
        //
        // Place the H-bridge into coast mode.
        //
        HWREG(PWM0_BASE + M_PLUS_GENH) = HIGH_SIDE_OFF;
        HWREG(PWM0_BASE + M_PLUS_GENL) = LOW_SIDE_OFF;
        HWREG(PWM0_BASE + M_MINUS_GENH) = HIGH_SIDE_OFF;
        HWREG(PWM0_BASE + M_MINUS_GENL) = LOW_SIDE_OFF;
    }
    else
    {
        //
        // Place the H-bridge into brake mode.
        //
        HWREG(PWM0_BASE + M_PLUS_GENH) = HIGH_SIDE_OFF;
        HWREG(PWM0_BASE + M_PLUS_GENL) = LOW_SIDE_ON;
        HWREG(PWM0_BASE + M_MINUS_GENH) = HIGH_SIDE_OFF;
        HWREG(PWM0_BASE + M_MINUS_GENL) = LOW_SIDE_ON;
    }
}

//*****************************************************************************
//
// Resets the gate driver, clearing any latched fault conditions.
//
//*****************************************************************************
void
HBridgeGateDriverReset(void)
{
    //
    // Drive the gate driver reset signal low.
    //
    ROM_GPIOPinWrite(GATE_RESET_PORT, GATE_RESET_PIN, 0);

    //
    // Delay for 1us.
    //
    SysCtlDelay(SYSCLK / (1000000 * 3));

    //
    // Drive the gate driver reset signal high.
    //
    ROM_GPIOPinWrite(GATE_RESET_PORT, GATE_RESET_PIN, GATE_RESET_PIN);
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
    ROM_GPIOPinTypePWM(HBRIDGE_AHI_PORT, HBRIDGE_AHI_PIN);
    ROM_GPIOPinTypePWM(HBRIDGE_ALO_PORT, HBRIDGE_ALO_PIN);
    ROM_GPIOPinTypePWM(HBRIDGE_BHI_PORT, HBRIDGE_BHI_PIN);
    ROM_GPIOPinTypePWM(HBRIDGE_BLO_PORT, HBRIDGE_BLO_PIN);

    //
    // Initialize the gate driver control signals.
    //
    ROM_GPIOPinTypeGPIOOutput(GATE_RESET_PORT, GATE_RESET_PIN);
    ROM_GPIOPinTypeGPIOInput(GATE_FAULT_PORT, GATE_FAULT_PIN);

    //
    // Reset the gate driver.
    //
    HBridgeGateDriverReset();

    //
    // Configure the PWM generators.
    //
    ROM_PWMGenConfigure(PWM0_BASE, GEN_M_PLUS,
                        (PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_SYNC |
                         PWM_GEN_MODE_DBG_STOP));
    ROM_PWMGenConfigure(PWM0_BASE, GEN_M_MINUS,
                        (PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_SYNC |
                         PWM_GEN_MODE_DBG_STOP));
    ROM_PWMGenConfigure(PWM0_BASE, GEN_TIMING,
                        PWM_GEN_MODE_DOWN | PWM_GEN_MODE_DBG_STOP);

    //
    // Set the counter period in each generator.
    //
    ROM_PWMGenPeriodSet(PWM0_BASE, GEN_M_PLUS, SYSCLK_PER_PWM_PERIOD);
    ROM_PWMGenPeriodSet(PWM0_BASE, GEN_M_MINUS, SYSCLK_PER_PWM_PERIOD);
    ROM_PWMGenPeriodSet(PWM0_BASE, GEN_TIMING, SYSCLK_PER_UPDATE);

    //
    // Set the default output.  The value for this depends on the brake/coast
    // setting.
    //
    HBridgeBrakeCoast(HBridgeBrakeCoastMode());

    //
    // Set the trigger enable for the M+ generator to initiate the ADC sample
    // sequence.  Set the interrupt enable for the timing generator.
    //
    ROM_PWMGenIntTrigEnable(PWM0_BASE, GEN_M_PLUS, PWM_TR_CNT_BD);
    ROM_PWMGenIntTrigEnable(PWM0_BASE, GEN_TIMING, PWM_INT_CNT_ZERO);

    //
    // Enable the PWM counters in the generators specified.
    //
    ROM_PWMGenEnable(PWM0_BASE, GEN_M_PLUS);
    ROM_PWMGenEnable(PWM0_BASE, GEN_M_MINUS);
    ROM_PWMGenEnable(PWM0_BASE, GEN_TIMING);

    //
    // Synchronize the counters in all generators.
    //
    ROM_PWMSyncTimeBase(PWM0_BASE, GEN_M_PLUS_BIT | GEN_M_MINUS_BIT);

    //
    // If the debugger stops the system, the PWM outputs should be shut down,
    // otherwise, they could be left in a full on state, which will result in
    // shoot-through.  Instead, the motor is put into coast mode while the
    // processor is halted by the debugger.
    //
    ROM_PWMOutputFault(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT |
                                   PWM_OUT_2_BIT | PWM_OUT_3_BIT), true);

    //
    // Force a global sync so that any pending updates to CMPA, GENA, or GENB
    // are handled.
    //
    ROM_PWMSyncUpdate(PWM0_BASE, GEN_M_PLUS_BIT | GEN_M_MINUS_BIT);

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
    unsigned long ulOn, ulMode;

    //
    // See if there is a fault condition indicated by the gate driver.
    //
    if(ROM_GPIOPinRead(GATE_FAULT_PORT, GATE_FAULT_PIN) == 0)
    {
        //
        // Indicate that there is a gate driver fault.
        //
        ControllerFaultSignal(LM_FAULT_GATE_DRIVE);
    }

    //
    // Determine if the drive is in brake or coast mode.
    //
    ulMode = HBridgeBrakeCoastMode();

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
        HBridgeBrakeCoast(ulMode);
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
            // Compute the number of clocks that the high side MOSFET should be
            // turned on.
            //
            ulOn = (((((0 - g_lHBridgeV) * g_lHBridgeVMax) / 32767) *
                     SYSCLK_PER_PWM_PERIOD) / 32767);

            //
            // See if the high side MOSFET should be on for most of the PWM
            // period.
            //
            if(ulOn >= (SYSCLK_PER_PWM_PERIOD - PWM_MIN_WIDTH))
            {
                //
                // No switching is required; simply leave the low side of the
                // positive terminal and the high side of the negative terminal
                // on the entire time.
                //
                HWREG(PWM0_BASE + M_PLUS_CMP) = IGNORE;
                HWREG(PWM0_BASE + M_MINUS_CMP) = IGNORE;
                HWREG(PWM0_BASE + M_ADC_CMP) = ADC_SAMPLE_DELTA;
                HWREG(PWM0_BASE + M_PLUS_GENH) = HIGH_SIDE_OFF;
                HWREG(PWM0_BASE + M_PLUS_GENL) = LOW_SIDE_ON;
                HWREG(PWM0_BASE + M_MINUS_GENH) = HIGH_SIDE_ON;
                HWREG(PWM0_BASE + M_MINUS_GENL) = LOW_SIDE_OFF;
            }

            //
            // Otherwise, see if the high side MOSFET should be off for most of
            // the PWM period.
            //
            else if(ulOn < PWM_MIN_WIDTH)
            {
                //
                // No switching is required; simply put the H-bridge into brake
                // or coast mode as configured.
                //
                HBridgeBrakeCoast(ulMode);
            }

            //
            // Otherwise, the output should be on only part of the time.
            //
            else
            {
                //
                // Turn on the PWM high side output for the specified time
                // and the low side output on for the remaining time.
                //
                HWREG(PWM0_BASE + M_PLUS_CMP) = IGNORE;
                HWREG(PWM0_BASE + M_MINUS_CMP) =
                    (SYSCLK_PER_PWM_PERIOD - ulOn) / 2;
                HWREG(PWM0_BASE + M_ADC_CMP) =
                    ((SYSCLK_PER_PWM_PERIOD - ulOn) / 2) + ADC_SAMPLE_DELTA;
                HWREG(PWM0_BASE + M_PLUS_GENH) = HIGH_SIDE_OFF;
                HWREG(PWM0_BASE + M_PLUS_GENL) = LOW_SIDE_ON;
                HWREG(PWM0_BASE + M_MINUS_GENH) = HIGH_SIDE_PULSE;
                HWREG(PWM0_BASE + M_MINUS_GENL) = LOW_SIDE_PULSE;
            }
        }
        else
        {
            //
            // The limit switches do not allow the motor to be driven in
            // reverse, so set the H-bridge to brake or coast mode.
            //
            HBridgeBrakeCoast(ulMode);
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
            // Compute the number of clocks that the high side MOSFET should be
            // turned on.
            //
            ulOn = ((((g_lHBridgeV * g_lHBridgeVMax) / 32767) *
                     SYSCLK_PER_PWM_PERIOD) / 32767);

            //
            // See if the high side MOSFET should be on for most of the PWM
            // period.
            //
            if(ulOn >= (SYSCLK_PER_PWM_PERIOD - PWM_MIN_WIDTH))
            {
                //
                // No switching is required; simply leave the high side of the
                // positive terminal and the low side of the negative terminal
                // on the entire time.
                //
                HWREG(PWM0_BASE + M_PLUS_CMP) = IGNORE;
                HWREG(PWM0_BASE + M_MINUS_CMP) = IGNORE;
                HWREG(PWM0_BASE + M_ADC_CMP) = ADC_SAMPLE_DELTA;
                HWREG(PWM0_BASE + M_PLUS_GENH) = HIGH_SIDE_ON;
                HWREG(PWM0_BASE + M_PLUS_GENL) = LOW_SIDE_OFF;
                HWREG(PWM0_BASE + M_MINUS_GENH) = HIGH_SIDE_OFF;
                HWREG(PWM0_BASE + M_MINUS_GENL) = LOW_SIDE_ON;
            }

            //
            // Otherwise, see if the high side MOSFET should be off for most of
            // the PWM period.
            //
            else if(ulOn < PWM_MIN_WIDTH)
            {
                //
                // No switching is required; simply put the H-bridge into brake
                // or coast mode as configured.
                //
                HBridgeBrakeCoast(ulMode);
            }

            //
            // Otherwise, the output should be on only part of the time.
            //
            else
            {
                //
                // Turn on the PWM high side output for the specified time
                // and the low side output on for the remaining time.
                //
                HWREG(PWM0_BASE + M_PLUS_CMP) =
                    (SYSCLK_PER_PWM_PERIOD - ulOn) / 2;
                HWREG(PWM0_BASE + M_MINUS_CMP) = IGNORE;
                HWREG(PWM0_BASE + M_ADC_CMP) =
                    ((SYSCLK_PER_PWM_PERIOD - ulOn) / 2) + ADC_SAMPLE_DELTA;
                HWREG(PWM0_BASE + M_PLUS_GENH) = HIGH_SIDE_PULSE;
                HWREG(PWM0_BASE + M_PLUS_GENL) = LOW_SIDE_PULSE;
                HWREG(PWM0_BASE + M_MINUS_GENH) = HIGH_SIDE_OFF;
                HWREG(PWM0_BASE + M_MINUS_GENL) = LOW_SIDE_ON;
            }
        }
        else
        {
            //
            // The limit switches do not allow the motor to be driven forward,
            // so set the H-bridge to brake or coast mode.
            //
            HBridgeBrakeCoast(ulMode);
        }
    }

    //
    // Force a global sync so that any pending updates to CMPA, GENA, or GENB
    // are handled.
    //
    ROM_PWMSyncUpdate(PWM0_BASE, GEN_M_PLUS_BIT | GEN_M_MINUS_BIT);
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
    HBridgeBrakeCoast(HBridgeBrakeCoastMode());

    //
    // Force a global sync so that any pending updates to CMPA, GENA, or GENB
    // are handled.
    //
    ROM_PWMSyncUpdate(PWM0_BASE, GEN_M_PLUS_BIT | GEN_M_MINUS_BIT);
}
