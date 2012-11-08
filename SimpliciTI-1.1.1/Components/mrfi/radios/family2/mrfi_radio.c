/**************************************************************************************************
  Revised:        $Date: 2007-07-06 11:19:00 -0700 (Fri, 06 Jul 2007) $
  Revision:       $Revision: 13579 $

  Copyright 2007-2008 Texas Instruments Incorporated.  All rights reserved.

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
 *   Radios: CC2510, CC2511, CC1110, CC1111
 *   Primary code file for supported radios.
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include <string.h>
#include "mrfi.h"
#include "bsp.h"
#include "bsp_macros.h"
#include "mrfi_defs.h"
#include "../common/mrfi_f1f2.h"
#include "bsp_external/mrfi_board_defs.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */
#if (defined MRFI_CC2510) || (defined MRFI_CC2511)

  #define MRFI_RSSI_OFFSET     71   /* for 250 kbsp. no units */

 /* Worst case wait period in RX state before RSSI becomes valid.
  * These numbers are from Design Note DN505 with added safety margin.
  */
  #define MRFI_RSSI_VALID_DELAY_US    1000

#elif (defined MRFI_CC1110) || (defined MRFI_CC1111)

  #define MRFI_RSSI_OFFSET     73   /* for 433MHz @ 250 kbsp. no units */

  /* Worst case wait period in RX state before RSSI becomes valid.
   * These numbers are from Design Note DN505 with added safety margin.
   */
  #define MRFI_RSSI_VALID_DELAY_US    1300

#else
  #error "ERROR: RSSI Offset value not defined for this radio.
#endif


#define MRFI_LENGTH_FIELD_OFS               __mrfi_LENGTH_FIELD_OFS__
#define MRFI_LENGTH_FIELD_SIZE              __mrfi_LENGTH_FIELD_SIZE__
#define MRFI_HEADER_SIZE                    __mrfi_HEADER_SIZE__
#define MRFI_FRAME_BODY_OFS                 __mrfi_DST_ADDR_OFS__
#define MRFI_BACKOFF_PERIOD_USECS           __mrfi_BACKOFF_PERIOD_USECS__

#define MRFI_MIN_SMPL_FRAME_SIZE            (MRFI_HEADER_SIZE + NWK_HDR_SIZE)

/* Max time we can be in a critical section within the delay function.
 * This could be fine-tuned by observing the overhead is calling the bsp delay
 * function. The overhead should be very small compared to this value.
 * Note that the max value for this must be less than 19 usec with the
 * default CLKCON.TICKSPD and CLKCON.CLOCKSPD settings and external 26 MHz
 * crystal as a clock source (which we use). The CCxx11 USB uses a 48 MHz
 * crystal to support USB divided down to 24 MHz for radio. This gives a
 * maximum of 21 usec per chunk for these radios.
 *
 * Be careful of direct calls to Mrfi_DelayUsec().
 */
#define MRFI_MAX_DELAY_US 16 /* usec */


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *    Radio Definitions
 *   - - - - - - - - - -
 */

#if (defined MRFI_CC1110) || (defined MRFI_CC1111)
#define MRFI_SETTING_PA_TABLE0    0x8E
#define MRFI_RADIO_PARTNUM        0x01
#define MRFI_RADIO_MIN_VERSION    3

#elif (defined MRFI_CC2510) || (defined MRFI_CC2511)
#define MRFI_SETTING_PA_TABLE0    0xFE
#define MRFI_RADIO_PARTNUM        0x81
#define MRFI_RADIO_MIN_VERSION    4

#else
#error "ERROR: Unspecified or unsupported radio."
#endif

/* bit of PARTNUM register that signifies chip has USB capability */
#define MRFI_RADIO_PARTNUM_USB_BIT          0x10

/* rx metrics definitions, known as appended "packet status bytes" in datasheet parlance */
#define MRFI_RX_METRICS_CRC_OK_MASK         __mrfi_RX_METRICS_CRC_OK_MASK__
#define MRFI_RX_METRICS_LQI_MASK            __mrfi_RX_METRICS_LQI_MASK__

/* register RFST - command strobes */
#define SFSTXON       0x00
#define SCAL          0x01
#define SRX           0x02
#define STX           0x03
#define SIDLE         0x04

/* register MARCSTATE - state values */
#define RXTX_SWITCH   0x15
#define RX            0x0D
#define IDLE          0x01

/* register IEN2 - bit definitions */
#define RFIE          BV(0)

/* register S1CON - bit definitions */
#define RFIF_1        BV(1)
#define RFIF_0        BV(0)

/* register DMAARM - bit definitions */
#define ABORT         BV(7)

/* register CLKCON - bit definitions */
#define OSC           BV(6)

/* register SLEEP - bit definitions */
#define XOSC_STB      BV(6)
#define OSC_PD        BV(2)

/* register RFIF - bit definitions */
#define IRQ_DONE      BV(4)
#define IRQ_RXOVFL    BV(6)

