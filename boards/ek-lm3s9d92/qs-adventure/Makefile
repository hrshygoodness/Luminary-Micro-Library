#******************************************************************************
#
# Makefile - Rules for building the quickstart application.
#
# Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 10636 of the EK-LM3S9D92 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S9D92

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
VPATH=../../../third_party/bget
VPATH+=../../../utils
VPATH+=../../../third_party/zip

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=../../..
IPATH+=../../../third_party/lwip-1.3.2/apps
IPATH+=../../../third_party/lwip-1.3.2/ports/stellaris/include
IPATH+=../../../third_party/lwip-1.3.2/src/include
IPATH+=../../../third_party/lwip-1.3.2/src/include/ipv4
IPATH+=../../../third_party
IPATH+=../../../third_party/zip

#
# The default rule, which causes the quickstart application to be built.
#
all: ${COMPILER}
all: ${COMPILER}/qs-adventure.axf

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
# Rules for building the quickstart application.
#
${COMPILER}/qs-adventure.axf: ${COMPILER}/bget.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/clib.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/control.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/enet_if.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/extern.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/fileio.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/input.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/interpre.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/io.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/locator.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/lwiplib.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/math.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/memory.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/object.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/operand.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/osdepend.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/property.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/qs-adventure.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/screen.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/text.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/uartstdio.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/usb_if.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/ustdlib.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/variable.o
${COMPILER}/qs-adventure.axf: ${COMPILER}/zip.o
${COMPILER}/qs-adventure.axf: ${ROOT}/usblib/${COMPILER}-cm3/libusb-cm3.a
${COMPILER}/qs-adventure.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/qs-adventure.axf: qs-adventure.ld
SCATTERgcc_qs-adventure=qs-adventure.ld
ENTRY_qs-adventure=ResetISR
CFLAGSgcc=-DTARGET_IS_FIRESTORM_RA2

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
