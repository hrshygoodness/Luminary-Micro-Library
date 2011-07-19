#******************************************************************************
#
# Makefile - Rules for building the grlib_demo example.
#
# Copyright (c) 2008-2010 Texas Instruments Incorporated.  All rights reserved.
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
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=..
IPATH+=../../..

#
# The default rule, which causes the grlib_demo example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/grlib_demo.axf

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
# Rules for building the grlib_demo example.
#
${COMPILER}/grlib_demo.axf: ${COMPILER}/grlib_demo.o
${COMPILER}/grlib_demo.axf: ${COMPILER}/images.o
${COMPILER}/grlib_demo.axf: ${COMPILER}/kitronix320x240x16_ssd2119_8bit.o
${COMPILER}/grlib_demo.axf: ${COMPILER}/set_pinout.o
${COMPILER}/grlib_demo.axf: ${COMPILER}/sound.o
${COMPILER}/grlib_demo.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/grlib_demo.axf: ${COMPILER}/tlv320aic23b.o
${COMPILER}/grlib_demo.axf: ${COMPILER}/touch.o
${COMPILER}/grlib_demo.axf: ${COMPILER}/ustdlib.o
${COMPILER}/grlib_demo.axf: ${ROOT}/grlib/${COMPILER}/libgr.a
${COMPILER}/grlib_demo.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/grlib_demo.axf: grlib_demo.ld
SCATTERgcc_grlib_demo=grlib_demo.ld
ENTRY_grlib_demo=ResetISR

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
