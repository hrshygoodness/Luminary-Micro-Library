//*****************************************************************************
//
// limit.h - Supports the operation of the limit switches.
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

#ifndef __LIMIT_H__
#define __LIMIT_H__

//*****************************************************************************
//
// The bit positions of the flags in g_ulLimitFlags.
//
//*****************************************************************************
#define LIMIT_FLAG_FWD_OK       0
#define LIMIT_FLAG_REV_OK       1
#define LIMIT_FLAG_SFWD_OK      2
#define LIMIT_FLAG_SREV_OK      3
#define LIMIT_FLAG_STKY_FWD_OK  4
#define LIMIT_FLAG_STKY_REV_OK  5
#define LIMIT_FLAG_STKY_SFWD_OK 6
#define LIMIT_FLAG_STKY_SREV_OK 7

//*****************************************************************************
//
// This function determines if either the state of the forward limit switch or
// the forward soft limit allows the motor to operate in the forward direction.
//
//*****************************************************************************
#define LimitForwardOK()                                                      \
        (HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_FWD_OK) == 1)

//*****************************************************************************
//
// This function determines if only the state of the forward soft limit allows
// the motor to operate in the forward direction.
//
//*****************************************************************************
#define LimitSoftForwardOK()                                                  \
        (HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_SFWD_OK) == 1)

//*****************************************************************************
//
// This function determines if either the foward limit or the forward soft
// limit has ever tripped.  The state of this 'sticky' flag can only be cleared
// by calling LimitStickyForwardClear().
//
//*****************************************************************************
#define LimitStickyForwardOK()                                                \
        (HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_FWD_OK) == 1)

//*****************************************************************************
//
// This function determines if only the forward soft limit has ever tripped.
// The state of this 'sticky' flag can only be cleared by calling
// LimitStickySoftForwardClear().
//
//*****************************************************************************
#define LimitStickySoftForwardOK()                                            \
        (HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_SFWD_OK) == 1)

//*****************************************************************************
//
// This function determines if the state of the reverse limit switch allows the
// motor to operate in the reverse direction.
//
//*****************************************************************************
#define LimitReverseOK()                                                      \
        (HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_REV_OK) == 1)

//*****************************************************************************
//
// This function determines if only the state of the reverse soft limit allows
// the motor to operate in the reverse direction.
//
//*****************************************************************************
#define LimitSoftReverseOK()                                                  \
        (HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_SREV_OK) == 1)

//*****************************************************************************
//
// This function determines if either the reverse limit or the reverse soft
// limit has ever tripped.  The state of this 'sticky' flag can only be cleared
// by calling LimitStickyReverseClear().
//
//*****************************************************************************
#define LimitStickyReverseOK()                                                \
        (HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_REV_OK) == 1)

//*****************************************************************************
//
// This function determines if only the reverse soft limit has ever tripped.
// The state of this 'sticky' flag can only be cleared by calling
// LimitStickySoftReverseClear().
//
//*****************************************************************************
#define LimitStickySoftReverseOK()                                            \
        (HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_SREV_OK) == 1)

//*****************************************************************************
//
// This function resets the sticky flag for the forward limits.
//
//*****************************************************************************
#define LimitStickyForwardClear()                                             \
        HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_FWD_OK) = 1

//*****************************************************************************
//
// This function resets the sticky flag for only the forward soft limit.
//
//*****************************************************************************
#define LimitStickySoftForwardClear()                                         \
        HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_SFWD_OK) = 1

//*****************************************************************************
//
// This function resets the sticky flag for the reverse limits.
//
//*****************************************************************************
#define LimitStickyReverseClear()                                             \
        HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_REV_OK) = 1

//*****************************************************************************
//
// This function resets the sticky flag for only the reverse soft limit.
//
//*****************************************************************************
#define LimitStickySoftReverseClear()                                         \
        HWREGBITW(&g_ulLimitFlags, LIMIT_FLAG_STKY_SREV_OK) = 1

//*****************************************************************************
//
// Function prototypes.
//
//*****************************************************************************
extern unsigned long g_ulLimitFlags;
extern void LimitInit(void);
extern void LimitPositionEnable(void);
extern void LimitPositionDisable(void);
extern unsigned long LimitPositionActive(void);
extern void LimitPositionForwardSet(long lPosition, unsigned long ulLessThan);
extern void LimitPositionForwardGet(long *plPosition,
                                    unsigned long *pulLessThan);
extern void LimitPositionReverseSet(long lPosition, unsigned long ulLessThan);
extern void LimitPositionReverseGet(long *plPosition,
                                    unsigned long *pulLessThan);
extern void LimitTick(void);

#endif // __LIMIT_H__
