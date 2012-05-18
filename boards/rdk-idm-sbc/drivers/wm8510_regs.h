//*****************************************************************************
//
// wm8510_regs.h - Register definitions for the WM8510 mono audio DAC/ADC.
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
// This is part of revision 8555 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#ifndef __WM8510_REGS_H__
#define __WM8510_REGS_H__

//*****************************************************************************
//
//  Register offsets.
//
//*****************************************************************************
#define WM8510_RESET_REG            0x00
#define WM8510_POWER1_REG           0x01
#define WM8510_POWER2_REG           0x02
#define WM8510_POWER3_REG           0x03
#define WM8510_AUD_IF_REG           0x04
#define WM8510_COMP_CTRL_REG        0x05
#define WM8510_CLK_CTRL_REG         0x06
#define WM8510_ADD_CTRL_REG         0x07
#define WM8510_GPIO_REG             0x08
#define WM8510_DAC_CTRL_REG         0x0A
#define WM8510_DAC_VOL_REG          0x0B
#define WM8510_ADC_CTRL_REG         0x0E
#define WM8510_ADC_VOL_REG          0x0F
#define WM8510_DAC_LIMIT1_REG       0x18
#define WM8510_DAC_LIMIT2_REG       0x19
#define WM8510_NOTCH_FILT1_REG      0x1B
#define WM8510_NOTCH_FILT2_REG      0x1C
#define WM8510_NOTCH_FILT3_REG      0x1D
#define WM8510_NOTCH_FILT4_REG      0x1E
#define WM8510_ALC_CTRL1_REG        0x20
#define WM8510_ALC_CTRL2_REG        0x21
#define WM8510_ALC_CTRL3_REG        0x22
#define WM8510_NOISE_GATE_REG       0x23
#define WM8510_PLL_N_REG            0x24
#define WM8510_PLL_K1_REG           0x25
#define WM8510_PLL_K2_REG           0x26
#define WM8510_PLL_K3_REG           0x27
#define WM8510_ATTN_CTRL_REG        0x28
#define WM8510_INPUT_CTRL_REG       0x2C
#define WM8510_INPUT_PGA_GAIN_REG   0x2D
#define WM8510_ADC_BOOST_CTRL_REG   0x2F
#define WM8510_OUTPUT_CTRL_REG      0x31
#define WM8510_SPKR_MIX_CTRL_REG    0x32
#define WM8510_SPKR_VOL_CTRL_REG    0x36
#define WM8510_MONO_MIX_CTRL_REG    0x38

//*****************************************************************************
//
// Definitions for the Power Management 1 Register - WM8510_POWER1_REG
//
//*****************************************************************************
#define POWER1_VMID_OFF             0x000
#define POWER1_VMID_50K             0x001
#define POWER1_VMID_500K            0x002
#define POWER1_VMID_5K              0x003
#define POWER1_BUFIO_EN             0x004
#define POWER1_BIAS_EN              0x008
#define POWER1_MICB_EN              0x010
#define POWER1_PLL_EN               0x020
#define POWER1_MIC2_EN              0x040
#define POWER1_BUFDCOP_EN           0x100

//*****************************************************************************
//
// Definitions for the Power Management 2 Register - WM8510_POWER2_REG
//
//*****************************************************************************
#define POWER2_ADC_EN               0x001
#define POWER2_INP_PGA_EN           0x004
#define POWER2_BOOST_EN             0x010

//*****************************************************************************
//
// Definitions for the Power Management 3 Register - WM8510_POWER3_REG
//
//*****************************************************************************
#define POWER3_DAC_EN               0x001
#define POWER3_SPKR_MIX_EN          0x004
#define POWER3_MONO_MIX_EN          0x008
#define POWER3_SPKR_P_EN            0x020
#define POWER3_SPKR_N_EN            0x040
#define POWER3_MONO_EN              0x080

