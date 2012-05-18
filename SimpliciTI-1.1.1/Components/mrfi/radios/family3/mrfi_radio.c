/**************************************************************************************************
  Revised:        $Date: 2007-07-06 11:19:00 -0700 (Fri, 06 Jul 2007) $
  Revision:       $Revision: 13579 $

  Copyright 2007-2009 Texas Instruments Incorporated.  All rights reserved.

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
 *   Radios: CC2520
 *   Primary code file for supported radios.
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include <string.h>
#include "bsp_macros.h"
#include "bsp.h"
#include "mrfi_spi.h"
#include "mrfi.h"
#include "mrfi_defs.h"
#include "bsp_external/mrfi_board_defs.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *     IEEE 802.15.4 definitions
 *   - - - - - - - - - - - - - - -
 */
#define IEEE_PHY_PACKET_SIZE_MASK   0x7F
#define IEEE_USECS_PER_SYMBOL       16


/* ------------------------------------------------------------------------------------------------
 *                                    Global Constants
 * ------------------------------------------------------------------------------------------------
 */
const uint8_t mrfiBroadcastAddr[] = { 0xFF, 0xFF, 0xFF, 0xFF };

/*
 *  Verify number of table entries matches the corresponding #define.
 *  If this assert is hit, most likely the number of initializers in the
 *  above array must be adjusted.
 */
BSP_STATIC_ASSERT(MRFI_ADDR_SIZE == ((sizeof(mrfiBroadcastAddr)/sizeof(mrfiBroadcastAddr[0])) * sizeof(mrfiBroadcastAddr[0])));

/* ------------------------------------------------------------------------------------------------
 *                                       Global Variables
 * ------------------------------------------------------------------------------------------------
 */
#ifdef MRFI_PA_LNA_ENABLED
uint8_t mrfiLnaHighGainMode = 1;
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */
#if (defined MRFI_CC2520)

  #define MRFI_RSSI_OFFSET    76   /* no units */

  /*
   *  For RSSI to be valid, we must wait for 20 symbol periods:
   *  - 12 symbols to go from idle to rx state
   *  - 8 symbols to calculate the RSSI value
   */
  #define MRFI_RSSI_VALID_DELAY_US    (20 * IEEE_USECS_PER_SYMBOL)

  #define MRFI_VREG_SETTLE_TIME_USECS        100    /* microseconds */

#else
  #error "ERROR: Some of the radio params are not defined for this radio."
#endif




#define MRFI_LENGTH_FIELD_OFFSET            __mrfi_LENGTH_FIELD_OFS__
#define MRFI_LENGTH_FIELD_SIZE              __mrfi_LENGTH_FIELD_SIZE__
#define MRFI_HEADER_SIZE                    __mrfi_HEADER_SIZE__
#define MRFI_FRAME_BODY_OFS                 __mrfi_DST_ADDR_OFS__
#define MRFI_BACKOFF_PERIOD_USECS           __mrfi_BACKOFF_PERIOD_USECS__
#define MRFI_DSN_OFFSET                     __mrfi_DSN_OFS__
#define MRFI_FCF_OFFSET                     __mrfi_FCF_OFS__


/*
 *  The FCF (Frame Control Field) will always have this value for *all* frames:
 *
 *    bits    description                         setting
 *  --------------------------------------------------------------------------------------
 *      0-2   Frame Type                          001 - data frame
 *        3   Security Enabled                      0 - security disabled
 *        4   Frame Pending                         0 - no pending data
 *        5   Ack Request                           0 - no Ack request
 *        6   PAN ID Compression                    0 - no PAN ID compression
 *        7   Reserved                              0 - reserved
 *  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *      8-9   Reserved                             00 - reserved
 *    10-11   Destination Addressing Mode          10 - PAN ID + 16-bit short address
 *    12-13   Frame Version                        00 - IEEE Std 802.15.4-2003
 *    14-15   Source Addressing Mode               10 - PAN ID + 16-bit short address
 *
 */
#define MRFI_FCF_0_7              (0x01)
#define MRFI_FCF_8_15             (0x88)
#define MRFI_MIN_SMPL_FRAME_SIZE  (MRFI_HEADER_SIZE + NWK_HDR_SIZE)

/* rx metrics definitions, known as appended "packet status bytes" in datasheet parlance */
#define MRFI_RX_METRICS_CRC_OK_MASK         __mrfi_RX_METRICS_CRC_OK_MASK__
#define MRFI_RX_METRICS_LQI_MASK            __mrfi_RX_METRICS_LQI_MASK__

/* Max time we can be in a critical section within the delay function.
 */
#define MRFI_MAX_DELAY_US                   16 /* usec */

/* Random number generator params */
#define MRFI_RANDOM_OFFSET                   67
#define MRFI_RANDOM_MULTIPLIER              109

#define MRFI_FILTER_ADDRESS_SET       BV(1)
#define MRFI_FILTER_ADDRESS_ENABLED   BV(2)

/* ------------------------------------------------------------------------------------------------
 *                                          Radio Abstraction
 * ------------------------------------------------------------------------------------------------
 */
#if (defined MRFI_CC2520)
  #define MRFI_RADIO_PARTNUM          0x84
  #define MRFI_RADIO_MIN_VERSION      0x00
#else
  #error "ERROR: Missing or unrecognized radio."
#endif

  /* The SW timer is calibrated by adjusting the call to the microsecond delay
   * routine. This allows maximum calibration control with repects to the longer
   * times requested by applicationsd and decouples internal from external calls
   * to the microsecond routine which can be calibrated independently.
   */
#if defined(SW_TIMER)
#define APP_USEC_VALUE    1000
#else
#define APP_USEC_VALUE    1000
#endif

/* ------------------------------------------------------------------------------------------------
 *                                    Local Constants
 * ------------------------------------------------------------------------------------------------
 */
