/**************************************************************************************************
  Filename:       mrfi_spi.c
  Revised:        $Date: 2009-10-18 18:47:34 -0700 (Sun, 18 Oct 2009) $
  Revision:       $Revision: 20933 $

  Copyright 2008-2009 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "mrfi_spi.h"

#define MRFI_SPI_ASSERT(x)        MRFI_ASSERT(x)

/* ------------------------------------------------------------------------------------------------
 *                                            Defines
 * ------------------------------------------------------------------------------------------------
 */
#define SPI_ACCESS_BUF_LEN      3

#define NUM_BYTES_STROBE_CMD    1
#define NUM_BYTES_BIT_SET_CLR   2
#define NUM_BYTES_REG_ACCESS    2
#define NUM_BYTES_MEM_ACCESS    3
#define NUM_BYTES_RANDOM        3

#define FIFO_ACCESS_TX_WRITE        0
#define FIFO_ACCESS_RX_READ         1

/* ------------------------------------------------------------------------------------------------
 *                                         Global Variables
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                       Local Prototypes
 * ------------------------------------------------------------------------------------------------
 */
static void spiFifoAccess(uint8_t * pData, uint8_t len, uint8_t writeFlag);
static uint8_t spiSendBytes(uint8_t * pBytes, uint8_t numBytes);
static void spiWriteRamByte(uint16_t ramAddr, uint8_t byte);

/**************************************************************************************************
 * @fn          mrfiSpiInit
 *
 * @brief       Initialize SPI.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiSpiInit(void)
{
  /* configure all SPI related pins */
  MRFI_SPI_CONFIG_CSN_PIN_AS_OUTPUT();
  MRFI_SPI_CONFIG_SCLK_PIN_AS_OUTPUT();
  MRFI_SPI_CONFIG_SI_PIN_AS_OUTPUT();
  MRFI_SPI_CONFIG_SO_PIN_AS_INPUT();

  /* set CSn to default high level */
  MRFI_SPI_SET_CHIP_SELECT_OFF();

  /* initialize the SPI registers */
  MRFI_SPI_INIT();
}

/**************************************************************************************************
 * @fn          mrfiSpiCmdStrobe
 *
 * @brief       Send command strobe to the radio.  Returns status byte read during transfer
 *              of strobe command.
 *
 * @param       opCode - op code of the strobe command
 *
 * @return      status byte of radio
 **************************************************************************************************
 */
uint8_t mrfiSpiCmdStrobe(uint8_t opCode)
{
  uint8_t buf[SPI_ACCESS_BUF_LEN];

  /* set first byte of array as op code */
  buf[0] = opCode;

  /* send strobe command and pass up return value */
  return( spiSendBytes(&buf[0], NUM_BYTES_STROBE_CMD) );
}

/**************************************************************************************************
 * @fn          mfriSpiReadReg
 *
 * @brief       Read value from radio regiser.
 *
 * @param       regAddr - address of register
 *
 * @return      register value
 **************************************************************************************************
 */
uint8_t mrfiSpiReadReg(uint8_t regAddr)
{
  /* fast register access is available for registers in first 0x40 bytes */
  if (regAddr <= 0x3F)
  {
    uint8_t buf[NUM_BYTES_REG_ACCESS];

    /* set up a register read command */
    buf[0] = REGRD | regAddr;

    /* send read command and pass up return value */
    return( spiSendBytes(&buf[0], NUM_BYTES_REG_ACCESS) );
  }
  else
  {
    uint8_t buf[NUM_BYTES_MEM_ACCESS];

    /* set up a memory read command, note memory above 0xFF not accessible via this function */
    buf[0] = MEMRD;
    buf[1] = regAddr;

    /* send read command and pass up return value */
    return( spiSendBytes(&buf[0], NUM_BYTES_MEM_ACCESS) );
  }
}

/**************************************************************************************************
 * @fn          mrfiSpiWriteReg
 *
 * @brief       Write value to radio register.
 *
 * @param       regAddr  - address of register
 * @param       regValue - register value to write
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiSpiWriteReg(uint8_t regAddr, uint8_t regValue)
{
  /* fast register access is available for registers in first 0x40 bytes */
  if (regAddr <= 0x3F)
  {
    uint8_t buf[NUM_BYTES_REG_ACCESS];

    /* set up a register write command */
    buf[0] = REGWR | regAddr;
    buf[1] = regValue;

    /* send write command */
    spiSendBytes(&buf[0], NUM_BYTES_REG_ACCESS);
  }
  else
  {
    uint8_t buf[NUM_BYTES_MEM_ACCESS];

    /* set up a memory write command, note memory above 0xFF not accessible via this function */
    buf[0] = MEMWR;
    buf[1] = regAddr;
    buf[2] = regValue;
    /* send write command */
    spiSendBytes(&buf[0], NUM_BYTES_MEM_ACCESS);
  }
}

