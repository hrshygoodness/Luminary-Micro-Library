#******************************************************************************
#
# Makefile - Rules for building the I2S Record and Playback Demo Application.
#
# Copyright (c) 2009-2010 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S9B96

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
VPATH=../drivers

#
# Where to find header files that do not live in the source directory.
#
IPATH=..
IPATH+=../../..

#
# The default rule, which causes the I2S Record and Playback Demo Application to be built.
#
all: ${COMPILER}
all: ${COMPILER}/i2s_filter.axf

#
# The rule to clean out all the build products.
#
clean:
	@rm -rf ${COMPILER} ${wildcard *~}

#
# The rule to create the target directory.
#
${COMPILER}:
	@mkdir ${COMPILER}

#
# Rules for building the I2S Record and Playback Demo Application.
#
${COMPILER}/i2s_filter.axf: ${COMPILER}/i2s_filter.o
${COMPILER}/i2s_filter.axf: ${COMPILER}/set_pinout.o
${COMPILER}/i2s_filter.axf: ${COMPILER}/sound.o
${COMPILER}/i2s_filter.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/i2s_filter.axf: ${COMPILER}/tlv320aic23b.o
${COMPILER}/i2s_filter.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/i2s_filter.axf: i2s_filter.ld
SCATTERgcc_i2s_filter=i2s_filter.ld
ENTRY_i2s_filter=ResetISR

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