/*
 *  Logical channel table - this table translates logical channel into
 *  actual radio channel number. Each derived channel is
 *  masked with 0xFF to prevent generation of an illegal channel number.
 *
 *  This table is easily customized.  Just replace or add entries as needed.
 *  If the number of entries changes, the corresponding #define must also
 *  be adjusted.  It is located in mrfi_defs.h and is called __mrfi_NUM_LOGICAL_CHANS__.
 *  The static assert below ensures that there is no mismatch.
 */
static const uint8_t mrfiLogicalChanTable[] =
{
  15,
  20,
  25,
  26
};

/* verify number of table entries matches the corresponding #define */
BSP_STATIC_ASSERT(__mrfi_NUM_LOGICAL_CHANS__ == ((sizeof(mrfiLogicalChanTable)/sizeof(mrfiLogicalChanTable[0])) * sizeof(mrfiLogicalChanTable[0])));

#if !defined(MRFI_PA_LNA_ENABLED)
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
static const uint8_t mrfiRFPowerTable[] =
{
  0x03,
  0x2C,
  0x32
};
#else  /* !MRFI_PA_LNA_ENABLED */
/* If LNA enabled we use 5 dBm, 10dBm and 15dBm as the entries defaulting to 15 */
static const uint8_t mrfiRFPowerTable[] =
{
  0x49,
  0x79,
  0xE0
};
#endif  /* !MRFI_PA_LNA_ENABLED */

/* verify number of table entries matches the corresponding #define */
BSP_STATIC_ASSERT(__mrfi_NUM_POWER_SETTINGS__ == ((sizeof(mrfiRFPowerTable)/sizeof(mrfiRFPowerTable[0])) * sizeof(mrfiRFPowerTable[0])));

/* ------------------------------------------------------------------------------------------------
 *                                       Local Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void Mrfi_FiFoPIsr(void); /* this called from mrfi_board.c */

static void   Mrfi_RxModeOn(void);
static void   Mrfi_RxModeOff(void);
static void   Mrfi_DelayUsec(uint16_t howLong);
static void   Mrfi_RandomBackoffDelay(void);
static void   Mrfi_TurnOnRadioPower(void);
static void   Mrfi_TurnOffRadioPower(void);
static int8_t Mrfi_CalculateRssi(uint8_t rawValue);
static void   Mrfi_DelayUsecSem(uint16_t);

/* ------------------------------------------------------------------------------------------------
 *                                       Local Variables
 * ------------------------------------------------------------------------------------------------
 */
static uint8_t      mrfiRadioState  = MRFI_RADIO_STATE_UNKNOWN;
static uint8_t      mrfiRndSeed = 0;
static mrfiPacket_t mrfiIncomingPacket;

static uint8_t mrfiFilterAddr[4] = {0, 0, 0, 0};
static uint8_t mrfiAddrFilterStatus = 0x0;
static uint8_t mrfiCurrentLogicalChannel = 0; /* Default logical channel */
static uint8_t mrfiCurrentPowerLevel = MRFI_NUM_POWER_SETTINGS - 1;


/* reply delay support */
static volatile uint8_t  sKillSem = 0;
static volatile uint8_t  sReplyDelayContext = 0;
static          uint16_t sReplyDelayScalar = 0;

/* ------------------------------------------------------------------------------------------------
 *                                       Local Macros
 * ------------------------------------------------------------------------------------------------
 */
#define MRFI_RSSI_VALID_WAIT()                                                \
{                                                                             \
  int16_t delay = MRFI_RSSI_VALID_DELAY_US;                                   \
  do                                                                          \
  {                                                                           \
    if(mrfiSpiCmdStrobe(SNOP) & RSSI_VALID)                                   \
    {                                                                         \
      break;                                                                  \
    }                                                                         \
    Mrfi_DelayUsec(64); /* sleep */                                           \
    delay -= 64;                                                              \
  }while(delay > 0);                                                          \
}                                                                             \

/* See Bug #1 CC2520 errata - swrz024.pdf. Flush must be done twice. */
#define MRFI_RADIO_FLUSH_RX_BUFFER()                                          \
{                                                                             \
  bspIState_t s;                                                              \
  BSP_ENTER_CRITICAL_SECTION(s);                                              \
  mrfiSpiCmdStrobe(SFLUSHRX);                                                 \
  mrfiSpiCmdStrobe(SFLUSHRX);                                                 \
  BSP_EXIT_CRITICAL_SECTION(s);                                               \
}

#define MRFI_RADIO_FLUSH_TX_BUFFER()  mrfiSpiCmdStrobe(SFLUSHTX)
#define MRFI_SAMPLED_CCA()            (SAMPLED_CCA_BV & mrfiSpiReadReg(FSMSTAT1))


