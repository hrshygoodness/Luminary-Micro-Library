/**************************************************************************************************
  Revised:        $Date: 2009-01-13 12:04:00 -0700 (Wed, 13 Jan 2009) $
  Revision:       $Revision: 18768 $

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
 *   Radios: CC2500, CC1100, CC1101
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
#include "bsp_external/mrfi_board_defs.h"
#include "mrfi_defs.h"
#include "mrfi_spi.h"
#include "../common/mrfi_f1f2.h"


/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */

#if (defined MRFI_CC2500)

 #define MRFI_RSSI_OFFSET    72   /* no units */

 /* Worst case wait period in RX state before RSSI becomes valid.
  * These numbers are from Design Note DN505 with added safety margin.
  */
  #define MRFI_RSSI_VALID_DELAY_US    1000

#elif (defined MRFI_CC1100)

  #define MRFI_RSSI_OFFSET    79   /* no units */

 /* Worst case wait period in RX state before RSSI becomes valid.
  * These numbers are from Design Note DN505 with added safety margin.
  */
  #define MRFI_RSSI_VALID_DELAY_US    1300

#elif (defined MRFI_CC1101) || (defined MRFI_CC1100E_470) || (defined MRFI_CC1100E_950)

  #define MRFI_RSSI_OFFSET    74   /* no units */

 /* Worst case wait period in RX state before RSSI becomes valid.
  * These numbers are from Design Note DN505 with added safety margin.
  */
  #define MRFI_RSSI_VALID_DELAY_US    1300

#else
  #error "ERROR: RSSI offset value not defined for this radio"
#endif


#define MRFI_LENGTH_FIELD_OFS               __mrfi_LENGTH_FIELD_OFS__
#define MRFI_LENGTH_FIELD_SIZE              __mrfi_LENGTH_FIELD_SIZE__
#define MRFI_HEADER_SIZE                    __mrfi_HEADER_SIZE__
#define MRFI_FRAME_BODY_OFS                 __mrfi_DST_ADDR_OFS__
#define MRFI_BACKOFF_PERIOD_USECS           __mrfi_BACKOFF_PERIOD_USECS__

#define MRFI_RANDOM_OFFSET                   67
#define MRFI_RANDOM_MULTIPLIER              109
#define MRFI_MIN_SMPL_FRAME_SIZE            (MRFI_HEADER_SIZE + NWK_HDR_SIZE)

/* rx metrics definitions, known as appended "packet status bytes" in datasheet parlance */
#define MRFI_RX_METRICS_CRC_OK_MASK         __mrfi_RX_METRICS_CRC_OK_MASK__
#define MRFI_RX_METRICS_LQI_MASK            __mrfi_RX_METRICS_LQI_MASK__

/* GDO functionality */
#define MRFI_GDO_SYNC           6
#define MRFI_GDO_CCA            9
#define MRFI_GDO_PA_PD          27  /* low when transmit is active, low during sleep */
#define MRFI_GDO_LNA_PD         28  /* low when receive is active, low during sleep */

/* ---------- Radio Abstraction ---------- */
#if (defined MRFI_CC1100)
#define MRFI_RADIO_PARTNUM          0x00
#define MRFI_RADIO_MIN_VERSION      3

#elif (defined MRFI_CC1101)
#define MRFI_RADIO_PARTNUM          0x00
#define MRFI_RADIO_MIN_VERSION      4

#elif (defined MRFI_CC1100E_470)
#define MRFI_RADIO_PARTNUM          0x00
#define MRFI_RADIO_MIN_VERSION      5

#elif (defined MRFI_CC1100E_950)
#define MRFI_RADIO_PARTNUM          0x00
#define MRFI_RADIO_MIN_VERSION      5

#elif (defined MRFI_CC2500)
#define MRFI_RADIO_PARTNUM          0x80
#define MRFI_RADIO_MIN_VERSION      3
#else
#error "ERROR: Missing or unrecognized radio."
#endif

/* GDO0 output pin configuration */
#define MRFI_SETTING_IOCFG0     MRFI_GDO_SYNC

/* Main Radio Control State Machine control configuration:
 * Auto Calibrate - when going from IDLE to RX/TX
 * PO_TIMEOUT is extracted from SmartRF setting.
 * XOSC is OFF in Sleep state.
 */
