//*****************************************************************************
//
// stepctrl.c - Control signals applied to step the motor
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-STEPPER Firmware Package.
//
//*****************************************************************************

#include "inc/hw_adc.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "stepcfg.h"

//*****************************************************************************
//
//! \page stepctrl_intro Introduction
//!
//! The Step Control module is used for controlling the drive signals to the
//! stepper motor. The functions in this module are called by the Step
//! Sequencer in order to set the control pins to the specific values needed
//! to attain the correct position in the step sequence.
//!
//! Normally there is no reason that an application needs to call any of
//! these functions directly, nor make direct access to any of the module
//! global variables. The following explains how this module is used by
//! the Step Sequencer module.
//!
//! First, the module is initialized by calling StepCtrlInit(). Then,
//! open or closed-loop PWM mode, or Chopper mode is selected by calling
//! StepCtrlOpenPWMMode(), StepCtrlClosedPWMMode() or StepCtrlChopMode().
//! These functions should be called whenever the control mode is changed,
//! when the motor is stopped. The module is not designed to handle control
//! mode changes when the motor is running.
//!
//! Functions are provided to set a specific winding to be controlled
//! in open-loop PWM, closed-loop PWM or chopper mode, fast or slow current
//! decay, and using a specific control value. These functions are
//! StepCtrlChopSlow(), StepCtrlChopFast(), StepCtrlOpenPwmSlow(),
//! StepCtrlOpenPwmFast(), StepCtrlClosedPwmSlow() and StepCtrlClosedPwmFast().
//! If open-loop PWM mode is used, then the control value is the amount of
//! time the winding signal should be turned on (determining PWM duty cycle).
//! If closed-loop PWM or chopper mode is used, then the control value is the
//! current threshold that should be used for chopping.
//!
//! Finally, two interrupt handlers are provided. There is a timer interrupt
//! handler, StepCtrlTimerHandler().  This is used for measuring either the
//! fixed on time for open-loop PWM mode, or the blanking off time for
//! chopper mode.  There is also an interrupt handler for ADC conversion:
//! StepCtrlADCHandler(). This interrupt handler is invoked whenever an ADC
//! conversion is completed. This handler is used in chopper mode to measure
//! the winding current, and decide whether to turn the winding off (chopping).
//! It is also used in closed-loop PWM mode to measure the winding current
//! and adjust the PWM duty cycle to maintain the proper current in the
//! winding.
//!
//! <b>Optimizations</b>
//!
//! An optimization that has been made is that direct register
//! accesses are made to the peripherals in the interrupt handlers, instead
//! of making calls to the DriverLib peripheral library. The HWREG macro
//! is used to make the register accesses. This too provides somewhat more
//! efficient code than making a function call, at the possible expense of
//! a slight increase in code size. Wherever a register access is made like
//! this, there is a comment in the code showing the equivalent DriverLib call.
//!
//! It is likely that an actual stepper application will use either chopper
//! or one of the PWM control modes, but not all three. In this case, if
//! desired, the user could modify this module to remove the code that
//! implements the unused control methods. To do this, the functions that
//! implement the unused method could be deleted, and the places in the code
//! where there is a run-time  conditional based on the control method can
//! be changed to eliminate the unused branch. These changes would make the
//! resulting code somewhat smaller, and more efficient to execute.
//!
//! <b>Motor Control Circuit</b>
//!
//! The microcontroller can control the current in the motor windings through
//! the use of three control signals for each winding. There is one signal
//! for each side of the H-bridge, designated P and N, and there is an
//! enable signal. (Note that on the board schematic, the two sides of
//! the H-bridge are designated 1 and 2).
//!
//! The diagram below shows a simplified representation of the H-bridge
//! circuit. Each side has two switches to control whether the voltage applied
//! to that side of the winding is the bus voltage (+V) or ground (GND). The
//! polarity of the motor winding is designated so that the P side of the
//! H-bridge is considered positive. Therefore, when the P high-side switch
//! (Ph) is closed, and the N low-side switch is closed (Nl), the bus voltage
//! will be connected on the positive side of the winding, and ground connected
//! on the negative side of the winding. Thus, positive bus voltage is
//! applied to the winding, and the current will flow in the positive
//! direction. Likewise, if the P low-side switch (Pl), and N high-side (Nh)
//! are closed, then negative voltage is applied to the winding and current
//! will flow in the negative direction.
//! \verbatim
//!                                                   ^ +V
//!                                    H-Bridge       |
//!                                 Circuit Diagram   |
//!  H-Bridge Switching Table                 +-------+-------+
//! +-------------------------+               |               |
//! |EN P N | Ph Pl Nh Nl | V |        Ph----\                 /----Nh
//! |-------+-----------------|               \               /
//! | 0 0 0 |             | 0 |               |    WINDING    |
//! | 1 0 0 |    X     X  | 0 |               +----/\/\/\/----+
//! | 1 0 1 |    X  X     | - |               |    +     -    |
//! | 1 1 0 | X        X  | + |        Pl----\                 /----Nl
//! | 1 1 1 |   ---N/A---     |               \               /
//! +-------------------------+               |               |
//!                                           +-------+-------+
//!                                                   |
//!                                                   |
//!                                                  --- GND
//!                                                   -
//! \endverbatim
//! The control circuit is designed so that both the high- and low-side
//! switches cannot be closed at the same time. Otherwise, it would be
//! possible to short out the power supply.
//!
//! When the enable signal is off, all the switches are open. When the
//! enable signal is on, either the high- or low-side switch will be on,
//! depending on the state of the control signal. The Switching Table above
//! shows the three control signals available for each winding, their
//! possible states, and the resulting connections of the H-bridge switches.
//! An "X" in the table means that switch is closed. The V column shows the
//! voltage applied to the winding: none (0), positive (+), or negative (-).
//!
//! The following table shows how the pins of the microcontroller are
//! assigned to the motor control signals. Each of the control signals
//! can be controlled by a PWM generator. There are two signals per
//! PWM generator, with each PWM generator having an "A" and a "B" half.
//! The "A" and "B" halves of the PWM generator should not be confused with
//! the fact that the two windings are called A and B.
//! For each winding, the two switching signals are controlled from a single
//! PWM generator. The "A" half of the generator controls the P side of the
//! winding and the "B" half of the generator controls the N side.
//! For the enable signals, there is one PWM generator that controls the
//! two signals, one for each winding. In this case, the A half of the
//! PWM generator controls the enable signal for the A winding, and the B
//! half of the PWM generator controls the B winding. In the table below,
//! the control signals are shown for each winding, associated with the
//! PWM outputs. The values shown in parentheses indicate which PWM
//! generator controls that PWM output.
//!
//! \verbatim
//!    H-Bridge Control Pin Assignments
//! +------------------------------------+
//! |        |     Winding               |
//! |--------+---------------------------|
//! |  Ctrl  |      A      |      B      |
//! |--------+---------------------------|
//! | P-side | D0/PWM0(0A) | B0/PWM2(1A) |
//! | N-side | D1/PWM1(0B) | B1/PWM3(1B) |
//! | Enable | E0/PWM4(2A) | E1/PWM5(2B) |
//! +------------------------------------+
//! \endverbatim
//!
//! <b>Current Decay Modes</b>
//!
//! In order to stop the current in a winding, the voltage must be removed.
//! There are two ways to do this. First, if both sides of the H-bridge are
//! set the same way, so that both low-side switches are closed, then both
//! sides of the winding are connected to ground. Since the motor winding
//! is inductive, if there was already current flowing in the winding, it
//! will continue to circulate in the winding and decay slowly. This is
//! called slow decay mode.
//!
//! On the other hand, if all of the switches are opened (by turning the
//! enable signal off), then there is also no voltage applied to the
//! winding, except that now neither side of the winding is connected to
//! anything. In this case, the current cannot continue to circulate and
//! decays rapidly. This is called fast decay mode.
//!
//! <b>Using PWM Outputs as GPIOs</b>
//!
//! The motor control circuits are driven by the microcontroller's PWM
//! outputs. The PWM outputs are used essentially as if they were GPIO
//! outputs when a control signal needs to be set to the on or off value.
//! This is done by setting a very short PWM period, and
//! then programming the PWM generator to drive the output low or high for
//! all events. The reason this is done is because the PWM generator outputs
//! can be programmed to be automatically placed in a safe state if the
//! hardware fault pin is asserted. So, if the hardware fault pin is used,
//! then the control pins to the motor drive circuitry will automatically
//! go into the safe state, which is to disable the gate drivers and open
//! all the H-bridge switches.
//!
//! The second reason that the PWM generators are used this way is because
//! there are some places in the code where a certain control signal
//! needs to be changed to be high or low. To accomplish this, the interrupt
//! handler code just writes a pre-computed value to the PWM generator control
//! register. It doesn't need to know if the output is being used for actual
//! PWM, or just as a GPIO in chopper mode. Otherwise, more run-time
//! conditionals would be needed to change the pin configurations between
//! PWM and GPIO depending on whether PWM was needed for a pin.
//!
//! <b>Chopper Operation</b>
//!
//! The chopper works by using the ADC to measure the winding current when
//! the winding has voltage applied, and turning off the voltage to the
//! winding when the current goes above the target current threshold. The
//! control signal is left off for the off blanking time before being turned
//! on again.
//!
//! Chopping control of a winding is started when one of the functions
//! StepCtrlChopSlow() or StepCtrlChopFast() is called. These functions
//! will set the gate driver control signals so that the voltage
//! is applied to the winding in the correct direction and start an
//! A/D conversion. This will cause current to begin to flow in the
//! winding, and when the A/D conversion is complete it will cause an
//! interrupt.
//!
//! The interrupt handler for the ADC looks at the measured current. If the
//! current is below the threshold, it leaves the winding on and starts another
//! conversion. If the current is above the threshold it turns the control
//! signal to the winding off, and starts up a timer with a timeout of the
//! off blanking time.
//!
//! When the timer expires, a timer interrupt is generated. The interrupt
//! handler for the timer turns the winding back on and starts up an A/D
//! conversion, and the whole chopping cycle repeats. This continues until
//! the next step requires a change of current in the winding.
//!
//! <b>Open-loop PWM Operation</b>
//!
//! The current in the winding can also be controlled using PWM instead of
//! by measuring the current with the chopper. The PWM method works by
//! turning the winding on for a fixed amount of time, called the
//! "fixed rise time". This time allows the current to rise rapidly
//! in the winding before PWM is started.
//!
//! PWM control of a winding is started when one of the StepCtrlOpenPwmSlow()
//! or StepCtrlOpenPwmFast() functions is called. These functions
//! will set the gate driver control signals so that the voltage is applied
//! to the winding in the correct direction. Then the timer is started with
//! a timeout of the fixed rise time.
//!
//! When the timer expires, a timer interrupt is generated. The interrupt
//! handler for the timer programs the PWM generator control register to
//! change the control pin from being on, to being controlled by the
//! PWM duty cycle. In this way, the pin starts switching on and off
//! as determined by the PWM frequency and duty cycle. This continues until
//! the next step requires a change of current in the winding.
//!
//! <b>Closed-loop PWM Operation</b>
//!
//! There is another PWM mode available called closed-loop PWM.  The "closed"
//! part refers to the fact that measured current feedback is used to
//! determine the PWM duty cycle.
//!
//! This method works by synchronizing the ADC acquisition with the PWM
//! pulse so that the ADC acquisition is started when the pulse is on.
//! When the acquisition is complete the ADC interrupt is triggered.  In
//! the ADC handler, the current measurement is taken from the ADC and
//! the PWM duty cycle is recalculated based on the measured current.  If
//! the current is too low, then the duty cycle is increased.  If the current
//! approaches the target level, then the PWM duty cycle is decreased.  If
//! the measured current is too high, then the PWM duty cycle is set to the
//! minimum pulse width.  The minimum pulse width is needed in order to
//! make a current measurement.
//!
//! This method has the advantage of using feedback to regulate the
//! current in the winding (as in chopper mode), yet does not require
//! the intensive processing of chopper mode to take data and switch the
//! output signals.  This method relies on the ability of the hardware to
//! synchronize the PWM and ADC modules.  This has the effect of isolating
//! the action of taking ADC data and switching the outputs from the
//! processing to determine the pulse width setting, making it much less
//! sensitive to interrupt latency.
//!
//! The code for implementing the Step Control module is contained in
//! <tt>stepctrl.c</tt>, with <tt>stepctrl.h</tt> containing the definitions
//! for the variables and functions exported to the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup stepctrl_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! Defines the IDLE state for the fixed timer.
//
//*****************************************************************************
#define TIMER_STATE_IDLE        0

