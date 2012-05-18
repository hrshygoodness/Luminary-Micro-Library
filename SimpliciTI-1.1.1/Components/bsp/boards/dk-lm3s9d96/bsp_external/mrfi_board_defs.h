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
 *   Target : Texas Instruments DK-LM3S9D96
 *            Stellaris Development Kit with EM Adapter
 *   Radios : CC2500
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
 *                     GDO0 Pin Configuration (on PH0 for MOD1, PG0 for MOD2)
 * ------------------------------------------------------------------------------------------------
 */
#define __mrfi_GDO0_BIT__                     0
#define MRFI_GDO0_BASE (MOD2_CONNECTION ? GPIO_PORTG_BASE : GPIO_PORTH_BASE)
#define MRFI_CONFIG_GDO0_PIN_AS_INPUT()       st( MAP_GPIOPinTypeGPIOInput(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__)); )
#define MRFI_GDO0_PIN_IS_HIGH()               (MAP_GPIOPinRead(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__)))

#define MRFI_GDO0_INT_VECTOR                  (MOD2_CONNECTION ? INT_GPIOG : INT_GPIOH)
#define MRFI_ENABLE_GDO0_INT()                st( MAP_GPIOPinIntEnable(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__)); )
#define MRFI_DISABLE_GDO0_INT()               st( MAP_GPIOPinIntDisable(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__)); )
#define MRFI_GDO0_INT_IS_ENABLED()              ( HWREG(MRFI_GDO0_BASE + GPIO_O_IM) & BV(__mrfi_GDO0_BIT__) )
#define MRFI_CLEAR_GDO0_INT_FLAG()            st( MAP_GPIOPinIntClear(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__)); )
#define MRFI_GDO0_INT_FLAG_IS_SET()             ( MAP_GPIOPinIntStatus(MRFI_GDO0_BASE, false) & BV(__mrfi_GDO0_BIT__) )
#define MRFI_CONFIG_GDO0_RISING_EDGE_INT()    st( MAP_GPIOIntTypeSet(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__), GPIO_RISING_EDGE); )
#define MRFI_CONFIG_GDO0_FALLING_EDGE_INT()   st( MAP_GPIOIntTypeSet(MRFI_GDO0_BASE, BV(__mrfi_GDO0_BIT__), GPIO_FALLING_EDGE); )

/* ------------------------------------------------------------------------------------------------
 *                     GDO2 Pin Configuration (on PH1 for MOD1, PG1    for MOD2)
 * ------------------------------------------------------------------------------------------------
 */
#define __mrfi_GDO2_BIT__                     1
#define MRFI_GDO2_BASE (MOD2_CONNECTION ? GPIO_PORTG_BASE : GPIO_PORTH_BASE)
#define MRFI_CONFIG_GDO2_PIN_AS_INPUT()       st( MAP_GPIOPinTypeGPIOInput(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__)); )
#define MRFI_GDO2_PIN_IS_HIGH()               (MAP_GPIOPinRead(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__)))

#define MRFI_GDO2_INT_VECTOR                  (MOD2_CONNECTION ? INT_GPIOG : INT_GPIOH)
#define MRFI_ENABLE_GDO2_INT()                st( MAP_GPIOPinIntEnable(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__)); )
#define MRFI_DISABLE_GDO2_INT()               st( MAP_GPIOPinIntDisable(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__)); )
#define MRFI_GDO2_INT_IS_ENABLED()             ( HWREG(MRFI_GDO2_BASE + GPIO_O_IM) & BV(__mrfi_GDO2_BIT__) )
#define MRFI_CLEAR_GDO2_INT_FLAG()            st( MAP_GPIOPinIntClear(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__)); )
#define MRFI_GDO2_INT_FLAG_IS_SET()            ( MAP_GPIOPinIntStatus(MRFI_GDO2_BASE, false) & BV(__mrfi_GDO2_BIT__) )
#define MRFI_CONFIG_GDO2_RISING_EDGE_INT()    st( MAP_GPIOIntTypeSet(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__), GPIO_RISING_EDGE); )
#define MRFI_CONFIG_GDO2_FALLING_EDGE_INT()   st( MAP_GPIOIntTypeSet(MRFI_GDO2_BASE, BV(__mrfi_GDO2_BIT__), GPIO_FALLING_EDGE); )

/* ------------------------------------------------------------------------------------------------
 *
 * Additional definitions and macros required for family 3 radios (CC2520)
 *
 * ------------------------------------------------------------------------------------------------
 */
#ifdef MRFI_CC2520

#ifdef MRFI_PA_LNA_ENABLED
extern uint8_t mrfiLnaHighGainMode;
#endif

