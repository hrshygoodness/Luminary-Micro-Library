#******************************************************************************
#
# Makefile - Rules for building the I2S Demo Application.
#
# Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
# Software License Agreement
# 
# Texas Instruments (TI) is supplying this software for use solely and
# exclusively on TI's microcontroller products. The software is owned by
# TI and/or its suppliers, and is protected under applicable copyright
# laws. You may not combine this software with "viral" open-source
# software in order to form a larger program.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
# NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
# NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
# CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
# DAMAGES, FOR ANY REASON WHATSOEVER.
# 
# This is part of revision 8555 of the DK-LM3S9B96 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S9B96

#
# Set the processor variant.
#
VARIANT=cm3

#
# The base directory for StellarisWare.
#
ROOT=../../..

#
# Include the common make definitions.
#
include ${ROOT}/makedefs

#
# Where to find source files that do not live in this directory.
#
VPATH=../../../third_party/bget
VPATH+=../drivers
VPATH+=../../../third_party/speex-1.2rc1/libspeex
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=..
IPATH+=../../..
IPATH+=../../../third_party/speex-1.2rc1/include
IPATH+=../../../third_party/speex-1.2rc1/libspeex
IPATH+=../../../third_party/speex-1.2rc1/stellaris
IPATH+=../../../third_party

#
# The default rule, which causes the I2S Demo Application to be built.
#
all: ${COMPILER}
all: ${COMPILER}/i2s_speex_enc.axf

#
# The rule to clean out all the build products.
#
clean:
	@rm -rf ${COMPILER} ${wildcard *~}

#
# The rule to create the target directory.
#
${COMPILER}:
	@mkdir -p ${COMPILER}

#
# Rules for building the I2S Demo Application.
#
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/bget.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/bits.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/cb_search.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/cmdline.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/exc_10_16_table.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/exc_10_32_table.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/exc_20_32_table.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/exc_5_256_table.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/exc_5_64_table.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/exc_8_128_table.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/filters.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/gain_table.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/gain_table_lbr.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/i2s_speex_enc.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/lpc.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/lsp.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/lsp_tables_nb.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/ltp.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/modes.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/nb_celp.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/quant_lsp.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/set_pinout.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/sound.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/speex.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/speex_callbacks.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/speexlib.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/tlv320aic23b.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/uartstdio.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/ustdlib.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/vbr.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/vq.o
${COMPILER}/i2s_speex_enc.axf: ${COMPILER}/window.o
${COMPILER}/i2s_speex_enc.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/i2s_speex_enc.axf: i2s_speex_enc.ld
SCATTERgcc_i2s_speex_enc=i2s_speex_enc.ld
ENTRY_i2s_speex_enc=ResetISR
CFLAGSgcc=-DTARGET_IS_TEMPEST_RB1 -DHAVE_CONFIG_H -DCONFIG_STELLARIS

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
