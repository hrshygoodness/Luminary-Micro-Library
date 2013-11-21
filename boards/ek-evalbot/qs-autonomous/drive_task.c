//****************************************************************************
//
// drive_task.c - Control of EVALBOT drive.
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
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "drivers/motor.h"
#include "drivers/sensors.h"
#include "drive_task.h"
#include "pid.h"

//****************************************************************************
//
// Defines the timer peripheral to use as a free-running 32-bit timer for
// wheel speed measurement.
//
//****************************************************************************
#define WHEEL_TIMER_PERIPH SYSCTL_PERIPH_TIMER0
#define WHEEL_TIMER_BASE TIMER0_BASE

//****************************************************************************
//
// Defines the maximum and minimum reasonable RPM that is controllable with
// the EVALBOT motors.
//
//****************************************************************************
#define MAX_WHEEL_RPM   100
#define MIN_WHEEL_RPM   5

//****************************************************************************
//
// These variables hold constants that are used in speed calculations, that
// can be pre-computed when the app is started.  The reason they are not
// macro constants is because the values depend on the system clock speed.
//
//****************************************************************************
static unsigned long g_ulWheelTimerTickConstant;
static unsigned long g_ulMinRPMTicks;
static unsigned long g_ulMaxRPMTicks;

//****************************************************************************
//
// A structure to hold instance data about each motor (left and right).
//
//****************************************************************************
typedef struct
{
    //
    // A flag to indicate if the motor is in reverse.
    //
    tBoolean bReverse;

    //
    // A flag to indicate if the motor is running
    //
    volatile tBoolean bRunning;

    //
    // The target speed, in RPM, in 32.0 format
    //
    long lTargetSpeed;

    //
    // The actual speed, in RPM, in 32.0 format
    //
    volatile long lActualSpeed;

    //
    // The value of the free-running wheel speed timer the last time this
    // wheel had a "click".
    //
    volatile unsigned long ulLastEdgeTick;

    //
    // The presently commanded duty cycle of the motor, in percent, in
    // 16.16 format
    //
    long lDuty;

    //
    // The state of the PID control loop for this wheel
    //
    tPIDState sPID;

} tMotorDrive;

static tMotorDrive g_sMotorDrives[2];

//****************************************************************************
//
// The wheel/motor PID gain values.  All are in 16.16 format.
//
//****************************************************************************
#define MOTORPID_PGAIN 4096         // 4096/65536 ==> 1/16
#define MOTORPID_IGAIN 0
#define MOTORPID_DGAIN 0

//****************************************************************************
//
// The minimum and maximum values that should be allowed for the PID
// integrator, to prevent integrator windup.  These values should be set so
// the max/min value when multiplied by the IGAIN will produce the min/max
// desired output from the PID controller.  These values are in 16.16 format.
//
//****************************************************************************
#define MOTORPID_INTEGRATOR_MAX 0   //(((10 << 16) / MOTORPID_IGAIN) << 16)
#define MOTORPID_INTEGRATOR_MIN 0   //(((-10 << 16) / MOTORPID_IGAIN) << 16)

//****************************************************************************
//
// This function is called from the wheel sensor interrupt handler
// (see drivers/sensors.c).  It is called whenever one of the wheel sensors
// has "clicked" past one of the 8 positions on the wheel.  This handler is
// used to measure the time between wheel clicks, and thus derive the current
// wheel speed in RPM.
//
// This function runs in interrupt context.
//
//****************************************************************************
static void
DriveWheelSensorHandler(tWheel eWheel)
{
    unsigned long ulNowTicks;
    unsigned long ulElapsed;

    //
    // Make sure that the motor specified is valid
    //
    if((eWheel != WHEEL_LEFT) && (eWheel != WHEEL_RIGHT))
    {
        //
        // Do nothing if bad value was passed in
        //
        return;
    }

    //
    // Get the current value of the wheel tick timer
    //
    ulNowTicks = ROM_TimerValueGet(WHEEL_TIMER_BASE, TIMER_A);

    //
    // If the wheel was previously not running, then save this edge time,
    // mark this wheel as now running, and return without computing a new
    // wheel speed.  This is done so that a bogus speed is not calculated
    // on the first click when the motor starts from a stop.
    //
    if(!g_sMotorDrives[eWheel].bRunning)
    {
        g_sMotorDrives[eWheel].ulLastEdgeTick = ulNowTicks;
        g_sMotorDrives[eWheel].bRunning = true;
        return;
    }

    //
    // Compute how many ticks since the last wheel click.
    // Note: use of unsigned, full-scale math means that tick timer rollover
    // is taken care of automatically.
    //
    ulElapsed = ulNowTicks - g_sMotorDrives[eWheel].ulLastEdgeTick;

    //
    // Cap the measured elapsed time to the reasonable range.
    //
    if(ulElapsed > g_ulMinRPMTicks)
    {
        ulElapsed = g_ulMinRPMTicks;
    }
    else if(ulElapsed < g_ulMaxRPMTicks)
    {
        ulElapsed = g_ulMaxRPMTicks;
    }

    //
    // Save the timer tick value from this pass, so that next time it will
    // be used for determining the elapsed time.
    //
    g_sMotorDrives[eWheel].ulLastEdgeTick = ulNowTicks;

    //
    // Now compute the actual speed.  The speed is in RPM in 32.0 format.
    //
    g_sMotorDrives[eWheel].lActualSpeed = g_ulWheelTimerTickConstant /
                                          ulElapsed;
}

