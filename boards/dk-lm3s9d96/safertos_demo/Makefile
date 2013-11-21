#******************************************************************************
#
# Makefile - Rules for building the SafeRTOS example.
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
# This is part of revision 10636 of the DK-LM3S9D96 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S9D96

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
# The default rule, which causes the SafeRTOS example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/safertos_demo.axf

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
# Rules for building the SafeRTOS example.
#
${COMPILER}/safertos_demo.axf: ${COMPILER}/cgifuncs.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/display_task.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/fs.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/httpd.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/idle_task.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/images.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/kitronix320x240x16_ssd2119_8bit.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/led_task.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/locator.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/lwip_task.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/lwiplib.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/random.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/safertos_demo.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/set_pinout.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/spider_task.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/touch.o
${COMPILER}/safertos_demo.axf: ${COMPILER}/ustdlib.o
${COMPILER}/safertos_demo.axf: ${ROOT}/grlib/${COMPILER}-cm3/libgr-cm3.a
${COMPILER}/safertos_demo.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/safertos_demo.axf: safertos_demo.ld
SCATTERgcc_safertos_demo=safertos_demo.ld
ENTRY_safertos_demo=ResetISR
CFLAGSgcc=-DTARGET_IS_FIRESTORM_RA2

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
