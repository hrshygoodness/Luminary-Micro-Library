#******************************************************************************
#
# Makefile - Rules for building the Blox Game.
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
VPATH+=../../../third_party/blox
VPATH+=../drivers
VPATH+=../../../third_party/lwip-1.3.2/apps/httpserver_raw
VPATH+=../../../third_party/jpeg
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=..
IPATH+=../../..
IPATH+=../../../third_party/lwip-1.3.2/apps
IPATH+=../../../third_party/bget
IPATH+=../../../third_party/blox
IPATH+=../../../third_party/lwip-1.3.2/ports/stellaris/include
IPATH+=../../../third_party/lwip-1.3.2/src/include
IPATH+=../../../third_party/lwip-1.3.2/src/include/ipv4
IPATH+=../../../third_party/jpeg
IPATH+=../../../utils

#
# The default rule, which causes the Blox Game to be built.
#
all: ${COMPILER}
all: ${COMPILER}/qs-blox.axf

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
# Rules for building the Blox Game.
#
${COMPILER}/qs-blox.axf: ${COMPILER}/bget.o
${COMPILER}/qs-blox.axf: ${COMPILER}/blox.o
${COMPILER}/qs-blox.axf: ${COMPILER}/blox_fs.o
${COMPILER}/qs-blox.axf: ${COMPILER}/blox_screen.o
${COMPILER}/qs-blox.axf: ${COMPILER}/blox_web.o
${COMPILER}/qs-blox.axf: ${COMPILER}/cgifuncs.o
${COMPILER}/qs-blox.axf: ${COMPILER}/httpd.o
${COMPILER}/qs-blox.axf: ${COMPILER}/images.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jcomapi.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdapimin.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdapistd.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdcoefct.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdcolor.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jddctmgr.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdhuff.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdinput.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdmainct.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdmarker.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdmaster.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdmerge.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdphuff.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdpostct.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jdsample.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jerrorlmi.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jidctfst.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jidctint.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jidctred.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jmemlmi.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jmemmgr.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jpgwidget.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jquant1.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jramdatasrc.o
${COMPILER}/qs-blox.axf: ${COMPILER}/jutils.o
${COMPILER}/qs-blox.axf: ${COMPILER}/kitronix320x240x16_ssd2119_idm_sbc.o
${COMPILER}/qs-blox.axf: ${COMPILER}/locator.o
${COMPILER}/qs-blox.axf: ${COMPILER}/lwiplib.o
${COMPILER}/qs-blox.axf: ${COMPILER}/qs-blox.o
${COMPILER}/qs-blox.axf: ${COMPILER}/random.o
${COMPILER}/qs-blox.axf: ${COMPILER}/sdram.o
${COMPILER}/qs-blox.axf: ${COMPILER}/set_pinout.o
${COMPILER}/qs-blox.axf: ${COMPILER}/shapes.o
${COMPILER}/qs-blox.axf: ${COMPILER}/sound.o
${COMPILER}/qs-blox.axf: ${COMPILER}/sound_effects.o
${COMPILER}/qs-blox.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/qs-blox.axf: ${COMPILER}/swupdate.o
${COMPILER}/qs-blox.axf: ${COMPILER}/touch.o
${COMPILER}/qs-blox.axf: ${COMPILER}/uartstdio.o
${COMPILER}/qs-blox.axf: ${COMPILER}/usb_keyboard.o
${COMPILER}/qs-blox.axf: ${COMPILER}/ustdlib.o
${COMPILER}/qs-blox.axf: ${COMPILER}/wav.o
${COMPILER}/qs-blox.axf: ${COMPILER}/wm8510.o
${COMPILER}/qs-blox.axf: ${ROOT}/usblib/${COMPILER}-cm3/libusb-cm3.a
${COMPILER}/qs-blox.axf: ${ROOT}/grlib/${COMPILER}-cm3/libgr-cm3.a
${COMPILER}/qs-blox.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/qs-blox.axf: qs-blox.ld
SCATTERgcc_qs-blox=qs-blox.ld
ENTRY_qs-blox=ResetISR
CFLAGSgcc=-DINCLUDE_BGET_STATS -DTARGET_IS_TEMPEST_RB1

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
