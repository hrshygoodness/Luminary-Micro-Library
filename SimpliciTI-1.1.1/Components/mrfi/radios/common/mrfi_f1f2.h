/**************************************************************************************************
  Revised:        $Date: 2007-07-06 11:19:00 -0700 (Fri, 06 Jul 2007) $
  Revision:       $Revision: 13579 $

  Copyright 2007 Texas Instruments Incorporated.  All rights reserved.

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

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS”
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

/* ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 *   MRFI (Minimal RF Interface)
 *   Include file for MRFI common code between
 *   radio family 1 and radio family 2.
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

#ifndef MRFI_COMMON_H
#define MRFI_COMMON_H


/* ------------------------------------------------------------------------------------------------
 *                                       SmartRF Configuration
 * ------------------------------------------------------------------------------------------------
 */
#if (defined MRFI_CC1100)
#include "smartrf/CC1100/smartrf_CC1100.h"
#elif (defined MRFI_CC1101)
#include "smartrf/CC1101/smartrf_CC1101.h"
#elif (defined MRFI_CC2500)
#include "smartrf/CC2500/smartrf_CC2500.h"
#elif (defined MRFI_CC1110)
#include "smartrf/CC1110/smartrf_CC1110.h"
#elif (defined MRFI_CC1111)
#include "smartrf/CC1111/smartrf_CC1111.h"
#elif (defined MRFI_CC2510)
#include "smartrf/CC2510/smartrf_CC2510.h"
#elif (defined MRFI_CC2511)
#include "smartrf/CC2511/smartrf_CC2511.h"
#elif (defined MRFI_CC1100E_470)
#include "smartrf/CC1100E/470/smartrf_CC1100E.h"
#elif (defined MRFI_CC1100E_950)
#include "smartrf/CC1100E/950/smartrf_CC1100E.h"
#else
#error "ERROR: A valid radio is not specified."
#endif


/* ------------------------------------------------------------------------------------------------
 *                                         Prototypes
 * ------------------------------------------------------------------------------------------------
 */
uint8_t MRFI_RxAddrIsFiltered(uint8_t * pAddr);


/**************************************************************************************************
 */
#endif
