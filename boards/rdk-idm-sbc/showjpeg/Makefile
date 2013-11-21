#******************************************************************************
#
# Makefile - Rules for building the JPEG Decompression and Display Application.
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
# This is part of revision 10636 of the RDK-IDM-SBC Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S9B92

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
VPATH+=../../../third_party/jpeg
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
IPATH+=../../../third_party/jpeg
IPATH+=../../../third_party

#
# The default rule, which causes the JPEG Decompression and Display Application to be built.
#
all: ${COMPILER}
all: ${COMPILER}/showjpeg.axf

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
# Rules for building the JPEG Decompression and Display Application.
#
${COMPILER}/showjpeg.axf: ${COMPILER}/bget.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jcomapi.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdapimin.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdapistd.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdcoefct.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdcolor.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jddctmgr.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdhuff.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdinput.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdmainct.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdmarker.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdmaster.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdmerge.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdphuff.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdpostct.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jdsample.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jerrorlmi.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jidctfst.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jidctint.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jidctred.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jmemlmi.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jmemmgr.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jpgwidget.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jquant1.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jramdatasrc.o
${COMPILER}/showjpeg.axf: ${COMPILER}/jutils.o
${COMPILER}/showjpeg.axf: ${COMPILER}/kitronix320x240x16_ssd2119_idm_sbc.o
${COMPILER}/showjpeg.axf: ${COMPILER}/locator.o
${COMPILER}/showjpeg.axf: ${COMPILER}/lwiplib.o
${COMPILER}/showjpeg.axf: ${COMPILER}/sdram.o
${COMPILER}/showjpeg.axf: ${COMPILER}/set_pinout.o
${COMPILER}/showjpeg.axf: ${COMPILER}/showjpeg.o
${COMPILER}/showjpeg.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/showjpeg.axf: ${COMPILER}/swupdate.o
${COMPILER}/showjpeg.axf: ${COMPILER}/touch.o
${COMPILER}/showjpeg.axf: ${COMPILER}/uartstdio.o
${COMPILER}/showjpeg.axf: ${COMPILER}/ustdlib.o
${COMPILER}/showjpeg.axf: ${ROOT}/grlib/${COMPILER}-cm3/libgr-cm3.a
${COMPILER}/showjpeg.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/showjpeg.axf: showjpeg.ld
SCATTERgcc_showjpeg=showjpeg.ld
ENTRY_showjpeg=ResetISR
CFLAGSgcc=-DINCLUDE_BGET_STATS -DTARGET_IS_TEMPEST_RB1

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