/* ------------------------------------------------------------------------------------------------
 *                                       Functions
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          MRFI_Init
 *
 * @brief       Initialize MRFI.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_Init(void)
{
  memset(&mrfiIncomingPacket, 0x0, sizeof(mrfiIncomingPacket));
  /* Configure Output lines */
  MRFI_CONFIG_RESETN_PIN_AS_OUTPUT();
  MRFI_CONFIG_VREG_EN_PIN_AS_OUTPUT();

  /* Configure Input lines */
  MRFI_CONFIG_TX_FRAME_DONE_AS_INPUT();
  MRFI_CONFIG_FIFO_AS_INPUT();
  MRFI_CONFIG_FIFOP_AS_INPUT();

  /* Initialize SPI */
  mrfiSpiInit();

  /* Power up the radio chip */
  Mrfi_TurnOnRadioPower();

  /* Confirm that we are talking to the right hardware */
  MRFI_ASSERT(mrfiSpiReadReg(CHIPID) == MRFI_RADIO_PARTNUM);

  /* Random Number Generator:
   * The seed value for the randon number generator logic
   * is derived from the radio.
   */

  /* Set radio in rx mode, but with symbol search disabled. Used for RSSI
   * measurments or when we don't care about the received frames.
   */
  mrfiSpiWriteReg(FRMCTRL0, FRMCTRL0_RESET_VALUE | RX_MODE_RSSI_ONLY);

  /* Turn on the receiver */
  mrfiSpiCmdStrobe(SRXON);

  /*
   *  Wait for RSSI to be valid. RANDOM command strobe can be used
   *  to generate random number only after this.
   */
  MRFI_RSSI_VALID_WAIT();


  /* Get random byte from the radio */
  mrfiRndSeed = mrfiSpiRandomByte();

 /*
  *  The seed value must not be zero.  If it is, the pseudo random sequence
  *  will be always be zero. There is an extremely small chance this seed could
  *  randomly be zero (more likely some type of hardware problem would cause
  *  this). If it is zero, initialize it to something.
  */
  if(mrfiRndSeed == 0)
  {
      mrfiRndSeed = 0x80;
  }

  /* Random number initialization is done. Turn the radio off */
  Mrfi_TurnOffRadioPower();

  /* Initial radio state is - OFF state */
  mrfiRadioState = MRFI_RADIO_STATE_OFF;

  /**********************************************************************************
   *                            Compute reply delay scalar
   *
   * The IEEE radio has a fixed data rate of 250 Kbps. Data rate inference
   * from radio regsiters is not necessary for this radio.
   *
   * The maximum delay needed depends on the MAX_APP_PAYLOAD parameter. Figure
   * out how many bits that will be when overhead is included. Bits/bits-per-second
   * is seconds to transmit (or receive) the maximum frame. We multiply this number
   * by 1000 to find the time in milliseconds. We then additionally multiply by
   * 10 so we can add 5 and divide by 10 later, thus rounding up to the number of
   * milliseconds. This last won't matter for slow transmissions but for faster ones
   * we want to err on the side of being conservative and making sure the radio is on
   * to receive the reply. The semaphore monitor will shut it down. The delay adds in
   * a platform fudge factor that includes processing time on peer plus lags in Rx and
   * processing time on receiver's side. Also includes round trip delays from CCA
   * retries. This portion is included in PLATFORM_FACTOR_CONSTANT defined in mrfi.h.
   *
   * **********************************************************************************
   */

#define   PHY_PREAMBLE_SYNC_BYTES     8

  {
    uint32_t bits, dataRate = 250000;

    bits = ((uint32_t)((PHY_PREAMBLE_SYNC_BYTES + MRFI_MAX_FRAME_SIZE)*8))*10000;

    /* processing on the peer + the Tx/Rx time plus more */
    sReplyDelayScalar = PLATFORM_FACTOR_CONSTANT + (((bits/dataRate)+5)/10);
  }

    /* set default channel */
  MRFI_SetLogicalChannel( mrfiCurrentLogicalChannel );

  /* set default power */
  MRFI_SetRFPwr(mrfiCurrentPowerLevel);

  /* Random delay: This prevents devices on the same power source from repeated
   *  transmit collisions on power up.
   */
  Mrfi_RandomBackoffDelay();

  /* Clean out buffer to protect against spurious frames */
  memset(mrfiIncomingPacket.frame, 0x00, sizeof(mrfiIncomingPacket.frame));
  memset(mrfiIncomingPacket.rxMetrics, 0x00, sizeof(mrfiIncomingPacket.rxMetrics));

  BSP_ENABLE_INTERRUPTS();
}


/**************************************************************************************************
 * @fn          MRFI_WakeUp
 *
 * @brief       Wake up radio from off/sleep state.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_WakeUp(void)
{
  /* if radio is asleep, wake it up */
  if(mrfiRadioState == MRFI_RADIO_STATE_OFF)
  {
    /* enter idle mode */
    mrfiRadioState = MRFI_RADIO_STATE_IDLE;

    /* turn on radio power */
    Mrfi_TurnOnRadioPower();

    /* Configure the radio registers. All radio settings that are lost
     * on MRFI_Sleep() call must be restored here. Since we are putting the
     * radio in LPM2 power mode, all register and memory values that are
     * different from reset must be restored.
     */

    MRFI_BOARD_CONFIG_RADIO_GPIO();

#ifdef MRFI_PA_LNA_ENABLED

    /* Init ports */
    MRFI_BOARD_PA_LNA_CONFIG_PORTS();

    if(mrfiLnaHighGainMode)
    {
      /* Set LNA to High Gain Mode */
      MRFI_BOARD_PA_LNA_HGM();
    }
    else
    {
     /* Set LNA to Low Gain Mode */
      MRFI_BOARD_PA_LNA_LGM();
    }

#endif

    /* Set FIFO_P threshold to max (127). Thus a FIFO_P signal is set whenever
     * a full frame is received.
     */
    mrfiSpiWriteReg(FIFOPCTRL, 0x7F);

    /* Accept only DATA frames. Reject CMD/BECAON/ACK frames. */
    mrfiSpiWriteReg(FRMFILT1, 0x10);

    /* Restore the address filter settings */
    if(mrfiAddrFilterStatus & MRFI_FILTER_ADDRESS_SET)
    {
      MRFI_SetRxAddrFilter(mrfiFilterAddr);
    }

    if(mrfiAddrFilterStatus & MRFI_FILTER_ADDRESS_ENABLED)
    {
      MRFI_EnableRxAddrFilter();
    }
    else
    {
      MRFI_DisableRxAddrFilter();
    }

    /* Following values need to be changed from their reset value.
     * See Table-21 CC2520 datasheet.
     */

    mrfiSpiWriteReg(TXPOWER,  mrfiRFPowerTable[mrfiCurrentPowerLevel]);
    mrfiSpiWriteReg(CCACTRL0, 0xF8);
    mrfiSpiWriteReg(MDMCTRL0, 0x85);
    mrfiSpiWriteReg(MDMCTRL1, 0x14);
    mrfiSpiWriteReg(RXCTRL,   0x3F);
    mrfiSpiWriteReg(FSCTRL,   0x5A);
    mrfiSpiWriteReg(FSCAL1,   0x2B);
    mrfiSpiWriteReg(AGCCTRL1, 0x11);
    mrfiSpiWriteReg(ADCTEST0, 0x10);
    mrfiSpiWriteReg(ADCTEST1, 0x0E);
    mrfiSpiWriteReg(ADCTEST2, 0x03);

    /* set channel */
    MRFI_SetLogicalChannel(mrfiCurrentLogicalChannel);

    /* set power output level */

  }
}


