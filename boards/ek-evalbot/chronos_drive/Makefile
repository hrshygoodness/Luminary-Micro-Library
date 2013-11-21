#******************************************************************************
#
# Makefile - Rules for building the EvalBot eZ430-Chronos Remote Control.
#
# Copyright (c) 2010-2013 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 10636 of the Stellaris Firmware Development Package.
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
VPATH=../../../SimpliciTI-1.1.1
VPATH+=../drivers
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=..
IPATH+=../../..
IPATH+=../../../SimpliciTI-1.1.1
IPATH+=../../../SimpliciTI-1.1.1/Components/bsp
IPATH+=../../../SimpliciTI-1.1.1/Components/bsp/drivers
IPATH+=../../../SimpliciTI-1.1.1/Components/bsp/boards/ek-evalbot
IPATH+=../../../SimpliciTI-1.1.1/Components/mrfi
IPATH+=../../../SimpliciTI-1.1.1/Components/simpliciti/nwk
IPATH+=../../../SimpliciTI-1.1.1/Components/simpliciti/nwk_applications

#
# The default rule, which causes the EvalBot eZ430-Chronos Remote Control to be built.
#
all: ${COMPILER}
all: ${COMPILER}/chronos_drive.axf

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
# Rules for building the EvalBot eZ430-Chronos Remote Control.
#
${COMPILER}/chronos_drive.axf: ${COMPILER}/chronos_drive.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/dac.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/display96x16x1.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/io.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/motor.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/scheduler.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/sensors.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/simplicitilib.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/sound.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/sounds.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/uartstdio.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/ustdlib.o
${COMPILER}/chronos_drive.axf: ${COMPILER}/wav.o
${COMPILER}/chronos_drive.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/chronos_drive.axf: chronos_drive.ld
SCATTERgcc_chronos_drive=chronos_drive.ld
ENTRY_chronos_drive=ResetISR
CFLAGSgcc=-DSIMPLICITI_CONFIG_HEADER -DTARGET_IS_TEMPEST_RB1

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
