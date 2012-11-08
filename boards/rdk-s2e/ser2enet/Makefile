#******************************************************************************
#
# Makefile - Rules for building the Serial to Ethernet Converter.
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
# This is part of revision 9453 of the RDK-S2E Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S6432

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
VPATH=../../../third_party/lwip-1.3.2/apps/httpserver_raw
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=../../..
IPATH+=../../../third_party/lwip-1.3.2/apps
IPATH+=../../../third_party/lwip-1.3.2/ports/stellaris/include
IPATH+=../../../third_party/lwip-1.3.2/src/include
IPATH+=../../../third_party/lwip-1.3.2/src/include/ipv4

#
# The default rule, which causes the Serial to Ethernet Converter to be built.
#
all: ${COMPILER}
all: ${COMPILER}/ser2enet.axf

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
# Rules for building the Serial to Ethernet Converter.
#
${COMPILER}/ser2enet.axf: ${COMPILER}/config.o
${COMPILER}/ser2enet.axf: ${COMPILER}/flash_pb.o
${COMPILER}/ser2enet.axf: ${COMPILER}/fs_s2e.o
${COMPILER}/ser2enet.axf: ${COMPILER}/httpd.o
${COMPILER}/ser2enet.axf: ${COMPILER}/locator.o
${COMPILER}/ser2enet.axf: ${COMPILER}/lwiplib.o
${COMPILER}/ser2enet.axf: ${COMPILER}/ringbuf.o
${COMPILER}/ser2enet.axf: ${COMPILER}/ser2enet.o
${COMPILER}/ser2enet.axf: ${COMPILER}/serial.o
${COMPILER}/ser2enet.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/ser2enet.axf: ${COMPILER}/swupdate.o
${COMPILER}/ser2enet.axf: ${COMPILER}/telnet.o
${COMPILER}/ser2enet.axf: ${COMPILER}/uartstdio.o
${COMPILER}/ser2enet.axf: ${COMPILER}/upnp.o
${COMPILER}/ser2enet.axf: ${COMPILER}/ustdlib.o
${COMPILER}/ser2enet.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/ser2enet.axf: ser2enet.ld
SCATTERgcc_ser2enet=ser2enet.ld
ENTRY_ser2enet=ResetISR

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