/**************************************************************************************************
 * @fn          MRFI_SetRxAddrFilter
 *
 * @brief       Set the address used for filtering received packets.
 *
 * @param       pAddr - pointer to address to use for filtering
 *
 * @return      0 - success; 1 - failure
 **************************************************************************************************
 */
uint8_t MRFI_SetRxAddrFilter(uint8_t * pAddr)
{
  /*
   *  Determine if filter address is valid.  Cannot be a reserved value.
   *   -Reserved PAN ID's of 0xFFFF and 0xFFFE.
   *   -Reserved short address of 0xFFFF.
   */
  if ((((pAddr[0] == 0xFF) || (pAddr[0] == 0xFE)) && (pAddr[1] == 0xFF))  ||
        ((pAddr[2] == 0xFF) && (pAddr[3] == 0xFF)))
  {
    /* unable to set filter address */
    return( 1 );
  }

  /* Can access radio only if it is not OFF */
  if(mrfiRadioState != MRFI_RADIO_STATE_OFF)
  {
    /* set the hardware address registers */
    spiWriteRamByte(0x3F2, pAddr[0]); /* PANIDL */
    spiWriteRamByte(0x3F3, pAddr[1]); /* PANIDH */
    spiWriteRamByte(0x3F4, pAddr[2]); /* SHORTADDRL */
    spiWriteRamByte(0x3F5, pAddr[3]); /* SHORTADDRH */
  }

  /* Save the address so we can restore it after sleep. */
  mrfiFilterAddr[0] = pAddr[0];
  mrfiFilterAddr[1] = pAddr[1];
  mrfiFilterAddr[2] = pAddr[2];
  mrfiFilterAddr[3] = pAddr[3];

  /* remember if address was set */
  mrfiAddrFilterStatus |= MRFI_FILTER_ADDRESS_SET;

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
  MRFI_ASSERT( mrfiAddrFilterStatus & MRFI_FILTER_ADDRESS_SET ); /* filter address not set */

  mrfiAddrFilterStatus |= MRFI_FILTER_ADDRESS_ENABLED;

  /* Can access radio only if it is not OFF */
  if(mrfiRadioState != MRFI_RADIO_STATE_OFF)
  {
    /* enable hardware filtering on the radio */
    mrfiSpiBitSet(FRMFILT0, 0);
  }
}


/**************************************************************************************************
 * @fn          MRFI_DisableRxAddrFilter
 *
 * @brief       Disable received packet filtering.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_DisableRxAddrFilter(void)
{
  mrfiAddrFilterStatus &= ~(MRFI_FILTER_ADDRESS_ENABLED);

  /* Can access radio only if it is not OFF */
  if(mrfiRadioState != MRFI_RADIO_STATE_OFF)
  {
    /* disable hardware filtering on the radio */
    mrfiSpiBitClear(FRMFILT0, 0);
  }
}


/**************************************************************************************************
 * @fn          MRFI_Transmit
 *
 * @brief       Transmit a packet.
 *
 * @param       pPacket - pointer to packet to transmit
 *              txType  - FORCED or CCA
 *
 * @return      Return code indicates success or failure of transmit:
 *                  MRFI_TX_RESULT_SUCCESS - transmit succeeded
 *                  MRFI_TX_RESULT_FAILED  - transmit failed because CCA failed
 **************************************************************************************************
 */