/* register RFIM - bit definitions */
#define IM_DONE       BV(4)

/* register PKTSTATUS - bit definitions */
#define MRFI_PKTSTATUS_CCA   BV(4)
#define MRFI_PKTSTATUS_CS    BV(6)

/* random number generator */
#define RCTRL_CLOCK_LFSR    BV(2)

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *    Radio Register Settings
 *   - - - - - - - - - - - - -
 */

/* Main Radio Control State Machine control configuration - Calibrate when going from IDLE to RX or TX. */
#define MRFI_SETTING_MCSM0      0x14

/* Main Radio Control State Machine control configuration - Go to RX state after RX
 * and go to IDLE after TX.
 */
#define MRFI_SETTING_MCSM1      0x3C

/*
 *  Packet Length - Setting for maximum allowed packet length.
 *  The PKTLEN setting does not include the length field but maximum frame size does.
 *  Subtract length field size from maximum frame size to get value for PKTLEN.
 */
#define MRFI_SETTING_PKTLEN     (MRFI_MAX_FRAME_SIZE - MRFI_LENGTH_FIELD_SIZE)

/* Packet automation control - Original value except WHITE_DATA is extracted from SmartRF setting. */
#define MRFI_SETTING_PKTCTRL0   (0x05 | (SMARTRF_SETTING_PKTCTRL0 & BV(6)))

/* Packet automation control - base value is power up value whick has APPEND_STATUS enabled */
#define MRFI_SETTING_PKTCTRL1_BASE              BV(2)
#define MRFI_SETTING_PKTCTRL1_ADDR_FILTER_OFF   MRFI_SETTING_PKTCTRL1_BASE
#define MRFI_SETTING_PKTCTRL1_ADDR_FILTER_ON    (MRFI_SETTING_PKTCTRL1_BASE | (BV(1)|BV(0)))

/* early versions of SmartRF Studio did not export this value */
#ifndef SMARTRF_SETTING_FSCAL1
#define SMARTRF_SETTING_FSCAL1  0x80
#endif

/* TEST0 Various Test Settings - the VCO_SEL_CAL_EN bit must be zero */
#define MRFI_SETTING_TEST0      (SMARTRF_SETTING_TEST0 & ~(BV(1)))

/* if SmartRF setting for PA_TABLE0 is supplied, use that instead of built-in default */
#ifdef SMARTRF_SETTING_PA_TABLE0
#undef MRFI_SETTING_PA_TABLE0
#define MRFI_SETTING_PA_TABLE0   SMARTRF_SETTING_PA_TABLE0
#endif


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *    DMA Configuration Values
 *   - - - - - - - - - - - - - -
 */

/* DMA channel number */
#define MRFI_DMA_CHAN               0

/* DMA configuration data structure size */
#define RXTX_DMA_STRUCT_SIZE        8

/* byte offset 4 (upper bits of LEN are never used so are always set to zero) */
#define RXTX_DMA_VLEN_XFER_BYTES_PLUS_1       ((/*  VLEN = */( 1 ))<<5)
#define RXTX_DMA_VLEN_XFER_BYTES_PLUS_3       ((/*  VLEN = */( 4 ))<<5)

/* byte offset 5 */
#define RXTX_DMA_LEN            (MRFI_MAX_FRAME_SIZE + MRFI_RX_METRICS_SIZE)

/* byte offset 6 */
#define RXTX_DMA_WORDSIZE       (/*  WORDSIZE = */(  0 )  << 7)
#define RXTX_DMA_TMODE          (/*     TMODE = */(  0 )  << 5)
#define RXTX_DMA_TRIG           (/*      TRIG = */( 19 )  << 0)

/* byte offset 7 */
#define RXTX_DMA_SRCINC_PLUS_1  (/*    SRCINC = */( 1 )  << 6)
#define RXTX_DMA_SRCINC_NONE    (/*    SRCINC = */( 0 )  << 6)
#define RXTX_DMA_DESTINC_PLUS_1 (/*   DESTINC = */( 1 )  << 4)
#define RXTX_DMA_DESTINC_NONE   (/*   DESTINC = */( 0 )  << 4)
#define RXTX_DMA_IRQMASK        (/*   IRQMASK = */( 0 )  << 3)
#define RXTX_DMA_M8             (/*        M8 = */( 0 )  << 2)
#define RXTX_DMA_PRIORITY       (/*  PRIORITY = */( 1 )  << 0)

  /* The SW timer is calibrated by adjusting the call to the microsecond delay
   * routine. This allows maximum calibration control with repects to the longer
   * times requested by applicationsd and decouples internal from external calls
   * to the microsecond routine which can be calibrated independently.
   */