//*****************************************************************************
//
//! Defines the FIXED_ON state for the fixed timer.
//
//*****************************************************************************
#define TIMER_STATE_FIXED_ON    1

//*****************************************************************************
//
//! Defines the BLANK_OFF state for the fixed timer.
//
//*****************************************************************************
#define TIMER_STATE_BLANK_OFF   2

//*****************************************************************************
//
//! Defines the ADC_DELAY state for the fixed timer.
//
//*****************************************************************************
#define TIMER_STATE_ADC_DELAY   3

//*****************************************************************************
//
//! Defines the IDLE state for the ADC handler.
//
//*****************************************************************************
#define ADC_STATE_IDLE          0

//*****************************************************************************
//
//! Defines the CHOP state for the ADC handler.
//
//*****************************************************************************
#define ADC_STATE_CHOP          1

//*****************************************************************************
//
//! Defines the Closed-loop PWM state for the ADC handler.
//
//*****************************************************************************
#define ADC_STATE_CLOSEDPWM     2

//*****************************************************************************
//
//! Defines the minimum PWM pulse width for closed-loop PWM mode.  The PWM must
//! have a minimum pulse width because current measurement can only be
//! taken when the output is on.
//
//*****************************************************************************
#define MIN_PWM_COUNTS ((SYSTEM_CLOCK * 3) / 1000000L)  // 3 microseconds

//*****************************************************************************
//
//! For closed-loop PWM mode, defines the amount of time after the center of a
//! PWM pulse before an ADC acquisition is triggered.  This is used to
//! adjust the setup and hold time of the current signal for an ADC
//! measurement.
//
//*****************************************************************************
#define ACQ_DELAY_COUNTS ((SYSTEM_CLOCK * 10) / 10000000L)  // 1 microsecond

//*****************************************************************************
//
//! For closed-loop PWM mode, defines the maximum difference between the
//! measured and desired current before the PWM output will be turned on 100%.
//! For differences less than this amount, the PWM output is adjusted to
//! some value less than 100%.
//
//*****************************************************************************
#define MAX_CURRENT_DELTA (MILLIAMPS2COUNTS(1000))      // 1000 milliamps

//*****************************************************************************
//
//! Defines the amount of delay after the winding is turned on, before
//! the ADC acquisition starts, when using chopper mode. The delay is
//! to allow for rise time in the current sense circuit before an ADC
//! acquisition is started. The value is in units of microseconds.
//
//*****************************************************************************
#define ADC_DELAY               2

//*****************************************************************************
//
//! When using the chopper control method, this determines the amount of
//! time that the control signal is left turned off, before being turned
//! on again. It is measured in units of microseconds, with a range of
//! 10-65535. Great care should be taken when changing this value because an
//! incorrect value could cause too much current to flow in the motor winding.
//
//*****************************************************************************
unsigned short g_usBlankOffTime = 100;

//*****************************************************************************
//
//! When using the open-loop PWM control method, this determines the amount
//! of time that the signal is left turned on, before PWM is applied.
//! It is measured in units of microseconds, with a range of 1-65535. This
//! value should be set to an appropriate value prior to starting the PWM
//! control mode.
//
//*****************************************************************************
unsigned short g_usFixedOnTime = 1;

//*****************************************************************************
//
//! The PWM period in units of system clock ticks. This value is set
//! when StepCtrlOpenPWMMode() is called to set up a PWM control mode. The
//! value is used for PWM control methods in order to be able to compute
//! on and off times in the PWM cycle.
//
//*****************************************************************************
unsigned long g_ulPwmPeriod = SYSTEM_CLOCK / 20000;

//*****************************************************************************
//
//! This array is used to store the ADC data for each of the A and B
//! windings. Storage is allocated for up to 8 samples on each winding,
//! even though the sequencers are programmed for fewer samples than 8.
//! This amount of space is allowed because it is the theoretical maximum
//! number of samples that could be returned when reading all the data from
//! an ADC sequencer, and ensures that an adjacent variable in memory will
//! never be overwritten.
//!
//! The data stored in this array is in units of raw ADC counts, and must
//! be converted in order to find the current in engineering units.
//
//*****************************************************************************
unsigned long g_ulCurrentRaw[2][8];

//*****************************************************************************
//
//! This is used to store the peak current measured in chopper mode, for
//! each winding. The value stored is the last current reading taken when
//! the chopper detects that the current has gone past the threshold. This
//! should represent the current level when the winding is on.
//! The value is reset to 0 whenever the winding is turned off, because
//! when the winding is off, no current can be measured.
//
//*****************************************************************************
unsigned long g_ulPeakCurrentRaw[2];

//*****************************************************************************
//
//! Defines the register offset of the PWM generators used for winding A.
//! This would be changed if the PWM outputs were wired differently.
//
//*****************************************************************************
#define WINDING_A_PWM_GEN_OFFSET    PWM_GEN_0_OFFSET

//*****************************************************************************
//
//! Defines the register offset of the PWM generators used for winding B.
//! This would be changed if the PWM outputs were wired differently.
//
//*****************************************************************************
#define WINDING_B_PWM_GEN_OFFSET    PWM_GEN_1_OFFSET

//*****************************************************************************
//
//! Defines the register offset of the PWM generators used for the H-bridge
//! enable signals (for both windings A and B).
//
//*****************************************************************************
#define WINDING_EN_PWM_GEN_OFFSET   PWM_GEN_2_OFFSET

//*****************************************************************************
//
//! Defines the register address of the base of the PWM generator that is
//! used to control the H-bridge enable signals for windings A and B.
//
//*****************************************************************************
#define WINDING_EN_GEN_BASE (PWM0_BASE + WINDING_EN_PWM_GEN_OFFSET)

//*****************************************************************************
//
//! Defines the value that is written to the PWM control register that
//! will cause the output to be turned on.
//
//*****************************************************************************
#define CTRL_PIN_ON_VAL     0xFFF

//*****************************************************************************
//
//! Defines the value that is written to the PWM control register that
//! will cause the output to be turned off.
//
//*****************************************************************************
#define CTRL_PIN_OFF_VAL    0xAAA

//*****************************************************************************
//
//! Defines the value that is written to the PWM control register that
//! will cause the PWM generator output to start generating PWM based on
//! comparator A.  Note that this "A" refers to part of the PWM generator
//! (A and B comparators) and is not necessarily the same thing as the "A"
//! winding.
//
//*****************************************************************************
#define CTRL_PIN_PWMA_VAL   (PWM_X_GENA_ACTCMPAU_ONE |                        \
                             PWM_X_GENA_ACTCMPAD_ZERO |                       \
                             PWM_X_GENA_ACTZERO_ZERO)

//*****************************************************************************
//
//! Defines the value that is written to the PWM control register that
//! will cause the PWM generator output to start generating PWM based on
//! comparator B.  Note that this "B" refers to part of the PWM generator
//! (A and B comparators) and is not necessarily the same thing as the "B"
//! winding.
//
//*****************************************************************************
#define CTRL_PIN_PWMB_VAL   (PWM_X_GENA_ACTCMPBU_ONE |                        \
                             PWM_X_GENA_ACTCMPBD_ZERO |                       \
                             PWM_X_GENA_ACTZERO_ZERO)

//*****************************************************************************
//
//! A structure that holds register addresses and control values that are
//! used by the winding control code. The values are pre-computed as much
//! as possible to reduce the amount of run-time calculations needed,
//! especially for interrupt service routines.
//
//*****************************************************************************
typedef struct
{
    //
    //! The register base address of the PWM generator used to control
    //! the H-bridge for the winding.
    //
    unsigned long ulPwmGenBase;

    //
    //! The bit identifier of the PWM generator used to control the winding.
    //
    unsigned long ulPwmGenBit;

    //
    //! The load register address for the timer used for fixed timing
    //! methods.
    //
    unsigned long ulTmrLoadAddr;

    //
    //! The value used to enable the timer used for the winding.
    //
    unsigned long ulTmrEnaVal;

    //
    //! A value which is used to indicate winding A or B. It is used
    //! as an offset to the correct PWM control register for the H-bridge
    //! enable signals. The value is 0(A) or 4(B).
    //
    unsigned long ulPwmAB;

    //
    //! The PWM generator control register that is used by the timer
    //! and ADC ISRs to start or stop PWM control on the winding.
    //
    unsigned long ulPwmGenCtlReg;

    //
    //! The value to be applied to the PWM generator control register, by
    //! the timer or ADC ISRs, to cause PWM to start or stop running.
    //
    unsigned long ulPwmGenCtlVal;

    //
    //! The current threshold to use for chopper mode in raw ADC counts.
    //
    unsigned long ulChopperCurrent;

    //
    //! The sequencer used for chopper ADC samples for this winding.
    //
    unsigned char ucADCSeq;

    //
    //! The state of the fixed timer handler.
    //
    unsigned char ucTimerState;

    //
    //! The state of the ADC handler.
    //
    unsigned char ucADCState;
}
tWinding;