/*=================================================================================================
 * @fn          spiSendBytes
 *
 * @brief       Primitive for sending byte via SPI.  Returns SPI transferred during sending
 *              of last byte.
 *
 * @param       pSendBytes   - pointer to bytes to send over SPI
 * @param       numSendBytes - number of bytes to send
 *
 * @return      byte read from SPI after last byte sent
 *=================================================================================================
 */
static uint8_t spiSendBytes(uint8_t * pBytes, uint8_t numBytes)
{
  mrfiSpiIState_t s;
  uint8_t returnValue;

  /*-------------------------------------------------------------------------------
   *  Disable interrupts that call SPI functions.
   */
  MRFI_SPI_ENTER_CRITICAL_SECTION(s);

  /*-------------------------------------------------------------------------------
   *  Turn chip select "off" and then "on" to clear any current SPI access.
   */
  MRFI_SPI_SET_CHIP_SELECT_OFF();
  MRFI_SPI_SET_CHIP_SELECT_ON();

  /*-------------------------------------------------------------------------------
   *  execute based on type of access
   */
  while (numBytes)
  {
    MRFI_SPI_WRITE_BYTE(*pBytes);
    pBytes++;
    numBytes--;
    MRFI_SPI_WAIT_DONE();
  }

 /*-------------------------------------------------------------------------------
  *  SPI data register now contains the status byte. The status byte is
  *  discarded.
  */
  returnValue = MRFI_SPI_READ_BYTE();

  /*-------------------------------------------------------------------------------
   *  Turn off chip select.  Enable interrupts that call SPI functions.
   */
  MRFI_SPI_SET_CHIP_SELECT_OFF();
  MRFI_SPI_EXIT_CRITICAL_SECTION(s);

  return(returnValue);
}

/**************************************************************************************************
 * @fn          mrfiSpiWriteRamUint16
 *
 * @brief       Write unsigned 16-bit value to radio RAM.
 *
 * @param       ramAddr    - radio RAM address
 * @param       pWriteData - pointer to data to write
 * @param       len        - length of data in bytes
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiSpiWriteRamUint16(uint16_t ramAddr, uint16_t data)
{
  spiWriteRamByte(ramAddr,   data & 0xFF);
  spiWriteRamByte(ramAddr+1, data >> 8);
}

/**************************************************************************************************
 * @fn          mrfiSpiWriteRam
 *
 * @brief       Write data to radio RAM.
 *
 * @param       ramAddr - radio RAM address
 * @param       pData   - pointer to data to write
 * @param       len     - length of data in bytes
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiSpiWriteRam(uint16_t ramAddr, uint8_t * pData, uint8_t len)
{
  while (len)
  {
    spiWriteRamByte(ramAddr, *pData);
    ramAddr++;
    pData++;
    len--;
  }
}

/*=================================================================================================
 * @fn          spiWriteRamByte
 *
 * @brief       Write a byte to radio RAM.
 *
 * @param       ramAddr - radio RAM address
 * @param       byte    - data byte to write to RAM
 *
 * @return      none
 *=================================================================================================
 */
static void spiWriteRamByte(uint16_t ramAddr, uint8_t byte)
{
  uint8_t buf[NUM_BYTES_MEM_ACCESS];

  MRFI_SPI_ASSERT(ramAddr <= 0xFFF); /* address out of range */

  /* setup for a memory write */
  buf[0] = MEMWR | (ramAddr >> 8);
  buf[1] = ramAddr & 0xFF;
  buf[2] = byte;

  /* send bytes out via SPI */
  spiSendBytes(&buf[0], NUM_BYTES_MEM_ACCESS);
}

