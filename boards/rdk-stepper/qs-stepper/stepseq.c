//*****************************************************************************
//
// stepseq.c - Step sequencing module.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the RDK-STEPPER Firmware Package.
//
//*****************************************************************************

#include <limits.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "utils/isqrt.h"
#include "commands.h"
#include "stepcfg.h"
#include "stepctrl.h"
#include "stepseq.h"

//*****************************************************************************
//
//! \page stepseq_intro Introduction
//!
//! The Step Sequencer module is used for generating the steps in the
//! correct sequence, and with the correct timing to cause the motor to
//! run in the desired speed and direction. Whenever any of the driving
//! parameters (position, speed, accel, decel) is changed, a speed profile
//! is computed which will cause the motor to accelerate to the running
//! speed, and then decelerate to a stop at the target position. As the
//! motor is running, the time between steps is calculated in real time
//! in order to achieve the acceleration and deceleration ramps.
//!
//! Normally, there is no reason that an application needs to call any of
//! these functions directly, nor make direct access to any of the module
//! global variables. The following explains how this module is used by
//! the Stepper API module.
//!
//! First, the module is initialized by calling StepSeqInit(). Then, the
//! following functions are called to configure the operation of the
//! stepper motor: StepSeqControlMode(), StepSeqStepMode(), StepSeqDecayMode(),
//! and StepSeqCurrent(). These are used to set up the winding current
//! control method, the stepping mode, the decay mode, and the driving
//! current.
//!
//! When it is time to make the stepper motor move, the function
//! StepSeqMove() is called. This function will compute the speed
//! profile based on the input parameters, and the current motor status.
//! If the motor is already running, then a new speed profile is calculated
//! to satisfy the new motion request, which can include real-time speed
//! changes.
//!
//! If the motor is running, it can be stopped gracefully by calling
//! StepSeqStop(). This will decelerate the motor from whatever speed it
//! is running to a full stop using the last specified deceleration value.
//! When the motor is stopped this way, position knowledge is retained.
//! If the motor must be stopped immediately, then StepSeqShutdown() can be
//! used. This function will immediately disable the motor control signals
//! and stop the stepping sequence. When this happens, the known position
//! information may no longer be accurate.
//!
//! The StepSeqHandler() function is the interrupt handler for the
//! step timer. It is called at each step or half step time to generate
//! the next step in the sequence.
//!
//! <b>Optimizations</b>
//!
//! The StepSeqMove() function is somewhat complicated in order to
//! handle all of the different cases of the new motion requested
//! and the current status of the motor. The simple case is when the motor
//! is stopped. Calculating a new speed profile is easy in this case.
//! But if the motor is already moving, then it becomes more complicated.
//! For example, the motor may need to speed up or slow down from one
//! running speed to a new running speed, as well as change the target
//! position. Or, the new target position may be in the opposite direction
//! from the present direction, which means the motor needs to be decelerated
//! to a stop, and then run in the other direction.
//!
//! In a real application, it may be reasonable to put some constraints
//! on the need to dynamically change the motion parameters of a running
//! motor. For example, it may be completely reasonable to require that
//! the motor be stopped before a new motion command is issued. In this case,
//! a large amount of the code in StepSeqMove() could be removed to save
//! code space and complexity. This is a potential optimization that could
//! be made by the user.
//!
//! An optimization that was made in the code is the the use of direct
//! access to peripheral registers instead of making calls to the
//! DriverLib peripheral library. This is done in the step timer interrupt
//! handler in order to make the interrupt code execute just a bit faster.
//! Whenever a register access is made like this, there is a comment in
//! the code showing the equivalent DriverLib call.
//!
//! It is likely that an actual stepper application will use only one of
//! the three current control methods (Open-loop PWM, closed-loop PWM, or
//! chopper mode).  In this case, the user could modify this module to
//! remove the code that is related to the unused control method. One of
//! the functions such as StepSeqUsingOpenPwm() or StepSeqUsingChop() could
//! be removed along with conditional code that refers to these functions.
//! These changes would make the resulting code somewhat smaller and more
//! efficient to execute.
//!
//! <b>Step Sequencing</b>
//!
//! The Step Sequencer determines how the current must be set in each of the
//! motor windings (A and B) in order to cause the motor to step in the
//! correct direction, and must make each step at a certain time in order
//! to make the motor run at the correct speed.
//!
//! A step sequence table like that shown below is used to determine how
//! much current should be applied to each winding at each point in the
//! stepping sequence. There are 4 full steps for a complete stepping
//! cycle. In an ideal environment, the current waveform to drive the motor
//! through one full cycle (of 4 full steps) is a sine wave in each winding,
//! 90 degrees out of phase. This waveform is represented as a series
//! of "micro-steps" in the step sequence table. The table covers 4 full
//! steps, each further divided into 8 micro-steps, for a total of 32
//! micro-steps in a full stepping cycle.
//!
//! \verbatim
//!             Stepping Sequence
//!          +----------------------+
//!          |Step |    A       B   |
//!          |----------------------|
//!     F--> | 0-4 |  46341   46341 |
//!          | 0-5 |  54491   36410 |
//!          | 0-6 |  60547   25080 |
//!          | 0-7 |  64277   12785 |
//!     H--> | 1-0 |  65536       0 |
//!          | 1-1 |  64277  -12785 |
//!          | 1-2 |  60547  -25080 |
//!          | 1-3 |  54491  -36410 |
//!     F--> | 1-4 |  46341  -46341 |
//!          | 1-5 |  36410  -54491 |
//!          | 1-6 |  25080  -60547 |
//!          | 1-7 |  12785  -64277 |
//!     H--> | 2-0 |      0  -65536 |
//!          | 2-1 | -12785  -64277 |
//!          | 2-2 | -25080  -60547 |
//!          | 2-3 | -36410  -54491 |
//!     F--> | 2-4 | -46341  -46341 |
//!          | 2-5 | -54491  -36410 |
//!          | 2-6 | -60547  -25080 |
//!          | 2-7 | -64277  -12785 |
//!     H--> | 3-0 | -65536       0 |
//!          | 3-1 | -64277   12785 |
//!          | 3-2 | -60547   25080 |
//!          | 3-3 | -54491   36410 |
//!     F--> | 3-4 | -46341   46341 |
//!          | 3-5 | -36410   54491 |
//!          | 3-6 | -25080   60547 |
//!          | 3-7 | -12785   64277 |
//!     H--> | 0-0 |      0   65536 |
//!          | 0-1 |  12785   64277 |
//!          | 0-2 |  25080   60547 |
//!          | 0-3 |  36410   54491 |
//!          +----------------------+
//! \endverbatim
//!
//! The current for the winding is computed by multiplying the drive
//! current (which is the maximum current to use) by the value from the
//! table (a signed value), and then dividing by 65536. The result is a
//! waveform that varies sinusoidally as the motor steps through a full
//! cycle, from 0 to the positive and negative values of the drive current.
//!
//! If the full stepping mode is used, then only those entries marked with
//! "F" are used. If half stepping mode is used, then those entries
//! marked with "F" and "H" are used. If micro-stepping mode is used, then
//! all the entries are used.
//!
//! Looking at the entries marked with "F", you can see that both windings
//! are on at each whole step point.  This is normal whole stepping.  However,
//! whole stepping could also be performed by using just the entries marked
//! with "H", in which case only one winding is on at each step point.  This
//! is called "wave" drive and is also an option for driving the stepper.
//!
//! Micro-stepping is appropriate for low step rates and allows the motor
//! to move more smoothly between full step points. Micro-stepping places
//! a greater burden on the CPU because the current in the windings must be
//! recomputed and changed more often. As the step rate increases the
//! benefit of using micro-stepping is reduced and half or full stepping
//! should be used instead.
//!
//! The speed of the motor is determined by the time between steps (the
//! "step time"). The step time is the time between full, half, or
//! micro-steps, depending on which mode is used. The shorter the
//! step time, the faster the motor runs. In the simplest case
//! of constant speed, the step time is just the inverse of the motor
//! speed expressed as a step rate (in steps/sec). For acceleration and
//! deceleration, the instantaneous speed changes at every step time, so
//! the step time for the next step must be computed at every step. The
//! method for computing speed profiles is taken from the article,
//! <i>"Generate stepper-motor speed profiles in real time"</i> by
//! David Austin, Embedded Systems Design, Jan 2005. The article is
//! available at http://www.embedded.com/.
//!
//! Refer to the figure below for a typical speed profile. In this case
//! the motor is stopped. The number of steps to accelerate to the
//! running speed and decelerate back to 0 are computed (accel and decel
//! can be different rates). Once the number of steps for accel and
//! decel are known, then the absolute position of the profile transition
//! points can be computed. The transition points are used at each step
//! time to determine if the method for computing step time needs to
//! change. For example, as the motor is stepping to each position toward
//! the stop position, when it reaches the "Decel Pos", it will change
//! the step time calculation from a constant to a gradually increasing
//! step time at each step (thus causing the motor to slow).
//!
//! \verbatim
//!                              Speed Profile
//!              Speed
//!                ^
//!                |
//!                |
//!     Run Speed -|- - - - - - ***********************
//!                |          * |                     |*
//!                |        *   |                     | *
//!                |      *     |                     |  *
//!                |    *       |                     |   *
//!     Stopped(0)-+----|-------|---------------------|---|-----> Position
//!                     |       |                     |   |
//!                     |     Run Pos                 |  Stop Pos
//!                     |                             |
//!                 Accel Pos                     Decel Pos
//! \endverbatim
//!
//! If the motor is already moving, then computing the speed profile
//! becomes more complex. The algorithm must take into account whether
//! the motor is in accel, run, or decel phases, whether the new speed
//! is higher or lower than the current speed, whether the accel and decel
//! rates have changed, and whether the new position can be reached given
//! the current position, speed, and deceleration rate.
//!
//! In the Step Sequencer module, the main work of computing the speed
//! profile is done by the StepSeqMove() function. The output of this
//! function is the calculation of the speed profile transition points.
//! The function StepSeqHandler() performs the actual stepping of the motor.
//! This function is an interrupt handler that is triggered when the
//! hardware timer that is used as a step timer, times out. Upon entry,
//! StepSeqHandler() advances by one full, half, or micro step through the
//! step sequence table, in the appropriate direction, to determine
//! the winding settings for the next step. It then calls into the
//! Step Control module to cause the winding current to change. Then the
//! new position is compared against the speed profile transition points
//! to determine the phase of the speed profile (the "motor status", which
//! is stop, accel, decel, or run). This is used to compute the step
//! time of the next step, and the step timer is started up again. Thus,
//! the step sequencing continues until the stop position is reached.
//!
//! The code for implementing the Step Sequencer is contained in
//! <tt>stepseq.c</tt>, with <tt>stepseq.h</tt> containing the definitions
//! for the variables and functions exported to the application.
//
//*****************************************************************************