#define MRFI_SETTING_MCSM0      (0x10 | (SMARTRF_SETTING_MCSM0 & (BV(2)|BV(3))))

/* Main Radio Control State Machine control configuration:
 * - Remain RX state after RX
 * - Go to IDLE after TX
 * - RSSI below threshold and NOT receiving.
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

/* FIFO threshold - this register has fields that need to be configured for the CC1101 */
#define MRFI_SETTING_FIFOTHR    (0x07 | (SMARTRF_SETTING_FIFOTHR & (BV(4)|BV(5)|BV(6))))

/* Max time we can be in a critical section within the delay function.
 * This could be fine-tuned by observing the overhead is calling the bsp delay
 * function. The overhead should be very small compared to this value.
 * Note that the max value for this must be less than 19 usec with the
 * default CLKCON.TICKSPD and CLKCON.CLOCKSPD settings and external 26 MHz
 * crystal as a clock source (which we use).
 *
 * Be careful of direct calls to Mrfi_DelayUsec().
 */
#define MRFI_MAX_DELAY_US 16 /* usec */

#define MRFI_PKTSTATUS_CCA BV(4)
#define MRFI_PKTSTATUS_CS  BV(6)

  /* The SW timer is calibrated by adjusting the call to the microsecond delay
   * routine. This allows maximum calibration control with repects to the longer
   * times requested by applicationsd and decouples internal from external calls
   * to the microsecond routine which can be calibrated independently.
   */
#if defined(SW_TIMER)
#define APP_USEC_VALUE    496
#else
#define APP_USEC_VALUE    1000
#endif

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */
#define MRFI_SYNC_PIN_IS_HIGH()                     MRFI_GDO0_PIN_IS_HIGH()
#define MRFI_ENABLE_SYNC_PIN_INT()                  MRFI_ENABLE_GDO0_INT()
#define MRFI_DISABLE_SYNC_PIN_INT()                 MRFI_DISABLE_GDO0_INT()
#define MRFI_SYNC_PIN_INT_IS_ENABLED()              MRFI_GDO0_INT_IS_ENABLED()
#define MRFI_CLEAR_SYNC_PIN_INT_FLAG()              MRFI_CLEAR_GDO0_INT_FLAG()
#define MRFI_SYNC_PIN_INT_FLAG_IS_SET()             MRFI_GDO0_INT_FLAG_IS_SET()
#define MRFI_CONFIG_SYNC_PIN_FALLING_EDGE_INT()     MRFI_CONFIG_GDO0_FALLING_EDGE_INT()

#define MRFI_PAPD_PIN_IS_HIGH()                     MRFI_SYNC_PIN_IS_HIGH()
#define MRFI_CLEAR_PAPD_PIN_INT_FLAG()              MRFI_CLEAR_SYNC_PIN_INT_FLAG()
#define MRFI_PAPD_INT_FLAG_IS_SET()                 MRFI_SYNC_PIN_INT_FLAG_IS_SET()
#define MRFI_CONFIG_PAPD_FALLING_EDGE_INT()         MRFI_CONFIG_SYNC_PIN_FALLING_EDGE_INT()

#define MRFI_CONFIG_GDO0_AS_PAPD_SIGNAL()           mrfiSpiWriteReg(IOCFG0, MRFI_GDO_PA_PD)
#define MRFI_CONFIG_GDO0_AS_SYNC_SIGNAL()           mrfiSpiWriteReg(IOCFG0, MRFI_GDO_SYNC)


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
    if(mrfiSpiReadReg(PKTSTATUS) & (MRFI_PKTSTATUS_CCA | MRFI_PKTSTATUS_CS))  \
    {                                                                         \
      break;                                                                  \
    }                                                                         \
    Mrfi_DelayUsec(64); /* sleep */                                           \
    delay -= 64;                                                              \
  }while(delay > 0);                                                          \
}                                                                             \

#define MRFI_STROBE_IDLE_AND_WAIT()                   \
{                                                     \
  mrfiSpiCmdStrobe( SIDLE );                          \
  while (mrfiSpiCmdStrobe( SNOP ) & 0xF0) ;           \
}

/* ------------------------------------------------------------------------------------------------
 *                                    Local Constants
 * ------------------------------------------------------------------------------------------------
 */