//*****************************************************************************
//
//! The table that holds the register addresses and control values
//! for each winding.
//
//*****************************************************************************
tWinding sWinding[] =
{
    {
        PWM0_BASE + WINDING_A_PWM_GEN_OFFSET,
        PWM_GEN_0_BIT,
        FIXED_TMR_BASE + TIMER_O_TAILR,
        TIMER_CTL_TAEN,
        0,
        0, 0, 0,
        WINDING_A_ADC_SEQUENCER, TIMER_STATE_IDLE, ADC_STATE_IDLE
    },
    {
        PWM0_BASE + WINDING_B_PWM_GEN_OFFSET,
        PWM_GEN_1_BIT,
        FIXED_TMR_BASE + TIMER_O_TBILR,
        TIMER_CTL_TBEN,
        4,
        0, 0, 0,
        WINDING_B_ADC_SEQUENCER, TIMER_STATE_IDLE, ADC_STATE_IDLE
    }
};

//*****************************************************************************
//
//! The interrupt handler for fixed timing.
//!
//! \param ulWinding specifies which winding is being processed.  It can
//! be one of WINDING_ID_A or WINDING_ID_B.
//!
//! This handler is called from the specific interrupt handler for the timer
//! used for winding A or winding B.  There are two timers, one for each
//! winding, each with an interrupt handler.  This handler is used for the
//! common processing for both.  The timers are used to generate a timeout
//! either for the fixed on time if open-loop PWM mode is used, or the off
//! blanking time if chopper mode is used.
//!
//! If open-loop PWM mode is used, then when this timer times out, it switches
//! the control output to PWM (which was previously on). If chopper mode is
//! used, then when this timer times out, it turns the control signal back
//! on (which was previously off), and starts an ADC conversion.
//!
//! On entry here, the interrupt has already been acknowledged by the
//! specific timer interrupt handler that called here.
//!
//! \return None.
//
//*****************************************************************************
static void
StepCtrlTimerHandler(unsigned long ulWinding)
{
    unsigned long ulPwmComp;
    tWinding *pWinding;

    //
    // Get a pointer to the winding data.
    //
    pWinding = &sWinding[ulWinding];

    //
    // Make sure that there is a register assigned to control the pin.
    //
    if(pWinding->ulPwmGenCtlReg)
    {
        switch(pWinding->ucTimerState)
        {
            //
            // This state is used in PWM mode. The control signal has
            // been turned on and left on until this timer times out.
            // When this happens, the pin is changed to start PWM
            // at the appropriate duty cycle for the target current.
            //
            case TIMER_STATE_FIXED_ON:
            {
                //
                // The PWM cycle needs to be restarted at the end of
                // the fixed on time. This is done by short cycling the PWM
                // generator with the control set to turn off the pin for
                // all triggers. This will cause the pin to turn off. The
                // generator is reset - this will cause it to restart at 0.
                // Then the PWM generator is programmed to generate normal
                // PWM. This should cause the pin to start normal PWM cycle.
                //
                // Program the PWM generator to turn off the pin for all
                // conditions.
                //
                HWREG(pWinding->ulPwmGenCtlReg) = CTRL_PIN_OFF_VAL;

                //
                // Read the PWM load register value and save it aside. Then
                // load the PWM gen with a very short period.
                // Equivalent DriverLib call:
                //  ulPwmComp = PWMGenPeriodGet(PWM0_BASE, [PWM_GEN_X]);
                //  PWMGenPeriodSet(PWM0_BASE, [PWM_GEN_X], 3);
                //
                ulPwmComp = HWREG(pWinding->ulPwmGenBase + PWM_O_X_LOAD);
                HWREG(pWinding->ulPwmGenBase + PWM_O_X_LOAD) = 3;

                //
                // Reset the PWM generator time base at the end of the
                // fixed on time so that it starts at the beginning of a cycle.
                // Equivalent DriverLib call:
                //  PWMSyncTimeBase(PWM0_BASE, pWinding->ulPwmGenBit);
                //
                HWREG(PWM0_BASE + PWM_O_SYNC) = pWinding->ulPwmGenBit;

                //
                // Switch the winding control pin to a new value. In PWM
                // mode, then pin will have been on at this point,
                // and the line below will switch it to using PWM.
                // The values in .ulTmrPwmReg and .ulTmrPwmVal were preset
                // when the fixed timing mode was started, and are used to
                // control one of the winding positive, negative, or
                // enable signals.
                //
                HWREG(pWinding->ulPwmGenCtlReg) = pWinding->ulPwmGenCtlVal;

                //
                // Restore the PWM generator period, so that it starts to
                // cycle normally, from 0.
                // Equivalent DriverLib call:
                //  PWMGenPeriodSet(PWM0_BASE, [PWM_GEN_X], ulPwmComp);
                //
                HWREG(pWinding->ulPwmGenBase + PWM_O_X_LOAD) = ulPwmComp;

                break;
            }

            //
            // This state is used in chopper mode. The control signal has
            // been turned off for the "off blanking" time to allow the
            // current to decay, and now the pin will be turned on again.
            // The timer will be started again to provide a short delay
            // before ADC sampling occurs.
            //
            case TIMER_STATE_BLANK_OFF:
            {
                //
                // Switch the winding control pin to a new value. In chopper
                // mode, then pin will have been off at this point,
                // and the line below will switch it on.
                // The values in .ulTmrPwmReg and .ulTmrPwmVal were preset
                // when the fixed timing mode was started, and are used to
                // control one of the winding positive, negative, or
                // enable signals.
                //
                HWREG(pWinding->ulPwmGenCtlReg) = pWinding->ulPwmGenCtlVal;

                //
                // Load the timer with the ADC delay. This is the time
                // until the ADC acquisition should be started. This is
                // used to allow the current sense signal to rise before
                // starting to sample with ADC.
                // Equivalent DriverLib call:
                //  TimerLoadSet(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>,
                //               ADC_DELAY);
                //
                HWREG(pWinding->ulTmrLoadAddr) = ADC_DELAY;

                //
                // Start the fixed timer running. When the timer expires
                // it will start up the ADC acquisition.
                // Equivalent DriverLib call:
                //  TimerEnable(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>);
                //
                HWREG(FIXED_TMR_BASE + TIMER_O_CTL) |= pWinding->ulTmrEnaVal;

                //
                // Set the next state to start the ADC.
                //
                pWinding->ucTimerState = TIMER_STATE_ADC_DELAY;
                break;
            }

            //
            // This state is used in chopper mode. The control signal
            // has been turned on, but the ADC not started yet. So now
            // the ADC acquisition will be started to measure the winding
            // current.
            //
            case TIMER_STATE_ADC_DELAY:
            {
                //
                // Enable the ADC sequencer
                // Equivalent DriverLib call:
                //  ADCSequenceEnable(ADC0_BASE, pWinding->ucAdcSeq);
                //
                HWREG(ADC0_BASE + ADC_O_ACTSS) |= 1 << pWinding->ucADCSeq;

                //
                // Trigger ADC to take a sample for chopper comparison.
                // Equivalent DriverLib call:
                //  ADCProcessorTrigger(ADC0_BASE, pWinding->ucAdcSeq);
                //
                HWREG(ADC0_BASE + ADC_O_PSSI) = 1 << pWinding->ucADCSeq;

                //
                // Set the next state to idle.
                //
                pWinding->ucTimerState = TIMER_STATE_IDLE;
                break;
            }

            //
            // If this state is entered, then do nothing, just exit.
            //
            case TIMER_STATE_IDLE:
            default:
            {
                break;
            }
        }
    }
}

//*****************************************************************************
//
//! The interrupt handler for the timer used for winding A fixed timing.
//!
//! This handler is called when the timer for winding A times out. This
//! timer is used to generate a timeout either for the fixed on time if
//! PWM mode is used, or the off blanking time if chopper mode is used.
//!
//! This interrupt handler calls a common timer handler for both
//! A and B windings.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlTimerAIntHandler(void)
{
    //
    // Clear the timer interrupt
    // Equivalent DriverLib call:
    //  TimerIntClear(FIXED_TMR_BASE, TIMER_TIMA_TIMEOUT);
    //
    HWREG(FIXED_TMR_BASE + TIMER_O_ICR) = TIMER_TIMA_TIMEOUT;

    //
    // Call the common timer interrupt handler.
    //
    StepCtrlTimerHandler(WINDING_ID_A);
}

//*****************************************************************************
//
//! The interrupt handler for the timer used for winding B fixed timing.
//!
//! This handler is called when the timer for winding B times out. This
//! timer is used to generate a timeout either for the fixed on time if
//! PWM mode is used, or the off blanking time if chopper mode is used.
//!
//! This interrupt handler calls a common timer handler for both
//! A and B windings.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlTimerBIntHandler(void)
{
    //
    // Clear the timer interrupt
    // Equivalent DriverLib call:
    //  TimerIntClear(FIXED_TMR_BASE, TIMER_TIMB_TIMEOUT);
    //
    HWREG(FIXED_TMR_BASE + TIMER_O_ICR) = TIMER_TIMB_TIMEOUT;

    //
    // Call the common timer interrupt handler.
    //
    StepCtrlTimerHandler(WINDING_ID_B);
}

