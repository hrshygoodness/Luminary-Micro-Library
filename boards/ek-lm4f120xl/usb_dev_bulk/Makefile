#******************************************************************************
#
# Makefile - Rules for building the USB device bulk example.
#
# Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM4F120H5QR

#
# Set the processor variant.
#
VARIANT=cm4f

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
VPATH=
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=../../..

#
# The default rule, which causes the USB device bulk example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/usb_dev_bulk.axf

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
# Rules for building the USB device bulk example.
#
${COMPILER}/usb_dev_bulk.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/usb_dev_bulk.axf: ${COMPILER}/uartstdio.o
${COMPILER}/usb_dev_bulk.axf: ${COMPILER}/usb_bulk_structs.o
${COMPILER}/usb_dev_bulk.axf: ${COMPILER}/usb_dev_bulk.o
${COMPILER}/usb_dev_bulk.axf: ${COMPILER}/ustdlib.o
${COMPILER}/usb_dev_bulk.axf: ${ROOT}/usblib/${COMPILER}-cm4f/libusb-cm4f.a
${COMPILER}/usb_dev_bulk.axf: ${ROOT}/driverlib/${COMPILER}-cm4f/libdriver-cm4f.a
${COMPILER}/usb_dev_bulk.axf: usb_dev_bulk.ld
SCATTERgcc_usb_dev_bulk=usb_dev_bulk.ld
ENTRY_usb_dev_bulk=ResetISR
CFLAGSgcc=-DTARGET_IS_BLIZZARD_RA1 -DUART_BUFFERED

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
