#******************************************************************************
#
# Makefile - Rules for building the USB host mass storage example.
#
# Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 9453 of the RDK-IDM-SBC Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S9B92

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
VPATH=../../../third_party/bget
VPATH+=../drivers
VPATH+=../../../third_party/fatfs/port
VPATH+=../../../third_party/fatfs/src
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=..
IPATH+=../../..
IPATH+=../../../third_party/bget
IPATH+=../../../third_party/lwip-1.3.2/ports/stellaris/include
IPATH+=../../../third_party/lwip-1.3.2/src/include
IPATH+=../../../third_party/lwip-1.3.2/src/include/ipv4
IPATH+=../../../third_party
IPATH+=../../../utils

#
# The default rule, which causes the USB host mass storage example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/usb_host_msc.axf

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
# Rules for building the USB host mass storage example.
#
${COMPILER}/usb_host_msc.axf: ${COMPILER}/cmdline.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/fat_usbmsc.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/ff.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/kitronix320x240x16_ssd2119_idm_sbc.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/locator.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/lwiplib.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/set_pinout.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/swupdate.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/touch.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/uartstdio.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/usb_host_msc.o
${COMPILER}/usb_host_msc.axf: ${COMPILER}/ustdlib.o
${COMPILER}/usb_host_msc.axf: ${ROOT}/usblib/${COMPILER}-cm3/libusb-cm3.a
${COMPILER}/usb_host_msc.axf: ${ROOT}/grlib/${COMPILER}-cm3/libgr-cm3.a
${COMPILER}/usb_host_msc.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/usb_host_msc.axf: usb_host_msc.ld
SCATTERgcc_usb_host_msc=usb_host_msc.ld
ENTRY_usb_host_msc=ResetISR
CFLAGSgcc=-DUART_BUFFERED -DTARGET_IS_TEMPEST_RB1

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