//*****************************************************************************
//
// Definitions for the Audio Interface Register - WM8510_AUD_IF_REG
//
//*****************************************************************************
#define AUD_IF_ADC_LR_SWAP          0x002
#define AUD_IF_DAC_LR_SWAP          0x004
#define AUD_IF_FMT_RIGHT_JUST       0x000
#define AUD_IF_FMT_LEFT_JUST        0x008
#define AUD_IF_FMT_I2S              0x010
#define AUD_IF_FMT_DSP_PCM          0x018
#define AUD_IF_16_BIT_WORDS         0x000
#define AUD_IF_20_BIT_WORDS         0x020
#define AUD_IF_24_BIT_WORDS         0x040
#define AUD_IF_32_BIT_WORDS         0x060
#define AUD_IF_FRAME_INVERTED       0x080
#define AUD_IF_BCLK_INVERTED        0x100

//*****************************************************************************
//
// Definitions for the Companding Control Register - WM8510_COMP_CTRL_REG
//
//*****************************************************************************
#define COMP_LOOPBACK_EN            0x001
#define ADC_COMP_OFF                0x000
#define ADC_COMP_ULAW               0x004
#define ADC_COMP_ALAW               0x006
#define DAC_COMP_OFF                0x000
#define DAC_COMP_ULAW               0x010
#define DAC_COMP_ALAW               0x018

//*****************************************************************************
//
// Definitions for the Clock Generation Control Register - WM8510_CLK_CTRL_REG
//
//*****************************************************************************
#define CLOCK_MASTER                0x001
#define CLOCK_SLAVE                 0x000
#define CLOCK_BCLK_DIVIDE_1         0x000
#define CLOCK_BCLK_DIVIDE_2         0x004
#define CLOCK_BCLK_DIVIDE_4         0x008
#define CLOCK_BCLK_DIVIDE_8         0x00C
#define CLOCK_BCLK_DIVIDE_16        0x010
#define CLOCK_BCLK_DIVIDE_32        0x014
#define CLOCK_MCLK_DIVIDE_1         0x000
#define CLOCK_MCLK_DIVIDE_1_5       0x020
#define CLOCK_MCLK_DIVIDE_2         0x040
#define CLOCK_MCLK_DIVIDE_3         0x060
#define CLOCK_MCLK_DIVIDE_4         0x080
#define CLOCK_MCLK_DIVIDE_6         0x0A0
#define CLOCK_MCLK_DIVIDE_8         0x0C0
#define CLOCK_MCLK_DIVIDE_12        0x0E0
#define CLOCK_SELECT_PLL            0x100

//*****************************************************************************
//
// Definitions for the Additional Control Register - WM8510_ADD_CTRL_REG
//
//*****************************************************************************
#define ADD_SLOW_CLK_EN             0x001
#define ADD_SAMP_RATE_48KHZ         0x000
#define ADD_SAMP_RATE_32KHZ         0x002
#define ADD_SAMP_RATE_24KHZ         0x004
#define ADD_SAMP_RATE_16KHZ         0x006
#define ADD_SAMP_RATE_12KHZ         0x008
#define ADD_SAMP_RATE_8KHZ          0x00A

//*****************************************************************************
//
// Definitions for the GPIO Stuff Register - WM8510_GPIO_REL
//
//*****************************************************************************
#define GPIO_SEL_CSB_INPUT          0x000
#define GPIO_SEL_JACK_INSERT_DET    0x001
#define GPIO_SEL_TEMP_OK            0x002
#define GPIO_SEL_AMUTE_ACTIVE       0x003
#define GPIO_SEL_PLL_CLK_OUTPUT     0x004
#define GPIO_SEL_PLL_LOCK           0x005
#define GPIO_POL_INVERT             0x008
#define GPIO_PLL_DIVIDE_1           0x000
#define GPIO_PLL_DIVIDE_2           0x010
#define GPIO_PLL_DIVIDE_3           0x020
#define GPIO_PLL_DIVIDE_4           0x030