/* ------------------------------------------------------------------------------------------------
 *                                 VREG_EN Pin Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define MRFI_VREG_EN_GPIO_BIT                0
#define MRFI_VREG_EN_BASE (MOD2_CONNECTION ? GPIO_PORTG_BASE : GPIO_PORTH_BASE)
#define MRFI_DRIVE_VREG_EN_PIN_HIGH()        st( MAP_GPIOPinWrite(MRFI_VREG_EN_BASE, BV(MRFI_VREG_EN_GPIO_BIT), BV(MRFI_VREG_EN_GPIO_BIT)); )
#define MRFI_DRIVE_VREG_EN_PIN_LOW()         st( MAP_GPIOPinWrite(MRFI_VREG_EN_BASE, BV(MRFI_VREG_EN_GPIO_BIT), 0); )
#define MRFI_CONFIG_VREG_EN_PIN_AS_OUTPUT()  st( MAP_GPIOPinTypeGPIOOutput(MRFI_VREG_EN_BASE, BV(MRFI_VREG_EN_GPIO_BIT)); )


/* ------------------------------------------------------------------------------------------------
 *                                  RESETN Pin Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define MRFI_RESETN_GPIO_BIT                 (MOD2_CONNECTION ? 5 : 4)
#define MRFI_RESETN_BASE (MOD2_CONNECTION ? GPIO_PORTH_BASE : GPIO_PORTC_BASE)
#define MRFI_DRIVE_RESETN_PIN_HIGH()         st( MAP_GPIOPinWrite(MRFI_RESETN_BASE, BV(MRFI_RESETN_GPIO_BIT), BV(MRFI_RESETN_GPIO_BIT)); )
#define MRFI_DRIVE_RESETN_PIN_LOW()          st( MAP_GPIOPinWrite(MRFI_RESETN_BASE, BV(MRFI_RESETN_GPIO_BIT), 0); )
#define MRFI_CONFIG_RESETN_PIN_AS_OUTPUT()   st( MAP_GPIOPinTypeGPIOOutput(MRFI_RESETN_BASE, BV(MRFI_RESETN_GPIO_BIT)); )

/*
  GPIO_0 <---- TX_FRM_DONE -----> PJ6 (both EM2 modules)
  GPIO_1 <----    FIFO     -----> PC7 (both EM2 modules)
  GPIO_2 <----    FIFOP    -----> PC6 (both EM2 modules)

#ifdef MRFI_PA_LNA_ENABLED
  GPIO_3 <-----------> HGM
  GPIO_4 <-----------> PA
  GPIO_5 <-----------> PAEN
#else
  GPIO_3 <-----------> PH1 (MOD1) PG1 (MOD2)
  GPIO_4 <-----------> PJ3 (both EM2 modules)
  GPIO_5 <-----------> PC4 (MOD1) PH5 (MOD2) - Shared with RESETn
#endif
*/

#define MRFI_BOARD_CONFIG_RADIO_GPIO() st(                                     \
    /* Program the radio GPIO_0 to output TX_FRM_DONE Exception */             \
    mrfiSpiWriteReg(GPIOCTRL0, 0x02);                                          \
    );

#ifdef MRFI_PA_LNA_ENABLED

#define MRFI_BOARD_PA_LNA_CONFIG_PORTS() st(                                   \
    /* HGM of CC2591 is connected to GPIO_3. Set it to HIGH (HGM) */           \
    mrfiSpiWriteReg(GPIOCTRL3, 0x7F);                                          \
 /* The following settings for LNA_PD/PA_PD are not documented in datasheet.*/ \
    /* EN of CC2591 is connected to GPIO_4. 0x46 == LNA_PD. */                 \
    mrfiSpiWriteReg(GPIOCTRL4, 0x46);                                          \
    /* PAEN of CC2591 is connected to GPIO_5. 0x47 == PA_PD. */                \
    mrfiSpiWriteReg(GPIOCTRL5, 0x47);                                          \
    /* Invert polarity of GPIO 4 and 5 */                                      \
    mrfiSpiWriteReg(GPIOPOLARITY, 0x0F);                                       \
    );

#define  MRFI_BOARD_PA_LNA_LGM()  st( mrfiLnaHighGainMode = 0; mrfiSpiWriteReg(GPIOCTRL3, 0x7E););
#define  MRFI_BOARD_PA_LNA_HGM()  st( mrfiLnaHighGainMode = 1; mrfiSpiWriteReg(GPIOCTRL3, 0x7F););

#endif

