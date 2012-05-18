/**************************************************************************************************
  Filename:       mrfi_board.c

  Copyright 2010 Texas Instruments Incorporated. All rights reserved.

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

/* ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 *   MRFI (Minimal RF Interface)
 *   Board code file.
 *   Target : Texas Instruments DK-LM3S9B96
 *            Stellaris Development Kit with EM Adapter
 *   Radios : CC2500
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "bsp.h"
#include "mrfi_defs.h"

#ifdef MRFI_CC2520
/* This function is defined in mrfi/radios/family3/mrfi_radio.c */
#define MRFI_RX_INT_HANDLER Mrfi_FiFoPIsr
#else
/* This function is defined in mrfi/radios/family1/mrfi_radio.c */
#define MRFI_RX_INT_HANDLER MRFI_GpioIsr
#endif

extern void MRFI_RX_INT_HANDLER(void);
/*
 * The common SPI implementation in the radio code assumes a non-FIFOed SPI
 * controller so we need to model this in the Stellaris implementation.  To
 * do this, every byte written is followed by a SPI read into this variable
 * and the value in this variable is returned any time a SPI read is requested.
 * This is a bit unpleasant but allows us to pick up support for all the
 * existing radio families without having to produce Stellaris-specific
 * code for each one.
 */
unsigned char g_ucSPIReadVal;

/**************************************************************************************************
 * @fn          MRFI_GpioPort1Isr
 *
 * @brief       -
 *
 * @param       -
 *
 * @return      -
 **************************************************************************************************
 */
BSP_ISR_FUNCTION( BSP_GpioPort1Isr, INT_GPIOH )
{
  /*
   *  This ISR is easily replaced.  The new ISR must simply
   *  include the following function call.
   */
    MRFI_RX_INT_HANDLER();
}


/**************************************************************************************************
 *                              Board-specific SPI pin definitions
 **************************************************************************************************
 */
#include "mrfi_board_defs.h"


/**************************************************************************************************
 */