uint8_t MRFI_Transmit(mrfiPacket_t * pPacket, uint8_t txType)
{
  uint8_t txResult = MRFI_TX_RESULT_SUCCESS;
  static uint8_t dsn = 0;

  /* radio must be awake to transmit */
  MRFI_ASSERT( mrfiRadioState != MRFI_RADIO_STATE_OFF );

  /* TX_DONE line status must be low. If high, some state logic problem. */
  MRFI_ASSERT(!MRFI_TX_DONE_STATUS());


  /* Turn off reciever. We ignore/drop incoming packets during transmit. */
  Mrfi_RxModeOff();


  /* --------------------------------------
   *    Populate the IEEE fields in frame
   *   ------------------------------------
   */

  /* set the sequence number, also known as DSN (Data Sequence Number) */
  pPacket->frame[MRFI_DSN_OFFSET]   = dsn++;
  pPacket->frame[MRFI_FCF_OFFSET]   = MRFI_FCF_0_7;
  pPacket->frame[MRFI_FCF_OFFSET+1] = MRFI_FCF_8_15;


  /* ------------------------------------------------------------------
   *    Write packet to transmit FIFO
   *   --------------------------------
   */
  {
    uint8_t txBufLen;
    uint8_t frameLen;
    uint8_t *p;

    /* flush FIFO of any previous transmit that did not go out */
    MRFI_RADIO_FLUSH_TX_BUFFER();

    /* set point at beginning of outgoing frame */
    p = &pPacket->frame[MRFI_LENGTH_FIELD_OFFSET];

    /* get number of bytes in the packet (does not include the length byte) */
    txBufLen = *p;

    /*
     *  Write the length byte to the FIFO.  This length does *not* include the length field
     *  itself but does include the size of the FCS (generically known as RX metrics) which
     *  is generated automatically by the radio.
     */
    frameLen = txBufLen + MRFI_RX_METRICS_SIZE;

    mrfiSpiWriteTxFifo(&frameLen, 1);

    /* skip the length field which we already sent to FIFO. */
    p++;
    /* write packet bytes to FIFO */
    mrfiSpiWriteTxFifo(p, txBufLen);
  }

  /* Forced transmit */
  if(txType == MRFI_TX_TYPE_FORCED)
  {
    /* NOTE: Bug (#1) described in the errata swrz024.pdf for CC2520:
     * We never strobe TXON when the radio is in receive state.
     * If this is changed, must implement the bug workaround as described in the
     * errata (flush the Rx FIFO).
     */

    /* strobe transmit */
    mrfiSpiCmdStrobe(STXON);

    /* wait for transmit to complete */
    while (!MRFI_TX_DONE_STATUS());

    /* Clear the TX_FRM_DONE exception flag register in the radio. */
    mrfiSpiBitClear(EXCFLAG0, 1);
  }
  else /* CCA Transmit */
  {
    /* set number of CCA retries */
    uint8_t ccaRetries = MRFI_CCA_RETRIES;

    MRFI_ASSERT( txType == MRFI_TX_TYPE_CCA );

    /* ======================================================================
    *    CCA Algorithm Loop
    * ======================================================================
    */
    while(1)
    {
      /* Turn ON the receiver to perform CCA. Can not call Mrfi_RxModeOn(),
      * since that will enable the rx interrupt, which we do not want.
      */
      mrfiSpiCmdStrobe(SRXON);

      /* Wait for RSSI to be valid. */
      MRFI_RSSI_VALID_WAIT();

      /* Request transmit on cca */
      mrfiSpiCmdStrobe(STXONCCA);

      /* If sampled CCA is set, transmit has begun. */
      if(MRFI_SAMPLED_CCA())
      {
        /* wait for transmit to complete */
        while( !MRFI_TX_DONE_STATUS() );

        /* Clear the TX_FRM_DONE exception flag register in the radio. */
        mrfiSpiBitClear(EXCFLAG0, 1);

        /* transmit is done. break out of CCA algorithm loop */
        break;
      }
      else
      {
        /* ------------------------------------------------------------------
         *    Clear Channel Assessment failed.
         * ------------------------------------------------------------------
         */

        /* Retry ? */
        if(ccaRetries != 0)
        {
          /* turn off reciever to conserve power during backoff */
          Mrfi_RxModeOff();

          /* delay for a random number of backoffs */
          Mrfi_RandomBackoffDelay();

          /* decrement CCA retries before loop continues */
          ccaRetries--;
        }
        else  /* No CCA retries left, abort */
        {
          /* set return value for failed transmit and break */
          txResult = MRFI_TX_RESULT_FAILED;
          break;
        }
      }
    } /* End CCA Algorithm Loop */
  }

  /* turn radio back off to put it in a known state */
  Mrfi_RxModeOff();

  /* If the radio was in RX state when transmit was attempted,
   * put it back in RX state.
   */
  if(mrfiRadioState == MRFI_RADIO_STATE_RX)
  {
    Mrfi_RxModeOn();
  }

  /* return the result of the transmit */
  return( txResult );
}


/**************************************************************************************************
 * @fn          MRFI_Receive
 *
 * @brief       Copies last packet received to the location specified.
 *              This function is meant to be called after the ISR informs
 *              higher level code that there is a newly received packet.
 *
 * @param       pPacket - pointer to location of where to copy received packet
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_Receive(mrfiPacket_t * pPacket)
{
  /* copy last received packed into indicated memory */
  *pPacket = mrfiIncomingPacket;
}


/**************************************************************************************************
 * @fn          MRFI_RxOn
 *
 * @brief       Turn on the receiver.  No harm is done if this function
 *              is called when receiver is already on.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_RxOn(void)
{
  /* radio must be powered ON before we can move it to RX state */
  MRFI_ASSERT( mrfiRadioState != MRFI_RADIO_STATE_OFF );

  /* Put the radio in RX state, if not already */
  if(mrfiRadioState != MRFI_RADIO_STATE_RX)
  {
    mrfiRadioState = MRFI_RADIO_STATE_RX;
    Mrfi_RxModeOn();
  }
}


/**************************************************************************************************
 * @fn          MRFI_RxIdle
 *
 * @brief       Put radio in idle mode (receiver is off).  No harm is done if
 *              this function is called when radio is already idle.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_RxIdle(void)
{
  /* radio must be powered ON to move it to idle mode */
  MRFI_ASSERT( mrfiRadioState != MRFI_RADIO_STATE_OFF );

  /* if radio is on, turn it off */
  if(mrfiRadioState == MRFI_RADIO_STATE_RX)
  {
    Mrfi_RxModeOff();
    mrfiRadioState = MRFI_RADIO_STATE_IDLE;
  }
}


/**************************************************************************************************
 * @fn          MRFI_Sleep
 *
 * @brief       Request radio go to sleep (Power OFF the radio chip).
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_Sleep(void)
{
  /* If radio is not asleep, put it to sleep */
  if(mrfiRadioState != MRFI_RADIO_STATE_OFF)
  {
    /* go to idle so radio is in a known state before sleeping */
    MRFI_RxIdle();

    /* turn off power to the radio */
    Mrfi_TurnOffRadioPower();

    /* Our new state is OFF */
    mrfiRadioState = MRFI_RADIO_STATE_OFF;
  }
}


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
  uint8_t phyChannel;

  /* logical channel is not valid */
  MRFI_ASSERT( chan < MRFI_NUM_LOGICAL_CHANS );

  /* make sure radio is off before changing channels */
  Mrfi_RxModeOff();

  /* convert logical channel number into physical channel number */
  phyChannel = mrfiLogicalChanTable[chan];

  /* write frequency value of new channel */
  mrfiSpiWriteReg(FREQCTRL, (FREQCTRL_BASE_VALUE + (FREQCTRL_FREQ_2405MHZ + 5 * ((phyChannel) - 11))));

  /* remember this. need it when waking up. */
  mrfiCurrentLogicalChannel = chan;

  /* Put the radio back in RX state, if it was in RX before channel change */
  if(mrfiRadioState == MRFI_RADIO_STATE_RX)
  {
    Mrfi_RxModeOn();
  }
}