/* Port bits */
#define __mrfi_TX_FRM_DONE_BASE  GPIO_PORTJ_BASE
#define __mrfi_TX_FRM_DONE_BIT   6
#define __mrfi_FIFO_BASE         GPIO_PORTC_BASE
#define __mrfi_FIFO_BIT          7
#define __mrfi_FIFOP_BASE        GPIO_PORTC_BASE
#define __mrfi_FIFOP_BIT         6

#define MRFI_FIFOP_INT_VECTOR    INT_GPIOC

/* Port status */
#define MRFI_TX_DONE_STATUS()           (MAP_GPIOPinRead(__mrfi_TX_FRM_DONE_BASE, BV(__mrfi_TX_FRM_DONE_BIT)))
#define MRFI_FIFOP_STATUS()             (MAP_GPIOPinRead(__mrfi_FIFOP_BASE, BV(__mrfi_FIFOP_BIT)))
#define MRFI_FIFO_STATUS()              (MAP_GPIOPinRead(__mrfi_FIFO_BASE, BV(__mrfi_FIFO_BIT)))

/* Port interrupt flags */
#define MRFI_FIFOP_INTERRUPT_FLAG()     (MAP_GPIOPinIntStatus(__mrfi_FIFOP_BASE, false) & BV(__mrfi_FIFOP_BIT)))

/* Port configure macros */
#define MRFI_CONFIG_TX_FRAME_DONE_AS_INPUT()  st( MAP_GPIOPinTypeGPIOInput(__mrfi_TX_FRM_DONE_BASE, BV(__mrfi_TX_FRM_DONE_BIT)); )
#define MRFI_CONFIG_FIFO_AS_INPUT()           st( MAP_GPIOPinTypeGPIOInput(__mrfi_FIFO_BASE, BV(__mrfi_FIFO_BIT)); )
#define MRFI_CONFIG_FIFOP_AS_INPUT()          st( MAP_GPIOPinTypeGPIOInput(__mrfi_FIFOP_BASE, BV(__mrfi_FIFOP_BIT)); )

/* Port control macros */
#define MRFI_ENABLE_RX_INTERRUPT()     st( MAP_GPIOPinIntEnable(__mrfi_FIFOP_BASE, BV(__mrfi_FIFOP_BIT)); )
#define MRFI_DISABLE_RX_INTERRUPT()    st( MAP_GPIOPinIntDisable(__mrfi_FIFOP_BASE, BV(__mrfi_FIFOP_BIT)); )
#define MRFI_CLEAR_RX_INTERRUPT_FLAG() st( MAP_GPIOPinIntClear(__mrfi_FIFOP_BASE, BV(__mrfi_FIFOP_BIT)); )

/* Additional SPI macros not defined elsewhere when using CC2520 */
#define MRFI_SPI_SET_CHIP_SELECT_OFF          MRFI_SPI_DRIVE_CSN_HIGH
#define MRFI_SPI_SET_CHIP_SELECT_ON           MRFI_SPI_DRIVE_CSN_LOW
#define MRFI_SPI_CHIP_SELECT_IS_OFF           MRFI_SPI_CSN_IS_HIGH

#endif

/* ------------------------------------------------------------------------------------------------
 *                                      SPI Configuration
 * ------------------------------------------------------------------------------------------------
 */

/* Chip selects for each of the 2 modules. */
#define MOD1_SPI_CSN_BASE                      GPIO_PORTE_BASE
#define MOD1_SPI_CSN_BIT                       1
#define MOD2_SPI_CSN_BASE                      GPIO_PORTJ_BASE
#define MOD2_SPI_CSN_BIT                       4

/* CSn Pin Configuration (PE1 and PJ4 for MOD1 and MOD2 respectively) */
#define ACTIVE_CSN_BIT                        (MOD2_CONNECTION ? MOD2_SPI_CSN_BIT : MOD1_SPI_CSN_BIT)
#define ACTIVE_CSN_BASE                       (MOD2_CONNECTION ? MOD2_SPI_CSN_BASE : MOD1_SPI_CSN_BASE)
#define INACTIVE_CSN_BIT                      (MOD2_CONNECTION ? MOD1_SPI_CSN_BIT : MOD2_SPI_CSN_BIT)
#define INACTIVE_CSN_BASE                     (MOD2_CONNECTION ? MOD1_SPI_CSN_BASE : MOD2_SPI_CSN_BASE)

/* Note - for safely, when we drive CSn low, we ensure that the inactive
 * module's CSn line is driven high.  This prevents both modules being
 * selected at once.
 */
