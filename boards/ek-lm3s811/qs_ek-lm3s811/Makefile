#******************************************************************************
#
# Makefile - Rules for building the qs_ek-lm3s811 example.
#
# Copyright (c) 2006-2013 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 10636 of the EK-LM3S811 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S811

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
# The default rule, which causes the qs_ek-lm3s811 example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/qs_ek-lm3s811.axf

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
# Rules for building the qs_ek-lm3s811 example.
#
${COMPILER}/qs_ek-lm3s811.axf: ${COMPILER}/display96x16x1.o
${COMPILER}/qs_ek-lm3s811.axf: ${COMPILER}/game.o
${COMPILER}/qs_ek-lm3s811.axf: ${COMPILER}/qs_ek-lm3s811.o
${COMPILER}/qs_ek-lm3s811.axf: ${COMPILER}/random.o
${COMPILER}/qs_ek-lm3s811.axf: ${COMPILER}/screen_saver.o
${COMPILER}/qs_ek-lm3s811.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/qs_ek-lm3s811.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/qs_ek-lm3s811.axf: qs_ek-lm3s811.ld
SCATTERgcc_qs_ek-lm3s811=qs_ek-lm3s811.ld
ENTRY_qs_ek-lm3s811=ResetISR

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
