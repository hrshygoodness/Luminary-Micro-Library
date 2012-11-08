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
 *   Radios: CC2500, CC1100, CC1101
 *   SPI interface code.
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "bsp_external/mrfi_board_defs.h"
#include "mrfi_spi.h"


/* ------------------------------------------------------------------------------------------------
 *                                            Defines
 * ------------------------------------------------------------------------------------------------
 */
#define DUMMY_BYTE                  0xDB

#define READ_BIT                    0x80
#define BURST_BIT                   0x40


/* ------------------------------------------------------------------------------------------------
 *                                            Macros
 * ------------------------------------------------------------------------------------------------
 */
#define MRFI_SPI_TURN_CHIP_SELECT_ON()        MRFI_SPI_DRIVE_CSN_LOW()
#define MRFI_SPI_TURN_CHIP_SELECT_OFF()       MRFI_SPI_DRIVE_CSN_HIGH()
#define MRFI_SPI_CHIP_SELECT_IS_OFF()         MRFI_SPI_CSN_IS_HIGH()

#define MRFI_SPI_DEBUG
#ifdef MRFI_SPI_DEBUG
#define MRFI_SPI_ASSERT(x)      BSP_ASSERT(x)
#else
#define MRFI_SPI_ASSERT(x)
#endif


/* ------------------------------------------------------------------------------------------------
 *                                       Local Prototypes
 * ------------------------------------------------------------------------------------------------
 */
static uint8_t spiRegAccess(uint8_t addrByte, uint8_t writeValue);
static void spiBurstFifoAccess(uint8_t addrByte, uint8_t * pData, uint8_t len);


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
  MRFI_SPI_DRIVE_CSN_HIGH();
  
  /* initialize the SPI registers */
  MRFI_SPI_INIT();
} 


/**************************************************************************************************
 * @fn          mrfiSpiCmdStrobe
 *
 * @brief       Send command strobe to the radio.  Returns status byte read during transfer
 *              of strobe command.
 *
 * @param       addr - address of register to strobe
 *
 * @return      status byte of radio
 **************************************************************************************************
 */
uint8_t mrfiSpiCmdStrobe(uint8_t addr)
{
  uint8_t statusByte;
  mrfiSpiIState_t s;

  MRFI_SPI_ASSERT( MRFI_SPI_IS_INITIALIZED() );       /* SPI is not initialized */
  MRFI_SPI_ASSERT((addr >= 0x30) && (addr <= 0x3D));  /* invalid address */

  /* disable interrupts that use SPI */
  MRFI_SPI_ENTER_CRITICAL_SECTION(s);

  /* turn chip select "off" and then "on" to clear any current SPI access */
  MRFI_SPI_TURN_CHIP_SELECT_OFF();
  MRFI_SPI_TURN_CHIP_SELECT_ON();

  /* send the command strobe, wait for SPI access to complete */
  MRFI_SPI_WRITE_BYTE(addr);
  MRFI_SPI_WAIT_DONE();

  /* read the readio status byte returned by the command strobe */
  statusByte = MRFI_SPI_READ_BYTE();

  /* turn off chip select; enable interrupts that call SPI functions */
  MRFI_SPI_TURN_CHIP_SELECT_OFF();
  MRFI_SPI_EXIT_CRITICAL_SECTION(s);

  /* return the status byte */
  return(statusByte);
}


/**************************************************************************************************
 * @fn          mrfiSpiReadReg
 *
 * @brief       Read value from radio register.
 *
 * @param       addr - address of register
 *
 * @return      register value
 **************************************************************************************************
 */
uint8_t mrfiSpiReadReg(uint8_t addr)
{
  MRFI_SPI_ASSERT(addr <= 0x3B);    /* invalid address */
  
  /*
   *  The burst bit is set to allow access to read-only status registers.
   *  This does not affect normal register reads.
   */
  return( spiRegAccess(addr | BURST_BIT | READ_BIT, DUMMY_BYTE) );
}


/**************************************************************************************************
 * @fn          mrfiSpiWriteReg
 *
 * @brief       Write value to radio register.
 *
 * @param       addr  - address of register
 * @param       value - register value to write
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiSpiWriteReg(uint8_t addr, uint8_t value)
{
  MRFI_SPI_ASSERT((addr <= 0x2E) || (addr == 0x3E));    /* invalid address */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
  spiRegAccess(addr, value);
}


/*=================================================================================================
 * @fn          spiRegAccess
 *
 * @brief       This function performs a read or write.  The
 *              calling code must configure the read/write bit of the register's address byte.
 *              This bit is set or cleared based on the type of access.
 *
 * @param       regAddrByte - address byte of register; the read/write bit already configured
 *
 * @return      register value
 *=================================================================================================
 */