#define MRFI_SPI_CONFIG_CSN_PIN_AS_OUTPUT()   st( MAP_GPIOPinTypeGPIOOutput(MOD1_SPI_CSN_BASE, BV(MOD1_SPI_CSN_BIT)); \
                                                  MAP_GPIOPinTypeGPIOOutput(MOD2_SPI_CSN_BASE, BV(MOD2_SPI_CSN_BIT)); )
#define MRFI_SPI_DRIVE_CSN_HIGH()             st( MAP_GPIOPinWrite(ACTIVE_CSN_BASE, BV(ACTIVE_CSN_BIT), BV(ACTIVE_CSN_BIT)); )
#define MRFI_SPI_DRIVE_CSN_LOW()              st(MAP_GPIOPinWrite(INACTIVE_CSN_BASE, BV(INACTIVE_CSN_BIT),BV(INACTIVE_CSN_BIT)); \
                                                 MAP_GPIOPinWrite(ACTIVE_CSN_BASE, BV(ACTIVE_CSN_BIT), 0); )
#define MRFI_SPI_CSN_IS_HIGH()                (MAP_GPIOPinRead(ACTIVE_CSN_BASE, BV(ACTIVE_CSN_BIT)))

/* SCLK Pin Configuration (PH4) */
#define __mrfi_SPI_SCLK_GPIO_BIT__            4
#define MRFI_SPI_CONFIG_SCLK_PIN_AS_OUTPUT()  st( MAP_GPIOPinTypeSSI(GPIO_PORTH_BASE, BV(__mrfi_SPI_SCLK_GPIO_BIT__)); )
#define MRFI_SPI_DRIVE_SCLK_HIGH()            /* Not used. */
#define MRFI_SPI_DRIVE_SCLK_LOW()             /* Not used. */

/* SI Pin Configuration (PF5) */
#define __mrfi_SPI_SI_GPIO_BIT__              5
#define MRFI_SPI_CONFIG_SI_PIN_AS_OUTPUT()    st( MAP_GPIOPinTypeSSI(GPIO_PORTF_BASE, BV(__mrfi_SPI_SI_GPIO_BIT__)); )
#define MRFI_SPI_DRIVE_SI_HIGH()              /* Not used. */
#define MRFI_SPI_DRIVE_SI_LOW()               /* Not used. */

/* SO Pin Configuration (PF4) */
#define __mrfi_SPI_SO_GPIO_BIT__              4
#define MRFI_SPI_CONFIG_SO_PIN_AS_INPUT()     st( MAP_GPIOPinTypeSSI(GPIO_PORTF_BASE, BV(__mrfi_SPI_SO_GPIO_BIT__)); )
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
#define SPI_RATE 1000000
#ifdef MRFI_CC2520
#define MRFI_SPI_INIT()                                                       \
st (                                                                          \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);                             \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);                            \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);                            \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);                            \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);                            \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);                            \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);                            \
  MAP_GPIOPinConfigure(GPIO_PH4_SSI1CLK);                                     \
  MAP_GPIOPinConfigure(GPIO_PF5_SSI1TX);                                      \
  MAP_GPIOPinConfigure(GPIO_PF4_SSI1RX);                                      \
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
  MAP_GPIOPinIntDisable(__mrfi_FIFOP_BASE, BV(__mrfi_FIFOP_BIT));             \
  MAP_GPIOIntTypeSet(__mrfi_FIFOP_BASE, BV(__mrfi_FIFOP_BIT),                 \
                     GPIO_RISING_EDGE);                                       \
  MAP_GPIOPinIntClear(__mrfi_FIFOP_BASE, BV(__mrfi_FIFOP_BIT));               \
  MAP_IntEnable(MRFI_FIFOP_INT_VECTOR);                                       \
  MAP_IntMasterEnable();                                                      \
)
#else
#define MRFI_SPI_INIT()                                                       \
st (                                                                          \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);                             \
  MAP_SysCtlPeripheralEnable((MOD2_CONNECTION ?                               \
                              SYSCTL_PERIPH_GPIOG: SYSCTL_PERIPH_GPIOH));     \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);                            \
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);                            \
  MAP_GPIOPinConfigure(GPIO_PH4_SSI1CLK);                                     \
  MAP_GPIOPinConfigure(GPIO_PF5_SSI1TX);                                      \
  MAP_GPIOPinConfigure(GPIO_PF4_SSI1RX);                                      \
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
#endif

#define MRFI_SPI_IS_INITIALIZED()  (HWREG(SSI1_BASE + SSI_O_CR1) & SSI_CR1_SSE)

/**************************************************************************************************
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */
#ifndef BSP_BOARD_DK_LM3S9D96
#error "ERROR: Mismatch between specified board and MRFI configuration."
#endif


/**************************************************************************************************
 */
#endif