//*****************************************************************************
//
//! \defgroup stepseq_api Definitions
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! An index which will select the drive current setting for applying
//! current to the windings.
//
//*****************************************************************************
#define DRIVE_CURRENT   0

//*****************************************************************************
//
//! An index which will select the holding current setting for applying
//! current to the windings.
//
//*****************************************************************************
#define HOLD_CURRENT    1

//*****************************************************************************
//
//! This is the step sequencing table. It is used to determine, at each
//! step time, the polarity and value that should be applied to the winding.
//! This table has entries for 8 microsteps per step, giving 32 microsteps
//! for one stepping cycle. It can be used for micro, half, or whole
//! stepping. Each entry going down represents a micro step, and the
//! columns are the windings, A and B. For whole stepping, only the entries
//! at 0, 8, 16, and 24 are used. For half stepping 0, 4, 8, 12, 16, 20,
//! 24, and 28 are used.
//
//*****************************************************************************
int nMicroStepTable[][2] =
{
    {  46341,    46341 },   // + +
    {  54491,    36410 },
    {  60547,    25080 },
    {  64277,    12785 },

    {  65536,    0     },   // + 0
    {  64277,   -12785 },
    {  60547,   -25080 },
    {  54491,   -36410 },

    {  46341,   -46341 },   // + -
    {  36410,   -54491 },
    {  25080,   -60547 },
    {  12785,   -64277 },

    {  0,       -65536 },   // 0 -
    { -12785,   -64277 },
    { -25080,   -60547 },
    { -36410,   -54491 },

    { -46341,   -46341 },   // - -
    { -54491,   -36410 },
    { -60547,   -25080 },
    { -64277,   -12785 },

    { -65536,        0 },   // - 0
    { -64277,    12785 },
    { -60547,    25080 },
    { -54491,    36410 },

    { -46341,    46341 },   // - +
    { -36410,    54491 },
    { -25080,    60547 },
    { -12785,    64277 },

    {      0,    65536 },   // 0 +
    {  12785,    64277 },
    {  25080,    60547 },
    {  36410,    54491 }
};

//*****************************************************************************
//
//! This holds the value from the step sequence table from the prior step.
//! It is used to see if the setting for a particular winding has changed
//! since the previous step. This is used to avoid extra calls to the
//! stepping functions if there has been no change to the winding.
//
//*****************************************************************************
static int nPrevStepLevel[2] =
{
    0, 0
};

//*****************************************************************************
//
//! The minimum stepping time in system clock ticks. This is computed
//! based on target running speed, and represents the step time once the
//! motor is running at full speed. This value is in fixed-point 24.8 format.
//
//*****************************************************************************
static unsigned long ulMinStepTime;

//*****************************************************************************
//
//! The step time for the zeroth step. This is the time until the motor
//! is first moved. To maintain precision, the step times are kept in
//! fixed-point 24.8 format, until they are actually written to the timer.
//
//*****************************************************************************
static unsigned long ulStep0Time;

//*****************************************************************************
//
//! The step time for the first step in the acceleration profile.. This is
//! the time between the first and second movements of the motor. The first
//! step time is a special case and is pre-computed. All following step times
//! during acceleration are computed on the fly. To maintain precision, all
//! step times are kept in fixed-point 24.8 format, until they are used.
//
//*****************************************************************************
static unsigned long ulStep1Time;

//*****************************************************************************
//
//! The denominator of the equation used for calculating step times
//! during acceleration. This is the denominator value used for the first
//! acceleration step.
//
//*****************************************************************************
static unsigned long ulAccelDenom;

//*****************************************************************************
//
//! The denominator of the equation used for calculating step times
//! during deceleration. This is the denominator value used for the first
//! deceleration step.
//
//*****************************************************************************
static unsigned long ulDecelDenom;

//*****************************************************************************
//
//! The denominator of the equation used for calculating step times
//! during acceleration or deceleration. This value is adjusted at every
//! step time.
//
//*****************************************************************************
static unsigned long ulDenom;

//*****************************************************************************
//
//! The step position at which an acceleration should begin. This will
//! normally be the first step after the current position.
//
//*****************************************************************************
static long lPosAccel;

//*****************************************************************************
//
//! The step position at which the step time should transition to running
//! speed, from a prior acceleration or deceleration. This is computed when
//! the motion is requested by StepSeqMove(), as some future position along
//! the speed profile.
//
//*****************************************************************************
static long lPosRun;

//*****************************************************************************
//
//! The step position at which a deceleration should begin. This is computed
//! when the motion is requested, as some future position along the speed
//! profile.
//
//*****************************************************************************
static long lPosDecel;

//*****************************************************************************
//
//! The step position at which motion should stop. This is computed
//! when the motion is requested, at some future position along the speed
//! profile, and should always be the end of the deceleration curve.
//
//*****************************************************************************
static long lPosStop;

//*****************************************************************************
//
//! The size of each step. This is in fixed-point 24.8 format, which means
//! that a value of 1 whole step is 0x100, and the value of a half-step is
//! 0x080 (one-half is 128/256 or 0x80/0x100). This value is signed to account
//! for direction.  At each step time, this value is added to the current
//! position to advance the current position.
//
//*****************************************************************************
static long lStepDelta = 0x100;

//*****************************************************************************
//
//! The step size mode, STEP_MODE_FULL, STEP_MODE_WAVE, STEP_MODE_HALF,
//! or STEP_MODE_MICRO.
//
//*****************************************************************************
static unsigned char ucStepMode = STEP_MODE_HALF;

//*****************************************************************************
//
//! The current decay mode, DECAY_MODE_SLOW, or DECAY_MODE_FAST.
//
//*****************************************************************************
static unsigned char ucDecayMode = DECAY_MODE_SLOW;

//*****************************************************************************
//
//! The current control method, CONTROL_MODE_CHOP, CONTROL_MODE_OPENPWM,
//! or CONTROL_MODE_CLOSEDPWM.
//
//*****************************************************************************
static unsigned char ucControlMode = CONTROL_MODE_CHOP;

//*****************************************************************************
//
//! This variable holds the value that is used for the PWM on time, when
//! PWM mode is used. This is a value that represents the current that
//! should be applied to the winding, and is computed and set by the
//! StepSeqCurrent() function. The first entry is the value representing
//! the drive current, and the second entry is the value representing the
//! holding current.
//
//*****************************************************************************
static long lPwmSetting[2];

//*****************************************************************************
//
//! This variable holds the value that is used for the chopper current
//! comparison. It is in units of raw ADC counts. This is a value that
//! represents the current that should be applied to the winding, and is
//! computed and set by the StepSeqCurrent() function. The first entry is
//! the value representing the drive current, and the second entry is the
//! value representing the holding current.
//
//*****************************************************************************
static long lChopSetting[2];

//*****************************************************************************
//
//! An index that is used for looking up the current setting to be applied
//! to the winding. It is either DRIVE_CURRENT or HOLD_CURRENT.
//
//*****************************************************************************
static unsigned int uSettingIdx = DRIVE_CURRENT;

//*****************************************************************************
//
//! The value of the drive current in milliamps. This is the value that
//! is passed in from the StepSeqCurrent() function. It is saved because it
//! is used for calculating PWM duty cycle.
//
//*****************************************************************************
static unsigned short usDriveI;

//*****************************************************************************
//
//! The value of the holding current in milliamps. This is the value that
//! is passed in from the StepSeqCurrent() function. It is saved because it
//! is used for calculating PWM duty cycle.
//
//*****************************************************************************
static unsigned short usHoldI;

//*****************************************************************************
//
//! The value of the maximum current in milliamps. This value represents
//! the maximum current that would flow through the winding if it were left
//! on, not the value that should be applied. This is the value that
//! is passed in from the StepSeqCurrent() function. It is saved because it
//! is used for calculating PWM duty cycle.
//
//*****************************************************************************
static unsigned short ulMaxI = USHRT_MAX;