#if defined(SW_TIMER)
#define APP_USEC_VALUE    100
#else
#define APP_USEC_VALUE    500
#endif

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */
#define HIGH_BYTE_OF_WORD(x)            ((uint8_t) (((uint16_t)(x)) >> 8))
#define LOW_BYTE_OF_WORD(x)             ((uint8_t) (((uint16_t)(x)) & 0xFF))



/* There is no bit in h/w to tell if RSSI in the register is valid or not.
 * The hardware needs to be in RX state for a certain amount of time before
 * a valid RSSI value is calculated and placed in the register. This min
 * wait time is defined by MRFI_BOARD_RSSI_VALID_DELAY_US. We don't need to
 * add such delay every time RSSI value is needed. If the Carier Sense signal
 * is high or CCA signal is high, we know that the RSSI value must be valid.
 * We use that knowledge to reduce our wait time. We break down the delay loop
 * in multiple chunks and during each iteration, check for the CS and CCA
 * signal. If either of these signals is high, we return immediately. Else,
 * we wait for the max delay specified.
 */
#define MRFI_RSSI_VALID_WAIT()                                                \
{                                                                             \
  int16_t delay = MRFI_RSSI_VALID_DELAY_US;                                   \
  do                                                                          \
  {                                                                           \
    if(PKTSTATUS & (MRFI_PKTSTATUS_CCA | MRFI_PKTSTATUS_CS))  \
    {                                                                         \
      break;                                                                  \
    }                                                                         \
    Mrfi_DelayUsec(64); /* sleep */                                           \
    delay -= 64;                                                              \
  }while(delay > 0);                                                          \
}                                                                             \

#define MRFI_STROBE_IDLE_AND_WAIT()  \
{                                    \
  RFST = SIDLE;                      \
  while (MARCSTATE != IDLE) ;        \
}

/* ------------------------------------------------------------------------------------------------
 *                                       Local Prototypes
 * ------------------------------------------------------------------------------------------------
 */
static void   Mrfi_RxModeOn(void);
static void   Mrfi_RxModeOff(void);
static void   Mrfi_DelayUsec(uint16_t);
static void   Mrfi_RandomBackoffDelay(void);
static int8_t Mrfi_CalculateRssi(uint8_t);
static void   Mrfi_DelayUsecSem(uint16_t);

/* ------------------------------------------------------------------------------------------------
 *                                       Local Variables
 * ------------------------------------------------------------------------------------------------
 */
static uint8_t mrfiRadioState  = MRFI_RADIO_STATE_UNKNOWN;
static mrfiPacket_t mrfiIncomingPacket;

/* reply delay support */
static volatile uint8_t  sKillSem = 0;
static volatile uint8_t  sReplyDelayContext = 0;
static          uint16_t sReplyDelayScalar = 0;
static          uint16_t sBackoffHelper = 0;

#if (MRFI_DMA_CHAN == 0)
uint8_t XDATA mrfiDmaCfg[RXTX_DMA_STRUCT_SIZE];
#define MRFI_DMA_CFG_ADDRESS   &mrfiDmaCfg[0]
#endif


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
  /* ------------------------------------------------------------------
   *    Run-time integrity checks
   *   ---------------------------
   */

  memset(&mrfiIncomingPacket, 0x0, sizeof(mrfiIncomingPacket));
  /* verify the correct radio is installed */
  MRFI_ASSERT( (PARTNUM & ~MRFI_RADIO_PARTNUM_USB_BIT) == MRFI_RADIO_PARTNUM );
  MRFI_ASSERT( VERSION >= MRFI_RADIO_MIN_VERSION );  /* obsolete radio version */


  /* ------------------------------------------------------------------
   *    Switch to high speed crystal oscillator.
   *   ------------------------------------------
   */

  /* power up both oscillators - high speed crystal oscillator will power up,
   * the RC oscillator will remain powered-up and selected.
   */
  SLEEP &= ~OSC_PD;

  /* wait for high speed crystal to become stable */
  while(!(SLEEP & XOSC_STB));

  /* switch from RC oscillator to high speed crystal oscillator */
  CLKCON &= ~OSC;

  /* power down the oscillator not selected, i.e. the RC oscillator */
  SLEEP |= OSC_PD;

  /* ------------------------------------------------------------------
   *    Variable Initialization
   *   -------------------------
   */


  /* ------------------------------------------------------------------
   *    DMA Initialization
   *   --------------------
   */
#if (MRFI_DMA_CHAN == 0)
  DMA0CFGH = HIGH_BYTE_OF_WORD( &mrfiDmaCfg[0] );
  DMA0CFGL = LOW_BYTE_OF_WORD ( &mrfiDmaCfg[0] );
