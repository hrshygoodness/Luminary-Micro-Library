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

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS?  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY
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
 *   Shared code between Radio Family 1 & Family 2
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "mrfi.h"
#include "mrfi_defs.h"
#include "mrfi_f1f2.h"
#include "bsp.h"
#include "bsp_macros.h"


/* ------------------------------------------------------------------------------------------------
 *                                          Common
 * ------------------------------------------------------------------------------------------------
 */

/* Packet automation control - base value is power up value whick has APPEND_STATUS enabled; no CRC autoflush */
#define PKTCTRL1_BASE_VALUE         BV(2)
#define PKTCTRL1_ADDR_FILTER_OFF    PKTCTRL1_BASE_VALUE
#define PKTCTRL1_ADDR_FILTER_ON     (PKTCTRL1_BASE_VALUE | (BV(0)|BV(1)))

#ifdef MRFI_ASSERTS_ARE_ON
#define RX_FILTER_ADDR_INITIAL_VALUE  0xFF
#endif


/* ------------------------------------------------------------------------------------------------
 *                                       Radio Abstraction
 * ------------------------------------------------------------------------------------------------
 */

/* ----- Radio Family 1 ----- */
#if (defined MRFI_RADIO_FAMILY1)
#include "../family1/mrfi_spi.h"
#define MRFI_WRITE_REGISTER(x,y)      mrfiSpiWriteReg( x, y )

/* ----- Radio Family 2 ----- */
#elif (defined MRFI_RADIO_FAMILY2)
#define MRFI_WRITE_REGISTER(x,y)      st( x = y; )

/* unknown or missing radio family */
#else
#error "ERROR: Expected radio family not specified.  Most likely a project issue."
#endif


/* ------------------------------------------------------------------------------------------------
 *                                    Global Constants
 * ------------------------------------------------------------------------------------------------
 */
const uint8_t mrfiBroadcastAddr[] = { 0xFF, 0xFF, 0xFF, 0xFF };

/* verify number of table entries matches the corresponding #define */
BSP_STATIC_ASSERT(MRFI_ADDR_SIZE == ((sizeof(mrfiBroadcastAddr)/sizeof(mrfiBroadcastAddr[0])) * sizeof(mrfiBroadcastAddr[0])));


/* ------------------------------------------------------------------------------------------------
 *                                    Local Constants
 * ------------------------------------------------------------------------------------------------
 */

/*
 *  Logical channel table - this table translates logical channel into
 *  actual radio channel number.  Channel 0, the default channel, is
 *  determined by the channel exported from SmartRF Studio.  The other
 *  table entries are derived from that default.  Each derived channel is
 *  masked with 0xFF to prevent generation of an illegal channel number.
 *
 *  This table is easily customized.  Just replace or add entries as needed.
 *  If the number of entries changes, the corresponding #define must also
 *  be adjusted.  It is located in mrfi_defs.h and is called __mrfi_NUM_LOGICAL_CHANS__.
 *  The static assert below ensures that there is no mismatch.
 */
#if defined( MRFI_CC2500 ) || defined( MRFI_CC2510 ) || defined( MRFI_CC2511 )
static const uint8_t mrfiLogicalChanTable[] =
{
  SMARTRF_SETTING_CHANNR,
  103,
  202,
  212
};
#elif defined( MRFI_CC1100 ) || defined( MRFI_CC1101 ) || defined( MRFI_CC1110 ) || defined( MRFI_CC1111 )
static const uint8_t mrfiLogicalChanTable[] =
{
  SMARTRF_SETTING_CHANNR,
  50,
  80,
  110
};
#elif defined(MRFI_CC1100E_470)
static const uint8_t mrfiLogicalChanTable[] =
{
  SMARTRF_SETTING_CHANNR,
  40,
  60,
  80
};
#elif defined(MRFI_CC1100E_950)
static const uint8_t mrfiLogicalChanTable[] =
{
  SMARTRF_SETTING_CHANNR,
  10,
  15,
  20
};
#else
#error "ERROR: A valid radio is not specified."
#endif

/* verify number of table entries matches the corresponding #define */
BSP_STATIC_ASSERT(__mrfi_NUM_LOGICAL_CHANS__ == ((sizeof(mrfiLogicalChanTable)/sizeof(mrfiLogicalChanTable[0])) * sizeof(mrfiLogicalChanTable[0])));

