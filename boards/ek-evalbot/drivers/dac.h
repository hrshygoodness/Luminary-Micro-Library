//*****************************************************************************
//
// dac.h - Prototypes and definitions for supporting the TLV320AIC3107 audio
//         DAC on EVALBOT.
//
// Copyright (c) 2011-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the Stellaris Firmware Development Package.
//
//*****************************************************************************
#ifndef __DAC_H__
#define __DAC_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//  TLV320AIC3107 Page 0 Register offsets.
//
//*****************************************************************************
#define TI_PAGE_SELECT_R            0   // Page Select Reg
#define TI_SOFTWARE_RESET_R         1   // Software Reset Reg
#define TI_CODEC_SAMPLE_RATE_R      2   // Code Sample Rate Select Reg
#define TI_PLL_PROG_A_R             3   // PLL Programming Reg A
#define TI_PLL_PROG_B_R             4   // PLL Programming Reg B
#define TI_PLL_PROG_C_R             5   // PLL Programming Reg C
#define TI_PLL_PROG_D_R             6   // PLL Programming Reg D
#define TI_CODEC_DATAPATH_R         7   // Codec Datapath Setup Reg
#define TI_ASDI_CTL_A_R             8   // Audio Serial Data Interf Ctrl Reg A
#define TI_ASDI_CTL_B_R             9   // Audio Serial Data Interf Ctrl Reg B
#define TI_ASDI_CTL_C_R             10  // Audio Serial Data Interf Ctrl Reg C
#define TI_ACO_FLAG_R               11  // Audio Codec Overflow Flag Reg
#define TI_ACDF_CTL_R               12  // Audio Codec Digital Filter Ctrl Reg
#define TI_HBPD_A_R                 13  // Headset / Button Press Detect Reg A
#define TI_HBPD_B_R                 14  // Headset / Button Press Detect Reg B
#define TI_LEFT_ADC_PGA_GAIN_CTL_R  15  // Left ADC PGA Gain Control Reg
#define TI_RIGHT_ADC_PGA_GAIN_CTL_R 16  // Right ADC PGA Gain Control Reg
#define TI_MIC3LR_LEFT_CTL_R        17  // MIC3L/R to Left ADC Control Reg
#define TI_MIC3LR_RIGHT_CTL_R       18  // MIC3L/R to Right ADC Control Reg
#define TI_LINE1L_LEFT_ADC_CTL_R    19  // LINE1L to Left ADC Control Reg
#define TI_LINE2L_LEFT_ADC_CTL_R    20  // LINE2L to Left ADC Control Reg
#define TI_LINE1R_LEFT_ADC_CTL_R    21  // LINE1R to Left ADC Control Reg
#define TI_LINE1R_RIGHT_ADC_CTL_R   22  // LINE1R to Right ADC Control Reg
#define TI_LINE2R_RIGHT_ADC_CTL_R   23  // LINE2R to Right ADC Control Reg
#define TI_LINE1L_RIGHT_ADC_CTL_R   24  // LINE1L to Right ADC Control Reg
#define TI_MICBIAS_CTL_R            25  // MICBIAS Control Reg
#define TI_LEFT_AGC_CTL_A_R         26  // Left AGC Control Reg A
#define TI_LEFT_AGC_CTL_B_R         27  // Left AGC Control Reg B
#define TI_LEFT_AGC_CTL_C_R         28  // Left AGC Control Reg C
#define TI_RIGHT_AGC_CTL_A_R        29  // Right AGC Control Reg A
#define TI_RIGHT_AGC_CTL_B_R        30  // Right AGC Control Reg B
#define TI_RIGHT_AGC_CTL_C_R        31  // Right AGC Control Reg C
#define TI_LEFT_AGC_GAIN_R          32  // Left AGC Gain Reg
#define TI_RIGHT_AGC_GAIN_R         33  // Right AGC Gain Reg
#define TI_LEFT_AGC_NGD_R           34  // Left AGC Noise Gate Debounce Reg
#define TI_RIGHT_AGC_NGD_R          35  // Right AGC Noise Gate Debounce Reg
#define TI_ADC_FLAG_R               36  // ADC Flag Reg
#define TI_DACPOD_CTL_R             37  // DAC Power and Output Driver Ctrl Reg
#define TI_HPOD_CTL_R               38  // High Power Output Driver Ctrl Reg

