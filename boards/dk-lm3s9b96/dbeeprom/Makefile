#******************************************************************************
#
# Makefile - Rules for building the Tempest DK Daughter Board EEPROM Application.
#
# Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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
VPATH=../drivers
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=..
IPATH+=../../..

#
# The default rule, which causes the Tempest DK Daughter Board EEPROM Application to be built.
#
all: ${COMPILER}
all: ${COMPILER}/dbeeprom.axf

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
# Rules for building the Tempest DK Daughter Board EEPROM Application.
#
${COMPILER}/dbeeprom.axf: ${COMPILER}/cmdline.o
${COMPILER}/dbeeprom.axf: ${COMPILER}/dbeeprom.o
${COMPILER}/dbeeprom.axf: ${COMPILER}/set_pinout.o
${COMPILER}/dbeeprom.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/dbeeprom.axf: ${COMPILER}/uartstdio.o
${COMPILER}/dbeeprom.axf: ${COMPILER}/ustdlib.o
${COMPILER}/dbeeprom.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/dbeeprom.axf: dbeeprom.ld
SCATTERgcc_dbeeprom=dbeeprom.ld
ENTRY_dbeeprom=ResetISR
CFLAGSgcc=-DTARGET_IS_TEMPEST_RB1

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