//*****************************************************************************
//
//! The interrupt handler for the ADC winding current sample.
//!
//! \param ulWinding specifies which winding is being processed.  It can
//! be one of WINDING_ID_A or WINDING_ID_B.
//!
//! This handler is called from the interrupt handler for the ADC sequencer
//! for winding A or B. The ADC is used for closed-loop PWM and Chopper modes.
//! If chopper mode is used then the sample acquisition was started either
//! from this function, or from the winding fixed timer interrupt handler.  If
//! closed-loop PWM mode is used, then the ADC sample acquisition is triggered
//! from the PWM generator at a certain point in the PWM cycle (when the
//! output is turned on).
//!
//! <b>Chopper operation:</b>
//!
//! This interrupt handler compares the current sampled from the ADC
//! with the chopping threshold. If the measured current is below the
//! threshold, then a new acquisition is started and the control pin is left
//! in the on state. If the measured current is above the threshold, then
//! the control pin is turned off, and the winding timer is used to start
//! the off blanking time interval.
//!
//! <b>Closed-loop PWM operation:</b>
//!
//! The interrupt handler compares the current samples from the ADC with
//! the chopping threshold.  If the measured current is above the threshold,
//! then the PWM output is set to the minimum width.  If the measured current
//! is below the threshold, then the PWM duty cycle is adjusted so that
//! the duty cycle is related to the difference between the actual current
//! and the desired current.  The larger the difference, the higher the duty
//! cycle.
//!
//! On entry here, the interrupt has already been acknowledged by the
//! specific ADC interrupt handler that called here.
//!
//! \return None.
//
//*****************************************************************************
static void
StepCtrlADCHandler(unsigned long ulWinding)
{
    static unsigned int bDynamicExtend = 1;
    long lSampleCount;
    unsigned long ulDelta;
    unsigned long ulDuty;
    tWinding *pWinding;

    //
    // Get a pointer to the winding data.
    //
    pWinding = &sWinding[ulWinding];

    //
    // Read the winding current data from the ADC sequencer, and
    // store it.
    //
    lSampleCount = ADCSequenceDataGet(ADC0_BASE, pWinding->ucADCSeq,
                                  &g_ulCurrentRaw[ulWinding][0]);

    //
    // Make sure that there is a register assigned to control the pin,
    // the handler is in an active state, and that the expected number of
    // ADC samples were retrieved.
    //
    if(pWinding->ulPwmGenCtlReg && pWinding->ucADCState)
    {
        //
        // If the sample count is not correct, then just start a new
        // acquisition and return.
        //
        if(lSampleCount != 1)
        {
            //
            // Equivalent DriverLib call:
            //  ADCProcessorTrigger(ADC0_BASE, pWinding->ucADCSeq);
            //
            HWREG(ADC0_BASE + ADC_O_PSSI) = 1 << pWinding->ucADCSeq;
            return;
        }

        //
        // Save the peak measured value for current.
        // This could be removed if the current reporting was not needed.
        //
        if((g_ulCurrentRaw[ulWinding][0] > g_ulPeakCurrentRaw[ulWinding]))
        {
            g_ulPeakCurrentRaw[ulWinding] = g_ulCurrentRaw[ulWinding][0];
        }

        //
        // Process the current control method for operating in chopper mode.
        //
        if(pWinding->ucADCState == ADC_STATE_CHOP)
        {
            //
            // Compare the measured winding current to see if it is above the
            // threshold. If this condition is true, then turn off the winding
            // control pin, and start the off blanking timer.
            //
            if((g_ulCurrentRaw[ulWinding][0] >= pWinding->ulChopperCurrent))
            {
                //
                // Turn the control pin off.
                //
                HWREG(pWinding->ulPwmGenCtlReg) = CTRL_PIN_OFF_VAL;

                //
                // Disable the ADC sequencer.
                // Equivalent DriverLib call:
                //  ADCSequenceDisable(ADC0_BASE, pWinding->ucADCSeq);
                //
                HWREG(ADC0_BASE + ADC_O_ACTSS) &= ~(1 << pWinding->ucADCSeq);

                //
                // Load the fixed timer with the blanking time. The blanking
                // time is multiplied by an extension factor. Each time the
                // code passes through here with the current above the
                // threshold, the extension factor is increased. The effect is
                // to lengthen the time that the voltage remains off if the
                // current in the winding is not dropping.
                // Equivalent DriverLib call:
                //  TimerLoadSet(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>,
                //               g_usBlankOffTime * bDynamicExtend);
                //
                HWREG(pWinding->ulTmrLoadAddr) = g_usBlankOffTime *
                                                 bDynamicExtend;
                bDynamicExtend++;

                //
                // Start the fixed interval timer running. When the timer
                // expires it will turn the signal back on and trigger another
                // ADC sample.
                // Equivalent DriverLib call:
                //  TimerEnable(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>);
                //
                HWREG(FIXED_TMR_BASE + TIMER_O_CTL) |= pWinding->ulTmrEnaVal;

                //
                // Set the state of the fixed time to indicate blanking time.
                //
                pWinding->ucTimerState = TIMER_STATE_BLANK_OFF;
            }

            //
            // Else, if the measured current was below the threshold, then
            // leave the control pin on, and start another ADC acquisition.
            //
            else
            {
                //
                // Any passes through here means that the current was below
                // the threshold, so reset the dynamic extension factor.
                //
                bDynamicExtend = 1;

                //
                // Start next ADC acquisition.
                // Equivalent DriverLib call:
                //  ADCProcessorTrigger(ADC0_BASE, pWinding->ucADCSeq);
                //
                HWREG(ADC0_BASE + ADC_O_PSSI) = 1 << pWinding->ucADCSeq;
            }
        }   // end of ADC_STATE_CHOP processing

        //
        // Process the current control method for closed-loop PWM mode.
        //
        else if(pWinding->ucADCState == ADC_STATE_CLOSEDPWM)
        {
            //
            // Compare the measured winding current to see if it is above the
            // threshold. If this condition is true, then set the PWM output
            // to the minimum pulse width.  The minimum pulse width must be
            // some non-zero value in order to allow the current measurement
            // to be made. Current can only be measured when the output is on.
            //
            if((g_ulCurrentRaw[ulWinding][0] >= pWinding->ulChopperCurrent))
            {
                ulDuty = MIN_PWM_COUNTS;
            }

            //
            // Else, the measured current is below the threshold, so
            // compute the duty cycle based on the difference between the
            // threshold and the actual measured current.
            //
            else
            {
                //
                // Compute the difference from the desired current.
                //
                ulDelta = pWinding->ulChopperCurrent -
                                            g_ulCurrentRaw[ulWinding][0];

                //
                // If the difference is too large (the current is too low
                // by a large amount), then set the output to nearly 100%.
                //
                if(ulDelta >= MAX_CURRENT_DELTA)
                {
                    ulDuty = g_ulPwmPeriod - 4;
                }

                //
                // else, the difference is not too large, so compute
                // the duty cycle based on the difference.
                //
                else
                {
                    //
                    // Compute the duty cycle based on a simple linear
                    // equation as follows:
                    //  ulDelta == 0   --> ulDuty = 0%
                    //  ulDelta == MAX --> ulDuty = 100%
                    //
                    ulDuty = g_ulPwmPeriod * ulDelta / MAX_CURRENT_DELTA;

                    //
                    // If the computed duty cycle works out to be 100%,
                    // then make is slightly shorter to ensure a switch
                    // in the output.  This will avoid the situation where
                    // the turn-on and turn-off values are the same in the
                    // PWM comparator.
                    //
                    if(ulDuty == g_ulPwmPeriod)
                    {
                        ulDuty = g_ulPwmPeriod - 4;
                    }

                    //
                    // If the computed duty cycle is 0, then make sure it is
                    // at least the minimum value.
                    //
                    if(ulDuty < MIN_PWM_COUNTS)
                    {
                        ulDuty = MIN_PWM_COUNTS;
                    }
                }
            }

            //
            // Load the newly computed duty cycle into the comparator.
            // Equivalent DriverLib call:
            //  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_n, ulDuty);
            //
            HWREG(pWinding->ulPwmGenCtlReg) =
                                (g_ulPwmPeriod - ulDuty) / 2;

        } // end of ADC_STATE_CLOSEDPWM processing
    }
}

//*****************************************************************************
//
//! The interrupt handler for the ADC sequencer used for winding A.
//!
//! This handler is called when the ADC sequencer used for winding A
//! finishes taking samples.
//!
//! This interrupt handler calls a common handler for the ADC interrupts
//! for winding A and B.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlADCAIntHandler(void)
{
    //
    // Clear the ADC interrupt for this sequencer.
    // Equivalent DriverLib call:
    //  ADCIntClear(ADC0_BASE, WINDING_A_ADC_SEQUENCER);
    //
    HWREG(ADC0_BASE + ADC_O_ISC) = 1 << WINDING_A_ADC_SEQUENCER;

    //
    // Call the common ADC interrupt handler.
    //
    StepCtrlADCHandler(WINDING_ID_A);
}

//*****************************************************************************
//
//! The interrupt handler for the ADC sequencer used for winding B.
//!
//! This handler is called when the ADC sequencer used for winding B
//! completes taking samples.
//!
//! This interrupt handler calls a common handler for the ADC interrupts
//! for winding A and B.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlADCBIntHandler(void)
{
    //
    // Clear the ADC interrupt for this sequencer.
    // Equivalent DriverLib call:
    //  ADCIntClear(ADC0_BASE, WINDING_B_ADC_SEQUENCER);
    //
    HWREG(ADC0_BASE + ADC_O_ISC) = 1 << WINDING_B_ADC_SEQUENCER;

    //
    // Call the common ADC interrupt handler.
    //
    StepCtrlADCHandler(WINDING_ID_B);
}

