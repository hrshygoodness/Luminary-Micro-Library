#******************************************************************************
#
# Makefile - Rules for building the board finder utility.
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
# This is part of revision 9453 of the Stellaris Firmware Development Package.
#
#******************************************************************************

#
# The name of this application.
#
APP:=finder

#
# The object files that comprise this application.
#
OBJS:=finder.o                                            \
      gui.o                                               \
      ../../third_party/windows/fltk-1.1.10/lib/libfltk.a

#
# The resources for the Windows version.
#
RES:=finder.res

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
LIBS:=ole32 uuid wsock32 iphlpapi comctl32
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
	@rm -rf finder.res gui.cxx gui.h

#
# A dependency to ensure that fltk is built before the application.
#
finder.o: ../../third_party/windows/fltk-1.1.10/lib/libfltk.a gui.cxx

#
# A rule to generate gui.cxx.
#
gui.cxx: gui.fl ../../third_party/windows/fltk-1.1.10/lib/libfltk.a
	@if [ x${VERBOSE} = x ];                                     \
	 then                                                        \
	     echo "  FLUID gui.fl";                                  \
	 else                                                        \
	     echo "fluid -c gui.fl";                                 \
	 fi;                                                         \
	 ../../third_party/windows/fltk-1.1.10/fluid/fluid -c gui.fl

#
# A dependency to ensure that the Windows resources are rebuilt if the icon
# file is modified.
#
finder.res: finder.ico