static uint8_t spiRegAccess(uint8_t addrByte, uint8_t writeValue)
{
  uint8_t readValue;
  mrfiSpiIState_t s;

  MRFI_SPI_ASSERT( MRFI_SPI_IS_INITIALIZED() );   /* SPI is not initialized */

  /* disable interrupts that use SPI */
  MRFI_SPI_ENTER_CRITICAL_SECTION(s);

  /* turn chip select "off" and then "on" to clear any current SPI access */
  MRFI_SPI_TURN_CHIP_SELECT_OFF();
  MRFI_SPI_TURN_CHIP_SELECT_ON();

  /* send register address byte, the read/write bit is already configured */
  MRFI_SPI_WRITE_BYTE(addrByte);
  MRFI_SPI_WAIT_DONE();

  /*
   *  Send the byte value to write.  If this operation is a read, this value
   *  is not used and is just dummy data.  Wait for SPI access to complete.
   */
  MRFI_SPI_WRITE_BYTE(writeValue);
  MRFI_SPI_WAIT_DONE();

  /*
   *  If this is a read operation, SPI data register now contains the register
   *  value which will be returned.  For a read operation, it contains junk info
   *  that is not used.
   */
  readValue = MRFI_SPI_READ_BYTE();

  /* turn off chip select; enable interrupts that call SPI functions */
  MRFI_SPI_TURN_CHIP_SELECT_OFF();
  MRFI_SPI_EXIT_CRITICAL_SECTION(s);

  /* return the register value */
  return(readValue);
}


/**************************************************************************************************
 * @fn          mrfiSpiWriteTxFifo
 *
 * @brief       Write data to radio transmit FIFO.
 *
 * @param       pData - pointer for storing write data
 * @param       len   - length of data in bytes
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiSpiWriteTxFifo(uint8_t * pData, uint8_t len)
{
  spiBurstFifoAccess(TXFIFO | BURST_BIT, pData, len);
}


/**************************************************************************************************
 * @fn          macSpiReadRxFifo
 *
 * @brief       Read data from radio receive FIFO.
 *
 * @param       pData - pointer for storing read data
 * @param       len   - length of data in bytes
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiSpiReadRxFifo(uint8_t * pData, uint8_t len)
{
  spiBurstFifoAccess(RXFIFO | BURST_BIT | READ_BIT, pData, len);
}


/*=================================================================================================
 * @fn          spiBurstFifoAccess
 *
 * @brief       Burst mode access used for reading or writing to radio FIFOs.
 *
 *              For more efficient interrupt latency, this function does not keep interrupts
 *              disabled for its entire execution.  It is designed to recover if an interrupt
 *              occurs that accesses SPI.  See comments in code for further details.
 *
 * @param       addrByte - first byte written to SPI, contains address and mode bits
 * @param       pData    - pointer to data to read or write
 * @param       len      - length of data in bytes
 *
 * @return      none
 *=================================================================================================
 */
static void spiBurstFifoAccess(uint8_t addrByte, uint8_t * pData, uint8_t len)
{
  mrfiSpiIState_t s;

  MRFI_SPI_ASSERT( MRFI_SPI_IS_INITIALIZED() );   /* SPI is not initialized */
  MRFI_SPI_ASSERT(len != 0);                      /* zero length is not allowed */
  MRFI_SPI_ASSERT(addrByte & BURST_BIT);          /* only burst mode supported */

  /* disable interrupts that use SPI */
  MRFI_SPI_ENTER_CRITICAL_SECTION(s);

  /* turn chip select "off" and then "on" to clear any current SPI access */
  MRFI_SPI_TURN_CHIP_SELECT_OFF();
  MRFI_SPI_TURN_CHIP_SELECT_ON();

  /*-------------------------------------------------------------------------------
   *  Main loop.  If the SPI access is interrupted, execution comes back to
   *  the start of this loop.  Loop exits when nothing left to transfer.
   */
  do
  {
    /* send FIFO access command byte, wait for SPI access to complete */
    MRFI_SPI_WRITE_BYTE(addrByte);
    MRFI_SPI_WAIT_DONE();

    /*-------------------------------------------------------------------------------
     *  Inner loop.  This loop executes as long as the SPI access is not interrupted.
     *  Loop completes when nothing left to transfer.
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
       *  SPI data register holds data just read.  If this is a read operation,
       *  store the value into memory.
       */
      if (addrByte & READ_BIT)
      {
        *pData = MRFI_SPI_READ_BYTE();
      }

      /*-------------------------------------------------------------------------------
       *  At least one byte of data has transferred.  Briefly enable (and then disable)
       *  interrupts that can call SPI functions.  This provides a window for any timing
       *  critical interrupts that might be pending.
       *
       *  To improve latency, take care of pointer increment within the interrupt
       *  enabled window.
       */
      MRFI_SPI_EXIT_CRITICAL_SECTION(s);
      pData++;
      MRFI_SPI_ENTER_CRITICAL_SECTION(s);

      /*-------------------------------------------------------------------------------
       *  If chip select is "off" the SPI access was interrupted (all SPI access
       *  functions leave chip select in the "off" state).  In this case, turn
       *  back on chip select and break to the main loop.  The main loop will
       *  pick up where the access was interrupted.
       */
      if (MRFI_SPI_CHIP_SELECT_IS_OFF())
      {
        MRFI_SPI_TURN_CHIP_SELECT_ON();
        break;
      }

    /*-------------------------------------------------------------------------------
     */
    } while (len); /* inner loop */
  } while (len);   /* main loop */

  /* turn off chip select; enable interrupts that call SPI functions */
  MRFI_SPI_TURN_CHIP_SELECT_OFF();
  MRFI_SPI_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
*/