static const uint8_t mrfiRadioCfg[][2] =
{
  /* internal radio configuration */
  {  IOCFG0,    MRFI_SETTING_IOCFG0       },
  {  MCSM1,     MRFI_SETTING_MCSM1        }, /* CCA mode, RX_OFF_MODE and TX_OFF_MODE */
  {  MCSM0,     MRFI_SETTING_MCSM0        }, /* AUTO_CAL and XOSC state in sleep */
  {  PKTLEN,    MRFI_SETTING_PKTLEN       },
  {  PKTCTRL0,  MRFI_SETTING_PKTCTRL0     },
#ifdef MRFI_CC1101
  {  FIFOTHR,   MRFI_SETTING_FIFOTHR      },
#endif

  /* imported SmartRF radio configuration */
  {  FSCTRL1,   SMARTRF_SETTING_FSCTRL1   },
  {  FSCTRL0,   SMARTRF_SETTING_FSCTRL0   },
  {  FREQ2,     SMARTRF_SETTING_FREQ2     },
  {  FREQ1,     SMARTRF_SETTING_FREQ1     },
  {  FREQ0,     SMARTRF_SETTING_FREQ0     },
  {  MDMCFG4,   SMARTRF_SETTING_MDMCFG4   },
  {  MDMCFG3,   SMARTRF_SETTING_MDMCFG3   },
  {  MDMCFG2,   SMARTRF_SETTING_MDMCFG2   },
  {  MDMCFG1,   SMARTRF_SETTING_MDMCFG1   },
  {  MDMCFG0,   SMARTRF_SETTING_MDMCFG0   },
  {  DEVIATN,   SMARTRF_SETTING_DEVIATN   },
  {  FOCCFG,    SMARTRF_SETTING_FOCCFG    },
  {  BSCFG,     SMARTRF_SETTING_BSCFG     },
  {  AGCCTRL2,  SMARTRF_SETTING_AGCCTRL2  },
  {  AGCCTRL1,  SMARTRF_SETTING_AGCCTRL1  },
  {  AGCCTRL0,  SMARTRF_SETTING_AGCCTRL0  },
  {  FREND1,    SMARTRF_SETTING_FREND1    },
  {  FREND0,    SMARTRF_SETTING_FREND0    },
  {  FSCAL3,    SMARTRF_SETTING_FSCAL3    },
  {  FSCAL2,    SMARTRF_SETTING_FSCAL2    },
  {  FSCAL1,    SMARTRF_SETTING_FSCAL1    },
  {  FSCAL0,    SMARTRF_SETTING_FSCAL0    },
  {  TEST2,     SMARTRF_SETTING_TEST2     },
  {  TEST1,     SMARTRF_SETTING_TEST1     },
  {  TEST0,     SMARTRF_SETTING_TEST0     },
};


/* ------------------------------------------------------------------------------------------------
 *                                       Local Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void MRFI_GpioIsr(void); /* this called from mrfi_board.c */
static void Mrfi_SyncPinRxIsr(void);
static void Mrfi_RxModeOn(void);
static void Mrfi_RandomBackoffDelay(void);
static void Mrfi_RxModeOff(void);
static void Mrfi_DelayUsec(uint16_t howLong);
static void Mrfi_DelayUsecSem(uint16_t howLong);
static int8_t Mrfi_CalculateRssi(uint8_t rawValue);

/* ------------------------------------------------------------------------------------------------
 *                                       Local Variables
 * ------------------------------------------------------------------------------------------------
 */
static uint8_t mrfiRadioState  = MRFI_RADIO_STATE_UNKNOWN;
static mrfiPacket_t mrfiIncomingPacket;
static uint8_t mrfiRndSeed = 0;