//****************************************************************************
//
// This function is used to get the computed wheel/motor speed.  It does some
// checking to make sure that the wheel has not stopped.
//
//****************************************************************************
long
DriveSpeedGet(unsigned long ulMotor)
{
    unsigned long ulElapsed;
    unsigned long ulNowTicks;
    volatile unsigned long ulLastTick;

    //
    // First get the value of the most recent edge tick.  This value is
    // copied to here because it might be changed by the wheel tick handler
    // while we are in this function.
    //
    ulLastTick = g_sMotorDrives[ulMotor].ulLastEdgeTick;

    //
    // Get the current value of the wheel tick timer
    //
    ulNowTicks = ROM_TimerValueGet(WHEEL_TIMER_BASE, TIMER_A);

    //
    // Find out how much time has elapsed since the last wheel tick.
    //
    ulElapsed = ulNowTicks - ulLastTick;

    //
    // If it is longer than the time represented by minimum RPM, then
    // consider the speed to be 0.
    //
    if(ulElapsed > g_ulMinRPMTicks)
    {
        //
        // Mark the actual speed as 0 and the motor as stopped.
        //
        g_sMotorDrives[ulMotor].lActualSpeed = 0;
        g_sMotorDrives[ulMotor].bRunning = 0;
    }

    //
    // Return the actual speed
    //
    return(g_sMotorDrives[ulMotor].lActualSpeed);
}

//*****************************************************************************
//
// Runs the wheel/motors at a specified RPM, using a PID controller to maintain
// the target speed.  The speed error is fed to a PID controller, the output
// of which is used to adjust the motor duty cycle up or down.  After this
// calculation, the motor is then commanded to the new duty cycle.
//
// This function is called periodically as a "task" using the simple scheduler
// from the application's main loop (in qs-autonomous.c).
//
//*****************************************************************************
void
DriveTask(void *pvParam)
{
    unsigned int uMotor;
    long lDiff;
    long lPIDOutput;
    unsigned short usPercent;

    //
    // Process once each for left and right motor
    //
    for(uMotor = 0; uMotor < 2; uMotor++)
    {
        tMotorDrive *pMotor = &g_sMotorDrives[uMotor];

        //
        // Compute the difference between target speed and actual speed.
        //
        lDiff = pMotor->lTargetSpeed - pMotor->lActualSpeed;

        //
        // Feed the difference to the PID loop.  The output is interpreted
        // as a duty cycle adjustment (positive or negative), in percent,
        // in 16.16 format
        //
        lPIDOutput = PIDUpdate(&pMotor->sPID, lDiff << 16);

        //
        //
        // Restrict the duty cycle adjustment to a maximum range of
        // +/- 10%.
        //
        if(lPIDOutput > (10 << 16))
        {
            lPIDOutput = 10 << 16;
        }
        else if(lPIDOutput < -(10 << 16))
        {
            lPIDOutput = -10 << 16;
        }

        //
        // Now apply the adjustment to the motor duty cycle.
        //
        pMotor->lDuty += lPIDOutput;

        //
        // Keep the resulting duty cycle between 0 and 100%
        //
        if(pMotor->lDuty > (100 << 16))
        {
            pMotor->lDuty = 100 << 16;
        }
        else if(pMotor->lDuty < 0)
        {
            pMotor->lDuty = 0;
        }

        //
        // The duty cycle is kept in units of percent, in 16.16 format.  Pass
        // the new duty cycle value to the motor duty cycle control function.
        // The duty cycle needs to be converted to 8.8 format which is
        // expected by that function.
        //
        usPercent = (unsigned short)(pMotor->lDuty >> 8);
        MotorSpeed((unsigned char)uMotor, usPercent);
    }
}