//*****************************************************************************
//
//! The most recent value used for deceleration, in steps/sec^2. It is
//! saved because it is used by the StepSeqStop() function as the deceleration
//! value to use for immediately decelerating the motor to a stop.
//
//*****************************************************************************
static unsigned short usLastDecel;

//*****************************************************************************
//
//! A flag that indicates that the motor is in the process of stopping.
//! This is used to avoid multiple attempts to stop the motor if it is
//! already stopping.
//
//*****************************************************************************
static unsigned char bStopping = 0;

//*****************************************************************************
//
//! A flag that indicates there is a deferred move pending. A deferred
//! move is used when StepSeqMove() is called with a new target position that
//! is not reachable. This is either because the position is "behind" the
//! the current motion direction, or because it is too close ahead of the
//! current position and there are not enough steps left to decelerate to
//! that stopping position. In these cases, the move target is saved
//! as a deferred move that will be made as soon as the motor stops.
//
//*****************************************************************************
static unsigned char bDeferredMove = 0;

//*****************************************************************************
//
//! The target position of the deferred move.
//
//*****************************************************************************
static long lDeferredPosition;

//*****************************************************************************
//
//! The target speed of the deferred move.
//
//*****************************************************************************
static unsigned short usDeferredSpeed;

//*****************************************************************************
//
//! The acceleration to be used for the deferred move.
//
//*****************************************************************************
static unsigned short usDeferredAccel;

//*****************************************************************************
//
//! The deceleration to be used for the deferred move.
//
//*****************************************************************************
static unsigned short usDeferredDecel;

//*****************************************************************************
//
//! The time between each step in system clock ticks. This value
//! is recomputed at each step during acceleration and deceleration,
//! and is fixed when the motor is running at a continuous speed. To
//! maintain precision, this step time is kept in fixed-point 24.8 format for
//! all calculations, and converted to integer format only when it is written
//! to the timer.
//
//*****************************************************************************
unsigned long g_ulStepTime = 0;

//*****************************************************************************
//
//! The current position of the motor. This value is updated every
//! step time. The motor position is kept in fixed-point 24.8 format.
//
//*****************************************************************************
volatile long g_lCurrentPos = 0;

//*****************************************************************************
//
//! The motor status, one of: MOTOR_STATUS_STOP, MOTOR_STATUS_RUN,
//! MOTOR_STATUS_ACCEL, MOTOR_STATUS_DECEL.
//
//*****************************************************************************
unsigned char g_ucMotorStatus = MOTOR_STATUS_STOP;

//*****************************************************************************
//
//! The PWM frequency that will be used for PWM mode. This is used to
//! calculate the PWM period when the StepSeqControlMode() function is
//! called. This value should be set to the desired PWM frequency prior
//! to calling that function.
//
//*****************************************************************************
unsigned short g_usPwmFreq = 20000;

//*****************************************************************************
//
//! Computes the initial value of C which is the step time.
//!
//! \param w is the rate of acceleration, specified in steps per second^2.
//!
//! From David Austin's article, the starting inter-step delay is based on the
//! acceleration.
//!
//! \return Returns the initial value of C in 24.8 fixed-point notation.
//
//*****************************************************************************
#define GET_C0(w)           LongDiv256((SYSTEM_CLOCK * 1.41421356), isqrt(w))

//*****************************************************************************
//
//! Computes the number of steps to accelerate to speed.
//!
//! \param s is the target speed specified in steps per second.
//! \param w is the rate of acceleration specified in steps per second^2.
//!
//! From David Austin's article, the number of steps to accelerate at the given
//! rate up to the given speed. This is used for computing the number of
//! steps for both acceleration and deceleration.
//!
//! \return Returns the number of steps required to reach the given speed at
//! the given constant rate of acceleration.
//
//*****************************************************************************
#define GET_NUM_STEPS(s, w)     ((((s) * (s)) + (w)) / (2 * (w)))

//*****************************************************************************
//
//! Computes the minimum inter-step delay.
//!
//! \param s is the target speed specified in steps per second.
//!
//! From David Austin's article, the minimum inter-step delay based on the
//! maximum speed. This is the constant step time used when the motor
//! is running at speed.
//!
//! \return Returns the minimum value of C in 24.8 fixed-point notation.
//
//*****************************************************************************
#define GET_CMIN(s)             LongDiv256((SYSTEM_CLOCK), (s))

//*****************************************************************************
//
//! Multiplies a number by a fraction specified as a numerator and denominator.
//!
//! \param ulValue is the value to be multiplied.
//! \param ulNum is the numerator of the fraction.
//! \param ulDenom is the denominator of the fraction.
//!
//! This function take an integer and multiplies it by a fraction specified by
//! a numerator and denominator. Assuming infinite precision of the operands,
//! this would be "(ulValue * ulNum) / ulDenom", though with the fixed
//! precision of the processor the multiply would overflow if performed as is.
//!
//! \return The result of the multiplication.
//
//*****************************************************************************
static unsigned long
MulDiv(unsigned long ulValue, unsigned long ulNum, unsigned long ulDenom)
{
    //
    // Perform the multiplication in pieces to maintain the full accuracy and
    // return the result.
    //
    return(((((ulValue / 65536) * ulNum) / ulDenom) * 65536) +
           (((((ulValue / 65536) * ulNum) % ulDenom) * 65536) / ulDenom) +
           (((ulValue % 65536) * ulNum) / ulDenom));
}

//*****************************************************************************
//
//! Divides two numbers, returning the result in 24.8 fixed-point notation.
//!
//! \param ulNum is the numerator for the division.
//! \param ulDenom is the denominator for the division.
//!
//! This function takes two integers and divides them, returning the results in
//! 24.8 fixed-point notation (with full accuracy). Assuming infinite
//! precision of the operands, this would be "(256 * ulNum) / ulDenom", though
//! with the fixed precision of the processor the multiply would overflow if
//! performed as is.
//!
//! \return The result of the division.
//
//*****************************************************************************
static unsigned long
LongDiv256(unsigned long ulNum, unsigned long ulDenom)
{
    //
    // Perform the division in pieces to maintain the full accuracy and return
    // the 24.8 fixed-point result.
    //
    return(((ulNum / ulDenom) * 256) + (((ulNum % ulDenom) * 256) / ulDenom));
}

//*****************************************************************************
//
//! Sets the current decay mode to fast or slow.
//!
//! \param ucMode is the current decay mode, DECAY_MODE_SLOW or
//! DECAY_MODE_FAST.
//!
//! This function sets the decay mode. It takes effect immediately, which
//! means the the value can be changed while the motor is running.
//!
//! \return None.
//
//*****************************************************************************
void
StepSeqDecayMode(unsigned char ucMode)
{
    //
    // Update the current decay mode.
    //
    ucDecayMode = ucMode;
}

//*****************************************************************************
//
//! Sets the current control method to chopper, open-loop PWM or
//! closed-loop PWM.
//!
//! \param ucMode is the current control mode, CONTROL_MODE_OPENPWM,
//!              CONTROL_MODE_CLOSEDPWM or CONTROL_MODE_CHOPPER.
//!
//! This function sets the method used for current control. The motor must
//! be stopped (MOTOR_STATUS_STOP), or else this function does nothing.
//! If the mode is PWM, then the PWM period is computed from g_ulPwmFreq,
//! which should have been set to the desired PWM frequency prior to calling
//! this function.
//!
//! Also, StepSeqCurrent() should have been called prior to calling this
//! function in order to set the drive and holding current.
//!
//! \return None.
//
//*****************************************************************************
void
StepSeqControlMode(unsigned char ucMode)
{
    //
    // If the control mode is not actually being changed, or the motor
    // is not stopped, then just return.
    //
    if((ucMode == ucControlMode) || (g_ucMotorStatus != MOTOR_STATUS_STOP))
    {
        return;
    }

    //
    // Set the new control mode.
    //
    ucControlMode = ucMode;

    //
    // If the new mode is open-loop PWM, then configure the step controller for
    // PWM mode, and recompute the PWM duty cycle for the drive and
    // holding current.
    //
    if(ucMode == CONTROL_MODE_OPENPWM)
    {
        StepCtrlOpenPWMMode(SYSTEM_CLOCK / g_usPwmFreq);
        lPwmSetting[DRIVE_CURRENT] = (g_ulPwmPeriod * usDriveI) / ulMaxI;
        lPwmSetting[HOLD_CURRENT] = (g_ulPwmPeriod * usHoldI) / ulMaxI;
    }

    //
    // Else, if the mode is closed-loop PWM, then configure the step
    // controller for closed-loop PWM.  The current setting that will be
    // used is set from the StepSeqCurrent() function.
    //
    else if(ucMode == CONTROL_MODE_CLOSEDPWM)
    {
        StepCtrlClosedPWMMode(SYSTEM_CLOCK / g_usPwmFreq);
    }

    //
    // If the mode is chopper, then just configure the step controller
    // for chopper mode.
    //
    else
    {
        StepCtrlChopMode();
    }
}