#define TI_HPOS_CTL_R               40  // High Power Output Stage Control Reg
#define TI_DACOS_CTL_R              41  // DAC Output Switching Control Reg
#define TI_ODPR_R                   42  // Output Driver Pop Reduction Reg
#define TI_LEFT_DAC_DIG_VOL_CTL_R   43  // Left DAC Digital Volume Control Reg
#define TI_RIGHT_DAC_DIG_VOL_CTL_R  44  // Right DAC Digital Volume Control Reg
#define TI_LINE2L_HPLOUT_VOL_CTL_R  45  // LINE2L to HPLOUT Volume Control Reg
#define TI_PGA_L_HPLOUT_VOL_CTL_R   46  // PGA_L to HPLOUT Volume Control Reg
#define TI_DAC_L1_HPLOUT_VOL_CTL_R  47  // DAC_L1 to HPLOUT Volume Control Reg
#define TI_LINE2R_HPLOUT_VOL_CTL_R  48  // LINE2R to HPLOUT Volume Control Reg
#define TI_PGA_R_HPLOUT_VOL_CTL_R   49  // PGA_R to HPLOUT Volume Control Reg
#define TI_DAC_R1_HPLOUT_VOL_CTL_R  50  // DAC_R1 to HPLOUT Volume Control Reg
#define TI_HPLOUT_OUTPUT_LVL_CTL_R  51  // HPLOUT Output Level Control Reg
#define TI_LINE2L_HPCOM_VOL_CTL_R   52  // LINE2L to HPCOM Volume Control Reg
#define TI_PGA_L_HPCOM_VOL_CTL_R    53  // PGA_L to HPCOM Volume Control Reg
#define TI_DAC_L1_HPCOM_VOL_CTL_R   54  // DAC_L1 to HPCOM Volume Control Reg
#define TI_LINE2R_HPCOM_VOL_CTL_R   55  // LINE2R to HPCOM Volume Control Reg
#define TI_PGA_R_HPCOM_VOL_CTL_R    56  // PGA_R to HPCOM Volume Control Reg
#define TI_DAC_R1_HPCOM_VOL_CTL_R   57  // DAC_R1 to HPCOM Volume Control Reg
#define TI_HPCOM_OUTPUT_LVL_CTL_R   58  // HPCOM Output Level Control Reg
#define TI_LINE2L_HPROUT_VOL_CTL_R  59  // LINE2L to HPROUT Volume Control Reg
#define TI_PGA_L_HPROUT_VOL_CTL_R   60  // PGA_L to HPROUT Volume Control Reg
#define TI_DAC_L1_HPROUT_VOL_CTL_R  61  // DAC_L1 to HPROUT Volume Control Reg
#define TI_LINE2R_HPROUT_VOL_CTL_R  62  // LINE2R to HPROUT Volume Control Reg
#define TI_PGA_R_HPROUT_VOL_CTL_R   63  // PGA_R to HPROUT Volume Control Reg
#define TI_DAC_R1_HPROUT_VOL_CTL_R  64  // DAC_R1 to HPROUT Volume Control Reg
#define TI_HPROUT_OUTPUT_LVL_CTL_R  65  // HPROUT Output Level Control Reg

#define TI_CLASSD_BYPASS_SWITCH_CTL_R 73  // Class-D and Bypass Switch Ctrl Reg

#define TI_ADC_DC_DITHER_CTL_R      76    // ADC DC Dither Control Reg

