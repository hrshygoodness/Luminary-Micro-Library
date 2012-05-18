#******************************************************************************
#
# Makefile - Rules for building the qs-keypad example.
#
# Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 8555 of the RDK-IDM-L35 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S1958

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
VPATH=../drivers
VPATH+=../../../third_party/fatfs/port
VPATH+=../../../third_party/fatfs/src
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=..
IPATH+=../../..
IPATH+=../../../third_party

#
# The default rule, which causes the qs-keypad example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/qs-keypad.axf

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
# Rules for building the qs-keypad example.
#
${COMPILER}/qs-keypad.axf: ${COMPILER}/cmdline.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/demo_images.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/ff.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/grlib_demo.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/images.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/kitronix320x240x16_ssd2119.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/log.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/mmc-rdk-idm.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/qs-keypad.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/random.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/relay.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/sound.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/touch.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/uartstdio.o
${COMPILER}/qs-keypad.axf: ${COMPILER}/ustdlib.o
${COMPILER}/qs-keypad.axf: ${ROOT}/grlib/${COMPILER}-cm3/libgr-cm3.a
${COMPILER}/qs-keypad.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/qs-keypad.axf: qs-keypad.ld
SCATTERgcc_qs-keypad=qs-keypad.ld
ENTRY_qs-keypad=ResetISR
CFLAGSgcc=-DUART_BUFFERED

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
