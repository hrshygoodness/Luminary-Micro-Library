# UNIX C makefile for the ZIP Infocom interpreter

CC = cc
CFLAGS = -O
LDFLAGS =
LIBS = -ltermcap

INC = ztypes.h
OBJS = zip.o control.o extern.o fileio.o input.o interpre.o math.o memory.o \
	object.o operand.o osdepend.o property.o screen.o text.o variable.o \
	unixio.o

zip : $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LIBS)

$(OBJS) : $(INC) extern.c

clean :
	-rm *.o zip
