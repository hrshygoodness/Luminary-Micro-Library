#******************************************************************************
#
# Makefile - Rules for building the lang_demo example.
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
# This is part of revision 9453 of the RDK-IDM Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S6918

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
VPATH+=../../../third_party/fonts/lang_demo
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=..
IPATH+=../../..
IPATH+=../../../third_party/lwip-1.3.2/ports/stellaris/include
IPATH+=../../../third_party/lwip-1.3.2/src/include
IPATH+=../../../third_party/lwip-1.3.2/src/include/ipv4
IPATH+=../../../third_party/fonts/lang_demo

#
# The default rule, which causes the lang_demo example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/lang_demo.axf

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
# Rules for building the lang_demo example.
#
${COMPILER}/lang_demo.axf: ${COMPILER}/fontcustom14pt.o
${COMPILER}/lang_demo.axf: ${COMPILER}/fontcustom20pt.o
${COMPILER}/lang_demo.axf: ${COMPILER}/fontcustomr14pt.o
${COMPILER}/lang_demo.axf: ${COMPILER}/fontcustomr20pt.o
${COMPILER}/lang_demo.axf: ${COMPILER}/formike240x320x16_ili9320.o
${COMPILER}/lang_demo.axf: ${COMPILER}/images.o
${COMPILER}/lang_demo.axf: ${COMPILER}/lang_demo.o
${COMPILER}/lang_demo.axf: ${COMPILER}/langremap.o
${COMPILER}/lang_demo.axf: ${COMPILER}/language.o
${COMPILER}/lang_demo.axf: ${COMPILER}/locator.o
${COMPILER}/lang_demo.axf: ${COMPILER}/lwiplib.o
${COMPILER}/lang_demo.axf: ${COMPILER}/sound.o
${COMPILER}/lang_demo.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/lang_demo.axf: ${COMPILER}/swupdate.o
${COMPILER}/lang_demo.axf: ${COMPILER}/touch.o
${COMPILER}/lang_demo.axf: ${COMPILER}/ustdlib.o
${COMPILER}/lang_demo.axf: ${ROOT}/grlib/${COMPILER}-cm3/libgr-cm3.a
${COMPILER}/lang_demo.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/lang_demo.axf: lang_demo.ld
SCATTERgcc_lang_demo=lang_demo.ld
ENTRY_lang_demo=ResetISR

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