/* reply delay support */
static volatile uint8_t  sKillSem = 0;
static volatile uint8_t  sReplyDelayContext = 0;
static          uint16_t sReplyDelayScalar = 0;
static          uint16_t sBackoffHelper = 0;

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
   *    Initialization
   *   -----------------
   */

  memset(&mrfiIncomingPacket, 0x0, sizeof(mrfiIncomingPacket));

  /* initialize GPIO pins */
  MRFI_CONFIG_GDO0_PIN_AS_INPUT();

  /* initialize SPI */
  mrfiSpiInit();

  /* ------------------------------------------------------------------
   *    Radio power-up reset
   *   ----------------------
   */
  MRFI_ASSERT(MRFI_SPI_CSN_IS_HIGH());

  /* pulse CSn low then high */
  MRFI_SPI_DRIVE_CSN_LOW();
  Mrfi_DelayUsec(10);
  MRFI_SPI_DRIVE_CSN_HIGH();

  /* hold CSn high for at least 40 microseconds */
  Mrfi_DelayUsec(40);

  /* pull CSn low and wait for SO to go low */
  MRFI_SPI_DRIVE_CSN_LOW();
  while (MRFI_SPI_SO_IS_HIGH());

  /* directly send strobe command - cannot use function as it affects CSn pin */
  MRFI_SPI_WRITE_BYTE(SRES);
  MRFI_SPI_WAIT_DONE();

  /* wait for SO to go low again, reset is complete at that point */
  while (MRFI_SPI_SO_IS_HIGH());

  /* return CSn pin to its default high level */
  MRFI_SPI_DRIVE_CSN_HIGH();

  /* ------------------------------------------------------------------
   *    Run-time integrity checks
   *   ---------------------------
   */

  /* verify that SPI is working, PKTLEN is an arbitrary read/write register used for testing */
#ifdef MRFI_ASSERTS_ARE_ON
#define TEST_VALUE 0xA5
  do
  {
    mrfiSpiWriteReg( PKTLEN, TEST_VALUE );
  }
  while(!(mrfiSpiReadReg( PKTLEN ) == TEST_VALUE )); /* SPI is not responding */
#endif

  /* verify the correct radio is installed */
  MRFI_ASSERT( mrfiSpiReadReg( PARTNUM ) == MRFI_RADIO_PARTNUM );      /* incorrect radio specified */
  MRFI_ASSERT( mrfiSpiReadReg( VERSION ) >= MRFI_RADIO_MIN_VERSION );  /* obsolete radio specified  */

  /* ------------------------------------------------------------------
   *    Configure radio
   *   -----------------
   */

  /* initialize radio registers */
  {
    uint8_t i;

    for (i=0; i<(sizeof(mrfiRadioCfg)/sizeof(mrfiRadioCfg[0])); i++)
    {
      mrfiSpiWriteReg(mrfiRadioCfg[i][0], mrfiRadioCfg[i][1]);
    }
  }

  /* Initial radio state is IDLE state */
  mrfiRadioState = MRFI_RADIO_STATE_IDLE;

  /* set default channel */
  MRFI_SetLogicalChannel( 0 );

  /* set default power */
  MRFI_SetRFPwr(MRFI_NUM_POWER_SETTINGS - 1);

  /* Generate Random seed:
   * We will use the RSSI value to generate our random seed.
   */

  /* Put the radio in RX state */
  mrfiSpiCmdStrobe( SRX );

  /* delay for the rssi to be valid */
  MRFI_RSSI_VALID_WAIT();

  /* use most random bit of rssi to populate the random seed */
  {
    uint8_t i;
    for(i=0; i<16; i++)
    {
      mrfiRndSeed = (mrfiRndSeed << 1) | (mrfiSpiReadReg(RSSI) & 0x01);
    }
  }

  /* Force the seed to be non-zero by setting one bit, just in case... */
  mrfiRndSeed |= 0x0080;

  /* Turn off RF. */
  Mrfi_RxModeOff();

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
   * Note that we assume a 26 MHz clock for the radio...
   * ***************************************************************************************
   */
