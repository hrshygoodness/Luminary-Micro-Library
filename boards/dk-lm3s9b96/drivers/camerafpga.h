//*****************************************************************************
//
// camerafpga.h -  Label definitions relating to the registers offered by the
//                 FPGA on the FPGA/Camera/LCD daughter board.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 9107 of the DK-LM3S9B96 Firmware Package.
//
//*****************************************************************************

#ifndef __CAMERAFPGA_H__
#define __CAMERAFPGA_H__

//*****************************************************************************
//
// Definitions of the addresses of each of the control registers within the
// FPGA (assuming that the EPI address map is configured to use the aperture
// at 0xA0000000).
//
//*****************************************************************************
#define FPGA_BASE_ADDR      0xA0000000

#define FPGA_VERSION_REG    0xA0000000
#define FPGA_SYSCTRL_REG    0xA0000002
#define FPGA_IRQEN_REG      0xA0000004
#define FPGA_IRQSTAT_REG    0xA0000006
#define FPGA_MEMPAGE_REG    0xA0000008
#define FPGA_TPORT_REG      0xA000000A
#define FPGA_LCDSET_REG     0xA0000010
#define FPGA_LCDCLR_REG     0xA0000012
#define FPGA_LCDCMD_REG     0xA0000014
#define FPGA_LCDDATA_REG    0xA0000016
#define FPGA_CHRMKEY_REG    0xA0000022
#define FPGA_VCRM_REG       0xA0000026
#define FPGA_VML_REG        0xA0000030
#define FPGA_VMH_REG        0xA0000032
#define FPGA_VMS_REG        0xA0000034
#define FPGA_LRM_REG        0xA0000036
#define FPGA_LVML_REG       0xA0000040
#define FPGA_LVMH_REG       0xA0000042
#define FPGA_LVMS_REG       0xA0000044
#define FPGA_LGML_REG       0xA0000050
#define FPGA_LGHM_REG       0xA0000052
#define FPGA_LGMS_REG       0xA0000054
#define FPGA_MP1ONC_REG     0xA0000056
#define FPGA_MP1CR_REG      0xA0000058
#define FPGA_MP1CC_REG      0xA000005A
#define FPGA_MP1L_REG       0xA000005C
#define FPGA_MP1H_REG       0xA000005E
#define FPGA_MP1S_REG       0xA0000060
#define FPGA_MPORT1_REG     0xA0000080
#define FPGA_MEMWIN         0xA0000400

//*****************************************************************************
//
// The size of the window into FPGA memory accessed via FPGA_MEMWIN.
//
//*****************************************************************************
#define FPGA_MEMWIN_SIZE    0x400

//*****************************************************************************
//
// Macros allowing arbitrary addresses in FPGA SRAM to be read.
//
//*****************************************************************************
#define FPGAREAD(addr, result)                                                \
{                                                                             \
    HWREGH(FPGA_MEMPAGE_REG) = ((addr) >> 10);                                \
    result = HWREG(FPGA_MEMWIN + ((addr) & (FPGA_MEMWIN_SIZE - 1)));          \
}

#define FPGAREADH(addr, result)                                               \
{                                                                             \
    HWREGH(FPGA_MEMPAGE_REG) = ((addr) >> 10);                                \
    result = HWREGH(FPGA_MEMWIN + ((addr) & (FPGA_MEMWIN_SIZE - 1)));         \
}

//*****************************************************************************
//
// Macros allowing arbitrary addresses in FPGA SRAM to be written.
//
//*****************************************************************************
#define FPGAWRITE(addr, val)                                                  \
{                                                                             \
    HWREGH(FPGA_MEMPAGE_REG) = ((addr) >> 10);                                \
    HWREG(FPGA_MEMWIN + ((addr) & (FPGA_MEMWIN_SIZE - 1))) = (val);           \
}

#define FPGAWRITEH(addr, val)                                                 \
{                                                                             \
    HWREGH(FPGA_MEMPAGE_REG) = ((addr) >> 10);                                \
    HWREGH(FPGA_MEMWIN + ((addr) & (FPGA_MEMWIN_SIZE - 1))) = (val);          \
}

//*****************************************************************************
//
// Macros allowing the current (x, y) position accessed via the memory
// apertures to be set.
//
//*****************************************************************************
#define FPGA_AP1_XY_SET(x, y)                                                 \
{                                                                             \
    HWREGH(FPGA_MP1CR_REG) = (y);                                             \
    HWREGH(FPGA_MP1CC_REG) = (x);                                             \
}

#define FPGA_AP2_XY_SET(x, y)                                                 \
{                                                                             \
    HWREGH(FPGA_MP2CR_REG) = (y);                                             \
    HWREGH(FPGA_MP2CC_REG) = (x);                                             \
}

//*****************************************************************************
//
// Definitions of the bits in the FPGA_SYSCTRL_REG register.
//
//*****************************************************************************
#define FPGA_SYSCTRL_VCEN   0x0001
#define FPGA_SYSCTRL_VDEN   0x0002
#define FPGA_SYSCTRL_GDEN   0x0004
#define FPGA_SYSCTRL_CMKEN  0x0008
#define FPGA_SYSCTRL_VSCALE 0x0010
#define FPGA_SYSCTRL_MPVI1  0x0020
#define FPGA_SYSCTRL_QVGA   0x0040

//*****************************************************************************
//
// Definitions of the bits in the FPGA_IRQEN_REG and FPGA_IRQSTAT_REG registers.
//
//*****************************************************************************
#define FPGA_ISR_VCFSI      0x0001
#define FPGA_ISR_VCFEI      0x0002
#define FPGA_ISR_VRMI       0x0004
#define FPGA_ISR_LTSI       0x0008
#define FPGA_ISR_LTEI       0x0010
#define FPGA_ISR_LRMI       0x0020

//*****************************************************************************
//
// Definitions of the bits in the FPGA_LCDSET_REG and FPGA_LCDCLR_REG registers.
//
//*****************************************************************************
#define LCD_CONTROL_BKLIGHT   0x08
#define LCD_CONTROL_NRESET    0x04
#define LCD_CONTROL_YN        0x02
#define LCD_CONTROL_XN        0x01

//*****************************************************************************
//
// Macros used to extract red, green and blue components from an RGB565 pixel.
//
//*****************************************************************************
#define BLUEFROM565(pix)  (((pix) & 0x001F) << 3)
#define GREENFROM565(pix) (((pix) & 0x07E0) >> 3)
#define REDFROM565(pix)   (((pix) & 0xF100) >> 8)

#endif // __CAMERAFPGA_H__
