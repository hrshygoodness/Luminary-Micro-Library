#******************************************************************************
#
# Makefile - Rules for building the USB device mouse example.
#
# Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 10636 of the EK-LM3S3748 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S3748

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
IPATH=..
IPATH+=../../..

#
# The default rule, which causes the USB device mouse example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/usb_dev_mouse.axf

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
# Rules for building the USB device mouse example.
#
${COMPILER}/usb_dev_mouse.axf: ${COMPILER}/buttons.o
${COMPILER}/usb_dev_mouse.axf: ${COMPILER}/formike128x128x16.o
${COMPILER}/usb_dev_mouse.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/usb_dev_mouse.axf: ${COMPILER}/uartstdio.o
${COMPILER}/usb_dev_mouse.axf: ${COMPILER}/usb_dev_mouse.o
${COMPILER}/usb_dev_mouse.axf: ${COMPILER}/usb_mouse_structs.o
${COMPILER}/usb_dev_mouse.axf: ${ROOT}/usblib/${COMPILER}-cm3/libusb-cm3.a
${COMPILER}/usb_dev_mouse.axf: ${ROOT}/grlib/${COMPILER}-cm3/libgr-cm3.a
${COMPILER}/usb_dev_mouse.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/usb_dev_mouse.axf: usb_dev_mouse.ld
SCATTERgcc_usb_dev_mouse=usb_dev_mouse.ld
ENTRY_usb_dev_mouse=ResetISR
CFLAGSgcc=-DUART_BUFFERED -DTARGET_IS_DUSTDEVIL_RA0

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