//*****************************************************************************
//
//! Sets the step size to whole-steps, half-steps, or micro-steps.
//!
//! \param ucMode is the step size, STEP_MODE_FULL, STEP_MODE_WAVE,
//!               STEP_MODE_HALF, or STEP_MODE_MICRO.
//!
//! This function sets the step size. The size can be either full (normal
//! or wave), half- or micro-steps.
//!
//! The motor must be stopped (MOTOR_STATUS_STOP) or else this function
//! does nothing.
//!
//! \return None.
//
//*****************************************************************************
void
StepSeqStepMode(unsigned char ucMode)
{
    //
    // If the motor is stopped, update the step size mode.
    //
    if(g_ucMotorStatus == MOTOR_STATUS_STOP)
    {
        ucStepMode = ucMode;
    }
}

//*****************************************************************************
//
//! Sets the drive and holding current to be used by the motor.
//!
//! \param usDrive is the drive current in milliamps.
//! \param usHold is the holding current in milliamps.
//! \param ulMax is the maximum current that would flow in the winding
//!              given the winding resistance and bus voltage.
//!
//! This function takes the specified drive and hold currents in
//! milliamps and converts it and stores it in units that are used by
//! the chopping and PWM control methods.
//!
//! For the chopper and closed-loop PWM methods, the specified current is
//! converted into the equivalent value in ADC counts, which is what is
//! measured by ADC when making current measurements.
//!
//! For open-loop PWM, the duty cycle is calculated for the drive and hold
//! currents by using the ratio to the ulMax, maximum current specified.
//!
//! Note that the ulMax current specified is not the maximum safe current
//! that should flow in the winding. It is the maximum current that would
//! flow if the full bus voltage were applied to the winding with no
//! modulation. This is usually a value that is much larger than the
//! maximum safe winding current.
//!
//! This function takes effect immediately, which means the current
//! values can be changed while the motor is running.
//!
//! \return None.
//
//*****************************************************************************
void
StepSeqCurrent(unsigned short usDrive, unsigned short usHold,
               unsigned long ulMax)
{
    //
    // Based on the provided currents passed in to this function,
    // compute the chopper setting is raw ADC counts, and store the
    // values for drive and hold current.
    //
    lChopSetting[DRIVE_CURRENT] = MILLIAMPS2COUNTS(usDrive);
    lChopSetting[HOLD_CURRENT] = MILLIAMPS2COUNTS(usHold);

    //
    // Compute the PWM pulse widths, in PWM counts, for the
    // drive and hold currents.
    //
    lPwmSetting[DRIVE_CURRENT] = (g_ulPwmPeriod * usDrive) / ulMax;
    lPwmSetting[HOLD_CURRENT] = (g_ulPwmPeriod * usHold) / ulMax;

    //
    // The current settings are saved because they will be needed for
    // recomputing the PWM pulse widths if the PWM frequency is changed.
    //
    usDriveI = usDrive;
    usHoldI = usHold;
    ulMaxI = ulMax;
}

//*****************************************************************************
//
//! Performs a single step of a sequence for a single winding using
//! open-loop PWM.
//!
//! \param ulWinding is the winding ID, WINDING_ID_A or WINDING_ID_B.
//! \param uTableIdx is the index into the step sequence table.
//!
//! This function determines the setting for a single winding based on
//! the position in the step sequence table (uTableIdx), and calls the
//! appropriate function to control that winding using PWM.
//! It handles both slow and fast current decay modes.
//!
//! This function should be called once for each of the two windings at
//! each step.
//!
//! \return None.
//
//*****************************************************************************
static void
StepSeqUsingOpenPwm(unsigned long ulWinding, unsigned int uTableIdx)
{
    int nStepLevel;
    long lSetting;

    //
    // Determine the level for this winding, at this point in the
    // step sequence. This will be a signed value from 0-65536, where
    // +/-65536 represents full positive or negative value of the
    // current setting.
    //
    nStepLevel = nMicroStepTable[uTableIdx][ulWinding];

    //
    // If full or half stepping is used, then the values taken from the
    // table may be less than full magnitude. This is particularly
    // noticeable in half stepping where the magnitude will change while
    // a winding is on. The motor actually works better if the winding
    // is set to the fixed current, and not varied at each half step
    // point. Therefore, if micro stepping is not being used, then
    // normalize the value from the table to just be full positive,
    // full negative, or 0.
    //
    if(ucStepMode != STEP_MODE_MICRO)
    {
        if(nStepLevel > 0)
        {
            nStepLevel = 65536;
        }
        else if(nStepLevel < 0)
        {
            nStepLevel = -65536;
        }
    }

    //
    // If the setting has not changed, then no need to do it again ...
    // skip the rest.
    //
    if(nStepLevel != nPrevStepLevel[ulWinding])
    {
        //
        // Determine the setting for this winding by multiplying
        // the PWM setting by the level. This will result in
        // a negative, positive, or 0 PWM setting representing
        // the target current.
        //
        lSetting = (lPwmSetting[uSettingIdx] * nStepLevel) / 65536;

        //
        // Call the appropriate winding handler according to the
        // decay mode.
        // If this was the last step (indicated by lStepDelta == 0),
        // and there is no holding current (lSetting == 0)),
        // then the gate driver needs to be left in the slow decay
        // configuration.
        //
        if((ucDecayMode == DECAY_MODE_SLOW) ||
           ((lStepDelta == 0) && (lSetting == 0)))
        {
            StepCtrlOpenPwmSlow(ulWinding, lSetting);
        }
        else
        {
            StepCtrlOpenPwmFast(ulWinding, lSetting);
        }
    }

    //
    // Remember the setting for this winding, for comparison next time.
    //
    nPrevStepLevel[ulWinding] = nStepLevel;
}

//*****************************************************************************
//
//! Performs a single step of a sequence for a single winding using
//! closed-loop PWM.
//!
//! \param ulWinding is the winding ID, WINDING_ID_A or WINDING_ID_B.
//! \param uTableIdx is the index into the step sequence table.
//!
//! This function determines the setting for a single winding based on
//! the position in the step sequence table (uTableIdx), and calls the
//! appropriate function to control that winding using PWM.
//! It handles both slow and fast current decay modes.
//!
//! This function should be called once for each of the two windings at
//! each step.
//!
//! \return None.
//
//*****************************************************************************
static void
StepSeqUsingClosedPwm(unsigned long ulWinding, unsigned int uTableIdx)
{
    int nStepLevel;
    long lSetting;

    //
    // Determine the level for this winding, at this point in the
    // step sequence. This will be a signed value from 0-65536, where
    // +/-65536 represents full positive or negative value of the
    // current setting.
    //
    nStepLevel = nMicroStepTable[uTableIdx][ulWinding];

    //
    // If full or half stepping is used, then the values taken from the
    // table may be less than full magnitude. This is particularly
    // noticeable in half stepping where the magnitude will change while
    // a winding is on. The motor actually works better if the winding
    // is set to the fixed current, and not varied at each half step
    // point. Therefore, if micro stepping is not being used, then
    // normalize the value from the table to just be full positive,
    // full negative, or 0.
    //
    if(ucStepMode != STEP_MODE_MICRO)
    {
        if(nStepLevel > 0)
        {
            nStepLevel = 65536;
        }
        else if(nStepLevel < 0)
        {
            nStepLevel = -65536;
        }
    }

    //
    // If the setting has not changed, then no need to do it again ...
    // skip the rest.
    //
    if(nStepLevel != nPrevStepLevel[ulWinding])
    {
        //
        // Determine the setting for this winding by multiplying
        // the chop current setting by the level. This will result in
        // a negative, positive, or 0 current setting representing
        // the target current.
        // Chopping current is used as the control variable for
        // closed-loop PWM because the current is being measured in
        // this mode, and used as feedback to the control signal.
        //
        lSetting = (lChopSetting[uSettingIdx] * nStepLevel) / 65536;

        //
        // Call the appropriate winding handler according to the
        // decay mode.
        // If this was the last step (indicated by lStepDelta == 0),
        // and there is no holding current (lSetting == 0)),
        // then the gate driver needs to be left in the slow decay
        // configuration.
        //
        if((ucDecayMode == DECAY_MODE_SLOW) ||
           ((lStepDelta == 0) && (lSetting == 0)))
        {
            StepCtrlClosedPwmSlow(ulWinding, lSetting);
        }
        else
        {
            StepCtrlClosedPwmFast(ulWinding, lSetting);
        }
    }

    //
    // Remember the setting for this winding, for comparison next time.
    //
    nPrevStepLevel[ulWinding] = nStepLevel;
}

