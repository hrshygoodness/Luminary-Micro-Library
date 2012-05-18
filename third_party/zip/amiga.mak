# Amiga Aztec-C 5.2 makefile for the ZIP Infocom interpreter

CC = ln
CFLAGS = -ms -so -wlc
LDFLAGS =
LIBS = -lc

INC = ztypes.h
OBJS = zip.o control.o extern.o fileio.o input.o interpre.o math.o memory.o \
	object.o operand.o property.o screen.o text.o variable.o getopt.o \
	amigaio.o

zip : $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LIBS)

$(OBJS) : $(INC) extern.c