/**************************************************************************************************
 * @fn          MRFI_SetRFPwr
 *
 * @brief       Set RF power level.
 *
 * @param       idx - index into power level table
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_SetRFPwr(uint8_t idx)
{
  /* is power level specified valid? */
  MRFI_ASSERT( idx < MRFI_NUM_POWER_SETTINGS );

  /* make sure radio is off before changing power level */
  Mrfi_RxModeOff();

  /* write value of new power level */
  mrfiSpiWriteReg(TXPOWER, mrfiRFPowerTable[idx]);

  /* remember this. need it when waking up. */
  mrfiCurrentPowerLevel = idx;

  /* Put the radio back in RX state, if it was in RX before change */
  if(mrfiRadioState == MRFI_RADIO_STATE_RX)
  {
    Mrfi_RxModeOn();
  }
}

/**************************************************************************************************
 * @fn          MRFI_Rssi
 *
 * @brief       Returns "live" RSSI value
 *
 * @param       none
 *
 * @return      RSSI value in units of dBm.
 **************************************************************************************************
 */
int8_t MRFI_Rssi(void)
{
  /* Radio must be in RX state to measure rssi. */
  MRFI_ASSERT( mrfiRadioState == MRFI_RADIO_STATE_RX );

  /* Wait for the RSSI to be valid:
   * Just having the Radio ON is not enough to read
   * the correct RSSI value. The Radio must in RX mode for
   * a certain duration.
   */
  MRFI_RSSI_VALID_WAIT();

  /* Read and convert RSSI to decimal and do offset compensation. */
  return( Mrfi_CalculateRssi(mrfiSpiReadReg(RSSI)) );
}


/**************************************************************************************************
 * @fn          MRFI_RandomByte
 *
 * @brief       Returns a random byte. This is a pseudo-random number generator.
 *              The generated sequence will repeat every 256 values.
 *              The sequence itself depends on the initial seed value.
 *
 * @param       none
 *
 * @return      a random byte
 **************************************************************************************************
 */
uint8_t MRFI_RandomByte(void)
{
  mrfiRndSeed = (mrfiRndSeed*MRFI_RANDOM_MULTIPLIER) + MRFI_RANDOM_OFFSET;

  return mrfiRndSeed;
}

/**************************************************************************************************
 * @fn          MRFI_PostKillSem
 *
 * @brief       Post to the loop-kill semaphore that will be checked by the iteration loops
 *              that control the delay thread.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_PostKillSem(void)
{

  if (sReplyDelayContext)
  {
    sKillSem = 1;
  }

  return;
}


/**************************************************************************************************
 * @fn          MRFI_DelayMs
 *
 * @brief       Delay the specified number of milliseconds.
 *
 * @param       milliseconds - delay time
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_DelayMs(uint16_t milliseconds)
{
  while(milliseconds)
  {
    Mrfi_DelayUsec( APP_USEC_VALUE );
    milliseconds--;
  }
}

/**************************************************************************************************
 * @fn          MRFI_ReplyDelay
 *
 * @brief       Delay number of milliseconds scaled by data rate. Check semaphore for
 *              early-out. Run in a separate thread when the reply delay is
 *              invoked. Cleaner then trying to make MRFI_DelayMs() thread-safe
 *              and reentrant.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_ReplyDelay()
{
  bspIState_t s;
  uint16_t    milliseconds = sReplyDelayScalar;

  BSP_ENTER_CRITICAL_SECTION(s);
  sReplyDelayContext = 1;
  BSP_EXIT_CRITICAL_SECTION(s);

  while (milliseconds)
  {
    Mrfi_DelayUsecSem( APP_USEC_VALUE );
    if (sKillSem)
    {
      break;
    }
    milliseconds--;
  }

  BSP_ENTER_CRITICAL_SECTION(s);
  sKillSem           = 0;
  sReplyDelayContext = 0;
  BSP_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          MRFI_GetRadioState
 *
 * @brief       Returns the current radio state.
 *
 * @param       none
 *
 * @return      radio state - off/idle/rx
 **************************************************************************************************
 */
uint8_t MRFI_GetRadioState(void)
{
  return mrfiRadioState;
}


/**************************************************************************************************
 * @fn          Mrfi_TurnOnRadioPower
 *
 * @brief       Power ON the radio chip.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_TurnOnRadioPower(void)
{
  /* put radio chip into reset */
  MRFI_DRIVE_RESETN_PIN_LOW();

  /* enable the voltage regulator */
  MRFI_DRIVE_VREG_EN_PIN_HIGH();

  /* wait for the chip to power up */
  Mrfi_DelayUsec(MRFI_VREG_SETTLE_TIME_USECS);

  /* release from reset */
  MRFI_DRIVE_RESETN_PIN_HIGH();

  /* wait for the radio crystal oscillator to stabilize */
  MRFI_SPI_SET_CHIP_SELECT_ON();
  while (!MRFI_SPI_SO_IS_HIGH());
  MRFI_SPI_SET_CHIP_SELECT_OFF();
}


/**************************************************************************************************
 * @fn          Mrfi_TurnOffRadioPower
 *
 * @brief       Power OFF the radio chip.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_TurnOffRadioPower(void)
{
  /* put chip into reset and then turn off voltage regulator */
  MRFI_DRIVE_RESETN_PIN_LOW();
  MRFI_DRIVE_VREG_EN_PIN_LOW();
}