//*****************************************************************************
//
//! Performs a single step of a sequence for a single winding using
//! chopper mode.
//!
//! \param ulWinding is the winding ID, WINDING_ID_A or WINDING_ID_B.
//! \param uTableIdx is the index into the step sequence table.
//!
//! This function determines the setting for a single winding based on
//! the position in the step sequence table (uTableIdx), and calls the
//! appropriate function to control that winding using the chopper method.
//! It handles both slow and fast current decay modes.
//!
//! This function should be called once for each of the two windings, at
//! each step.
//!
//! \return None.
//
//*****************************************************************************
static void
StepSeqUsingChop(unsigned long ulWinding, unsigned int uTableIdx)
{
    int nStepLevel;
    long lSetting;

    //
    // Determine the level for this winding, at this point in the
    // step sequence. This will be a signed value from 0-65536, where
    // +/-65536 represents full positive or negative value of the
    // current setting.
    //
    nStepLevel = nMicroStepTable[uTableIdx][ulWinding];

    //
    // If full or half stepping is used, then the values taken from the
    // table may be less than full magnitude. This is particularly
    // noticeable in half stepping where the magnitude will change while
    // a winding is on. The motor actually works better if the winding
    // is set to the fixed current, and not varied at each half step
    // point. Therefore, if micro stepping is not being used, then
    // normalize the value from the table to just be full positive,
    // full negative, or 0.
    //
    if(ucStepMode != STEP_MODE_MICRO)
    {
        if(nStepLevel > 0)
        {
            nStepLevel = 65536;
        }
        else if(nStepLevel < 0)
        {
            nStepLevel = -65536;
        }
    }

    //
    // If the setting has not changed, then no need to do it again ...
    // skip the rest.
    //
    if(nStepLevel != nPrevStepLevel[ulWinding])
    {
        //
        // Determine the setting for this winding by multiplying
        // the chopping current setting by the level. This will result in
        // a negative, positive, or 0 chopper current setting representing
        // the target current.
        //
        lSetting = (lChopSetting[uSettingIdx] * nStepLevel) / 65536;

        //
        // Call the appropriate winding handler according to the
        // decay mode.
        // If this was the last step (indicated by lStepDelta == 0),
        // and there is no holding current (lSetting == 0)),
        // then the gate driver needs to be left in the slow decay
        // configuration.
        //
        if((ucDecayMode == DECAY_MODE_SLOW) ||
           ((lStepDelta == 0) && (lSetting == 0)))
        {
            StepCtrlChopSlow(ulWinding, lSetting);
        }
        else
        {
            StepCtrlChopFast(ulWinding, lSetting);
        }
    }

    //
    // Remember the setting for this winding, for comparison next time.
    //
    nPrevStepLevel[ulWinding] = nStepLevel;
}

//*****************************************************************************
//
//! Processes a single step.
//!
//! This function is called when the step timer times out. It is called
//! once for each step (or half/micro step). The motor position is advanced,
//! along with an index that points into the step sequence table.
//! Then functions are called to cause the actual step to take place according
//! to the position in the step sequence table.
//!
//! Once the step has taken place, then the current position is compared with
//! the different points in the speed profile (accel, run, decel, stop), and
//! actions taken to implement that part of the speed profile. These are
//! summarized as follows:
//! - ACCEL - the step time for the first step is used. At each step, the
//! new step time is calculated in order to decrease the time between steps
//! and thus causes the motor to accelerate.
//! - RUN - at the run point, the step timer is set to use the duration that
//! is equivalent to the running speed.
//! - DECEL - the step time for the first decelerating step is used. At each
//! following step, the new step time is calculated in order to increase
//! the time between steps, and thus causes the motor to decelerate.
//! - STOP - the motor is set to use the holding current, and the step timer
//! is stopped so no more steps will occur.
//!
//! \return None.
//
//*****************************************************************************
void
StepSeqHandler(void)
{
    unsigned uTableIdx;

    //
    // Clear the step timer interrupt.
    // Equivalent DriverLib call:
    //  TimerIntClear(STEP_TMR_BASE, TIMER_TIMA_TIMEOUT);
    //
    HWREG(STEP_TMR_BASE + TIMER_O_ICR) = TIMER_TIMA_TIMEOUT;

    //
    // Advance the current position by one step size. lStepDelta is
    // signed, so this works for either forward or reverse direction.
    //
    g_lCurrentPos += lStepDelta;

    //
    // Look up the position in the step sequence table. There are 32
    // entries in the table, so 5 bits of the 8 fractional bits are used.
    // Shift the fractional part of the position down to get the 5
    // most significant fractional bits. This forms the index into
    // the 32 position micro step sequence table.
    //
    // If wave mode is used, then offset the entry in the step sequence
    // table by half a step (4 micro-steps).  This will use the half step
    // points for full stepping resulting in wave drive.
    //
    if(ucStepMode == STEP_MODE_WAVE)
    {
        uTableIdx = ((g_lCurrentPos + 0x80) >> 5) & 0x1f;
    }
    else
    {
        uTableIdx = (g_lCurrentPos >> 5) & 0x1f;
    }

    //
    // Call the appropriate step sequence handler for either chopper or
    // PWM control methods. Call once for each winding.
    //
    if(ucControlMode == CONTROL_MODE_OPENPWM)
    {
        StepSeqUsingOpenPwm(WINDING_ID_A, uTableIdx);
        StepSeqUsingOpenPwm(WINDING_ID_B, uTableIdx);
    }
    else if(ucControlMode == CONTROL_MODE_CLOSEDPWM)
    {
        StepSeqUsingClosedPwm(WINDING_ID_A, uTableIdx);
        StepSeqUsingClosedPwm(WINDING_ID_B, uTableIdx);
    }
    else
    {
        StepSeqUsingChop(WINDING_ID_A, uTableIdx);
        StepSeqUsingChop(WINDING_ID_B, uTableIdx);
    }

    //
    // If the step size has been set to 0, that means we are at the
    // stopping position, and no more stepping needs to be done.
    //
    if(lStepDelta == 0)
    {
        //
        // Disable the step timer.
        // Equivalent DriverLib call:
        //  TimerDisable(STEP_TMR_BASE, TIMER_A);
        //
        HWREG(STEP_TMR_BASE + TIMER_O_CTL) = 0;

        //
        // Set the motor status to indicate stopped, and clear flag
        // indicating stopping in progress.
        //
        g_ucMotorStatus = MOTOR_STATUS_STOP;
        bStopping = 0;

        //
        // Step time of 0, means speed of 0.
        //
        g_ulStepTime = 0;

        //
        // Now that the motor is completely stopped, check to see if there
        // is a pending deferred move.
        //
        if(bDeferredMove)
        {
            bDeferredMove = 0;

            //
            // Call the Move function to start up the deferred move.
            //
            StepSeqMove(lDeferredPosition, usDeferredSpeed,
                        usDeferredAccel, usDeferredDecel);
        }
    }

    //
    // If the current position is the stop position, then transition to stop.
    //
    else if(g_lCurrentPos == lPosStop)
    {
        //
        // Disable the step timer.
        // Equivalent DriverLib call:
        //  TimerDisable(STEP_TMR_BASE, TIMER_A);
        //
        HWREG(STEP_TMR_BASE + TIMER_O_CTL) = 0;

        //
        // Set this index to indicate holding current. The next time a
        // a winding is set, it will use the holding current instead of the
        // driving current.
        //
        uSettingIdx = HOLD_CURRENT;

        //
        // Set the step size to 0, to indicate to the step handler that
        // the last position has been reached and the stepping can be stopped.
        //
        lStepDelta = 0;

        //
        // Reset the saved step levels to a large value, to ensure that the
        // next time a step is made, it will not be skipped over.
        //
        nPrevStepLevel[WINDING_ID_A] = INT_MAX;
        nPrevStepLevel[WINDING_ID_B] = INT_MAX;

        //
        // Set the step timer for one more step, using the last step time.
        // This will cause the step handler to be entered one more time,
        // but with a step size of 0, so the actual position will not be
        // changed. This is done to cause a switch to holding current
        // after the last step. g_ulStepTime is converted from 24.8
        // format before being written to the timer register.
        // Equivalent DriverLib call:
        //  TimerLoadSet(STEP_TMR_BASE, TIMER_A, (g_ulStepTime + 128) >> 8);
        //  TimerEnable(STEP_TMR_BASE, TIMER_A);
        //
        HWREG(STEP_TMR_BASE + TIMER_O_TAILR) = (g_ulStepTime + 128) >> 8;
        HWREG(STEP_TMR_BASE + TIMER_O_CTL) = TIMER_CTL_TAEN |
                                             TIMER_CTL_TASTALL;
    }

    //
    // If the current position is the run position, then transition to
    // running at continuous speed.
    //
    else if(g_lCurrentPos == lPosRun)
    {
        //
        // Disable the step timer.
        // Equivalent DriverLib call:
        //  TimerDisable(STEP_TMR_BASE, TIMER_A);
        //
        HWREG(STEP_TMR_BASE + TIMER_O_CTL) = 0;

        //
        // Change the timer to run continuously. Since the speed will now
        // be constant, there is no need to change the timeout between
        // every step.
        // Equivalent DriverLib call:
        //  TimerConfigure(STEP_TMR_BASE, TIMER_CFG_PERIODIC);
        //
        HWREG(STEP_TMR_BASE + TIMER_O_TAMR) = TIMER_TAMR_TAMR_PERIOD;

        //
        // Load the new step time (corresponding to the running speed)
        // into the step timer and start it running. The step time is
        // converted from 24.8 format as it is written to the timer
        // register.
        // Equivalent DriverLib call:
        //  TimerLoadSet(STEP_TMR_BASE, TIMER_A, (g_ulStepTime + 128) >> 8);
        //  TimerEnable(STEP_TMR_BASE, TIMER_A);
        //
        g_ulStepTime = ulMinStepTime;
        HWREG(STEP_TMR_BASE + TIMER_O_TAILR) = (g_ulStepTime + 128) >> 8;
        HWREG(STEP_TMR_BASE + TIMER_O_CTL) = TIMER_CTL_TAEN |
                                             TIMER_CTL_TASTALL;

        //
        // Set motor state to run
        //
        g_ucMotorStatus = MOTOR_STATUS_RUN;
    }

    //
    // If the position is at the decel point, or if the motor is still
    // in the decel portion of the speed profile, then change the motor
    // speed to cause it to (continue to) decelerate.
    //
    else if((g_lCurrentPos == lPosDecel) ||
            (g_ucMotorStatus == MOTOR_STATUS_DECEL))
    {
        //
        // Disable the step timer.
        // Equivalent DriverLib call:
        //  TimerDisable(STEP_TMR_BASE, TIMER_A);
        //
        HWREG(STEP_TMR_BASE + TIMER_O_CTL) = 0; // &= TIMER_CTL_TAEN;

        //
        // If this is the decel position, then set the beginning deceleration
        // parameters.
        //
        if(g_lCurrentPos == lPosDecel)
        {
            //
            // Set the denominator to the pre-computed value for the first
            // decel step.
            //
            ulDenom = ulDecelDenom;

            //
            // Set the motor status to indicate decel.
            //
            g_ucMotorStatus = MOTOR_STATUS_DECEL;

            //
            // Set the timer for one shot. This is because now the interval
            // will need to change for each step.
            // Equivalent DriverLib call:
            //  TimerConfigure(STEP_TMR_BASE, TIMER_CFG_ONE_SHOT);
            //
            HWREG(STEP_TMR_BASE + TIMER_O_TAMR) = TIMER_TAMR_TAMR_1_SHOT;
        }

        //
        // Compute the new, decelerating, step time.
        //
        g_ulStepTime += (2 * g_ulStepTime) / ulDenom;

        //
        // Load the new step time into the step timer and start it running.
        // The step time is converted from 24.8 format as it is written to
        // the timer register.
        // Equivalent DriverLib call:
        //  TimerLoadSet(STEP_TMR_BASE, TIMER_A, (g_ulStepTime + 128) >> 8);
        //  TimerEnable(STEP_TMR_BASE, TIMER_A);
        //
        HWREG(STEP_TMR_BASE + TIMER_O_TAILR) = (g_ulStepTime + 128) >> 8;
        HWREG(STEP_TMR_BASE + TIMER_O_CTL) = TIMER_CTL_TAEN |
                                             TIMER_CTL_TASTALL;

        //
        // Adjust the denominator for the next step.
        //
        if(ulDenom > 4)
        {
            ulDenom -= 4;
        }
    }

    //
    // If the position is at the accel point, or if the motor is still
    // in the accel portion of the speed profile, then change the motor
    // speed to cause it to (continue to) accelerate.
    //
    else if((g_lCurrentPos == lPosAccel) ||
            (g_ucMotorStatus == MOTOR_STATUS_ACCEL))
    {
        //
        // Disable the step timer.
        // Equivalent DriverLib call:
        //  TimerDisable(STEP_TMR_BASE, TIMER_A);
        //
        HWREG(STEP_TMR_BASE + TIMER_O_CTL) = 0; // &= TIMER_CTL_TAEN;

        //
        // If this is the accel position, then set the beginning acceleration
        // parameters.
        //
        if(g_lCurrentPos == lPosAccel)
        {
            //
            // Set the denominator to the pre-computed value for the first
            // decel step.
            //
            ulDenom = ulAccelDenom;

            //
            // Set the motor status to indicate accel.
            //
            g_ucMotorStatus = MOTOR_STATUS_ACCEL;

            //
            // Set the step time to a pre-computed value for the first step.
            // This is needed because the first step is a special case
            // in the calculation.
            //
            g_ulStepTime = ulStep1Time;

            //
            // Set the timer for one shot. This is because now the interval
            // will need to change for each step.
            // Equivalent DriverLib call:
            //  TimerConfigure(STEP_TMR_BASE, TIMER_CFG_ONE_SHOT);
            //
            HWREG(STEP_TMR_BASE + TIMER_O_TAMR) = TIMER_TAMR_TAMR_1_SHOT;
        }
        else
        {
            //
            // If this is not the first step, then calculate the next
            // step time for acceleration.
            //
            g_ulStepTime -= (2 * g_ulStepTime) / ulDenom;
        }

        //
        // Load the new step time into the step timer and start it running.
        // The step time is converted from 24.8 format as it is written
        // to the timer register.
        // Equivalent DriverLib call:
        //  TimerLoadSet(STEP_TMR_BASE, TIMER_A, (g_ulStepTime + 128) >> 8);
        //  TimerEnable(STEP_TMR_BASE, TIMER_A);
        //
        HWREG(STEP_TMR_BASE + TIMER_O_TAILR) = (g_ulStepTime + 128) >> 8;
        HWREG(STEP_TMR_BASE + TIMER_O_CTL) = TIMER_CTL_TAEN |
                                             TIMER_CTL_TASTALL;

        //
        // Adjust the denominator for the next step.
        //
        ulDenom += 4;
    }
}

