#******************************************************************************
#
# Makefile - Rules for building the data logger utility.
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
# This is part of revision 8555 of the Stellaris Firmware Development Package.
#
#******************************************************************************

#
# The name of this application.
#
APP:=logger

#
# The object files that comprise this application.
#
OBJS:=logger.o                                            \
      CLoggerUI.o                                         \
      CStripChart.o                                       \
      ../../third_party/windows/fltk-1.1.10/lib/libfltk.a

#
# The resources for the Windows version.
#
RES:=logger.res

#
# Include the generic rules.
#
include ../toolsdefs

#
# Modify some of the generic defines.
#
LD:=${CXX}
CFLAGS:=${CFLAGS} -g -I ../../third_party/windows/fltk-1.1.10
LDFLAGS:=${LDFLAGS} -g
ifneq ($(findstring CYGWIN, ${os}), )
LDFLAGS:=${LDFLAGS} -mwindows
LIBS:=ole32 uuid wsock32 iphlpapi comctl32 setupapi
else
LIBS:=X11 Xpm pthread
endif

#
# Build FLTK.
#
../../third_party/windows/fltk-1.1.10/lib/libfltk.a:
	@make -C .. ../third_party/windows/fltk-1.1.10/lib/libfltk.a

#
# Clean out additional build products.
#
clean::
	@rm -rf logger.res CLoggerUI.cxx CLoggerUI.h

#
# A dependency to ensure that fltk is built before the application.
#
logger.o: ../../third_party/windows/fltk-1.1.10/lib/libfltk.a CLoggerUI.cxx

#
# A rule to generate CLoggerUI.cxx.
#
CLoggerUI.cxx: CLoggerUI.fl ../../third_party/windows/fltk-1.1.10/lib/libfltk.a
	@if [ x${VERBOSE} = x ];                                     \
	 then                                                        \
	     echo "  FLUID CLoggerUI.fl";                            \
	 else                                                        \
	     echo "fluid -c CLoggerUI.fl";                           \
	 fi;                                                         \
	 ../../third_party/windows/fltk-1.1.10/fluid/fluid -c CLoggerUI.fl

#
# A rule to generate CLoggerUI.h.
#
CLoggerUI.h: CLoggerUI.fl ../../third_party/windows/fltk-1.1.10/lib/libfltk.a
	@if [ x${VERBOSE} = x ];                                     \
	 then                                                        \
	     echo "  FLUID CLoggerUI.fl";                            \
	 else                                                        \
	     echo "fluid -c CLoggerUI.fl";                           \
	 fi;                                                         \
	 ../../third_party/windows/fltk-1.1.10/fluid/fluid -c CLoggerUI.fl

#
# A dependency to ensure that the Windows resources are rebuilt if the icon
# file is modified.
#
logger.res: logger.ico