//****************************************************************************
//
// This function stops the motors from running.
//
//****************************************************************************
void
DriveStop(void)
{
    unsigned int uMotor;

    //
    // Process once each for left and right motor
    //
    for(uMotor = 0; uMotor < 2; uMotor++)
    {
        tMotorDrive *pMotor = &g_sMotorDrives[uMotor];

        //
        // Command the motor hardware to stop
        //
        MotorSpeed((unsigned char)uMotor, 0);
        MotorStop(uMotor);

        //
        // Set the actual and target speeds to 0
        //
        pMotor->lActualSpeed = 0;
        pMotor->lTargetSpeed = 0;

        //
        // Mark motor as not running
        //
        pMotor->bRunning = 0;

        //
        // Reset the PID controller
        //
        PIDReset(&pMotor->sPID);
    }
}

//****************************************************************************
//
// This function starts (or continues) the motors running, specifying
// the speed and direction.  The direction can be one of the following:
// MOTOR_DRIVE_FORWARD, MOTOR_DRIVE_REVERSE, MOTOR_DRIVE_TURN_LEFT,
// MOTOR_DRIVE_TURN_RIGHT.
//
// The speed can be an RPM in the range of 0 to MAX_WHEEL_RPM.
//
//****************************************************************************
void
DriveRun(unsigned long ulDirection, unsigned long ulSpeed)
{
    //
    // Validate the inputs
    //
    if(((ulDirection != MOTOR_DRIVE_REVERSE) &&
        (ulDirection != MOTOR_DRIVE_FORWARD) &&
        (ulDirection != MOTOR_DRIVE_TURN_LEFT) &&
        (ulDirection != MOTOR_DRIVE_TURN_RIGHT)) ||
       (ulSpeed > MAX_WHEEL_RPM))
    {
        return;
    }

    //
    // Determine which direction each motor needs to run
    //
    switch(ulDirection)
    {
        //
        // Turn left - left motor reverse, right motor forward
        //
        case MOTOR_DRIVE_TURN_LEFT:
        {
            g_sMotorDrives[MOTOR_DRIVE_LEFT].bReverse = 1;
            g_sMotorDrives[MOTOR_DRIVE_RIGHT].bReverse = 0;
            break;
        }

        //
        // Turn right - left motor forward, right motor reverse
        //
        case MOTOR_DRIVE_TURN_RIGHT:
        {
            g_sMotorDrives[MOTOR_DRIVE_LEFT].bReverse = 0;
            g_sMotorDrives[MOTOR_DRIVE_RIGHT].bReverse = 1;
            break;
        }

        //
        // Forward - both motors forward
        //
        case MOTOR_DRIVE_FORWARD:
        {
            g_sMotorDrives[MOTOR_DRIVE_LEFT].bReverse = 0;
            g_sMotorDrives[MOTOR_DRIVE_RIGHT].bReverse = 0;
            break;
        }

        //
        // Reverse - both motors reverse
        //
        case MOTOR_DRIVE_REVERSE:
        {
            g_sMotorDrives[MOTOR_DRIVE_LEFT].bReverse = 1;
            g_sMotorDrives[MOTOR_DRIVE_RIGHT].bReverse = 1;
            break;
        }

        //
        // Default - should never happen so return with all stopped
        //
        default:
        {
            DriveStop();
            return;
        }
    }

    //
    // Set the target motor speed
    //
    g_sMotorDrives[MOTOR_DRIVE_LEFT].lTargetSpeed = ulSpeed;
    g_sMotorDrives[MOTOR_DRIVE_RIGHT].lTargetSpeed = ulSpeed;

    //
    // Set each motor to forward or reverse as needed
    //
    MotorDir(LEFT_SIDE,
             g_sMotorDrives[MOTOR_DRIVE_LEFT].bReverse ? REVERSE : FORWARD);
    MotorDir(RIGHT_SIDE,
             g_sMotorDrives[MOTOR_DRIVE_RIGHT].bReverse ? REVERSE : FORWARD);

    //
    // As an initial value, set the motor speed duty in percent to the same
    // as the request RPM.  This just takes advantage of a coincidence that
    // PWM duty cycle = RPM (approx).  Enable the motor to start running.
    //
    g_sMotorDrives[MOTOR_DRIVE_LEFT].lDuty = ulSpeed << 16;
    g_sMotorDrives[MOTOR_DRIVE_RIGHT].lDuty = ulSpeed << 16;
    MotorSpeed(LEFT_SIDE, (unsigned short)(ulSpeed << 8));
    MotorSpeed(RIGHT_SIDE, (unsigned short)(ulSpeed << 8));
    MotorRun(LEFT_SIDE);
    MotorRun(RIGHT_SIDE);

    //
    // Make sure the wheel is marked as not running.  This will force the
    // wheel speed calculator to collect two edges before computing an
    // actual speed.  Then set the actual wheel speed to the commanded speed
    // so that the initial error calculation will be much closer to actual.
    //
    g_sMotorDrives[LEFT_SIDE].bRunning = 0;
    g_sMotorDrives[RIGHT_SIDE].bRunning = 0;
    g_sMotorDrives[LEFT_SIDE].lActualSpeed = ulSpeed;
    g_sMotorDrives[RIGHT_SIDE].lActualSpeed = ulSpeed;

    //
    // Since we (maybe) changed the speed, reset the PID controller
    //
    PIDReset(&g_sMotorDrives[LEFT_SIDE].sPID);
    PIDReset(&g_sMotorDrives[RIGHT_SIDE].sPID);
}

