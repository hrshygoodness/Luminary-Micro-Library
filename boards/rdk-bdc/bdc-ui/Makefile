#******************************************************************************
#
# Makefile - Rules for building the Brushed DC User Interface.
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
# This is part of revision 9453 of the RDK-BDC Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S2965

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
# The default rule, which causes the Brushed DC User Interface to be built.
#
all: ${COMPILER}
all: ${COMPILER}/bdc-ui.axf

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
# Rules for building the Brushed DC User Interface.
#
${COMPILER}/bdc-ui.axf: ${COMPILER}/about.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/bdc-ui.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/buttons.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/can_comm.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/config.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/current.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/dev_list.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/help.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/menu.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/position.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/rit128x96x4.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/speed.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/splash.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/status.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/update.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/ustdlib.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/vcomp.o
${COMPILER}/bdc-ui.axf: ${COMPILER}/voltage.o
${COMPILER}/bdc-ui.axf: ${ROOT}/grlib/${COMPILER}-cm3/libgr-cm3.a
${COMPILER}/bdc-ui.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/bdc-ui.axf: bdc-ui.ld
SCATTERgcc_bdc-ui=bdc-ui.ld
ENTRY_bdc-ui=ResetISR

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