#endif


  /* ------------------------------------------------------------------
   *    Configure radio
   *   -----------------
   */

  /* internal radio register configuration */
  MCSM1     = MRFI_SETTING_MCSM1;
  MCSM0     = MRFI_SETTING_MCSM0;
  PKTLEN    = MRFI_SETTING_PKTLEN;
  PKTCTRL0  = MRFI_SETTING_PKTCTRL0;
  PA_TABLE0 = MRFI_SETTING_PA_TABLE0;
  TEST0     = MRFI_SETTING_TEST0;

  /* imported SmartRF radio register configuration */
  FSCTRL1  = SMARTRF_SETTING_FSCTRL1;
  FSCTRL0  = SMARTRF_SETTING_FSCTRL0;
  FREQ2    = SMARTRF_SETTING_FREQ2;
  FREQ1    = SMARTRF_SETTING_FREQ1;
  FREQ0    = SMARTRF_SETTING_FREQ0;
  MDMCFG4  = SMARTRF_SETTING_MDMCFG4;
  MDMCFG3  = SMARTRF_SETTING_MDMCFG3;
  MDMCFG2  = SMARTRF_SETTING_MDMCFG2;
  MDMCFG1  = SMARTRF_SETTING_MDMCFG1;
  MDMCFG0  = SMARTRF_SETTING_MDMCFG0;
  DEVIATN  = SMARTRF_SETTING_DEVIATN;
  FOCCFG   = SMARTRF_SETTING_FOCCFG;
  BSCFG    = SMARTRF_SETTING_BSCFG;
  AGCCTRL2 = SMARTRF_SETTING_AGCCTRL2;
  AGCCTRL1 = SMARTRF_SETTING_AGCCTRL1;
  AGCCTRL0 = SMARTRF_SETTING_AGCCTRL0;
  FREND1   = SMARTRF_SETTING_FREND1;
  FREND0   = SMARTRF_SETTING_FREND0;
  FSCAL3   = SMARTRF_SETTING_FSCAL3;
  FSCAL2   = SMARTRF_SETTING_FSCAL2;
  FSCAL1   = SMARTRF_SETTING_FSCAL1;
  FSCAL0   = SMARTRF_SETTING_FSCAL0;
  TEST2    = SMARTRF_SETTING_TEST2;
  TEST1    = SMARTRF_SETTING_TEST1;


  /* Initial radio state is IDLE state */
  mrfiRadioState = MRFI_RADIO_STATE_IDLE;

  /* set default channel */
  MRFI_SetLogicalChannel( 0 );

  /* Seed for the Random Number Generator */
  {
    uint16_t rndSeed = 0x0;

    /* Put radio in RX mode */
    RFST = SRX;

    /* Delay for valid RSSI. Otherwise same RSSI
     * value could be read every time.
     */
    MRFI_RSSI_VALID_WAIT();

    {
      uint8_t i;
      for(i=0; i<16; i++)
      {
        /* use most random bit of RSSI to populate the random seed */
        rndSeed = (rndSeed << 1) | (RSSI & 0x01);
      }
    }

    /* Force the seed to be non-zero by setting one bit, just in case... */
    rndSeed |= 0x0080;

    /* Need to write to RNDL twice to seed it */
    RNDL = rndSeed & 0xFF;
    RNDL = rndSeed >> 8;

    /* Call RxModeOff() instead of an idle strobe.
     * This will clean up any flags that were set while radio was in rx state.
     */
    Mrfi_RxModeOff();
  }


  /*****************************************************************************************
   *                            Compute reply delay scalar
   *
   * Formula from data sheet for all the narrow band radios is:
   *
   *                (256 + DATAR_Mantissa) * 2^(DATAR_Exponent)
   * DATA_RATE =    ------------------------------------------ * f(xosc)
   *                                    2^28
   *
   * To try and keep some accuracy we change the exponent of the denominator
   * to (28 - (exponent from the configuration register)) so we do a division
   * by a smaller number. We find the power of 2 by shifting.
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
   * Note: We assume a 26 MHz oscillator frequency for the non-USB target radios
   *       and 24 MHz for the USB-target radios.
   * ***************************************************************************************
   */
#if defined(MRFI_CC2510)  ||  defined(MRFI_CC1110)
#define   MRFI_RADIO_OSC_FREQ         26000000
#elif defined(MRFI_CC2511)  ||  defined(MRFI_CC1111)
#define   MRFI_RADIO_OSC_FREQ         24000000
#endif