#define TI_LINE2L_LEFT_LOP_VOL_CTL_R  80    // LINE2L to LEFT_LOP Vol Ctrl Reg
#define TI_PGA_L_LEFT_LOP_VOL_CTL_R   81    // PGA_L to LEFT_LOP Vol Ctrl Reg
#define TI_DAC_L1_LEFT_LOP_VOL_CTL_R  82    // DAC_L1 to LEFT_LOP Vol Ctrl Reg
#define TI_LINE2R_LEFT_LOP_VOL_CTL_R  83    // LINE2R to LEFT_LOP Vol Ctrl Reg
#define TI_PGA_R_LEFT_LOP_VOL_CTL_R   84    // PGA_R to LEFT_LOP Vol Ctrl Reg
#define TI_DAC_R1_LEFT_LOP_VOL_CTL_R  85    // DAC_R1 to LEFT_LOP Vol Ctrl Reg
#define TI_LEFT_LOP_OUTPUT_LVL_CTL_R  86    // LEFT_LOP Output Level Ctrl Reg
#define TI_LINE2L_RIGHT_LOP_VOL_CTL_R 87    // LINE2L to RIGHT_LOP Vol Ctrl Reg
#define TI_PGA_L_RIGHT_LOP_VOL_CTL_R  88    // PGA_L to RIGHT_LOP Vol Ctrl Reg
#define TI_DAC_L1_RIGHT_LOP_VOL_CTL_R 89    // DAC_L1 to RIGHT_LOP Vol Ctrl Reg
#define TI_LINE2R_RIGHT_LOP_VOL_CTL_R 90    // LINE2R to RIGHT_LOP Vol Ctrl Reg
#define TI_PGA_R_RIGhT_LOP_VOL_CTL_R  91    // PGA_R to RIGHT_LOP Vol Ctrl Reg
#define TI_DAC_R1_RIGHT_LOP_VOL_CTL_R 92    // DAC_R1 to RIGHT_LOP Vol Ctrl Reg
#define TI_RIGHT_LOP_OUTPUT_LVL_CTL_R 93    // RIGHT_LOP Output Level Ctrl Reg
#define TI_MODULE_PWR_STAT_R        94  // Module Power Status Reg
#define TI_ODSCD_STAT_R             95  // Output Driver Short Circuit Detect
                                        // Status Reg
#define TI_STICKY_INT_FLAGS_R       96  // Sticky Interrupt Flags Reg
#define TI_RT_INT_FLAGS_R           97  // Real-time Interrupt Flags Reg
#define TI_GPIO1_CTL_R              98  // GPIO1 Control Reg

#define TI_CODEC_CLKIN_SRC_SEL_R    101 // CODEC CLKIN Source Selection Reg
#define TI_CLK_GEN_CTL_R            102 // Clock Generation Control Reg
#define TI_LEFT_AGC_ATTACK_TIME_R   103 // Left AGC New Prog Attack Time Reg
#define TI_LEFT_AGC_DECAY_TIME_R    104 // Left AGC Programmable Decay Time Reg
#define TI_RIGHT_AGC_ATTACK_TIME_R  105 // Right AGC Prog Attack Time Reg
#define TI_RIGHT_AGC_DECAY_TIME_R   106 // Right AGC New Prog Decay Time Reg
#define TI_ADC_DP_I2C_COND_R        107 // New Programmable ADC Digital Path
                                        // and I2C Bus Condition Reg
#define TI_PASBSDP_R                108 // Passive Analog Signal Bypass
                                        // Selection During Powerdown Reg
#define TI_DAC_QCA_R                109 // DAC Quiescent Current Adjustment Reg

//*****************************************************************************
//
// I2C Addresses for the TI DAC.
//
//*****************************************************************************
#define TI_TLV320AIC3107_ADDR           0x018

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
extern tBoolean DACInit(void);
extern void DACVolumeSet(unsigned long ulVolume);
extern unsigned long DACVolumeGet(void);
extern void  DACClassDEn(void);
extern void  DACClassDDis(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __DAC_H__
