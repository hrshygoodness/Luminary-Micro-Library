#******************************************************************************
#
# Makefile - Rules for building the EvalBot Autonomous Drive Quickstart Example.
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
VPATH=../drivers
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=..
IPATH+=../../..

#
# The default rule, which causes the EvalBot Autonomous Drive Quickstart Example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/qs-autonomous.axf

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
# Rules for building the EvalBot Autonomous Drive Quickstart Example.
#
${COMPILER}/qs-autonomous.axf: ${COMPILER}/auto_task.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/dac.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/display96x16x1.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/display_task.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/drive_task.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/io.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/led_task.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/motor.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/pid.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/qs-autonomous.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/random.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/scheduler.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/sensors.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/sound.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/sound_task.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/sounds.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/uartstdio.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/ustdlib.o
${COMPILER}/qs-autonomous.axf: ${COMPILER}/wav.o
${COMPILER}/qs-autonomous.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/qs-autonomous.axf: qs-autonomous.ld
SCATTERgcc_qs-autonomous=qs-autonomous.ld
ENTRY_qs-autonomous=ResetISR
CFLAGSgcc=-DTARGET_IS_TEMPEST_RB1 -DUART_BUFFERED

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
