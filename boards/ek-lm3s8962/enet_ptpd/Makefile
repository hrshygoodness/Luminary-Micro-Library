#******************************************************************************
#
# Makefile - Rules for building the Sample IEEE-1588 Application using Open Source PTPd with lwIP 1.3.2.
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
# This is part of revision 10636 of the EK-LM3S8962 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S8962

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
VPATH=../../../third_party/ptpd-1rc1/src/dep-lmi
VPATH+=../drivers
VPATH+=../../../third_party/lwip-1.3.2/apps/httpserver_raw
VPATH+=../../../third_party/ptpd-1rc1/src
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
# The default rule, which causes the Sample IEEE-1588 Application using Open Source PTPd with lwIP 1.3.2 to be built.
#
all: ${COMPILER}
all: ${COMPILER}/enet_ptpd.axf

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
# Rules for building the Sample IEEE-1588 Application using Open Source PTPd with lwIP 1.3.2.
#
${COMPILER}/enet_ptpd.axf: ${COMPILER}/enet_ptpd.o
${COMPILER}/enet_ptpd.axf: ${COMPILER}/httpd.o
${COMPILER}/enet_ptpd.axf: ${COMPILER}/lmi_fs.o
${COMPILER}/enet_ptpd.axf: ${COMPILER}/locator.o
${COMPILER}/enet_ptpd.axf: ${COMPILER}/lwiplib.o
${COMPILER}/enet_ptpd.axf: ${COMPILER}/ptpdlib.o
${COMPILER}/enet_ptpd.axf: ${COMPILER}/random.o
${COMPILER}/enet_ptpd.axf: ${COMPILER}/rit128x96x4.o
${COMPILER}/enet_ptpd.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/enet_ptpd.axf: ${COMPILER}/uartstdio.o
${COMPILER}/enet_ptpd.axf: ${COMPILER}/ustdlib.o
${COMPILER}/enet_ptpd.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/enet_ptpd.axf: enet_ptpd.ld
SCATTERgcc_enet_ptpd=enet_ptpd.ld
ENTRY_enet_ptpd=ResetISR

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
