#******************************************************************************
#
# Makefile - Rules for building the font rasterization utility.
#
# Copyright (c) 2007-2010 Texas Instruments Incorporated.  All rights reserved.
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
# This is part of revision 6594 of the Stellaris Firmware Development Package.
#
#******************************************************************************

#
# The name of this application.
#
APP:=ftrasterize

#
# The object files that comprise this application.
#
OBJS:=ftrasterize.o                           \
      freetype-2.3.5/objs/.libs/libfreetype.a

#
# Include the generic rules.
#
include ../toolsdefs

#
# Add a -I for freetype to the CFLAGS.
#
CFLAGS:=${CFLAGS} -I freetype-2.3.5/include

#
# Fetch the tarball that contains FreeType.
#
freetype-2.3.5.tar.bz2:
	@URL=http://download.savannah.gnu.org/releases/freetype; \
	 URL=$${URL}/freetype-2.3.5.tar.bz2;                     \
	 if [ x${VERBOSE} = x ];                                 \
	 then                                                    \
	     echo "  WGET  `basename $${URL}`";                  \
	 else                                                    \
	     echo wget $${URL};                                  \
	 fi;                                                     \
	 wget $${URL}

#
# Unpack and build FreeType.
#
freetype-2.3.5/objs/.libs/libfreetype.a: freetype-2.3.5.tar.bz2
	@if [ ! -d freetype-2.3.5 ];                             \
	 then                                                    \
	     if [ x${VERBOSE} = x ];                             \
	     then                                                \
	         echo "  TAR   ${^}";                            \
	     else                                                \
	         echo tar -xjf ${^};                             \
	     fi;                                                 \
	     tar -xjf freetype-2.3.5.tar.bz2;                    \
	 fi;                                                     \
	 cd freetype-2.3.5;                                      \
	 if [ ! -f builds/unix/ftconfig.h ];                     \
	 then                                                    \
	     if [ x${VERBOSE} = x ];                             \
	     then                                                \
	         echo "  CONFIGURE";                             \
	     else                                                \
	         echo ./configure;                               \
	     fi;                                                 \
	     ./configure CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" \
	                 --with-zlib=no > log 2>&1;              \
	 fi;                                                     \
	 if [ x${VERBOSE} = x ];                                 \
	 then                                                    \
	     echo "  MAKE";                                      \
	 else                                                    \
	     echo make;                                          \
	 fi;                                                     \
	 make >> log 2>&1

#
# Fetch the tarball that contains the Computer Modern fonts.
#
amsfonts.zip:
	@URL=ftp://ftp.ams.org/pub/tex/amsfonts.zip; \
	 if [ x${VERBOSE} = x ];                     \
	 then                                        \
	     echo "  WGET  `basename $${URL}`";      \
	 else                                        \
	     echo "wget $${URL}";                    \
	 fi;                                         \
	 wget $${URL}

#
# Unpack the Computer Modern fonts.
#
amsfonts: amsfonts.zip
	@unzip -q amsfonts.zip -d amsfonts

#
# The rule to generate the Computer Modern compressed bitmap fonts.
#
fonts: ftrasterize${EXT} amsfonts
	@FONTS=amsfonts/fonts/type1/public/amsfonts/cm;                     \
	 for i in 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48; \
	 do                                                                 \
	     echo "  GEN   $${i} point";                                    \
	     ./ftrasterize -f cm -s $${i} $${FONTS}/cmr17.pfb;              \
	     ./ftrasterize -f cm -s $${i} -b $${FONTS}/cmb10.pfb;           \
	     ./ftrasterize -f cm -s $${i} -i $${FONTS}/cmti12.pfb;          \
	     ./ftrasterize -f cmss -s $${i} $${FONTS}/cmss17.pfb;           \
	     ./ftrasterize -f cmss -s $${i} -b $${FONTS}/cmssbx10.pfb;      \
	     ./ftrasterize -f cmss -s $${i} -i $${FONTS}/cmssi17.pfb;       \
	     ./ftrasterize -f cmsc -s $${i} $${FONTS}/cmcsc10.pfb;          \
	     ./ftrasterize -f cmtt -m -s $${i} $${FONTS}/cmtt10.pfb;        \
	 done

#
# Clean out additional build products.
#
clean::
	@rm -rf freetype-2.3.5 amsfonts ${wildcard font*.c}

#
# A dependency to ensure that freetype is built before the ftrasterize
# application.
#
ftrasterize.o: freetype-2.3.5/objs/.libs/libfreetype.a