#define   MRFI_RADIO_OSC_FREQ        26000000
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

  /* Clean out buffer to protect against spurious frames */
  memset(mrfiIncomingPacket.frame, 0x00, sizeof(mrfiIncomingPacket.frame));
  memset(mrfiIncomingPacket.rxMetrics, 0x00, sizeof(mrfiIncomingPacket.rxMetrics));

  /* ------------------------------------------------------------------
   *    Configure interrupts
   *   ----------------------
   */

  /*
   *  Configure and enable the SYNC signal interrupt.
   *
   *  This interrupt is used to indicate receive.  The SYNC signal goes
   *  high when a receive OR a transmit begins.  It goes high once the
   *  sync word is received or transmitted and then goes low again once
   *  the packet completes.
   */
  MRFI_CONFIG_GDO0_AS_SYNC_SIGNAL();
  MRFI_CONFIG_SYNC_PIN_FALLING_EDGE_INT();
  MRFI_CLEAR_SYNC_PIN_INT_FLAG();

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
  uint8_t txBufLen;
  uint8_t returnValue = MRFI_TX_RESULT_SUCCESS;

  /* radio must be awake to transmit */
  MRFI_ASSERT( mrfiRadioState != MRFI_RADIO_STATE_OFF );

  /* Turn off reciever. We can ignore/drop incoming packets during transmit. */
  Mrfi_RxModeOff();

  /* compute number of bytes to write to transmit FIFO */
  txBufLen = pPacket->frame[MRFI_LENGTH_FIELD_OFS] + MRFI_LENGTH_FIELD_SIZE;

  /* ------------------------------------------------------------------
   *    Write packet to transmit FIFO
   *   --------------------------------
   */
  mrfiSpiWriteTxFifo(&(pPacket->frame[0]), txBufLen);


  /* ------------------------------------------------------------------
   *    Immediate transmit
   *   ---------------------
   */
  if (txType == MRFI_TX_TYPE_FORCED)
  {
    /* Issue the TX strobe. */
    mrfiSpiCmdStrobe( STX );

    /* Wait for transmit to complete */
    while(!MRFI_SYNC_PIN_INT_FLAG_IS_SET());

    /* Clear the interrupt flag */
    MRFI_CLEAR_SYNC_PIN_INT_FLAG();
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

    /* For CCA algorithm, we need to know the transition from the RX state to
     * the TX state. There is no need for SYNC signal in this logic. So we
     * can re-configure the GDO_0 output from the radio to be PA_PD signal
     * instead of the SYNC signal.
     * Since both SYNC and PA_PD are used as falling edge interrupts, we
     * don't need to reconfigure the MCU input.
     */
    MRFI_CONFIG_GDO0_AS_PAPD_SIGNAL();

    /* ===============================================================================
     *    Main Loop
     *  =============
     */
    for (;;)
    {
      /* Radio must be in RX mode for CCA to happen.
       * Otherwise it will transmit without CCA happening.
       */

      /* Can not use the Mrfi_RxModeOn() function here since it turns on the
       * Rx interrupt, which we don't want in this case.
       */
      mrfiSpiCmdStrobe( SRX );

      /* wait for the rssi to be valid. */
      MRFI_RSSI_VALID_WAIT();

      /*
       *  Clear the PA_PD pin interrupt flag.  This flag, not the interrupt itself,
       *  is used to capture the transition that indicates a transmit was started.
       *  The pin level cannot be used to indicate transmit success as timing may
       *  prevent the transition from being detected.  The interrupt latch captures
       *  the event regardless of timing.
       */
      MRFI_CLEAR_PAPD_PIN_INT_FLAG();

      /* send strobe to initiate transmit */
      mrfiSpiCmdStrobe( STX );

      /* Delay long enough for the PA_PD signal to indicate a
       * successful transmit. This is the 250 XOSC periods
       * (9.6 us for a 26 MHz crystal) See section 19.6 of 2500 datasheet.
       * Found out that we need a delay of atleast 20 us on CC2500 and
       * 25 us on CC1100 to see the PA_PD signal change.
       */
      Mrfi_DelayUsec(25);


      /* PA_PD signal goes from HIGH to LOW when going from RX state.
       * This transition is trapped as a falling edge interrupt flag
       * to indicate that CCA passed and the transmit has started.
       */
      if (MRFI_PAPD_INT_FLAG_IS_SET())
      {
        /* ------------------------------------------------------------------
        *    Clear Channel Assessment passed.
        *   ----------------------------------
        */

        /* Clear the PA_PD int flag */
        MRFI_CLEAR_PAPD_PIN_INT_FLAG();

        /* PA_PD signal stays LOW while in TX state and goes back to HIGH when
         * the radio transitions to RX state.
         */
        /* wait for transmit to complete */
        while (!MRFI_PAPD_PIN_IS_HIGH());

        /* transmit done, break */
        break;
      }
      else
      {
        /* ------------------------------------------------------------------
         *    Clear Channel Assessment failed.
         *   ----------------------------------
         */

        /* Turn off radio and save some power during backoff */

        /* NOTE: Can't use Mrfi_RxModeOff() - since it tries to update the
         * sync signal status which we are not using during the TX operation.
         */
        MRFI_STROBE_IDLE_AND_WAIT();

        /* flush the receive FIFO of any residual data */
        mrfiSpiCmdStrobe( SFRX );

        /* Retry ? */
        if (ccaRetries != 0)
        {
          /* delay for a random number of backoffs */
          Mrfi_RandomBackoffDelay();

          /* decrement CCA retries before loop continues */
          ccaRetries--;
        }
        else /* No CCA retries are left, abort */
        {
          /* set return value for failed transmit and break */
          returnValue = MRFI_TX_RESULT_FAILED;
          break;
        }
      } /* CCA Failed */
    } /* CCA loop */
  }/* txType is CCA */

  /* Done with TX. Clean up time... */

  /* Radio is already in IDLE state */

  /*
   * Flush the transmit FIFO.  It must be flushed so that
   * the next transmit can start with a clean slate.
   */
  mrfiSpiCmdStrobe( SFTX );

  /* Restore GDO_0 to be SYNC signal */
  MRFI_CONFIG_GDO0_AS_SYNC_SIGNAL();

  /* If the radio was in RX state when transmit was attempted,
   * put it back to Rx On state.
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
 * @fn          Mrfi_SyncPinRxIsr
 *
 * @brief       This interrupt is called when the SYNC signal transition from high to low.
 *              The sync signal is routed to the sync pin which is a GPIO pin.  This high-to-low
 *              transition signifies a receive has completed.  The SYNC signal also goes from
 *              high to low when a transmit completes.   This is protected against within the
 *              transmit function by disabling sync pin interrupts until transmit completes.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_SyncPinRxIsr(void)
{
  uint8_t frameLen;
  uint8_t rxBytes;

  /* We should receive this interrupt only in RX state
   * Should never receive it if RX was turned On only for
   * some internal mrfi processing like - during CCA.
   * Otherwise something is terribly wrong.
   */
  MRFI_ASSERT( mrfiRadioState == MRFI_RADIO_STATE_RX );

  /* ------------------------------------------------------------------
   *    Get RXBYTES
   *   -------------
   */

  /*
   *  Read the RXBYTES register from the radio.
   *  Bit description of RXBYTES register:
   *    bit 7     - RXFIFO_OVERFLOW, set if receive overflow occurred
   *    bits 6:0  - NUM_BYTES, number of bytes in receive FIFO
   *
   *  Due a chip bug, the RXBYTES register must read the same value twice
   *  in a row to guarantee an accurate value.
   */
  {
    uint8_t rxBytesVerify;

    rxBytesVerify = mrfiSpiReadReg( RXBYTES );

    do
    {
      rxBytes = rxBytesVerify;
      rxBytesVerify = mrfiSpiReadReg( RXBYTES );
    }
    while (rxBytes != rxBytesVerify);
  }


  /* ------------------------------------------------------------------
   *    FIFO empty?
   *   -------------
   */

  /*
   *  See if the receive FIFIO is empty before attempting to read from it.
   *  It is possible nothing the FIFO is empty even though the interrupt fired.
   *  This can happen if address check is enabled and a non-matching packet is
   *  received.  In that case, the radio automatically removes the packet from
   *  the FIFO.
   */
  if (rxBytes == 0)
  {
    /* receive FIFO is empty - do nothing, skip to end */
  }
  else
  {
    /* receive FIFO is not empty, continue processing */

    /* ------------------------------------------------------------------
     *    Process frame length
     *   ----------------------
     */

    /* read the first byte from FIFO - the packet length */
    mrfiSpiReadRxFifo(&frameLen, MRFI_LENGTH_FIELD_SIZE);

    /*
     *  Make sure that the frame length just read corresponds to number of bytes in the buffer.
     *  If these do not match up something is wrong.
     *
     *  This can happen for several reasons:
     *   1) Incoming packet has an incorrect format or is corrupted.
     *   2) The receive FIFO overflowed.  Overflow is indicated by the high
     *      bit of rxBytes.  This guarantees the value of rxBytes value will not
     *      match the number of bytes in the FIFO for overflow condition.
     *   3) Interrupts were blocked for an abnormally long time which
     *      allowed a following packet to at least start filling the
     *      receive FIFO.  In this case, all received and partially received
     *      packets will be lost - the packet in the FIFO and the packet coming in.
     *      This is the price the user pays if they implement a giant
     *      critical section.
     *   4) A failed transmit forced radio to IDLE state to flush the transmit FIFO.
     *      This could cause an active receive to be cut short.
     *
     *  Also check the sanity of the length to guard against rogue frames.
     */
    if ((rxBytes != (frameLen + MRFI_LENGTH_FIELD_SIZE + MRFI_RX_METRICS_SIZE))           ||
        ((frameLen + MRFI_LENGTH_FIELD_SIZE) > MRFI_MAX_FRAME_SIZE) ||
        (frameLen < MRFI_MIN_SMPL_FRAME_SIZE)
       )
    {
      bspIState_t s;

      /* mismatch between bytes-in-FIFO and frame length */

      /*
       *  Flush receive FIFO to reset receive.  Must go to IDLE state to do this.
       *  The critical section guarantees a transmit does not occur while cleaning up.
       */
      BSP_ENTER_CRITICAL_SECTION(s);
      MRFI_STROBE_IDLE_AND_WAIT();
      mrfiSpiCmdStrobe( SFRX );
      mrfiSpiCmdStrobe( SRX );
      BSP_EXIT_CRITICAL_SECTION(s);

      /* flush complete, skip to end */
    }
    else
    {
      /* bytes-in-FIFO and frame length match up - continue processing */

      /* ------------------------------------------------------------------
       *    Get packet
       *   ------------
       */

      /* clean out buffer to help protect against spurious frames */
      memset(mrfiIncomingPacket.frame, 0x00, sizeof(mrfiIncomingPacket.frame));

      /* set length field */
      mrfiIncomingPacket.frame[MRFI_LENGTH_FIELD_OFS] = frameLen;

      /* get packet from FIFO */
      mrfiSpiReadRxFifo(&(mrfiIncomingPacket.frame[MRFI_FRAME_BODY_OFS]), frameLen);

      /* get receive metrics from FIFO */
      mrfiSpiReadRxFifo(&(mrfiIncomingPacket.rxMetrics[0]), MRFI_RX_METRICS_SIZE);


      /* ------------------------------------------------------------------
       *    CRC check
       *   ------------
       */

      /*
       *  Note!  Automatic CRC check is not, and must not, be enabled.  This feature
       *  flushes the *entire* receive FIFO when CRC fails.  If this feature is
       *  enabled it is possible to be reading from the FIFO and have a second
       *  receive occur that fails CRC and automatically flushes the receive FIFO.
       *  This could cause reads from an empty receive FIFO which puts the radio
       *  into an undefined state.
       */

      /* determine if CRC failed */
      if (!(mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_CRC_LQI_OFS] & MRFI_RX_METRICS_CRC_OK_MASK))
      {
        /* CRC failed - do nothing, skip to end */
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
    }
  }

  /* ------------------------------------------------------------------
   *    End of function
   *   -------------------
   */
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
  /* clear any residual receive interrupt */
  MRFI_CLEAR_SYNC_PIN_INT_FLAG();

  /* send strobe to enter receive mode */
  mrfiSpiCmdStrobe( SRX );

  /* enable receive interrupts */
  MRFI_ENABLE_SYNC_PIN_INT();
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

  /* if radio is off, turn it on */
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
  MRFI_DISABLE_SYNC_PIN_INT();

  /* turn off radio */
  MRFI_STROBE_IDLE_AND_WAIT();

  /* flush the receive FIFO of any residual data */
  mrfiSpiCmdStrobe( SFRX );

  /* clear receive interrupt */
  MRFI_CLEAR_SYNC_PIN_INT_FLAG();
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