//*****************************************************************************
//
// Definitions for the DAC Control Register - WM8510_DAC_CTRL_REG
//
//*****************************************************************************
#define DAC_OUTPUT_INVERTED         0x001
#define DAC_AUTO_MUTE_EN            0x004
#define DAC_OVERSAMPLE_64X          0x000
#define DAC_OVERSAMPLE_128X         0x008
#define DAC_DEEMPHASIS_NONE         0x000
#define DAC_DEEMPHASIS_32KHZ        0x010
#define DAC_DEEMPHASIS_44_1KHZ      0x020
#define DAC_DEEMPHASIS_48KHZ        0x030
#define DAC_SOFT_MUTE_EN            0x040

//*****************************************************************************
//
// Definitions for the DAC Digital Volume Register - WM8510_DAC_VOL_REG
//
// Volume is defined in 0.5dB steps from -127dB to 0.  Only the extremities of
// the range are defined here.
//
//*****************************************************************************
#define DAC_VOLUME_MUTE             0x001
#define DAC_VOLUME_0DB              0x0FF

//*****************************************************************************
//
// Definitions for the ADC Control Register - WM8510_ADC_CTRL_REG
//
//*****************************************************************************
#define ADC_POLARITY_INVERTED       0x001
#define ADC_OVERSAMPLE_64X          0x000
#define ADC_OVERSAMPLE_128X         0x004
#define ADC_HPF_CUTOFF_FREQ_M       0x070
#define ADC_HPF_MODE_1ST_ORDER      0x000
#define ADC_HPF_MODE_2ND_ORDER      0x080
#define ADC_HPF_EN                  0x100

//*****************************************************************************
//
// Definitions for the ADC Digital Volume Register - WM8510_ADC_VOL_REG
//
// Volume is defined in 0.5dB steps from -127dB to 0.  Only the extremities of
// the range are defined here.
//
//*****************************************************************************
#define ADC_VOLUME_MUTE             0x001
#define ADC_VOLUME_0DB              0x0FF

//*****************************************************************************
//
// Definitions for the DAC Limiter 1 Register - WM8510_DAC_LIMIT1_REG
//
//*****************************************************************************
#define DAC_LIMIT_ATTACK_94US       0x000
#define DAC_LIMIT_ATTACK_188US      0x001
#define DAC_LIMIT_ATTACK_375US      0x002
#define DAC_LIMIT_ATTACK_750US      0x003
#define DAC_LIMIT_ATTACK_1500US     0x004
#define DAC_LIMIT_ATTACK_3MS        0x005
#define DAC_LIMIT_ATTACK_6MS        0x006
#define DAC_LIMIT_ATTACK_12MS       0x007
#define DAC_LIMIT_ATTACK_24MS       0x008
#define DAC_LIMIT_ATTACK_48MS       0x009
#define DAC_LIMIT_ATTACK_96MS       0x00A
#define DAC_LIMIT_ATTACK_192MS      0x00B
#define DAC_LIMIT_DECAY_750US       0x000
#define DAC_LIMIT_DECAY_1500US      0x010
#define DAC_LIMIT_DECAY_3MS         0x020
#define DAC_LIMIT_DECAY_6MS         0x030
#define DAC_LIMIT_DECAY_12MS        0x040
#define DAC_LIMIT_DECAY_24MS        0x050
#define DAC_LIMIT_DECAY_48MS        0x060
#define DAC_LIMIT_DECAY_96MS        0x070
#define DAC_LIMIT_DECAY_192MS       0x080
#define DAC_LIMIT_DECAY_384MS       0x090
#define DAC_LIMIT_DECAY_768MS       0x0A0
#define DAC_LIMIT_DECAY_1536MS      0x0B0
#define DAC_LIMIT_EN                0x100

