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

/* ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 *   MRFI (Minimal RF Interface)
 *   Board definition file.
 *   Target : Texas Instruments EK-EvalBot
 *            Stellaris Development Kit
 *   Radios : CC21101, CC2500
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

#ifndef MRFI_BOARD_DEFS_H
#define MRFI_BOARD_DEFS_H

/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "bsp.h"
#include "driverlib/interrupt.h"
#include "inc/hw_ints.h"
#include "inc/hw_ssi.h"
#include "inc/hw_gpio.h"

/* ------------------------------------------------------------------------------------------------
 *                                Global Storing Last SPI Read Result
 * ------------------------------------------------------------------------------------------------
 */

/*
 * The common SPI implementation in the radio code assumes a non-FIFOed SPI
 * controller so we need to model this in the Stellaris implementation.  To
 * do this, every byte written is followed by a SPI read into this variable
 * and the value in this variable is returned any time a SPI read is requested.
 * This is a bit unpleasant but allows us to pick up support for all the
 * existing radio families without having to product Stellaris-specific
 * code for each one.
 */
extern unsigned char g_ucSPIReadVal;

/* ------------------------------------------------------------------------------------------------
 *                                           Defines
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                     GDO0 Pin Configuration (on PH2)
 * ------------------------------------------------------------------------------------------------
 */
#define __mrfi_GDO0_BIT__                     2
#define MRFI_GDO0_BASE                        GPIO_PORTH_BASE
#define MRFI_CONFIG_GDO0_PIN_AS_INPUT()       st( MAP_GPIOPinTypeGPIOInput(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__)); )
#define MRFI_GDO0_PIN_IS_HIGH()               (MAP_GPIOPinRead(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__)))

#define MRFI_GDO0_INT_VECTOR                  INT_GPIOH
#define MRFI_ENABLE_GDO0_INT()                st( MAP_GPIOPinIntEnable(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__)); )
#define MRFI_DISABLE_GDO0_INT()               st( MAP_GPIOPinIntDisable(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__)); )
#define MRFI_GDO0_INT_IS_ENABLED()              ( HWREG(MRFI_GDO0_BASE + GPIO_O_IM) & BV(__mrfi_GDO0_BIT__) )
#define MRFI_CLEAR_GDO0_INT_FLAG()            st( MAP_GPIOPinIntClear(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__)); )
#define MRFI_GDO0_INT_FLAG_IS_SET()             ( MAP_GPIOPinIntStatus(MRFI_GDO0_BASE, false) & BV(__mrfi_GDO0_BIT__) )
#define MRFI_CONFIG_GDO0_RISING_EDGE_INT()    st( MAP_GPIOIntTypeSet(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__), GPIO_RISING_EDGE); )
#define MRFI_CONFIG_GDO0_FALLING_EDGE_INT()   st( MAP_GPIOIntTypeSet(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__), GPIO_FALLING_EDGE); )

/* ------------------------------------------------------------------------------------------------
 *                     GDO2 Pin Configuration (on PH3)
 * ------------------------------------------------------------------------------------------------
 */
#define __mrfi_GDO2_BIT__                     3
#define MRFI_GDO2_BASE                        GPIO_PORTH_BASE
#define MRFI_CONFIG_GDO2_PIN_AS_INPUT()       st( MAP_GPIOPinTypeGPIOInput(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__)); )
#define MRFI_GDO2_PIN_IS_HIGH()               (MAP_GPIOPinRead(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__)))

#define MRFI_GDO2_INT_VECTOR                  INT_GPIOH
#define MRFI_ENABLE_GDO2_INT()                st( MAP_GPIOPinIntEnable(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__)); )
#define MRFI_DISABLE_GDO2_INT()               st( MAP_GPIOPinIntDisable(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__)); )
#define MRFI_GDO2_INT_IS_ENABLED()             ( HWREG(MRFI_GDO2_BASE + GPIO_O_IM) & BV(__mrfi_GDO2_BIT__) )
#define MRFI_CLEAR_GDO2_INT_FLAG()            st( MAP_GPIOPinIntClear(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__)); )
#define MRFI_GDO2_INT_FLAG_IS_SET()            ( MAP_GPIOPinIntStatus(MRFI_GDO2_BASE, false) & BV(__mrfi_GDO2_BIT__) )
#define MRFI_CONFIG_GDO2_RISING_EDGE_INT()    st( MAP_GPIOIntTypeSet(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__), GPIO_RISING_EDGE); )
#define MRFI_CONFIG_GDO2_FALLING_EDGE_INT()   st( MAP_GPIOIntTypeSet(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__), GPIO_FALLING_EDGE); )

/* ------------------------------------------------------------------------------------------------
 *                                      SPI Configuration
 * ------------------------------------------------------------------------------------------------
 */

/* Chip select pin definition */
#define MOD_SPI_CSN_BASE                      GPIO_PORTH_BASE
#define MOD_SPI_CSN_BIT                       5

#define MRFI_SPI_CONFIG_CSN_PIN_AS_OUTPUT()   st( MAP_GPIOPinTypeGPIOOutput(MOD_SPI_CSN_BASE, BV(MOD_SPI_CSN_BIT)); )
#define MRFI_SPI_DRIVE_CSN_HIGH()             st( MAP_GPIOPinWrite(MOD_SPI_CSN_BASE, BV(MOD_SPI_CSN_BIT), BV(MOD_SPI_CSN_BIT)); )
#define MRFI_SPI_DRIVE_CSN_LOW()              st(MAP_GPIOPinWrite(MOD_SPI_CSN_BASE, BV(MOD_SPI_CSN_BIT), 0); )
#define MRFI_SPI_CSN_IS_HIGH()                (MAP_GPIOPinRead(MOD_SPI_CSN_BASE, BV(MOD_SPI_CSN_BIT)))