/**************************************************************************************************
 * @fn          MRFI_Sleep
 *
 * @brief       Request radio go to sleep.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_Sleep(void)
{
  bspIState_t s;

  /* Critical section necessary for watertight testing and
   * setting of state variables.
   */
  BSP_ENTER_CRITICAL_SECTION(s);

  /* If radio is not asleep, put it to sleep */
  if(mrfiRadioState != MRFI_RADIO_STATE_OFF)
  {
    /* go to idle so radio is in a known state before sleeping */
    MRFI_RxIdle();

    mrfiSpiCmdStrobe( SPWD );

    /* Our new state is OFF */
    mrfiRadioState = MRFI_RADIO_STATE_OFF;
  }

  BSP_EXIT_CRITICAL_SECTION(s);
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
  /* if radio is already awake, just ignore wakeup request */
  if(mrfiRadioState != MRFI_RADIO_STATE_OFF)
  {
    return;
  }

  /* drive CSn low to initiate wakeup */
  MRFI_SPI_DRIVE_CSN_LOW();

  /* wait for MISO to go high indicating the oscillator is stable */
  while (MRFI_SPI_SO_IS_HIGH());

  /* wakeup is complete, drive CSn high and continue */
  MRFI_SPI_DRIVE_CSN_HIGH();

/*
 *  The test registers must be restored after sleep for the CC1100 and CC2500 radios.
 *  This is not required for the CC1101 radio.
 */