//****************************************************************************
//
// This function initializes the motor speed control module.  It must
// be called before any other function in this file.
//
//****************************************************************************
void
DriveInit(void)
{
    unsigned int uMotor;

    //
    // Initialize a hardware timer that will just be used as a free-running
    // timer for measuring duration between wheel sensor pulses.  Interrupts
    // are not used, just let it run.
    //
    ROM_SysCtlPeripheralEnable(WHEEL_TIMER_PERIPH);
    ROM_TimerConfigure(WHEEL_TIMER_BASE, TIMER_CFG_PERIODIC_UP);
    ROM_TimerLoadSet(WHEEL_TIMER_BASE, TIMER_BOTH, 0xffffffff);
    ROM_TimerEnable(WHEEL_TIMER_BASE, TIMER_BOTH);

    //
    // Compute the wheel tick rate constant.  This is part of the RPM
    // calculation that can be pre-computed.  This is based on the
    // following math:
    //
    //           Ftick(tick/sec) * 60(sec/min)
    // Srpm = ----------------------------------  =====> (rev/min)
    //        Telapsed(tick/click) * 8(click/rev)
    //
    // where:
    //  Ftick is the frequency of the wheel timer in ticks/second
    //  Telapsed is the elapsed time between "clicks" in timer ticks
    //  There are 8 wheel "clicks" per revolution
    //
    // The above can then be reduced to:
    //
    // RPM = (Ftick * 15) / (Telapsed * 2)
    //
    // All of this is known now except Telapsed, so the constant is:
    //
    // Kwheel = Ftick * 15 / 2
    //
    g_ulWheelTimerTickConstant = (ROM_SysCtlClockGet() * 15) / 2;

    //
    // Pre-compute the number of ticks represented by the minimum and
    // maximum reasonable RPM so that they can be used in range comparison
    //
    g_ulMaxRPMTicks = g_ulWheelTimerTickConstant / MAX_WHEEL_RPM;
    g_ulMinRPMTicks = g_ulWheelTimerTickConstant / MIN_WHEEL_RPM;

    //
    // Initialize the motor hardware
    //
    MotorsInit();

    //
    // Initialize the wheel sensors, providing handler for wheel clicks
    //
    WheelSensorsInit(DriveWheelSensorHandler);

    //
    // Process once each for left and right motor
    //
    for(uMotor = 0; uMotor < 2; uMotor++)
    {
        tMotorDrive *pMotor = &g_sMotorDrives[uMotor];

        //
        // Initialize the PID controller
        //
        PIDInitialize(&pMotor->sPID,
                      MOTORPID_INTEGRATOR_MAX, MOTORPID_INTEGRATOR_MIN,
                      MOTORPID_PGAIN, MOTORPID_IGAIN, MOTORPID_DGAIN);

        //
        // Set the actual and target speeds to 0
        //
        pMotor->lActualSpeed = 0;
        pMotor->lTargetSpeed = 0;
        pMotor->bRunning = 0;
    }

    //
    // Enable the wheel sensors
    //
    WheelSensorEnable();
    WheelSensorIntEnable(WHEEL_LEFT);
    WheelSensorIntEnable(WHEEL_RIGHT);
}