//*****************************************************************************
//
//! Configures the winding control signals for chopper mode.
//!
//! This function should be called prior to using chopper mode as the
//! control method. It configures the control signals and the PWM
//! generators to be used in chopper mode.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlChopMode(void)
{
    //
    // Set the PWM control register to 0. This will cause timer and ADC
    // ISRs to do nothing in case they fire again. Also, force all output
    // pins off.
    //
    sWinding[WINDING_ID_A].ulPwmGenCtlReg = 0;
    sWinding[WINDING_ID_B].ulPwmGenCtlReg = 0;
    HWREG(PWM0_BASE + PWM_GEN_0_OFFSET + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
    HWREG(PWM0_BASE + PWM_GEN_0_OFFSET + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
    HWREG(PWM0_BASE + PWM_GEN_1_OFFSET + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
    HWREG(PWM0_BASE + PWM_GEN_1_OFFSET + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;

    //
    // Turn on the enable pins now, this will prevent a current surge
    // when the motor is first moved.
    //
    HWREG(PWM0_BASE + PWM_GEN_2_OFFSET + PWM_O_X_GENA) = CTRL_PIN_ON_VAL;
    HWREG(PWM0_BASE + PWM_GEN_2_OFFSET + PWM_O_X_GENB) = CTRL_PIN_ON_VAL;

    //
    // Set the ADC sequencers to use a processor trigger.
    //
    ADCSequenceConfigure(ADC0_BASE, WINDING_A_ADC_SEQUENCER,
                         ADC_TRIGGER_PROCESSOR, WINDING_A_ADC_PRIORITY);
    ADCSequenceConfigure(ADC0_BASE, WINDING_B_ADC_SEQUENCER,
                         ADC_TRIGGER_PROCESSOR, WINDING_B_ADC_PRIORITY);

    //
    // Set the PWM period for all generators. Since, for chopper mode,
    // the PWM generator outputs are used just to switch the outputs
    // on or off, and not generate PWM, the PWM period is set very
    // short so that any changes will take effect right away.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, 4);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, 4);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 4);

    //
    // Reset the peak winding current measurements.
    //
    g_ulPeakCurrentRaw[WINDING_ID_A] = 0;
    g_ulPeakCurrentRaw[WINDING_ID_B] = 0;

    //
    // Set the ADC processing state to chop mode.
    //
    sWinding[WINDING_ID_A].ucADCState = ADC_STATE_CHOP;
    sWinding[WINDING_ID_B].ucADCState = ADC_STATE_CHOP;
}

//*****************************************************************************
//
//! Configures the winding control signals for Open-loop PWM mode.
//!
//! \param ulPeriod is the PWM period in system clock ticks
//!
//! This function should be called prior to using open-loop PWM mode
//! as the control method. It configures the control signals and the
//! PWM generators to be used in open-loop PWM mode.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlOpenPWMMode(unsigned long ulPeriod)
{
    //
    // Save the value passed in, it is needed later.
    //
    g_ulPwmPeriod = ulPeriod;

    //
    // Set the PWM control register to 0. This will cause timer and ADC
    // ISRs to do nothing in case they fire again. Also, force all output
    // pins off.
    //
    sWinding[WINDING_ID_A].ulPwmGenCtlReg = 0;
    sWinding[WINDING_ID_B].ulPwmGenCtlReg = 0;
    HWREG(PWM0_BASE + PWM_GEN_0_OFFSET + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
    HWREG(PWM0_BASE + PWM_GEN_0_OFFSET + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
    HWREG(PWM0_BASE + PWM_GEN_1_OFFSET + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
    HWREG(PWM0_BASE + PWM_GEN_1_OFFSET + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;

    //
    // Turn on the enable pins now, this will prevent a current surge
    // when the motor is first moved.
    //
    HWREG(PWM0_BASE + PWM_GEN_2_OFFSET + PWM_O_X_GENA) = CTRL_PIN_ON_VAL;
    HWREG(PWM0_BASE + PWM_GEN_2_OFFSET + PWM_O_X_GENB) = CTRL_PIN_ON_VAL;

    //
    // Set the PWM period for all generators.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ulPeriod);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, ulPeriod);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, ulPeriod);

    //
    // Reset the peak winding current measurements.
    //
    g_ulPeakCurrentRaw[WINDING_ID_A] = 0;
    g_ulPeakCurrentRaw[WINDING_ID_B] = 0;

    //
    // Set the ADC processing state to idle (not used for PWM).
    //
    sWinding[WINDING_ID_A].ucADCState = ADC_STATE_IDLE;
    sWinding[WINDING_ID_B].ucADCState = ADC_STATE_IDLE;

    //
    // Set the ADC sequencers to use a processor trigger.  This will
    // prevent them from being triggered by the PWM generators.
    //
    ADCSequenceConfigure(ADC0_BASE, WINDING_A_ADC_SEQUENCER,
                         ADC_TRIGGER_PROCESSOR, WINDING_A_ADC_PRIORITY);
    ADCSequenceConfigure(ADC0_BASE, WINDING_B_ADC_SEQUENCER,
                         ADC_TRIGGER_PROCESSOR, WINDING_B_ADC_PRIORITY);
}

//*****************************************************************************
//
//! Configures the winding control signals for Closed-Loop PWM mode.
//!
//! \param ulPeriod is the PWM period in system clock ticks
//!
//! This function should be called prior to using Closed-loop PWM mode
//! as the control method. It configures the control signals and the
//! PWM generators to be used in PWM mode, and sets up the trigger
//! points for ADC acqusition for current measurement.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlClosedPWMMode(unsigned long ulPeriod)
{
    //
    // Set up for PWM generation.
    //
    StepCtrlOpenPWMMode(ulPeriod);

    //
    // The following lines of code are used to set the 3 PWM generators
    // out of phase with each other.  This is done so that the ADC data
    // collection, which is triggered at certain points in the PWM cycle
    // occurs at different times for each generator.  This ensures that
    // all of the generators will not trigger an ADC acquisition at the
    // same time, and thus causing possible delays in acquisition.  When
    // an acquisition is triggered, we want to acquire the current sample
    // as soon as possible.
    //
    // Force generator 0 to reset at 0.
    //
    PWMSyncTimeBase(PWM0_BASE, PWM_GEN_0_BIT);

    //
    // Now wait for gen 0 to get halfway to it's max value (from 0).
    // Max value is at half of period, so divisor is 4.
    //
    while(HWREG(PWM0_BASE + PWM_GEN_0_OFFSET + PWM_O_X_COUNT) < ulPeriod / 4)
    {
    }

    //
    // Force generator 1 to reset to 0.
    //
    PWMSyncTimeBase(PWM0_BASE, PWM_GEN_1_BIT);

    //
    // Now wait for gen 0 to get halfway back to 0 (from max).
    //
    while(HWREG(PWM0_BASE + PWM_GEN_0_OFFSET + PWM_O_X_COUNT) > ulPeriod / 4)
    {
    }

    //
    // Force generator 2 to reset to 0.
    //
    PWMSyncTimeBase(PWM0_BASE, PWM_GEN_2_BIT);

    //
    // Set pulse width to minimum for all outputs.
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_0, MIN_PWM_COUNTS);
    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_1, MIN_PWM_COUNTS);
    PWMPulseWidthSet(PWM0_BASE, PWM_GEN_2, MIN_PWM_COUNTS);

    //
    // For generators 0 and 1, set comparator B to have its falling edge
    // a specified amount of time after the middle of the PWM period.
    // This will be used to trigger ADC acquisitions and allows for
    // adjusting the setup and hold times for an ADC acquisition.
    //
    HWREG(PWM0_BASE + PWM_GEN_0_OFFSET + PWM_O_X_CMPB) =
                                        (ulPeriod / 2) - ACQ_DELAY_COUNTS;
    HWREG(PWM0_BASE + PWM_GEN_1_OFFSET + PWM_O_X_CMPB) =
                                        (ulPeriod / 2) - ACQ_DELAY_COUNTS;


    //
    // For PWM generators 0 and 1, which are used to control the polarity
    // signal of the H-bridge (one for each winding), set the ADC trigger to
    // be the falling edge of the B comparator.  For generator 2, which is
    // used to control the enable signals for both windings, set the ADC
    // trigger to the "load" event, which is the middle of the PWM pulse.
    //
    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_0, PWM_TR_CNT_BD);
    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_1, PWM_TR_CNT_BD);
    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_2, PWM_TR_CNT_LOAD);

    //
    // Set the ADC processing state to closed-loop PWM mode.
    //
    sWinding[WINDING_ID_A].ucADCState = ADC_STATE_CLOSEDPWM;
    sWinding[WINDING_ID_B].ucADCState = ADC_STATE_CLOSEDPWM;

    //
    // Set the control reg to NULL so that the ADC handler will not try
    // to do anything with it yet.
    //
    sWinding[WINDING_ID_A].ulPwmGenCtlReg = 0;
    sWinding[WINDING_ID_B].ulPwmGenCtlReg = 0;

    //
    // Enable the ADC sequencers for windings A and B.  This will start
    // the ADC acquisitions running, triggered at specific points in the PWM
    // cycle. The ADC handler will do nothing at the moment because no current
    // control has been set up.  That will be handled when
    // StepCtrlClosedPWMSlow() or StepCtrlClosedPWMFast() is called.
    //
    ADCSequenceEnable(ADC0_BASE, WINDING_A_ADC_SEQUENCER);
    ADCSequenceEnable(ADC0_BASE, WINDING_B_ADC_SEQUENCER);
}

//*****************************************************************************
//
//! Sets up a step using chopper mode and slow decay.
//!
//! \param ulWinding is the winding ID (A or B)
//! \param lSetting is the chopping current (signed), in raw ADC counts
//!
//! This function will configure the chopper to control the pins needed
//! for slow current decay. It drives the winding positive or negative
//! (or off), according to the value and sign of the lSetting parameter.
//! Once the control signals are set to apply voltage to the winding,
//! an ADC acquisition is started. This will start the chopper running
//! for this winding.
//!
//! For slow current decay, one side of the H-bridge is set high, and the
//! other side is set low, to cause current to flow in the positive or
//! negative direction. The gate drivers are always enabled. To control
//! current, the "high" side of the H-bridge is switched between high and
//! low. When it is high, the bus voltage is applied to the winding and
//! current flows. When it is low, then both sides of the H-bridge will be
//! low, and bus voltage is removed, but current continues to circulate in
//! the winding as it decays slowly.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlChopSlow(unsigned long ulWinding, long lSetting)
{
    tWinding *pWinding;
    pWinding = &sWinding[ulWinding];

    //
    // Disable the fixed interval timer so it can be updated.
    // Set the load register to the off blanking time.
    // Equivalent DriverLib call:
    //  TimerDisable(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>);
    //  TimerLoadSet(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>, g_usBlankOffTime);
    //
    HWREG(FIXED_TMR_BASE + TIMER_O_CTL) &= ~pWinding->ulTmrEnaVal;
    HWREG(pWinding->ulTmrLoadAddr) = g_usBlankOffTime;

    //
    // Winding current is positive, set the negative side low,
    // and start chopping the positive side.
    //
    if(lSetting > 0)
    {
        //
        // Save the chopping current threshold.
        //
        pWinding->ulChopperCurrent = lSetting;

        //
        // Save the PWM register address and value that will be used by the
        // ADC and timer ISRs to control the signal on the control pin.
        //
        pWinding->ulPwmGenCtlReg = pWinding->ulPwmGenBase + PWM_O_X_GENA;
        pWinding->ulPwmGenCtlVal = CTRL_PIN_ON_VAL;

        //
        // Set positive side high and negative side low, so the
        // winding is turned on, in the positive direction.
        // This will apply voltage and start the current flowing.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_ON_VAL;

        //
        // Enable the ADC sequencer
        // Equivalent DriverLib call:
        //  ADCSequenceEnable(ADC0_BASE, pWinding->ucADCSeq);
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= 1 << pWinding->ucADCSeq;

        //
        // Trigger ADC to take a sample for chopper comparison.
        // Equivalent DriverLib call:
        //  ADCProcessorTrigger(ADC0_BASE, pWinding->ucADCSeq);
        //
        HWREG(ADC0_BASE + ADC_O_PSSI) = 1 << pWinding->ucADCSeq;
    }

    //
    // Winding current is negative, set the positive side low,
    // and start chopping the negative side.
    //
    else if(lSetting < 0)
    {
        //
        // Save the chopping current threshold. Account for sign.
        //
        pWinding->ulChopperCurrent = -lSetting;

        //
        // Save the PWM register address and value that will be used by the
        // ADC and timer ISRs to control the signal on the control pin.
        //
        pWinding->ulPwmGenCtlReg = pWinding->ulPwmGenBase + PWM_O_X_GENB;
        pWinding->ulPwmGenCtlVal = CTRL_PIN_ON_VAL;

        //
        // Set negative side high and positive side low, so the
        // winding is turned on, in the negative direction.
        // This will apply voltage and start the current flowing.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_ON_VAL;

        //
        // Enable the ADC sequencer
        // Equivalent DriverLib call:
        //  ADCSequenceEnable(ADC0_BASE, pWinding->ucADCSeq);
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= 1 << pWinding->ucADCSeq;

        //
        // Trigger ADC to take a sample for chopper comparison.
        // Equivalent DriverLib call:
        //  ADCProcessorTrigger(ADC0_BASE, pWinding->ucADCSeq);
        //
        HWREG(ADC0_BASE + ADC_O_PSSI) = 1 << pWinding->ucADCSeq;
    }

    //
    // Winding current is 0, set both sides low.
    //
    else
    {
        //
        // Set the chopping current threshold to 0.
        //
        pWinding->ulChopperCurrent = 0;

        //
        // Set both positive and negative side low, so that there is
        // no voltage applied to the winding.
        //
        pWinding->ulPwmGenCtlReg = 0;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
    }

    //
    // Turn on the enable signal for the winding. ulPwmAB is used to
    // select the correct half (A or B) of the PWM generator for the
    // winding enable signals. For slow decay, the enable signal is
    // always left on.
    //
    HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
            CTRL_PIN_ON_VAL;
}

//*****************************************************************************
//
//! Sets up a step using chopper mode and fast decay.
//!
//! \param ulWinding is the winding ID (A or B)
//! \param lSetting is the chopping current (signed), in raw ADC counts
//!
//! This function will configure the chopper to control the pins needed
//! for fast current decay. It drives the winding positive or negative
//! (or off), according to the value and sign of the lSetting parameter.
//! Once the control signals are set to apply voltage to the winding,
//! an ADC acquisition is started. This will start the chopper running
//! for this winding.
//!
//! For fast current decay, one side of the H-bridge is set high, and the
//! other side is set low, to cause current to flow in the positive or
//! negative direction. The gate driver enable signal is then turned on
//! or off to control the current. When the enable signal is on, bus voltage
//! is applied to the winding and current flows. When the enable signal is
//! off, all the H-bridge switches are open and the current in the winding
//! decays rapidly.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlChopFast(unsigned long ulWinding, long lSetting)
{
    tWinding *pWinding;
    pWinding = &sWinding[ulWinding];

    //
    // Disable the fixed interval timer so it can be updated.
    // Set the load register to the off blanking time.
    // Equivalent DriverLib call:
    //  TimerDisable(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>);
    //  TimerLoadSet(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>, g_usBlankOffTime);
    //
    HWREG(FIXED_TMR_BASE + TIMER_O_CTL) &= ~pWinding->ulTmrEnaVal;
    HWREG(pWinding->ulTmrLoadAddr) = g_usBlankOffTime;

    //
    // Save the PWM register address and value that will be used by the
    // ADC and timer ISRs to control the signal on the control pin.
    //
    pWinding->ulPwmGenCtlReg =
                WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB;
    pWinding->ulPwmGenCtlVal = CTRL_PIN_ON_VAL;

    //
    // Winding current is positive, set the negative side low, and the
    // positive side high, and start chopping the enable signal.
    //
    if(lSetting > 0)
    {
        //
        // Save the chopping current threshold.
        //
        pWinding->ulChopperCurrent = lSetting;

        //
        // Set positive side high and negative side low, so the
        // winding is turned on, in the positive direction.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_ON_VAL;

        //
        // Turn on the enable pin so that the H-bridge is enabled and
        // current will begin to flow in the winding.
        //
        HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                                            CTRL_PIN_ON_VAL;

        //
        // Enable the ADC sequencer
        // Equivalent DriverLib call:
        //  ADCSequenceEnable(ADC0_BASE, pWinding->ucADCSeq);
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= 1 << pWinding->ucADCSeq;

        //
        // Trigger ADC to take a sample for chopper comparison.
        // Equivalent DriverLib call:
        //  ADCProcessorTrigger(ADC0_BASE, pWinding->ucADCSeq);
        //
        HWREG(ADC0_BASE + ADC_O_PSSI) = 1 << pWinding->ucADCSeq;
    }

    //
    // Winding current is negative, set the positive side low, and the
    // negative side high, and start chopping the enable signal.
    //
    else if(lSetting < 0)
    {
        //
        // Save the chopping current threshold. Account for sign.
        //
        pWinding->ulChopperCurrent = -lSetting;

        //
        // Set negative side high and positive side low, so the
        // winding is turned on, in the negative direction.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_ON_VAL;

        //
        // Turn on the enable pin so that the H-bridge is enabled and
        // current will begin to flow in the winding.
        //
        HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                                            CTRL_PIN_ON_VAL;

        //
        // Enable the ADC sequencer
        // Equivalent DriverLib call:
        //  ADCSequenceEnable(ADC0_BASE, pWinding->ucADCSeq);
        //
        HWREG(ADC0_BASE + ADC_O_ACTSS) |= 1 << pWinding->ucADCSeq;

        //
        // Trigger ADC to take a sample for chopper comparison.
        // Equivalent DriverLib call:
        //  ADCProcessorTrigger(ADC0_BASE, pWinding->ucADCSeq);
        //
        HWREG(ADC0_BASE + ADC_O_PSSI) = 1 << pWinding->ucADCSeq;
    }

    //
    // Winding current is 0. Set both sides low, and stop chopping on
    // the enable signal, leaving the enable signal off.
    //
    else
    {
        //
        // Set the chopping current threshold to 0.
        //
        pWinding->ulChopperCurrent = 0;

        //
        // Set both positive and negative sides low, so that no
        // voltage is applied to the winding, and turn off the
        // winding enable pin. This will have the effect of opening
        // all the switches.
        //
        pWinding->ulPwmGenCtlReg = 0;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
        HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                CTRL_PIN_OFF_VAL;
    }
}

//*****************************************************************************
//
//! Sets up a step using Open-loop PWM mode and slow decay.
//!
//! \param ulWinding is the winding ID (A or B)
//! \param lSetting is the duration that the signal is on, in system
//!                 clock ticks (signed to indicate polarity)
//!
//! This function will configure the PWM to control the pins needed
//! for slow current decay. It drives the winding positive or negative
//! (or off), according to the value and sign of the lSetting parameter.
//! Once the control signals are set to apply voltage to the winding,
//! the fixed timer is started with a timeout value for the fixed rise
//! time. When the fixed timer times out, it will set the PWM generator
//! to start using PWM for the control signal.  There is no feedback from
//! the measured current, which is why this is called open-loop PWM.
//!
//! The lSetting parameter is the duration in system clock ticks, that
//! the PWM output will be on. This will be a fraction of the PWM period,
//! resulting in the PWM duty cycle. It is a signed value, to indicate
//! the direction the current should flow.
//!
//! For slow current decay, one side of the H-bridge is set high, and the
//! other side is set low, to cause current to flow in the positive or
//! negative direction. The gate drivers are always enabled. To control
//! current, the "high" side of the H-bridge is switched between high and
//! low. When it is high, the bus voltage is applied to the winding and
//! current flows. When it is low, then both sides of the H-bridge will be
//! low, and bus voltage is removed, but current continues to circulate in
//! the winding as it decays slowly.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlOpenPwmSlow(unsigned long ulWinding, long lSetting)
{
    tWinding *pWinding;
    pWinding = &sWinding[ulWinding];

    //
    // Disable the fixed interval timer so it can be updated.
    // Set the load register to the fixed on time.
    // Equivalent DriverLib call:
    //  TimerDisable(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>);
    //  TimerLoadSet(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>, g_usFixedOnTime);
    //
    HWREG(FIXED_TMR_BASE + TIMER_O_CTL) &= ~pWinding->ulTmrEnaVal;
    HWREG(pWinding->ulTmrLoadAddr) = g_usFixedOnTime;

    //
    // Winding current is positive
    //
    if(lSetting > 0)
    {
        //
        // Load the PWM comparator register to set the PWM pulse width
        // (duty cycle) according to the lSetting parameter. This sets
        // up the PWM, but the pin is not being switched yet.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_CMPA) =
                                        (g_ulPwmPeriod - lSetting) / 2;

        //
        // Set the negative side low. The positive side will be turned
        // on (below) so that current flows in the winding in the positive
        // direction.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;

        //
        // If the fixed on time is 0, then just set the positive side pin
        // directly to start PWM, and dont bother to start the fixed timer.
        // This will start PWMming on the winding.
        //
        if(g_usFixedOnTime == 0)
        {
            HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_PWMA_VAL;
        }

        //
        // If the fixed on time is set to a non-zero value, then the pin
        // is turned on for a fixed amount of time. The fixed timer is set
        // to time out after the fixed on time. The timer handler will
        // start PWM on the pin.
        //
        else
        {
            //
            // Turn the pin on.
            //
            HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_ON_VAL;

            //
            // Save the PWM register address and value that will be used by the
            // timer ISR to turn the PWM on when the fixed time expires.
            //
            pWinding->ulPwmGenCtlReg = pWinding->ulPwmGenBase + PWM_O_X_GENA;
            pWinding->ulPwmGenCtlVal = CTRL_PIN_PWMA_VAL;

            //
            // Start the fixed interval timer running. When the timer expires
            // it will start PWMming on the high side signal.
            // Equivalent DriverLib call:
            //  TimerEnable(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>);
            //
            HWREG(FIXED_TMR_BASE + TIMER_O_CTL) |= pWinding->ulTmrEnaVal;

            //
            // Set the timer state to indicate fixed on timing.
            //
            pWinding->ucTimerState = TIMER_STATE_FIXED_ON;
        }

    }

    //
    // Winding current is negative.
    //
    else if(lSetting < 0)
    {
        //
        // Load the PWM comparator register to set the PWM pulse width
        // (duty cycle) according to the lSetting parameter. This sets
        // up the PWM, but the pin is not being switched yet.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_CMPA) =
                                        (g_ulPwmPeriod + lSetting) / 2;

        //
        // Set the positive side low. The negative side will be turned
        // on (below) so that current flows in the winding in the negative
        // direction.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;

        //
        // If the fixed on time is 0, then just set the negative side pin
        // directly to start PWM, and dont bother to start the fixed timer.
        // This will start PWMming on the winding.
        //
        if(g_usFixedOnTime == 0)
        {
            HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_PWMA_VAL;
        }

        //
        // If the fixed on time is set to a non-zero value, then the pin
        // is turned on for a fixed amount of time. The fixed timer is set
        // to time out after the fixed on time. The timer handler will
        // start PWM on the pin.
        //
        else
        {
            //
            // Turn the pin on.
            //
            HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_ON_VAL;

            //
            // Save the PWM register address and value that will be used by the
            // timer ISR to turn the PWM on when the fixed time expires.
            //
            pWinding->ulPwmGenCtlReg = pWinding->ulPwmGenBase + PWM_O_X_GENB;
            pWinding->ulPwmGenCtlVal = CTRL_PIN_PWMA_VAL;

            //
            // Start the fixed interval timer running. When the timer expires
            // it will start PWMming on the low side signal.
            // Equivalent DriverLib call:
            //  TimerEnable(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>);
            //
            HWREG(FIXED_TMR_BASE + TIMER_O_CTL) |= pWinding->ulTmrEnaVal;

            //
            // Set the timer state to indicate fixed on timing.
            //
            pWinding->ucTimerState = TIMER_STATE_FIXED_ON;
        }
    }

    //
    // Winding is off
    //
    else
    {
        //
        // Set both positive and negative side low, so that there is
        // no voltage applied to the winding.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
    }

    //
    // Turn on the enable signal for the winding.
    //
    HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                                                            CTRL_PIN_ON_VAL;
}

//*****************************************************************************
//
//! Sets up a step using Open-loop PWM mode and fast decay.
//!
//! \param ulWinding is the winding ID (A or B)
//! \param lSetting is the duration that the signal is on, in system
//!                 clock ticks (signed to indicate polarity)
//!
//! This function will configure the PWM to control the pins needed
//! for fast current decay. It drives the winding positive or negative
//! (or off), according to the value and sign of the lSetting parameter.
//! Once the control signals are set to apply voltage to the winding,
//! the fixed timer is started with a timeout value for the fixed rise
//! time. When the fixed timer times out, it will set the PWM generator
//! to start using PWM for the control signal.  There is no feedback from
//! the measured current in the winding, which is why this is called
//! open-loop PWM.
//!
//! The lSetting parameter is the duration in system clock ticks, that
//! the PWM output will be on. This will be a fraction of the PWM period,
//! resulting in the PWM duty cycle. It is a signed value, to indicate the
//! direction the current should flow.
//!
//! For fast current decay, one side of the H-bridge is set high, and the
//! other side is set low, to cause current to flow in the positive or
//! negative direction. The gate driver enable signal is then turned on
//! or off to control the current. When the enable signal is on, bus voltage
//! is applied to the winding and current flows. When the enable signal is
//! off, all the H-bridge switches are open and the current in the winding
//! decays rapidly.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlOpenPwmFast(unsigned long ulWinding, long lSetting)
{
    tWinding *pWinding;
    pWinding = &sWinding[ulWinding];

    //
    // Disable the fixed interval timer so it can be updated.
    // Set the load register to the fixed on time.
    // Equivalent DriverLib call:
    //  TimerDisable(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>);
    //  TimerLoadSet(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>, g_usFixedOnTime);
    //
    HWREG(FIXED_TMR_BASE + TIMER_O_CTL) &= ~pWinding->ulTmrEnaVal;
    HWREG(pWinding->ulTmrLoadAddr) = g_usFixedOnTime;

    //
    // Winding current is positive
    //
    if(lSetting > 0)
    {
        //
        // Set positive side high and negative side low, so the
        // winding is turned on, in the positive direction.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_ON_VAL;

        //
        // Set the PWM pulse width for the winding enable signal, but
        // dont start PWM yet.
        //
        HWREG(WINDING_EN_GEN_BASE + PWM_O_X_CMPA + pWinding->ulPwmAB) =
                                            (g_ulPwmPeriod - lSetting) / 2;

        //
        // If the fixed on time is 0, then just set the enable pin for
        // the winding directly to start PWM, and don't bother to start
        // the fixed timer. This will start PWMming on the winding
        // enable signal.
        //
        if(g_usFixedOnTime == 0)
        {
            HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                    (ulWinding == WINDING_ID_A) ?  CTRL_PIN_PWMA_VAL :
                                                   CTRL_PIN_PWMB_VAL;
        }

        //
        // If the fixed on time is set to a non-zero value, then the
        // winding enable pin is turned on for a fixed amount of time.
        // The fixed timer is set to time out after the fixed on time.
        // The timer handler will start PWM on the enable pin.
        //
        else
        {
            //
            // Turn the enable pin on.
            //
            HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                                                CTRL_PIN_ON_VAL;

            //
            // Save the PWM register address and value that will be used by the
            // timer ISR to turn the PWM on when the fixed time expires.
            //
            pWinding->ulPwmGenCtlReg =
                        WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB;
            pWinding->ulPwmGenCtlVal =
                        (ulWinding == WINDING_ID_A) ?  CTRL_PIN_PWMA_VAL :
                                                       CTRL_PIN_PWMB_VAL;

            //
            // Start the fixed interval timer running. When the timer expires
            // it will start PWMming the enable signal.
            // Equivalent DriverLib call:
            //  TimerEnable(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>);
            //
            HWREG(FIXED_TMR_BASE + TIMER_O_CTL) |= pWinding->ulTmrEnaVal;

            //
            // Set the timer state to indicate fixed on timing.
            //
            pWinding->ucTimerState = TIMER_STATE_FIXED_ON;
        }
    }

    //
    // Winding current is negative.
    //
    else if(lSetting < 0)
    {
        //
        // Set negative side high and positive side low, so the
        // winding is turned on, in the negative direction.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_ON_VAL;

        //
        // Set the PWM pulse width for the winding enable signal, but
        // dont start PWM yet.
        //
        HWREG(WINDING_EN_GEN_BASE + PWM_O_X_CMPA + pWinding->ulPwmAB) =
                                            (g_ulPwmPeriod + lSetting) / 2;

        //
        // If the fixed on time is 0, then just set the enable pin for
        // the winding directly to start PWM, and don't bother to start
        // the fixed timer. This will start PWMming on the winding
        // enable signal.
        //
        if(g_usFixedOnTime == 0)
        {
            HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                    (ulWinding == WINDING_ID_A) ?  CTRL_PIN_PWMA_VAL :
                                                   CTRL_PIN_PWMB_VAL;
        }

        //
        // If the fixed on time is set to a non-zero value, then the
        // winding enable pin is turned on for a fixed amount of time.
        // The fixed timer is set to time out after the fixed on time.
        // The timer handler will start PWM on the enable pin.
        //
        else
        {
            //
            // Turn the enable pin on.
            //
            HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                                                CTRL_PIN_ON_VAL;

            //
            // Save the timer register address and value that will be used by the
            // timer ISR to turn the PWM on when the fixed time expires.
            //
            pWinding->ulPwmGenCtlReg =
                        WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB;
            pWinding->ulPwmGenCtlVal =
                        (ulWinding == WINDING_ID_A) ?  CTRL_PIN_PWMA_VAL :
                                                       CTRL_PIN_PWMB_VAL;

            //
            // Start the fixed interval timer running.
            // Equivalent DriverLib call:
            //  TimerEnable(FIXED_TMR_BASE, <TIMER_A OR TIMER_B>);
            //
            HWREG(FIXED_TMR_BASE + TIMER_O_CTL) |= pWinding->ulTmrEnaVal;

            //
            // Set the timer state to indicate fixed on timing.
            //
            pWinding->ucTimerState = TIMER_STATE_FIXED_ON;
        }
    }

    //
    // Winding is off
    //
    else
    {
        //
        // Set both positive and negative sides low, so that no
        // voltage is applied to the winding, and turn off the
        // winding enable pin. This will have the effect of opening
        // all the switches.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
        HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                CTRL_PIN_OFF_VAL;
    }
}

//*****************************************************************************
//
//! Sets up a step using Closed-loop PWM mode and slow decay.
//!
//! \param ulWinding is the winding ID (A or B)
//! \param lSetting is the target winding current (signed), in raw ADC counts
//!
//! This function will configure the PWM to control the pins needed for
//! slow current decay.  It sets the winding for positive or negative
//! current, depending on the value of the lSetting parameter.  The high
//! side will be set to switch on and off according to a PWM generator.
//! The pulse width will be controlled by the ADC handler which will measure
//! the actual current and adjust the PWM pulse width accordingly.
//!
//! For slow current decay, one side of the H-bridge is set high, and the
//! other side is set low, to cause current to flow in the positive or
//! negative direction. The gate drivers are always enabled. To control
//! current, the "high" side of the H-bridge is switched between high and
//! low. When it is high, the bus voltage is applied to the winding and
//! current flows. When it is low, then both sides of the H-bridge will be
//! low, and bus voltage is removed, but current continues to circulate in
//! the winding as it decays slowly.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlClosedPwmSlow(unsigned long ulWinding, long lSetting)
{
    unsigned long ulSeqTrigger;
    tWinding *pWinding;
    pWinding = &sWinding[ulWinding];

    //
    // Stop ADC handler from doing anything while the current is being
    // reconfigured.
    //
    pWinding->ulPwmGenCtlReg = 0;

    //
    // Initialize the pulse width to minimum, prevents an initial overcurrent.
    // Equivalent DriverLib call:
    //  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_n, MIN_PWM_COUNTS);
    //
    HWREG(pWinding->ulPwmGenBase + PWM_O_X_CMPA) =
                                    (g_ulPwmPeriod - MIN_PWM_COUNTS) / 2;

    //
    // Winding current is positive, set the negative side low,
    // and PWM on the positive side.
    //
    if(lSetting > 0)
    {
        //
        // Save the target current threshold.
        //
        pWinding->ulChopperCurrent = lSetting;

        //
        // Set positive side to PWM using comparator A, and the negative
        // side low, so the winding is turned on in the positive direction.
        // This will apply positive, PWM'd voltage.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_PWMA_VAL;

        //
        // Save the address of the PWM comparator used to adjust the pulse
        // width.  This will be used by the ADC handler to adjust the
        // duty cycle according to the measured current.
        //
        pWinding->ulPwmGenCtlReg = pWinding->ulPwmGenBase + PWM_O_X_CMPA;
    }

    //
    // Winding current is negative, set the positive side low,
    // and PWm on the negative side.
    //
    else if(lSetting < 0)
    {
        //
        // Save the target current threshold.
        //
        pWinding->ulChopperCurrent = -lSetting;

        //
        // Set negative side to PWM using comparator A, and the positive
        // side low, so the winding is turned on in the negative direction.
        // This will apply negative, PWM'd voltage.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_PWMA_VAL;

        //
        // Save the address of the PWM comparator used to adjust the pulse
        // width.  This will be used by the ADC handler to adjust the
        // duty cycle according to the measured current.
        //
        pWinding->ulPwmGenCtlReg = pWinding->ulPwmGenBase + PWM_O_X_CMPA;
    }

    //
    // Winding current is 0, set both sides low.
    //
    else
    {
        //
        // Set the target current threshold to 0.
        //
        pWinding->ulChopperCurrent = 0;

        //
        // Set both positive and negative side low, so that there is
        // no voltage applied to the winding.
        //
        pWinding->ulPwmGenCtlReg = 0;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
    }

    //
    // Turn on the enable signal for the winding. ulPwmAB is used to
    // select the correct half (A or B) of the PWM generator for the
    // winding enable signals. For slow decay, the enable signal is
    // always left on.
    //
    HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
            CTRL_PIN_ON_VAL;

    //
    // Set the ADC sequencer for this winding to trigger on the PWM
    // generator.  This will cause the ADC to trigger acquisitions based
    // on the falling edge of comparator B.
    //
    // Get the current register value for sequencer triggers.
    //
    ulSeqTrigger = HWREG(ADC0_BASE + ADC_O_EMUX) &
                        ~(0x0f << (pWinding->ucADCSeq * 4));

    //
    // Figure out which PWM trigger to use.
    //
    if(ulWinding == WINDING_ID_A)
    {
        ulSeqTrigger |= ADC_TRIGGER_PWM0 << (pWinding->ucADCSeq * 4);
    }
    else
    {
        ulSeqTrigger |= ADC_TRIGGER_PWM1 << (pWinding->ucADCSeq * 4);
    }

    //
    // Save the new trigger value.
    //
    HWREG(ADC0_BASE + ADC_O_EMUX) = ulSeqTrigger;
}

//*****************************************************************************
//
//! Sets up a step using Closed-loop PWM mode and fast decay.
//!
//! \param ulWinding is the winding ID (A or B)
//! \param lSetting is the target winding current (signed), in raw ADC counts
//!
//! This function will configure the PWM to control the pins needed for
//! fast current decay.  Its sets the winding gate drivers for positive or
//! negative current depending on the lSetting parameter.  Then the enable
//! signal for the winding is set to be switched on and off by a PWM
//! generator.  The pulse width will be controlled by the ADC handler which
//! will measure the actual current and adjust the PWM pulse width accordingly.
//!
//! For fast current decay, one side of the H-bridge is set high, and the
//! other side is set low, to cause current to flow in the positive or
//! negative direction. The gate driver enable signal is then turned on
//! or off to control the current. When the enable signal is on, bus voltage
//! is applied to the winding and current flows. When the enable signal is
//! off, all the H-bridge switches are open and the current in the winding
//! decays rapidly.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlClosedPwmFast(unsigned long ulWinding, long lSetting)
{
    unsigned long ulSeqTrigger;
    tWinding *pWinding;
    pWinding = &sWinding[ulWinding];

    //
    // Stop ADC handler from doing anything while the current is being
    // reconfigured.
    //
    pWinding->ulPwmGenCtlReg = 0;

    //
    // Initialize the pulse width to minimum, prevents an initial overcurrent.
    // Equivalent DriverLib call:
    //  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_n, MIN_PWM_COUNTS);
    //
    HWREG(WINDING_EN_GEN_BASE + PWM_O_X_CMPA + pWinding->ulPwmAB) =
                                    (g_ulPwmPeriod - MIN_PWM_COUNTS) / 2;

    //
    // Winding current is positive
    //
    if(lSetting > 0)
    {
        //
        // Save the target current threshold.
        //
        pWinding->ulChopperCurrent = lSetting;

        //
        // Set positive side high and negative side low, so the
        // winding is turned on, in the positive direction.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_ON_VAL;

        //
        // Set the enable pin to PWM, based on comparator A or B (for
        // winding A or B).
        //
        HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                    (ulWinding == WINDING_ID_A) ?  CTRL_PIN_PWMA_VAL :
                                                   CTRL_PIN_PWMB_VAL;

        //
        // Save the address of the PWM comparator used to adjust the pulse
        // width.  This will be used by the ADC handler to adjust the
        // duty cycle according to the measured current.
        //
        pWinding->ulPwmGenCtlReg = WINDING_EN_GEN_BASE + PWM_O_X_CMPA +
                                    pWinding->ulPwmAB;
    }

    //
    // Winding current is negative.
    //
    else if(lSetting < 0)
    {
        //
        // Save the target current threshold.
        //
        pWinding->ulChopperCurrent = -lSetting;

        //
        // Set negative side high and positive side low, so the
        // winding is turned on, in the negative direction.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_ON_VAL;

        //
        // Set the enable pin to PWM, based on comparator A or B (for
        // winding A or B).
        //
        HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                (ulWinding == WINDING_ID_A) ?  CTRL_PIN_PWMA_VAL :
                                               CTRL_PIN_PWMB_VAL;

        //
        // Save the address of the PWM comparator used to adjust the pulse
        // width.  This will be used by the ADC handler to adjust the
        // duty cycle according to the measured current.
        //
        pWinding->ulPwmGenCtlReg = WINDING_EN_GEN_BASE + PWM_O_X_CMPA +
                                    pWinding->ulPwmAB;
    }

    //
    // Winding is off
    //
    else
    {
        //
        // Set both positive and negative sides low, so that no
        // voltage is applied to the winding, and turn off the
        // winding enable pin. This will have the effect of opening
        // all the switches.
        //
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENA) = CTRL_PIN_OFF_VAL;
        HWREG(pWinding->ulPwmGenBase + PWM_O_X_GENB) = CTRL_PIN_OFF_VAL;
        HWREG(WINDING_EN_GEN_BASE + PWM_O_X_GENA + pWinding->ulPwmAB) =
                CTRL_PIN_OFF_VAL;
    }

    //
    // Set the ADC sequencer for this winding to trigger on the PWM
    // generator.  This will cause the ADC to trigger acquisitions based
    // on the center of the PWM pulse on the enable line (because PWM_GEN_2
    // was configured for trigger on load).
    //
    // Get the current register value for sequencer triggers.
    //
    ulSeqTrigger = HWREG(ADC0_BASE + ADC_O_EMUX) &
                        ~(0x0f << (pWinding->ucADCSeq * 4));

    //
    // Set to trigger on PWM 2 (the PWM generator used for the enable
    // signals).
    //
    ulSeqTrigger |= ADC_TRIGGER_PWM2 << (pWinding->ucADCSeq * 4);

    //
    // Save the new trigger value.
    //
    HWREG(ADC0_BASE + ADC_O_EMUX) = ulSeqTrigger;
}

//*****************************************************************************
//
//! Initializes the step control module.
//!
//! This function initializes all the peripherals used by this module
//! for controlling the stepper motor.
//!
//! \return None.
//
//*****************************************************************************
void
StepCtrlInit(void)
{
    //
    // Enable the PWM peripheral block, and the GPIO ports associated
    // with the PWM pins.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(FIXED_TMR_PERIPH);

    //
    // Enable the ADC peripheral, needed for winding current.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    //
    // Set the ADC to run at the maximum rate of 500 ksamples.
    //
    SysCtlADCSpeedSet(SYSCTL_ADCSPEED_500KSPS);

    //
    // Enable the peripherals that should continue to run when the processor
    // is sleeping.
    //
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralSleepEnable(FIXED_TMR_PERIPH);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_ADC0);

    //
    // Set up the timer used for the fixed interval timer.
    //
    TimerConfigure(FIXED_TMR_BASE, TIMER_CFG_SPLIT_PAIR |
                        TIMER_CFG_A_ONE_SHOT | TIMER_CFG_B_ONE_SHOT);
    TimerControlStall(FIXED_TMR_BASE, TIMER_BOTH, 1);
    TimerPrescaleSet(FIXED_TMR_BASE, TIMER_BOTH, 50);
    TimerIntEnable(FIXED_TMR_BASE, TIMER_TIMA_TIMEOUT | TIMER_TIMB_TIMEOUT);
    IntEnable(FIXED_TMR_INT_A);
    IntEnable(FIXED_TMR_INT_B);
    IntPrioritySet(FIXED_TMR_INT_A, FIXED_TMR_INT_PRI);
    IntPrioritySet(FIXED_TMR_INT_B, FIXED_TMR_INT_PRI);

    //
    // Initialize the ADC sequencers for windings A and B.
    // There is one sequencer for each winding. Configure each to take
    // two samples and generate an interrupt.
    // Two samples are used for each, because the first sample taken by
    // the ADC after the winding is first turned on, occurs too fast
    // for an accurate measurement to be made. By using two samples at
    // each acquisition, it is guaranteed that the second sample will be
    // an accurate measurement of the current.
    //
    ADCSequenceConfigure(ADC0_BASE, WINDING_A_ADC_SEQUENCER,
                         ADC_TRIGGER_PROCESSOR, WINDING_A_ADC_PRIORITY);
    ADCSequenceStepConfigure(ADC0_BASE, WINDING_A_ADC_SEQUENCER, 0,
                             WINDING_A_ADC_CHANNEL | ADC_CTL_END | ADC_CTL_IE);

    ADCSequenceConfigure(ADC0_BASE, WINDING_B_ADC_SEQUENCER,
                         ADC_TRIGGER_PROCESSOR, WINDING_B_ADC_PRIORITY);
    ADCSequenceStepConfigure(ADC0_BASE, WINDING_B_ADC_SEQUENCER, 0,
                             WINDING_B_ADC_CHANNEL | ADC_CTL_END | ADC_CTL_IE);

    //
    // Enable the ADC sequencers, and enable the interrupts.
    //
    ADCSequenceEnable(ADC0_BASE, WINDING_A_ADC_SEQUENCER);
    ADCIntEnable(ADC0_BASE, WINDING_A_ADC_SEQUENCER);
    IntEnable(WINDING_A_ADC_INT);
    IntPrioritySet(WINDING_A_ADC_INT, ADC_INT_PRI);

    ADCSequenceEnable(ADC0_BASE, WINDING_B_ADC_SEQUENCER);
    ADCIntEnable(ADC0_BASE, WINDING_B_ADC_SEQUENCER);
    IntEnable(WINDING_B_ADC_INT);
    IntPrioritySet(WINDING_B_ADC_INT, ADC_INT_PRI);

    //
    // Initialize all of the PWM generators.
    //
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN |
                    PWM_GEN_MODE_NO_SYNC | PWM_GEN_MODE_DBG_STOP);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_UP_DOWN |
                    PWM_GEN_MODE_NO_SYNC | PWM_GEN_MODE_DBG_STOP);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_UP_DOWN |
                    PWM_GEN_MODE_NO_SYNC | PWM_GEN_MODE_DBG_STOP);

    //
    // PWM 4 and 5 pins are connected to the gate driver enable pins,
    // which are active low, so set them to invert.
    //
    PWMOutputInvert(PWM0_BASE, PWM_OUT_4_BIT | PWM_OUT_5_BIT, 1);

    //
    // Enable all the PWM generators.
    //
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);

    //
    // Configure the PWM pins to be controlled by the PWM generators.
    //
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Start off in chopper mode, by default
    //
    StepCtrlChopMode();

    //
    // Configure PWM outputs to safe on fault.
    // Enable the PWM outputs.
    //
    PWMOutputState(PWM0_BASE,
                   PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT |
                   PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT, 1);
    PWMOutputFault(PWM0_BASE,
                   PWM_OUT_0_BIT | PWM_OUT_1_BIT | PWM_OUT_2_BIT |
                   PWM_OUT_3_BIT | PWM_OUT_4_BIT | PWM_OUT_5_BIT, 1);
}

//*****************************************************************************
//
// Close the Doxyen group.
//! @}
//
//*****************************************************************************
