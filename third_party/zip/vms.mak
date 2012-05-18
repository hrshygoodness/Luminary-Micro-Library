# VMS makefile for the ZIP Infocom interpreter

# For VAX C:
#CC = cc
#LIBS = ,sys$library:vaxcrtl/libr

# For DEC C (VAX or Alpha)
CC = cc/decc
LIBS =

CFLAGS = /defi=vms
LD = link
LDFLAGS = /exec=$(MMS$TARGET_NAME)

INC = ztypes.h
OBJS = zip.obj,control.obj,extern.obj,fileio.obj,input.obj,interpre.obj,\
	math.obj,memory.obj,object.obj,operand.obj,osdepend.obj,property.obj,\
	screen.obj,text.obj,variable.obj,getopt.obj,smgio.obj

zip.exe : $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS)

$(OBJS) : $(INC) extern.c

clean :
	- delete *.obj.,zip.exe.