/**************************************************************************************************
 * @fn          mrfiSpiReadRam
 *
 * @brief       Read data from radio RAM.
 *
 * @param       ramAddr     - radio RAM address
 * @param       pReadData   - pointer to read data
 * @param       len         - length of data in bytes
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiSpiReadRam(uint16_t ramAddr, uint8_t *pReadData, uint8_t len)
{
  mrfiSpiIState_t s;
  uint8_t i;

  /* Address out of range */
  MRFI_SPI_ASSERT(ramAddr <= 0xFFF);

  /* Buffer must be valid */
  MRFI_SPI_ASSERT(pReadData != NULL);

  /* Radio must be powered */


  /* Disable interrupts that call SPI functions. */
  MRFI_SPI_ENTER_CRITICAL_SECTION(s);

  /*-------------------------------------------------------------------------------
   *  Turn chip select "off" and then "on" to clear any current SPI access.
   */
  MRFI_SPI_SET_CHIP_SELECT_OFF();
  MRFI_SPI_SET_CHIP_SELECT_ON();

  /* Send MEMRD via SPI */
  MRFI_SPI_WRITE_BYTE(MEMRD | (ramAddr >> 8));
  MRFI_SPI_WAIT_DONE();
  MRFI_SPI_WRITE_BYTE(ramAddr & 0xFF);
  MRFI_SPI_WAIT_DONE();

  /* Get the data */
  for (i=0; i<len; i++)
  {
    /* Write 0 to SPI to get the data */
    MRFI_SPI_WRITE_BYTE(0);
    /* Wait for Done */
    MRFI_SPI_WAIT_DONE();
    /* Fill up the buffer*/
    *pReadData++ = MRFI_SPI_READ_BYTE();
  }

  /*-------------------------------------------------------------------------------
   *  Turn off chip select and re-enable interrupts that use SPI.
   */
  MRFI_SPI_SET_CHIP_SELECT_OFF();
  MRFI_SPI_EXIT_CRITICAL_SECTION(s);
}

/**************************************************************************************************
 * @fn          mrfiSpiWriteTxFifo
 *
 * @brief       Write data to radio FIFO.
 *
 * @param       pData - pointer for storing write data
 * @param       len   - length of data in bytes
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiSpiWriteTxFifo(uint8_t * pData, uint8_t len)
{
  spiFifoAccess(pData, len, FIFO_ACCESS_TX_WRITE);
}

/**************************************************************************************************
 * @fn          mrfiSpiReadRxFifo
 *
 * @brief       Read data from radio FIFO.
 *
 * @param       pData - pointer for storing read data
 * @param       len   - length of data in bytes
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiSpiReadRxFifo(uint8_t * pData, uint8_t len)
{
  spiFifoAccess(pData, len, FIFO_ACCESS_RX_READ);
}

/*=================================================================================================
 * @fn          spiFifoAccess
 *
 * @brief       Read/write data to or from radio FIFO.
 *
 * @param       pData - pointer to data to read or write
 * @param       len   - length of data in bytes
 *
 * @return      none
 *=================================================================================================
 */
static void spiFifoAccess(uint8_t * pData, uint8_t len, uint8_t accessType)
{
  mrfiSpiIState_t s;


  MRFI_SPI_ASSERT(len != 0); /* zero length is not allowed */

  /*-------------------------------------------------------------------------------
   *  Disable interrupts that call SPI functions.
   */
  MRFI_SPI_ENTER_CRITICAL_SECTION(s);

  /*-------------------------------------------------------------------------------
   *  Turn chip select "off" and then "on" to clear any current SPI access.
   */
  MRFI_SPI_SET_CHIP_SELECT_OFF();
  MRFI_SPI_SET_CHIP_SELECT_ON();

  /*-------------------------------------------------------------------------------
   *  Main loop.  If the SPI access is interrupted, execution comes back to
   *  the start of this loop.  Loop exits when nothing left to transfer.
   *  (Note: previous test guarantees at least one byte to transfer.)
   */
  do
  {
    /*-------------------------------------------------------------------------------
     *  Send FIFO access command byte.  Wait for SPI access to complete.
     */
    if (accessType == FIFO_ACCESS_TX_WRITE)
    {
      MRFI_SPI_WRITE_BYTE( TXBUF );
    }
    else
    {
      MRFI_SPI_WRITE_BYTE( RXBUF );
    }
    MRFI_SPI_WAIT_DONE();


    /*-------------------------------------------------------------------------------
     *  Inner loop.  This loop executes as long as the SPI access is not interrupted.
     *  Loop completes when nothing left to transfer.
     *  (Note: previous test guarantees at least one byte to transfer.)
     */
    do
    {
      MRFI_SPI_WRITE_BYTE(*pData);

      /*-------------------------------------------------------------------------------
       *  Use idle time.  Perform increment/decrement operations before pending on
       *  completion of SPI access.
       *
       *  Decrement the length counter.  Wait for SPI access to complete.
       */
      len--;
      MRFI_SPI_WAIT_DONE();

      /*-------------------------------------------------------------------------------
       *  SPI data register holds data just read, store the value
       *  into memory.
       */
      if (accessType != FIFO_ACCESS_TX_WRITE)
      {
        *pData = MRFI_SPI_READ_BYTE();
      }

      /*-------------------------------------------------------------------------------
       *  At least one byte of data has transferred.  Briefly enable (and then disable)
       *  interrupts that call SPI functions.  This provides a window for any timing
       *  critical interrupts that might be pending.
       *
       *  To improve latency, take care of pointer increment within the interrupt
       *  enabled window.
       */
      MRFI_SPI_EXIT_CRITICAL_SECTION(s);
      pData++;
      MRFI_SPI_ENTER_CRITICAL_SECTION(s);

      /*-------------------------------------------------------------------------------
       *  If chip select is "off" the SPI access was interrupted.  In this case,
       *  turn back on chip select and break to the main loop.  The main loop will
       *  pick up where the access was interrupted.
       */
      if (MRFI_SPI_CHIP_SELECT_IS_OFF())
      {
        MRFI_SPI_SET_CHIP_SELECT_ON();
        break;
      }

    /*-------------------------------------------------------------------------------
     */
    } while (len); /* inner loop */
  } while (len);   /* main loop */

  /*-------------------------------------------------------------------------------
   *  Turn off chip select and re-enable interrupts that use SPI.
   */
  MRFI_SPI_SET_CHIP_SELECT_OFF();
  MRFI_SPI_EXIT_CRITICAL_SECTION(s);
}