/* SCLK Pin Configuration (PH4) */
#define __mrfi_SPI_SCLK_GPIO_BIT__            4
#define MRFI_SPI_CONFIG_SCLK_PIN_AS_OUTPUT()  st( MAP_GPIOPinTypeSSI(GPIO_PORTH_BASE, BV(__mrfi_SPI_SCLK_GPIO_BIT__)); )
#define MRFI_SPI_DRIVE_SCLK_HIGH()            /* Not used. */
#define MRFI_SPI_DRIVE_SCLK_LOW()             /* Not used. */

/* SI Pin Configuration (PH7) */
#define __mrfi_SPI_SI_GPIO_BIT__              7
#define MRFI_SPI_CONFIG_SI_PIN_AS_OUTPUT()    st( MAP_GPIOPinTypeSSI(GPIO_PORTH_BASE, BV(__mrfi_SPI_SI_GPIO_BIT__)); )
#define MRFI_SPI_DRIVE_SI_HIGH()              /* Not used. */
#define MRFI_SPI_DRIVE_SI_LOW()               /* Not used. */

/* SO Pin Configuration (PH6) */
#define __mrfi_SPI_SO_GPIO_BIT__              6
#define MRFI_SPI_CONFIG_SO_PIN_AS_INPUT()     st( MAP_GPIOPinTypeSSI(GPIO_PORTH_BASE, BV(__mrfi_SPI_SO_GPIO_BIT__)); )
#define MRFI_SPI_SO_IS_HIGH()                 (MAP_GPIOPinRead(GPIO_PORTF_BASE, BV(__mrfi_SPI_SO_GPIO_BIT__)))

/* SPI Port Configuration */
#define MRFI_SPI_CONFIG_PORT()                /* Not used. */

/* read/write macros */
#define MRFI_SPI_WRITE_BYTE(x)                st( MAP_SSIDataPut(SSI1_BASE, (x));                       \
                                                  {                                                     \
                                                      unsigned long ulTemp;                             \
                                                      MAP_SSIDataGet(SSI1_BASE, &ulTemp);               \
                                                      g_ucSPIReadVal = (unsigned char)(ulTemp & 0xFF);  \
                                                  }                                                     \
                                              )
#define MRFI_SPI_READ_BYTE()                  g_ucSPIReadVal
#define MRFI_SPI_WAIT_DONE()                  while(MAP_SSIBusy(SSI1_BASE));

/* SPI critical section macros */
typedef bspIState_t mrfiSpiIState_t;
#define MRFI_SPI_ENTER_CRITICAL_SECTION(x)    BSP_ENTER_CRITICAL_SECTION(x)
#define MRFI_SPI_EXIT_CRITICAL_SECTION(x)     BSP_EXIT_CRITICAL_SECTION(x)


/*
 *  Radio SPI Specifications
 * -----------------------------------------------
 *    Max SPI Clock   :  1 MHz
 *    Data Order      :  MSB transmitted first
 *    Clock Polarity  :  low when idle
 *    Clock Phase     :  sample leading edge
 */

/* Initialization macro  Note that configuration of individual pins is done
 * elsewhere. This macro is responsible for peripheral-level configuration
 * and pin muxing.  It also enables the SSI peripheral and flushes the receive
 * FIFO just in case it has anything in it. Additionally, since there's no
 * other obvious place to put it, we enable the interrupt for the GPIO that
 * the radio uses to signal reception of data here too.
 */
#define SPI_RATE 100000
#define MRFI_SPI_INIT()                                                       \
st (                                                                          \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);                             \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);                            \
  MAP_GPIOPinConfigure(GPIO_PH4_SSI1CLK);                                     \
  MAP_GPIOPinConfigure(GPIO_PH7_SSI1TX);                                      \
  MAP_GPIOPinConfigure(GPIO_PH6_SSI1RX);                                      \
  MAP_SSIConfigSetExpClk(SSI1_BASE, MAP_SysCtlClockGet(),                     \
                         SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, SPI_RATE, 8);  \
  MAP_SSIEnable(SSI1_BASE);                                                   \
  {                                                                           \
      unsigned long ulTemp;                                                   \
      while(MAP_SSIDataGetNonBlocking(SSI1_BASE, &ulTemp))                    \
      {                                                                       \
      }                                                                       \
  }                                                                           \
                                                                              \
  MAP_GPIOPinIntDisable(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__));               \
  MAP_GPIOPinIntClear(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__));                 \
  MAP_IntEnable(MRFI_GDO0_INT_VECTOR);                                        \
  MAP_IntMasterEnable();                                                      \
)

#define MRFI_SPI_IS_INITIALIZED()  (HWREG(SSI1_BASE + SSI_O_CR1) & SSI_CR1_SSE)

/**************************************************************************************************
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */
#ifndef BSP_BOARD_EK_EVALBOT
#error "ERROR: Mismatch between specified board and MRFI configuration."
#endif
#ifdef MRFI_CC2520
#error "ERROR: The EK-EvalBot EM connector does not support CC2520 - sorry!"
#endif


/**************************************************************************************************
 */
#endif