/**************************************************************************************************
 * @fn          Mrfi_RxModeOff
 *
 * @brief       Disable frame receiving.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_RxModeOff(void)
{
  /* NOTE: Bug (#1) described in the errata swrz024.pdf for CC2520:
   * The sequence of sending the RFOFF strobe takes care of the bug.
   * If this is changed, ensure that the bug workaround is in place.
   */

  /*disable receive interrupts */
  MRFI_DISABLE_RX_INTERRUPT();

  /* turn off radio */
  mrfiSpiCmdStrobe(SRFOFF);

  /* flush the receive FIFO of any residual data */
  MRFI_RADIO_FLUSH_RX_BUFFER();

  /* clear receive interrupt */
  MRFI_CLEAR_RX_INTERRUPT_FLAG();
}


/**************************************************************************************************
 * @fn          Mrfi_RxModeOn
 *
 * @brief       Enable frame receiving.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_RxModeOn(void)
{
  /* NOTE: Bug #1 described in the errata swrz024.pdf for CC2520:
   * This function is never called if the radio is already in receive state.
   * If this is changed, must implement the bug workaround as described in the
   * errata (flush the Rx FIFO).
   */

  /* clear any residual receive interrupt */
  MRFI_CLEAR_RX_INTERRUPT_FLAG();

  /* send strobe to enter receive mode */
  mrfiSpiCmdStrobe(SRXON);

  /* enable receive interrupts */
  MRFI_ENABLE_RX_INTERRUPT();
}


/**************************************************************************************************
 * @fn          Mrfi_FiFoPIsr
 *
 * @brief       Interrupt Service Routine for handling FIFO_P event.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void Mrfi_FiFoPIsr(void)
{
  uint8_t numBytes;
  uint8_t i;

  /* NOTE: Bug #2 described in the errata swrz024.pdf for CC2520:
   * There is a possiblity of small glitch in the fifo_p signal
   * (2 cycle of 32 MHz). Workaround is to make sure that fifo_p signal stays
   * high for longer than that. Else, it is a false alarm.
   */
  if(!MRFI_FIFOP_STATUS()) return;
  if(!MRFI_FIFOP_STATUS()) return;

  /* Ah... it is for real... Continue processing. */

  /* We should receive this interrupt only in RX state
   * Should never receive it if RX was turned ON only for
   * some internal mrfi processing like - during CCA.
   * Otherwise something is terribly wrong.
   */
  MRFI_ASSERT( mrfiRadioState == MRFI_RADIO_STATE_RX );

  do
  {
    /*
     * Pend on frame rx completion. First time through this always passes.
     * Later, though, it is possible that the Rx FIFO has bytes but we
     * havn't received a complete frame yet.
     */
    while( !MRFI_FIFOP_STATUS() );

    /* Check for Rx overflow. Checking here means we may flush a valid frame */
    if( MRFI_FIFOP_STATUS() && !MRFI_FIFO_STATUS() )
    {
      /* Flush receive FIFO to recover from overflow */
      MRFI_RADIO_FLUSH_RX_BUFFER();
      break;
    }

    /* clear interrupt flag so we can detect another frame later. */
    MRFI_CLEAR_RX_INTERRUPT_FLAG();

    /*
     *  Determine number of bytes to be read from receive FIFO.  The first byte
     *  has the number of bytes in the packet.  A mask must be applied though
     *  to strip off unused bits.  The number of bytes in the packet does not
     *  include the length byte itself but does include the FCS (generically known
     *  as RX metrics).
     */
    mrfiSpiReadRxFifo(&numBytes, 1);
    numBytes &= IEEE_PHY_PACKET_SIZE_MASK;

    /* see if frame will fit in maximum available buffer or is too small */
    if (((numBytes + MRFI_LENGTH_FIELD_SIZE - MRFI_RX_METRICS_SIZE) > MRFI_MAX_FRAME_SIZE) ||
         (numBytes < MRFI_MIN_SMPL_FRAME_SIZE))
    {
      uint8_t dummy;
      /* packet is too big or too small. remove it from FIFO */
      for (i=0; i<numBytes; i++)
      {
        /* read and discard bytes from FIFO */
        mrfiSpiReadRxFifo(&dummy, 1);
      }
    }
    else
    {
      uint8_t *p, nextByte;

      /* Clear out my buffer to remove leftovers in case a bogus packet gets through */
      for(i=0; i < MRFI_MAX_FRAME_SIZE; i++)
      {
        mrfiIncomingPacket.frame[i] = 0;
      }

      /* set pointer at first byte of frame storage */
      p  = &mrfiIncomingPacket.frame[MRFI_LENGTH_FIELD_OFFSET];

      /*
       *  Store frame length into the incoming packet memory.  Size of rx metrics
       *  is subtracted to get the MRFI frame length which separates rx metrics from
       *  the frame length.
       */
      *p = numBytes - MRFI_RX_METRICS_SIZE;

      /* read frame bytes from rx FIFO and store into incoming packet memory */
      p++;
      mrfiSpiReadRxFifo(p, numBytes-MRFI_RX_METRICS_SIZE);

      /* The next two bytes in the rx fifo are:
       * - RSSI of the received frams
       * - CRC OK bit and the 7 bit wide correlation value.
       * Read this rx metrics and store to incoming packet.
       */

      /* Add the RSSI offset to get the proper RSSI value. */
      mrfiSpiReadRxFifo(&nextByte, 1);
      mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_RSSI_OFS] = Mrfi_CalculateRssi(nextByte);

      /* The second byte has 7 bits of Correlation value and 1 bit of
       * CRC pass/fail info. Remove the CRC pass/fail bit info.
       * Also note that for CC2520 radio this is the correlation value and not
       * the LQI value. Some convertion is needed to extract the LQI value.
       * This convertion is left to the application at this time.
       */
      mrfiSpiReadRxFifo(&nextByte, 1);
      mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_CRC_LQI_OFS] = nextByte & MRFI_RX_METRICS_LQI_MASK;

      /* Eliminate frames that are the correct size but we can tell are bogus
       * by their frame control fields OR if CRC failed.
       */
      if( (nextByte & MRFI_RX_METRICS_CRC_OK_MASK) &&
          (mrfiIncomingPacket.frame[MRFI_FCF_OFFSET] == MRFI_FCF_0_7) &&
          (mrfiIncomingPacket.frame[MRFI_FCF_OFFSET+1] == MRFI_FCF_8_15))
      {
        /* call external, higher level "receive complete" (CRC checking is done by hardware) */
        MRFI_RxCompleteISR();
      }
    }

    /* If the client code takes long time to process the frame,
     * rx fifo could overflow during this time. As soon as this condition is
     * reached, the radio fsm stops all activities till the rx fifo is flushed.
     * It also puts the fifo signal low. When we come out of this while loop,
     * we really don't know if it is because of overflow condition or
     * there is no data in the fifo. So we must check for overflow condition
     * before exiting the ISR otherwise it could get stuck in this "overflow"
     * state forever.
     */

  } while( MRFI_FIFO_STATUS() ); /* Continue as long as there is some data in FIFO */

  /* Check if we exited the loop due to fifo overflow.
   * and not due to no data in fifo.
   */
  if( MRFI_FIFOP_STATUS() && !MRFI_FIFO_STATUS() )
  {
    /* Flush receive FIFO to recover from overflow */
    MRFI_RADIO_FLUSH_RX_BUFFER();
  }
}


