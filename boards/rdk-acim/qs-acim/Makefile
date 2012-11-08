#******************************************************************************
#
# Makefile - Rules for building the A/C induction motor application.
#
# Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 9453 of the RDK-ACIM Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S818

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
IPATH=../../..

#
# The default rule, which causes the A/C induction motor application to be built.
#
all: ${COMPILER}
all: ${COMPILER}/qs-acim.axf

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
# Rules for building the A/C induction motor application.
#
${COMPILER}/qs-acim.axf: ${COMPILER}/adc_ctrl.o
${COMPILER}/qs-acim.axf: ${COMPILER}/brake.o
${COMPILER}/qs-acim.axf: ${COMPILER}/cpu_usage.o
${COMPILER}/qs-acim.axf: ${COMPILER}/flash_pb.o
${COMPILER}/qs-acim.axf: ${COMPILER}/inrush.o
${COMPILER}/qs-acim.axf: ${COMPILER}/main.o
${COMPILER}/qs-acim.axf: ${COMPILER}/pwm_ctrl.o
${COMPILER}/qs-acim.axf: ${COMPILER}/sine.o
${COMPILER}/qs-acim.axf: ${COMPILER}/sinemod.o
${COMPILER}/qs-acim.axf: ${COMPILER}/speed_sense.o
${COMPILER}/qs-acim.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/qs-acim.axf: ${COMPILER}/svm.o
${COMPILER}/qs-acim.axf: ${COMPILER}/ui.o
${COMPILER}/qs-acim.axf: ${COMPILER}/ui_onboard.o
${COMPILER}/qs-acim.axf: ${COMPILER}/ui_serial.o
${COMPILER}/qs-acim.axf: ${COMPILER}/vf.o
${COMPILER}/qs-acim.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/qs-acim.axf: qs-acim.ld
SCATTERgcc_qs-acim=qs-acim.ld
ENTRY_qs-acim=ResetISR

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
