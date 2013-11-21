#******************************************************************************
#
# Makefile - Rules for building the Sample Ethernet Application using lwIP 1.3.2.
#
# Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 10636 of the EK-LM3S6965 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S6965

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
VPATH+=../../../third_party/lwip-1.3.2/apps/httpserver_raw
VPATH+=../../../third_party/fatfs/port
VPATH+=../../../third_party/fatfs/src
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=..
IPATH+=../../..
IPATH+=../../../third_party/lwip-1.3.2/apps
IPATH+=../../../third_party/lwip-1.3.2/ports/stellaris/include
IPATH+=../../../third_party/lwip-1.3.2/src/include
IPATH+=../../../third_party/lwip-1.3.2/src/include/ipv4
IPATH+=../../../third_party

#
# The default rule, which causes the Sample Ethernet Application using lwIP 1.3.2 to be built.
#
all: ${COMPILER}
all: ${COMPILER}/enet_lwip.axf

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
# Rules for building the Sample Ethernet Application using lwIP 1.3.2.
#
${COMPILER}/enet_lwip.axf: ${COMPILER}/enet_lwip.o
${COMPILER}/enet_lwip.axf: ${COMPILER}/ff.o
${COMPILER}/enet_lwip.axf: ${COMPILER}/httpd.o
${COMPILER}/enet_lwip.axf: ${COMPILER}/lmi_fs.o
${COMPILER}/enet_lwip.axf: ${COMPILER}/locator.o
${COMPILER}/enet_lwip.axf: ${COMPILER}/lwiplib.o
${COMPILER}/enet_lwip.axf: ${COMPILER}/mmc-ek-lm3s6965.o
${COMPILER}/enet_lwip.axf: ${COMPILER}/rit128x96x4.o
${COMPILER}/enet_lwip.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/enet_lwip.axf: ${COMPILER}/uartstdio.o
${COMPILER}/enet_lwip.axf: ${COMPILER}/ustdlib.o
${COMPILER}/enet_lwip.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/enet_lwip.axf: enet_lwip.ld
SCATTERgcc_enet_lwip=enet_lwip.ld
ENTRY_enet_lwip=ResetISR

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