/**************************************************************************************************
 * @fn          Mrfi_CalculateRssi
 *
 * @brief       Does binary to decimal convertion and offset compensation.
 *
 * @param       raw rssi value
 *
 * @return      RSSI value in units of dBm.
 **************************************************************************************************
 */
int8_t Mrfi_CalculateRssi(uint8_t rawValue)
{
  int16_t rssi;

  /* The raw value is in 2's complement and in 1 dB steps. Convert it to
   * decimal taking into account the offset value.
   */
  if(rawValue >= 128)
  {
    rssi = (int16_t)(rawValue - 256) - MRFI_RSSI_OFFSET;
  }
  else
  {
    rssi = rawValue - MRFI_RSSI_OFFSET;
  }

  /* Restrict this value to least value can be held in an 8 bit signed int */
  if(rssi < -128)
  {
    rssi = -128;
  }

  return rssi;
}


/**************************************************************************************************
 * @fn          Mrfi_RandomBackoffDelay
 *
 * @brief       Waits for random amount of time before returning.
 *              The range is: 1*250 to 16*250 us.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_RandomBackoffDelay(void)
{
  uint8_t backoffs;
  uint8_t i;

  /* calculate random value for backoffs - 1 to 16 */
  backoffs = (MRFI_RandomByte() & 0x0F) + 1;

  /* delay for randomly computed number of backoff periods */
  for (i=0; i<backoffs; i++)
  {
    Mrfi_DelayUsec( MRFI_BACKOFF_PERIOD_USECS );
  }
}


/****************************************************************************************************
 * @fn          Mrfi_DelayUsec
 *
 * @brief       Execute a delay loop using the MAC timer. The macro actually used to do the delay
 *              is not reentrant. This routine makes the delay execution reentrant by breaking up
 *              the requested delay up into small chunks and executing each chunk as a critical
 *              section. The chunk size is choosen to be the smallest value used by MRFI. The delay
 *              is only approximate because of the overhead computations. It errs on the side of
 *              being too long.
 *
 * input parameters
 * @param   howLong - number of microseconds to delay
 *
 * @return      none
 ****************************************************************************************************
 */
static void Mrfi_DelayUsec(uint16_t howLong)
{
  bspIState_t s;
  uint16_t count = howLong/MRFI_MAX_DELAY_US;

  if (howLong)
  {
    do
    {
      BSP_ENTER_CRITICAL_SECTION(s);
      BSP_DELAY_USECS(MRFI_MAX_DELAY_US);
      BSP_EXIT_CRITICAL_SECTION(s);
    } while (count--);
  }

  return;
}

/****************************************************************************************************
 * @fn          Mrfi_DelayUsecSem
 *
 * @brief       Execute a delay loop using a HW timer. See comments for Mrfi_DelayUsec().
 *              Delay specified number of microseconds checking semaphore for
 *              early-out. Run in a separate thread when the reply delay is
 *              invoked. Cleaner then trying to make MRFI_DelayUsec() thread-safe
 *              and reentrant.
 *
 * input parameters
 * @param   howLong - number of microseconds to delay
 *
 * @return      none
 ****************************************************************************************************
 */
static void Mrfi_DelayUsecSem(uint16_t howLong)
{
  bspIState_t s;
  uint16_t count = howLong/MRFI_MAX_DELAY_US;

  if (howLong)
  {
    do
    {
      BSP_ENTER_CRITICAL_SECTION(s);
      BSP_DELAY_USECS(MRFI_MAX_DELAY_US);
      BSP_EXIT_CRITICAL_SECTION(s);
      if (sKillSem)
      {
        break;
      }
    } while (count--);
  }

  return;
}


/**************************************************************************************************
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */


#define MRFI_RADIO_TX_FIFO_SIZE     128  /* from datasheet */

/* verify largest possible packet fits within FIFO buffer */
#if ((MRFI_MAX_FRAME_SIZE + MRFI_RX_METRICS_SIZE) > MRFI_RADIO_TX_FIFO_SIZE)
#error "ERROR:  Maximum possible packet length exceeds FIFO buffer.  Decrease value of maximum application payload."
#endif

/**************************************************************************************************
*/
