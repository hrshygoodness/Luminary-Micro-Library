# Microsoft Quick C makefile for the ZIP Infocom interpreter

CC = qcl
CFLAGS = /AL /Ox /DNDEBUG
LD = qlink
LDFLAGS = /NOI /ST:0x1000
LIBS =

INC = ztypes.h
OBJS = zip.obj control.obj extern.obj fileio.obj input.obj interpre.obj \
	math.obj memory.obj object.obj operand.obj osdepend.obj property.obj \
	screen.obj text.obj variable.obj getopt.obj mscio.obj

zip.exe : $(OBJS)
	$(LD) $(LDFLAGS) @<<
$(OBJS)
$@
NUL
$(LIBS);
<<

$(OBJS) : $(INC) extern.c

