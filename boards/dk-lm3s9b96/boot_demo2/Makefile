#******************************************************************************
#
# Makefile - Rules for building the boot_demo2 example.
#
# Copyright (c) 2009-2011 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 7611 of the DK-LM3S9B96 Firmware Package.
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
IPATH+=../../../third_party/lwip-1.3.2/apps
IPATH+=../../../third_party/bget
IPATH+=../../../third_party/lwip-1.3.2/ports/stellaris/include
IPATH+=../../../third_party/lwip-1.3.2/src/include
IPATH+=../../../third_party/lwip-1.3.2/src/include/ipv4
IPATH+=../../../third_party

# The default rule, which causes the boot_demo2 example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/boot_demo2.axf

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
# Rules for building the boot_demo2 example.
#
${COMPILER}/boot_demo2.axf: ${COMPILER}/boot_demo2.o
${COMPILER}/boot_demo2.axf: ${COMPILER}/kitronix320x240x16_ssd2119_8bit.o
${COMPILER}/boot_demo2.axf: ${COMPILER}/lwiplib.o
${COMPILER}/boot_demo2.axf: ${COMPILER}/set_pinout.o
${COMPILER}/boot_demo2.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/boot_demo2.axf: ${COMPILER}/touch.o
${COMPILER}/boot_demo2.axf: ${COMPILER}/ustdlib.o
${COMPILER}/boot_demo2.axf: ${ROOT}/grlib/${COMPILER}/libgr.a
${COMPILER}/boot_demo2.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/boot_demo2.axf: boot_demo2.ld
SCATTERgcc_boot_demo2=boot_demo2.ld
ENTRY_boot_demo2=ResetISR
CFLAGSgcc=-DTARGET_IS_TEMPEST_RB1

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