/**************************************************************************************************
 * @fn          mrfiSpiRandomByte
 *
 * @brief       returns a random byte
 *
 * @param       none
 *
 * @return      a random byte
 **************************************************************************************************
 */
uint8_t mrfiSpiRandomByte(void)
{
  uint8_t sendBytes[NUM_BYTES_RANDOM];

  /* setup to send RANDOM command to radio*/
  sendBytes[0] = RANDOM;

  /*
   *  Send RANDOM command via SPI. Note that valid return value needs
   *  two dummy writes (i.e. NUM_BYTES_RANDOM equals 3). */
  return( spiSendBytes(&sendBytes[0], NUM_BYTES_RANDOM) );
}

/**************************************************************************************************
 * @fn          mrfiSpiBitClear
 *
 * @brief       Clear the specified bit of the register.
 *              Only one bit can be cleared at a time.
 *
 * @param       addr - register address
 *              bit  - bit to be cleared. This is not a bit mask.
 *
 * @return      radio status byte
 **************************************************************************************************
 */
uint8_t mrfiSpiBitClear(uint8_t addr, uint8_t bit)
{
  uint8_t sendBytes[SPI_ACCESS_BUF_LEN];

  /* Address out of range:
   * Bit access is available to only first few registers.
   */
  MRFI_SPI_ASSERT(addr <= 0x1F);

  /* setup to send Bit Clear command to radio*/
  sendBytes[0] = BCLR;
  sendBytes[1] = (addr << 3) | (bit);

  /* Send BCLR command via SPI. */
  return( spiSendBytes(&sendBytes[0], NUM_BYTES_BIT_SET_CLR) );
}

/**************************************************************************************************
 * @fn          mrfiSpiBitSet
 *
 * @brief       Set the specified bit of the register.
 *              Only one bit can be set at a time.
 *
 * @param       addr - register address
 *              bit  - bit to be set. This is not a bit mask.
 *
 * @return      radio status byte
 **************************************************************************************************
 */
uint8_t mrfiSpiBitSet(uint8_t addr, uint8_t bit)
{
  uint8_t sendBytes[SPI_ACCESS_BUF_LEN];

  /* Address out of range:
   * Bit access is available to only first few registers.
   */
  MRFI_SPI_ASSERT(addr <= 0x1F);

  /* setup to send Bit Clear command to radio*/
  sendBytes[0] = BSET;
  sendBytes[1] = (addr << 3) | (bit);

  /* Send BSET command via SPI. */
  return( spiSendBytes(&sendBytes[0], NUM_BYTES_BIT_SET_CLR) );
}

/**************************************************************************************************
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */
#if (FIFO_ACCESS_TX_WRITE != 0)
#error "ERROR!  Code optimized for FIFO_ACCESS_TX_WRITE equal to zero."
#endif