//*****************************************************************************
//
//! Initiates a move of the motor by calculating a speed profile and starting
//! up the step sequencing.
//!
//! \param lNewPosition is the new target motor position in steps
//!                     in fixed-point 24.8 format.
//! \param usSpeed is the target running speed in steps/sec.
//! \param usAccel is the acceleration in steps/sec^2.
//! \param usDecel is the deceleration in steps/sec^2.
//!
//! This function calculates a speed profile for the specified motion.
//! It looks at the current position and the number of steps needed to
//! accelerate to, and decelerate from, in order to end up at the specified
//! target position. The acceleration, running, deceleration, and stopping
//! points are calculated, along with parameters needed for calculating the
//! acceleration and deceleration step times, done by the step handler
//! StepSeqHandler().
//!
//! The current motion is also considered. If the motor is already moving,
//! then the current speed and direction are taken into consideration, and
//! a new speed profile is computed. If the new speed is different from the
//! current speed, then this may involve accelerating or decelerating from the
//! current speed to get to the new speed. It may also be the case that
//! given the current speed and direction that it is not possible to reach the
//! new target position without first stopping the motor and then moving it
//! again. In this case, a deferred move is set up, and the motor is
//! commanded to decelerate to a stop. Once it stops, then the deferred
//! move is made to ensure the motor will end up at the correct position.
//!
//! The parameter \e lNewPosition is a signed number representing the
//! motor position in fixed-point 24.8 format.  The upper 24 bits are the
//! (signed) whole step position, while the lower 8 bits are the fractional
//! step position.  While this allows for a theoretical resolution of 1/256
//! step size, the motor does not actually support micro-steps that small.
//! The value of the lower 8 bits (the fractional step) is the fractional
//! value multiplied by 256.  For example, the lower 8 bits of a half-step
//! is 0x80 (0.5 * 256).  Likewise, a quarter step is 0x40 (0.25 * 256).  In
//! order to use half-steps you must be using half- or micro-stepping mode.
//! In order to use fractional steps smaller than half, you must be using
//! micro-stepping mode.
//!
//! \return None.
//
//*****************************************************************************
void
StepSeqMove(long lNewPosition, unsigned short usSpeed,
            unsigned short usAccel, unsigned short usDecel)
{
    long lMoveSteps;
    long lAccelSteps;
    long lDecelSteps;
    long lAccelDecel;
    long lOldAccelSteps;
    long lOldDecelSteps;
    unsigned short usOldSpeed;
    static long lPrevStepDelta = 0x100;

    //
    // Disable the step timer interrupt to avoid having a step occur while
    // we are making changes to the speed profile. The timer keeps running
    // if it was already running.
    // Equivalent DriverLib call:
    //  TimerIntDisable(STEP_TMR_BASE, TIMER_TIMA_TIMEOUT);
    //
    HWREG(STEP_TMR_BASE + TIMER_O_IMR) &= ~TIMER_TIMA_TIMEOUT;

    //
    // Remember what the step size was before any changes are made.
    //
    lPrevStepDelta = lStepDelta;

    //
    // Compute the number of steps in the move (signed)
    //
    lMoveSteps = lNewPosition - g_lCurrentPos;

    //
    // Make adjustments based on stepping mode, full or half
    //
    if(ucStepMode == STEP_MODE_HALF)
    {
        //
        // Set the step size increment for half steps.
        //
        lStepDelta = 0x80;

        //
        // Normalize the move size to half steps (lose the fractional)
        //
        lMoveSteps >>= 7;

        //
        // If half steps, then the speed, accel and decel parameters need
        // to be adjusted to half step units.
        //
        usSpeed *= 2;
        usAccel *= 2;
        usDecel *= 2;
    }

    else if(ucStepMode == STEP_MODE_MICRO)
    {
        //
        // Set the step size increment for micro steps.
        //
        lStepDelta = 0x20;

        //
        // Normalize the move size to micro steps (lose the unused lower bits).
        //
        lMoveSteps >>= 5;

        //
        // If micro steps, then the speed, accel and decel parameters need
        // to be adjusted to micro step units.
        //
        usSpeed *= 8;
        usAccel *= 8;
        usDecel *= 8;
    }
    else
    {
        //
        // Set the step size increment for whole steps.
        //
        lStepDelta = 0x100;

        //
        // Normalize the move size to whole steps (lose the fractional)
        //
        lMoveSteps >>= 8;

        //
        // For whole steps, no adjustment is needed for speed, accel,
        // and decel parameters.
        //
    }

    //
    // If the move is in the negative direction, then make the
    // step size increment signed negative, and get the absolute value of
    // the number of move steps, which will be needed as abs value
    // in later calculation.
    //
    if(lMoveSteps < 0)
    {
        lStepDelta = -lStepDelta;
        lMoveSteps = -lMoveSteps;
    }

    //
    // Compute the number of steps for the acceleration and
    // deceleration ramps.
    //
    lAccelSteps = GET_NUM_STEPS(usSpeed, usAccel);
    lDecelSteps = GET_NUM_STEPS(usSpeed, usDecel);

    //
    // Make sure there is at least one accel step. This is needed
    // by the step handler.
    //
    if(lAccelSteps == 0)
    {
        lAccelSteps = 1;
    }

    //
    // In this case, the motor is not already moving, so the computation
    // of the speed profile does not need to take into account an
    // existing motion.
    //
    if(g_ucMotorStatus == MOTOR_STATUS_STOP)
    {
        //
        // If the number of steps is 0, then that means the motor is
        // already at the target position, and nothing else needs to
        // be done.
        //
        if(lMoveSteps == 0)
        {
            return;
        }

        //
        // If the total move is not large enough for the number of steps
        // in the accel and decel ramps, then adjustments need to be made.
        // The profile will be to accel to a point then decel, there
        // will be no run phase since there is not time to reach full
        // speed before needing to decel.
        //
        if((lAccelSteps + lDecelSteps) >= lMoveSteps)
        {
            //
            // Save the total number of steps needed for accel and decel.
            //
            lAccelDecel = lAccelSteps + lDecelSteps;

            //
            // Compute the portion of the total move that should be used
            // for acceleration ramp, based on the ratio of the accel
            // to decel rates.
            //
            lAccelSteps = (lMoveSteps * lAccelSteps);
            lAccelSteps += lAccelDecel / 2;
            lAccelSteps /= lAccelDecel;

            //
            // The remainder of the steps in the move are for decel.
            //
            lDecelSteps = lMoveSteps - lAccelSteps;

            //
            // Since the motor will never reach running speed, there
            // is no run transition point. Set it to one position
            // behind the current position so that the step handler
            // will never see a run point.
            //
            lPosRun = g_lCurrentPos - lStepDelta;   // no run point
        }

        //
        // The total move is big enough to have a run phase.
        //
        else
        {
            //
            // Set the run point to the end of the acceleration phase.
            //
            lPosRun = g_lCurrentPos + (lAccelSteps * lStepDelta);
        }

        //
        // Set the accel position to be the next step.
        //
        lPosAccel = g_lCurrentPos + lStepDelta;

        //
        // Compute the decel and stop positions.
        //
        lPosStop = g_lCurrentPos + (lMoveSteps * lStepDelta);
        lPosDecel = lPosStop - (lDecelSteps * lStepDelta);

        //
        // If the accel and decel points are the same, then that means the
        // motion is only one or two steps. In this case, toss the decel
        // point so that there will be one step of accel and then stop.
        //
        if(lPosDecel == lPosAccel)
        {
            lPosDecel = g_lCurrentPos - lStepDelta;
        }

        //
        // Set the denominator value that will be used in the step time
        // calculation formula at the start of the accel and decel ramps.
        //
        ulAccelDenom = 5;
        ulDecelDenom = (lDecelSteps * 4) - 1;

        //
        // Compute the step time at the running speed.
        //
        ulMinStepTime = GET_CMIN(usSpeed);

        //
        // Calculate the step times for the zeroth and first steps.
        // The first step has a special correction to adjust for inaccuracy
        // in the step time calulation (for the first step only).
        // These values are kept in 24.8 format to maintain precision.
        //
        ulStep0Time = GET_C0(usAccel);
        ulStep1Time = MulDiv(ulStep0Time, 4056, 10000);

        //
        // Update the step time variable to match the first step, in case
        // another profile change is made before the first step can occur.
        //
        g_ulStepTime = ulStep1Time;

        //
        // Set the step time and start the step timer running.
        //
        TimerLoadSet(STEP_TMR_BASE, TIMER_A, ulStep0Time >> 8);
        TimerEnable(STEP_TMR_BASE, TIMER_A);

        //
        // Change the motor status to indicate accel, so that future
        // profile changes will see that the motor is already
        // doing something.
        //
        g_ucMotorStatus = MOTOR_STATUS_ACCEL;
    }

    //
    // In this case, the motor is already moving, so the current
    // direction and speed need to be taken into account when computing
    // a new speed profile.
    //
    else
    {
        //
        // The new position may not be reachable for two reasons:
        // the distance to the new position is less than the number
        // of steps needed to decelerate to it, OR, the new position
        // is behind the current position, in the other direction.
        // In either case, the motor needs to be stopped and use a new
        // move to get to the new position.
        //
        if((lDecelSteps > lMoveSteps) ||
           ((lPrevStepDelta * lStepDelta) < 0) ||
           (lMoveSteps == 0))
        {
            //
            // Set the deferred move flag, and store all the deferred
            // move parameters.
            //
            bDeferredMove = 1;

            //
            // If half steps is used, then the requested speed, accel and
            // decel were already adjusted by a factor of 2. So they need
            // to be "unadjusted" back to their original values for the
            // deferred move values.
            //
            if(ucStepMode == STEP_MODE_HALF)
            {
                usSpeed /= 2;
                usAccel /= 2;
                usDecel /= 2;
            }

            //
            // If micro steps are used, then the requested speed, accel and
            // decel were already adjusted by a factor of 8. So they need
            // to be "unadjusted" back to their original values for the
            // deferred move values.
            //
            else if(ucStepMode == STEP_MODE_MICRO)
            {
                usSpeed /= 8;
                usAccel /= 8;
                usDecel /= 8;
            }

            lDeferredPosition = lNewPosition;
            usDeferredSpeed = usSpeed;
            usDeferredAccel = usAccel;
            usDeferredDecel = usDecel;

            //
            // Restore the step size to the value and sign it had before
            // it was possibly changed above. This is so that the motor
            // will continue in the correct direction.
            //
            lStepDelta = lPrevStepDelta;

            //
            // Command the motor to stop. This will cause an immediate
            // deceleration profile to be computed, causing the motor
            // to stop as soon as possible. There is nothing else to do
            // until the motor stops. When it stops, the step handler
            // will see that there is a deferred move and start up a new
            // move.
            //
            StepSeqStop();
            return;
        }

        //
        // Getting to here means that the new position is reachable,
        // given the current speed and position.
        //

        //
        // Find the current speed that the motor is running. This could
        // even be during and accel or decel ramp. The value will be the
        // instantaneous speed. The step time must be converted from 24.8
        // format.
        //
        if(g_ulStepTime)
        {
            usOldSpeed = SYSTEM_CLOCK / ((g_ulStepTime + 128) >> 8);
        }
        else
        {
            usOldSpeed = 0;
        }

        //
        // Compute the accel and decel steps needed to reach the current
        // speed.
        //
        lOldAccelSteps = GET_NUM_STEPS(usOldSpeed, usAccel);
        lOldDecelSteps = GET_NUM_STEPS(usOldSpeed, usDecel);

        //
        // In this case, the new speed is faster than the current speed,
        // so the motor needs to be sped up.
        //
        if(usSpeed > usOldSpeed)
        {
            //
            // Find the number of acceleration steps needed to go
            // from the current speed to the new, faster speed.
            // lAccelSteps was already computed as the number of accel
            // steps from stopped to the new speed. So adjust since
            // the motor is not stopped.
            //
            lAccelSteps -= lOldAccelSteps;

            //
            // Make sure there is at least one accel step.
            //
            if(lAccelSteps == 0)
            {
                lAccelSteps = 1;
            }

            //
            // Compute the denominator for the first acceleration
            // step, which is based on how far along the motor is
            // toward the target speed.
            //
            ulAccelDenom = (lOldAccelSteps * 4) + 1;

            //
            // Set the initial step time to the current step time.
            //
            ulStep1Time = g_ulStepTime;

            //
            // Set the accel point to the next step.
            //
            lPosAccel = g_lCurrentPos + lStepDelta;

            //
            // Compute the remaining speed profile points.
            //
            lPosStop = g_lCurrentPos + (lMoveSteps * lStepDelta);
            lPosDecel = lPosStop - (lDecelSteps * lStepDelta);

            //
            // If the combination of accel and decel steps is bigger than
            // the total move, then it is not possible to achieve the new
            // higher speed. So just accel until the decel point is
            // reached.
            //
            if((lAccelSteps + lDecelSteps) > lMoveSteps)
            {
                //
                // Set the run point behind the current position so
                // it is not used.
                //
                lPosRun = g_lCurrentPos - lStepDelta;
            }
            else
            {
                //
                // Compute a normal run point at the end of the
                // accel phase.
                //
                lPosRun = g_lCurrentPos + (lAccelSteps * lStepDelta);
            }

            //
            // Compute the denominator used for the first decel step.
            //
            ulDecelDenom = (lDecelSteps * 4) - 1;

            //
            // Compute the step time at running speed. Step time
            // is kept in 24.8 format.
            //
            ulMinStepTime = GET_CMIN(usSpeed);

            //
            // Set the status to accel.
            //
            g_ucMotorStatus = MOTOR_STATUS_ACCEL;
        }

        //
        // Else, in this case, the motor needs to slow down. There
        // will be 2 decel points. The first is immediate in order
        // to slow the motor from the current speed, to the new slower
        // speed. The second "normal" decel point is at a future
        // position and is where the motor slows from the new running
        // speed to a stop at the new target position.
        //
        else if(usSpeed < usOldSpeed)
        {
            //
            // Compute the denominator for the next step, which will
            // be the first decel step from the current speed. Also
            // compute the number of steps to go from the current speed,
            // to the new, slower speed.
            //
            ulDenom = (lOldDecelSteps * 4) - 1;
            lOldDecelSteps -= lDecelSteps;

            //
            // Make sure there is at least 1 decel step to keep step
            // handler happy.
            //
            if(lOldDecelSteps == 0)
            {
                lOldDecelSteps = 1;
            }

            //
            // No acceleration is needed, so set the accel point just
            // behind the current position, so that the step handler
            // never sees it.
            //
            lPosAccel = g_lCurrentPos - lStepDelta;

            //
            // Compute the run, decel, and stop points. Note
            // that this decel point is to go from the new speed
            // to a stop (after the decel from the current speed
            // to the new speed).
            //
            lPosRun = g_lCurrentPos + (lOldDecelSteps * lStepDelta);
            lPosStop = g_lCurrentPos + (lMoveSteps * lStepDelta);
            lPosDecel = lPosStop - (lDecelSteps * lStepDelta);

            //
            // Compute the denominator for deceleration from the new
            // target speed to a stop.
            //
            ulDecelDenom = (lDecelSteps * 4) - 1;

            //
            // Compute the step time when running at the new
            // target speed. Step time is kept in 24.8 format.
            //
            ulMinStepTime = GET_CMIN(usSpeed);

            //
            // Set the motor status to decel.
            //
            g_ucMotorStatus = MOTOR_STATUS_DECEL;
        }

        //
        // In this case, the new speed is the same as the current speed
        // (though position, accel and decel may have changed). It is not
        // necessary to change the speed, so just set the motor to running
        // at the current speed, and compute the new decel profile.
        //
        else
        {
            //
            // No acceleration is needed so set the accel point behind
            // the current position.
            //
            lPosAccel = g_lCurrentPos - lStepDelta;

            //
            // Set the run point to be at the next step. If the motor
            // was already in the run phase, then this will not change
            // anything. If the motor was in accel or decel, this will
            // cause it to immediately switch to run at the current
            // speed.
            //
            lPosRun = g_lCurrentPos + lStepDelta;

            //
            // Set the stop, decel points, and the decel denominator
            // as normal.
            //
            lPosStop = g_lCurrentPos + (lMoveSteps * lStepDelta);
            lPosDecel = lPosStop - (lDecelSteps * lStepDelta);
            ulDecelDenom = (lDecelSteps * 4) - 1;

            //
            // Compute the step time for the running speed.
            //
            ulMinStepTime = GET_CMIN(usSpeed);
        }

        //
        // We might be in the process of stopping as the result of a stop
        // command (StepSeqStop), so clear the stopping flag since we will
        // now have a new motion.
        //
        bStopping = 0;
    }

    //
    // Set this index to indicate drive current. This means that
    // driving current will be applied to the windings when a step
    // is made.
    //
    uSettingIdx = DRIVE_CURRENT;

    //
    // Remember the last deceleration value. It may be needed in
    // by StepSeqStop() in case the motor needs to be suddenly
    // decelerated to a stop.
    //
    usLastDecel = usDecel;

    //
    // Re-enable the step timer interrupt.
    // Equivalent DriverLib call:
    //  TimerIntEnable(STEP_TMR_BASE, TIMER_TIMA_TIMEOUT);
    //
    HWREG(STEP_TMR_BASE + TIMER_O_IMR) |= TIMER_TIMA_TIMEOUT;
}