//*****************************************************************************
//
// Definitions for the DAC Limiter 2 Register - WM8510_DAC_LIMIT2_REG
//
//*****************************************************************************
#define DAC_LIMIT_BOOST_M           0x00F
#define DAC_LIMIT_BOOST_0DB         0x000
#define DAC_LIMIT_BOOST_1DB         0x001
#define DAC_LIMIT_BOOST_2DB         0x002
#define DAC_LIMIT_BOOST_3DB         0x003
#define DAC_LIMIT_BOOST_4DB         0x004
#define DAC_LIMIT_BOOST_5DB         0x005
#define DAC_LIMIT_BOOST_6DB         0x006
#define DAC_LIMIT_BOOST_7DB         0x007
#define DAC_LIMIT_BOOST_8DB         0x008
#define DAC_LIMIT_BOOST_9DB         0x009
#define DAC_LIMIT_BOOST_10DB        0x00A
#define DAC_LIMIT_BOOST_11DB        0x00B
#define DAC_LIMIT_BOOST_12DB        0x00C
#define DAC_LIMIT_LEVEL_MINUS_1DB   0x000
#define DAC_LIMIT_LEVEL_MINUS_2DB   0x010
#define DAC_LIMIT_LEVEL_MINUS_3DB   0x020
#define DAC_LIMIT_LEVEL_MINUS_4DB   0x030
#define DAC_LIMIT_LEVEL_MINUS_5DB   0x040

//*****************************************************************************
//
// Definitions for the Notch Filter 1 Register - WM8510_NOTCH_FILT1_REG
//
//*****************************************************************************
#define NOTCH_FILT1_A0_13_7_M       0x07F
#define NOTCH_FILT1_EN              0x080
#define NOTCH_FILT_UPDATE           0x100

//*****************************************************************************
//
// Definitions for the Notch Filter 2 Register - WM8510_NOTCH_FILT2_REG
//
//*****************************************************************************
#define NOTCH_FILT2_A0_6_0_M        0x07F
//
//NOTCH_FILT_UPDATE applies to this register too.
//

//*****************************************************************************
//
// Definitions for the Notch Filter 3 Register - WM8510_NOTCH_FILT3_REG
//
//*****************************************************************************
#define NOTCH_FILT3_A1_13_7_M        0x07F
//
//NOTCH_FILT_UPDATE applies to this register too.
//

//*****************************************************************************
//
// Definitions for the Notch Filter 4 Register - WM8510_NOTCH_FILT4_REG
//
//*****************************************************************************
#define NOTCH_FILT4_A1_6_0_M        0x07F
//
//NOTCH_FILT_UPDATE applies to this register too.
//

//*****************************************************************************
//
// Definitions for the ALC Control 1 Register - WM8510_ALC_CTRL1_REG
//
//*****************************************************************************
#define ALC_MIN_GAIN_MINUS_12DB     0x000
#define ALC_MIN_GAIN_MINUS_6DB      0x001
#define ALC_MIN_GAIN_0DB            0x002
#define ALC_MIN_GAIN_6DB            0x003
#define ALC_MIN_GAIN_12DB           0x004
#define ALC_MIN_GAIN_18DB           0x005
#define ALC_MIN_GAIN_24DB           0x006
#define ALC_MIN_GAIN_30DB           0x007
#define ALC_MAX_GAIN_MINUS_6_75DB   0x000
#define ALC_MAX_GAIN_MINUS_0_75DB   0x008
#define ALC_MAX_GAIN_5_25DB         0x010
#define ALC_MAX_GAIN_11_25DB        0x018
#define ALC_MAX_GAIN_17_25DB        0x020
#define ALC_MAX_GAIN_23_25DB        0x028
#define ALC_MAX_GAIN_29_25DB        0x030
#define ALC_MAX_GAIN_35_25DB        0x038
#define ALC_SELECT_ON               0x100