#define   PHY_PREAMBLE_SYNC_BYTES    8

  {
    uint32_t dataRate, bits;
    uint16_t exponent, mantissa;

    /* mantissa is in MDMCFG3 */
    mantissa = 256 + SMARTRF_SETTING_MDMCFG3;

    /* exponent is lower nibble of MDMCFG4. */
    exponent = 28 - (SMARTRF_SETTING_MDMCFG4 & 0x0F);

    /* we can now get data rate */
    dataRate = mantissa * (MRFI_RADIO_OSC_FREQ>>exponent);

    bits = ((uint32_t)((PHY_PREAMBLE_SYNC_BYTES + MRFI_MAX_FRAME_SIZE)*8))*10000;

    /* processing on the peer + the Tx/Rx time plus more */
    sReplyDelayScalar = PLATFORM_FACTOR_CONSTANT + (((bits/dataRate)+5)/10);

    /* This helper value is used to scale the backoffs during CCA. At very
     * low data rates we need to backoff longer to prevent continual sampling
     * of valid frames which take longer to send at lower rates. Use the scalar
     * we just calculated divided by 32. With the backoff algorithm backing
     * off up to 16 periods this will result in waiting up to about 1/2 the total
     * scalar value. For high data rates this does not contribute at all. Value
     * is in microseconds.
     */
    sBackoffHelper = MRFI_BACKOFF_PERIOD_USECS + (sReplyDelayScalar>>5)*1000;
  }

  /* ------------------------------------------------------------------
   *    Configure interrupts
   *   ----------------------
   */

  /* enable general RF interrupts */
  IEN2 |= RFIE;

  /* enable global interrupts */
  BSP_ENABLE_INTERRUPTS();
}


/**************************************************************************************************
 * @fn          MRFI_Transmit
 *
 * @brief       Transmit a packet using CCA algorithm.
 *
 * @param       pPacket - pointer to packet to transmit
 *
 * @return      Return code indicates success or failure of transmit:
 *                  MRFI_TX_RESULT_SUCCESS - transmit succeeded
 *                  MRFI_TX_RESULT_FAILED  - transmit failed because CCA failed
 **************************************************************************************************
 */
uint8_t MRFI_Transmit(mrfiPacket_t * pPacket, uint8_t txType)
{
  uint8_t ccaRetries;
  uint8_t returnValue = MRFI_TX_RESULT_SUCCESS;

  /* radio must be awake to transmit */
  MRFI_ASSERT( mrfiRadioState != MRFI_RADIO_STATE_OFF );

  /* Turn off reciever. We can ignore/drop incoming packets during transmit. */
  Mrfi_RxModeOff();

  /* configure DMA channel for transmit */
  {
    uint8_t XDATA * pCfg;

    pCfg = MRFI_DMA_CFG_ADDRESS;
    *pCfg++ = /* offset 0 : */  HIGH_BYTE_OF_WORD( &(pPacket->frame[0]) );  /* SRCADDRH */
    *pCfg++ = /* offset 1 : */  LOW_BYTE_OF_WORD ( &(pPacket->frame[0]) );  /* SRCADDRL */
    *pCfg++ = /* offset 2 : */  HIGH_BYTE_OF_WORD( &X_RFD );  /* DSTADDRH */
    *pCfg++ = /* offset 3 : */  LOW_BYTE_OF_WORD ( &X_RFD );  /* DSTADDRL */
    *pCfg++ = /* offset 4 : */  RXTX_DMA_VLEN_XFER_BYTES_PLUS_1;
    *pCfg++ = /* offset 5 : */  RXTX_DMA_LEN;
    *pCfg++ = /* offset 6 : */  RXTX_DMA_WORDSIZE | RXTX_DMA_TMODE | RXTX_DMA_TRIG;
    *pCfg   = /* offset 7 : */  RXTX_DMA_SRCINC_PLUS_1 | RXTX_DMA_DESTINC_NONE |
                                RXTX_DMA_IRQMASK | RXTX_DMA_M8 | RXTX_DMA_PRIORITY;
  }

  /* ------------------------------------------------------------------
   *    Immediate transmit
   *   ---------------------
   */
  if (txType == MRFI_TX_TYPE_FORCED)
  {
    /* ARM the DMA channel */
    DMAARM |= BV( MRFI_DMA_CHAN );

    /* Strobe TX */
    RFST = STX;

    /* wait for transmit to complete */
    while (!(RFIF & IRQ_DONE));

    /* Clear the interrupt flag */
    RFIF &= ~IRQ_DONE;
  }
  else
  {
    /* ------------------------------------------------------------------
     *    CCA transmit
     *   ---------------
     */

    MRFI_ASSERT( txType == MRFI_TX_TYPE_CCA );

    /* set number of CCA retries */
    ccaRetries = MRFI_CCA_RETRIES;

    /* ===============================================================================
     *    CCA Loop
     *  =============
     */
    while(1)
    {
      /* ARM the DMA channel */
      DMAARM |= BV( MRFI_DMA_CHAN );

      /* strobe to enter receive mode */
      RFST = SRX;

      /* Wait for radio to enter the RX mode */
      while(MARCSTATE != RX);

      /* wait for the rssi to be valid. */
      MRFI_RSSI_VALID_WAIT();

      /* Strobe TX-if-CCA */
      RFST = STX;

      if(MARCSTATE != RX) /* ck if exited rx state */
      {
        /* ------------------------------------------------------------------
         *    Clear Channel Assessment passed.
         * ----------------------------------
         */

        /* wait for transmit to complete */
        while (!(RFIF & IRQ_DONE));

        /* Clear the interrupt flag */
        RFIF &= ~IRQ_DONE;

        break;
      }
      else
      {
        /* ------------------------------------------------------------------
         *    Clear Channel Assessment failed.
         * ----------------------------------
         */

        /* Retry ? */
        if (ccaRetries != 0)
        {
          /* turn off reciever to conserve power during backoff */
          Mrfi_RxModeOff();

          /* delay for a random number of backoffs */
          Mrfi_RandomBackoffDelay();

          /* decrement CCA retries before loop continues */
          ccaRetries--;
        }
        else  /* No CCA retries are left, abort */
        {
          /* set return value for failed transmit and break */
          returnValue = MRFI_TX_RESULT_FAILED;
          break;
        }
      } /* CCA Failed */
    } /* CCA loop */
  }/* txType is CCA */


  /* Done with TX. Clean up time... */

  /* turn radio back off to put it in a known state */
  Mrfi_RxModeOff();

  /* If the radio was in RX state when transmit was attempted,
   * put it back in RX state.
   */
  if(mrfiRadioState == MRFI_RADIO_STATE_RX)
  {
    Mrfi_RxModeOn();
  }

  return( returnValue );
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
  *pPacket = mrfiIncomingPacket;
}