//*****************************************************************************
//
//! Initiates a stop of the motor as quickly as possible, without
//! loss of control.
//!
//! This function will immediately decelerate the motor to a stop using
//! the last specified deceleration rate. This is a graceful way to stop
//! the motor quickly, and does not cause loss of position information.
//!
//! If the holding current has a non-zero value, then when the motor is
//! stopped, the holding current will be applied.
//!
//! \return None.
//
//*****************************************************************************
void
StepSeqStop(void)
{
    unsigned short usSpeed;
    long lDecelSteps;

    //
    // Disable the step timer interrupt to avoid having a step occur
    // while we are making changes to the speed profile. The timer
    // continues to run.
    // Equivalent DriverLib call:
    //  TimerIntDisable(STEP_TMR_BASE, TIMER_TIMA_TIMEOUT);
    //
    HWREG(STEP_TMR_BASE + TIMER_O_IMR) &= ~TIMER_TIMA_TIMEOUT;

    //
    // Only make changes if the motor is not stopped, or in the
    // process of stopping.
    //
    if((g_ucMotorStatus != MOTOR_STATUS_STOP) && !bStopping)
    {
        //
        // Set flag to indicate the motor is in the process of stopping.
        //
        bStopping = 1;

        //
        // Compute the current speed. The step time must be converted
        // from 24.8 format.
        //
        if(g_ulStepTime)
        {
            usSpeed = SYSTEM_CLOCK / ((g_ulStepTime + 128) >> 8);
        }
        else
        {
            usSpeed = 0;
        }

        //
        // Given the current speed, and the last used decel rate,
        // compute how many steps to stop.
        //
        lDecelSteps = GET_NUM_STEPS(usSpeed, usLastDecel);
        if(lDecelSteps == 0)
        {
            lDecelSteps = 1;
        }

        //
        // Compute the denominator for deceleration from the current
        // speed to a stop.
        //
        ulDecelDenom = (lDecelSteps * 4) - 1;

        //
        // Compute the new stop position.
        // Add one extra to allow for one more step taken by the sequencer
        // before it starts to decelerate.
        //
        lPosStop = g_lCurrentPos + ((lDecelSteps + 1) * lStepDelta);

        //
        // Adjust the stop position by rounding to the next whole step
        // to insure that step sequence doesn't get confused.
        //
        // If half stepping is is used, round up by a half step.
        //
        if(ucStepMode == STEP_MODE_HALF)
        {
            lPosStop += 1 * lStepDelta;
            lPosStop &= ~0xff;
        }

        //
        // If micro stepping is used, then round up by half of the
        // number of micro steps per step.
        //
        else if(ucStepMode == STEP_MODE_MICRO)
        {
            lPosStop += 8 * lStepDelta;
            lPosStop &= ~0xff;
        }

        //
        // Set the start of the decel ramp by backing up from the stop
        // position, by the number of decel steps.
        //
        lPosDecel = lPosStop - (lDecelSteps * lStepDelta);

        //
        // We dont want to hit the run point while decelerating, so
        // move the run point behind the current position.
        //
        lPosRun = g_lCurrentPos - lStepDelta;
    }

    //
    // Re-enable the step timer interrupt so it can continue to run
    // but now with a new speed profile.
    // Equivalent DriverLib call:
    //  TimerIntEnable(STEP_TMR_BASE, TIMER_TIMA_TIMEOUT);
    //
    HWREG(STEP_TMR_BASE + TIMER_O_IMR) |= TIMER_TIMA_TIMEOUT;
}