/*
 *  RF Power setting table - this table translates logical power value
 *  to radio register setting.  The logical power value is used directly
 *  as an index into the power setting table. The values in the table are
 *  from low to high. The default settings set 3 values: -20 dBm, -10 dBm,
 *  and 0 dBm. The default at startup is the highest value. Note that these
 *  are approximate depending on the radio. Information is taken from the
 *  data sheet.
 *
 *  This table is easily customized.  Just replace or add entries as needed.
 *  If the number of entries changes, the corresponding #define must also
 *  be adjusted.  It is located in mrfi_defs.h and is called __mrfi_NUM_POWER_SETTINGS__.
 *  The static assert below ensures that there is no mismatch.
 */
#if defined( MRFI_CC2500 )
static const uint8_t mrfiRFPowerTable[] =
{
  0x46,
  0x97,
  0xFE
};

#elif defined( MRFI_CC2510 ) || defined( MRFI_CC2511 )
static const uint8_t mrfiRFPowerTable[] =
{
  0xC1,
  0xCB,
  0xFE
};

#elif defined( MRFI_CC1100 )
static const uint8_t mrfiRFPowerTable[] =
{
  0x0D,
  0x34,
  0x8E
};

#elif defined( MRFI_CC1101 )
static const uint8_t mrfiRFPowerTable[] =
{
  0x0F,
  0x27,
  0x50
};

#elif defined( MRFI_CC1110 ) || defined( MRFI_CC1111 )
static const uint8_t mrfiRFPowerTable[] =
{
  0x0E,
  0x27,
  0x50
};
#elif defined(MRFI_CC1100E_470)
static const uint8_t mrfiRFPowerTable[] =
{
 0x0E,
 0x34,
 0x60
};
#elif defined(MRFI_CC1100E_950)
static const uint8_t mrfiRFPowerTable[] =
{
  0x0E,
  0x27,
  0x8E
};
#endif

/* verify number of table entries matches the corresponding #define */
BSP_STATIC_ASSERT(__mrfi_NUM_POWER_SETTINGS__ == ((sizeof(mrfiRFPowerTable)/sizeof(mrfiRFPowerTable[0])) * sizeof(mrfiRFPowerTable[0])));


/* ------------------------------------------------------------------------------------------------
 *                                       Local Variables
 * ------------------------------------------------------------------------------------------------
 */
static uint8_t mrfiRxFilterEnabled=0;
static uint8_t mrfiRxFilterAddr[MRFI_ADDR_SIZE] = { RX_FILTER_ADDR_INITIAL_VALUE };


/**************************************************************************************************
 * @fn          MRFI_SetLogicalChannel
 *
 * @brief       Set logical channel.
 *
 * @param       chan - logical channel number
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_SetLogicalChannel(uint8_t chan)
{
  /* logical channel is not valid? */
  MRFI_ASSERT( chan < MRFI_NUM_LOGICAL_CHANS );

  /* make sure radio is off before changing channels */
  Mrfi_RxModeOff();

  MRFI_WRITE_REGISTER( CHANNR, mrfiLogicalChanTable[chan] );

  /* turn radio back on if it was on before channel change */
  if(mrfiRadioState == MRFI_RADIO_STATE_RX)
  {
    Mrfi_RxModeOn();
  }
}

/**************************************************************************************************
 * @fn          MRFI_SetRFPwr
 *
 * @brief       Set RF Output power level.
 *
 * @param       idx - index into power table.
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_SetRFPwr(uint8_t idx)
{
  /* is power level specified valid? */
  MRFI_ASSERT( idx < MRFI_NUM_POWER_SETTINGS );

  /* make sure radio is off before changing power levels */
  Mrfi_RxModeOff();

  MRFI_WRITE_REGISTER( PA_TABLE0, mrfiRFPowerTable[idx] );

  /* turn radio back on if it was on before power level change */
  if(mrfiRadioState == MRFI_RADIO_STATE_RX)
  {
    Mrfi_RxModeOn();
  }
}

/**************************************************************************************************
 * @fn          MRFI_SetRxAddrFilter
 *
 * @brief       Set the address used for filtering received packets.
 *
 * @param       pAddr - pointer to address to use for filtering
 *
 * @return      zero     : successfully set filter address
 *              non-zero : illegal address
 **************************************************************************************************
 */
