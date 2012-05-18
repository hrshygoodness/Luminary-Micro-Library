/**************************************************************************************************

  Copyright 2010 Texas Instruments Incorporated.  All rights reserved.

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
 *   Target : Texas Instruments DK-LM3S9D96
 *            Stellaris Development Kit with EM Adapter
 *   Board definition file.
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 */

#ifndef BSP_BOARD_DEFS_H
#define BSP_BOARD_DEFS_H


/* ------------------------------------------------------------------------------------------------
 *                                     Board Unique Define
 * ------------------------------------------------------------------------------------------------
 */
#define BSP_BOARD_DK_LM3S9D96

/* ------------------------------------------------------------------------------------------------
 *                                    Configuration header
 * ------------------------------------------------------------------------------------------------
 */

/*
 * SimpliciTI relies upon a significant number of configuration options that are
 * passed on the compiler command line.  The following gives you the option of
 * setting these via a header file instead.  If SIMPLICIY_CONFIG_HEADER is
 * defined, then the configuration options will be read from the header and
 * they should not be provided via the command line (or via a ".dat" file as
 * is done for the IAR projects).  This aids portability between toolchains
 * since the header file definitions are standard across all ANSI C compilers.
 */
#ifdef SIMPLICITI_CONFIG_HEADER
#include "simpliciti_config.h"
#endif

/* ------------------------------------------------------------------------------------------------
 *                                           Mcu
 * ------------------------------------------------------------------------------------------------
 */
#include "mcus/bsp_stellaris_defs.h"

/* ------------------------------------------------------------------------------------------------
 *                               Peripheral Driver Library
 * ------------------------------------------------------------------------------------------------
 */
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Clock
 * ------------------------------------------------------------------------------------------------
 */
#include "bsp_config.h"
#define __bsp_CLOCK_MHZ__    BSP_CONFIG_CLOCK_MHZ


/* ------------------------------------------------------------------------------------------------
 *                                     Board Initialization
 * ------------------------------------------------------------------------------------------------
 */
#define BSP_BOARD_C               "bsp_board.c"
#define BSP_INIT_BOARD()          BSP_InitBoard()
#define BSP_DELAY_USECS(x)        BSP_Delay(x)

void BSP_InitBoard(void);
void BSP_Delay(uint16_t usec);

/**************************************************************************************************
 */
#endif