//*****************************************************************************
//
// Definitions for the ALC Control 2 Register - WM8510_ALC_CTRL2_REG
//
// Level is defined in -1.5dB steps between the limits defined here.
// Hold time doubles with each step from 2.67mS to 43.691S.
//
//*****************************************************************************
#define ALC_LEVEL_M                 0x00F
#define ALC_LEVEL_MINUS_28_5DB      0x000
#define ALC_LEVEL_MINUS_6DB         0x00F
#define ALC_HOLD_TIME_M             0x0F0
#define ALC_HOLD_TIME_0MS           0x000
#define ALC_HOLD_TIME_2_67MS        0x010
#define ALC_HOLD_TIME_43_691S       0x0F0
#define ALC_ZERO_CROSS_EN           0x100

//*****************************************************************************
//
// Definitions for the ALC Control 3 Register - WM8510_ALC_CTRL3_REG
//
// See datasheet for the meaning of each possible attack and decay setting.
// The meaning depends upon various other settings.
//
//*****************************************************************************
#define ALC_ATTACK_M                0x00F
#define ALC_DECAY_M                 0x0F0
#define ALC_MODE_ALC                0x000
#define ALC_MODE_LIMITER            0x100

//*****************************************************************************
//
// Definitions for the Noise Gate Register - WM8510_NOISE_GATE_REG
//
//*****************************************************************************
#define NOISE_GATE_THOLD_M          0x007
#define NOISE_GATE_THOLD_MINUS_39DB 0x000
#define NOISE_GATE_THOLD_MINUS_45DB 0x001
#define NOISE_GATE_THOLD_MINUS_51DB 0x002
#define NOISE_GATE_THOLD_MINUS_57DB 0x003
#define NOISE_GATE_THOLD_MINUS_63DB 0x004
#define NOISE_GATE_THOLD_MINUS_70DB 0x005
#define NOISE_GATE_THOLD_MINUS_76DB 0x006
#define NOISE_GATE_THOLD_MINUS_81DB 0x007
#define NOISE_GATE_EN               0x100

//*****************************************************************************
//
// Definitions for the PLL N Register - WM8510_PLL_N_REG
//
// PLL_N value must be greater than 5 and less than 13.
//
//*****************************************************************************
#define PLL_N_M                     0x00F
#define PLL_MCLK_PRESCALE_NONE      0x000
#define PLL_MCLK_PRESCALE_2         0x010

//*****************************************************************************
//
// Definitions for the PLL K 1 Register - WM8510_PLL_K1_REG
//
//*****************************************************************************
#define PLL_K_23_18_M               0x03F

//*****************************************************************************
//
// Definitions for the PLL K 2 Register - WM8510_PLL_K2_REG
//
//*****************************************************************************
#define PLL_K_17_9_M                0x1FF

//*****************************************************************************
//
// Definitions for the PLL K 3 Register - WM8510_PLL_K3_REG
//
//*****************************************************************************
#define PLL_K_9_0_M                0x1FF

//*****************************************************************************
//
// Definitions for the Attenuation Control Register - WM8510_ATTN_CTRL_REG
//
//*****************************************************************************
#define ATTN_SPKR_ATTN_0DB          0x000
#define ATTN_SPKR_ATTN_MINUS_10DB   0x002
#define ATTN_MONO_ATTN_0DB          0x000
#define ATTN_MONO_ATTN_MINUS_10DB   0x004

//*****************************************************************************
//
// Definitions for the Input Control Register - WM8510_INPUT_CTRL_REG
//
//*****************************************************************************
#define INPUT_MICP_TO_PGAP         0x001
#define INPUT_MICN_TO_PGAN         0x002
#define INPUT_MIC2P_TO_PGAP        0x004
#define INPUT_MIC2_MODE_MIXER      0x008
#define INPUT_MIC2_MODE_INV_BUFFER 0x000
#define INPUT_MIC_BIAS_0_9AVDD     0x000
#define INPUT_MIC_BIAS_0_75AVDD    0x100

