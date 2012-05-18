#******************************************************************************
#
# Makefile - Rules for building the utilities.
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
# This is part of revision 8555 of the Stellaris Firmware Development Package.
#
#******************************************************************************

#
# The directories that should be built.
#
DIRS=aes_gen_key \
     bdc-comm    \
     converter   \
     dfuwrap     \
     eflash      \
     finder      \
     ftrasterize \
     logger      \
     makefsfile  \
     pnmtoc      \
     sflash

#
# The default rule, which causes the above directories to be recursively built.
#
all:
	@for i in ${DIRS};                  \
	 do                                 \
	     if [ -d $${i} ];               \
	     then                           \
	         make -C $${i} || exit $$?; \
	     fi;                            \
	 done

#
# The rule to clean out all the build products.
#
clean:
	@for i in ${DIRS};              \
	 do                             \
	     if [ -d $${i} ];           \
	     then                       \
	         make -C $${i} clean;   \
	     fi;                        \
	 done;                          \
	 rm -f ${wildcard *~} __dummy__

#
# Build FLTK.
#
../third_party/windows/fltk-1.1.10/lib/libfltk.a:
	@cd ../third_party/windows/fltk-1.1.10;           \
	 if [ ! -f config.h ];                            \
	 then                                             \
	     export CFLAGS="-g0 -O2";                     \
	     export CPPFLAGS="-g0 -O2";                   \
	     export CXXFLAGS="-g0 -O2";                   \
	     CONFIG_FLAGS=--enable-localjpeg;             \
	     CONFIG_FLAGS+=' --enable-localzlib';         \
	     CONFIG_FLAGS+=' --enable-localpng';          \
	     CONFIG_FLAGS+=' --enable-threads';           \
	     CONFIG_FLAGS+=' --disable-gl';               \
	     if [ x${VERBOSE} = x ];                      \
	     then                                         \
	         echo "  CONFIGURE";                      \
	         ./configure $${CONFIG_FLAGS} > log 2>&1; \
	     else                                         \
	         echo ./configure $${CONFIG_FLAGS};       \
	         ./configure $${CONFIG_FLAGS};            \
	     fi;                                          \
	 fi;                                              \
	 if [ x${VERBOSE} = x ];                          \
	 then                                             \
	     echo "  MAKE";                               \
	     make >> log 2>&1;                            \
	 else                                             \
	     echo make;                                   \
	     make;                                        \
	 fi;                                              \