/**************************************************************************************************
 * @fn          MRFI_RfIsr
 *
 * @brief       -
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
BSP_ISR_FUNCTION( MRFI_RfIsr, RF_VECTOR )
{
  uint8_t frameLen;

  /* We should receive this interrupt only in RX state
   * Should never receive it if RX was turned On only for
   * some internal mrfi processing like - during CCA.
   * Otherwise something is terribly wrong.
   */
  MRFI_ASSERT( mrfiRadioState == MRFI_RADIO_STATE_RX );

  /* Check for overflow */
  if ((RFIF & IRQ_DONE) && (RFIF & IRQ_RXOVFL))
  {
    RFIF &= ~IRQ_DONE;
    RFIF &= ~IRQ_RXOVFL;
    S1CON &= ~(RFIF_1 | RFIF_0); /* Clear MCU interrupt flag */

    /* Only way out of this is to go to IDLE state */
    Mrfi_RxModeOff();

    /* zero-out MRFI buffer to help NWK eliminate undetected rogue frames if they pass here */
    memset(mrfiIncomingPacket.frame, 0x00, sizeof(mrfiIncomingPacket.frame));

    /* OK to start again... */
    Mrfi_RxModeOn();

    return;
  }

  RFIF &= ~IRQ_DONE;           /* Clear the interrupt at the source */
  S1CON &= ~(RFIF_1 | RFIF_0); /* Clear MCU interrupt flag */

  /* ------------------------------------------------------------------
   *    Copy RX Metrics into packet structure
   *   ---------------------------------------
   */
  {
    uint8_t offsetToRxMetrics = mrfiIncomingPacket.frame[MRFI_LENGTH_FIELD_OFS] + 1;
    /* The metrics were DMA'd so they may reside on the frame buffer rather than the
     * metrics buffer. Get them to the proper location.
     */
    memmove(mrfiIncomingPacket.rxMetrics,&mrfiIncomingPacket.frame[offsetToRxMetrics], sizeof(mrfiIncomingPacket.rxMetrics));
  }


  /* ------------------------------------------------------------------
   *    CRC  and frame length check
   *   ------------
   */

  frameLen = mrfiIncomingPacket.frame[MRFI_LENGTH_FIELD_OFS];
  /* determine if CRC or length check failed */
  if (!(mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_CRC_LQI_OFS] & MRFI_RX_METRICS_CRC_OK_MASK) ||
       ((frameLen + MRFI_LENGTH_FIELD_SIZE) > MRFI_MAX_FRAME_SIZE) ||
       (frameLen < MRFI_MIN_SMPL_FRAME_SIZE)
     )
  {
    /* CRC or length check failed - do nothing, skip to end */
  }
  else
  {
    /* CRC passed - continue processing */

    /* ------------------------------------------------------------------
     *    Filtering
     *   -----------
     */

    /* if address is not filtered, receive is successful */
    if (!MRFI_RxAddrIsFiltered(MRFI_P_DST_ADDR(&mrfiIncomingPacket)))
    {
      {
        /* ------------------------------------------------------------------
         *    Receive successful
         *   --------------------
         */

        /* Convert the raw RSSI value and do offset compensation for this radio */
        mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_RSSI_OFS] =
            Mrfi_CalculateRssi(mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_RSSI_OFS]);

        /* Remove the CRC valid bit from the LQI byte */
        mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_CRC_LQI_OFS] =
          (mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_CRC_LQI_OFS] & MRFI_RX_METRICS_LQI_MASK);


        /* call external, higher level "receive complete" processing routine */
        MRFI_RxCompleteISR();
      }
   }
  }

  /* zero-out MRFI buffer to help NWK eliminate undetected rogue frames if they pass here */
  memset(mrfiIncomingPacket.frame, 0x00, sizeof(mrfiIncomingPacket.frame));

  /* arm DMA channel for next receive */
  DMAARM |= BV( MRFI_DMA_CHAN );
}