//*****************************************************************************
//
// Definitions for the Input PGA Gain Control Reg. - WM8510_INPUT_PGA_GAIN_REG
//
// Input PGA volume is defined in 0.75dB steps from -12dB to +35.25dB.
//
//*****************************************************************************
#define INPUT_PGA_VOL_M            0x03F
#define INPUT_PGA_VOL_MINUS_12DB   0x000
#define INPUT_PGA_VOL_35_25DB      0x03F
#define INPUT_PGA_MUTE_EN          0x040
#define INPUT_PGA_ZERO_CROSS_EN    0x080

//*****************************************************************************
//
// Definitions for the ADC Boost Control Register - WM8510_ADC_BOOST_CTRL_REG
//
//*****************************************************************************
#define PGA_BOOST_MIC2_M            0x007
#define PGA_BOOST_MIC2_DISABLE      0x000
#define PGA_BOOST_MIC2_MINUS_12DB   0x001
#define PGA_BOOST_MIC2_MINUS_9DB    0x002
#define PGA_BOOST_MIC2_MINUS_6DB    0x003
#define PGA_BOOST_MIC2_MINUS_3DB    0x004
#define PGA_BOOST_MIC2_0DB          0x005
#define PGA_BOOST_MIC2_3DB          0x006
#define PGA_BOOST_MIC2_6DB          0x007
#define PGA_BOOST_MICP_M            0x070
#define PGA_BOOST_MICP_DISABLE      0x000
#define PGA_BOOST_MICP_MINUS_12DB   0x010
#define PGA_BOOST_MICP_MINUS_9DB    0x020
#define PGA_BOOST_MICP_MINUS_6DB    0x030
#define PGA_BOOST_MICP_MINUS_3DB    0x040
#define PGA_BOOST_MICP_0DB          0x050
#define PGA_BOOST_MICP_3DB          0x060
#define PGA_BOOST_MICP_6DB          0x070
#define PGA_BOOST_INPUT_0DB         0x000
#define PGA_BOOST_INPUT_20DB        0x100

//*****************************************************************************
//
// Definitions for the Output Control Register - WM8510_OUTPUT_CTRL_REG
//
//*****************************************************************************
#define OUTPUT_VREF_RESIST_1K       0x000
#define OUTPUT_VREF_RESIST_30K      0x001
#define OUTPUT_THERMAL_SHUTDN_EN    0x002
#define OUTPUT_SPKR_BOOST_EN        0x004
#define OUTPUT_MONO_BOOST_EN        0x008

//*****************************************************************************
//
// Definitions for the Speaker Mixer Control Reg. - WM8510_SPKR_MIX_CTRL_REG
//
//*****************************************************************************
#define SPKR_MIX_DAC_TO_SPK         0x001
#define SPKR_MIX_BYP_TO_SPK         0x002
#define SPKR_MIX_MIC2_TO_SPK        0x020

//*****************************************************************************
//
// Definitions for the Speaker Volume Control Reg. - WM8510_SPKR_VOL_CTRL_REG
//
// Speaker volume is defined in 1dB steps from -57dB to +6dB.
//
//*****************************************************************************
#define SPKR_VOL_M                  0x03F
#define SPKR_VOL_MINUS_57DB         0x000
#define SPKR_VOL_0DB                0x039
#define SPKR_VOL_6DB                0x03F
#define SPKR_VOL_MUTE               0x040
#define SPKR_VOL_ZERO_CROSS_EN      0x080

//*****************************************************************************
//
// Definitions for the Mono Mixer Control Register - WM8510_MONO_MIX_CTRL_REG
//
//*****************************************************************************
#define MONO_MIX_DAC_TO_MONO        0x001
#define MONO_MIX_BYP_TO_MONO        0x002
#define MONO_MIX_MIC2_TO_MONO       0x004
#define MONO_MIX_MUTE               0x040

//*****************************************************************************
//
// I2C Addresses for the DAC.
//
//*****************************************************************************
#define WM8510_I2C_ADDR_0   0x01A

#endif // __WM8510_REGS_H__

