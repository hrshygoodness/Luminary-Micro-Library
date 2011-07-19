#******************************************************************************
#
# Makefile - Rules for building the Quickstart Example Application.
#
# Copyright (c) 2009-2010 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 6594 of the DK-LM3S9B96 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S9B96

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
VPATH+=../../../third_party/lwip-1.3.2/apps/httpserver_raw
VPATH+=../../../third_party/jpeg
VPATH+=../../../third_party/fatfs/port
VPATH+=../../../third_party/fatfs/src
VPATH+=../../../utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=.
IPATH+=..
IPATH+=../../..
IPATH+=../../../third_party/lwip-1.3.2/apps
IPATH+=../../../third_party/bget
IPATH+=../../../third_party/lwip-1.3.2/ports/stellaris/include
IPATH+=../../../third_party/lwip-1.3.2/src/include
IPATH+=../../../third_party/lwip-1.3.2/src/include/ipv4
IPATH+=../../../third_party/jpeg
IPATH+=../../../third_party

#
# The default rule, which causes the Quickstart Example Application to be built.
#
all: ${COMPILER}
all: ${COMPILER}/qs-checkout.axf

#
# The rule to clean out all the build products.
#
clean:
	@rm -rf ${COMPILER} ${wildcard *~}

#
# The rule to create the target directory.
#
${COMPILER}:
	@mkdir ${COMPILER}

#
# Rules for building the Quickstart Example Application.
#
${COMPILER}/qs-checkout.axf: ${COMPILER}/audioplay.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/bget.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/cmdline.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/dual-disk-driver.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/extflash.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/extram.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/ff.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/file.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/fswrapper.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/grlib_demo.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/gui_widgets.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/httpd.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/images.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/imageview.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jcomapi.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdapimin.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdapistd.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdcoefct.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdcolor.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jddctmgr.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdhuff.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdinput.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdmainct.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdmarker.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdmaster.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdmerge.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdphuff.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdpostct.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jdsample.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jerrorlmi.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jidctfst.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jidctint.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jidctred.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jmemlmi.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jmemmgr.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jpgwidget.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jquant1.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jramdatasrc.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/jutils.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/kitronix320x240x16_ssd2119_8bit.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/locator.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/lwiplib.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/qs-checkout.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/set_pinout.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/sound.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/ssiflash.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/tftp.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/tftp_qs.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/thumbwheel.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/tlv320aic23b.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/touch.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/uartstdio.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/usb_funcs.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/usb_mouse_structs.o
${COMPILER}/qs-checkout.axf: ${COMPILER}/ustdlib.o
${COMPILER}/qs-checkout.axf: ${ROOT}/usblib/${COMPILER}/libusb.a
${COMPILER}/qs-checkout.axf: ${ROOT}/grlib/${COMPILER}/libgr.a
${COMPILER}/qs-checkout.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/qs-checkout.axf: qs-checkout.ld
SCATTERgcc_qs-checkout=qs-checkout.ld
ENTRY_qs-checkout=ResetISR
CFLAGSgcc=-DUART_BUFFERED -DDISK0_DK_LM3S9B96 -DDISK1_USB_MSC -DINCLUDE_BGET_STATS

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