/**************************************************************************************************
 * @fn          MRFI_Sleep
 *
 * @brief       Request radio go to sleep.
 *
 * @param       none
 *
 * @return      zero : if successfully went to sleep
 *              non-zero : if sleep was not entered
 **************************************************************************************************
 */
void MRFI_Sleep(void)
{
  /* If radio is not asleep, put it to sleep */
  if(mrfiRadioState != MRFI_RADIO_STATE_OFF)
  {
    bspIState_t s;

    /* critical section necessary for watertight testing and setting of state variables */
    BSP_ENTER_CRITICAL_SECTION(s);

   /* go to idle so radio is in a known state before sleeping */
   MRFI_RxIdle();

   /* There is no individual power control to the RF block on this radio.
    * So putting it to Idle is the best we can do.
    */

    /* Our new state is OFF */
    mrfiRadioState = MRFI_RADIO_STATE_OFF;

   BSP_EXIT_CRITICAL_SECTION(s);
  }
}


/**************************************************************************************************
 * @fn          MRFI_WakeUp
 *
 * @brief       Wake up radio from sleep state.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_WakeUp(void)
{
  /* verify high speed crystal oscillator is selected, required for radio operation */
  MRFI_ASSERT( !(CLKCON & OSC) );

  /* if radio is already awake, just ignore wakeup request */
  if(mrfiRadioState != MRFI_RADIO_STATE_OFF)
  {
    return;
  }

  /* restore radio registers that are reset during sleep */
  FSCAL3 = SMARTRF_SETTING_FSCAL3;
  FSCAL2 = SMARTRF_SETTING_FSCAL2;
  FSCAL1 = SMARTRF_SETTING_FSCAL1;

  /* enter idle mode */
  mrfiRadioState = MRFI_RADIO_STATE_IDLE;
  MRFI_STROBE_IDLE_AND_WAIT();
}

/**************************************************************************************************
 * @fn          MRFI_RandomByte
 *
 * @brief       Returns a random byte
 *
 * @param       none
 *
 * @return      a random byte
 **************************************************************************************************
 */
uint8_t MRFI_RandomByte(void)
{
  /* clock the random generator once to get a new random value */
  ADCCON1 |= RCTRL_CLOCK_LFSR;

  return RNDL;
}



/**************************************************************************************************
 * @fn          Mrfi_RxModeOn
 *
 * @brief       Put radio into receive mode.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_RxModeOn(void)
{
  uint8_t XDATA * pCfg;

  /* configure DMA channel for receive */
  pCfg = MRFI_DMA_CFG_ADDRESS;
  *pCfg++ = /* offset 0 : */  HIGH_BYTE_OF_WORD( &X_RFD );  /* SRCADDRH */
  *pCfg++ = /* offset 1 : */  LOW_BYTE_OF_WORD ( &X_RFD );  /* SRCADDRL */
  *pCfg++ = /* offset 2 : */  HIGH_BYTE_OF_WORD( &(mrfiIncomingPacket.frame[0]) );  /* DSTADDRH */
  *pCfg++ = /* offset 3 : */  LOW_BYTE_OF_WORD ( &(mrfiIncomingPacket.frame[0]) );  /* DSTADDRL */
  *pCfg++ = /* offset 4 : */  RXTX_DMA_VLEN_XFER_BYTES_PLUS_3;
  *pCfg++ = /* offset 5 : */  RXTX_DMA_LEN;
  *pCfg++ = /* offset 6 : */  RXTX_DMA_WORDSIZE | RXTX_DMA_TMODE | RXTX_DMA_TRIG;
  *pCfg   = /* offset 7 : */  RXTX_DMA_SRCINC_NONE | RXTX_DMA_DESTINC_PLUS_1 |
                              RXTX_DMA_IRQMASK | RXTX_DMA_M8 | RXTX_DMA_PRIORITY;

  /* abort any DMA transfer that might be in progress */
  DMAARM = ABORT | BV( MRFI_DMA_CHAN );

  /* clean out buffer to help protect against spurious frames */
  memset(mrfiIncomingPacket.frame, 0x00, sizeof(mrfiIncomingPacket.frame));

  /* arm the dma channel for receive */
  DMAARM |= BV( MRFI_DMA_CHAN );

  /* Clear interrupts */
  S1CON &= ~(RFIF_1 | RFIF_0); /* Clear MCU interrupt flag */
  RFIF &= ~IRQ_DONE;           /* Clear the interrupt at the source */

  /* send strobe to enter receive mode */
  RFST = SRX;

  /* enable "receive/transmit done" interrupts */
  RFIM |= IM_DONE;
}

