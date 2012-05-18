#******************************************************************************
#
# Makefile - Rules for building the Brushed DC Motor Controller.
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
# This is part of revision 8555 of the RDK-BDC24 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S2616

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
VPATH=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=..
IPATH+=../../..

#
# The default rule, which causes the Brushed DC Motor Controller to be built.
#
all: ${COMPILER}
all: ${COMPILER}/qs-bdc24.axf

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
# Rules for building the Brushed DC Motor Controller.
#
${COMPILER}/qs-bdc24.axf: ${COMPILER}/adc_ctrl.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/button.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/can_if.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/commands.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/controller.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/encoder.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/fan.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/flash_pb.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/hbridge.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/led.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/limit.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/math.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/message.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/param.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/pid.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/qs-bdc24.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/servo_if.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/uart_if.o
${COMPILER}/qs-bdc24.axf: ${COMPILER}/wdog.o
${COMPILER}/qs-bdc24.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/qs-bdc24.axf: qs-bdc24.ld
SCATTERgcc_qs-bdc24=qs-bdc24.ld
ENTRY_qs-bdc24=ResetISR
CFLAGSgcc=-DTARGET_IS_DUSTDEVIL_RA0

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
