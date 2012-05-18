/**************************************************************************************************
  Filename:       bsp_msp430_defs.h
  Revised:        $Date: 2009-10-11 18:52:46 -0700 (Sun, 11 Oct 2009) $
  Revision:       $Revision: 20897 $

  Copyright 2007-2010 Texas Instruments Incorporated.  All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights granted under
  the terms of a software license agreement between the user who downloaded the software,
  his/her employer (which must be your employer) and Texas Instruments Incorporated (the
  "License"). You may not use this Software unless you agree to abide by the terms of the
  License. The License limits your use, and you acknowledge, that the Software may not be
  modified, copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio frequency
  transceiver, which is integrated into your product. Other than for the foregoing purpose,
  you may not use, reproduce, copy, prepare derivative works of, modify, distribute,
  perform, display or sell this Software and/or its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED �AS IS�
  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY
  WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
  IN NO EVENT SHALL TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE
  THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY
  INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST
  DATA, COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY
  THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *   BSP (Board Support Package)
 *   MCU : Texas Instruments Stellaris family
 *   Microcontroller definition file.
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 */

#ifndef BSP_STELLARIS_DEFS_H
#define BSP_STELLARIS_DEFS_H

/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */
#define BSP_MCU_STELLARIS

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/cpu.h"

/*
 * Note: This implementation does not automatically hook an ISR defined using
 * __bsp_ISR_FUNCTION__ since the Stellaris vector table is typically stored
 * in flash.  As a result, it is vital that you install the function
 * BSP_GpioPort1Isr() in the vector table for the GPIO port which contains the
 * pin used as the asynchronous signal from the radio.
 *
 */
#define __bsp_ISTATE_T__            unsigned char
#define __bsp_ISR_FUNCTION__(f,v)   void f(void)

#define BSP_EARLY_INIT(void)

#define __bsp_ENABLE_INTERRUPTS__()       CPUcpsie()
#define __bsp_DISABLE_INTERRUPTS__()      CPUcpsid()
#define __bsp_INTERRUPTS_ARE_ENABLED__()  (CPUprimask() ? false : true)

#define __bsp_GET_ISTATE__()              CPUprimask()
#define __bsp_RESTORE_ISTATE__(x)         do                                \
                                          {                                 \
                                              if(x)                         \
                                              {                             \
                                                  CPUcpsid();               \
                                              }                             \
                                              else                          \
                                              {                             \
                                                  CPUcpsie();               \
                                              }                             \
                                           } while (0);

#define __bsp_QUOTED_PRAGMA__(x)          _Pragma(#x)

/* ------------------------------------------------------------------------------------------------
 *                                          Common
 * ------------------------------------------------------------------------------------------------
 */
#define __bsp_LITTLE_ENDIAN__   1
#define __bsp_CODE_MEMSPACE__   /* blank */
#define __bsp_XDATA_MEMSPACE__  /* blank */

typedef   signed char     int8_t;
typedef   signed short    int16_t;
typedef   signed long     int32_t;

typedef   unsigned char   uint8_t;
typedef   unsigned short  uint16_t;
typedef   unsigned long   uint32_t;

#ifndef NULL
#define NULL 0
#endif

/**************************************************************************************************
 */
#endif