#ifndef MRFI_CC1101
  mrfiSpiWriteReg( TEST2, SMARTRF_SETTING_TEST2 );
  mrfiSpiWriteReg( TEST1, SMARTRF_SETTING_TEST1 );
  mrfiSpiWriteReg( TEST0, SMARTRF_SETTING_TEST0 );
#endif

  /* enter idle mode */
  mrfiRadioState = MRFI_RADIO_STATE_IDLE;
  MRFI_STROBE_IDLE_AND_WAIT();
}


/**************************************************************************************************
 * @fn          MRFI_GpioIsr
 *
 * @brief       Interrupt Service Routine for handling GPIO interrupts.  The sync pin interrupt
 *              comes in through GPIO.  This function is designed to be compatible with "ganged"
 *              interrupts.  If the GPIO interrupt services more than just a single pin (very
 *              common), this function just needs to be called from the higher level interrupt
 *              service routine.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_GpioIsr(void)
{
  /* see if sync pin interrupt is enabled and has fired */
  if (MRFI_SYNC_PIN_INT_IS_ENABLED() && MRFI_SYNC_PIN_INT_FLAG_IS_SET())
  {
    /*  clear the sync pin interrupt, run sync pin ISR */

    /*
     *  NOTE!  The following macro clears the interrupt flag but it also *must*
     *  reset the interrupt capture.  In other words, if a second interrupt
     *  occurs after the flag is cleared it must be processed, i.e. this interrupt
     *  exits then immediately starts again.  Most microcontrollers handle this
     *  naturally but it must be verified for every target.
     */
    MRFI_CLEAR_SYNC_PIN_INT_FLAG();
    Mrfi_SyncPinRxIsr();
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
  uint8_t regValue;

  /* Radio must be in RX state to measure rssi. */
  MRFI_ASSERT( mrfiRadioState == MRFI_RADIO_STATE_RX );

  /* Wait for the RSSI to be valid:
   * Just having the Radio ON is not enough to read
   * the correct RSSI value. The Radio must in RX mode for
   * a certain duration. This duration depends on
   * the baud rate and the received signal strength itself.
   */
  MRFI_RSSI_VALID_WAIT();

  /* Read the RSSI value */
  regValue = mrfiSpiReadReg( RSSI );

  /* convert and do offset compensation */
  return( Mrfi_CalculateRssi(regValue) );
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

/****************************************************************************************************
 * @fn          Mrfi_DelayUsec
 *
 * @brief       Execute a delay loop using HW timer. The macro actually used to do the delay
 *              is not thread-safe. This routine makes the delay execution thread-safe by breaking
 *              up the requested delay up into small chunks and executing each chunk as a critical
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


#define MRFI_RADIO_TX_FIFO_SIZE     64  /* from datasheet */

/* verify largest possible packet fits within FIFO buffer */
#if ((MRFI_MAX_FRAME_SIZE + MRFI_RX_METRICS_SIZE) > MRFI_RADIO_TX_FIFO_SIZE)
#error "ERROR:  Maximum possible packet length exceeds FIFO buffer.  Decrease value of maximum application payload."
#endif

/* verify that the SmartRF file supplied is compatible */
#if ((!defined SMARTRF_RADIO_CC2500) && \
     (!defined SMARTRF_RADIO_CC1100) && \
     (!defined SMARTRF_RADIO_CC1101) && \
     (!defined SMARTRF_RADIO_CC1100E))
#error "ERROR:  The SmartRF export file is not compatible."
#endif

/*
 *  These asserts happen if there is extraneous compiler padding of arrays.
 *  Modify compiler settings for no padding, or, if that is not possible,
 *  comment out the offending asserts.
 */
BSP_STATIC_ASSERT(sizeof(mrfiRadioCfg) == ((sizeof(mrfiRadioCfg)/sizeof(mrfiRadioCfg[0])) * sizeof(mrfiRadioCfg[0])));


/**************************************************************************************************
*/
