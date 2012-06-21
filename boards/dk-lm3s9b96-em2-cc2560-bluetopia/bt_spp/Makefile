#******************************************************************************
#
# Makefile - Rules for building the SPP demo for Bluetopia Bluetooth stack.
#
# Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 8555 of the DK-LM3S9B96-EM2-CC2560-BLUETOPIA Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S9B96

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
VPATH=../../../Bluetopia/btpsvend
VPATH+=../../dk-lm3s9b96/drivers
VPATH+=../../../Bluetopia/btpskrnl/noos
VPATH+=../../../Bluetopia/hcitrans/noos
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=../../..
IPATH+=../../../Bluetopia/btpsvend
IPATH+=../../dk-lm3s9b96
IPATH+=../../../Bluetopia/include
IPATH+=../../../Bluetopia/btpskrnl/noos
IPATH+=../../../Bluetopia/hcitrans/noos

#
# The default rule, which causes the SPP demo for Bluetopia Bluetooth stack to be built.
#
all: ${COMPILER}
all: ${COMPILER}/bt_spp.axf

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
# Rules for building the SPP demo for Bluetopia Bluetooth stack.
#
${COMPILER}/bt_spp.axf: ${COMPILER}/BTPSKRNL.o
${COMPILER}/bt_spp.axf: ${COMPILER}/BTPSVEND.o
${COMPILER}/bt_spp.axf: ${COMPILER}/HCITRANS.o
${COMPILER}/bt_spp.axf: ${COMPILER}/bluetooth.o
${COMPILER}/bt_spp.axf: ${COMPILER}/bt_spp.o
${COMPILER}/bt_spp.axf: ${COMPILER}/graphics.o
${COMPILER}/bt_spp.axf: ${COMPILER}/images.o
${COMPILER}/bt_spp.axf: ${COMPILER}/kitronix320x240x16_ssd2119_8bit.o
${COMPILER}/bt_spp.axf: ${COMPILER}/set_pinout.o
${COMPILER}/bt_spp.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/bt_spp.axf: ${COMPILER}/touch.o
${COMPILER}/bt_spp.axf: ${COMPILER}/ustdlib.o
${COMPILER}/bt_spp.axf: ${COMPILER}/widgets.o
${COMPILER}/bt_spp.axf: ${ROOT}/Bluetopia/${COMPILER}-cm3/libBluetopiaNoOS-cm3.a
${COMPILER}/bt_spp.axf: ${ROOT}/grlib/${COMPILER}-cm3/libgr-cm3.a
${COMPILER}/bt_spp.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/bt_spp.axf: bt_spp.ld
SCATTERgcc_bt_spp=bt_spp.ld
ENTRY_bt_spp=ResetISR
CFLAGSgcc=-DTARGET_IS_TEMPEST_RB1 -D__NEWLIB__ -DUART_BUFFERED -DDEBUG_ENABLED -DBTPS_KERNEL_NON_THREADED -DDEBUG_ZONES=DBG_ZONE_ANY

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
