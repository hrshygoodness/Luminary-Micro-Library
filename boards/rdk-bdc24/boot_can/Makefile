#******************************************************************************
#
# Makefile - Rules for building the CAN Boot Loader.
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
VPATH=../../../boot_loader

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=../../..
IPATH+=../../../boot_loader

#
# The default rule, which causes the CAN Boot Loader to be built.
#
all: ${COMPILER}
all: ${COMPILER}/boot_can.axf

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
# Rules for building the CAN Boot Loader.
#
${COMPILER}/boot_can.axf: ${COMPILER}/bl_autobaud.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_can.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_check.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_decrypt.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_enet.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_flash.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_i2c.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_main.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_packet.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_ssi.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_startup_${COMPILER}.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_uart.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_usb.o
${COMPILER}/boot_can.axf: ${COMPILER}/bl_usbfuncs.o
${COMPILER}/boot_can.axf: ../../../boot_loader/bl_link.ld
SCATTERgcc_boot_can=../../../boot_loader/bl_link.ld
ENTRY_boot_can=ResetISR
CFLAGSgcc=-DTARGET_IS_DUSTDEVIL_RA0

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