/**************************************************************************************************
 * @fn          MRFI_RxOn
 *
 * @brief       Turn on the receiver.  No harm is done if this function is called when
 *              receiver is already on.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_RxOn(void)
{
  /* radio must be awake before we can move it to RX state */
  MRFI_ASSERT( mrfiRadioState != MRFI_RADIO_STATE_OFF );

  /* Put the radio in RX state, if not already */
  if(mrfiRadioState != MRFI_RADIO_STATE_RX)
  {
    mrfiRadioState = MRFI_RADIO_STATE_RX;
    Mrfi_RxModeOn();
  }
}

/**************************************************************************************************
 * @fn          Mrfi_RxModeOff
 *
 * @brief       -
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_RxModeOff(void)
{
  /*disable receive interrupts */
  RFIM &= ~IM_DONE;

  /* turn off radio */
  MRFI_STROBE_IDLE_AND_WAIT();

  /* Abort any ongoing DMA transfer */
  DMAARM = ABORT | BV( MRFI_DMA_CHAN );

  /* Clear any pending DMA interrupts */
  DMAIRQ &= ~BV(MRFI_DMA_CHAN);

  /* flush the receive FIFO of any residual data */
  /* no need for flush. only 1 byte deep fifo. */

  /* clear receive interrupt */
  S1CON &= ~(RFIF_1 | RFIF_0); /* Clear MCU interrupt flag */
  RFIF &= ~IRQ_DONE;           /* Clear the interrupt at the source */
}


/**************************************************************************************************
 * @fn          MRFI_RxIdle
 *
 * @brief       Put radio in idle mode (receiver if off).  No harm is done this function is
 *              called when radio is already idle.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_RxIdle(void)
{
  /* radio must be awake to move it to idle mode */
  MRFI_ASSERT( mrfiRadioState != MRFI_RADIO_STATE_OFF );

  /* if radio is on, turn it off */
  if(mrfiRadioState == MRFI_RADIO_STATE_RX)
  {
    Mrfi_RxModeOff();
    mrfiRadioState = MRFI_RADIO_STATE_IDLE;
  }
}

/****************************************************************************************************
 * @fn          Mrfi_DelayUsec
 *
 * @brief       Execute a delay loop using HW timer. The macro actually used to do the delay
 *              is not thread-safe. This routine makes the delay execution thread-safe by breaking
 *              up the requested delay into small chunks and executing each chunk as a critical
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
void Mrfi_DelayUsec(uint16_t howLong)
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
  while (milliseconds)
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
 * @fn          Mrfi_RandomBackoffDelay
 *
 * @brief       -
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
    Mrfi_DelayUsec( sBackoffHelper );
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
   * a certain duration. This duration depends on
   * the baud rate and the received signal strength itself.
   */
  MRFI_RSSI_VALID_WAIT();

  /* Convert RSSI to decimal and do offset compensation. */
  return( Mrfi_CalculateRssi(RSSI) );
}

/**************************************************************************************************
 * @fn          Mrfi_CalculateRssi
 *
 * @brief       Does binary to decimal conversiont and offset compensation.
 *
 * @param       none
 *
 * @return      RSSI value in units of dBm.
 **************************************************************************************************
 */
int8_t Mrfi_CalculateRssi(uint8_t rawValue)
{
  int16_t rssi;

  /* The raw value is in 2's complement and in half db steps. Convert it to
   * decimal taking into account the offset value.
   */
  if(rawValue >= 128)
  {
    rssi = (int16_t)(rawValue - 256)/2 - MRFI_RSSI_OFFSET;
  }
  else
  {
    rssi = (rawValue/2) - MRFI_RSSI_OFFSET;
  }

  /* Restrict this value to least value can be held in an 8 bit signed int */
  if(rssi < -128)
  {
    rssi = -128;
  }

  return rssi;
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
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */
#if (MRFI_DMA_CHAN != 0)
#error "ERROR: Code implementation requires use of DMA channel zero."
/*  Using a channel other than zero is not difficult to implement.  The hardware
 *  requires channels 1-4 to use a common configuration structure.  For this module
 *  to use a channel other than zero, this data structure would need to be integrated
 *  with the external code.  Hooks are provided to make this as easy as possible.
 */
#endif


#define MRFI_RADIO_TX_FIFO_SIZE     64  /* from datasheet */

/* verify largest possible packet fits within FIFO buffer */
#if ((MRFI_MAX_FRAME_SIZE + MRFI_RX_METRICS_SIZE) > MRFI_RADIO_TX_FIFO_SIZE)
#error "ERROR:  Maximum possible packet length exceeds FIFO buffer.  Decrease value of maximum application payload."
#endif

/* verify that the SmartRF file supplied is compatible */
#if ((!defined SMARTRF_RADIO_CC2510) && \
     (!defined SMARTRF_RADIO_CC2511) && \
     (!defined SMARTRF_RADIO_CC1110) && \
     (!defined SMARTRF_RADIO_CC1111))
#error "ERROR:  The SmartRF export file is not compatible."
#endif


/**************************************************************************************************
*/
