#******************************************************************************
#
# Makefile - Rules for building the RDK-IDM-SBC Board Checkout Example Application.
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
# The default rule, which causes the RDK-IDM-SBC Board Checkout Example Application to be built.
#
all: ${COMPILER}
all: ${COMPILER}/idm-checkout.axf

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
# Rules for building the RDK-IDM-SBC Board Checkout Example Application.
#
${COMPILER}/idm-checkout.axf: ${COMPILER}/audioplay.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/bget.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/cmdline.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/dual-disk-driver.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/ff.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/file.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/fswrapper.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/grlib_demo.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/gui_widgets.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/httpd.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/idm-checkout.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/images.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/imageview.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jcomapi.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdapimin.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdapistd.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdcoefct.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdcolor.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jddctmgr.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdhuff.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdinput.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdmainct.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdmarker.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdmaster.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdmerge.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdphuff.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdpostct.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jdsample.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jerrorlmi.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jidctfst.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jidctint.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jidctred.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jmemlmi.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jmemmgr.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jpgwidget.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jquant1.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jramdatasrc.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/jutils.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/kitronix320x240x16_ssd2119_idm_sbc.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/locator.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/lwiplib.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/sdram.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/set_pinout.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/sound.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/ssiflash.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/swupdate.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/tftp.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/touch.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/uartstdio.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/usb_mouse.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/ustdlib.o
${COMPILER}/idm-checkout.axf: ${COMPILER}/wm8510.o
${COMPILER}/idm-checkout.axf: ${ROOT}/usblib/${COMPILER}-cm3/libusb-cm3.a
${COMPILER}/idm-checkout.axf: ${ROOT}/grlib/${COMPILER}-cm3/libgr-cm3.a
${COMPILER}/idm-checkout.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/idm-checkout.axf: idm-checkout.ld
SCATTERgcc_idm-checkout=idm-checkout.ld
ENTRY_idm-checkout=ResetISR
CFLAGSgcc=-DUART_BUFFERED -DDISK0_RDK_IDM_SBC -DDISK1_USB_MSC -DINCLUDE_BGET_STATS -DTARGET_IS_TEMPEST_RB1

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