uint8_t MRFI_SetRxAddrFilter(uint8_t * pAddr)
{
  /*
   *  If first byte of filter address match fir byte of broadcast address,
   *  there is a conflict with hardware filtering.
   */
  if (pAddr[0] == mrfiBroadcastAddr[0])
  {
    /* unable to set filter address */
    return( 1 );
  }

  /*
   *  Set the hardware address register.  The hardware address filtering only recognizes
   *  a single byte but this does provide at least some automatic hardware filtering.
   */
  MRFI_WRITE_REGISTER( ADDR, pAddr[0] );

  /* save a copy of the filter address */
  {
    uint8_t i;

    for (i=0; i<MRFI_ADDR_SIZE; i++)
    {
      mrfiRxFilterAddr[i] = pAddr[i];
    }
  }

  /* successfully set filter address */
  return( 0 );
}


/**************************************************************************************************
 * @fn          MRFI_EnableRxAddrFilter
 *
 * @brief       Enable received packet filtering.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_EnableRxAddrFilter(void)
{
  MRFI_ASSERT(mrfiRxFilterAddr[0] != mrfiBroadcastAddr[0]); /* filter address must be set before enabling filter */

  /* set flag to indicate filtering is enabled */
  mrfiRxFilterEnabled = 1;

  /* enable hardware filtering on the radio */
  MRFI_WRITE_REGISTER( PKTCTRL1, PKTCTRL1_ADDR_FILTER_ON );
}


/**************************************************************************************************
 * @fn          MRFI_DisableRxAddrFilter
 *
 * @brief       Disable received packet filtering.
 *
 * @param       pAddr - pointer to address to test for filtering
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_DisableRxAddrFilter(void)
{
  /* clear flag that indicates filtering is enabled */
  mrfiRxFilterEnabled = 0;

  /* disable hardware filtering on the radio */
  MRFI_WRITE_REGISTER( PKTCTRL1, PKTCTRL1_ADDR_FILTER_OFF );
}


/**************************************************************************************************
 * @fn          MRFI_RxAddrIsFiltered
 *
 * @brief       Determine if address is filtered.
 *
 * @param       none
 *
 * @return      zero     : address is not filtered
 *              non-zero : address is filtered
 **************************************************************************************************
 */
uint8_t MRFI_RxAddrIsFiltered(uint8_t * pAddr)
{
  uint8_t i;
  uint8_t addrByte;
  uint8_t filterAddrMatches;
  uint8_t broadcastAddrMatches;

  /* first check to see if filtering is even enabled */
  if (!mrfiRxFilterEnabled)
  {
    /*
     *  Filtering is not enabled, so by definition the address is
     *  not filtered.  Return zero to indicate address is not filtered.
     */
    return( 0 );
  }

  /* clear address byte match counts */
  filterAddrMatches    = 0;
  broadcastAddrMatches = 0;

  /* loop through address to see if there is a match to filter address of broadcast address */
  for (i=0; i<MRFI_ADDR_SIZE; i++)
  {
    /* get byte from address to check */
    addrByte = pAddr[i];

    /* compare byte to filter address byte */
    if (addrByte == mrfiRxFilterAddr[i])
    {
      filterAddrMatches++;
    }
    if (addrByte == mrfiBroadcastAddr[i])
    {
      broadcastAddrMatches++;
    }
  }

  /*
   *  If address is *not* filtered, either the "filter address match count" or
   *  the "broadcast address match count" will equal the total number of bytes
   *  in the address.
   */
  if ((broadcastAddrMatches == MRFI_ADDR_SIZE) || (filterAddrMatches == MRFI_ADDR_SIZE))
  {
    /* address *not* filtered, return zero */
    return( 0 );
  }
  else
  {
    /* address filtered, return non-zero */
    return( 1 );
  }
}


/**************************************************************************************************
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */

/*
 *  These asserts happen if there is extraneous compiler padding of arrays.
 *  Modify compiler settings for no padding, or, if that is not possible,
 *  comment out the offending asserts.
 */
BSP_STATIC_ASSERT(sizeof(mrfiLogicalChanTable) == ((sizeof(mrfiLogicalChanTable)/sizeof(mrfiLogicalChanTable[0])) * sizeof(mrfiLogicalChanTable[0])));
BSP_STATIC_ASSERT(sizeof(mrfiBroadcastAddr) == ((sizeof(mrfiBroadcastAddr)/sizeof(mrfiBroadcastAddr[0])) * sizeof(mrfiBroadcastAddr[0])));


/**************************************************************************************************
*/