//*****************************************************************************
//
//! Stops the motor immediately with no deceleration, and turns off all
//! control signals.
//!
//! This function will disable all control methods, stop the step sequencing,
//! and set all the control signals to a safe level. The H-bridges will be
//! disabled. Position knowledge will be lost.
//!
//! \return None.
//
//*****************************************************************************
void
StepSeqShutdown(void)
{
    //
    // Disable the step timer so that the step handler will
    // no longer try to control any pins.
    //
    TimerDisable(STEP_TMR_BASE, TIMER_BOTH);

    //
    // Turn all the signals off on both windings. Using the PwmFast
    // control method with a value of 0 will ensure that both sides
    // of the H-bridge are set to low value, and the H-bridge enable
    // signal will be turned off. So all the switches will be opened.
    //
    StepCtrlOpenPwmFast(WINDING_ID_A, 0);
    StepCtrlOpenPwmFast(WINDING_ID_B, 0);

    //
    // Indicate that the motor is stopped, so that a new speed profile
    // will be computed for the next motion.
    //
    g_ucMotorStatus = MOTOR_STATUS_STOP;
    bStopping = 0;
}

//*****************************************************************************
//
//! Initializes the step sequencer module.
//!
//! This function will configure the peripherals needed to run the step
//! sequencer.
//!
//! \return None.
//
//*****************************************************************************
void
StepSeqInit(void)
{
    //
    // Initialize the step controller.
    //
    StepCtrlInit();

    //
    // Enable the timer used for a step timer, and configure it to
    // run as a 32 bit one-shot timer. Enable the timer interrupt.
    //
    SysCtlPeripheralEnable(STEP_TMR_PERIPH);
    SysCtlPeripheralSleepEnable(STEP_TMR_PERIPH);
    TimerConfigure(STEP_TMR_BASE, TIMER_CFG_ONE_SHOT);
    TimerControlStall(STEP_TMR_BASE, TIMER_A, true);
    TimerIntEnable(STEP_TMR_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(STEP_TMR_INT);
    IntPrioritySet(STEP_TMR_INT, STEP_TMR_INT_PRI);
}

//*****************************************************************************
//
// Close the Doxyen group.
//! @}
//
//*****************************************************************************
